/* * structured - Tools for the Generation and Visualization of Large-scale
 * Three-dimensional Reconstructions from Image Data. This software includes
 * source code from other projects, which is subject to different licensing,
 * see COPYING for details. If this project is used for research see COPYING
 * for making the appropriate citations.
 * Copyright (C) 2013 Matthew Johnson-Roberson <mattkjr@gmail.com>
 *
 * This file is part of structured.
 *
 * structured is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * structured is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with structured.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TEXTURINGQUERY_H
#define TEXTURINGQUERY_H
#include <string>
#include "SpatialIndex.h"
#include <osg/Geode>
#include <osg/BoundingBox>
#include <osg/Geometry>
#include "TexPyrAtlas.h"
#include "TexturedSource.h"
#include "calibFile.h"
void addDups(osg::Geode *geode);
namespace vpb{
    class MyDestinationTile;
};
class CamProjAndDist{
public:
    SpatialIndex::id_type id;
    double dist;
    osg::Vec2 uv;
};

typedef std::vector<CamProjAndDist> CamDists;
int checkCached(std::string mf,std::string cachedloc,std::string &sha2hash);
bool loadCached(const std::string &file,osg::Vec4Array *ids,osg::Vec2Array *texCoords);
bool writeCached(const std::string &outfilename,const std::string  sha2hash,osg::Vec4Array *ids,osg::Vec2Array *texCoords);
std::string getHash(std::string mf);

class TexturingQuery
{
    friend class TexturedSource;
public:
    TexturingQuery(TexturedSource *source,const CameraCalib &calib,TexPyrAtlas &atlasGen,bool useTextureArray);
    ~TexturingQuery();
    bool projectModel(osg::Geode *,float margin=0.0);

    struct sort_pred {
        bool operator()(const CamProjAndDist &left, const CamProjAndDist &right) {
            return left.dist < right.dist;
        }
    };
    vpb::MyDestinationTile *_tile;
    //bool checkAndLoadCache(osg::Vec4Array *ids,osg::Vec2Array *texCoords);

    void addImagesToAtlasGen(std::map<SpatialIndex::id_type,int> allIds,  std::vector<std::set<SpatialIndex::id_type> > *sets);

protected:
    typedef std::multimap<int,SpatialIndex::id_type> ProjectsToMap;

    // overwritable by e.g. sonar
    virtual double getDistToCenter(osg::Vec3 v, TexturedSource::ProjectionCamera cam);
    virtual void findCamProjAndDist(CamProjAndDist &cpad,osg::Vec3 v,SpatialIndex::id_type id);
    virtual osg::Vec2 reprojectPt(const osg::Matrixf &mat,const osg::Vec3 &v, bool *is_visible=NULL);

    osg::Vec2 convertToUV(const osg::Vec2 &pix);
    const CamDists getClosest(std::vector<int> tri_v,const osg::Vec3Array &verts);
 //   std::string _bbox_file;
    const CameraCalib _calib;
    //void remapSharedVert(osg::PrimitiveSet& prset, osg::Vec3Array &verts,std::map<int,int> remap);

    std::string baseName;
    bool projectAllTrianglesOutCore(osg::Vec4Array* camIdxArr,TexBlendCoord &texCoordsArray,
                                             const osg::PrimitiveSet& prset, const osg::Vec3Array &verts,float margin);

    bool projectAllTriangles(osg::Vec4Array* camIdxArr,TexBlendCoord  &texCoordsArray,
                                             const osg::PrimitiveSet& prset, const osg::Vec3Array &verts);
    osg::StateSet *generateStateAndArray2DRemap( osg::Vec4Array *v,  osg::Vec2Array* texCoordsArray, int texSizeIdx);
    ProjectsToMap reproj;

    typedef std::pair<unsigned int, std::string> AttributeAlias;
    std::vector<osg::ref_ptr<osg::Image> >getRemappedImages(std::map<SpatialIndex::id_type,int> allIds,int sizeIdx);
    void generateStateAndSplitDrawables(std::vector<osg::Geometry*> &geoms,osg::Vec4Array *v, const osg::PrimitiveSet& prset,
                                        osg::Vec2Array* texCoordsArray,
                                        const osg::Vec3Array &verts,int texSizeIdx);
    AttributeAlias _vertexAlias;

public :
    bool _useTextureArray;
    TexPyrAtlas &_atlasGen;
    bool _useAtlas;
    TexturedSource *_source;

    std::vector<std::set<SpatialIndex::id_type> > sets;



};

typedef std::map<SpatialIndex::id_type,int> idmap_t;
typedef std::map<int,SpatialIndex::id_type> idbackmap_t;

void calcAllIdsBack(osg::Vec4Array *v,std::map<SpatialIndex::id_type,int> &allIds,std::map<int,SpatialIndex::id_type>  &backMap);

std::map<SpatialIndex::id_type,int> calcAllIds(osg::Vec4Array *v);
bool loadShaderSource(osg::Shader* obj, const std::string& fileName );

#endif // TEXTURINGQUERY_H
