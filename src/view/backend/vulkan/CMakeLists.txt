target_sources(psx PRIVATE
    window.cc
    builder.cc
    vertex_buffer.cc
    vram.cc
)

target_sources(psx-test PRIVATE
    window.cc
    builder.cc
    vertex_buffer.cc
    vram.cc
)

add_subdirectory(shaders)