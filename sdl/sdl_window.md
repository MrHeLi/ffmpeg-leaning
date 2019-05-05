# SDL2常用函数&结构分析:SDL_Window&SDL_CreateWindow

## SDL_Window

`SDL_Window`：结构体，描述了一个窗体对象，表示的是会呈现在设备上一个窗体，所有图像的载体。一般通过`SDL_CreateWindow`函数创建，和`SDL_DestroyWindow`销毁。

## SDL_CreateWindow

函数原型：

```c
SDL_Window* SDL_CreateWindow(const char* title,
                             int         x,
                             int         y,
                             int         w,
                             int         h,
                             Uint32      flags)
```

参数列表含义：

| 参数      | 含义                                                         |
| --------- | ------------------------------------------------------------ |
| **title** | 窗口的标题，采用UTF-8编码                                    |
| **x**     | 窗口的x坐标，是中间（SDL_WINDOWPOS_CENTERED）还是不指定（SDL_WINDOWPOS_UNDEFINED） |
| **y**     | 窗口的y坐标, SDL_WINDOWPOS_CENTERED, or SDL_WINDOWPOS_UNDEFINED |
| **w**     | 窗口的宽度，以像素为单位                                     |
| **h**     | 窗口的高度，以像素为单位                                     |
| **flags** | 0，或一个或多个SDL_WindowFlags，                             |

SDL_WindowFlags列表：

| flags                         | 含义                                                 |
| ----------------------------- | ---------------------------------------------------- |
| SDL_WINDOW_FULLSCREEN         | 全屏窗口                                             |
| SDL_WINDOW_FULLSCREEN_DESKTOP | 当前桌面分辨率的全屏窗口                             |
| SDL_WINDOW_OPENGL             | 窗口可用于OpenGL上下文                               |
| SDL_WINDOW_VULKAN             | 可用于Vulkan实例的窗口                               |
| SDL_WINDOW_HIDDEN             | 隐藏窗口                                             |
| SDL_WINDOW_BORDERLESS         |                                                      |
| SDL_WINDOW_RESIZABLE          | 窗口可以调整大小                                     |
| SDL_WINDOW_MINIMIZED          | 窗口最小化                                           |
| SDL_WINDOW_MAXIMIZED          | 窗口最大化                                           |
| SDL_WINDOW_INPUT_GRABBED      | 窗口可以捕获键盘输入焦点                             |
| SDL_WINDOW_ALLOW_HIGHDPI      | 如果支持，则应在高DPI模式下创建窗口（> = SDL 2.0.1） |

如果使用`SDL_WINDOW_ALLOW_HIGHDPI`标志创建窗口，则其大小（以像素为单位）可能与具有高DPI支持的平台（例如iOS和Mac OS X）上的屏幕坐标大小不同。 使用`SDL_GetWindowSize()`可以获得当前window大小，使用`SDL_GL_GetDrawableSize()`或`SDL_GetRendererOutputSize()`查询可绘制的大小（以像素为单位）。

如果窗口设置为全屏，则不会使用宽度和高度参数w和h。 但是，无效的大小参数（例如，太大）可能仍然失败。 对于窗口创建的所有平台，窗口大小实际上限制为16384 x 16384。

返回值：成功，则返回创建的窗口，失败时为NULL。调用SDL_GetError()可以获得本次异常信息。

## 示例代码

使用SDL2创建一个应用窗口

```c
#include "SDL.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
    SDL_Window *window;                    // 申明SDL_Window指针
    SDL_Init(SDL_INIT_VIDEO);              // 初始化SDL2系统
    // 创建一个应用窗口
    window = SDL_CreateWindow(
        "An SDL2 window",                  // 窗口title
        SDL_WINDOWPOS_UNDEFINED,           // x
        SDL_WINDOWPOS_UNDEFINED,           // y
        640,                               // 宽
        480,                               // 高
        SDL_WINDOW_OPENGL                  // flags - see below
    );
    // 检查窗体是否创建成功，如果不成功window 为NULL
    if (window == NULL) {
        // 如果窗体没有创建成功，后面的程序已经没有执行的必要了。
        printf("Could not create window: %s\n", SDL_GetError());
        return 1;
    }
    // The window is open: could enter program loop here (see SDL_PollEvent())
    SDL_Delay(3000);  // 暂停执行3秒
    // 关闭并回收窗体
    SDL_DestroyWindow(window);
    // 退出
    SDL_Quit();
    return 0;
}
```

### 