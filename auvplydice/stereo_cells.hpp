//
// Functions to read in the stereo pose data produced by seabed_slam, and divide
// the poses into a set of cells while avoiding segmenting the overlap regions.
//

#ifndef AUV_STEREO_CELLS_HPP
#define AUV_STEREO_CELLS_HPP

#include <string>
#include <vector>

class Stereo_Pose_Data
{
public:
   unsigned int id;
   double time;
   double pose[6];
   std::string left_name;
   std::string right_name;
   double alt;
   double radius;
   bool overlap;
};


class Bounds
{
public:
   Bounds( );
   Bounds( const std::vector<Stereo_Pose_Data> &poses );
   
   void set( double min_x, double max_x, double min_y, double max_y );

   double area( void );
    
   double min_x;
   double max_x;
   double min_y;
   double max_y;
};



class Cell_Data
{
public:

   Cell_Data( );

   Cell_Data( const std::vector<const Stereo_Pose_Data*> &poses,
              const Bounds                               &bounds );
              

   std::vector<const Stereo_Pose_Data*> poses;
   Bounds bounds;
};


//
// Public functions
//

std::vector<Stereo_Pose_Data> load_stereo_pose_file( const std::string &file_name );

std::vector<Cell_Data> calc_cells( const std::vector<Stereo_Pose_Data> &poses ); 



#endif // !AUV_STEREO_CELLS_HPP



