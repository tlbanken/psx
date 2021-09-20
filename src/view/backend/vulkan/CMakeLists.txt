target_sources(psx PRIVATE
    window.cc
    renderer.cc
    device.cc
    swapchain.cc
    pipeline.cc
    util.cc
    builder.cc
)

target_sources(psx-test PRIVATE
    window.cc
    renderer.cc
    device.cc
    swapchain.cc
    pipeline.cc
    util.cc
    builder.cc
)

add_subdirectory(shaders)