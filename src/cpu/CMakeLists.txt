add_subdirectory(asm)

target_sources(psx PRIVATE
    cpu.cpp
    cop0.cpp
    cache.cpp
)

target_sources(psx-test PRIVATE
    cpu.cpp
    cop0.cpp
    cache.cpp
)

