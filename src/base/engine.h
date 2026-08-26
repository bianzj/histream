//
// Created by admin on 2024/1/25.
//

#ifndef FIELD_ENGINE_H
#define FIELD_ENGINE_H

#include "src/raytracing/raytracingio.h"
#include "src/raytracing/raytracing.h"
#include "src/voxeleb/voxeleb.h"
#include "src/voxeleb/voxelebio.h"
#include "src/voxelrt/voxelrt.h"
#include "src/base/appsetting.h"
#include "src/base/fileio.h"




class Engine {
public:
    Engine(){};

    void input(std::string path, std::string V);
    void init(Mode mode);
    bool create();
    void run();
    void destroy();

    Mode m_mode;
    AppSetting appSetting;
    std::shared_ptr<FileIO>       m_pFileio;
    std::shared_ptr<Raytracing>   m_pRaytracing;
    std::shared_ptr<RaytracingIO> m_pRaytracingio;
    std::shared_ptr<Voxeleb>    m_pVoxeleb;
    std::shared_ptr<VoxelebIO>  m_pVoxelebio;
    std::shared_ptr<Voxelrt>    m_pVoxelrt;
    std::shared_ptr<VoxelrtIO>  m_pVoxelrtio;

};


#endif //FIELD_ENGINE_H
