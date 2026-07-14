cmake_minimum_required(VERSION 3.29)
project(OOP)

set(CMAKE_CXX_STANDARD 20)
set(SDL2_DIR "E:/x86_64-w64-mingw32")
set(SDL2_TTF_DIR "E:/sdl.ttf/x86_64-w64-mingw32")

include_directories(
        "${SDL2_DIR}/include"
        "${SDL2_DIR}/include/SDL2"
        "${SDL2_TTF_DIR}/include"
        "${SDL2_TTF_DIR}/include/SDL2"
)

link_directories(
        "${SDL2_DIR}/lib"
        "${SDL2_TTF_DIR}/lib"
)

add_executable(OOP OOP_project.cpp)

target_link_libraries(OOP mingw32 SDL2main SDL2 SDL2_ttf)