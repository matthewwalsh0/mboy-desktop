cmake_minimum_required(VERSION 2.8.9)
project(mboy_desktop)
set(CMAKE_CXX_STANDARD 14)

# mboy-core
set(LIB_CORE libmboy_core.a)
link_directories(../mboy-core/build)
include_directories(../mboy-core/include)

# minifb
set(LIB_MINIFB libminifb.a)
link_directories(../minifb)
include_directories(../minifb/include)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(mboy_desktop main.cpp)

target_link_libraries(mboy_desktop ${LIB_CORE} ${LIB_MINIFB} ${SDL2_LIBRARIES}
    "-framework Cocoa"
    "-framework QuartzCore"
    "-framework Metal"
    "-framework MetalKit"
)
