#include "shader_loader.h"
#include "vulkan_context.h"
#include <fstream>
#include <stdexcept>
#include <cstring>

ShaderLoader::ShaderLoader(VulkanContext* ctx) : context(ctx) {}

ShaderLoader::~ShaderLoader() {}

VkShaderModule ShaderLoader::loadShaderModule(const std::string& filename) {
    auto code = readFile(filename);
    
    // Convert char vector to uint32_t vector safely
    if (code.size() % sizeof(uint32_t) != 0) {
        throw std::runtime_error("SPIR-V file size is not 4-byte aligned: " + filename);
    }
    const uint32_t* p = reinterpret_cast<const uint32_t*>(code.data());
    size_t wordCount = code.size() / sizeof(uint32_t);
    std::vector<uint32_t> codeUint(p, p + wordCount);
    return createShaderModule(codeUint);
}

VkShaderModule ShaderLoader::createShaderModule(const std::vector<uint32_t>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(context->getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    
    return shaderModule;
}

void ShaderLoader::destroyShaderModule(VkShaderModule module) {
    vkDestroyShaderModule(context->getDevice(), module, nullptr);
}

std::vector<char> ShaderLoader::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + filename);
    }
    
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    
    return buffer;
}