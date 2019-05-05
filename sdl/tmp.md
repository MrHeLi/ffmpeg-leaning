| SDL_RENDERER_SOFTWARE      | 渲染器是软件后备     |
| -------------------------- | -------------------- |
| SDL_RENDERER_ACCELERATED   | 渲染器使用硬件加速   |
| SDL_RENDERER_PRESENTVSYNC  | 显示与刷新率同步     |
| SDL_RENDERER_TARGETTEXTURE | 渲染器支持渲染到纹理 |



| **renderer** | 渲染器上下文                                           |
| ------------ | ------------------------------------------------------ |
| **format**   | SDL_PixelFormat Enum中的枚举值之一; 请参阅备注了解详情 |
| **access**   | SDL_TextureAccess中的枚举值之一; 请参阅备注了解详情    |
| **w**        | 纹理的宽度（以像素为单位）                             |
| **h**        | 纹理的高度（以像素为单位）                             |

| SDL_TEXTUREACCESS_STATIC    | 变化很少，不可锁定 |
| --------------------------- | ------------------ |
| SDL_TEXTUREACCESS_STREAMING | 经常变化，可锁定   |
| SDL_TEXTUREACCESS_TARGET    | 可以用作渲染目标   |



| SDL_PIXELFORMAT_UNKNOWN     |                                                       |
| --------------------------- | ----------------------------------------------------- |
| SDL_PIXELFORMAT_INDEX1LSB   |                                                       |
| SDL_PIXELFORMAT_INDEX1MSB   |                                                       |
| SDL_PIXELFORMAT_INDEX4LSB   |                                                       |
| SDL_PIXELFORMAT_INDEX4MSB   |                                                       |
| SDL_PIXELFORMAT_INDEX8      |                                                       |
| SDL_PIXELFORMAT_RGB332      |                                                       |
| SDL_PIXELFORMAT_RGB444      |                                                       |
| SDL_PIXELFORMAT_RGB555      |                                                       |
| SDL_PIXELFORMAT_BGR555      |                                                       |
| SDL_PIXELFORMAT_ARGB4444    |                                                       |
| SDL_PIXELFORMAT_RGBA4444    |                                                       |
| SDL_PIXELFORMAT_ABGR4444    |                                                       |
| SDL_PIXELFORMAT_BGRA4444    |                                                       |
| SDL_PIXELFORMAT_ARGB1555    |                                                       |
| SDL_PIXELFORMAT_RGBA5551    |                                                       |
| SDL_PIXELFORMAT_ABGR1555    |                                                       |
| SDL_PIXELFORMAT_BGRA5551    |                                                       |
| SDL_PIXELFORMAT_RGB565      |                                                       |
| SDL_PIXELFORMAT_BGR565      |                                                       |
| SDL_PIXELFORMAT_RGB24       |                                                       |
| SDL_PIXELFORMAT_BGR24       |                                                       |
| SDL_PIXELFORMAT_RGB888      |                                                       |
| SDL_PIXELFORMAT_RGBX8888    |                                                       |
| SDL_PIXELFORMAT_BGR888      |                                                       |
| SDL_PIXELFORMAT_BGRX8888    |                                                       |
| SDL_PIXELFORMAT_ARGB8888    |                                                       |
| SDL_PIXELFORMAT_RGBA8888    |                                                       |
| SDL_PIXELFORMAT_ABGR8888    |                                                       |
| SDL_PIXELFORMAT_BGRA8888    |                                                       |
| SDL_PIXELFORMAT_ARGB2101010 |                                                       |
| SDL_PIXELFORMAT_RGBA32      | 当前平台的RGBA字节数组颜色的别名（> = SDL 2.0.5）     |
| SDL_PIXELFORMAT_ARGB32      | 当前平台的ARGB字节数组颜色数据的别名（> = SDL 2.0.5） |
| SDL_PIXELFORMAT_BGRA32      | 当前平台的BGRA字节数组颜色的别名（> = SDL 2.0.5）     |
| SDL_PIXELFORMAT_ABGR32      | 当前平台（> = SDL 2.0.5）的ABGR字节数组颜色数据的别名 |
| SDL_PIXELFORMAT_YV12        | 平面模式：Y + V + U（3planes）                        |
| SDL_PIXELFORMAT_IYUV        | 平面模式：Y + U + V (3 planes)                        |
| SDL_PIXELFORMAT_YUY2        | 打包模式：Y0 + U0 + Y1 + V0 (1 plane)                 |
| SDL_PIXELFORMAT_UYVY        | 打包模式：U0 + Y0 + V0 + Y1(1 plane)                  |
| SDL_PIXELFORMAT_YVYU        | 打包模式：Y0 + V0 + Y1 + U0 (1 plane)                 |
| SDL_PIXELFORMAT_NV12        | 平面模式：Y + U / V交错（2个plane）（> = SDL 2.0.4）  |
| SDL_PIXELFORMAT_NV21        | 平面模式：Y + V / U交错（2个plane）（> = SDL 2.0.4）  |











