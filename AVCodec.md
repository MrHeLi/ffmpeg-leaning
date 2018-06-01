# FFmpeg: AVCodec结构体解析

## AVCodec

`AVCodec`是FFmpeg比较重要的结构体之一，主要用于存储编解码器的信息。

## 术语

术语链接：

https://blog.csdn.net/qq_25333681/article/details/80088126

##结构定义及成员解读

```C

typedef struct AVCodec {
    /**
     * 编解码器实现的名称。
     * 该名称是全局唯一的（但编码器和解码器可以共享名称）。
     * 这是从用户角度查找编解码器的主要方式。
     */
    const char *name;
    /**
     * 编解码器的描述性名称，比前面的名称更具可读性。
     * 您应该使用NULL_IF_CONFIG_SMALL（）宏来定义它。
     */
    const char *long_name;
    enum AVMediaType type;//编解码器类型，视频，音频，或者字幕
    enum AVCodecID id;//全局唯一的编解码器ID
    /**
     * Codec capabilities.
     * see AV_CODEC_CAP_*
     */
    int capabilities;
    const AVRational *supported_framerates; ///支持帧率的数组，用于视频
    const enum AVPixelFormat *pix_fmts;     ///< 支持的像素格式数组，或者如果未知，则为NULL，数组以-1结尾。用于视频
    const int *supported_samplerates;       ///< 支持的音频采样率数组，或者如果未知，则为NULL，数组以0结尾。用于音频
    const enum AVSampleFormat *sample_fmts; ///<支持的采样数组，或者如果未知，则为NULL，数组以-1结尾。用于音频
    const uint64_t *channel_layouts;         ///< 支持声道数组，如果未知，则为NULL。 数组以0结尾，用于音频
    uint8_t max_lowres;                     ///< maximum value for lowres supported by the decoder
    const AVClass *priv_class;              ///< 私有上下文的AVClass
    const AVProfile *profiles;              ///< 已识别配置文件的数组，或者如果未知，则为NULL，数组以{FF_PROFILE_UNKNOWN}结尾

    /*****************************************************************
     * 以下所有的字段都不是公共API，不可在libavcodec以外使用。以后新增字段都会放在上面。
     *****************************************************************
     */
    int priv_data_size;//私有数据大小
    struct AVCodec *next;
    /**
     * @name Frame-level threading support functions
     * @{
     */
    /**
     * 如果已定义，则在创建线程上下文时调用它们。
     * 如果编解码器在init（）中分配可写表，请在此处重新分配它们。
     * priv_data将被设置为原件的副本。
     */
    int (*init_thread_copy)(AVCodecContext *);
    /**
     * Copy necessary context variables from a previous thread context to the current one.
     * If not defined, the next thread will start automatically; otherwise, the codec
     * must call ff_thread_finish_setup().
     *
     * dst and src will (rarely) point to the same context, in which case memcpy should be skipped.
     */
    int (*update_thread_context)(AVCodecContext *dst, const AVCodecContext *src);
    /** @} */

    /**
     * 私有编解码器默认值。
     */
    const AVCodecDefault *defaults;

    /**
     * 初始化时从avcodec_register（）调用的编解码器静态数据。
     */
    void (*init_static_data)(struct AVCodec *codec);

    int (*init)(AVCodecContext *);
    int (*encode_sub)(AVCodecContext *, uint8_t *buf, int buf_size,
                      const struct AVSubtitle *sub);
    /**
     * Encode data to an AVPacket.
     *
     * @param      avctx          codec context
     * @param      avpkt          output AVPacket (may contain a user-provided buffer)
     * @param[in]  frame          AVFrame containing the raw data to be encoded
     * @param[out] got_packet_ptr encoder sets to 0 or 1 to indicate that a
     *                            non-empty packet was returned in avpkt.
     * @return 0 on success, negative error code on failure
     */
    int (*encode2)(AVCodecContext *avctx, AVPacket *avpkt, const AVFrame *frame,
                   int *got_packet_ptr);
    int (*decode)(AVCodecContext *, void *outdata, int *outdata_size, AVPacket *avpkt);
    int (*close)(AVCodecContext *);
    /**
     * Decode/encode API with decoupled packet/frame dataflow. The API is the
     * same as the avcodec_ prefixed APIs (avcodec_send_frame() etc.), except
     * that:
     * - never called if the codec is closed or the wrong type,
     * - AVPacket parameter change side data is applied right before calling
     *   AVCodec->send_packet,
     * - if AV_CODEC_CAP_DELAY is not set, drain packets or frames are never sent,
     * - only one drain packet is ever passed down (until the next flush()),
     * - a drain AVPacket is always NULL (no need to check for avpkt->size).
     */
    int (*send_frame)(AVCodecContext *avctx, const AVFrame *frame);
    int (*send_packet)(AVCodecContext *avctx, const AVPacket *avpkt);
    int (*receive_frame)(AVCodecContext *avctx, AVFrame *frame);
    int (*receive_packet)(AVCodecContext *avctx, AVPacket *avpkt);
    /**
     * Flush buffers.
     * Will be called when seeking
     */
    void (*flush)(AVCodecContext *);
    /**
     * Internal codec capabilities.
     * See FF_CODEC_CAP_* in internal.h
     */
    int caps_internal;
} AVCodec;
```

