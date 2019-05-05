# SDL2:第二个程序(Mac):显示BMP位图

继上一个SDL2程序（[SDL2:第一个程序(Mac)](https://blog.csdn.net/qq_25333681/article/details/89765073)），我们使用SDL2打开了一个黑色的窗口。

世界应该是彩色的，怎么能让黑色霸占呢，是时候让阳光驱逐黑暗了，今天，我要用SDL2显示一张图片。

为此，我花重金请了位模特，拍了张照片。我待会儿就要让小王子进入SDL的世界。

![little_prince](/Users/heli/github/ffmpeg-leaning/sdl/sdl_practic_2/little_prince.bmp)

## 代码

还是第一个程序的代码，只不过在上面稍作修改。

首先是CMakeLists.txt文件

```cmake
cmake_minimum_required(VERSION 3.10)
project(PlaySDL)

set(CMAKE_CXX_STANDARD 11)
set(SDL_DIR /usr/local/Cellar/sdl2/2.0.9_1/)
include_directories(${SDL_DIR}/include/)
link_libraries(${SDL_DIR}/lib/)

add_executable(PlaySDL main.cpp)

target_link_libraries(PlaySDL SDL2 SDL2_test SDL2main)
```

什么都没改，但为了不让你回到第一篇文章去看，我还是贴在这里了。

下面是代码部分，只做了点小修改：

```c++
#include <iostream>
extern "C" {
#include <SDL2/SDL.h>
#include <SDL2/SDL_test_images.h>
}
using namespace  std;

const int WIDTH = 960, HEIGHT = 540;
int main() {
    SDL_Surface *imageSurface = NULL; // 申明用于加载图片的SDL_Surface
    SDL_Surface *windowSurface = NULL; // 申明用于窗体相关的SDL_Surface
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        cout << "SDL could not initialized with error: " << SDL_GetError() << endl;
    }
    SDL_Window *window = SDL_CreateWindow("Hello SDL world!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_ALLOW_HIGHDPI);
    if (NULL == window) {
        cout << "SDL could not create window with error: " << SDL_GetError() << endl;
    }

    windowSurface = SDL_GetWindowSurface(window);
    imageSurface = SDL_LoadBMP("little_prince.bmp");
    if (NULL == imageSurface) {
        cout << "SDL could not load image with error: " << SDL_GetError() << endl;
    }
    SDL_Event windowEvent;
    while(true) {
        if (SDL_PollEvent(&windowEvent)) {
            if (SDL_QUIT == windowEvent.type) {
                cout << "SDL quit!!" << endl;
                break;
            }
        }
        SDL_BlitSurface(imageSurface, NULL, windowSurface, NULL);
        SDL_UpdateWindowSurface(window);
    }

    imageSurface = NULL;
    windowSurface = NULL;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
```

little_prince.bmp需要是真正的bmp格式图片。SDL默认值支持bmp格式的，如果使用其他格式的图片，你可能拿到的是些乱七八糟的东西。

至于怎么使用SDL展示其它格式的图片，下一篇文章中会讲。

看一下运行效果：

![sdl_play_2](/Users/heli/github/ffmpeg-leaning/sdl/sdl_practic_2/sdl_play_2.png)

加载出来了，小王子战胜了黑暗世界。

## 代码调用流程

![sdl_process_2](/Users/heli/github/ffmpeg-leaning/sdl/sdl_practic_2/sdl_process_2.png)

代码流程图中，黄色部分函数调用，是从[第一个程序](https://blog.csdn.net/qq_25333681/article/details/89765073)copy过来的，它几乎是所有SDL程序的必要流程了，要丰富SDL程序，都是在黄色框架之间扩充功能和代码，相关函数解释可以查看：

[SDL2常用函数&结构分析:SDL_Init](https://blog.csdn.net/qq_25333681/article/details/89787836)

[SDL2常用函数&结构分析:SDL_Window&SDL_CreateWindow
SDL_Window](https://blog.csdn.net/qq_25333681/article/details/89787867)

[SDL2常用函数&结构分析:SDL_Event&SDL_PollEvent](https://blog.csdn.net/qq_25333681/article/details/89787901)

绿色部分，是本次代码新增，用于显示BMP位图的极简逻辑，相关函数解释可以查看：

[SDL2常用函数&结构分析:SDL_Surface&SDL_GetWindowSurface&SDL_LoadBMP](https://blog.csdn.net/qq_25333681/article/details/89789479)

[SDL2常用函数&结构分析:SDL_BlitSurface&SDL_UpdateWindowSurface](https://blog.csdn.net/qq_25333681/article/details/89789707)