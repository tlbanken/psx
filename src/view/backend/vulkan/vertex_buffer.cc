/*
 * vertex_buffer.cc
 *
 * Travis Banken
 * 10/23/2021
 * 
 * Vertex Buffer class for vulkan.
 */

#include "vertex_buffer.hh"

#define VBUFFER_INFO(...) PSXLOG_INFO("Vulkan Buffer", __VA_ARGS__)
#define VBUFFER_WARN(...) PSXLOG_WARN("Vulkan Buffer", __VA_ARGS__)
#define VBUFFER_ERROR(...) PSXLOG_ERROR("Vulkan Buffer", __VA_ARGS__)
#define VBUFFER_FATAL(...) VBUFFER_ERROR(__VA_ARGS__); throw std::runtime_error(PSX_FMT(__VA_ARGS__))

namespace Psx {
namespace Vulkan {

VertexBuffer::VertexBuffer() {}

VertexBuffer::VertexBuffer(VkDevice device, VkPhysicalDevice physical_device, 
                           size_t size, VkAllocationCallbacks *allocator)
{
    initialize(device, physical_device, size, allocator);
}

void VertexBuffer::Resize(VkDevice device, VkPhysicalDevice physical_device,
                          size_t size, VkAllocationCallbacks *allocator)
{
    Destroy(device, allocator);
    initialize(device, physical_device, size, allocator);
}

void VertexBuffer::Destroy(VkDevice device, VkAllocationCallbacks *allocator)
{
    VBUFFER_INFO("Destroying buffer with {} vertices", m_vertices.size());
    vkUnmapMemory(device, m_buffer_memory);
    vkDestroyBuffer(device, m_buffer, allocator);
    vkFreeMemory(device, m_buffer_memory, allocator);
}

bool VertexBuffer::SetVertex(const Vertex& vert, size_t index)
{
    if (index >= m_vertices.size()) {
        return false;
    }
    m_vertices[index] = vert;
    return true;
}

Vertex& VertexBuffer::At(size_t index)
{
    PSX_ASSERT(index < m_vertices.size());
    return m_vertices[index];
}

/*
 * Attempt to add a vertext to the buffer. If there is no room, signal
 * to the caller that it is time to flush the buffer.
 */
bool VertexBuffer::PushVertex(const Vertex& vert)
{
    if (m_next_index >= m_vertices.size()) {
        return false;
    }
    m_vertices[m_next_index++] = vert;
    return true;
}

void VertexBuffer::Flush()
{
    size_t size = m_vertices.size();
    m_vertices.clear();
    m_vertices.resize(size);
    // TODO: may need to "zero" out the vectors
    m_next_index = 0;
}

/*
 * Draw the vertex buffer out to the command buffer.
 */
void VertexBuffer::Draw(VkCommandBuffer command_buffer)
{
    // TODO: Only do this on vertex change
    memcpy(m_raw_memory, m_vertices.data(), m_next_index * sizeof(Vertex));

    VkBuffer vertex_buffers[] = {m_buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);

    vkCmdDraw(command_buffer, (u32)m_next_index, 1, 0, 0);
}

// *** PRIVATE METHODS ***

void VertexBuffer::initialize(VkDevice device, VkPhysicalDevice physical_device,
            size_t size, VkAllocationCallbacks *allocator)
{
    VBUFFER_INFO("Creating buffer for {} vertices.", size);
    m_vertices.resize(size);
    m_next_index = 0;

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = sizeof(Vertex) * size;
    buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult res = vkCreateBuffer(device, &buffer_info, allocator, &m_buffer);
    if (res != VK_SUCCESS) {
        VBUFFER_FATAL("Failed to create vertex buffer. [rc: {}]", res);
    }

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(device, m_buffer, &mem_reqs);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = findMemoryType(
        physical_device,
        mem_reqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    // allocate the memory
    res = vkAllocateMemory(device, &alloc_info, allocator, &m_buffer_memory);
    if (res != VK_SUCCESS) {
        VBUFFER_FATAL("Failed to allocate vertex buffer memory. [rc: %d]", res);
    }

    vkBindBufferMemory(device, m_buffer, m_buffer_memory, 0);

    // map the memory for later drawing
    vkMapMemory(device, m_buffer_memory, 0, buffer_info.size, 0, &m_raw_memory);
}

u32 VertexBuffer::findMemoryType(VkPhysicalDevice dev, u32 type_filter, VkMemoryMapFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(dev, &mem_props);

    for (u32 i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (mem_props.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    VBUFFER_FATAL("Failed to find suitable device memory type!");
    return (u32)-1;
}

} // end ns
}