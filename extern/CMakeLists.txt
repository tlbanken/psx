# ==============================
# VULKAN
# ==============================
message(STATUS "Looking for Vulkan...")
find_package(Vulkan)
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Failed to find Vulkan. Verify that Vulkan SDK is installed and supported.")
    return()
endif()

# TODO SPIR-V shader tool

# ==============================
# fmtlib/fmt library
# ==============================
add_library(fmt STATIC
    fmt/src/format.cc
    fmt/src/os.cc
)

target_include_directories(fmt PRIVATE "${PROJECT_SOURCE_DIR}/extern/fmt/include")

set(FMT_HDRS
    ${PROJECT_SOURCE_DIR}/extern/fmt/include/fmt/args.h
    ${PROJECT_SOURCE_DIR}/extern/fmt/include/fmt/chrono.h
    ${PROJECT_SOURCE_DIR}/extern/fmt/include/fmt/color.h
    ${PROJECT_SOURCE_DIR}/extern/fmt/include/fmt/compile.h
    ${PROJECT_SOURCE_DIR}/extern/fmt/include/fmt/core.h
    ${PROJECT_SOURCE_DIR}/extern/fmt/include/fmt/format.h
    ${PROJECT_SOURCE_DIR}/extern/fmt/include/fmt/format-inl.h
    ${PROJECT_SOURCE_DIR}/extern/fmt/include/fmt/locale.h
    ${PROJECT_SOURCE_DIR}/extern/fmt/include/fmt/os.h
    ${PROJECT_SOURCE_DIR}/extern/fmt/include/fmt/ostream.h
    ${PROJECT_SOURCE_DIR}/extern/fmt/include/fmt/printf.h
    ${PROJECT_SOURCE_DIR}/extern/fmt/include/fmt/ranges.h
    ${PROJECT_SOURCE_DIR}/extern/fmt/include/fmt/xchar.h
)

# Copy headers to /extern/include/fmt dir
file(COPY ${FMT_HDRS} DESTINATION "${PROJECT_SOURCE_DIR}/extern/include/fmt")

# ==============================
# GLFW
# ==============================
# GLFW DOES NOT WORK FOR M1 MACS :(
# add_subdirectory(glfw)

# set(GLFW_HDRS
#     ${PROJECT_SOURCE_DIR}/extern/glfw/include/GLFW/glfw3.h
# )

# # Copy headers to extern/include/glfw
# file(COPY ${GLFW_HDRS} DESTINATION "${PROJECT_SOURCE_DIR}/extern/include/glfw")

# ==============================
# SDL2
# ==============================
add_subdirectory(SDL) # link against SDL2-static

# ==============================
# Dear ImGui Library
# ==============================
add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_vulkan.cpp
    imgui/backends/imgui_impl_sdl.cpp
)

target_include_directories(imgui PRIVATE "${PROJECT_SOURCE_DIR}/extern/imgui/")
target_include_directories(imgui PRIVATE "${PROJECT_SOURCE_DIR}/extern/imgui/backends")
target_include_directories(imgui PRIVATE "${PROJECT_SOURCE_DIR}/extern/SDL/include")
target_include_directories(imgui PRIVATE ${Vulkan_INCLUDE_DIRS})

set(IMGUI_HDRS
    ${PROJECT_SOURCE_DIR}/extern/imgui/imgui.h
    ${PROJECT_SOURCE_DIR}/extern/imgui/backends/imgui_impl_vulkan.h
    # ${PROJECT_SOURCE_DIR}/extern/imgui/backends/imgui_impl_glfw.h
    ${PROJECT_SOURCE_DIR}/extern/imgui/backends/imgui_impl_sdl.h
)

# Copy headers to extern/include/imgui
file(COPY ${IMGUI_HDRS} DESTINATION "${PROJECT_SOURCE_DIR}/extern/include/imgui")


# ==============================
# GLM
# ==============================
add_subdirectory(glm)