## AVMediaType

前面的AVCodec结构体解析中，已经说明AVMediaType表示的是编解码器类型，一般为视频、音频或者字幕。

定义如下：

```c
enum AVMediaType {
    AVMEDIA_TYPE_UNKNOWN = -1,  ///< 通常视为AVMEDIA_TYPE_DATA类型
    AVMEDIA_TYPE_VIDEO,//视频
    AVMEDIA_TYPE_AUDIO,//音频
    AVMEDIA_TYPE_DATA,          ///< 不透明的数据信息，通常是连续的。
    AVMEDIA_TYPE_SUBTITLE,//字幕
    AVMEDIA_TYPE_ATTACHMENT,    ///< 不透明的数据信息，通常很不连续
    AVMEDIA_TYPE_NB//牛逼（NB）类型？无法理解，谁知道的告诉我啊
};
```

## AVCodecID

定义：超长代码，拣极少一部分贴，见名知意

```
enum AVCodecID {
    AV_CODEC_ID_NONE,

    /* 视频编解码器 */
    AV_CODEC_ID_MPEG1VIDEO,//用于MPEG1
    AV_CODEC_ID_MPEG2VIDEO, ///< 用于MPEG-1/2视频解码的首选ID
#if FF_API_XVMC
    AV_CODEC_ID_MPEG2VIDEO_XVMC,
#endif /* FF_API_XVMC */
    AV_CODEC_ID_H261,
    AV_CODEC_ID_H263,
    AV_CODEC_ID_MJPEG,
    AV_CODEC_ID_MPEG4,
    AV_CODEC_ID_WMV1,
    AV_CODEC_ID_H264,
    AV_CODEC_ID_VP3,
    AV_CODEC_ID_VP5,
    AV_CODEC_ID_VP6,
    AV_CODEC_ID_GIF,
    /* PCM 编解码器 */
    AV_CODEC_ID_FIRST_AUDIO = 0x10000,     ///< A dummy id pointing at the start of audio codecs
    /……
};
```

## AVPixelFormat

代码依然超长

```
enum AVPixelFormat {
    AV_PIX_FMT_NONE = -1,
    AV_PIX_FMT_YUV420P,   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
    AV_PIX_FMT_YUYV422,   ///< packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
    AV_PIX_FMT_RGB24,     ///< packed RGB 8:8:8, 24bpp, RGBRGB...
    AV_PIX_FMT_BGR24,     ///< packed RGB 8:8:8, 24bpp, BGRBGR...
    AV_PIX_FMT_YUV422P,   ///< planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
    AV_PIX_FMT_YUV444P,   ///< planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
    AV_PIX_FMT_YUV410P,   ///< planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
    AV_PIX_FMT_YUV411P,   ///< planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y 
    /*
     * 略
    */
    AV_PIX_FMT_NB         ///< number of pixel formats, DO NOT USE THIS if you want to link with shared libav* because the number of formats might differ between versions
};
```



## AVSampleFormat

```c
enum AVSampleFormat {
    AV_SAMPLE_FMT_NONE = -1,
    AV_SAMPLE_FMT_U8,          ///< unsigned 8 bits
    AV_SAMPLE_FMT_S16,         ///< signed 16 bits
    AV_SAMPLE_FMT_S32,         ///< signed 32 bits
    AV_SAMPLE_FMT_FLT,         ///< float
    AV_SAMPLE_FMT_DBL,         ///< double

    AV_SAMPLE_FMT_U8P,         ///< unsigned 8 bits, planar
    AV_SAMPLE_FMT_S16P,        ///< signed 16 bits, planar
    AV_SAMPLE_FMT_S32P,        ///< signed 32 bits, planar
    AV_SAMPLE_FMT_FLTP,        ///< float, planar
    AV_SAMPLE_FMT_DBLP,        ///< double, planar
    AV_SAMPLE_FMT_S64,         ///< signed 64 bits
    AV_SAMPLE_FMT_S64P,        ///< signed 64 bits, planar

    AV_SAMPLE_FMT_NB           ///< Number of sample formats. DO NOT USE if linking dynamically
};
```

# 参考链接

雷大神：https://blog.csdn.net/leixiaohua1020/article/details/14215833