# SDL2:第三个程序(Mac):显示任意图片

在上一个程序[SDL2:第二个程序(Mac):显示BMP位图](https://blog.csdn.net/qq_25333681/article/details/89790195)中，我们利用SDL2的公共API，在SDL窗体上加载了一个位图（BMP）。

要加载任意格式的图片，仅仅依靠默认API是不够的，需要引入另外的图片库：`sdl2_image`

## 安装sdl2_image

执行命令：

```shell
brew install sdl2_image
```

执行结果：

```shell
==> Installing dependencies for sdl2_image: jpeg, libpng, libtiff and webp
==> Installing sdl2_image dependency: jpeg
🍺  /usr/local/Cellar/jpeg/9c
==> Installing sdl2_image dependency: libpng
🍺  /usr/local/Cellar/libpng/1.6.37
==> Installing sdl2_image dependency: libtiff
🍺  /usr/local/Cellar/libtiff/4.0.10_1
==> Installing sdl2_image dependency: webp
🍺  /usr/local/Cellar/webp/1.0.2
==> Installing sdl2_image
🍺  /usr/local/Cellar/sdl2_image/2.0.4
```

常见的图片格式，这里都有了。brew帮我安装了支持这么多格式图片库，真是个好帮手。

剩下就是代码了。

## 示例代码

因为需要使用到`sdl2_image`的库文件，所以CMakeLists.txt文件相较于上一个程序会有所差异，有注释的地方是新增的。

```cmake
cmake_minimum_required(VERSION 3.10)
project(PlaySDL)

set(CMAKE_CXX_STANDARD 11)
set(MY_LIBRARY_DIR /usr/local/Cellar)
set(SDL_DIR ${MY_LIBRARY_DIR}/sdl2/2.0.9_1/)

# sdl2_image安装目录，可以通过brew info sdl2_image 查看
set(SDL_IMAGE_DIR ${MY_LIBRARY_DIR}/sdl2_image/2.0.4/) 
include_directories(${SDL_DIR}/include/SDL2/
        ${SDL_IMAGE_DIR}/include/) # 添加sdl2_image库的头文件搜索路径

link_libraries(${SDL_DIR}/lib/
        ${SDL_IMAGE_DIR}/lib/) # 添加sdl2_image库的库文件搜索路径

add_executable(PlaySDL main.cpp)

target_link_libraries(PlaySDL SDL2 SDL2_image) # 链接SDL2_image库
```

main.cpp代码：

```c
#include <iostream>
#include <SDL.h>
#include <SDL2/SDL_image.h>

using namespace  std;
const int WIDTH = 960, HEIGHT = 540;
int main() {
    SDL_Surface *imageSurface = NULL;
    SDL_Surface *windowSurface = NULL;
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        cout << "SDL could not initialized with error: " << SDL_GetError() << endl;
        return -1;
    }
    SDL_Window *window = SDL_CreateWindow("Hello SDL world!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_ALLOW_HIGHDPI);
    if (NULL == window) {
        cout << "SDL could not create window with error: " << SDL_GetError() << endl;
        return -1;
    }
    if (!(IMG_Init(IMG_INIT_JPG) & IMG_INIT_JPG)) {
        cout << "SDL_image could not init with error: " << IMG_GetError() << endl;
        return -1;
    }
    windowSurface = SDL_GetWindowSurface(window);
//    imageSurface = SDL_LoadBMP("little_prince.bmp");
    imageSurface = IMG_Load("little_prince.jpg");
    if (NULL == imageSurface) {
        cout << "SDL could not load image with error: " << SDL_GetError() << endl;
        return -1;
    }
    SDL_Event windowEvent;
    int count = 0;
    while(true) {
        if (SDL_PollEvent(&windowEvent)) {
            if (SDL_QUIT == windowEvent.type) {
                cout << "SDL quit!!" << endl;
                break;
            }
        }
        count ++;
        cout << "while count" << count << endl;
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

代码部分，和[SDL2:第二个程序(Mac):显示BMP位图](https://blog.csdn.net/qq_25333681/article/details/89790195)基本上没有差别，只是将加载小王子图片的函数，从`SDL_LoadBMP`改为了`IMG_Load`：

```c
//    imageSurface = SDL_LoadBMP("little_prince.bmp");
		imageSurface = IMG_Load("little_prince.jpg");
```

还有一点就是参数部分了，从原来bmp格式的文件换成了jpg格式的文件。我们一开始的目的是什么来着？没错，就是加载任意图片。

那么，IMG_Load除了能加载jpg格式图片外，其它图片支持么？

先放一张程序运行效果图，再公布答案。

![sdl_play_3](/Users/heli/github/ffmpeg-leaning/sdl/sdl_practic_3/sdl_play_3.png)

没错，这个函数支持所有的图片格式。

## 异常：'SDL.h' file not found

前面的代码是异常解决后的代码，刚开是我的CMakeLists.txt文件起始是这样的：

```cmake
cmake_minimum_required(VERSION 3.10)
project(PlaySDL)

set(CMAKE_CXX_STANDARD 11)
set(MY_LIBRARY_DIR /usr/local/Cellar)
set(SDL_DIR ${MY_LIBRARY_DIR}/sdl2/2.0.9_1/)

# sdl2_image安装目录，可以通过brew info sdl2_image 查看
set(SDL_IMAGE_DIR ${MY_LIBRARY_DIR}/sdl2_image/2.0.4/) 
include_directories(${SDL_DIR}/include/
        ${SDL_IMAGE_DIR}/include/) # 添加sdl2_image库的头文件搜索路径

link_libraries(${SDL_DIR}/lib/
        ${SDL_IMAGE_DIR}/lib/) # 添加sdl2_image库的库文件搜索路径

add_executable(PlaySDL main.cpp)

target_link_libraries(PlaySDL SDL2 SDL2_image) # 链接SDL2_image库
```

两者之间的细微差别在于，我在添加SDL2的头文件搜索路径时，是这么添加的：

`include_directories(${SDL_DIR}/include/}`，对应的引用方式为：`#include <SDL2/SDL.h>`

在没有引入SDL_image库之前，完全没问题。但当使用了SDL_image之后，就会出现SDL.h文件找不到的情况，具体异常如下：

```shell
/usr/local/Cellar/sdl2_image/2.0.4/include/SDL2/SDL_image.h:27:10: fatal error: 'SDL.h' file not found
#include "SDL.h"
         ^~~~~~~
1 error generated.
```

一切都是因为`SDL_image.h`文件的路径和CMakeLists.txt中的搜索路径不匹配，`SDL_image.h`关于`SDL.h`的引入是`#include "SDL.h"`是这样的，而我的CMakeLists.txt中头文件的所有路径是：`${SDL_DIR}/include/`，在这个路径下只有一个SDL2的文件夹，肯定没有SDL.h文件，当然就会报错了。

正确的添加姿势是：`include_directories(${SDL_DIR}/include/SDL2}`，对应的引用方式为：`#include <SDL.h>`



[github源码链接](https://github.com/MrHeLi/ffmpeg-leaning/tree/master/sdl/sdl_practic_3/PlaySDL)