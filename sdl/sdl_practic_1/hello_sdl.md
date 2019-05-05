# Clion第一个SDL程序(Mac)

SDL官网：[https://www.libsdl.org/](https://www.libsdl.org/)

SDL2.0文档地址:[http://wiki.libsdl.org/Introduction](http://wiki.libsdl.org/Introduction)

参考YouTuber:Sonar Systems的教学视频，大家可以去关注学习一下。

## 简介

直接翻译了一下官网介绍：

简单的DirectMedia Layer是一个跨平台开发库，旨在通过OpenGL和Direct3D提供对音频，键盘，鼠标，操纵杆和图形硬件的低层访问。 它被视频播放软件，模拟器和流行游戏使用，包括Valve屡获殊荣的目录和许多Humble Bundle游戏。

SDL支持Windows，Mac OS X，Linux，iOS和Android。 可以在源代码中找到对其他平台的支持。

SDL是用C语言编写的，与C ++本身一起工作，并且有几种其他语言可用的绑定，包括C＃和Python。

SDL 2.0在zlib许可下分发。 此许可证允许您在任何软件中自由使用SDL。

## SDL开发包位置

用命令：`brew info sdl2`

```shell
Superli-2:ffmpeg-leaning superli$ brew info sdl2
sdl2: stable 2.0.9 (bottled), HEAD
Low-level access to audio, keyboard, mouse, joystick, and graphics
https://www.libsdl.org/
/usr/local/Cellar/sdl2/2.0.9_1 (87 files, 4.5MB) *
  Poured from bottle on 2019-05-02 at 19:53:15
From: https://github.com/Homebrew/homebrew-core/blob/master/Formula/sdl2.rb
==> Options
--HEAD
	Install HEAD version
==> Analytics
install: 74,178 (30 days), 199,950 (90 days), 518,556 (365 days)
install_on_request: 8,650 (30 days), 23,572 (90 days), 77,346 (365 days)
build_error: 0 (30 days)
```

可以看到我的SDL开发包在：/usr/local/Cellar/sdl2/2.0.9_1目录下。实际上这是Xcode自带的，并没有让我手动安装。

如果没有安装的同学，可以敲命令：`brew install sdl2`安装。

注意是sdl2而不是sdl，两个版本的api向去甚远，别怪我没有提醒你。

## Clion创建第一个SDL程序

Clion创建工程的大致流程是：Clion - > File -> New Project -> C++ Excutable -> 在location里改一下工程目录-> create。

修改CMakeLists.txt文件成这样：

```cmake
cmake_minimum_required(VERSION 3.10)
project(PlaySDL)

set(CMAKE_CXX_STANDARD 11)
set(SDL_DIR /usr/local/Cellar/sdl2/2.0.9_1/) # 这个SDL开发包的路径，可以通过brew info sdl2查出来
include_directories(${SDL_DIR}/include/) # 添加SDL头文件搜索路径
link_libraries(${SDL_DIR}/lib/) # 增加SDL链接库目录

add_executable(PlaySDL main.cpp)

target_link_libraries(PlaySDL SDL2 SDL2_test SDL2main) # 链接目标库
```

`target_link_libraries`中包含的`SDL2`、`SDL2_test`、`SDL2main`是SDL的共享库，在/usr/local/Cellar/sdl2/2.0.9_1/lib/目录下。

main.cpp文件是这样的：

```c++
#include <iostream>
extern "C" {
#include <SDL2/SDL.h>
}
using namespace  std;

const int WIDTH = 400, HEIGHT = 400; // SDL窗口的宽和高
int main() {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) { // 初始化SDL
        cout << "SDL could not initialized with error: " << SDL_GetError() << endl;
    }
    SDL_Window *window = SDL_CreateWindow("Hello SDL world!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_ALLOW_HIGHDPI); // 创建SDL窗口
    if (NULL == window) {
        cout << "SDL could not create window with error: " << SDL_GetError() << endl;
    }

    SDL_Event windowEvent; // SDL窗口事件
    while(true) {
        if (SDL_PollEvent(&windowEvent)) { // 对当前待处理事件进行轮询。
            if (SDL_QUIT == windowEvent.type) { // 如果事件为推出SDL，结束循环。
                cout << "SDL quit!!" << endl;
                break;
            }
        }
    }
    SDL_DestroyWindow(window); // 退出SDL窗体
    SDL_Quit(); // SDL退出
    return 0;
}
```

好了就是这么简单，运行看一下效果：

![hello_sdl](https://github.com/MrHeLi/ffmpeg-leaning/blob/master/sdl/hello_sdl.png)

就是这么丑，我们后面会让它漂亮起来的。

## 代码调用流程

对代码中使用的对象和函数，作简要分析。调用流程如下：

![sdl_process_1](/Users/heli/github/ffmpeg-leaning/sdl/sdl_practic_1/sdl_process_1.png)

代码中具体函数的作用，请参见源码注释或官方文档，图方便的话，也可以看本系列文章。