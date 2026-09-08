//
// Created by admin on 2024/1/25.
//

#include "engine.h"



void Engine::init(Mode mode)
{
#ifdef HISTREAM_ENABLE_FACET
    if (mode == Mode::eFacetRT) {
        m_pFacetrt = std::make_shared<Facetrt>();
        m_pFacetrtio = std::make_shared<FacetrtIO>();
        return;
    }
    if (mode == Mode::eFacetEB) {
        m_pFaceteb = std::make_shared<Faceteb>();
        m_pFacetebio = std::make_shared<FacetebIO>();
        return;
    }
#else
#endif
    appSetting.init();

    if(mode == Mode::eRaytracing) {
        m_pRaytracing = std::make_shared<Raytracing>();
        m_pRaytracingio = std::make_shared<RaytracingIO>();
    }else if(mode == Mode::eVoxelEB) {
        m_pVoxeleb = std::make_shared<Voxeleb>();
        m_pVoxelebio = std::make_shared<VoxelebIO>();
    }else if(mode == Mode::eVoxelRT) {
        m_pVoxelrt = std::make_shared<Voxelrt>();
        m_pVoxelrtio = std::make_shared<VoxelrtIO>();
    }else if(mode == Mode::eHexRT) {
        m_pHexrt = std::make_shared<Hexrt>();
        m_pHexrtio = std::make_shared<HexrtIO>();
    }else if(mode == Mode::eHexEB) {
        m_pHexeb = std::make_shared<Hexeb>();
        m_pHexebio = std::make_shared<HexebIO>();
    }
}


void Engine::input(std::string path, std::string V, std::string outputPath){

    if (V == "eRaytracing") {
        m_mode = Mode::eRaytracing;
    } else if (V == "eVoxelEB") {
        m_mode = Mode::eVoxelEB;
    } else if (V == "eVoxelRT") {
        m_mode = Mode::eVoxelRT;
    } else if (V == "eFacetRT") {
        m_mode = Mode::eFacetRT;
    } else if (V == "eFacetEB") {
        m_mode = Mode::eFacetEB;
    } else if (V == "eHexRT") {
        m_mode = Mode::eHexRT;
    } else if (V == "eHexEB") {
        m_mode = Mode::eHexEB;
    }

    m_inputPath = std::move(path);
    m_outputPath = std::move(outputPath);
#ifdef HISTREAM_ENABLE_FACET
    if (m_mode == Mode::eFacetRT || m_mode == Mode::eFacetEB) {
        init(m_mode);
        if (m_mode == Mode::eFacetRT) {
            m_pFacetrtio->inputPath = m_inputPath;
            m_pFacetrtio->shaderDirectory = facetShaderDirectory("facetrt");
            m_pFacetrtio->outputPath = m_outputPath;
        } else {
            m_pFacetebio->inputPath = m_inputPath;
            m_pFacetebio->shaderDirectory = facetShaderDirectory("faceteb");
            m_pFacetebio->outputPath = m_outputPath;
        }
        return;
    }
#else
#endif
    m_pFileio = std::make_shared<FileIO>();
    m_pFileio->readXml(m_inputPath, m_mode);
    init(m_mode);

    if(m_mode == Mode::eRaytracing) {
        m_pRaytracing->setup(appSetting, m_pRaytracingio);
        m_pRaytracing->upload(m_pFileio, m_pRaytracingio);

        m_pRaytracingio->definedDir = m_pFileio->m_pRaytracingXml->definedDir;
        m_pRaytracingio->projectDir = m_pFileio->m_pRaytracingXml->projectDir;
    }else if(m_mode == Mode::eVoxelEB){

        m_pVoxeleb->setup(appSetting, m_pVoxelebio);
        m_pVoxeleb->upload(m_pFileio, m_pVoxelebio);

        m_pVoxelebio->definedDir = m_pFileio->m_pVoxelebXml->definedDir;
        m_pVoxelebio->projectDir = m_pFileio->m_pVoxelebXml->projectDir;
    }else if(m_mode == Mode::eVoxelRT){
        m_pVoxelrt->setup(appSetting, m_pVoxelrtio);
        m_pVoxelrt->upload(m_pFileio, m_pVoxelrtio);

        m_pVoxelrtio->definedDir = m_pFileio->m_pVoxelrtXml->definedDir;
        m_pVoxelrtio->projectDir = m_pFileio->m_pVoxelrtXml->projectDir;
    }else if(m_mode == Mode::eHexEB){
        m_pHexeb->setup(appSetting, m_pHexebio);
        m_pHexeb->upload(m_pFileio, m_pHexebio);

        m_pHexebio->definedDir = m_pFileio->m_pVoxelebXml->definedDir;
        m_pHexebio->projectDir = m_pFileio->m_pVoxelebXml->projectDir;
    }else if(m_mode == Mode::eHexRT){
        m_pHexrt->setup(appSetting, m_pHexrtio);
        m_pHexrt->upload(m_pFileio, m_pHexrtio);

        m_pHexrtio->definedDir = m_pFileio->m_pVoxelrtXml->definedDir;
        m_pHexrtio->projectDir = m_pFileio->m_pVoxelrtXml->projectDir;
    }

}

