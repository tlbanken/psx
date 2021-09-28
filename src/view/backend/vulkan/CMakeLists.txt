target_sources(psx PRIVATE
    window.cc
    builder.cc
)

target_sources(psx-test PRIVATE
    window.cc
    builder.cc
)

add_subdirectory(shaders)