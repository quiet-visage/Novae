cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_COMPILER gcc)

project(novae C)

file(GLOB_RECURSE SRCS "src/*.c" "src/*.h" "src/*/*.h" "src/*/*.c")
add_executable(${PROJECT_NAME} ${SRCS})

set(CMAKE_C_FLAGS_SANITIZE
    ${CMAKE_C_FLAGS_DEBUG}
    CACHE STRING "Flags used by the C compiler during sanitize builds." FORCE)
set(CMAKE_EXE_LINKER_FLAGS_SANITIZE
    ${CMAKE_EXE_LINKER_FLAGS_DEBUG}
    CACHE STRING "Flags used for linking binaries during sanitize builds."
          FORCE)

set(CMAKE_C_FLAGS_Profile
    "-O2 -pg"
    CACHE STRING "Flags used by the C compiler during profile builds." FORCE)
set(CMAKE_EXE_LINKER_FLAGS_Profile
    "-pg"
    CACHE STRING "Flags used for linking binaries during profile builds."
          FORCE)

message(
  "* build type options are : None Debug Release RelWithDebInfo MinSizeRel Sanitize"
)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Sanitize")
  target_compile_options(${PROJECT_NAME} PRIVATE -O0 -g3 -fsanitize=address)
  target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
endif()


set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 23)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall )

set(PLATFORM Desktop)
set(GRAPHICS GRAPHICS_API_OPENGL_43)
option(BUILD_EXAMPLES OFF)
option(CUSTOMIZE_BUILD ON)
option(SUPPORT_FILEFORMAT_SVG ON)

add_subdirectory(external/raylib-5.0)
add_subdirectory(external/field_fusion)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib field_fusion sqlite3)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders/ DESTINATION shaders)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION resources)
