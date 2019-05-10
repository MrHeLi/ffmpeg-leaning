# SDL2:第四个程序(Mac):显示YUV格式图片

在上一个程序[SDL2:第三个程序(Mac):显示任意图片](https://blog.csdn.net/qq_25333681/article/details/89808990)我们利用sdl2_image库已经可以做到显示任意格式的图片了。

可我这种最终要利用SDL2做视频工具的男人，怎么会满足于小小的图片呢。

因为FFmpeg解码出来的视频帧数据是以YUV数据的格式展现，所以我必须先弄清楚YUV格式，在SDL2中是怎么显示的。

为此，我专门用FFmpeg命令，将小王子的图片从jpg格式，转成了I420的YUV4:2:0的格式，以便测试。下载链接如下：[little_prince_yv12_960x540.yuv](https://download.csdn.net/download/qq_25333681/11167534)

不知道为啥，下载居然要积分，大家可以自己利用ffmpeg转一个就成。

```shell
ffmpeg -pix_fmt yuv420p -s 960x540 -i little_prince.jpg little_prince_yuv420p_960x540.yuv
```

* -pix_fmt：表示要用什么格式转换，yuv420p是参数，你也可以通过`ffmpeg -pix_fmts`查看其它支持的类型。
* -s：表示一帧的尺寸，这个尺寸将是你生成yuv数据的宽和高，需要牢牢记住，因为转换成yuv数据后，数据将不会存储任何无关信息，包括尺寸。
* -i：很简单，就是需要转换的文件，i表示input。参数不仅可以是jpg格式的，也可以是bmp等任何其它常见类型。
* little_prince_yuv420p_960x540.yuv：我的输出文件名，在文件名中，记录了yuv的格式，和尺寸，这些信息在显示过程中比较重要。

下面来看看代码：

## 代码

代码是在[SDL2:第三个程序(Mac):显示任意图片](https://blog.csdn.net/qq_25333681/article/details/89808990)基础上扩展而来，cmake部分，毫无变化：

```c++
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
```

其中有部分关于sdl2_image的库，在本次的程序中使用不到的可以直接删掉。

下面是代码部分：

```c++
#include <iostream>

#include <SDL.h>
#include <SDL2/SDL_image.h>

using namespace  std;

const int WIDTH = 960, HEIGHT = 540;
int main() {

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        cout << "SDL could not initialized with error: " << SDL_GetError() << endl;
        return -1;
    }
    SDL_Window *window = SDL_CreateWindow("Hello SDL world!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          WIDTH, HEIGHT, SDL_WINDOW_ALLOW_HIGHDPI);
    if (NULL == window) {
        cout << "SDL could not create window with error: " << SDL_GetError() << endl;
        return -1;
    }

    if (!(IMG_Init(IMG_INIT_JPG) & IMG_INIT_JPG)) {
        cout << "SDL_image could not init with error: " << IMG_GetError() << endl;
        return -1;
    }

    FILE* pFile = fopen("little_prince_i420_960x540.yuv", "rb");
    if (pFile == NULL) {
        cerr << "little_prince_i420_960x540.yuv open failed" << endl;
    }
    unsigned char *m_yuv_data;
    int frameSize = HEIGHT * WIDTH * 3 / 2; // 单帧数据的bit数
    m_yuv_data = (unsigned char*)malloc(frameSize * sizeof(unsigned char));
    fread(m_yuv_data, 1, frameSize, pFile);
    fclose(pFile);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0); // 创建渲染器
    // 创建纹理
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (texture != NULL) {
        SDL_Event windowEvent;
        while (true) {
            if (SDL_PollEvent(&windowEvent)) {
                if (SDL_QUIT == windowEvent.type) {
                    cout << "SDL quit!!" << endl;
                    break;
                }
            }
            SDL_UpdateTexture(texture, NULL, m_yuv_data, WIDTH); // 更新纹理
            SDL_RenderClear(renderer); // 清楚渲染器
            SDL_RenderCopy(renderer, texture, NULL, NULL); // 拷贝渲染器到纹理。
            SDL_RenderPresent(renderer); // 渲染
        }

        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }
}
```

先来看一下显示效果：

![sdl_play_4](/Users/heli/github/ffmpeg-leaning/sdl/sdl_practic_4/sdl_play_4.png)

##  YUV数据提取

这里需要讲一下关于YUV数据填充的部分，即：

```c++
FILE* pFile = fopen("little_prince_i420_960x540.yuv", "rb");
unsigned char *m_yuv_data;
int frameSize = HEIGHT * WIDTH * 3 / 2; // 单帧数据的bit数
m_yuv_data = (unsigned char*)malloc(frameSize * sizeof(unsigned char));
fread(m_yuv_data, 1, frameSize, pFile);
fclose(pFile);
```

fopen就不用说了，通过fopen获得我们前面通过ffmpeg制作好的yuv文件little_prince_yv12_960x540.yuv的句柄。核心问题是，如何从这个句柄中，提取出我们需要的Y、U、V分量。

关键在于YUV数据的格式和size。格式决定了YUV分量在文件中的排列方式，size决定了总的像素值。我们已经知道图片是I420格式。也就是yuv420p，它是三平面存储，每个像素内存计算方式是：

> 亮度\[\(4) ＋ U(1) + V(1)\]/4(像素) = 3/2bit

所有I420单帧图片要占用的内存为：`int frameSize = HEIGHT * WIDTH * 3 / 2`

接着就是分配一个无符号的内存空间，来存储从文件中读取的yuv数据。



本次程序的流程图如下：

![sdl_practice_workflow](/Users/heli/github/ffmpeg-leaning/sdl/sdl_practic_4/sdl_practice_workflow.png)



代码流程图中，黄色部分函数调用，是从[第一个程序](https://blog.csdn.net/qq_25333681/article/details/89765073)copy过来的，它几乎是所有SDL程序的必要流程了，要丰富SDL程序，都是在黄色框架之间扩充功能和代码，相关函数解释可以查看：

[SDL2常用函数&结构分析:SDL_Init](https://blog.csdn.net/qq_25333681/article/details/89787836)

[SDL2常用函数&结构分析:SDL_Window&SDL_CreateWindow
SDL_Window](https://blog.csdn.net/qq_25333681/article/details/89787867)

[SDL2常用函数&结构分析:SDL_Event&SDL_PollEvent](https://blog.csdn.net/qq_25333681/article/details/89787901)

绿色部分，是本次代码新增，用于显示YUV的极简逻辑，**从文件提取yuv数据**在本文中已经说到，其它没有提到的相关函数解释可以查看：

[SDL2常用函数&结构分析:SDL_Renderer&SDL_CreateRenderer&SDL_RenderCopy&SDL_RenderPresent](https://blog.csdn.net/qq_25333681/article/details/90050133)

[SDL2常用函数&结构分析:SDL_Texture&SDL_CreateTexture&SDL_UpdateTexture](https://blog.csdn.net/qq_25333681/article/details/90083753)

github地址：

[代码github地址](https://github.com/MrHeLi/ffmpeg-leaning/tree/master/sdl/sdl_practic_4/PlaySDL)