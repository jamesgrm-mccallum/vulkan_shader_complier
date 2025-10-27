#include "mesh.h"
#include "vulkan_context.h"

Mesh::Mesh(VulkanContext* ctx) : context(ctx) {}

Mesh::~Mesh() {}

void Mesh::setVertices(const std::vector<Vertex>& verts) {
    vertexCount = static_cast<uint32_t>(verts.size());
    createVertexBuffer(verts);
}

void Mesh::setIndices(const std::vector<uint16_t>& inds) {
    indexCount = static_cast<uint32_t>(inds.size());
    createIndexBuffer(inds);
}

void Mesh::draw(VkCommandBuffer commandBuffer) {
    VkBuffer vertexBuffers[] = {vertexBuffer->getBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    
    if (indexCount > 0) {
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
    } else {
        vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
    }
}

void Mesh::createVertexBuffer(const std::vector<Vertex>& vertices) {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    
    // Create staging buffer
    Buffer stagingBuffer(context);
    stagingBuffer.create(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    stagingBuffer.copyData(vertices.data(), bufferSize);
    
    // Create device local buffer
    vertexBuffer = std::make_unique<Buffer>(context);
    vertexBuffer->create(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    vertexBuffer->copyFrom(stagingBuffer, bufferSize);
    
    // Explicitly clean up staging buffer
    stagingBuffer.cleanup();
}

void Mesh::createIndexBuffer(const std::vector<uint16_t>& indices) {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    
    // Create staging buffer
    Buffer stagingBuffer(context);
    stagingBuffer.create(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    stagingBuffer.copyData(indices.data(), bufferSize);
    
    // Create device local buffer
    indexBuffer = std::make_unique<Buffer>(context);
    indexBuffer->create(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    indexBuffer->copyFrom(stagingBuffer, bufferSize);
    
    // Explicitly clean up staging buffer
    stagingBuffer.cleanup();
}