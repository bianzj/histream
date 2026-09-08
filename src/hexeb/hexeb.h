//
// Created by admin on 2024/1/26.
//

#ifndef FIELD_HEXEB_H
#define FIELD_HEXEB_H

#include <string>
#include <vector>
#include <map>
#include <iomanip>
#include "src/hexeb/hexebio.h"
#include "src/base/structs.h"
#include "src/base/geometry.h"
#include "src/base/compo.h"
#include "src/base/scene.h"
#include "src/hexeb/pipeline.h"
#include "src/base/virtualscreen.h"
#include "src/hexeb/buffer.h"
#include "src/base/accelstruct.h"
#include "src/hexeb/descriptor.h"
#include "src/hexeb/command.h"
#include "src/base/utils.h"

class Hexeb {
public:
    Hexeb(){
        m_pGeometry = std::make_shared<Geometry>();
        m_pScene = std::make_shared<Scene>();
        m_pCompo = std::make_shared<Compo>();

        m_pPipeline = std::make_shared<Pipeline>();
        m_pVirtual  = std::make_shared<VirtualScreen>();
        m_pBuffer = std::make_shared<Buffer>();
        m_pDescriptor = std::make_shared<Descriptor>();
        m_pCommand = std::make_shared<Command>();
    };


    bool setup(AppSetting &appsetting, std::shared_ptr<HexebIO> &voxellstio);
    bool upload(std::shared_ptr<FileIO> &fileio, std::shared_ptr<HexebIO> &voxellstio);
    bool create(std::shared_ptr<HexebIO> & modelio);
    bool run(std::shared_ptr<HexebIO> &modelio, std::shared_ptr<FileIO> &fileio);
    bool destroy( std::shared_ptr<HexebIO> &modelio);



    bool updateSetting(std::shared_ptr<HexebIO> &voxellstio);
    bool uploadSetting(std::shared_ptr<FileIO> &fileio, std::shared_ptr<HexebIO> &voxellstio);

    bool uploadAero(std::shared_ptr<FileIO> &fileio, std::shared_ptr<HexebIO> &voxellstio);



    bool uploadMeteo(std::shared_ptr<FileIO> &fileio, std::shared_ptr<HexebIO> &modelio);
    bool updateMeteo(std::shared_ptr<HexebIO> &modelio, int knode);

    void output(std::shared_ptr<HexebIO> &modelio, std::shared_ptr<FileIO> &fileio, int knode, int kangle);
    void outputTxt(std::shared_ptr<HexebIO> &modelio, std::shared_ptr<FileIO> &fileio, int kangle);
    void outputVoxel(std::shared_ptr<HexebIO> &modelio, std::shared_ptr<FileIO> &fileio);


    std::shared_ptr<Geometry> m_pGeometry;
    std::shared_ptr<Scene> m_pScene;
    std::shared_ptr<Compo> m_pCompo;
    std::shared_ptr<Pipeline> m_pPipeline;
    std::shared_ptr<VirtualScreen> m_pVirtual;
    std::shared_ptr<Buffer>  m_pBuffer;
    std::shared_ptr<Descriptor> m_pDescriptor;
    std::shared_ptr<Command> m_pCommand;

    bool uploadDefined(std::shared_ptr<FileIO> &fileio, std::shared_ptr<HexebIO> &modelio);
};


#endif //FIELD_HEXEB_H
