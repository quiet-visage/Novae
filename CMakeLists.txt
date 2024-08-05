cmake_minimum_required(VERSION 3.29)
set(CMAKE_CXX_COMPILER gcc)

project(novae C)

file(GLOB_RECURSE SRCS "src/*.c" "src/*.h" "src/*/*.h" "src/*/*.c")
add_executable(${PROJECT_NAME} ${SRCS})


set(CMAKE_C_FLAGS_PROFILE
  "-pg -g3")
set(CMAKE_EXE_LINKER_FLAGS_PROFILE
  "-pg")

set(CMAKE_C_FLAGS_SANITIZE
  "-g3 -fsanitize=address")
set(CMAKE_EXE_LINKER_FLAGS_SANITIZE
  "-fsanitize=address")

set(CMAKE_C_FLAGS_RELEASE
  "-O3")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE)

mark_as_advanced(
  CMAKE_C_FLAGS_PROFILE
  CMAKE_EXE_LINKER_FLAGS_PROFILE
  CMAKE_C_FLAGS_SANITIZE
  CMAKE_EXE_LINKER_FLAGS_SANITIZE
  CMAKE_C_FLAGS_RELEASE
  CMAKE_EXE_LINKER_FLAGS_RELEASE
)

message(
  "* build type options are : None Debug Release  Sanitize Profile"
)

message("* build type is : ${CMAKE_BUILD_TYPE}")


set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 23)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall )

set(PLATFORM Desktop)
# set(GRAPHICS GRAPHICS_API_OPENGL_43)
option(BUILD_EXAMPLES OFF)
option(CUSTOMIZE_BUILD ON)
option(SUPPORT_FILEFORMAT_SVG ON)

add_subdirectory(external/raylib-5.0)
add_subdirectory(external/field_fusion)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib field_fusion sqlite3)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders/ DESTINATION shaders)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/ DESTINATION resources)
