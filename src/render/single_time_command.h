#pragma once

#include <iostream>

#include <vulkan/vulkan.h>

bool begin_single_time_commands(VkCommandBuffer &command_buffer,
                                VkDevice &device, VkCommandPool &command_pool) {
  VkCommandBufferAllocateInfo cmd_allocate_info{};
  cmd_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmd_allocate_info.pNext = nullptr;
  cmd_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmd_allocate_info.commandPool = command_pool;
  cmd_allocate_info.commandBufferCount = 1;

  VkResult res =
      vkAllocateCommandBuffers(device, &cmd_allocate_info, &command_buffer);
  if (VK_SUCCESS != res) {
    std::cerr << "Failed to create single time command buffer: " << res
              << std::endl;
    return false;
  }

  VkCommandBufferBeginInfo cmd_begin_info{};
  cmd_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmd_begin_info.pNext = nullptr;
  cmd_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  res = vkBeginCommandBuffer(command_buffer, &cmd_begin_info);
  if (VK_SUCCESS != res) {
    std::cerr << "Failed to begin single time command buffer: " << res
              << std::endl;
    return false;
  }

  return true;
}

bool end_single_time_commands(VkCommandBuffer &command_buffer, VkDevice &device,
                              VkCommandPool &command_pool,
                              VkQueue &graphics_queue) {
  VkResult res = vkEndCommandBuffer(command_buffer);
  if (VK_SUCCESS != res) {
    std::cerr << "Failed to complete recording of single time command buffer: "
              << res << std::endl;
    return false;
  }

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;

  res = vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  if (VK_SUCCESS != res) {
    std::cerr
        << "Failed to submit single time command buffer to graphics queue: "
        << res << std::endl;
    return false;
  }

  res = vkQueueWaitIdle(graphics_queue);
  if (VK_SUCCESS != res) {
    std::cerr << "Failed to wait for graphics queue to complete execution "
                 "of single time command buffer: "
              << res << std::endl;
    return false;
  }

  vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);

  return true;
}
