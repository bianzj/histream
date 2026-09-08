//
// Created by admin on 2024/1/26.
//

#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include "voxeleb.h"



bool Voxeleb::setup(AppSetting &appsetting, std::shared_ptr<VoxelebIO> &modelio){


    modelio->m_device = appsetting.m_context.m_device;
    modelio->m_physicalDevice = appsetting.m_context.m_physicalDevice;
    modelio->m_instance = appsetting.m_context.m_instance;
    modelio->m_queues = appsetting.m_queues;
    modelio->m_queue =  modelio->m_queues[eGCT].queue;
    modelio->m_queueIndex = modelio->m_queues[eGCT].familyIndex;
    //    m_instance = appSetting.m_context.m_instance;
//    m_device = appSetting.m_context.m_device;
//    m_physicalDevice = appSetting.m_context.m_physicalDevice;
//    m_queues = appSetting.m_queues;


//    VkCommandPoolCreateInfo poolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
//    poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
//    vkCreateCommandPool(modelio->m_device, &poolCreateInfo, nullptr, &modelio->m_cmdPool);

    modelio->m_genCmdBuf.init(modelio->m_device,modelio->m_queueIndex);


//    VkPipelineCacheCreateInfo pipelineCacheInfo{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
//    vkCreatePipelineCache(modelio->m_device, &pipelineCacheInfo, nullptr, &modelio->m_pipelineCache);

    modelio->m_pAlloc  = std::make_shared<Allocator>();
    modelio->m_pAlloc->init(modelio->m_instance, modelio->m_device, modelio->m_physicalDevice);
    modelio->m_debug.setup(modelio->m_device);

    VkPhysicalDeviceProperties2 rayTracingProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
    rayTracingProperties.pNext = &(modelio->m_rtProperties);
    vkGetPhysicalDeviceProperties2(modelio->m_physicalDevice, &rayTracingProperties);

    if (modelio->useSBTWrapper)
    {
        modelio->m_sbtWrapper.setup(modelio->m_device, modelio->m_queueIndex, modelio->m_pAlloc.get(), modelio->m_rtProperties);
    }

    modelio->m_pAccelStruct->m_rtBuilder.setup(modelio->m_device, modelio->m_pAlloc.get(),modelio->m_queueIndex);



    return true;

}




bool Voxeleb::upload(std::shared_ptr<FileIO> &fileio, std::shared_ptr<VoxelebIO> &modelio){

    // auto & fileio = modelio->m_fileio;
    // auto & meshio = modelio->m_meshio;

    uploadDefined(fileio,modelio);
    m_pCompo->createCompProperty(fileio, modelio);
    m_pScene->createPrimObjScene(fileio, modelio);
    m_pGeometry->createGeometry(fileio,modelio);
//    defineOPO(modelio);
    uploadMeteo(fileio,modelio);
    uploadSetting(fileio, modelio);
    uploadAero(fileio, modelio);

    return true;
}

bool Voxeleb::uploadSetting(std::shared_ptr<FileIO> &fileio, std::shared_ptr<VoxelebIO> &modelio) {

    modelio->n_wave = fileio->m_pVoxelebXml->sensorxml.waves.size();
    modelio->n_angle = fileio->m_pVoxelebXml->sensorxml.viewAngles.size();
    modelio->isTemperature =  fileio->m_pVoxelebXml->sensorxml.isTemperature;
    modelio->isDisplay = fileio->m_pVoxelebXml->sensorxml.isDisplay;
    modelio->isAlbedo = fileio->m_pVoxelebXml->sensorxml.isAlbedo;
    modelio->isImage = fileio->m_pVoxelebXml->sensorxml.isImage;
    modelio->isProcess = fileio->m_pVoxelebXml->sensorxml.isProcess;
    modelio->imageSize = fileio->m_pVoxelebXml->sensorxml.resolution;
    modelio->maxDepth = fileio->m_pVoxelebXml->settingxml.maxDepth;
    modelio->n_sample = fileio->m_pVoxelebXml->settingxml.n_sample;

    return true;
}

