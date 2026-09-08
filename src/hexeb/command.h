//
// Created by admin on 2024/1/26.
//

#ifndef FIELD_COMMAND_H
#define FIELD_COMMAND_H

#include "hexebio.h"

class Command {
public:
    Command();

    bool create(std::shared_ptr<HexebIO> &VoxellstIO);
//    bool run(std::shared_ptr<HexebIO> &HexebIO);

    bool runEB(std::shared_ptr<HexebIO> &modelio);
    bool runRT(std::shared_ptr<HexebIO> &modelio);

//    void submit(std::shared_ptr<HexebIO> &HexebIO,glm::ivec3 dispatchSize,
//                VkDescriptorSet descSet, VkPipelineLayout pipelineLayout,
//                std::map<VoxelRadStage, VkPipeline> pipelines, VoxelRadStage stage,VoxelLstSetting setting,
//                const std::optional<VkSemaphore> &inSemaphore, const std::optional<VkSemaphore> &outSemaphore);

    void submit(std::shared_ptr<HexebIO> &VoxellstIO, VoxelEBStage stage, glm::ivec3 dispatchSize,
                const std::optional<std::vector<VkSemaphore>> &inSemaphore, const std::optional<VkSemaphore> &outSemaphore);


//    void submit(std::shared_ptr<HexebIO> &HexebIO,glm::ivec3 dispatchSize,
//                VkDescriptorSet descSet, VkPipelineLayout pipelineLayout,
//                VkPipeline pipeline, VoxelLstSetting setting,
//                const std::optional<VkSemaphore> &inSemaphore, const std::optional<VkSemaphore> &outSemaphore);

    void recordHexebCommandBuffer(VkCommandBuffer cmdBuf,
                                      VkDescriptorSet descSet, VkPipelineLayout pipelineLayout,
                                      VkPipeline pipeline,  VoxelLstSetting setting);

//    void Command::recordHexebCommandBuffer(VkCommandBuffer cmdBuf, VkDescriptorSet descSet, VkPipelineLayout pipelineLayout,
//                                      std::map<VoxelEBStage, VkPipeline> pipelines, VoxelEBStage stage, VoxelLstSetting setting);

//    void recordHexebCommandBuffer(const VkCommandBuffer& cmdBuf,std::shared_ptr<HexebIO> &HexebIO);
    void waitFence(std::shared_ptr<HexebIO> &VoxellstIO);
    void destroy(std::shared_ptr<HexebIO> &VoxellstIO);

};


#endif //FIELD_COMMAND_H
