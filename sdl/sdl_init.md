# SDL2常用函数&结构分析:SDL_Init

## SDL_Init

函数原型如下：

```c
int SDL_Init(Uint32 flags)
```

使用此函数初始化SDL库，必须在使用大多数其他SDL函数之前调用它。

参数：flags 表示需要初始化那些子系统。常见的子系统和对应的flag如下：

| flags                       | 子系统                             |
| --------------------------- | ---------------------------------- |
| **SDL_INIT_TIMER**          | 计时器子系统                       |
| **SDL_INIT_AUDIO**          | 音频子系统                         |
| **SDL_INIT_VIDEO**          | 视频子系统; 自动初始化事件子系统   |
| **SDL_INIT_JOYSTICK**       | 操纵杆子系统; 自动初始化事件子系统 |
| **SDL_INIT_HAPTIC**         | 触觉（力反馈）子系统               |
| **SDL_INIT_GAMECONTROLLER** | 控制子系统; 自动初始化操纵杆子系统 |
| **SDL_INIT_EVENTS**         | 事件子系统                         |
| **SDL_INIT_EVERYTHING**     | 所有上述子系统                     |
| **SDL_INIT_NOPARACHUTE**    | 兼容性; 这个标志被忽略了           |

返回值：int 成功时返回0，失败时返回负数错误码; 调用SDL_GetError()可以获得本次异常信息。

## 示例代码

以一个播放器的初始化为例：

```c
#include "SDL.h"
int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }
    /* ... */
    SDL_Quit();
    return 0;
}
```

`SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)`：表示需要初始化视频和音频子系统。