bool Voxeleb::updateSetting(std::shared_ptr<VoxelebIO> &modelio){

    // auto &opo = modelio->m_opo;

    // auto &sceneio = modelio->m_sceneio;

    modelio->setting.imageSize = modelio->imageSize;
    modelio->setting.n_wave = modelio->n_wave;
    modelio->setting.scale = modelio->stepsize_surface;
   // modelio->setting.isTemperature = modelio->isTemperature;
    modelio->setting.isDisplay = modelio->isDisplay;
    modelio->setting.maxDepth = modelio->maxDepth;
    modelio->setting.n_sample = modelio->n_sample;
    modelio->setting.voxelSize = modelio->voxelSize_XZY;


    return true;
    //modelio->setting.maxDepth = fileio->m_pXmlInput.
}


bool  Voxeleb::uploadMeteo(std::shared_ptr<FileIO> &fileio, std::shared_ptr<VoxelebIO> &modelio){


   // Utils::readascfileinout(meteofile,0,1,)
    modelio->startTimeNode = fileio->m_pVoxelebXml->meteoxml.startTimeNode;
    modelio->endTimeNode = fileio->m_pVoxelebXml->meteoxml.endTimeNode;
   fileio->readMeteo(modelio->m_defined,modelio->n_node,modelio->meteos,modelio->atomconds);
   return true;
}

bool  Voxeleb::updateMeteo(std::shared_ptr<VoxelebIO> &modelio, int knode){

    modelio->meteo = modelio->meteos[knode];

    nvvk::CommandPool cmdBufGet(modelio->m_device, modelio->m_queueIndex);
    vk::CommandBuffer cmdBuf = cmdBufGet.createCommandBuffer();
    vkCmdUpdateBuffer(cmdBuf, (*modelio->m_pMeteoBuffer).buffer, 0, sizeof(Meteo), &modelio->meteo);
    cmdBufGet.submitAndWait(cmdBuf);

    return true;
}

bool Voxeleb::uploadDefined(std::shared_ptr<FileIO> &fileio, std::shared_ptr<VoxelebIO> &modelio)
{

    fileio->readDefined(modelio->m_defined);

    return true;
}

bool Voxeleb::create(std::shared_ptr<VoxelebIO> &modelio) {


    m_pBuffer->createBuffer(modelio);
    m_pDescriptor->createDescriptor(modelio);
    if (!m_pPipeline->createPipeline(modelio)) {
        return false;
    }
    m_pCommand->create(modelio);
    updateSetting(modelio);
    return true;
}

bool Voxeleb::run(std::shared_ptr<VoxelebIO> &modelio, std::shared_ptr<FileIO> &fileio) {


// for(int knode = 72; knode < 75;knode = knode + 1) {
    /// 清除txt文件信息
    // 以写入模式打开文件（std::ios::trunc 会清空文件）
    std::ofstream file(modelio->projectDir + "\\result_statistics.txt", std::ios::trunc);
    // 检查是否成功打开
    if (!file.is_open()) {
        std::cerr << "Error: Could not clear file " << modelio->projectDir + "\\result_statistics.txt" << std::endl;
    }
    file << "t SZA SAA VZA VAA wavelength Radiance\n";
    // 文件内容已被清空，无需额外操作
    file.close(); // 显式关闭（可选）


    //modelio->startTimeNode = 63;
    //modelio->endTimeNode = 74;
    for(int knode = modelio->startTimeNode; knode < modelio->endTimeNode;knode = knode +1) {
    // for(int knode = 0; knode < 144;knode = knode + 1) {
        modelio->k_node = knode;

        updateMeteo(modelio,knode);

        // Energy balance starts with the solar-transmittance pass.  Upload the
        // sun direction for the current meteorological time before that pass;
        // otherwise it traces the XML/previous-time direction and the shadow
        // field lags behind the meteorological forcing.
        m_pGeometry->updateAngle(modelio, 0);

        m_pCommand->runEB(modelio);

        const Angle& solarAngle = modelio->angles[0];
        std::cout << "Time Info:"
                  << "    t_" << std::to_string(modelio->meteo.t)
                  << "    sza_" << std::to_string(solarAngle.sza)
                  << "    saa_" << std::to_string(solarAngle.saa) << std::endl;
        if (modelio->isProcess) {
            outputVoxel(modelio,fileio);
        }
    //
    //    if (knode == 73)
        // if (1)
        {
        for (int kangle = 0; kangle < modelio->n_angle; kangle++) {
            modelio->k_angle = kangle;
            m_pGeometry->updateAngle(modelio,kangle);

            Angle angle = modelio->angles[kangle];

            ////updateSetting(modelio);
            m_pCommand->runRT(modelio);

    //        std::cout << "Success: " << kangle << std::endl;


                std::cout << "Angle Info:"
                      << "    vza_" << std::to_string(angle.vza) << "    vaa_" << std::to_string(angle.vaa)
                      << "    sza_" << std::to_string(angle.sza) << "    saa_" << std::to_string(angle.saa) << std::endl;

                if (modelio->isImage)
                {
                    output(modelio,fileio,knode, kangle);
                }

                outputTxt(modelio, fileio, kangle);
            }
            // output(modelio,fileio,knode, kangle);
        }

    }
    return true;
}



