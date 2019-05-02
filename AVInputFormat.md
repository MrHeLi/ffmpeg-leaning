# FFmpeg:AVInputFormat结构体解析

## AVInputFormat

`AVInputFormat`是解复用器（解封装）作用时读取媒体文件并将其拆分为数据块（数据包）。每个数据包，包含一个或者多个编码帧

比较重要的字段有：

* long_name：格式的长名称（相对于短名称而言，更易于阅读）。
* mime_type：mime类型，它用于在探测时检查匹配的mime类型。
* next：用于链接下一个AVInputFormat。
* (*read_probe)：判断给定文件是否有可能被解析为此格式。 提供的缓冲区保证为AVPROBE_PADDING_SIZE字节大，因此除非您需要更多，否则无需检查。
* (*read_header)：读取格式头，并初始化AVFormatContext结构体。
* (*read_packet)：读取一个packet并存入pkt指针中。

## 结构定义及成员解读

```c
typedef struct AVInputFormat {
    const char *name; // 输入格式的短名称
    const char *long_name; // 格式的长名称（相对于短名称而言，更易于阅读）
    /**
     * Can use flags: AVFMT_NOFILE, AVFMT_NEEDNUMBER, AVFMT_SHOW_IDS,
     * AVFMT_GENERIC_INDEX, AVFMT_TS_DISCONT, AVFMT_NOBINSEARCH,
     * AVFMT_NOGENSEARCH, AVFMT_NO_BYTE_SEEK, AVFMT_SEEK_TO_PTS.
     */
    int flags;
    const char *extensions; // 如果定义了扩展，就不会进行格式探测。但因为该功能目前支持不够，不推荐使用
    const struct AVCodecTag * const *codec_tag; // 见名知意
    const AVClass *priv_class; ///< AVClass for the private context
    const char *mime_type; // mime类型，它用于在探测时检查匹配的mime类型。
		/* 此行下方的任何字段都不是公共API的一部分。 它们不能在libavformat之外使用，可以随意更改和删除。
		 * 应在上方添加新的公共字段。*/
    struct AVInputFormat *next; // 用于链接下一个AVInputFormat
    int raw_codec_id; // 原始demuxers将它们的解码器id保存在这里。
    int priv_data_size; // 私有数据大小，可以用于确定需要分配多大的内存来容纳下这些数据。
		/**
		 * 判断给定文件是否有可能被解析为此格式。 提供的缓冲区保证为AVPROBE_PADDING_SIZE字节大，因此除非您需		 * 要更多，否则无需检查。
		 */
    int (*read_probe)(AVProbeData *);
    /**
     * 读取格式头，并初始化AVFormatContext结构体
     * @return 0 表示操作成功
     */
    int (*read_header)(struct AVFormatContext *);
    /**
     * 读取一个packet并存入pkt指针中。pts和flags会被同时设置。
     * @return 0 表示操作成功, < 0 发生异常
     *         当返回异常时，pkt可定没有allocated或者在函数返回之前被释放了。
     */
    int (*read_packet)(struct AVFormatContext *, AVPacket *pkt);
     // 关闭流，AVFormatContext和AVStreams并不会被这个函数释放。
    int (*read_close)(struct AVFormatContext *);
    /**
     * 在stream_index的流中，使用一个给定的timestamp,seek到附近帧。
     * @param stream_index 不能为-1
     * @param flags 如果没有完全匹配，决定向前还是向后匹配。
     * @return >= 0 成功
     */
    int (*read_seek)(struct AVFormatContext *,
                     int stream_index, int64_t timestamp, int flags);
    // 获取stream[stream_index]的下一个时间戳，如果发生异常返回AV_NOPTS_VALUE
    int64_t (*read_timestamp)(struct AVFormatContext *s, int stream_index,
                              int64_t *pos, int64_t pos_limit);
    // 开始或者恢复播放，只有在播放rtsp格式的网络格式才有意义。
    int (*read_play)(struct AVFormatContext *);
    int (*read_pause)(struct AVFormatContext *);// 暂停播放，只有在播放rtsp格式的网络格式才有意义。
    /**
     * 快进到指定的时间戳
     * @param stream_index 需要快进操作的流
     * @param ts 需要快进到的地方
     * @param min_ts max_ts seek的区间，ts需要在这个范围中。
     */
    int (*read_seek2)(struct AVFormatContext *s, int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags);
    // 返回设备列表和其属性
    int (*get_device_list)(struct AVFormatContext *s, struct AVDeviceInfoList *device_list);
    // 初始化设备能力子模块
    int (*create_device_capabilities)(struct AVFormatContext *s, struct AVDeviceCapabilitiesQuery *caps);
    // 释放设备能力子模块
    int (*free_device_capabilities)(struct AVFormatContext *s, struct AVDeviceCapabilitiesQuery *caps);
} AVInputFormat;
```