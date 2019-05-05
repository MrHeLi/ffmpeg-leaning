# SDL2常用函数&结构分析:SDL_Surface&SDL_GetWindowSurface&SDL_LoadBMP

## SDL_Surface

`SDL_Surface`的官方定义为：A collection of pixels used in software blitting.(软件blitting中使用的像素集合）

这么一看，还真不好理解，blitting到底是个啥意思，起初我以为是文化差异造成的。Google了一下，发现有搜索这个解释的国外大兄弟还有很多，在Stack overflow中找到一个比较靠谱易懂的说法：

> Blitting means **bit-boundary block transfer** as defined by Wikipedia or **Block Information Transfer**, well known among the Pygame developers. Assume you have a Surface(your screen). And you would like to draw a circle on the screen. So what you want to do is, draw the circle and transfer the circle block of the buffer to the screen buffer, this process is called "Blitting". 

结合`SDL_Surface`的官方定义来看，可以这么理解：

`SDL_Surface`是一个用于Surface间相互拷贝buffer数据的像素集合。

`SDL_Surface`结构体的字段如下：

| Uint32                                                     | **flags**     | (internal use)                                               |
| ---------------------------------------------------------- | ------------- | ------------------------------------------------------------ |
| [SDL_PixelFormat](http://wiki.libsdl.org/SDL_PixelFormat)* | **format**    | 存储在surface中的像素的格式; 有关详细信息，请参阅SDL_PixelFormat（只读） |
| int                                                        | **w, h**      | 宽度和高度（以像素为单位）（只读）                           |
| int                                                        | **pitch**     | 一行像素的长度（以字节为单位）（只读）                       |
| void*                                                      | **pixels**    | 指向实际像素数据的指针; 有关详细信息（读写）                 |
| void*                                                      | **userdata**  | 你可以设置的任意指针（读写）                                 |
| int                                                        | **locked**    | 用于需要锁定的表面（内部使用）                               |
| void*                                                      | **lock_data** | 用于需要锁定的表面（内部使用）                               |
| [SDL_Rect](http://wiki.libsdl.org/SDL_Rect)                | **clip_rect** | 一个SDL_Rect结构，用于将blits剪切到表面，可以通过SDL_SetClipRect（）设置（只读） |
| SDL_BlitMap*                                               | **map**       | 快速blit映射到其他表面的信息（内部使用）                     |
| int                                                        | **refcount**  | 引用计数可以由应用程序递增                                   |

## SDL_GetWindowSurface

使用最佳格式创建与窗口关联的新surface，如有必要。 当窗口被破坏时，该surface将被释放。函数原型：

```c
SDL_Surface * SDLCALL SDL_GetWindowSurface(SDL_Window *window);
```

参数* window：是前面已经创建成功的window窗体对象那个指针。

返回值：窗体surface的framebuffer，异常时返回NULL。

注意：你可能无法在此窗口中将该surface与3D或渲染API结合使用。

## SDL_LoadBMP

SDL2原生库中，目前只默认支持了bmp格式，其它格式需要引用特定的库，或者自己实现。

函数原型：

```c
SDL_Surface* SDL_LoadBMP(const char* file)
```

该函数的作用是，加载指定的*.bmp文件，以获取该文件的`SDL_Surface`。

参数file：包含BMP图像的文件，注意这个文件一定得是bmp格式的文件，否则可能加载不出正常的图片。

返回值：返回指向新`SDL_Surface`结构的指针，如果有错误则返回NULL; 调用`SDL_GetError()`可以获取更多异常信息。

## 示例代码

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

