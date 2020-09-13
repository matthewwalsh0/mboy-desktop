cmake_minimum_required(VERSION 3.10.2)
project(mboy_desktop)
set(CMAKE_CXX_STANDARD 17)

# mboy-core
add_subdirectory(deps/mboy-core)

# argparse
include_directories(deps/argparse/include/argparse)

# sdl
add_subdirectory(deps/sdl)

# imgui
include_directories(deps/imgui)
include_directories(deps/imgui/examples)
file(GLOB IMGUI
    "deps/imgui/*.cpp",
    "deps/imgui/examples/imgui_impl_sdl.cpp",
    "deps/imgui/examples/imgui_impl_opengl2.cpp")

file(GLOB SOURCES "src/*.cpp" "src/*.h")
add_executable(mboy_desktop ${IMGUI} ${SOURCES})
target_link_libraries(mboy_desktop mboy_core SDL2 "-framework OpenGl")
