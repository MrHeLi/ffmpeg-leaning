cmake_minimum_required(VERSION 3.10)
project(PlaySDL)

set(CMAKE_CXX_STANDARD 11)
set(MY_LIBRARY_DIR /usr/local/Cellar)
set(SDL_DIR ${MY_LIBRARY_DIR}/sdl2/2.0.9_1/)
set(SDL_IMAGE_DIR ${MY_LIBRARY_DIR}/sdl2_image/2.0.4/)

include_directories(${SDL_DIR}/include/SDL2/
        ${SDL_IMAGE_DIR}/include/)

link_libraries(${SDL_DIR}/lib/
        ${SDL_IMAGE_DIR}/lib/)

add_executable(PlaySDL main.cpp)

target_link_libraries(PlaySDL SDL2 SDL2_image)