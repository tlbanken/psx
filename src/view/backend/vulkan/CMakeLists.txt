target_sources(psx PRIVATE
    window.cc
    builder.cc
    render.cc
)

target_sources(psx-test PRIVATE
    window.cc
    builder.cc
    render.cc
)

add_subdirectory(shaders)