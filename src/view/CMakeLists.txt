add_subdirectory(imgui)
add_subdirectory(backend)

target_sources(psx PRIVATE
    view.cc
)

target_sources(psx-test PRIVATE
    view.cc
)