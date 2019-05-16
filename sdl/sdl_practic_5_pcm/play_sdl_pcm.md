# SDL2:第五个程序:播放pcm数据

播放音频数据对一个播放器来说是不可或缺的，索性SDL2支持播放视频数据之外，也支持播放音频数据。废话不多说，先来试试吧！

## 使用FFmpeg命令提取PCM数据

这里从网上下了一首歌Forevermore，非常好听，是mp3格式的：Forevermore.mp3。

在提取Forevermore中的pcm数据时，为了使提取数据的基本采样率编解码格式等不发生改变，先用ffprobe命令探测一下该原数据的基本属性：

```shell
Superli-2:Desktop superli$ ffprobe Forevermore.mp3 
ffprobe version 4.0 Copyright (c) 2007-2018 the FFmpeg developers
  built with Apple LLVM version 9.1.0 (clang-902.0.39.1)
  configuration: 
  libavutil      56. 14.100 / 56. 14.100
  libavcodec     58. 18.100 / 58. 18.100
  libavformat    58. 12.100 / 58. 12.100
  libavdevice    58.  3.100 / 58.  3.100
  libavfilter     7. 16.100 /  7. 16.100
  libswscale      5.  1.100 /  5.  1.100
  libswresample   3.  1.100 /  3.  1.100
Input #0, mp3, from 'Forevermore.mp3':
  Metadata:
    encoder         : Lavf56.4.101
    disc            : 1
    track           : 10
    comment         : 163 key(Don't modify):L64FU3W4YxX3ZFTmbZ+8/S+68Aeva5aBGT9F8SijqsT8pNFuHKdV06BJS4v1A0bF1vFBqTyVaKDutQdoGroCvrYOOp2lT2DptMegHoLrOJSUlP8HjfG33C8WnxFZSTkpXKPuJpy2SCMpxN8Yls8kDVk0AyuBfxcgRa1C4sW5TuaaWAu70R1vzzOfeibXvpitDog/fydkmjeWDFCwEKgibbykqnCVnsMCGPJHF+0bJ
    artist          : Katie Herzig
    title           : Forevermore
    album           : Live In Studio: Acoustic Trio
  Duration: 00:02:22.00, start: 0.025056, bitrate: 327 kb/s
    Stream #0:0: Audio: mp3, 44100 Hz, stereo, fltp, 320 kb/s
    Stream #0:1: Video: mjpeg, yuvj444p(pc, bt470bg/unknown/unknown), 640x640 [SAR 72:72 DAR 1:1], 90k tbr, 90k tbn, 90k tbc
    Metadata:
      comment         : Other

```

可以用到的音频基本数据为：

* 声道数：stereo（双声道）
* 采样率为：44100Hz
* 音频格式为：fltp

下面使用ffmpeg提取pcm数据

```shell
ffmpeg -y -i Forevermore.mp3 -acodec pcm_s16le -f s16le -ac 2 -ar 44100 ForeverMore.pcm
```

| 参数               | 说明         |
| :----------------- | :----------- |
| -y                 | 允许覆盖     |
| -i Forevermore.mp3 | 源文件       |
| -acodec pcm_s16le  | 编码器       |
| -f s16le           | 强制文件格式 |
| -ac 2              | 双声道       |
| -ar 44100          | 采样率       |

用ffplay播放一下：

```shell
ffplay -ar 44100 -channels 2 -f s16le -i ForeverMore.pcm
```

可以正常播放，没啥问题。

数据准备好了，准备用SDL播放吧。

## 播放PCM数据的调用逻辑

逻辑不是很复杂，如下：

* SDL初始化音频模块：`SDL_Init`
* 打开音频设备：`SDL_OpenAudioDevice`
* 开始等待播放：`SDL_PauseAudioDevice`
* 填充数据：`SDL_QueueAudio`

下面一个个分开说。

### SDL初始化音频模块

SDL初始化个大模块的**函数原型**是：

```c++
int SDL_Init(Uint32 flags)
```

**参数**：是指定需要初始化的模块类型，可供选择的模块定义如下：

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

**返回值**：如果是0表示初始化成功，非0表示失败，通过`SDL_GetError() `调用查看失败信息。

###打开音频设备:SDL_OpenAudioDevice

**函数原型**为：

```c++
SDL_AudioDeviceID SDL_OpenAudioDevice(const char*          device,
                                      int                  iscapture,
                                      const SDL_AudioSpec* desired,
                                      SDL_AudioSpec*       obtained,
                                      int                  allowed_changes)
```

**功能**：该函数打开一个指定的音频设备。

**参数列表**：

