cmake_minimum_required(VERSION 2.8.9)
project(mboy_desktop)
set(CMAKE_CXX_STANDARD 14)

# mboy-core
set(LIB_CORE libmboy_core.a)
link_directories(../mboy-core/build)
include_directories(../mboy-core/include)

# imgui
include_directories(../imgui/)
include_directories(../imgui/examples/)
file(GLOB IMGUI "../imgui/*.cpp")

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(mboy_desktop
    ${IMGUI}
    ../imgui/examples/imgui_impl_sdl.cpp
    ../imgui/examples/imgui_impl_opengl2.cpp
    main.cpp)

target_link_libraries(mboy_desktop ${LIB_CORE} ${SDL2_LIBRARIES} "-framework OpenGl")
