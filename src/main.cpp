#include "vulkan_context.h"
#include "swapchain.h"
#include "pipeline.h"
#include "mesh.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <thread>
#include <chrono>

const int WIDTH = 800;
const int HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

class VulkanRenderer {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }
    
private:
    GLFWwindow* window = nullptr;
    VulkanContext context;
    Swapchain* swapchain = nullptr;
    Pipeline* pipeline = nullptr;
    Mesh* mesh = nullptr;
    
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;  // Track which fence is using each swapchain image
    uint32_t currentFrame = 0;
    bool framebufferResized = false;
    
    // Add throttling for resize events
    double lastResizeTime = 0.0;
    const double RESIZE_THROTTLE = 0.1; // 100ms minimum between recreations
    int resizeCount = 0;

    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Triangle", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }
    
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }
    
    void initVulkan() {
        std::cout << "Initializing Vulkan..." << std::endl;
        context.init(window);
        
        std::cout << "Creating swapchain..." << std::endl;
        swapchain = new Swapchain(&context, WIDTH, HEIGHT);
        swapchain->create();
        
        std::cout << "Loading shaders..." << std::endl;
        // Load compiled SPIR-V shaders (ensure they exist in ./shaders)
        pipeline = new Pipeline(&context, swapchain);
        try {
            pipeline->create("shaders/shader.vert.spv", "shaders/shader.frag.spv");
            std::cout << "Shaders loaded successfully!" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Failed to load shaders: " << e.what() << std::endl;
            std::cerr << "Make sure you compiled the shaders first!" << std::endl;
            std::cerr << "Run: cd shaders && glslangValidator -V shader.vert -o shader.vert.spv && glslangValidator -V shader.frag -o shader.frag.spv" << std::endl;
            throw;
        }
        
        createCommandBuffers();
        createSyncObjects();
        createTriangleMesh();
        
        // Initialize images in flight tracking
        imagesInFlight.resize(swapchain->getImageViews().size(), VK_NULL_HANDLE);
        
        std::cout << "Vulkan initialized." << std::endl;
    }
    
    void createTriangleMesh() {
        mesh = new Mesh(&context);
        // Simple RGB triangle
        std::vector<Vertex> vertices = {
            {{ 0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
            {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
            {{ 0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}}
        };

        
        mesh->setVertices(vertices);
        
        std::cout << "Created triangle mesh with " << vertices.size() << " vertices" << std::endl;
    }
    
    void createCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = context.getCommandPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();
        
        if (vkAllocateCommandBuffers(context.getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }
    
    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(context.getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(context.getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(context.getDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects!");
            }
        }
    }

    // NEW: make swapchain recreation robust by fully tearing down per-frame resources
    void destroySyncObjects() {
        for (size_t i = 0; i < imageAvailableSemaphores.size(); i++) {
            if (renderFinishedSemaphores[i])
                vkDestroySemaphore(context.getDevice(), renderFinishedSemaphores[i], nullptr);
            if (imageAvailableSemaphores[i])
                vkDestroySemaphore(context.getDevice(), imageAvailableSemaphores[i], nullptr);
            if (inFlightFences[i])
                vkDestroyFence(context.getDevice(), inFlightFences[i], nullptr);
        }
        imageAvailableSemaphores.clear();
        renderFinishedSemaphores.clear();
        inFlightFences.clear();
    }


    // NEW: free command buffers so old references can't linger across many recreations
    void freeCommandBuffers() {
        if (!commandBuffers.empty()) {
            vkFreeCommandBuffers(context.getDevice(), context.getCommandPool(),
                                 static_cast<uint32_t>(commandBuffers.size()),
                                 commandBuffers.data());
            commandBuffers.clear();
        }
    }

    
    void mainLoop() {
        std::cout << "Entering render loop..." << std::endl;
        
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            
            // Check for ESC key
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            
            drawFrame();
        }
        
        vkDeviceWaitIdle(context.getDevice());
        std::cout << "Exiting render loop..." << std::endl;
    }
    
    void drawFrame() {
        vkWaitForFences(context.getDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(context.getDevice(), swapchain->getSwapchain(), UINT64_MAX,
                                               imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            // Throttle resize events
            double currentTime = glfwGetTime();
            if (currentTime - lastResizeTime >= RESIZE_THROTTLE) {
                lastResizeTime = currentTime;
                recreateSwapchain();
            } else {
                // Avoid busy-spin when skipping a frame due to resize
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }
        
        // Check if a previous frame is using this image (wait for it to finish)
        if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(context.getDevice(), 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }
        // Mark the image as now being in use by this frame
        imagesInFlight[imageIndex] = inFlightFences[currentFrame];
        
        // Only reset fence after we know we will submit work
        vkResetFences(context.getDevice(), 1, &inFlightFences[currentFrame]);
        
        vkResetCommandBuffer(commandBuffers[currentFrame], 0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        
        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
        
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        
        if (vkQueueSubmit(context.getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
        
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        
        VkSwapchainKHR swapChains[] = {swapchain->getSwapchain()};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        
        result = vkQueuePresentKHR(context.getPresentQueue(), &presentInfo);
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            // Throttle resize events
            double currentTime = glfwGetTime();
            if (currentTime - lastResizeTime >= RESIZE_THROTTLE) {
                lastResizeTime = currentTime;
                recreateSwapchain();
            } else {
                // Avoid busy-spin when skipping a frame due to resize
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }
        
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
    
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }
        
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = swapchain->getRenderPass();
        renderPassInfo.framebuffer = swapchain->getFramebuffers()[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapchain->getExtent();
        
        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;
        
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipeline());
        
        mesh->draw(commandBuffer);
        
        vkCmdDraw(commandBuffer, mesh->getVertexCount(), 1, 0, 0);
        
        vkCmdEndRenderPass(commandBuffer);
        
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
    
    void recreateSwapchain() {
        // Debounce: wait for a short stable window (no size change)
        auto stable_for_ms = [&](int ms) {
            int w0, h0;
            glfwGetFramebufferSize(window, &w0, &h0);
            double start = glfwGetTime();
            while ((glfwGetTime() - start) * 1000.0 < ms) {
                int w1, h1;
                glfwGetFramebufferSize(window, &w1, &h1);
                if (w1 != w0 || h1 != h0) { // size changed; restart timer
                    w0 = w1; h0 = h1;
                    start = glfwGetTime();
                }
                glfwWaitEventsTimeout(0.01);
            }
        };
        stable_for_ms(75);

        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        resizeCount++;
        std::cout << "Recreating swapchain #" << resizeCount
                  << " (" << width << "x" << height << ")..." << std::endl;

        // 1) Ensure all GPU work referencing current swapchain is complete
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (i < inFlightFences.size() && inFlightFences[i])
                vkWaitForFences(context.getDevice(), 1, &inFlightFences[i], VK_TRUE, UINT64_MAX);
        }
        // IMPORTANT: make both queues idle (present can still be using old swapchain)
        vkQueueWaitIdle(context.getGraphicsQueue());
        vkQueueWaitIdle(context.getPresentQueue());

        // 2) Now it's safe to tear down per-frame resources bound to the old swapchain
        freeCommandBuffers();
        destroySyncObjects();

        // 3) Tear down pipeline (depends on render pass) and rebuild around new swapchain
        pipeline->cleanup();
        swapchain->recreate(width, height);

        // Recreate pipeline with new extent
        pipeline->create("shaders/shader.vert.spv", "shaders/shader.frag.spv");

        // Recreate per-frame resources
        createCommandBuffers();
        createSyncObjects();

        // Reset image tracking to match the new image count
        imagesInFlight.assign(swapchain->getImageViews().size(), VK_NULL_HANDLE);

        std::cout << "Swapchain recreation #" << resizeCount << " complete" << std::endl;
    }

    
    void cleanup() {
        // Make sure GPU is fully idle
        vkQueueWaitIdle(context.getGraphicsQueue());
        vkQueueWaitIdle(context.getPresentQueue());
        vkDeviceWaitIdle(context.getDevice());

        // Free cmd buffers explicitly (optional but tidy)
        freeCommandBuffers();

        for (size_t i = 0; i < imageAvailableSemaphores.size(); i++) {
            vkDestroySemaphore(context.getDevice(), renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(context.getDevice(), imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(context.getDevice(), inFlightFences[i], nullptr);
        }

        delete mesh;
        delete pipeline;
        delete swapchain;

        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main() {
    VulkanRenderer app;
    
    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "\n=== ERROR ===" << std::endl;
        std::cerr << e.what() << std::endl;
        std::cerr << "=============\n" << std::endl;
        return EXIT_FAILURE;
    }
    
    std::cout << "\nThank you for using Vulkan Shader Compiler!" << std::endl;
    return EXIT_SUCCESS;
}
