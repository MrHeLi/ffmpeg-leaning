# FFmpeg：AVIOContext结构体分析

分享一个FFmpeg的文档地址，如果本文有什么没讲清楚的，建议直接去查找：[传送门](https://www.ffmpeg.org/doxygen/4.1/index.html)

## AVIOContext

这个结构体，是FFmpeg中有关io操作的顶层结构体，是avio的核心。FFmpeg支持打开本地文件路径和流媒体协议的URL。

虽然`AVIOContext`时avio操作的核心，但`AVIOContext`中的所有函数指针都不应该直接调用，它们只应在实现自定义I / O时由客户端应用程序设置。 通常这些设置为`avio_alloc_context()`中指定的函数指针（下一节中的`read_packet函数指针`）。

`AVIOContext`的相关调用都是在`AVFormatContext`中间接触发的。

比较重要的字段有：

* (*read_packet)：读取音视频数据的函数。
* (*write_packet)：写入音视频数据的函数。
* (*read_pause)：暂停或恢复网络流媒体协议的播放 。

## 初始化与清理

典型的使用方式是：

```c
size_t buffer_size, avio_ctx_buffer_size = 4096;
struct buffer_data bd = { 0 };
AVFormatContext *fmt_ctx = avformat_alloc_context();
unsigned char *avio_ctx_buffer = (unsigned char *) av_malloc(avio_ctx_buffer_size);
AVIOContext *avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 0, &bd, &read_packet, NULL, NULL);
fmt_ctx->pb = avio_ctx;
// 清理
av_freep(&avio_ctx->buffer);
av_freep(&avio_ctx);
```

先通过文件的buffer大小，申请一段内存。然后使用`avio_alloc_context`为`AVIOContext`分配内存，申请时，注册内存数据读取的回掉接口`read_packet`，最后将申请到的`AVIOContext`句柄挂载到`AVFormatContext`的`pb`字段，然后就可以通过`AVFormatContext`对文件进行操作了。

## 结构定义及成员解读

```c
typedef struct AVIOContext {
  	// 私有选项（options）的类指针。如果AVIOContext是通过avio_open2()创建，av_class已设置并将选项传递给协议。如果是手动allocated的，av_class可能是被调用者设置。这个字段一定不能为空
    const AVClass *av_class;
    /*
     * 下面的图，显示了在读和写缓冲区时buf_ptr，buf_end，buf_size和pos之间的关系。
     **********************************************************************************
     *                                   READING
     **********************************************************************************
     *
     *                            |              buffer_size              |
     *                            |---------------------------------------|
     *                            |                                       |
     *
     *                         buffer          buf_ptr       buf_end
     *                            +---------------+-----------------------+
     *                            |/ / / / / / / /|/ / / / / / /|         |
     *  read buffer:              |/ / consumed / | to be read /|         |
     *                            |/ / / / / / / /|/ / / / / / /|         |
     *                            +---------------+-----------------------+
     *
     *                                                         pos
     *              +-------------------------------------------+-----------------+
     *  input file: |                                           |                 |
     *              +-------------------------------------------+-----------------+
     *
     *
     **********************************************************************************
     *                                   WRITING
     **********************************************************************************
     *
     *                                          |          buffer_size          |
     *                                          |-------------------------------|
     *                                          |                               |
     *
     *                                       buffer              buf_ptr     buf_end
     *                                          +-------------------+-----------+
     *                                          |/ / / / / / / / / /|           |
     *  write buffer:                           | / to be flushed / |           |
     *                                          |/ / / / / / / / / /|           |
     *                                          +-------------------+-----------+
     *
     *                                         pos
     *               +--------------------------+-----------------------------------+
     *  output file: |                          |                                   |
     *               +--------------------------+-----------------------------------+
     *
     */
    unsigned char *buffer;  // buffer起始地址
    int buffer_size;        // 可以读取或者写入的最大的buffer size
    unsigned char *buf_ptr; // 当前正在读或写操作的buffer地址
    unsigned char *buf_end; // 数据结束的buffer地址，如果读取函数返回的数据小于请求数据，buf_end可能小于buffer + buffer_size
    void *opaque;  // 一个私有指针，传递给read / write / seek / 等函数
    int (*read_packet)(void *opaque, uint8_t *buf, int buf_size); // 读取音视频数据的函数。
    int (*write_packet)(void *opaque, uint8_t *buf, int buf_size); // 写入音视频数据的函数
    int64_t (*seek)(void *opaque, int64_t offset, int whence);
    int64_t pos; // 当前buffer在文件中的位置
    int must_flush; // 如果下一个seek应该刷新，则为true
    int eof_reached; // 如果到达eof(end of file 文件尾)，则为true
    int write_flag; // 如果开放写，则为true
    int (*read_pause)(void *opaque, int pause); // 暂停或恢复网络流媒体协议的播放
    int64_t (*read_seek)(void *opaque, int stream_index,
                         int64_t timestamp, int flags); // 快进到指定timestamp
    int seekable; // 如果为0，表示不可seek操作。其它值查看AVIO_SEEKABLE_XXX
    int64_t maxsize; // max filesize，用于限制分配空间大小
    int direct; // avio_seek是否直接调用底层的seek功能。
    int64_t bytes_read; // 字节读取统计数据
    int seek_count; // seek计数
    int writeout_count; // 写入次数统计
    int orig_buffer_size; // 原始buffer大小
    const char *protocol_whitelist; // 允许协议白名单，以','分隔
    const char *protocol_blacklist; // 不允许的协议黑名单，以','分隔
		// 用于替换write_packet的回调函数。
    int (*write_data_type)(void *opaque, uint8_t *buf, int buf_size,
                           enum AVIODataMarkerType type, int64_t time);
} AVIOContext;
```