//
// Created by admin on 2024/1/25.
//

#include "descriptor.h"


bool Descriptor::createDescriptor(std::shared_ptr<HexrtIO> &modelio){

    auto & m_device = modelio->m_device;
    auto & bindings = modelio->m_bindings;
    auto & m_descSetLayout = modelio->m_descSetLayout;
    auto & m_descSetPool = modelio->m_descPool;
    auto & m_descSet = modelio->m_descSet;
    auto & voxelio = modelio->m_voxelio;
    auto & defined = modelio->m_defined;
    auto & meshio = modelio->m_meshio;
    auto & surfio = modelio->m_surfio;
    auto & instanceio = modelio->m_instanceio;
    auto & virtualio = modelio->m_virtualio;
 //   auto & sceneio = modelio->m_sceneio;


    VkShaderStageFlags flags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
    bindings.addBinding(HexrtbindingInd::spectral, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, flags);
    bindings.addBinding(HexrtbindingInd::thermal, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, flags);
    bindings.addBinding(HexrtbindingInd::canopy, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, flags);
    bindings.addBinding(HexrtbindingInd::meshLink, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, flags);
    bindings.addBinding(HexrtbindingInd::instanceLink, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, flags);
    bindings.addBinding(HexrtbindingInd::voxelLink, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,flags);
    bindings.addBinding(HexrtbindingInd::nano, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, flags);
    bindings.addBinding(HexrtbindingInd::tlas, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1,flags | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    bindings.addBinding(HexrtbindingInd::sensor, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, flags);
    bindings.addBinding(HexrtbindingInd::wave, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, flags);
    bindings.addBinding(HexrtbindingInd::light, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, flags);
    bindings.addBinding(HexrtbindingInd::dir, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, flags);
    bindings.addBinding(HexrtbindingInd::rads, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, flags);
    bindings.addBinding(HexrtbindingInd::netRad, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, flags);
    bindings.addBinding(HexrtbindingInd::storage, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, flags );
    bindings.addBinding(HexrtbindingInd::lad, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, flags);
    bindings.addBinding(HexrtbindingInd::hex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, flags);


    m_descSetLayout = bindings.createLayout(m_device);
    m_descSetPool = bindings.createPool(m_device, 1);
    m_descSet = nvvk::allocateDescriptorSet(m_device, m_descSetPool, m_descSetLayout);

    std::vector<VkWriteDescriptorSet> updates;

    // component    

    VkDescriptorBufferInfo dbiSpectral{meshio->m_pBufferSpectral->buffer, 0, VK_WHOLE_SIZE};
    updates.emplace_back(bindings.makeWrite(m_descSet, HexrtbindingInd::spectral, &dbiSpectral));


    if (meshio->thermals.size() > 0)
    {
        VkDescriptorBufferInfo dbiThermal{meshio->m_pBufferThermal->buffer, 0, VK_WHOLE_SIZE};
        updates.emplace_back(bindings.makeWrite(m_descSet, HexrtbindingInd::thermal, &dbiThermal));
    }


    VkDescriptorBufferInfo dbiCanopy{meshio->m_pBufferCanopy->buffer, 0, VK_WHOLE_SIZE};
    updates.emplace_back(bindings.makeWrite(m_descSet, HexrtbindingInd::canopy, &dbiCanopy));

    // link

    VkDescriptorBufferInfo dbiMeshLink{meshio->m_pBufferMeshLink->buffer, 0, VK_WHOLE_SIZE};
    updates.emplace_back(bindings.makeWrite(m_descSet, HexrtbindingInd::meshLink, &dbiMeshLink));

    VkDescriptorBufferInfo dbiInstanceLink{instanceio->m_pBufferInstanceLink->buffer, 0, VK_WHOLE_SIZE};
    updates.emplace_back(bindings.makeWrite(m_descSet, HexrtbindingInd::instanceLink, &dbiInstanceLink));

    VkDescriptorBufferInfo dbiVoxelLink{voxelio->m_pVoxelLinkBuffer->buffer, 0, VK_WHOLE_SIZE};
    updates.emplace_back(bindings.makeWrite(m_descSet, HexrtbindingInd::voxelLink, &dbiVoxelLink));

    VkDescriptorBufferInfo dbiNano{voxelio->m_pVoxelNanoBuffer->buffer, 0, VK_WHOLE_SIZE};
    updates.emplace_back(bindings.makeWrite(m_descSet, HexrtbindingInd::nano, &dbiNano));

    // Accelerate structure
    VkAccelerationStructureKHR tlas = modelio->m_pAccelStruct->getTlas();
    VkWriteDescriptorSetAccelerationStructureKHR descASInfo{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
    descASInfo.accelerationStructureCount = 1;
    descASInfo.pAccelerationStructures = &tlas;
    updates.emplace_back(bindings.makeWrite(m_descSet, HexrtbindingInd::tlas, &descASInfo));

    // sensor and light
    VkDescriptorBufferInfo dbiSensor{modelio->m_pBufferSensor->buffer, 0, VK_WHOLE_SIZE};
    updates.emplace_back(bindings.makeWrite(m_descSet, HexrtbindingInd::sensor, &dbiSensor));
    VkDescriptorBufferInfo dbiWave{modelio->m_pBufferWave->buffer, 0, VK_WHOLE_SIZE};
    updates.emplace_back(bindings.makeWrite(m_descSet, HexrtbindingInd::wave, &dbiWave));
    VkDescriptorBufferInfo dbiLight{modelio->m_pBufferLight->buffer, 0, VK_WHOLE_SIZE};
    updates.emplace_back(bindings.makeWrite(m_descSet, HexrtbindingInd::light, &dbiLight));



    VkDescriptorBufferInfo dbiDir{voxelio->m_pDirBuffer->buffer, 0, VK_WHOLE_SIZE};
    updates.emplace_back(bindings.makeWrite(m_descSet, HexrtbindingInd::dir, &dbiDir));
    VkDescriptorBufferInfo dbiRads{voxelio->m_pRadsBuffer->buffer, 0, VK_WHOLE_SIZE};
    updates.emplace_back(bindings.makeWrite(m_descSet, HexrtbindingInd::rads, &dbiRads));





    VkDescriptorBufferInfo dbiNetRad{voxelio->m_pNetRadBuffer->buffer, 0, VK_WHOLE_SIZE};
    updates.emplace_back(bindings.makeWrite(m_descSet, HexrtbindingInd::netRad, &dbiNetRad));

    VkDescriptorBufferInfo dbiStore{virtualio->m_pBufferStorage->buffer, 0, VK_WHOLE_SIZE};
    updates.emplace_back(bindings.makeWrite(m_descSet, HexrtbindingInd::storage, &dbiStore));

    VkDescriptorBufferInfo dbiLad{surfio->m_pBufferLad->buffer, 0, VK_WHOLE_SIZE};
    updates.emplace_back(bindings.makeWrite(m_descSet, HexrtbindingInd::lad, &dbiLad));

    VkDescriptorBufferInfo dbiHex{voxelio->m_pVoxelHexBuffer->buffer, 0, VK_WHOLE_SIZE};
    updates.emplace_back(bindings.makeWrite(m_descSet, HexrtbindingInd::hex, &dbiHex));

    vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(updates.size()), updates.data(), 0, nullptr);
    return true;
}

void Descriptor::destroy(std::shared_ptr<HexrtIO> &modelio) {

    vkDestroyDescriptorPool(modelio->m_device, modelio->m_descPool, nullptr);
    vkDestroyDescriptorSetLayout(modelio->m_device, modelio->m_descSetLayout, nullptr);


    modelio->m_descPool = VkDescriptorPool();
    modelio->m_descSetLayout = VkDescriptorSetLayout();
    modelio->m_descSet = VkDescriptorSet();

}









