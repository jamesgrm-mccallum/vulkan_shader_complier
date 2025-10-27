#pragma once

#include "buffer.h"
#include "pipeline.h"
#include <vector>
#include <memory>

class VulkanContext;

class Mesh {
public:
    Mesh(VulkanContext* context);
    ~Mesh();
    
    void setVertices(const std::vector<Vertex>& verts);
    void setIndices(const std::vector<uint16_t>& inds);
    
    void draw(VkCommandBuffer commandBuffer);
    
    uint32_t getVertexCount() const { return vertexCount; }
    uint32_t getIndexCount() const { return indexCount; }
    
private:
    void createVertexBuffer(const std::vector<Vertex>& vertices);
    void createIndexBuffer(const std::vector<uint16_t>& indices);
    
    VulkanContext* context;
    
    std::unique_ptr<Buffer> vertexBuffer;
    std::unique_ptr<Buffer> indexBuffer;
    
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
};