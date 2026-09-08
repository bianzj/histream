//
// Created by admin on 2024/1/26.
//

#ifndef FIELD_HEXRT_H
#define FIELD_HEXRT_H

#include <string>
#include <vector>
#include <map>
#include <iomanip>
#include "src/hexrt/hexrtio.h"
#include "src/base/structs.h"
#include "src/base/geometry.h"
#include "src/base/compo.h"
#include "src/base/scene.h"
#include "src/hexrt/pipeline.h"
#include "src/base/virtualscreen.h"
#include "src/hexrt/buffer.h"
#include "src/base/accelstruct.h"
#include "src/hexrt/descriptor.h"
#include "src/hexrt/command.h"
#include "src/base/utils.h"

class Hexrt {
public:
    Hexrt(){
        m_pGeometry = std::make_shared<Geometry>();
        m_pScene = std::make_shared<Scene>();
        m_pCompo = std::make_shared<Compo>();

        m_pPipeline = std::make_shared<Pipeline>();
        m_pVirtual  = std::make_shared<VirtualScreen>();
        m_pBuffer = std::make_shared<Buffer>();
        m_pDescriptor = std::make_shared<Descriptor>();
        m_pCommand = std::make_shared<Command>();
    };


    bool setup(AppSetting &appsetting, std::shared_ptr<HexrtIO> &voxellstio);
    bool upload(std::shared_ptr<FileIO> &fileio, std::shared_ptr<HexrtIO> &voxellstio);
    bool create(std::shared_ptr<HexrtIO> & modelio);
    bool run(std::shared_ptr<HexrtIO> &modelio, std::shared_ptr<FileIO> &fileio);
    bool destroy( std::shared_ptr<HexrtIO> &modelio);



    bool updateSetting(std::shared_ptr<HexrtIO> &voxellstio);
    bool uploadSetting(std::shared_ptr<FileIO> &fileio, std::shared_ptr<HexrtIO> &voxellstio);

    void output(std::shared_ptr<HexrtIO> &modelio, std::shared_ptr<FileIO> &fileio, int knode, int kangle);
    void outputVoxel(std::shared_ptr<HexrtIO> &modelio, std::shared_ptr<FileIO> &fileio);
    void outputPos(std::shared_ptr<HexrtIO> &modelio, std::shared_ptr<FileIO> &fileio, int knode, int kangle);

    std::shared_ptr<Geometry> m_pGeometry;
    std::shared_ptr<Scene> m_pScene;
    std::shared_ptr<Compo> m_pCompo;
    std::shared_ptr<Pipeline> m_pPipeline;
    std::shared_ptr<VirtualScreen> m_pVirtual;
    std::shared_ptr<Buffer>  m_pBuffer;
    std::shared_ptr<Descriptor> m_pDescriptor;
    std::shared_ptr<Command> m_pCommand;

    bool uploadDefined(std::shared_ptr<FileIO> &fileio, std::shared_ptr<HexrtIO> &modelio);
};


#endif //FIELD_HEXRT_H
