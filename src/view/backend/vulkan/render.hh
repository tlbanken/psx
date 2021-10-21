/*
 * render.hh
 *
 * Travis Banken
 * 10/10/2021
 * 
 * Renderer helper functions for vulkan rendering.
 */
#pragma once

#include <array>

#include <glm/glm.hpp>

#include "view/backend/vulkan/includes.hh"

namespace Psx {
namespace Vulkan {
namespace Render {

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

} // end ns
}
}
