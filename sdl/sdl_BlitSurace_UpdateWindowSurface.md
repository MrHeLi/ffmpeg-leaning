# SDL2常用函数&结构分析:SDL_BlitSurface&SDL_UpdateWindowSurface

## SDL_BlitSurface

函数功能：将一个surface的数据快速复制到目标surface中。

函数原型：

```c
int SDL_BlitSurface(SDL_Surface*    src,
                    const SDL_Rect* srcrect,
                    SDL_Surface*    dst,
                    SDL_Rect*       dstrect)
```

参数列表：

| **src**     | 被拷贝的SDL_Surface                                          |
| ----------- | ------------------------------------------------------------ |
| **srcrect** | SDL_Rect结构表示要复制的矩形的范围，如果为NULL的话，就全部复制 |
| **dst**     | 容纳拷贝数据的目标SDL_Surface                                |
| **dstrect** | SDL_Rect结构表示拷贝到的矩形范围，如果为NULL的话，就全部拷贝 |

返回值：如果blit成功则返回0，失败则返回负数错误代码; 调用SDL_GetError()可以获取更多异常信息。

## SDL_UpdateWindowSurface

函数功能：将窗口surface复制到屏幕。

函数原型：

```c
int SDL_UpdateWindowSurface(SDL_Window* window)
```

参数：window：要更新的窗口

返回值：成功时返回0，失败时返回负数错误码; 调用SDL_GetError()可以获取更多异常信息。

`SDL_BlitSurface` 和 `SDL_UpdateWindowSurface`函数一般是相伴出现，当已经准备好的surface被拷贝后，也就以为着，这部分数据需要被显示了。

## 示例代码

示例代码用于显示一张box.bmp图片。

```c
#include "SDL.h" // include SDL header
int main(int argc, char* argv[])
{
    SDL_Surface *screen;
    SDL_Window *window;
    SDL_Surface *image;
  
    SDL_Init(SDL_INIT_VIDEO); // 初始化视频子系统
    // 创建一个窗体
    window = SDL_CreateWindow("SDL2 Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
    // 获得一个与窗体关联的surface，赋值给screen
    screen = SDL_GetWindowSurface(window);
    image = SDL_LoadBMP("box.bmp"); // 加载一个bmp图片文件，并把surface赋值给image
    SDL_BlitSurface(image, NULL, screen, NULL); // 将image中的的数据拷贝到screen中，相当于直接显示
    SDL_FreeSurface(image); // image数据以经拷贝出去，已经失去价值
    SDL_UpdateWindowSurface(window); // 刷新窗体，让与窗体关联的screen中的数据能够显示出来。
    // show image for 2 seconds
    SDL_Delay(2000);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
```

