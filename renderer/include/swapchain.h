#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class VulkanContext;

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Swapchain {
public:
    Swapchain(VulkanContext* context, int width, int height);
    ~Swapchain();
    
    void create();
    void cleanup();
    void recreate(int width, int height);
    
    VkSwapchainKHR getSwapchain() const { return swapchain; }
    VkFormat getImageFormat() const { return swapchainImageFormat; }
    VkExtent2D getExtent() const { return swapchainExtent; }
    const std::vector<VkImageView>& getImageViews() const { return swapchainImageViews; }
    const std::vector<VkFramebuffer>& getFramebuffers() const { return swapchainFramebuffers; }
    VkRenderPass getRenderPass() const { return renderPass; }
    
    void createFramebuffers();
    
private:
    void createSwapchain();
    void createSwapchainWithOld(VkSwapchainKHR oldSwapchain);
    void createImageViews();
    void createRenderPass();
    
    SwapChainSupportDetails querySwapChainSupport();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    
    VulkanContext* context;
    int width;
    int height;
    
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> swapchainFramebuffers;
    VkRenderPass renderPass = VK_NULL_HANDLE;
};