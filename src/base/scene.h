//
// Created by admin on 2024/1/24.
//

#ifndef FIELD_SCENE_H
#define FIELD_SCENE_H

#include "src/base/fileio.h"
#include "src/base/meshio.h"
#include "structs.h"
#include "src/base/objloader.h"
#include "src/raytracing/raytracingio.h"
#include "src/voxeleb/voxelebio.h"
#include "src/voxelrt/voxelrtio.h"
#include "src/hexeb/hexebio.h"
#include "src/hexrt/hexrtio.h"
#include "src/thirdparty/NanoVDB.h"
#include "src/thirdparty/nanoutil/GridBuilder.h"
#include "src/thirdparty/nanoutil/Primitives.h"
#include "src/base/voxeldesigner.h"



class Scene {
public:
    Scene(){};

    // 异质性体元 (hex voxel) 开关: hexrt/hexeb 引擎在构建场景前置位,
    // createObjFilledVoxels 据此计算每个 OBJ 的 (Ax, Ay, Az, rho) 并写入
    // VoxelIO::voxelHexs / VoxelLink::hexId。
    bool hexVoxelEnabled{false};

    bool createObjScene(std::shared_ptr<FileIO> &fileio, std::shared_ptr<RaytracingIO> &raytracingio);


    bool createPrimObjScene(std::shared_ptr<FileIO> &fileio, std::shared_ptr<VoxelebIO> &voxellstio);
    bool createPrimObjScene(std::shared_ptr<FileIO> &fileio, std::shared_ptr<HexebIO> &hexebio);
    bool createPrimObj_Crown(PrimEntity & pe,nanovdb::GridBuilder<int32_t> &nanoBuilder,std::shared_ptr<VoxelebIO> &modelio);
    bool createPrimObj_Crowns(PrimEntity & pe,nanovdb::GridBuilder<int32_t> &nanoBuilder,std::shared_ptr<VoxelebIO> &modelio);
    bool createPrimObj_Building(PrimEntity & pe,nanovdb::GridBuilder<int32_t> &nanoBuilder,std::shared_ptr<VoxelebIO> &modelio);
    bool createPrimObj_Background(Background & background,nanovdb::GridBuilder<int32_t> &nanoBuilder, std::shared_ptr<VoxelebIO> &modelio);
    bool createPrimObj_Crown(PrimEntity & pe,nanovdb::GridBuilder<int32_t> &nanoBuilder,std::shared_ptr<HexebIO> &modelio);
    bool createPrimObj_Crowns(PrimEntity & pe,nanovdb::GridBuilder<int32_t> &nanoBuilder,std::shared_ptr<HexebIO> &modelio);
    bool createPrimObj_Building(PrimEntity & pe,nanovdb::GridBuilder<int32_t> &nanoBuilder, std::shared_ptr<HexebIO> &modelio);
    bool createPrimObj_Background(Background & background,nanovdb::GridBuilder<int32_t> &nanoBuilder, std::shared_ptr<HexebIO> &modelio);




    bool createPrimObjScene(std::shared_ptr<FileIO> &fileio, std::shared_ptr<VoxelrtIO> &voxelrtio);
    bool createPrimObjScene(std::shared_ptr<FileIO> &fileio, std::shared_ptr<HexrtIO> &hexrtio);
    bool createPrimObj_Crown(PrimEntity & pe,nanovdb::GridBuilder<int32_t> &nanoBuilder,std::shared_ptr<VoxelrtIO> &modelio);
    bool createPrimObj_Crowns(PrimEntity & pe,nanovdb::GridBuilder<int32_t> &nanoBuilder,std::shared_ptr<VoxelrtIO> &modelio);
    bool createPrimObj_Building(PrimEntity & pe,nanovdb::GridBuilder<int32_t> &nanoBuilder,std::shared_ptr<VoxelrtIO> &modelio);
    bool createPrimObj_Background(Background & background,nanovdb::GridBuilder<int32_t> &nanoBuilder, std::shared_ptr<VoxelrtIO> &modelio);
    bool createPrimObj_Crown(PrimEntity & pe,nanovdb::GridBuilder<int32_t> &nanoBuilder,std::shared_ptr<HexrtIO> &modelio);
    bool createPrimObj_Crowns(PrimEntity & pe,nanovdb::GridBuilder<int32_t> &nanoBuilder,std::shared_ptr<HexrtIO> &modelio);
    bool createPrimObj_Building(PrimEntity & pe,nanovdb::GridBuilder<int32_t> &nanoBuilder, std::shared_ptr<HexrtIO> &modelio);
    bool createPrimObj_Background(Background & background,nanovdb::GridBuilder<int32_t> &nanoBuilder, std::shared_ptr<HexrtIO> &modelio);



    void outputObjMesh(ObjMesh model, std::string &fileName);


    PrimMesh XYZ2XZY(PrimMesh model,int mark=0);
   ObjMesh XYZ2XZY(ObjMesh model);

};


#endif //FIELD_SCENE_H