bool Voxeleb::destroy(std::shared_ptr<VoxelebIO> & modelio){

    m_pBuffer->destroy(modelio);
    modelio->m_sbtWrapper.destroy();
    modelio->m_pAccelStruct->m_rtBuilder.destroy();

    m_pDescriptor->destroy(modelio);
    m_pPipeline->destroy(modelio);
    m_pCommand->destroy(modelio);



    vkDeviceWaitIdle(modelio->m_device);
    modelio->m_genCmdBuf.deinit();
    modelio->m_pAlloc->deinit();
    
    return false;

}



void Voxeleb::output(std::shared_ptr<VoxelebIO> &modelio, std::shared_ptr<FileIO> &fileio, int knode, int kangle) {
    VkBufferUsageFlags usage{VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT};
    const int width = modelio->imageSize.x;
    const int height = modelio->imageSize.y;
    const int nWave = modelio->n_wave;
    if (width <= 0 || height <= 0 || nWave <= 0
        || kangle < 0 || static_cast<size_t>(kangle) >= modelio->angles.size()) {
        return;
    }

    const size_t imageElements = static_cast<size_t>(width) * height;
    const size_t totalElements = imageElements * nWave;
    const VkDeviceSize bufferSize = totalElements * sizeof(float);
    nvvk::Buffer pixelBuffer = modelio->m_pAlloc->createBuffer(
        bufferSize, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    m_pVirtual->bufferToBuffer(
        modelio, *(modelio->m_virtualio->m_pBufferStorage), bufferSize, pixelBuffer);

    void* mappedData = modelio->m_pAlloc->map(pixelBuffer);
    std::vector<float> sourceData(totalElements);
    std::memcpy(sourceData.data(), mappedData, static_cast<size_t>(bufferSize));
    modelio->m_pAlloc->unmap(pixelBuffer);
    modelio->m_pAlloc->destroy(pixelBuffer);

    Angle angle = modelio->angles[kangle];
    Eigen::VectorXd cx;
    Eigen::VectorXd cy;
    m_pGeometry->orthcorrect(modelio, angle.vza, angle.vaa, cx, cy);

    std::vector<float> orthData(totalElements, 0.0f);
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            const int ii = static_cast<int>(i * cx[0] + j * cx[1] + i * j * cx[2] + cx[3]);
            const int jj = static_cast<int>(i * cy[0] + j * cy[1] + i * j * cy[2] + cy[3]);
            if (ii < 0 || ii >= width || jj < 0 || jj >= height) {
                continue;
            }

            const size_t destination = static_cast<size_t>(j) * width + i;
            const size_t source = static_cast<size_t>(jj) * width + ii;
            for (int band = 0; band < nWave; ++band) {
                const size_t bandOffset = static_cast<size_t>(band) * imageElements;
                const float value = sourceData[bandOffset + source];
                if (value != 0.0f) {
                    orthData[bandOffset + destination] = value;
                }
            }
        }
    }

    const float time = modelio->meteo.t;
    fileio->writeENVIdata(
        modelio->projectDir, orthData.data(), width, height, nWave, angle, time, -1);
}
void Voxeleb::outputTxt(std::shared_ptr<VoxelebIO>& modelio, std::shared_ptr<FileIO>& fileio, int kangle)
{
    VkBufferUsageFlags usage{VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT};
    const int width = modelio->imageSize.x;
    const int height = modelio->imageSize.y;
    const int nWave = modelio->n_wave;
    if (width <= 0 || height <= 0 || nWave <= 0
        || kangle < 0 || static_cast<size_t>(kangle) >= modelio->angles.size()) {
        return;
    }

    const size_t imageElements = static_cast<size_t>(width) * height;
    const size_t totalElements = imageElements * nWave;
    const VkDeviceSize bufferSize = totalElements * sizeof(float);
    nvvk::Buffer pixelBuffer = modelio->m_pAlloc->createBuffer(
        bufferSize, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    m_pVirtual->bufferToBuffer(
        modelio, *(modelio->m_virtualio->m_pBufferStorage), bufferSize, pixelBuffer);

    void* mappedData = modelio->m_pAlloc->map(pixelBuffer);
    std::vector<float> sourceData(totalElements);
    std::memcpy(sourceData.data(), mappedData, static_cast<size_t>(bufferSize));
    modelio->m_pAlloc->unmap(pixelBuffer);
    modelio->m_pAlloc->destroy(pixelBuffer);

    Angle angle = modelio->angles[kangle];
    Eigen::VectorXd cx;
    Eigen::VectorXd cy;
    m_pGeometry->orthcorrect(modelio, angle.vza, angle.vaa, cx, cy);

    std::vector<float> orthData(totalElements, 0.0f);
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            const int ii = static_cast<int>(i * cx[0] + j * cx[1] + i * j * cx[2] + cx[3]);
            const int jj = static_cast<int>(i * cy[0] + j * cy[1] + i * j * cy[2] + cy[3]);
            if (ii < 0 || ii >= width || jj < 0 || jj >= height) {
                continue;
            }

            const size_t destination = static_cast<size_t>(j) * width + i;
            const size_t source = static_cast<size_t>(jj) * width + ii;
            for (int band = 0; band < nWave; ++band) {
                const size_t bandOffset = static_cast<size_t>(band) * imageElements;
                const float value = sourceData[bandOffset + source];
                if (value != 0.0f) {
                    orthData[bandOffset + destination] = value;
                }
            }
        }
    }

    const float time = modelio->meteo.t;
    fileio->outImage.clear();
    for (int band = 0; band < nWave; ++band) {
        const float* bandBegin = orthData.data() + static_cast<size_t>(band) * imageElements;
        std::vector<float> outImage(bandBegin, bandBegin + imageElements);
        fileio->outImage.push_back(outImage);

        double sum = 0.0;
        size_t count = 0;
        for (float value : outImage) {
            if (value > 0.0f) {
                sum += value;
                ++count;
            }
        }
        fileio->outImageMeanValue.push_back(
            count == 0 ? 0.0f : static_cast<float>(sum / count));
    }

    std::ofstream outfile(modelio->projectDir + "\\result_statistics.txt", std::ios::app);
    if (!outfile.is_open()) {
        std::cerr << "Error: Could not open file "
                  << modelio->projectDir + "\\result_statistics.txt" << std::endl;
        return;
    }

    for (int band = 0; band < nWave; ++band) {
        outfile << time << " " << angle.sza << " " << angle.saa << " "
                << angle.vza << " " << angle.vaa << " " << modelio->waves[band] << " "
                << fileio->outImageMeanValue[
                       fileio->outImageMeanValue.size() - nWave + band]
                << "\n";
    }
}
void Voxeleb::outputVoxel(std::shared_ptr<VoxelebIO> &modelio, std::shared_ptr<FileIO> &fileio) {
    (void)fileio;
    if (!modelio || modelio->n_voxel <= 0 || !modelio->m_voxelio ||
        !modelio->m_voxelio->m_pTempeBuffer || !modelio->m_voxelio->m_pNetRadBuffer ||
        !modelio->m_voxelio->m_pFluxBuffer) {
        return;
    }

    const size_t voxelCount = static_cast<size_t>(modelio->n_voxel);
    std::vector<VoxelTempe> temperatures(voxelCount);
    std::vector<VoxelNetRad> netRadiation(voxelCount);
    std::vector<VoxelHeatflux> heatFlux(voxelCount);
    const auto download = [&](const nvvk::Buffer& source, void* destination, VkDeviceSize size) {
        nvvk::Buffer staging = modelio->m_pAlloc->createBuffer(
            size, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        m_pVirtual->bufferToBuffer(modelio, source, size, staging);
        void* mapped = modelio->m_pAlloc->map(staging);
        std::memcpy(destination, mapped, static_cast<size_t>(size));
        modelio->m_pAlloc->unmap(staging);
        modelio->m_pAlloc->destroy(staging);
    };
    download(*modelio->m_voxelio->m_pTempeBuffer, temperatures.data(), voxelCount * sizeof(VoxelTempe));
    download(*modelio->m_voxelio->m_pNetRadBuffer, netRadiation.data(), voxelCount * sizeof(VoxelNetRad));
    download(*modelio->m_voxelio->m_pFluxBuffer, heatFlux.data(), voxelCount * sizeof(VoxelHeatflux));

    const double julianTime = static_cast<double>(modelio->meteo.t);
    int day = static_cast<int>(std::floor(julianTime));
    int totalMinutes = static_cast<int>(std::llround((julianTime - day) * 1440.0));
    if (totalMinutes >= 1440) {
        day += totalMinutes / 1440;
        totalMinutes %= 1440;
    }
    std::ostringstream time;
    time << "DOY" << day << '_' << std::setw(2) << std::setfill('0') << totalMinutes / 60
         << '-' << std::setw(2) << std::setfill('0') << totalMinutes % 60;

    const std::filesystem::path directory = std::filesystem::path(modelio->projectDir) / "process";
    std::filesystem::create_directories(directory);
    const std::string stem = "energy_T=" + time.str();
    const std::filesystem::path binaryPath = directory / (stem + ".bin");
    const std::filesystem::path metadataPath = directory / (stem + ".json");
    std::ofstream binary(binaryPath, std::ios::binary | std::ios::trunc);
    if (!binary) {
        throw std::runtime_error("Cannot write process energy file: " + binaryPath.string());
    }
    for (size_t voxel = 0; voxel < voxelCount; ++voxel) {
        const float record[12] = {
            temperatures[voxel].sunlit, temperatures[voxel].shaded,
            netRadiation[voxel].directVrad, netRadiation[voxel].diffuseVrad,
            netRadiation[voxel].directTrad, netRadiation[voxel].diffuseTrad,
            heatFlux[voxel].Hsunlit, heatFlux[voxel].Hshaded,
            heatFlux[voxel].LEsunlit, heatFlux[voxel].LEshaded,
            heatFlux[voxel].Gsunlit, heatFlux[voxel].Gshaded
        };
        binary.write(reinterpret_cast<const char*>(record), sizeof(record));
    }
    binary.close();

    std::ofstream metadata(metadataPath, std::ios::trunc);
    if (!metadata) {
        throw std::runtime_error("Cannot write process metadata file: " + metadataPath.string());
    }
    metadata << std::setprecision(9)
             << "{\n  \"kind\": \"voxel-energy-process\",\n"
             << "  \"node\": " << modelio->k_node << ",\n"
             << "  \"julianTime\": " << modelio->meteo.t << ",\n"
             << "  \"time\": \"" << time.str() << "\",\n"
             << "  \"voxelCount\": " << voxelCount << ",\n"
             << "  \"dataFile\": \"" << binaryPath.filename().string() << "\",\n"
             << "  \"dataType\": \"float32-little-endian\",\n"
             << "  \"layout\": \"voxel-interleaved\",\n"
             << "  \"fields\": [\"temperatureSunlit\", \"temperatureShaded\", "
                "\"directVnir\", \"diffuseVnir\", \"directTir\", \"diffuseTir\", "
                "\"sensibleHeatSunlit\", \"sensibleHeatShaded\", "
                "\"latentHeatSunlit\", \"latentHeatShaded\", "
                "\"storageHeatSunlit\", \"storageHeatShaded\"]\n}\n";
    std::cout << "PROCESS\t" << metadataPath.string() << std::endl;
}


bool Voxeleb::uploadAero(std::shared_ptr<FileIO> &fileio, std::shared_ptr<VoxelebIO> &modelio) {


    if(fileio->m_pVoxelebXml->aerocondxml.aerotype == AeroType::ONE) {
        modelio->aeroconds.emplace_back(fileio->m_pVoxelebXml->aerocondxml.aerocond);
    }else if (fileio->m_pVoxelebXml->aerocondxml.aerotype == AeroType::image)
    {
        int a = 10;
    }else if(fileio->m_pVoxelebXml->aerocondxml.aerotype == AeroType::gridCal){
        int b = 10;
    }

    return false;


}
