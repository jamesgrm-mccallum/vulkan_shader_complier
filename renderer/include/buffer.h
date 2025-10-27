#pragma once

#include <vulkan/vulkan.h>

class VulkanContext;

class Buffer {
public:
    Buffer(VulkanContext* context);
    ~Buffer();
    
    void create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void cleanup();
    
    void copyData(const void* data, VkDeviceSize size);
    void copyFrom(Buffer& srcBuffer, VkDeviceSize size);
    
    VkBuffer getBuffer() const { return buffer; }
    VkDeviceMemory getMemory() const { return bufferMemory; }
    
private:
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    
    VulkanContext* context;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
};