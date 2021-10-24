target_sources(psx PRIVATE
    window.cc
    builder.cc
    vertex_buffer.cc
)

target_sources(psx-test PRIVATE
    window.cc
    builder.cc
    vertex_buffer.cc
)

add_subdirectory(shaders)