# FFmpeg：AVFrame结构体分析

最近发现了ffmpeg有个网页版的文档，分享给大家：[ffmpeg文档](https://www.ffmpeg.org/doxygen/4.1/index.html)，查询起来看是挺方便的。现在才发现，是不是有点太晚了？

之前都是直接看源码上的注释，反过来想想，那些注释写的也都听清晰易懂的哈（自我安慰）。

## AVFrame

`AVFrame`结构体描述了解码后的（原始）音频或视频数据。

`AVFrame`通常被分配一次，然后多次重复使用以持有不同的数据（例如，单个AVFrame持有从解码器接收的帧）。 在这种情况下，av_frame_unref()将释放帧所持有引用，再次重用之前将其重置为初始状态。

`AVFrame`描述的数据通常通过`AVBuffer` API引用计数。 底层缓冲区引用存储在`AVFrame.buf `或`AVFrame.extended_buf`中。 如果设置了至少一个引用，即`AVFrame.buf [0]！= NULL`，则`AVFrame`被认为是引用计数。 在这种情况下，每"平面（plane）"数据必须包含在`AVFrame.buf`或`AVFrame.extended_buf`中的一个缓冲区中。 所有数据可能只有一个缓冲区，或者每个平面可能有一个单独的缓冲区，或者介于两者之间的任何内容。

只要使用FFmpeg做解码，必然会使用到`AVFrame`结构体，它比较重要的字段有：

* `*data[AV_NUM_DATA_POINTERS]`：存放解码后的原始媒体数据的指针数组。
  * 对于视频数据而言，planar(YUV420)格式的数据，Y、U、V分量会被分别存放在data[0]、data[1]、data[2]……中。packet格式的数据会被存放在data[0]中。
  * 对于音频数据而言，data数组中，存放的是channel的数据，例如，data[0]、data[1]、data[2]分别对应channel 1，channel 2 等。
* linesize[AV_NUM_DATA_POINTERS]：视频或音频帧数据的行宽数组。
  * 对video而言：每个图片行的字节大小。linesize大小应该是CPU对齐的倍数，对于现代pc的CPU而言，即32或64的倍数。
  * 对audio而言：代表每个平面的字节大小。只会使用linesize[0]。 对于plane音频，每个通道 的plane必须大小相同。
* **extended_data：
  * 对于视频数据：只是简单的指向data[]。
  * 对于音频数据：planar音频，每个通道都有一个单独的数据指针，而linesize [0]包含每个通道缓冲区的大小。 对于packet音频，只有一个数据指针，linesize [0]包含所有通道的缓冲区总大小。
* key_frame：当前帧是否为关键帧，1表示是，0表示不是。
* pts：以time_base为单位的呈现时间戳（应向用户显示帧的时间）。

##  初始化与清理

```c
AVFrame* avFrame = av_frame_alloc(); // 初始化
av_frame_free(&avFrame); // 释放
av_frame_unref(avFrame); // 释放AVFrame的所有引用，以便重用
```

`av_frame_alloc`：分配AVFrame并将其字段设置为默认值。主要该函数只分配AVFrame的空间，它的data字段的指定的buffer需要其它函数分配。

`av_frame_free`：AVFrame的释放，必须使用该函数释放帧和其中的任何动态分配的对象，例如extended_data。 如果帧被引用计数，则它的引用计数-1。

`av_frame_unref`：取消引用帧引用的所有缓冲区并重置帧字段。

## 结构定义及成员解读

```c
typedef struct AVFrame {
#define AV_NUM_DATA_POINTERS 8
    uint8_t *data[AV_NUM_DATA_POINTERS]; // 存放媒体数据的指针数组
    int linesize[AV_NUM_DATA_POINTERS]; // 视频或音频帧数据的行宽
    uint8_t **extended_data; // 音频或视频数据的指针数组。
    int width, height; // 视频帧的款和高

    /**
     * number of audio samples (per channel) described by this frame
     */
    int nb_samples; // 当前帧的音频采样数（每个通道）
    int format; // 视频帧的像素格式，见enum AVPixelFormat，或音频的采样格式，见enum AVSampleForma
    int key_frame; // 当前帧是否为关键帧，1表示是，0表示不是。
    AVRational sample_aspect_ratio; // 视频帧的样本宽高比
    int64_t pts; // 以time_base为单位的呈现时间戳（应向用户显示帧的时间）。
    int64_t pkt_dts; // 从AVPacket复制而来的dts时间，当没有pts时间是，pkt_dts可以替代pts。
    int coded_picture_number; // 按解码先后排序的，解码图像数
    int display_picture_number; // 按显示前后排序的，显示图像数。
    int quality; // 帧质量，从1～FF_LAMBDA_MAX之间取之，1表示最好，FF_LAMBDA_MAX之间取之表示最坏。
    void *opaque; // user的私有数据。
    int interlaced_frame; // 图片的内容是隔行扫描的（交错帧）。
    int top_field_first; // 如果内容是隔行扫描的，则首先显示顶部字段。
    int sample_rate; // 音频数据的采样率
    uint64_t channel_layout; // 音频数据的通道布局。
    /**
     * AVBuffer引用，当前帧数据。 如果所有的元素为NULL，则此帧不是引用计数。 必须连续填充此数组，
     * 即如果buf [i]为非NULL，j <i，buf[j]也必须为非NULL。
     *
     * 每个数据平面最多可以有一个AVBuffer，因此对于视频，此数组始终包含所有引用。 
     * 对于具有多于AV_NUM_DATA_POINTERS个通道的平面音频，可能有多个缓冲区可以容纳在此阵列中。 
     * 然后额外的AVBufferRef指针存储在extended_buf数组中。
     */
    AVBufferRef *buf[AV_NUM_DATA_POINTERS];
    AVBufferRef **extended_buf; // AVBufferRef的指针
    int        nb_extended_buf; // extended_buf的数量
    enum AVColorSpace colorspace; // YUV颜色空间类型。
    int64_t best_effort_timestamp; // 算法预测的timestamp
    int64_t pkt_pos; // 记录上一个AVPacket输入解码器的位置。
    int64_t pkt_duration; // packet的duration
    AVDictionary *metadata;
    int channels; // 音频的通道数。
    int pkt_size; // 包含压缩帧的相应数据包的大小。

} AVFrame;
```

