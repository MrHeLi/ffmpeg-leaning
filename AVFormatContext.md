# FFmpeg: AVFormatContext结构体解析

## AVFormatContext

`AVFormatContext `是API中直接接触到的结构体，位于`avformat.h`中，是音视频数据的一种抽象和封装，该结构体的使用，贯穿了ffmpeg使用的整个流程，现在数据为王，不是没有道理的。

## 术语

* 流(stream)，一种用于形容音视频数据的名词。音视频播放中，数据都是通过循环，不断处理直到结束，跟水流及其相似。
* 混流/分流(muxing/demuxing)、混流器/分流器(muxer/demuxer):概念详情，请参考https://blog.csdn.net/qq_25333681/article/details/80088126文中**MUX和DEMUX**部分。

## 结构定义及成员解读

```c
typedef struct AVFormatContext {
    /**
     * A class for logging and @ref avoptions. Set by avformat_alloc_context().
     * Exports (de)muxer private options if they exist.
     */
    const AVClass *av_class;

    /**
     * 输入容器格式.
     * 用于分流，通过avformat_open_input()设置.
     */
    struct AVInputFormat *iformat;

    /**
     * 输出容器格式。
     *
     * 用于混流，必须在avformat_write_header()调用前设置.
     */
    struct AVOutputFormat *oformat;

    /**
     * Format private data. This is an AVOptions-enabled struct
     * if and only if iformat/oformat.priv_class is not NULL.
     *
     * - muxing: set by avformat_write_header()
     * - demuxing: set by avformat_open_input()
     */
    void *priv_data;

    /**
     * I/O 上下文.
     *
     * - 分流: 在avformat_open_input() 之前设置(用户必须手动释放)或者通过avformat_open_input()
     *		  自动设置.
     * - 混流: 在avformat_write_header()之前设置.用户必须注意及时关闭/释放IO上下文。
     *
     * 不要设置AVFMT_NOFILE标志给iformat/oformat.flags。因为这种情况下，该值为NULL，混/分流器会通过	 * 其它方式处理I/O。
     */
    AVIOContext *pb;

    /* 后面都是流信息 */
    /**
     * 信号流属性标志，AVFMTCTX_*的组合.
     * 通过libavformat设置.
     */
    int ctx_flags;

    /**
     * AVFormatContext.streams中的元素数量，其实就是流的总数.
     *
     * 通过avformat_new_stream()设置, 禁止其它代码修改。
     */
    unsigned int nb_streams;
    /**
     * 媒体中减重，所有流的列表，新的流由avformat_new_stream()创建。
     *
     * - 分流: 流在avformat_open_input()函数中由libavformat创建。如果AVFMTCTX_NOHEANDER被设置带  	 *		  ctx_flags中，新的流可能出现在av_read_frame()中。
     * - 混流: 流在avformat_write_header()函数之前被用户创建
     *
     * 在avformat_free_context()函数中，通过libavformat释放。
     */
    AVStream **streams;

    /**
     * 输入输出文件名
     *
     * - 分流: 通过avformat_open_input()设置。
     * - 混流: 在avformat_write_header()调用前，可以被使用者设置。
     */
    char filename[1024];

    /**
     * 组件第一帧的位置，用AV_TIME_BASE分数秒表示。禁止直接设置，由AVStream的值推导而来。
     *
     * - 分流：通过libavformat设置.
     */
    int64_t start_time;

    /**
     * 留的时长，以AV_TIME_BASE分数秒为单位。如果您不知道任何单个流的持续时间，也不设置其中的任何一个，	  * 请仅设置此值。 如果没有设置，该值可以被AVStream推导出来
     *
     * 只用于分流操作，通过libavformat设置。
     */
    int64_t duration;

    /**
     * 总流的比特率以bit/s为单位，如果流不可用，该值为0。如果流文件大小和时长已知，不要直接设置它，  		 * FFmpeg会自动计算。
     */
    int64_t bit_rate;

    unsigned int packet_size;
    int max_delay;

    /**
     * 修改分/混流器操作的标志，一个AVFMT_FLAG_*的组合。
     * 在avformat_open_input() / avformat_write_header()调用之前用户自行设置.
     */
    int flags;
#define AVFMT_FLAG_GENPTS       0x0001 ///< Generate missing pts even if it requires parsing future frames.
#define AVFMT_FLAG_IGNIDX       0x0002 ///< Ignore index.
#define AVFMT_FLAG_NONBLOCK     0x0004 ///< Do not block when reading packets from input.
#define AVFMT_FLAG_IGNDTS       0x0008 ///< Ignore DTS on frames that contain both DTS & PTS
#define AVFMT_FLAG_NOFILLIN     0x0010 ///< Do not infer any values from other values, just return what is stored in the container
#define AVFMT_FLAG_NOPARSE      0x0020 ///< Do not use AVParsers, you also must set AVFMT_FLAG_NOFILLIN as the fillin code works on frames and no parsing -> no frames. Also seeking to frames can not work if parsing to find frame boundaries has been disabled
#define AVFMT_FLAG_NOBUFFER     0x0040 ///< Do not buffer frames when possible
#define AVFMT_FLAG_CUSTOM_IO    0x0080 ///< The caller has supplied a custom AVIOContext, don't avio_close() it.
#define AVFMT_FLAG_DISCARD_CORRUPT  0x0100 ///< Discard frames marked corrupted
#define AVFMT_FLAG_FLUSH_PACKETS    0x0200 ///< Flush the AVIOContext every packet.
/**
 * 混流时，尽量避免将随机/不可控的数据写入输出中，包括随机IDs,实时时间戳/日期，混流器版本等等。
 *
 * 该标记主要用于测试
 */
#define AVFMT_FLAG_BITEXACT         0x0400
#define AVFMT_FLAG_MP4A_LATM    0x8000 ///< Enable RTP MP4A-LATM payload
#define AVFMT_FLAG_SORT_DTS    0x10000 ///< try to interleave outputted packets by dts (using this flag can slow demuxing down)
#define AVFMT_FLAG_PRIV_OPT    0x20000 ///< Enable use of private options by delaying codec open (this could be made default once all code is converted)
#if FF_API_LAVF_KEEPSIDE_FLAG
#define AVFMT_FLAG_KEEP_SIDE_DATA 0x40000 ///< Don't merge side data but keep it separate. Deprecated, will be the default.
#endif
#define AVFMT_FLAG_FAST_SEEK   0x80000 ///< Enable fast, but inaccurate seeks for some formats
#define AVFMT_FLAG_SHORTEST   0x100000 ///< Stop muxing when the shortest stream stops.
#define AVFMT_FLAG_AUTO_BSF   0x200000 ///< Wait for packet data before writing a header, and add bitstream filters as requested by the muxer

    /**
     * 从指定容器格式的输入中读取最大数据的大小。
     * 仅用于分流操作，用户可以在avformat_open_input()函数前设置。
     */
    int64_t probesize;

    /**
     * 从指定容器格式的输入中读取的最大数据时长（以AV_TIME_BASE为单位）。
     * 仅用于分流操作，在avformat_find_stream_info()调用前设置。为0时，让avformat自动选择。
     */
    int64_t max_analyze_duration;

    const uint8_t *key;
    int keylen;

    unsigned int nb_programs;
    AVProgram **programs;

    /**
     * 强制视频codec_id.
     * 分流操作: 用户设置。
     */
    enum AVCodecID video_codec_id;

    /**
     * 强制音频codec_id.
     * 分流操作: 用户设置。
     */
    enum AVCodecID audio_codec_id;

    /**
     * 强制字幕codec_id.
     * 分流操作: 用户设置。
     */
    enum AVCodecID subtitle_codec_id;

    /**
     * 每个索引使用的内存最大值（以字节为单位）。
     * 如果索引超出内存限制，项目会被丢弃以保持较小的内存占用。这回导致seeking较慢和不准确（取决于分流	  	   * 器）
     * 完全内存索引是强制性的分解器将忽略这一点。
     * - 混流操作: 不实用
     * - 分流操作: 由用户设置
     */
    unsigned int max_index_size;

    /**
     * 从设备获取的实时帧缓冲的最大内存大小（以字节为单位）
     */
    unsigned int max_picture_buffer;

    /**
     * AVChapter数组中的章节数量。
     * 混流时，章节信息通畅会写在文件头中，所以nb_chapters应该在写文件头之前被初始化。一些混流器（例如	 * mov、mkv)可以将章节写在预告中。为了在预告中撰写章节，在write_header调用时nb_chapters必须为0，	  * 并且在write_trailer被调用时为非0数。
     * - 混流操作: 用户设置
     * - 分流操作: libavformat设置
     */
    unsigned int nb_chapters;
    AVChapter **chapters;

    /**
     * 适用于整个文件的元数据。
     *
     * - 分流操作: libavformat在avformat_open_input()函数中设置。
     * - 混流操作: 调用者可以在avformat_write_header()函数调用前设置。
     *
     * 通过libavformat在函数avformat_free_context()中释放。
     */
    AVDictionary *metadata;

    /**
     * 从Unix纪元（1970年1月1日00:00）开始，以真实世界时间开始流的开始时间，以微秒为单位。 即，流在现实		* 世界被使用的pts=0时间。
     * - 混流操作: 在avformat_write_header()调用前被调用者设置。如果设置为0或AV_NOPTS_VALUE，则将	  * 		   使用当前时间(wall-time)。
     * - 分流操作: 由libavformat设置. 如果AV_NOPTS_VALUE未知，注意，一定数量的帧被获取后，该值可能变	  *            得已知。
     */
    int64_t start_time_realtime;

    /**
     * 用于确定avformat_find_stream_info（）中帧率的帧数。
     * 仅用于分流，在avformat_find_stream_info()调用前由调用者设置
     */
    int fps_probe_size;

    /**
     * 错误识别; 较高的值将检测到更多的错误，但可能会错误检测一些或多或少有效的部分作为错误。 在			 * avformat_open_input（）之前由调用方设置的仅用于解分流。
     */
    int error_recognition;

    /**
     * I/O层自定义中断回调函数。
     *
     * 分流操作: avformat_open_input()调用前由用户设置.
     * 混流操作: avformat_write_header()调用前由用户设置（主要用于AVFMT_NOFILE 格式）。如果用它来打	 * 开文件，该回调也会传递给avio_open2().
     */
    AVIOInterruptCB interrupt_callback;

    /**
     * 启用debug标志。
     */
    int debug;
#define FF_FDEBUG_TS        0x0001

    /**
     * Maximum buffering duration for interleaving.
     *
     * To ensure all the streams are interleaved correctly,
     * av_interleaved_write_frame() will wait until it has at least one packet
     * for each stream before actually writing any packets to the output file.
     * When some streams are "sparse" (i.e. there are large gaps between
     * successive packets), this can result in excessive buffering.
     *
     * This field specifies the maximum difference between the timestamps of the
     * first and the last packet in the muxing queue, above which libavformat
     * will output a packet regardless of whether it has queued a packet for all
     * the streams.
     *
     * Muxing only, set by the caller before avformat_write_header().
     */
    int64_t max_interleave_delta;

    /**
     * 允许非标准和实验性扩展
     * @see AVCodecContext.strict_std_compliance
     */
    int strict_std_compliance;

    /**
	 * 供用户检测文件上发生事件的标志。事件处理后，用户必须清除标志。AVFMT_EVENT_FLAG_ *的组合。
     */
    int event_flags;
#define AVFMT_EVENT_FLAG_METADATA_UPDATED 0x0001 ///< The call resulted in updated metadata.

    /**
     * 在等待第一个时间戳时要读取的最大数据包数。仅用于解码。
     */
    int max_ts_probe;

    /**
     * 避免混流过程中的负面时间戳。 AVFMT_AVOID_NEG_TS_ *常量中的任何值。 请注意，这只适用于使用			 * av_interleaved_write_frame。 （interleave_packet_per_dts正在使用中）
     * - 混流: 用户设置
     * - 分流: 不使用
     */
    int avoid_negative_ts;
#define AVFMT_AVOID_NEG_TS_AUTO             -1 ///< Enabled when required by target format
#define AVFMT_AVOID_NEG_TS_MAKE_NON_NEGATIVE 1 ///< Shift timestamps so they are non negative
#define AVFMT_AVOID_NEG_TS_MAKE_ZERO         2 ///< Shift timestamps so that they start at 0

    /**
     * 传输流id。
     * 这将被移入分流器的私有选项。 因此没有API / ABI兼容性
     */
    int ts_id;

    /**
     * 音频预加载以微秒为单位。 请注意，并非所有格式都支持此功能，如果在不支持的情况下使用它，则可能会发生	  * 不可预知的情况。
     * - 编码: 用户设置
     * - 解码: 不使用
     */
    int audio_preload;

    /**
     * 最大块时间（以微秒为单位）。 请注意，并非所有格式都支持此功能，如果在不支持的情况下使用它，则可能会	  * 发生不可预知的情况。
     * - 编码: 用户设置
     * - 解码: 不使用
     */
    int max_chunk_duration;

    /**
     * 最大块大小（以字节为单位）。注意，并非所有格式都支持此功能，如果在不支持的情况下使用它，可能会发生不	  * 可预知的情况。
     * - 编码: 用户设置
     * - 解码: 不使用
     */
    int max_chunk_size;

    /**
     *  强制使用wallclock时间戳作为pts / dts数据包在B帧存在的情况下存在未定义的结果。
     * - 编码: 不使用
     * - 解码: 用户设置
     */
    int use_wallclock_as_timestamps;

    /**
     * avio标志，用于强制使用AVIO_FLAG_DIRECT。
     * - 编码: 不使用
     * - 解码: 用户设置
     */
    int avio_flags;

    /**
     * 时长字段可以通过各种方式进行计算，并且可以使用此字段了解时长是如何计算的。
     * - 编码: 不使用
     * - 解码: 用户读取
     */
    enum AVDurationEstimationMethod duration_estimation_method;

    /**
     * 打开流时跳过初始字节
     * - 编码: 不使用
     * - 解码: 用户设置
     */
    int64_t skip_initial_bytes;

    /**
     * 正确的单个时间戳溢出
     * - 编码: 不使用
     * - 解码: 用户设置
     */
    unsigned int correct_ts_overflow;

    /**
     * 强制seeking到任意帧（即使没有关键帧）
     * - 编码: 不使用
     * - 解码: 用户设置
     */
    int seek2any;

    /**
     * 在每个数据包之后刷新I / O上下文。
     * - 编码: 用户设置
     * - 解码: 不使用
     */
    int flush_packets;

    /**
     * 格式探测分数。 最高分是AVPROBE_SCORE_MAX，当分流器探测格式时设置它。
     * - 编码: 不使用
     * - 解码: avformat设置，用户读取
     */
    int probe_score;

    /**
     * 要最大限度地读取以识别格式的字节数。
     * - 编码: 不使用
     * - 解码: 用户设置
     */
    int format_probesize;

    /**
     * ',' 分割的支持的解码器刘表，如果值为NULL，表示支持所有解码器。
     * - 编码: 不使用
     * - 解码: 用户设置
     */
    char *codec_whitelist;

    /**
     * ',' 分割的支持的分流器列表，如果值为NULL，所有分流器都支持。
     * - 编码: 不使用
     * - 解码: 用户设置
     */
    char *format_whitelist;

    /**
	  * libavformat内部使用的不透明字段。 不得以任何方式访问。
     */
    AVFormatInternal *internal;

    /**
     * IO重定位标志。
     * 当基础IO上下文读指针重新定位时，例如在执行基于字节的查找时，这由avformat设置。 分流器可以使用标志	  * 来检测这种变化。
     */
    int io_repositioned;

    /**
     * 强制视频解码器。强制数据解码器。这允许使用强制指定的解码器，即使有多个相同的codec_id.
     * 分流: 用户设置
     */
    AVCodec *video_codec;

    /**
     * 强制音频解码器。强制数据解码器。这允许使用强制指定的解码器，即使有多个相同的codec_id.
     * 分流: 用户设置
     */
    AVCodec *audio_codec;

    /**
     * 强制数据解码器。这允许使用强制指定的解码器，即使有多个相同的codec_id.
     * 分流: 用户设置
     */
    AVCodec *subtitle_codec;

    /**
     * 强制数据解码器。这允许使用强制指定的解码器，即使有多个相同的codec_id.
     * 分流: 用户设置
     */
    AVCodec *data_codec;

    /**
     * 在原数据头中，充当填充（分割）的字节数
     * 分流: 不使用
     * 混流: 用户可以通过av_format_set_metadata_header_padding设置.
     */
    int metadata_header_padding;

    /**
     * 用户数据，这是用户的私有数据空间。
     */
    void *opaque;

    /**
     * 设备用应用通讯的回调。
     */
    av_format_control_message control_message_cb;

    /**
     * 输出时移，以微妙为单位。
     * 混流: 用户设置
     */
    int64_t output_ts_offset;

    /**
     * 转储格式分隔符。可以是", " 或者 "\n" 等
     * - 混流: 用户设置
     * - 分流: 用户设置
     */
    uint8_t *dump_separator;

    /**
     * 强制数据codec_id.
     * 分流操作: 用户设置
     */
    enum AVCodecID data_codec_id;

#if FF_API_OLD_OPEN_CALLBACKS
    /**
     * Called to open further IO contexts when needed for demuxing.
     *
     * This can be set by the user application to perform security checks on
     * the URLs before opening them.
     * The function should behave like avio_open2(), AVFormatContext is provided
     * as contextual information and to reach AVFormatContext.opaque.
     *
     * If NULL then some simple checks are used together with avio_open2().
     *
     * Must not be accessed directly from outside avformat.
     * @See av_format_set_open_cb()
     *
     * Demuxing: Set by user.
     *
     * @deprecated Use io_open and io_close.
     */
    attribute_deprecated
    int (*open_cb)(struct AVFormatContext *s, AVIOContext **p, const char *url, int flags, const AVIOInterruptCB *int_cb, AVDictionary **options);
#endif

    /**
     * ',' 符号分割的支持协议列表separated list of allowed protocols.
     * - 编码: 不使用
     * - 解码: 用户设置
     */
    char *protocol_whitelist;

    /*
     * A callback for opening new IO streams.
     *
     * Whenever a muxer or a demuxer needs to open an IO stream (typically from
     * avformat_open_input() for demuxers, but for certain formats can happen at
     * other times as well), it will call this callback to obtain an IO context.
     *
     * @param s the format context
     * @param pb on success, the newly opened IO context should be returned here
     * @param url the url to open
     * @param flags a combination of AVIO_FLAG_*
     * @param options a dictionary of additional options, with the same
     *                semantics as in avio_open2()
     * @return 0 on success, a negative AVERROR code on failure
     *
     * @note Certain muxers and demuxers do nesting, i.e. they open one or more
     * additional internal format contexts. Thus the AVFormatContext pointer
     * passed to this callback may be different from the one facing the caller.
     * It will, however, have the same 'opaque' field.
     */
    int (*io_open)(struct AVFormatContext *s, AVIOContext **pb, const char *url,
                   int flags, AVDictionary **options);

    /**
     * 将AVFormateContext.io_open()打开流关闭的回调。
     */
    void (*io_close)(struct AVFormatContext *s, AVIOContext *pb);

    /**
     * ',' 符分割的不支持协议列表。
     * - 编码: 不使用
     * - 解码: 用户设置
     */
    char *protocol_blacklist;

    /**
     * 最大streams数量
     * - 编码: 不使用
     * - 解码: 用户设定
     */
    int max_streams;
} AVFormatContext;
```



