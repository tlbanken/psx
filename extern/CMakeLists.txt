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


