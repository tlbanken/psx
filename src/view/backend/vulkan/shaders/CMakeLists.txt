function(compile_shader TARGET SHADER)
    find_program(GLSLC glslc)
    message(STATUS "GLSL Compiler found at ${GLSLC}")

    get_filename_component(FILE_NAME ${SHADER} NAME)

    set(shader-path ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER})
    set(output-path ${CMAKE_CURRENT_SOURCE_DIR}/${FILE_NAME}.spv)

    # Add a custom command to compile GLSL to SPIR-V.
    add_custom_command(
           OUTPUT ${output-path}
           COMMAND ${GLSLC} -o ${output-path} ${shader-path}
           DEPENDS ${shader-path}
           IMPLICIT_DEPENDS CXX ${shader-path}
           VERBATIM)

    # Make sure our build depends on this output.
    # set_source_files_properties(${output-path} PROPERTIES GENERATED TRUE)
    # target_sources(${TARGET} PRIVATE ${output-path})
endfunction(compile_shader)


# add the sources as a dependacy to our main target
set(SHADER_SRC 
    shader.frag
    shader.vert
)

foreach(SHADER ${SHADER_SRC})
    message(STATUS "Compiling shader: ${SHADER}")
    compile_shader(psx ${SHADER})
    compile_shader(psx-test ${SHADER})
endforeach()


# custom clean and build
set(SHADER_SPIRV 
    shader.frag.spv
    shader.vert.spv
)
add_custom_target(shaders ALL DEPENDS ${SHADER_SPIRV})
set_directory_properties(ADDITIONAL_MAKE_CLEAN_FILES ${SHADER_SPIRV})