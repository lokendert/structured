//
// stereo_feature_finder_test.cpp
//
// A program to test the feature finding classes by running through a set of
// images loaded from a contents file.
//
// Each line of the contents file should have the following format:
//    <timestamp> <left_image_name> <right_image_name>
//
// Ian Mahon
// 06-03-05
//

#include <cmath>
#include <iostream>
#include <iomanip>
#include <fstream>

#include <sys/time.h>
#include <time.h>

#include "cv.h"
#include "highgui.h"
#include "auv_image_distortion.hpp"
#include "auv_stereo_geometry.hpp"

#include "auv_stereo_corner_finder.hpp"
#include "auv_stereo_ncc_corner_finder.hpp"
#include "auv_stereo_keypoint_finder.hpp"
#include "adt_raw_file.hpp"
#include "OSGExport.h"
#include  <boost/thread/xtime.hpp> 
#include "auv_mesh_utils.hpp"
#include "auv_mesh_io.hpp"

using namespace std;
using namespace libplankton;
using namespace libsnapper;


//
// Command-line arguments
//
static string stereo_config_file_name;
static string contents_file_name;
static string dir_name;

static bool no_simp=false;

static bool have_max_mesh_count = false;
static unsigned int max_mesh_count;
static int num_threads=1;
static bool display_debug_images = true;

static bool compress_textures = false;

static int lodNum=3;
static string stereo_calib_file_name;


static std::map<int,std::string> texture_file_names;

static int tex_size=512;


extern std::vector<GtsBBox *> bboxes_all;

//
// Parse command line arguments into global variables
//
static bool parse_args( int argc, char *argv[ ] )
{
  bool have_stereo_config_file_name = false;
  
   
  int i=1;
  while( i < argc )
    {
      if( strcmp( argv[i], "-r" ) == 0 )
	{
	  if( i == argc-1 ) return false;
	  tex_size = atoi( argv[i+1] );
	  i+=2;
	}
      else if( strcmp( argv[i], "-f" ) == 0 )
	{
	  if( i == argc-1 ) return false;
	  dir_name=string( argv[i+1]) ;
	  i+=2;
	}
      else if( strcmp( argv[i], "--nosimp" ) == 0 )
	{
	  no_simp=true;
	  i+=1;
	}
      else if( strcmp( argv[i], "--compress-tex" ) == 0 )
	{
	  compress_textures=true;
	  i+=1;
	}
      else if( strcmp( argv[i], "-n" ) == 0 )
	{
	  if( i == argc-1 ) return false;
	  have_max_mesh_count = true;
	  max_mesh_count = atoi( argv[i+1] );
	  i+=2;
	}
      else if( strcmp( argv[i], "-t" ) == 0 )
      {
         if( i == argc-1 ) return false;
         num_threads = atoi( argv[i+1] );
	 i+=2;
      }
      else if( !have_stereo_config_file_name )
	{
	  stereo_config_file_name = argv[i];
	  have_stereo_config_file_name = true;
	  i++;
	}
   
      else
	{
	  cerr << "Error - unknown parameter: " << argv[i] << endl;
	  return false;
	}
    }




  return ( have_stereo_config_file_name);
}
osg::Node *create_paged_lod(osg::Node * model,vector<string> lod_file_names){

  float cut_off_distance = 25.0f;
    float max_visible_distance = 125.0f;
    float max_dist=1e7;

  const osg::BoundingSphere& bs = model->getBound();
  if (bs.valid()){

    printf("%s dist: %g - %g\n\t%s dist: %g - %g\n\t%s dist: %g - %g\n",lod_file_names[0].c_str(),max_visible_distance,max_dist,lod_file_names[1].c_str(),cut_off_distance,max_visible_distance,lod_file_names[2].c_str(),0.0,cut_off_distance);  
    
    osg::PagedLOD* pagedlod = new osg::PagedLOD;

    pagedlod->setDatabasePath("");
    pagedlod->setCenter(bs.center());
    pagedlod->setRadius(bs.radius());
    pagedlod->setNumChildrenThatCannotBeExpired(2);
    
    pagedlod->setRange(0,max_visible_distance,max_dist);
    pagedlod->addChild(model);
    
    pagedlod->setRange(1,cut_off_distance,max_visible_distance);
    pagedlod->setFileName(1,lod_file_names[1]);
 
    pagedlod->setRange(2,0.0f,cut_off_distance);
    pagedlod->setFileName(2,lod_file_names[0]);
   
   
    return pagedlod;
  }
  return NULL;
}
void genPagedLod(vector< osg::ref_ptr <osg::Group> > nodes, vector< vector<string> > lodnames){
  osg::Group *total=new osg::Group;

  for(int i=0; i < (int)nodes.size(); i++){
  
    osg::Node *tmp=create_paged_lod(nodes[i].get(),lodnames[i]);
    total->addChild(tmp);
  }
  osgDB::ReaderWriter::WriteResult result = osgDB::Registry::instance()->writeNode(*total,"mesh/final.ive",osgDB::Registry::instance()->getOptions());

 if (result.success())	{
    osg::notify(osg::NOTICE)<<"Data written to '"<<"mesh/final.ive" <<"'."<< std::endl;

     
   
  }
  else if  (result.message().empty()){
    osg::notify(osg::NOTICE)<<"Warning: file write to '"<<"mesh/final.ive" <<"' no supported."<< std::endl;
  }

}
GNode *loadBBox(int num,std::map<int,GtsMatrix *> &gts_trans_map){
  char conf_name[255];
  
  sprintf(conf_name,"mesh-agg/bbox-%08d.txt",num);
  
  FILE *bboxfp = fopen(conf_name,"r");
  int count;
  GNode *bboxTree=NULL;
  GSList * bboxes = NULL;
  if(bboxfp){
    char name[255];
    double x1,x2,y1,y2,z1,z2;
    GtsMatrix *mtmp=gts_matrix_identity(NULL);
    int eof0, eof1,eof2,frame_count=0;
    while (eof0 != EOF && eof1 != EOF && eof2 != EOF){
      
      eof0 = fscanf(bboxfp,"%d %s %lf %lf %lf %lf %lf %lf" ,&count, name,
	     &x1,&y1,&z1,&x2,&y2,&z2);
      
      for(int i=0; i < 4; i++)
	for(int j=0; j < 4; j++)
	 eof1 = fscanf(bboxfp," %lf",&mtmp[i][j]);
     eof2 = fscanf(bboxfp,"\n");
      
     
      
      texture_file_names[count]=(name);
      GtsBBox *bbox= gts_bbox_new(gts_bbox_class(),NULL,x1,y1,z1,x2,y2,z2);
      bbox->bounded=(void *)count;
      bboxes= g_slist_prepend (bboxes,bbox);
      bboxes_all.push_back(bbox);
      gts_trans_map[count]=gts_matrix_inverse(mtmp);
      frame_count++;
    }
    gts_matrix_destroy(mtmp);
    fclose(bboxfp);

    bboxTree=gts_bb_tree_new(bboxes);
    return bboxTree;
  }
    printf("No bbox file bailing...\n");
    return NULL;


}

