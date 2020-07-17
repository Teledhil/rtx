#include "ray_tracing_extensions.h"

#include <iostream>

#include <vulkan/vulkan.h>

static PFN_vkCreateAccelerationStructureNV pfn_vkCreateAccelerationStructureNV =
    0;
static PFN_vkDestroyAccelerationStructureNV
    pfn_vkDestroyAccelerationStructureNV = 0;
static PFN_vkGetAccelerationStructureMemoryRequirementsNV
    pfn_vkGetAccelerationStructureMemoryRequirementsNV = 0;
static PFN_vkBindAccelerationStructureMemoryNV
    pfn_vkBindAccelerationStructureMemoryNV = 0;
static PFN_vkCmdBuildAccelerationStructureNV
    pfn_vkCmdBuildAccelerationStructureNV = 0;
static PFN_vkCmdCopyAccelerationStructureNV
    pfn_vkCmdCopyAccelerationStructureNV = 0;
static PFN_vkCmdTraceRaysNV pfn_vkCmdTraceRaysNV = 0;
static PFN_vkCreateRayTracingPipelinesNV pfn_vkCreateRayTracingPipelinesNV = 0;
static PFN_vkGetRayTracingShaderGroupHandlesNV
    pfn_vkGetRayTracingShaderGroupHandlesNV = 0;
static PFN_vkGetAccelerationStructureHandleNV
    pfn_vkGetAccelerationStructureHandleNV = 0;
static PFN_vkCmdWriteAccelerationStructuresPropertiesNV
    pfn_vkCmdWriteAccelerationStructuresPropertiesNV = 0;
