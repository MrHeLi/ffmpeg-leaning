# SDL2常用函数&结构分析:SDL_Texture&SDL_CreateTexture&SDL_UpdateTexture

## SDL_Texture

`SDL_Texture`是计算机图形图像中，如何在屏幕上显示图像的一个数据的抽象。中文也已翻译为纹理。这种概念在OpenGL中也有出现。对于`SDL_Texture`本身来说，它包含了显示驱动用于显示的特定数据结构。

`SDL_Texture`的创建一半通过`SDL_CreateTexture`创建，创建一次后，可以重复使用，通过`SDL_UpdateTexture`函数更新数据。

## SDL_CreateTexture

函数原型：

```c++
SDL_Texture* SDL_CreateTexture(SDL_Renderer* renderer,
                               Uint32        format,
                               int           access,
                               int           w,
                               int           h)
```

**函数功能**：使用此函数可为渲染器上下文创建纹理。

**参数列表**：

| 参数         | 释义                                                         |
| ------------ | ------------------------------------------------------------ |
| **renderer** | 渲染器上下文                                                 |
| **format**   | 渲染的**像素格式**，支持的格式，在 [SDL_PixelFormatEnum](https://wiki.libsdl.org/SDL_PixelFormatEnum)中，详情见**渲染器支持的像素格式** |
| **access**   | 纹理访问模式，定义在 [SDL_TextureAccess](https://wiki.libsdl.org/SDL_TextureAccess)中; 详见**纹理访问模式** |
| **w**        | 纹理的宽度（以像素为单位）                                   |
| **h**        | 纹理的高度（以像素为单位）                                   |

**返回值**：返回渲染器上下文的纹理。

**渲染器支持的像素格式：**

| SDL_PIXELFORMAT_UNKNOWN  |                                                    |
| ------------------------ | -------------------------------------------------- |
| SDL_PIXELFORMAT_RGB332   |                                                    |
| SDL_PIXELFORMAT_RGB444   |                                                    |
| SDL_PIXELFORMAT_RGB555   |                                                    |
| SDL_PIXELFORMAT_ARGB1555 |                                                    |
| SDL_PIXELFORMAT_RGBA5551 |                                                    |
| SDL_PIXELFORMAT_ABGR1555 |                                                    |
| SDL_PIXELFORMAT_BGRA5551 |                                                    |
| SDL_PIXELFORMAT_RGB565   |                                                    |
| SDL_PIXELFORMAT_RGBA32   | 当前平台的RGBA字节数组颜色的别名(>= SDL 2.0.5)     |
| SDL_PIXELFORMAT_ARGB32   | 当前平台的ARGB字节数组颜色数据的别名(>= SDL 2.0.5) |
| SDL_PIXELFORMAT_YV12     | 平面模式: Y + V + U (3 planes)                     |
| SDL_PIXELFORMAT_IYUV     | 平面模式: Y + U + V (3 planes)                     |
| SDL_PIXELFORMAT_YUY2     | 打包模式: Y0+U0+Y1+V0 (1 plane)                    |
| SDL_PIXELFORMAT_UYVY     | 打包模式: U0+Y0+V0+Y1 (1 plane)                    |
| SDL_PIXELFORMAT_YVYU     | 打包模式: Y0+V0+Y1+U0 (1 plane)                    |
| SDL_PIXELFORMAT_NV12     | 打包模式: Y + U/V 交错 (2 planes) (>= SDL 2.0.4)   |
| SDL_PIXELFORMAT_NV21     | 打包模式: Y + V/U 交错 (2 planes) (>= SDL 2.0.4)   |

**纹理访问模式**：

| 访问模式                    | 含义               |
| --------------------------- | ------------------ |
| SDL_TEXTUREACCESS_STATIC    | 变化很少，不可锁定 |
| SDL_TEXTUREACCESS_STREAMING | 经常变化，可锁定   |
| SDL_TEXTUREACCESS_TARGET    | 可以用作渲染目标   |

## SDL_UpdateTexture

函数原型：

```c++
int SDL_UpdateTexture(SDL_Texture*    texture,
                      const SDL_Rect* rect,
                      const void*     pixels,
                      int             pitch)
```

**函数功能**：使用新的像素数据更新给定的纹理矩形。也就是说，可以固定刷新纹理的某一分部区域。

**参数列表**：

| 参数        | 含义                                                         |
| ----------- | ------------------------------------------------------------ |
| **texture** | the texture to update                                        |
| **rect**    | 用SDL_Rect结构表达纹理中需要更新数据的区域, 如果为NULL则更新整个纹理 |
| **pixels**  | 纹理格式的原始像素数据，格式通常在创建纹理的函数中指定       |
| **pitch**   | 一行像素数据中的字节数，包括行之间的填充。字节数通常由纹理的格式决定 |

这里需要解释一下pitch的计算方式，以一帧尺寸为960x540，格式为**SDL_PIXELFORMAT_IYUV**为例，计算

pitch ：

SDL_PIXELFORMAT_IYUV，也就是IYUV类型，也称为YUV420p、I420，它在内存中的存储格式为三平面存储。

即：第一个平面960个字节的位置存Y分量，第二个平面480个字节存U分量，第三个平面480个字节存V分量。而这个函数中的pitch所说的**一行数像素数据中的字节数**，其实指的是第一个平面的长度（字节为单位），也就是960字节，虽然和宽度在数学上相等，但它们所表达的意思是不一样的，



另外，这是一个相当慢的函数，旨在用于不经常更改的静态纹理。

如果要经常更新纹理，则最好将纹理创建为流式传输并使用下面`SDL_LockTexture`锁定函数。 虽然此功能适用于流式纹理，但出于优化原因，如果之后锁定纹理，则可能无法获得像素。

> SDL_LockTexture：锁定纹理
>
> SDL_UnlockTexture：解锁纹理

**返回值**：

成功时返回0或失败时返回负错误代码

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
    int frameSize = HEIGHT * WIDTH * 12 / 8;
    m_yuv_data = (unsigned char*)malloc(frameSize * sizeof(unsigned char));
    fread(m_yuv_data, 1, frameSize, pFile);
    fclose(pFile);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0); // 创建渲染器
    // 创建纹理
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV,  SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
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
            SDL_RenderClear(renderer); // 清除渲染器
            SDL_RenderCopy(renderer, texture, NULL, NULL); // 拷贝纹理到渲染器
            SDL_RenderPresent(renderer); // 渲染
        }
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }
}
```