bool Engine::create() {

#ifdef HISTREAM_ENABLE_FACET
    if (m_mode == Mode::eFacetRT) {
        if (!m_pFacetrt || !m_pFacetrt->setup(m_pFacetrtio)) return false;
        if (!m_pFacetrt->upload(m_pFacetrtio)) return false;
        return m_pFacetrt->create(m_pFacetrtio);
    }
    if (m_mode == Mode::eFacetEB) {
        if (!m_pFaceteb || !m_pFaceteb->setup(m_pFacetebio)) return false;
        if (!m_pFaceteb->upload(m_pFacetebio)) return false;
        return m_pFaceteb->create(m_pFacetebio);
    }
#else
    if (m_mode == Mode::eFacetRT || m_mode == Mode::eFacetEB) return false; // facet 未编译
#endif
    if(m_mode == Mode::eRaytracing) {
        return m_pRaytracing->create(m_pRaytracingio);
    }else if(m_mode == Mode::eVoxelEB){
        return m_pVoxeleb->create(m_pVoxelebio);
    }else if(m_mode == Mode::eVoxelRT){
        return m_pVoxelrt->create(m_pVoxelrtio);
    }else if(m_mode == Mode::eHexEB){
        return m_pHexeb->create(m_pHexebio);
    }else if(m_mode == Mode::eHexRT){
        return m_pHexrt->create(m_pHexrtio);
    }
    return false;
}

std::string Engine::facetShaderDirectory(const char* name) const
{
    const std::filesystem::path local = std::filesystem::current_path() / "shader" / name;
    if (std::filesystem::exists(local)) {
        return local.string();
    }
#ifdef HISTREAM_FACETRT_SHADER_DIR
    if (std::string(name) == "facetrt") {
        return HISTREAM_FACETRT_SHADER_DIR;
    }
#endif
#ifdef HISTREAM_FACETEB_SHADER_DIR
    if (std::string(name) == "faceteb") {
        return HISTREAM_FACETEB_SHADER_DIR;
    }
#endif
    return local.string();
}

int Engine::run() {

#ifdef HISTREAM_ENABLE_FACET
    if (m_mode == Mode::eFacetRT) {
        return m_pFacetrt && m_pFacetrt->run(m_pFacetrtio) ? 0 : 1;
    }
    if (m_mode == Mode::eFacetEB) {
        return m_pFaceteb && m_pFaceteb->run(m_pFacetebio) ? 0 : 1;
    }
#else
    if (m_mode == Mode::eFacetRT || m_mode == Mode::eFacetEB) return 1; // facet 未编译
#endif
    if(m_mode == Mode::eRaytracing)
        return m_pRaytracing && m_pRaytracing->run(m_pRaytracingio,m_pFileio) ? 0 : 1;
    else if(m_mode == Mode::eVoxelEB)
        return m_pVoxeleb && m_pVoxeleb->run(m_pVoxelebio,m_pFileio) ? 0 : 1;
    else if(m_mode == Mode::eVoxelRT)
        return m_pVoxelrt && m_pVoxelrt->run(m_pVoxelrtio,m_pFileio) ? 0 : 1;
    else if(m_mode == Mode::eHexEB)
        return m_pHexeb && m_pHexeb->run(m_pHexebio,m_pFileio) ? 0 : 1;
    else if(m_mode == Mode::eHexRT)
        return m_pHexrt && m_pHexrt->run(m_pHexrtio,m_pFileio) ? 0 : 1;
    return 1;

}


void Engine::destroy() {

#ifdef HISTREAM_ENABLE_FACET
    if (m_mode == Mode::eFacetRT) {
        if (m_pFacetrt) m_pFacetrt->destroy(m_pFacetrtio);
        return;
    }
    if (m_mode == Mode::eFacetEB) {
        if (m_pFaceteb) m_pFaceteb->destroy(m_pFacetebio);
        return;
    }
#else
    if (m_mode == Mode::eFacetRT || m_mode == Mode::eFacetEB) return; // facet 未编译
#endif
    if(m_mode == Mode::eRaytracing)
        m_pRaytracing->destroy(m_pRaytracingio);
    else if(m_mode == Mode::eVoxelEB)
        m_pVoxeleb->destroy(m_pVoxelebio);
    else if(m_mode == Mode::eVoxelRT)
        m_pVoxelrt->destroy(m_pVoxelrtio);
    else if(m_mode == Mode::eHexEB)
        m_pHexeb->destroy(m_pHexebio);
    else if(m_mode == Mode::eHexRT)
        m_pHexrt->destroy(m_pHexrtio);
    appSetting.destroy();


}