static PFN_vkCompileDeferredNV pfn_vkCompileDeferredNV = 0;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateAccelerationStructureNV(
    VkDevice device, const VkAccelerationStructureCreateInfoNV* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkAccelerationStructureNV* pAccelerationStructure) {
  return pfn_vkCreateAccelerationStructureNV(device, pCreateInfo, pAllocator,
                                             pAccelerationStructure);
}
VKAPI_ATTR void VKAPI_CALL vkDestroyAccelerationStructureNV(
    VkDevice device, VkAccelerationStructureNV accelerationStructure,
    const VkAllocationCallbacks* pAllocator) {
  pfn_vkDestroyAccelerationStructureNV(device, accelerationStructure,
                                       pAllocator);
}
VKAPI_ATTR void VKAPI_CALL vkGetAccelerationStructureMemoryRequirementsNV(
    VkDevice device,
    const VkAccelerationStructureMemoryRequirementsInfoNV* pInfo,
    VkMemoryRequirements2KHR* pMemoryRequirements) {
  pfn_vkGetAccelerationStructureMemoryRequirementsNV(device, pInfo,
                                                     pMemoryRequirements);
}
VKAPI_ATTR VkResult VKAPI_CALL vkBindAccelerationStructureMemoryNV(
    VkDevice device, uint32_t bindInfoCount,
    const VkBindAccelerationStructureMemoryInfoNV* pBindInfos) {
  return pfn_vkBindAccelerationStructureMemoryNV(device, bindInfoCount,
                                                 pBindInfos);
}
VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructureNV(
    VkCommandBuffer commandBuffer, const VkAccelerationStructureInfoNV* pInfo,
    VkBuffer instanceData, VkDeviceSize instanceOffset, VkBool32 update,
    VkAccelerationStructureNV dst, VkAccelerationStructureNV src,
    VkBuffer scratch, VkDeviceSize scratchOffset) {
  pfn_vkCmdBuildAccelerationStructureNV(commandBuffer, pInfo, instanceData,
                                        instanceOffset, update, dst, src,
                                        scratch, scratchOffset);
}
VKAPI_ATTR void VKAPI_CALL vkCmdCopyAccelerationStructureNV(
    VkCommandBuffer commandBuffer, VkAccelerationStructureNV dst,
    VkAccelerationStructureNV src, VkCopyAccelerationStructureModeNV mode) {
  pfn_vkCmdCopyAccelerationStructureNV(commandBuffer, dst, src, mode);
}
VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysNV(
    VkCommandBuffer commandBuffer, VkBuffer raygenShaderBindingTableBuffer,
    VkDeviceSize raygenShaderBindingOffset,
    VkBuffer missShaderBindingTableBuffer, VkDeviceSize missShaderBindingOffset,
    VkDeviceSize missShaderBindingStride, VkBuffer hitShaderBindingTableBuffer,
    VkDeviceSize hitShaderBindingOffset, VkDeviceSize hitShaderBindingStride,
    VkBuffer callableShaderBindingTableBuffer,
    VkDeviceSize callableShaderBindingOffset,
    VkDeviceSize callableShaderBindingStride, uint32_t width, uint32_t height,
    uint32_t depth) {
  pfn_vkCmdTraceRaysNV(commandBuffer, raygenShaderBindingTableBuffer,
                       raygenShaderBindingOffset, missShaderBindingTableBuffer,
                       missShaderBindingOffset, missShaderBindingStride,
                       hitShaderBindingTableBuffer, hitShaderBindingOffset,
                       hitShaderBindingStride, callableShaderBindingTableBuffer,
                       callableShaderBindingOffset, callableShaderBindingStride,
                       width, height, depth);
}
VKAPI_ATTR VkResult VKAPI_CALL vkCreateRayTracingPipelinesNV(
    VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
    const VkRayTracingPipelineCreateInfoNV* pCreateInfos,
    const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) {
  return pfn_vkCreateRayTracingPipelinesNV(device, pipelineCache,
                                           createInfoCount, pCreateInfos,
                                           pAllocator, pPipelines);
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingShaderGroupHandlesNV(
    VkDevice device, VkPipeline pipeline, uint32_t firstGroup,
    uint32_t groupCount, size_t dataSize, void* pData) {
  return pfn_vkGetRayTracingShaderGroupHandlesNV(device, pipeline, firstGroup,
                                                 groupCount, dataSize, pData);
}
VKAPI_ATTR VkResult VKAPI_CALL vkGetAccelerationStructureHandleNV(
    VkDevice device, VkAccelerationStructureNV accelerationStructure,
    size_t dataSize, void* pData) {
  return pfn_vkGetAccelerationStructureHandleNV(device, accelerationStructure,
                                                dataSize, pData);
}
VKAPI_ATTR void VKAPI_CALL vkCmdWriteAccelerationStructuresPropertiesNV(
    VkCommandBuffer commandBuffer, uint32_t accelerationStructureCount,
    const VkAccelerationStructureNV* pAccelerationStructures,
    VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery) {
  pfn_vkCmdWriteAccelerationStructuresPropertiesNV(
      commandBuffer, accelerationStructureCount, pAccelerationStructures,
      queryType, queryPool, firstQuery);
}
VKAPI_ATTR VkResult VKAPI_CALL vkCompileDeferredNV(VkDevice device,
                                                   VkPipeline pipeline,
                                                   uint32_t shader) {
  return pfn_vkCompileDeferredNV(device, pipeline, shader);
}

bool rtx::ray_tracing_extensions::load(VkDevice device) {
  pfn_vkCreateAccelerationStructureNV =
      reinterpret_cast<PFN_vkCreateAccelerationStructureNV>(
          vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureNV"));
  if (!pfn_vkCreateAccelerationStructureNV) {
    std::cerr << "Failed to get function vkCreateAccelerationStructureNV."
              << std::endl;
    return false;
  }

  pfn_vkDestroyAccelerationStructureNV =
      reinterpret_cast<PFN_vkDestroyAccelerationStructureNV>(
          vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureNV"));
  if (!pfn_vkDestroyAccelerationStructureNV) {
    std::cerr << "Failed to get function vkDestroyAccelerationStructureNV."
              << std::endl;
    return false;
  }

  pfn_vkGetAccelerationStructureMemoryRequirementsNV =
      reinterpret_cast<PFN_vkGetAccelerationStructureMemoryRequirementsNV>(
          vkGetDeviceProcAddr(
              device, "vkGetAccelerationStructureMemoryRequirementsNV"));
  if (!pfn_vkGetAccelerationStructureMemoryRequirementsNV) {
    std::cerr << "Failed to get function "
                 "vkGetAccelerationStructureMemoryRequirementsNV."
              << std::endl;
    return false;
  }

  pfn_vkBindAccelerationStructureMemoryNV =
      reinterpret_cast<PFN_vkBindAccelerationStructureMemoryNV>(
          vkGetDeviceProcAddr(device, "vkBindAccelerationStructureMemoryNV"));
  if (!pfn_vkBindAccelerationStructureMemoryNV) {
    std::cerr << "Failed to get function "
                 "vkBindAccelerationStructureMemoryNV."
              << std::endl;
    return false;
  }

  pfn_vkCmdBuildAccelerationStructureNV =
      reinterpret_cast<PFN_vkCmdBuildAccelerationStructureNV>(
          vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructureNV"));
  if (!pfn_vkCmdBuildAccelerationStructureNV) {
    std::cerr << "Failed to get function vkCmdBuildAccelerationStructureNV."
              << std::endl;
    return false;
  }

  pfn_vkCmdCopyAccelerationStructureNV =
      reinterpret_cast<PFN_vkCmdCopyAccelerationStructureNV>(
          vkGetDeviceProcAddr(device, "vkCmdCopyAccelerationStructureNV"));
  if (!pfn_vkCmdCopyAccelerationStructureNV) {
    std::cerr << "Failed to get function vkCmdCopyAccelerationStructureNV."
              << std::endl;
    return false;
  }

  pfn_vkCmdTraceRaysNV = reinterpret_cast<PFN_vkCmdTraceRaysNV>(
      vkGetDeviceProcAddr(device, "vkCmdTraceRaysNV"));
  if (!pfn_vkCmdTraceRaysNV) {
    std::cerr << "Failed to get function vkCmdTraceRaysNV." << std::endl;
    return false;
  }

  pfn_vkCreateRayTracingPipelinesNV =
      reinterpret_cast<PFN_vkCreateRayTracingPipelinesNV>(
          vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesNV"));
  if (!pfn_vkCreateRayTracingPipelinesNV) {
    std::cerr << "Failed to get function vkCreateRayTracingPipelinesNV."
              << std::endl;
    return false;
  }

  pfn_vkGetRayTracingShaderGroupHandlesNV =
      reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesNV>(
          vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesNV"));
  if (!pfn_vkGetRayTracingShaderGroupHandlesNV) {
    std::cerr << "Failed to get function vkGetRayTracingShaderGroupHandlesNV."
              << std::endl;
    return false;
  }

  pfn_vkGetAccelerationStructureHandleNV =
      reinterpret_cast<PFN_vkGetAccelerationStructureHandleNV>(
          vkGetDeviceProcAddr(device, "vkGetAccelerationStructureHandleNV"));
  if (!pfn_vkGetAccelerationStructureHandleNV) {
    std::cerr << "Failed to get function vkGetAccelerationStructureHandleNV."
              << std::endl;
    return false;
  }

  pfn_vkCmdWriteAccelerationStructuresPropertiesNV =
      reinterpret_cast<PFN_vkCmdWriteAccelerationStructuresPropertiesNV>(
          vkGetDeviceProcAddr(device,
                              "vkCmdWriteAccelerationStructuresPropertiesNV"));
  if (!pfn_vkCmdWriteAccelerationStructuresPropertiesNV) {
    std::cerr << "Failed to get function "
                 "vkCmdWriteAccelerationStructuresPropertiesNV."
              << std::endl;
    return false;
  }

  pfn_vkCompileDeferredNV = reinterpret_cast<PFN_vkCompileDeferredNV>(
      vkGetDeviceProcAddr(device, "vkCompileDeferredNV"));
  if (!pfn_vkCompileDeferredNV) {
    std::cerr << "Failed to get function vkCompileDeferredNV." << std::endl;
    return false;
  }

  return true;
}
