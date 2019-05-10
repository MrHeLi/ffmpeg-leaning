# SDL2常用函数&结构分析:SDL_Renderer&SDL_CreateRenderer&SDL_RenderCopy&SDL_RenderPresent

## SDL_Renderer

`SDL_Renderer`是一个结构体，用来表示SDL2中渲染器的状态。从代码的使用上来讲，你也可以把它视为渲染器**本身**。我们可以利用它产生纹理，也可以用来渲染视图。通常，`SDL_Renderer`实例由`SDL_CreateRenderer`函数创建。

## SDL_CreateRenderer

函数原型：

```c++
SDL_Renderer* SDL_CreateRenderer(SDL_Window* window, int index, Uint32 flags)
```

**功能**：为指定窗口创建渲染器上下文。

**参数**：

* **SDL_Window* window**：和渲染器上下文关联的窗体指针。
* **int index**：即将初始化的渲染器驱动程序的索引，如果是-1表示flags标志的第一个可用驱动程序。
* **flags**：0，或者一个或多个SDL_RendererFlags合并在一起。SDL_RendererFlags见下表

| SDL_RendererFlags          | 渲染器标志                          |
| -------------------------- | ----------------------------------- |
| SDL_RENDERER_SOFTWARE      | the renderer is a software fallback |
| SDL_RENDERER_ACCELERATED   | 渲染器使用硬件加速                  |
| SDL_RENDERER_PRESENTVSYNC  | 显示与刷新率同步                    |
| SDL_RENDERER_TARGETTEXTURE | 渲染器支持渲染到纹理                |

实测，因为只是渲染一帧的YUV数据，所以这几个SDL_RendererFlags换来换去，一个或多个，都没有什么明显区别。可能需要在真正的视频播放场景下才会出现明显的差距吧。

**返回值**：

* **SDL_Renderer***：返回一个和参数SDL_Window关联的渲染器上下文指针。

## SDL_RenderCopy

函数原型：

```c++
int SDL_RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* srcrect, const SDL_Rect* dstrect)
```

功能：将制定区域（srcrect）的纹理数据，拷贝到渲染目标尺寸为（dstrect）的渲染器上下文（renderer）中，为下一步的渲染做准备。

参数列表：

| 参数         | 释义                                                         |
| ------------ | ------------------------------------------------------------ |
| **renderer** | 渲染器的上下文                                               |
| **texture**  | 纹理，纹理的显示效果如颜色、透明度（alpha值)等会收到其它接口设置最终产生不一样的效果，这些接口有：SDL_SetTextureColorMod、SDL_SetTextureAlphaMod、SDL_SetTextureBlendMode |
| **srcrect**  | 需要拷贝的SDL_Texture尺寸（用SDL_Rect结构表达），NULL则表示整个纹理 |
| **dstrect**  | 渲染区域的尺寸（用SDL_Rect结构表达），NULL表示整个渲染整个渲染区域; 如果纹理尺寸不够，将会拉伸或压缩纹理。 |

## SDL_RenderPresent

函数原型：

```c++
void SDL_RenderPresent(SDL_Renderer* renderer)
```

功能：将渲染器上下文中的数据，渲染到关联窗体上去。

参数：**renderer**：渲染器上下文。

SDL的渲染功能在后备缓冲区中运行。也就是说，调用诸如`SDL_RenderDrawLine`之类的渲染函数不会直接在屏幕上放置一条线，而是更新后备缓冲区。 因此，您可以构建整个场景并将组合后的缓冲区作为完整图片调用`SDL_RenderPresent`呈现给屏幕。

因此，在使用SDL的渲染API时，会对帧进行所有绘制，然后每帧调用此函数一次，以向用户显示最终绘图。

数据每次显示后，后备缓冲应视为无效。不要假设每帧之间遗留先前的内容。 强烈建议您在开始每个新帧的绘制之前调用SDL_RenderClear来初始化后备缓冲区，即使您打算覆盖每个像素也是如此。

## 示例代码

```c++
#include "SDL.h"

int main(int argc, char *argv[]) {
    SDL_Window *win = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *bitmapTex = NULL;
    SDL_Surface *bitmapSurface = NULL;
    int posX = 100, posY = 100, width = 320, height = 240;

    SDL_Init(SDL_INIT_VIDEO);

    win = SDL_CreateWindow("Hello World", posX, posY, width, height, 0);

    renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    bitmapSurface = SDL_LoadBMP("hello.bmp");
    bitmapTex = SDL_CreateTextureFromSurface(renderer, bitmapSurface);
    SDL_FreeSurface(bitmapSurface);

    while (1) {
        SDL_Event e;
        if (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                break;
            }
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, bitmapTex, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(bitmapTex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);

    SDL_Quit();
    return 0;
}

```

