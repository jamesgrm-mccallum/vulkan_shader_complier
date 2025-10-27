#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

class VulkanContext;
class Swapchain;

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    
    static VkVertexInputBindingDescription getBindingDescription();
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
};

class Pipeline {
public:
    Pipeline(VulkanContext* context, Swapchain* swapchain);
    ~Pipeline();
    
    void create(const std::string& vertShaderPath, const std::string& fragShaderPath);
    void cleanup();
    
    VkPipeline getPipeline() const { return graphicsPipeline; }
    VkPipelineLayout getLayout() const { return pipelineLayout; }
    
private:
    VulkanContext* context;
    Swapchain* swapchain;
    
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
};