| **device**          | 一个UTF-8类型的字符串，可以通过SDL_GetAudioDeviceName函数获取，不关心的传NULL就可以了 |
| ------------------- | ------------------------------------------------------------ |
| **iscapture**       | 该值非0的话，表示打开设备的目的是为了录制，而不是播放，你看SDL功能真强大 |
| **desired**         | 一个SDL_AudioSpec结构体，用来表示期望的输出格式。            |
| **obtained**        | 一个SDL_AudioSpec结构体，用来表示设备实际的输出格式。没用的话，传NULL也是可以的 |
| **allowed_changes** | 0或者其它如下表的值，用于表示播放时音频允许什么样的变化      |

| allowed_changes                  |                  |
| -------------------------------- | ---------------- |
| SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | 允许帧率改变     |
| SDL_AUDIO_ALLOW_FORMAT_CHANGE    | 允许格式改变     |
| SDL_AUDIO_ALLOW_CHANNELS_CHANGE  | 允许声道数改变   |
| SDL_AUDIO_ALLOW_ANY_CHANGE       | 允许以上任何改变 |

**返回值**：0表示出现异常，合法的设备ID值应该大于等于2。

### SDL_AudioSpec

这里需要说明一下SDL_AudioSpec结构体，原型如下：

```c++
/**
 *  The calculated values in this structure are calculated by SDL_OpenAudio().
 *
 *  For multi-channel audio, the default SDL channel mapping is:
 *  2:  FL FR                       (stereo)
 *  3:  FL FR LFE                   (2.1 surround)
 *  4:  FL FR BL BR                 (quad)
 *  5:  FL FR FC BL BR              (quad + center)
 *  6:  FL FR FC LFE SL SR          (5.1 surround - last two can also be BL BR)
 *  7:  FL FR FC LFE BC SL SR       (6.1 surround)
 *  8:  FL FR FC LFE BL BR SL SR    (7.1 surround)
 */
typedef struct SDL_AudioSpec
{
    int freq;                   /**< DSP频率 -- 即每秒的采样数 */
    SDL_AudioFormat format;     /**< 音频格式 */
    Uint8 channels;             /**< 声道数: 1 mono, 2 stereo */
    Uint8 silence;              /**< 音频缓冲区静音值(calculated) */
    Uint16 samples;             /**< 样本帧中的音频缓冲区大小（总样本除以通道数）*/
    Uint16 padding;             /**< 一些编译环境所必需的 */
    Uint32 size;                /**< 音频缓冲区大小（以字节为单位） */
    SDL_AudioCallback callback; /**< 为音频设备提供的回调（NULL表示使用SDL_QueueAudio（））. */
    void *userdata;             /**< Userdata传递给回调（忽略NULL回调）. */
} SDL_AudioSpec;
```

`SDL_AudioFormat`，决定了PCM数据采样位深以及存储方式，这个以后单独拎出来讲。在SDL中，SDL_AudioFormat可能值如下表：

| *8-bit support*                       | 支持8比特的格式有              |
| ------------------------------------- | ------------------------------ |
| AUDIO_S8                              | 有符号，8bit采样深度           |
| AUDIO_U8                              | 无符号，8bit采样深度           |
| ***16-bit support***                  | **支持16比特的格式有**         |
| AUDIO_S16LSB                          | 有符号16-bit，小端字节序       |
| AUDIO_S16MSB                          | 有符号16-bit，大端字节序       |
| AUDIO_S16SYS                          | 有符号16-bit，本地字节序       |
| AUDIO_S16                             | AUDIO_S16LSB                   |
| AUDIO_U16LSB                          | 无符号16-bit，小端字节序       |
| AUDIO_U16MSB                          | 无符号16-bit，大端字节序       |
| AUDIO_U16SYS                          | 无符号16-bit，本地字节序       |
| AUDIO_U16                             | AUDIO_U16LSB                   |
| ***32-bit support (new to SDL 2.0)*** | **支持32比特的格式有**         |
| AUDIO_S32LSB                          | 32-bit整数采样深度，小端字节序 |
| AUDIO_S32MSB                          | 32-bit整数采样深度，大端字节序 |
| AUDIO_S32SYS                          | 32-bit整数采样深度，本地字节序 |
| AUDIO_S32                             | AUDIO_S32LSB                   |
| ***float support (new to SDL 2.0)***  | **支持浮点型的格式有**         |
| AUDIO_F32LSB                          | 32-bit浮点型采样，小端字节序   |
| AUDIO_F32MSB                          | 32-bit浮点型采样，大端字节序   |
| AUDIO_F32SYS                          | 32-bit浮点型采样，本地字节序   |
| AUDIO_F32                             | AUDIO_F32LSB                   |

### 自定义回调函数SDL_AudioCallback

单独说一下SDL_AudioCallback函数吧，函数原型为：

```c++
/**
 *  \param userdata 应用指定的参数。保存在SDL_AudioSpec的userdata字段，该函数回调时会回传。
 *  \param stream 一个存储音频的buffer指针，回调时应该指向需要播放的数据指针。
 *  \param len    音频数据buffer长度，bit为单位。
 */
typedef void (SDLCALL * SDL_AudioCallback) (void *userdata, Uint8 * stream,
                                            int len);
```

