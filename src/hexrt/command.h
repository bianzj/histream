//
// Created by admin on 2024/1/26.
//

#ifndef FIELD_COMMAND_H
#define FIELD_COMMAND_H

#include "hexrtio.h"

class Command {
public:
    Command();

    bool create(std::shared_ptr<HexrtIO> &VoxellstIO);
//    bool run(std::shared_ptr<HexrtIO> &HexrtIO);

    bool runRT(std::shared_ptr<HexrtIO> &modelio);

//    void submit(std::shared_ptr<HexrtIO> &HexrtIO,glm::ivec3 dispatchSize,
//                VkDescriptorSet descSet, VkPipelineLayout pipelineLayout,
//                std::map<VoxelRadStage, VkPipeline> pipelines, VoxelRadStage stage,VoxelLstSetting setting,
//                const std::optional<VkSemaphore> &inSemaphore, const std::optional<VkSemaphore> &outSemaphore);

    void submit(std::shared_ptr<HexrtIO> &VoxellstIO, VoxelRTStage stage, glm::ivec3 dispatchSize,
                const std::optional<std::vector<VkSemaphore>> &inSemaphore, const std::optional<VkSemaphore> &outSemaphore);


//    void submit(std::shared_ptr<HexrtIO> &HexrtIO,glm::ivec3 dispatchSize,
//                VkDescriptorSet descSet, VkPipelineLayout pipelineLayout,
//                VkPipeline pipeline, VoxelLstSetting setting,
//                const std::optional<VkSemaphore> &inSemaphore, const std::optional<VkSemaphore> &outSemaphore);

    void recordHexrtCommandBuffer(VkCommandBuffer cmdBuf,
                                      VkDescriptorSet descSet, VkPipelineLayout pipelineLayout,
                                      VkPipeline pipeline,  VoxelRTSetting setting);

//    void Command::recordHexrtCommandBuffer(VkCommandBuffer cmdBuf, VkDescriptorSet descSet, VkPipelineLayout pipelineLayout,
//                                      std::map<VoxelEBStage, VkPipeline> pipelines, VoxelEBStage stage, VoxelLstSetting setting);

//    void recordHexrtCommandBuffer(const VkCommandBuffer& cmdBuf,std::shared_ptr<HexrtIO> &HexrtIO);
    void waitFence(std::shared_ptr<HexrtIO> &VoxellstIO);
    void destroy(std::shared_ptr<HexrtIO> &VoxellstIO);

};


#endif //FIELD_COMMAND_H
