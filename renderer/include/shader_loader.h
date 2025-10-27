#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

class VulkanContext;

class ShaderLoader {
public:
    ShaderLoader(VulkanContext* context);
    ~ShaderLoader();
    
    // Load compiled SPIR-V shader from file
    VkShaderModule loadShaderModule(const std::string& filename);
    
    // Load SPIR-V from memory (used with custom compiler output)
    VkShaderModule createShaderModule(const std::vector<uint32_t>& code);
    
    void destroyShaderModule(VkShaderModule module);
    
private:
    std::vector<char> readFile(const std::string& filename);
    
    VulkanContext* context;
};