# FFmpeg: AVCodecContext结构体解析

# AVCodecContext

`AVCodecContext`也是FFmpeg使用过程中比较重要的结构体，保存了编解码器上下文相关信息。不管是编码，还是解码都会用到，但在两种不同应用场景中，结构体中部分字段作用和表现并不一致，这一点需要特别注意。

`AVFormatContext`包含了一个媒体流的解码器的上下文信息，比较重要的有：

- AVMediaType：解码器类型，表示这个解码器是用来处理什么流的，音频、视频还是字母等。
- AVCodec：解码器结构体指针，在AVCodecContext创建之初就传进来。初始化后不可更改。
- AVCodecID：是什么类型的解码器，AV_CODEC_ID_MPEG1VIDEO还是AV_CODEC_ID_H263等。
- bit_rate：平均比特率，编码时为用户设置。解码时也可以用户设置，但也可能被libavcodec覆盖。
- gop_size: GOP的大小，编码时用户设置。解码时不会使用。
- sample_aspect_ratio：采样率。
- sample_rate：音频采样率。
- channels：声道数（音频概念）。
- AVSampleFormat：音频采样格式。

# 初始化

```c
AVCodec *videoCodec = avcodec_find_decoder(formatCtx->streams[videoStream]->codecpar->codec_id);
AVCodecContext *videoCodecContext = avcodec_alloc_context3(videoCodec);
```

首先使用`AVFormatContext`中保存的解码器的codec_id信息，通过`avcodec_find_decoder`函数，获取一个解码器（AVCodec)。然后以此为参数，通过`avcodec_alloc_context3`函数分初始化`AVCodecContext`指针。

# 结构定义及成员解读

```c
typedef struct AVCodecContext {
    const AVClass *av_class; // av_log信息，通过avcodec_alloc_context3函数设置
    int log_level_offset; // log 级别
    enum AVMediaType codec_type; // 解码器类型，例如：AVMEDIA_TYPE_VIDEO、AVMEDIA_TYPE_AUDIO
    const struct AVCodec  *codec; // 解码器结构体指针，在AVCodecContext创建之初就传进来。
    enum AVCodecID     codec_id; // 解码器ID
    unsigned int codec_tag; // 解码器tag
    void *priv_data; // 私有数据
    // 内部使用的上下文，通常被用在libavcodec的函数中，并且不是解码器指定的却别与priv_data
    struct AVCodecInternal *internal;
    void *opaque; // 用户的私有数据，可以携带特定应用内容。不管是编码还是解码时，都是被用户设置使用。
    // 平均比特率，编码时为用户设置。解码时也可以用户设置，但也可能被libavcodec覆盖。
    int64_t bit_rate;
    int bit_rate_tolerance; // 允许比特流偏离参考的比特数。 编码时用户设置，解码时不会用到。
    int global_quality; // 编解码器的全局质量，应与MPEG-1/2/4 qscale成比例。编码时使用
    int compression_level; // 压缩水平，编码时使用
    AVRational time_base; // 时基
    int width, height; // 只有在视频编解码时使用。
    // 比特流宽度/高度可以与宽度/高度不同，例如 当在输出之前裁剪解码的帧或者启用lowres时。解码时使用，可能被解码器覆盖。
    int coded_width, coded_height;
    int gop_size; // GOP的大小，编码时用户设置。解码时不会使用
    // 像素格式，请参阅AV_PIX_FMT_xxx。 如果从头文件中知道，可以由分路器设置。 如果解码器知道更好，可能会  被解码器覆盖。
    enum AVPixelFormat pix_fmt;
    int max_b_frames; // 非B帧之间的最大B帧数
    int has_b_frames; // 解码器中帧重排序缓冲区的大小。 对于MPEG-2，它是1个IPB帧或0个低延迟IP帧。
    int slice_count; // slice数
    int *slice_offset; // 以字节为单位切片帧中的偏移量
    AVRational sample_aspect_ratio; // 采样率
    int slice_flags; // slice 标志
    int keyint_min; // 最小GOP大小
    int refs; // 参考帧数
    enum AVColorSpace colorspace; // YUV色彩空间类型。
    enum AVColorRange color_range; // MPEG与JPEG YUV范围。
    int slices; // 切片数量。 表示图片细分的数量。 用于并行解码。
    int sample_rate; //采样率，只用于音频
    int channels; // 声道数
    enum AVSampleFormat sample_fmt; // 音频采样格式
    int frame_size; // 音频帧中每个声道的采样数。
    int frame_number; // 帧计数器，由libavcodec设置。
    int block_align; // 每个数据包的字节数
    uint64_t channel_layout;
    enum AVSampleFormat request_sample_fmt; // 所需的采样格式
  	// 在每个帧的开头调用此回调以获取它的数据缓冲区。
    int (*get_buffer2)(struct AVCodecContext *s, AVFrame *frame, int flags);
		// 通过vcodec_decode_video2()和avcodec_decode_audio4()调用返回的解码后的帧数
    int refcounted_frames; 
    struct AVHWAccel *hwaccel; // 正在使用的硬件加速器
		// 硬件加速器上下文 对于某些硬件加速器，用户需要提供全局上下文。
    void *hwaccel_context;
    int thread_count; // 线程计数用于决定应该将多少独立任务传递给execute（）
    /**
     * 使用哪种多线程方法。
     * 使用FF_THREAD_FRAME会使每个线程的解码延迟增加一帧，因此无法提供未来帧的客户端不应使用它。
     */
    int thread_type;
    int active_thread_type; // 编解码器正在使用哪种多线程方法。
    // 编解码器可以调用它来执行几个独立的事情。只有在完成所有任务后才会返回。用户可以用一些多线程实现替换它，默认实现将串行执行这些部分。
    int (*execute)(struct AVCodecContext *c, int (*func)(struct AVCodecContext *c2, void *arg), void *arg2, int *ret, int count, int size);
    enum AVDiscard skip_loop_filter; // 对所选帧进行跳过循环过滤。
    enum AVDiscard skip_frame; // 跳过所选帧的解码操作。
    uint8_t *subtitle_header; // 包含文本字幕样式信息的字母头。
    int subtitle_header_size; // 字母头的大小
    // 仅限音频。 编码器在音频开头插入的“启动”样本（填充）的数量。即调用者必须丢弃此数量的前导解码样本，以获得原始音频而不带前导填充。
    int initial_padding;
    AVRational pkt_timebase; // pkt_dts / pts和AVPacket.dts / pts的时基。
    // PTS校正的当前统计数据。
    int64_t pts_correction_num_faulty_pts; /// 到目前为止，不正确的PTS值的数量
    int64_t pts_correction_num_faulty_dts; /// 到目前为止，DTS值的错误数量
    int64_t pts_correction_last_pts;       /// 最后一帧的PTS
    int64_t pts_correction_last_dts;       /// 最后一帧的DTS
    char *codec_whitelist; // 解码器白名单，以 '，'分割。
    // 仅限音频。 编码器附加到音频末尾的填充量（在样本中）。 即调用者必须从流的末尾丢弃此数量的解码样本，以获得原始音频而不进行任何尾随填充。
    int trailing_padding;
    int64_t max_pixels; // 每个图像允许的最大像素点数。
} AVCodecContext;
```

`AVCodecContext`	结构题的成员还有很多，这里只是摘取了常见的做了说明，还有很多等待大家发掘。