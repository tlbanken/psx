cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(PSX VERSION 0.0.1)


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
      set(PEDANTIC_COMPILE_FLAGS "${PEDANTIC_COMPILE_FLAGS} -Wshift-overflow=2 -Wnull-dereference -Wduplicated-cond -fsanitize=address")
    endif ()
    set(WERROR_FLAG -Werror)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(PEDANTIC_COMPILE_FLAGS "-Wall -Wextra -pedantic -Wconversion -Wundef -Wdeprecated -Wweak-vtables -Wshadow -fsanitize=address")
    set(WERROR_FLAG -Werror)
endif ()

if (MSVC)
    set(PEDANTIC_COMPILE_FLAGS /W3)
    set(WERROR_FLAG /WX)
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PEDANTIC_COMPILE_FLAGS}")

# define exe
add_executable(psx)
add_executable(psx-test)

# Subdirs
add_subdirectory("${PROJECT_SOURCE_DIR}/src")
add_subdirectory("${PROJECT_SOURCE_DIR}/extern")
add_subdirectory("${PROJECT_SOURCE_DIR}/tests")

target_include_directories(psx PRIVATE "${PROJECT_SOURCE_DIR}/src" "${PROJECT_SOURCE_DIR}/extern/include")
target_include_directories(psx-test PRIVATE "${PROJECT_SOURCE_DIR}/src" "${PROJECT_SOURCE_DIR}/extern/include")

target_link_libraries(psx fmt)
target_link_libraries(psx-test fmt)


