target_sources(psx PRIVATE
    vulkan.cc
    vulkan_instance.cc
    utils.cc
)

target_sources(psx-test PRIVATE
    vulkan.cc
    vulkan_instance.cc
    utils.cc
)

add_subdirectory(shaders)