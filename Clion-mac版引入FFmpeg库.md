# Clion-mac版引入FFmepg库

## 碎碎念

利用FFmpeg库开发完了一个完整的Android播放器应用之后(后面抽空整理一下发出来)，我用起来并不是很爽。因为，我发现光是能开发播放器，并不能很好的解决实际工作场景下的问题。

现在市场上已经有很多播放器了，不管什么平台的，我们很少会从零开发播放器的机会。而现在的音视频领域的开发者，大部分时间都在做什么呢？

答案是**维护**。

我自己在工作中，做的最多的也是维护。说的直白一点，就是一线发现bug了，把日志丢给我们，我们定位后，能修复的修复，不能的给出解释，让客户规避。

常见的问题有：音视频同步、卡顿、噪点、黑屏、无声等。问题发生的原因，有时是解码能力、解码速度原因、有时可能网络原因，还有可能是片源问题。每当怀疑是片源问题时，总是感觉束手无策，没法直接证明，只能用其它手段曲线救国。

网上虽然有一些片源分析工具，但因为片源的问题往往千奇八怪，作用不大。

所以，准备用业务时间，基于FFmpeg自己撸一个工具类型的工程，专治片源的各种不服。这是开始的第一篇。

## 安装FFmpeg

直接按照官网的操作执行就成，哪里有你想要的操作系统版本和所有的FFmpeg历史版本。

[传送门](https://ffmpeg.org/download.html)

mac系统的小伙伴直接用这个命令也行：

``` 
brew install ffmpeg
```

安装完成后，可以通过如下命令查看ffmpeg目录：

```shell
brew info ffmpeg
```

输出如下：

```shell
[Superli-2:github superli$ brew info ffmpeg
ffmpeg: stable 4.1.1 (bottled), HEAD
Play, record, convert, and stream audio and video
https://ffmpeg.org/
/usr/local/Cellar/ffmpeg/3.4.2 (250 files, 50.9MB) *
  Poured from bottle on 2018-03-18 at 09:54:21
From: https://github.com/Homebrew/homebrew-core/blob/master/Formula/ffmpeg.rb
==> Dependencies
Build: nasm ✘, pkg-config ✘, texi2html ✘
Required: aom ✘, fontconfig ✘, freetype ✘, frei0r ✘, gnutls ✘, lame ✔, libass ✘, libbluray ✘, libsoxr ✘, libvorbis ✘, libvpx ✘, opencore-amr ✘, openjpeg ✘, opus ✘, rtmpdump ✘, rubberband ✘, sdl2 ✘, snappy ✘, speex ✘, tesseract ✘, theora ✘, x264 ✘, x265 ✘, xvid ✔, xz ✔
==> Options
--HEAD
	Install HEAD version
```

在输出中，有一个待会儿需要用到的信息——FFmpeg的安装路径：**/usr/local/Cellar/ffmpeg/3.4.2**

来看看里边都有些啥：

```shell
[Superli-2:3.4.2 superli$ ls -al
total 168
drwxr-xr-x  11 superli  admin    352  3 18  2018 .
drwxr-xr-x   3 superli  admin     96  3 18  2018 ..
drwxr-xr-x   3 superli  admin     96  2 12  2018 .brew
-rw-r--r--   1 superli  admin  65727  2 12  2018 Changelog
-rw-r--r--   1 superli  admin   1897  3 18  2018 INSTALL_RECEIPT.json
-rw-r--r--   1 superli  admin   4368  2 12  2018 LICENSE.md
-rw-r--r--   1 superli  admin   1893  2 12  2018 README.md
drwxr-xr-x   5 superli  admin    160  2 12  2018 bin
drwxr-xr-x  11 superli  admin    352  2 12  2018 include
drwxr-xr-x  39 superli  admin   1248  2 12  2018 lib
drwxr-xr-x   5 superli  admin    160  2 12  2018 share

```

其中的lib和include文件夹，待会儿我们需要用到。

## 新建一个Clion工程

虽然FFmpeg是用C语言编写的，但我还是选择了C++可执行的工程。谁让Java是我的入门语言呢。

## 引入库

引入和链接库的动作，当然是在Cmake文件中啦。Clion的Cmake文件是CmakeLists.txt。比如，我这个工程的CmakeLists.txt文件长这样：

```cmake
cmake_minimum_required(VERSION 3.10)
project(PlayFFmpeg) # 指定的项目名称

set(FFMPEG_DIR /usr/local/Cellar/ffmpeg/3.4.2) # FFmpeg的安装目录，可以通过命令"brew info ffmpeg"获取
include_directories(${FFMPEG_DIR}/include/) # 头文件搜索路径
link_directories(${FFMPEG_DIR}/lib/) # 动态链接库或静态链接库的搜索路径
set(CMAKE_CXX_STANDARD 11)

add_executable(PlayFFmpeg main.cpp)

# 添加链接库
target_link_libraries(
        PlayFFmpeg
        avcodec
        avdevice
        avfilter
        avformat
)
```

额，不好意思，加注释的部分，除了`project(PlayFFmpeg`意外，都是我自己添加的，这是一个引入了FFMepg库，可以正常使用的Cmake文件。注释部分的内容说明了当前代码行的作用，就不另作说明了。

另外需要注意的是，别把`add_executable()`和`target_link_libraries()`的顺序搞混了，如果你的Clion告诉你：

```shell
CMake Error at CMakeLists.txt:12 (target_link_libraries):
  Cannot specify link libraries for target "PlayFFmpeg" which is not built by
  this project.
```

记得检查一下`add_executable()`和`target_link_libraries()`的顺序。

另外，代码中`target_link_libraries()`链接的库并不多，你可以按需使用：你需要的所有FFmpeg的库，都在FFmpeg安装路径的lib目录下。

不管是windows还是mac系统，使用时，记得掐头去尾，例如：`libavcodec.a` 需要写成`avcodec`。

## 示例代码检查是否成功

```c++
#include <iostream>
extern "C" { 
#include <libavformat/avformat.h>
}
using namespace std;
int main() {
    cout << "Hello, FFmpeg!" << endl;
    cout << avformat_configuration() << endl; // 打印libavformat构建时配置信息。
    return 0;
}
```

FFmpeg是纯C语言开发的，使用它的库时，需要申明：`extern "C"`。

运行一下：

```shell
Hello, FFmpeg!
--prefix=/usr/local/Cellar/ffmpeg/3.4.2 --enable-shared --enable-pthreads --enable-version3 --enable-hardcoded-tables --enable-avresample --cc=clang --host-cflags= --host-ldflags= --disable-jack --enable-gpl --enable-libmp3lame --enable-libx264 --enable-libxvid --enable-opencl --enable-videotoolbox --disable-lzma

```

没什么问题，over！

后面应该会基于这个工程，做一个片源分析的工作。争取最后整理出一个可以解决部分工作中问题的工具出来。

