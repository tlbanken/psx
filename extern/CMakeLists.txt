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
    ${PROJECT_SOURCE_DIR}/extern/fmt/include/fmt/posix.h
    ${PROJECT_SOURCE_DIR}/extern/fmt/include/fmt/printf.h
    ${PROJECT_SOURCE_DIR}/extern/fmt/include/fmt/ranges.h
)

# Copy headers to /extern/include/fmt dir
file(COPY ${FMT_HDRS} DESTINATION "${PROJECT_SOURCE_DIR}/extern/include/fmt")

# ==============================
# GLFW
# ==============================
add_subdirectory(glfw)

set(GLFW_HDRS
    ${PROJECT_SOURCE_DIR}/extern/glfw/include/GLFW/glfw3.h
)

# Copy headers to extern/include/glfw
file(COPY ${GLFW_HDRS} DESTINATION "${PROJECT_SOURCE_DIR}/extern/include/glfw")

# ==============================
# GLAD
# ==============================
add_library(glad STATIC
    glad/src/glad.c
)

target_include_directories(glad PRIVATE "${PROJECT_SOURCE_DIR}/extern/glad/include/")

set(GLAD_HDRS
    ${PROJECT_SOURCE_DIR}/extern/glad/include/glad/glad.h
)

# Copy headers to extern/include/glad
file(COPY ${GLAD_HDRS} DESTINATION "${PROJECT_SOURCE_DIR}/extern/include/glad")

# ==============================
# Dear ImGui Library
# ==============================
add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_opengl3.cpp
    imgui/backends/imgui_impl_glfw.cpp
)

# use GLAD
add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLAD)

target_include_directories(imgui PRIVATE "${PROJECT_SOURCE_DIR}/extern/imgui/")
target_include_directories(imgui PRIVATE "${PROJECT_SOURCE_DIR}/extern/imgui/backends")
target_include_directories(imgui PRIVATE "${PROJECT_SOURCE_DIR}/extern/glad/include/")
target_include_directories(imgui PRIVATE "${PROJECT_SOURCE_DIR}/extern/glfw/include/")

set(IMGUI_HDRS
    ${PROJECT_SOURCE_DIR}/extern/imgui/imgui.h
    ${PROJECT_SOURCE_DIR}/extern/imgui/imconfig.h
    ${PROJECT_SOURCE_DIR}/extern/imgui/imstb_rectpack.h
    ${PROJECT_SOURCE_DIR}/extern/imgui/imstb_textedit.h
    ${PROJECT_SOURCE_DIR}/extern/imgui/imstb_truetype.h
    ${PROJECT_SOURCE_DIR}/extern/imgui/backends/imgui_impl_opengl3.h
    ${PROJECT_SOURCE_DIR}/extern/imgui/backends/imgui_impl_glfw.h
)

# Copy headers to extern/include/imgui
file(COPY ${IMGUI_HDRS} DESTINATION "${PROJECT_SOURCE_DIR}/extern/include/imgui")



