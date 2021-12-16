/*
 * vertex_buffer.hh
 *
 * Travis Banken
 * 10/23/2021
 * 
 * Vertex Buffer class for vulkan.
 */
#pragma once

#include "util/psxutil.hh"

#include <array>
#include <vector>

#include <glm/glm.hpp>

#include "includes.hh"

namespace Psx {
namespace Vulkan {

struct Vertex {
    glm::vec2 pos;
    glm::vec3 col;

    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription binding_description{};
        binding_description.binding = 0;
        binding_description.stride = sizeof(Vertex);
        binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return binding_description;
    }

    static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions{};

        // Position
        attribute_descriptions[0].binding = 0;
        attribute_descriptions[0].location = 0;
        attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // vec2 float
        attribute_descriptions[0].offset = offsetof(Vertex, pos);

        // Color
        attribute_descriptions[1].binding = 0;
        attribute_descriptions[1].location = 1;
        attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 float
        attribute_descriptions[1].offset = offsetof(Vertex, col);


        return attribute_descriptions;
    }
};

class VertexBuffer {
public:
    VertexBuffer();
    VertexBuffer(VkDevice device, VkPhysicalDevice physical_device,
                size_t size, VkAllocationCallbacks *allocator);
    ~VertexBuffer() {}

    void Resize(VkDevice device, VkPhysicalDevice physical_device,
                size_t new_size, VkAllocationCallbacks *allocator);
    void Destroy(VkDevice device, VkAllocationCallbacks *allocator);
    bool SetVertex(const Vertex& vert, size_t index);
    bool PushVertex(const Vertex& vert);
    void Flush();
    void Draw(VkCommandBuffer command_buffer);
    void Clear();
    Vertex& At(size_t index);

private:
    u32 findMemoryType(VkPhysicalDevice dev, u32 type_filter, VkMemoryMapFlags properties);
    void initialize(VkDevice device, VkPhysicalDevice physical_device,
                size_t size, VkAllocationCallbacks *allocator);

    std::vector<Vertex> m_vertices;
    void *m_raw_memory;
    VkBuffer m_buffer;
    VkDeviceMemory m_buffer_memory;
    size_t m_next_index;
};

} // end ns
}