//
// Display information on how to use this program
//
static void print_usage( void )
{
  cout << "USAGE:" << endl;
  cout << "   stereo_feature_finder_test [OPTIONS] <stereo_cfg> " << endl; 
  cout << endl;
  cout << "OPTIONS:" << endl;
  cout << "   -r <resize_scale>       Resize the images by a scaling factor." << endl;
 
  cout << "   -n <max_frame_count>    Set the maximum number of frames to be processed." << endl;
 cout << "    --compress-tex           Compress Textures" << endl;
  cout << endl;
}

int main( int argc, char *argv[ ] )
{
  //
  // Parse command line arguments
  //
  if( !parse_args( argc, argv ) )
    {
      print_usage( );
      exit( 1 );
    }
#ifdef _MCD_CHECK
  FILE *fpm=fopen("memstat.txt","w");
  _MCD_MemStatLog(fpm);
#endif 
  //
  // Figure out the directory that contains the config file 
  //
  string config_dir_name;
  int slash_pos = stereo_config_file_name.rfind( "/" );
  if( slash_pos != -1 )
    config_dir_name = stereo_config_file_name.substr( 0, slash_pos+1 );


  //
  // Open the config file
  // Ensure the option to display the feature finding debug images is on.
  //
  Config_File *config_file;
  try
    {
      config_file = new Config_File( stereo_config_file_name );
    }
  catch( string error )
    {
      cerr << "ERROR - " << error << endl;
      exit( 1 );
    }
  config_file->set_value( "SKF_SHOW_DEBUG_IMAGES"    , display_debug_images );
  config_file->set_value( "SCF_SHOW_DEBUG_IMAGES"    , display_debug_images );
  config_file->set_value( "NCC_SCF_SHOW_DEBUG_IMAGES", display_debug_images );

  


  //
  // Load the stereo camera calibration 
  //
  Stereo_Calib *calib = NULL;
  bool have_stereo_calib = false;
   
  if( config_file->get_value( "STEREO_CALIB_FILE", stereo_calib_file_name) )
    {
      stereo_calib_file_name = config_dir_name+stereo_calib_file_name;
      try
	{
	  calib = new Stereo_Calib( stereo_calib_file_name );
	}
      catch( string error )
	{
	  cerr << "ERROR - " << error << endl;
	  exit( 1 );
	}
      have_stereo_calib = true;
    }      

 
 
  
  auv_data_tools::makedir("mesh");
  if(num_threads > 1)
    g_thread_init (NULL);
  
  std::map<int,GtsMatrix *> gts_trans_map;
  
  string dicefile("mesh-agg/diced.txt");
  std::vector<string> meshNames;
  std::vector<vector<string > > outNames;
  std::vector<osg::ref_ptr<osg::Group>  > outNodes;

  struct stat BUF;
  bool have_dice=(stat(dicefile.c_str(),&BUF)!=-1);
  if(!have_dice){
    printf("Dice can't be found\n");
    exit(0);
    
  }else{
    
    FILE *dicefp=fopen(dicefile.c_str(),"r");
    char tmp[255];
    int eof=0;
    while(eof != EOF){
      eof=fscanf(dicefp,"%s\n",tmp);
      if(eof != EOF){
	printf("Diced files %s\n",tmp);
	meshNames.push_back("mesh-agg/"+string(tmp));
      }
    }
  }


 

  printf("Loading %d bbox files\n",meshNames.size());
  for(int i=0; i < (int) meshNames.size() && !(have_max_mesh_count && i >=(int) max_mesh_count); i++){
    std::vector<string> lodnames;
    for(int j=0; j < lodNum; j++){
      
      char out_name[255];
      
      sprintf(out_name,"mesh/blended-%02d-lod%d.ive",i,j);
      lodnames.push_back(osgDB::getSimpleFileName(string(out_name)).c_str());
      if(j == (lodNum -1)){
	osg::ref_ptr<osg::Group> node=	dynamic_cast<osg::Group*>(osgDB::readNodeFile(string(out_name)));
	outNodes.push_back(node);
	
      }
      
    }
    
    outNames.push_back(lodnames);
    
  }
  
  
  if(have_dice){
    genPagedLod(outNodes,outNames);
  }
 
  return 0;
}