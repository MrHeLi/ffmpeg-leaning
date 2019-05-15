cmake_minimum_required(VERSION 3.10)
project(PlayFFmpeg) # 指定的项目名称
set(CMAKE_CXX_STANDARD 11)

set(MY_LIBRARY_DIR /usr/local/Cellar)
set(FFMPEG_DIR ${MY_LIBRARY_DIR}/ffmpeg/4.1.3_1) # FFmpeg的安装目录，可以通过命令"brew info ffmpeg"获取
set(SDL_DIR ${MY_LIBRARY_DIR}/sdl2/2.0.9_1/)
find_package(Threads REQUIRED)


include_directories(${FFMPEG_DIR}/include/
        ${SDL_DIR}/include/) # 头文件搜索路径
link_directories(${FFMPEG_DIR}/lib/
        ${SDL_DIR}/lib/) # 动态链接库或静态链接库的搜索路径

add_executable(PlayFFmpeg main.cpp)

# 添加链接库
target_link_libraries(
        PlayFFmpeg
        avcodec
        avdevice
        avfilter
        avformat

        SDL2
)
