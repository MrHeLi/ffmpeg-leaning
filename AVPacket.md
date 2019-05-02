# FFmpeg：AVPacket结构体分析

## AVPacket

文档地址：[传送门](https://www.ffmpeg.org/doxygen/4.1/index.html)

在AVPacket结构体的说明部分：有这么一段描述，足够说明它的作用和重要性。

> 该结构存储压缩数据。 它通常由解复用器导出，然后作为输入传递给解码器，或作为编码器的输出接收，然后传递给复用器。
>
> 对于视频而言，它通常应包含一个压缩帧。 对于音频，它可能包含几个压缩帧。 允许编码器输出空包，没有压缩数据，只包含不重要的附加信息数据。例如在编码结束时更新一些流参数。
>
> AVPacket是FFmpeg中为数不多的结构之一，其大小是公共ABI的一部分。 因此，它可以在栈上分配，并且不会添加新的字段，除非libavcodec和libavformat有大的改动。

虽然前面引用部分已经说了`AVPacket`的作用时机，但这里还是单独强调和总结一下：

* 解码时：媒体源数据解封装(解复用)后，解码前。
* 编码时：YUV数据编码后，封装前。

`AVPacket`主要保存一些媒体流的基本信息，例如PTS、DTS时间。最重要的当然就是媒体数据的buffer地址了。

比较重要的有：

* pts：控制显示的pts时间
* dts：控制解码的dts时间
* *data：媒体数据buffer的指针
* duration：AVStream-> time_base单位中此数据包的持续时间，如果未知则为0。 在演示顺序中等于next_pts  -  this_pts。

## 初始化与清理

```c
AVPacket* avPacket = av_packet_alloc(); // 初始化
av_packet_unref(avPacket); // 清理
```

没什么稀奇，都是调用特定的函数来处理。这里需要提一下，虽然在第一小节提到，AVPacket是可以在栈内存上分配，也就是这样：`AVPacket avPacket`，但如非必要，我还是不建议这么做，毕竟有现成的API，就别给自己挖坑了。

还有一个函数：

`int av_packet_ref (AVPacket *dst, const AVPacket *src)`

它的作用是：

> 设置对给定数据包描述的数据的新引用。
>
> 如果src的data指针不为空，则将指针地址作为dst中data指针的值。 否则在dst中分配一个新缓冲区并将数据从src复制到其中。
>
> 所有其他字段都是从src复制的。

这个函数和av_packet_unref函数的作用刚好相反。我们详细了解一下这背后的故事：

我们在一开始有提到`AVPacket`保存了数据buffer的地址，所以，它实际上并没有包含数据内存本身，只是在它的字段中，用了一个`uint8_t *data;`来保存数据buffer的地址。

`av_packet_ref`函数的作用，就是从已有的AVPacket中复制一份出来。关于data部分，如果src的数据是引用计数的，直接把地址拷贝一份，然后把对应buffer的引用计数+1。如果不是，需要新分配一份保存数据的内存空间，并把src中的数据拷贝过来。

而`av_packet_unref`函数，则会看buffer的引用计数器，如果不为0就-1，为零的话则会清楚掉buffer，AVPacket的其它数据也会回到初始状态。

## 结构定义及成员解读

```c
typedef struct AVPacket {
    AVBufferRef *buf; // data的buffer引用指针计数结构体
    int64_t pts; // 控制显示的pts时间
    int64_t dts; // 控制解码的dts时间
    uint8_t *data; // 媒体数据buffer的指针
    int   size; // 数据大小
    int   stream_index; // 流index
    int   flags; // AV_PKT_FLAG值的组合
    AVPacketSideData *side_data; // 容器可以提供的附加数据包数据。 数据包可以包含几种类型的辅助信息。
		// AVStream-> time_base单位中此数据包的持续时间，如果未知则为0。 在演示顺序中等于next_pts  -  this_pts。
    int64_t duration;
    int64_t pos; // 流中的字节位置，如果未知则为-1
} AVPacket;
```

来扩展一下用来计数的结构体：

```c
typedef struct AVBufferRef {
    AVBuffer *buffer;
    // 当且仅当这是对缓冲区的唯一引用时才被认为是可写的，在这种情况下av_buffer_is_writable（）返回1。
    uint8_t *data;
    int      size; // 数据大小（以字节为单位）
} AVBufferRef;
```

这个结构体，保存了对数据缓冲区的引用。