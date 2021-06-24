/*
 * vulkan.hh
 *
 * Travis Banken
 * 6/21/21
 *
 * Vulkan backend for renderering.
 */

#pragma once

#include <vector>

namespace Psx {
namespace Vulkan {
    
void Init(std::vector<const char *> extensions);
void Shutdown();

}// end ns
}
