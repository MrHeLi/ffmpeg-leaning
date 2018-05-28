# FFmpeg:结构体分析—AVStream

本文使用FFmpeg版本为3.3.7，【[传送门](https://www.ffmpeg.org/download.html#releases)】

## AVStream简介

`AVStream`在FFmpeg使用过程中关于编解码至关重要的结构体之一，是对流（Stream）的封装和抽象，描述了视频、音频等流的编码格式等基本流信息。

该结构体在`avformat.h`中申明。

## 术语

如有看不懂的术语，请参考我的博客<https://blog.csdn.net/qq_25333681/article/details/80088126>

## 代码解读

```c
typedef struct AVStream {
    int index;    /**< 在AVFormatContext中的stream索引 */
    /**
     * 特定格式的stream id。
     * 解码: 由libavformat设定
     * 编码: 如果未设置，则由用户通过libavformat设置
     */
    int id;
#if FF_API_LAVF_AVCTX
    /**
     * @deprecated use the codecpar struct instead
     */
    attribute_deprecated
    AVCodecContext *codec;
#endif
    void *priv_data;

#if FF_API_LAVF_FRAC
    /**
     * @deprecated this field is unused
     */
    attribute_deprecated
    struct AVFrac pts;
#endif

    /**
     * 这是表示帧时间戳的基本时间单位（以秒为单位）。
     *
     * 解码: libavformat设置
     * 编码: 可以在avformat_write_header（）之前由调用者设置，以向混流器提供关于所需单位时间的提示。 	   * 在avformat_write_header（）中，混流器将用实际用于写入文件的时间戳（根据格式可能与用户提供的时间		* 戳相关或不相关）的单位时间覆盖该字段。
     */
    AVRational time_base;

    /**
     * 解码: 流显示序列中的第一帧pts时间，基于流时间（in stream time base.）。
     * 只有当你百分百确定该值就是真实的第一帧的pts时间，才可以设置它
     * 该值可能未定义(AV_NOPTS_VALUE).
     * @note The ASF header does NOT contain a correct start_time the ASF
     * 分流器禁止设置该值。
     */
    int64_t start_time;

    /**
     * 解码: 流时长，基于流时间（in stream time base.）
     *      如果一个源文件指定了比特率，而未指定流时长，该值将由比特率和文件大小估算。
     *
     * 编码: May be set by the caller before 用户可以在avformat_write_header()调用前设置，提示		 *      混流器估算时长
     */
    int64_t duration;

    int64_t nb_frames;                 ///< 表示该流的已知帧数，或者为0

    int disposition; /**< AV_DISPOSITION_* 推荐比特字段 */

    enum AVDiscard discard; ///< 选择那些数据包可以被丢掉而不用被分流器分流。

    /**
     * 采样率(如果未知，该值为0)
     * - 编码: 用户设置.
     * - 解码: libavformat设置.
     */
    AVRational sample_aspect_ratio;

    AVDictionary *metadata;//原数据信息

    /**
     * 平均帧率
     *
     * - 分流: 在创建流时或者才函数avformat_find_stream_info()函数中可能被设置。
     * - 混流: 可能在avformat_write_header()函数调用前被设置
     */
    AVRational avg_frame_rate;

    /**
     * 对于设置有AV_DISPOSITION_ATTACHED_PIC标志的流, 该数据包会包含该附加图片（专辑图片什么的）
     *
     * 解码: libavformat设置, 不能被用户修改。
     * 编码: 不使用
     */
    AVPacket attached_pic;

    /**
     * An array of side data that applies to the whole stream (i.e. the
     * container does not allow it to change between packets).
     *
     * There may be no overlap between the side data in this array and side data
     * in the packets. I.e. a given side data is either exported by the muxer
     * (demuxing) / set by the caller (muxing) in this array, then it never
     * appears in the packets, or the side data is exported / sent through
     * the packets (always in the first packet where the value becomes known or
     * changes), then it does not appear in this array.
     *
     * - demuxing: Set by libavformat when the stream is created.
     * - muxing: May be set by the caller before avformat_write_header().
     *
     * Freed by libavformat in avformat_free_context().
     *
     * @see av_format_inject_global_side_data()
     */
    AVPacketSideData *side_data;
    /**
     * The number of elements in the AVStream.side_data array.
     */
    int            nb_side_data;

    /**
     * 供用户检测流上发生的时间标志。 事件处理后，用户必须清除标志。 AVSTREAM_EVENT_FLAG_ *的组合。
     */
    int event_flags;
#define AVSTREAM_EVENT_FLAG_METADATA_UPDATED 0x0001 ///< The call resulted in updated metadata.

    /*****************************************************************
  	 *该行下面的所有字段不是公共API的一部分。 它们不能在libavformat之外使用，并且可以随意更改和删除。 	   *内部提示：请注意，物理删除这些字段将会破坏ABI。 用空字段替换已删除的字段，并向AVStreamInternal添	   *加新字段。
     *****************************************************************
     */

    /**
     * avformat_find_stream_info()函数使用的内部流信息
     */
#define MAX_STD_TIMEBASES (30*12+30+3+6)
    struct {
        int64_t last_dts;
        int64_t duration_gcd;
        int duration_count;
        int64_t rfps_duration_sum;
        double (*duration_error)[2][MAX_STD_TIMEBASES];
        int64_t codec_info_duration;
        int64_t codec_info_duration_fields;

        /**
         * 0  -> 解码器还未被检索到
         * >0 -> 解码器已被找到
         * <0 -> decoder with codec_id == -found_decoder has not been found
         */
        int found_decoder;

        int64_t last_duration;

        /**
         * 这些字段用于估算平均帧率
         */
        int64_t fps_first_dts;
        int     fps_first_dts_idx;
        int64_t fps_last_dts;
        int     fps_last_dts_idx;

    } *info;

    int pts_wrap_bits; /**< number of bits in pts (used for wrapping control) */

    // 时间戳生成支持:
    /**
     * 最后同步点的时间戳。
     *
     * 当AVCodecParserContext.dts_sync_point >= 0 时初始化，并且接受一个当前容器的DTS。否则，		 * AV_NOPTS_VALUE使用默认值
     */
    int64_t first_dts;
    int64_t cur_dts;
    int64_t last_IP_pts;
    int last_IP_duration;

    /**
     * 编解码器探测缓存的数据包数量
     */
    int probe_packets;

    /**
     * avformat_find_stream_info()调用期间，已经被分流的帧数
     */
    int codec_info_nb_frames;

    /* av_read_frame() 支持 */
    enum AVStreamParseType need_parsing;
    struct AVCodecParserContext *parser;

    /**
     * 正在混流操作的流在数据包缓冲中的最后一个数据包
     */
    struct AVPacketList *last_in_packet_buffer;
    AVProbeData probe_data;
#define MAX_REORDER_DELAY 16
    int64_t pts_buffer[MAX_REORDER_DELAY+1];

    AVIndexEntry *index_entries; /**< 只有当格式不支持本地seeking时使用*/
    int nb_index_entries;
    unsigned int index_entries_allocated_size;

    /**
     * 流的真实基准帧率.
     * 这是所有时间戳可以准确表示的最低帧速率（它是流中所有帧速率的最小公倍数）。 请注意，这个值只是一个猜	  * 测！ 例如，如果时基为1/90000，并且所有帧都具有约3600或1800个计时器滴答，则r_frame_rate将为		 * 50/1。
     *
     * avformat以外的代码应该使用此字段访问:
     * av_stream_get/set_r_frame_rate(stream)
     */
    AVRational r_frame_rate;

    /**
     * 流标志符
     * 这是MPEG-TS流标识符 +1
     * 0 意味着未知
     */
    int stream_identifier;

    int64_t interleaver_chunk_size;
    int64_t interleaver_chunk_duration;

    /**
     * 流探测状态
     * -1   -> 探测完毕
     *  0   -> 没有探测请求
     * rest -> 以request_probe作为接受的最低分数执行探测。
     * 不是公共API的一部分
     */
    int request_probe;
    /**
     * 表示直到下一个关键帧的所有内容都应该丢弃。
     */
    int skip_to_keyframe;

    /**
     * 在下一个数据包解码的帧开始时跳过的采样数。
     */
    int skip_samples;

    /**
     * 如果不是0，则应从流的开始位置跳过的样本数量（样本从pts == 0的包中移除，这也假定负时间戳不会发		 * 生）。 旨在用于具有ad-hoc无间断音频支持的mp3等格式。
     */
    int64_t start_skip_samples;

    /**
     * 如果不是0，应该从流中丢弃的第一个音频采样。 这是由设计丢弃的（需要全球采样计数），但无法避免由设计	  * 格式（如带有ad-hoc无间隙音频支持的mp3）破坏。
     */
    int64_t first_discard_sample;

    /**
     * 在first_discard_sample之后打算丢弃的最后一个样本之后的样本。 仅适用于框架边界。 用于防止早期	 * EOF，如果无间隙信息被破坏（考虑连接的MP3）。
     */
    int64_t last_discard_sample;

    /**
     * 在libavformat内部使用的内部解码帧的数量不会访问其生存期，这与信息不同，因此它不在该结构中。
     */
    int nb_decoded_frames;

    /**
     * 时间戳偏移添加到混流之前的时间戳
     * 非公共API
     */
    int64_t mux_ts_offset;

    /**
     * 内部数据检查时间戳的包装
     */
    int64_t pts_wrap_reference;

    /**
     * Options for behavior, when a wrap is detected.
     *
     * Defined by AV_PTS_WRAP_ values.
     *
     * If correction is enabled, there are two possibilities:
     * If the first time stamp is near the wrap point, the wrap offset
     * will be subtracted, which will create negative time stamps.
     * Otherwise the offset will be added.
     */
    int pts_wrap_behavior;

    /**
     * 禁止执行两次update_initial_durations（）的内部数据
     */
    int update_initial_durations_done;

    /**
     * 内部数据，用于从pts生成dts
     */
    int64_t pts_reorder_error[MAX_REORDER_DELAY+1];
    uint8_t pts_reorder_error_count[MAX_REORDER_DELAY+1];

    /**
     * 内部数据，用于分析DTS和检测错误的MPEG流
     */
    int64_t last_dts_for_order_check;
    uint8_t dts_ordered;
    uint8_t dts_misordered;

    /**
     * Internal data to inject global side data
     */
    int inject_global_side_data;

    /*****************************************************************
     * 该行上方的所有字段都不是公共API的一部分。 下面的字段是公共API和ABI的一部分。
     *****************************************************************
     */

    /**
     * 包含键和值的一系列字符串，用于描述推荐的编码器配置。
     * 系列以 ','分割.
     * 键和值由'='分割.
     */
    char *recommended_encoder_configuration;

    /**
     * 显示宽高比（如果未知，则为0）
     * - 编码: 不使用
     * - 解码: libavformat设置， 用于在内部计算显示宽高比。
     */
    AVRational display_aspect_ratio;

    struct FFFrac *priv_pts;

    /**
     * libavformat内部使用的不透明字段。 不得以任何方式访问。
     */
    AVStreamInternal *internal;

    /*
     * 与此流关联的编解码器参数。 分别在avformat_new_stream（）和avformat_free_context（）中由		 * libavformat分配和释放。
     *
     * - 分流: 由libavformat在流创建时填充或在avformat_find_stream_info（）赋值。
     * - 混流: 在avformat_write_header（）之前由调用者填充
     */
    AVCodecParameters *codecpar;
} AVStream;
```

