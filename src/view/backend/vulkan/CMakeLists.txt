target_sources(psx PRIVATE
    window.cc
    context.cc
    device.cc
    swapchain.cc
    util.cc
)

target_sources(psx-test PRIVATE
    window.cc
    context.cc
    device.cc
    swapchain.cc
    util.cc
)

add_subdirectory(shaders)