**功能**：该函数被音频设备回调，用于填充需要播放的音频数据。

**参数**的作用已在函数doc中表明。

**注意**：在SDL2中，共提供了两套用于播放音频数据的API：

|        | SDL_AudioSpec        | 打开音频设备        | 等待数据播放         | 数据填充          |
| ------ | -------------------- | ------------------- | -------------------- | ----------------- |
| 第一套 | callback = mCallback | SDL_OpenAudio       | SDL_PauseAudio       | SDL_AudioCallback |
| 第二套 | callback = NULL      | SDL_OpenAudioDevice | SDL_PauseAudioDevice | SDL_QueueAudio    |

两套API不能相互混用，会播放不起来的。

确定用那套API，从`SDL_AudioSpec`的`callback`字段就能确定。为`NULL`的使用第二套API，否则使用第一套。

**优缺点**：

* 第一套：第一套实际上已经被SDL2打上了**遗留函数**（Legacy）的标签，**好处**只有一个兼容性了。**缺点**是，需要自己实现回调函数，显得很麻烦，结构不简洁。
* 第二套：优点和缺点除了和第一套相反外，填充数据更加高效。本文使用的就是这种，不仅少了实现回调函数的代码，而且不用考虑填充数据的时间。

目前网络上大量流行的是第一套API，感觉源头差不多都是雷神。我只能说，膜拜大神，但不能盲从。所以本文使用了第二套API，估计这是网上首次使用第二套API的博客了。

想要看第一套API demo的小伙伴，可以去雷神的博客：[传送门](https://blog.csdn.net/leixiaohua1020/article/details/40544521)

想要看第二套API demo的，就老实留下来，因为你可能找不到别的了。

## 等待播放：SDL_PauseAudioDevice

**函数原型**：

```c++
void SDL_PauseAudioDevice(SDL_AudioDeviceID dev, int pause_on)
```

**功能**：使用此功能暂停或取消暂停指定设备上的音频播放。

参数列表：

| **dev**      | 设备ID，调用SDL_OpenAudioDevice函数返回值可以获取设备ID |
| ------------ | ------------------------------------------------------- |
| **pause_on** | 0取消暂停，非0表示暂停                                  |

## 填充数据：SDL_QueueAudio

**函数原型**：

```c++
int SDL_QueueAudio(SDL_AudioDeviceID dev, const void* data, Uint32 len)
```

**功能**：使用此函数可以在回调设备（即使用了第一套API需要回调函数填充数据的设备）上缓存更多音频，而不必通过回调函数填充音频数据。

参数列表：

| dev  | 设备ID                     |
| ---- | -------------------------- |
| data | 需要被填充的数据指针       |
| len  | 数据buffer长度，byte为单位 |

返回值：0表示工程，非零表示出现异常，可以通过SDL_GetError()获取更多异常信息。

接下来，上代码

## 代码

CMakeLists.txt文件：

```cmake
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

add_executable(PlaySDL main.cpap)

target_link_libraries(PlaySDL SDL2)
```

Main.cpp文件：

```c++
#include <iostream>

#include <SDL.h>

using namespace std;

int main() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        cout << "SDL could not initialized with error: " << SDL_GetError() << endl;
        return -1;
    }

    SDL_AudioSpec desired_spec;
    desired_spec.freq = 44100;
    desired_spec.format = AUDIO_S16SYS;
    desired_spec.channels = 2;
    desired_spec.silence = 0;
    desired_spec.samples = 1024;
    desired_spec.callback = NULL;

    SDL_AudioDeviceID deviceID;
    if ((deviceID = SDL_OpenAudioDevice(NULL, 0, &desired_spec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE)) < 2) {
        cout << "SDL_OpenAudioDevice with error deviceID : " << deviceID << endl;
        return -1;
    }

    FILE *pFile = fopen("ForeverMore_44100_2_16.pcm", "rb");
    if (pFile == NULL) {
        cerr << "ForeverMore_44100_2_16.pcm open failed" << endl;
    }
    Uint32 buffer_size = 4096;
    char *buffer = (char *) malloc(buffer_size);
    SDL_PauseAudioDevice(deviceID, 0);
    while (true) {
        if (fread(buffer, 1, buffer_size, pFile) != buffer_size) {
            cerr << "end of file" << endl;
            break;
        }
        SDL_QueueAudio(deviceID, buffer, buffer_size);
    }
    SDL_Delay(3*1000*60); // 暂停3分钟，等待播放完成。在做播放器时，可以通过ffmpeg获取duration时间，做适当延迟。
    SDL_CloseAudio();
    SDL_Quit();
    fclose(pFile);
}
```

已经提供提出PCM数据的方法了，这里就不把PCM数据放出来吧，有兴趣玩玩儿的自己动动手。