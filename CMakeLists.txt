cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(PSX VERSION 0.0.3)

set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE Release)

# Pedantic compile flags for gcc, clang, msvc
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(PEDANTIC_COMPILE_FLAGS "-pedantic-errors -Wall -Wextra -pedantic -Wold-style-cast -Wundef -Wredundant-decls -Wwrite-strings -Wpointer-arith -Wcast-qual -Wformat=2 -Wmissing-include-dirs -Wcast-align -Wctor-dtor-privacy -Wdisabled-optimization -Winvalid-pch -Woverloaded-virtual -Wconversion -Wswitch-enum -Wundef -Wno-ctor-dtor-privacy -Wno-format-nonliteral")
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.6)
      set(PEDANTIC_COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS} -Wnoexcept -Wno-dangling-else -Wno-unused-local-typedefs")
    endif ()
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
      set(PEDANTIC_COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS} -Wdouble-promotion -Wtrampolines -Wzero-as-null-pointer-constant -Wuseless-cast -Wvector-operation-performance -Wsized-deallocation -Wshadow")
    endif ()
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
      set(PEDANTIC_COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS} -Wshift-overflow=2 -Wnull-dereference -Wduplicated-cond")
      set(ADDR_SANITIZE_FLAGS "-fsanitize=address")
    endif ()
    set(WERROR_FLAG -Werror)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(PEDANTIC_COMPILE_FLAGS "-Wall -Wextra -pedantic -Wconversion -Wundef -Wdeprecated -Wweak-vtables -Wshadow")
    set(ADDR_SANITIZE_FLAGS "-fsanitize=address")
    set(WERROR_FLAG -Werror)
endif ()

if (MSVC)
    set(PEDANTIC_COMPILE_FLAGS /W3)
    set(WERROR_FLAG /WX)
endif ()

# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${PEDANTIC_COMPILE_FLAGS} ${ADDR_SANITIZE_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${PEDANTIC_COMPILE_FLAGS} -DPSX_DEBUG -DPSX_LOGGING")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Generate Compile Commands for Language server
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# define exe
add_executable(psx)
add_executable(psx-test)

add_custom_command(
        TARGET psx POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
                ${PROJECT_SOURCE_DIR}/compile_commands.json)

# Subdirs
add_subdirectory("${PROJECT_SOURCE_DIR}/src")
add_subdirectory("${PROJECT_SOURCE_DIR}/extern")
add_subdirectory("${PROJECT_SOURCE_DIR}/tests")

# configure
configure_file(src/core/config.h.in "${PROJECT_SOURCE_DIR}/src/core/config.h")

target_include_directories(psx PRIVATE "${PROJECT_SOURCE_DIR}/src" "${PROJECT_SOURCE_DIR}/extern/include")
target_include_directories(psx-test PRIVATE "${PROJECT_SOURCE_DIR}/src" "${PROJECT_SOURCE_DIR}/extern/include")

target_link_libraries(psx fmt imgui glfw glad)
target_link_libraries(psx-test fmt imgui glfw glad)


