add_subdirectory(asm)

target_sources(psx PRIVATE
    cpu.cc
    cop0.cc
)

target_sources(psx-test PRIVATE
    cpu.cc
    cop0.cc
)

