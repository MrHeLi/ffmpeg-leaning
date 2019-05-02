cmake_minimum_required(VERSION 3.10)
project(PlaySDL)

set(CMAKE_CXX_STANDARD 11)
set(SDL_DIR /usr/local/Cellar/sdl2/2.0.9_1/)
include_directories(${SDL_DIR}/include/)
link_libraries(${SDL_DIR}/lib/)

add_executable(PlaySDL main.cpp)

target_link_libraries(PlaySDL SDL2 SDL2_test SDL2main)