# SDL_UpdateTexture+ffmpeg播放YUV数据程序异常崩溃

## 异常代码：

```c++
#include <iostream>
#include <map>
#include <pthread.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <SDL2/SDL.h>
}
using namespace std;

static AVFormatContext *fmt_ctx = NULL;
static AVCodec *videoCodec = NULL, *audioCodec = NULL;
static AVCodecContext *audio_codec_ctx = NULL, *codec_ctx_video = NULL;
static int mWidth, mHeight;
static int index_video_stream = -1, index_audio_stream = -1;
static AVStream *video_stream = NULL, *audio_stream = NULL;
//static map<AVPixelFormat, SDL_PixelFormat> pixFormat_ff2sdl;
int pixelFormatFFmpeg = -1, pixelFormatSDL = -1;

//SDL
SDL_Renderer *renderer;
SDL_Texture *texture;
SDL_Window *sdl_window;

int init_sdl();

int free_sdl();

int main() {
    cout << "FFmpeg, 解封装" << endl;
    av_register_all();
    avformat_network_init();
    avcodec_register_all();
    string source_url = "http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4";

    int stream_count;
    if (avformat_open_input(&fmt_ctx, source_url.c_str(), NULL, NULL)) { // 打开媒体源，构建AVFormatContext
        cerr << "could not open source file:" << source_url << endl;
        exit(1);
    }

    if ((stream_count = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
        cerr << "could find stream information" << endl;
        exit(1);
    }

    // 找到所有流,初始化一些基本参数
    index_video_stream = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    mWidth = fmt_ctx->streams[index_video_stream]->codecpar->width;
    mHeight = fmt_ctx->streams[index_video_stream]->codecpar->height;
    index_audio_stream = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    cout << "index_video:" << index_video_stream << " index_audio:" << index_audio_stream
         << " width: " << mWidth << " height:" << mHeight << endl;
    // 找到并打开解码器
    videoCodec = avcodec_find_decoder(fmt_ctx->streams[index_video_stream]->codecpar->codec_id);
    codec_ctx_video = avcodec_alloc_context3(videoCodec); 
    avcodec_parameters_to_context(codec_ctx_video, fmt_ctx->streams[index_video_stream]->codecpar);
    codec_ctx_video->thread_count = 8; 
    cout << "thread_count = " << codec_ctx_video->thread_count << endl;
    int openCodecError = 0;
    if ((openCodecError = avcodec_open2(codec_ctx_video, videoCodec, NULL)) < 0) {
        cout << "cannot open specified audio codec" << endl;
    }

    init_sdl();

    AVPacket *avPacket = av_packet_alloc();
    AVFrame *avFrame = av_frame_alloc();
    int count = 0;

    while (true) {
        int result = av_read_frame(fmt_ctx, avPacket);
        if (result < 0) {
            cout << "end of file" << endl;
            break;
        }
        if (avPacket->stream_index != index_video_stream) {
            av_packet_unref(avPacket);
            continue;
        }
 
        result = avcodec_send_packet(codec_ctx_video, avPacket);
        av_packet_unref(avPacket);
        if (result != 0) {
            cout << "av_packet_unref failed" << endl;
            continue;
        }
        while (true) {
            result = avcodec_receive_frame(codec_ctx_video, avFrame);
            cout << "avFrame pts : " << avFrame->pts << " color format:" << avFrame->format << endl;
            result = SDL_UpdateTexture(texture, NULL, avFrame->data[0], avFrame->linesize[0]); // 这段代码调用出现崩溃。
            if (result != 0) {
                cout << "SDL_UpdateTexture failed" << endl;
                continue;
            }
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
            SDL_Delay(40);
        }
    }
    return 0;
}
```

这段代码实现的是：通过FFmpeg解码一个网络流，然后将解码后的YUV数据通过SDL2显示的功能。

问题出在SDL更新纹理的操作上：

```c++
result = SDL_UpdateTexture(texture, NULL, avFrame->data[0], avFrame->linesize[0]); // 这段代码调用出现崩溃。
```

通过调试，如果将这段调用的第二第三个参数传空，除了无法更新纹理数据外，没有crash的问题。

又根据crash日志中有：`MALLOC_LARGE`标志，所以，猜想是SDL_UpdateTexture函数在拷贝纹理时，错误的分配了空间。

## 解决

将SDL_UpdateTexture函数，换成另外一个更新纹理的函数，程序便能正常运行，也能播放了。另一个函数调用方式如下：

```c++
result = SDL_UpdateYUVTexture(texture, NULL, avFrame->data[0], avFrame->linesize[0],
                              avFrame->data[1], avFrame->linesize[1], avFrame->data[2],
                              avFrame->linesize[2]);
```

## Crash 的完整log

```shell
Process:               PlayFFmpeg [8424]
Path:                  /Users/USER/*/PlayFFmpeg
Identifier:            PlayFFmpeg
Version:               0
Code Type:             X86-64 (Native)
Parent Process:        clion [406]
Responsible:           PlayFFmpeg [8424]
User ID:               501

Date/Time:             2019-05-14 12:59:53.851 +0800
OS Version:            Mac OS X 10.13.6 (17G5019)
Report Version:        12
Bridge OS Version:     3.0 (14Y664)
Anonymous UUID:        06530B17-FCE0-D408-B7A2-45F2A6BFEB3B

Sleep/Wake UUID:       8F6EE886-4C0F-448F-8634-BD327424398E

Time Awake Since Boot: 92000 seconds
Time Since Wake:       430 seconds

System Integrity Protection: enabled

Crashed Thread:        0  Dispatch queue: com.apple.main-thread

Exception Type:        EXC_BAD_ACCESS (SIGSEGV)
Exception Codes:       KERN_INVALID_ADDRESS at 0x0000000113e3d000
Exception Note:        EXC_CORPSE_NOTIFY

Termination Signal:    Segmentation fault: 11
Termination Reason:    Namespace SIGNAL, Code 0xb
Terminating Process:   exc handler [0]

VM Regions Near 0x113e3d000:
    MALLOC_LARGE           00000001135cb000-0000000113e3d000 [ 8648K] rw-/rwx SM=PRV  
--> 
    OpenGL GLSL            0000000113e45000-0000000113e65000 [  128K] r--/rwx SM=PRV  

Thread 0 Crashed:: Dispatch queue: com.apple.main-thread
0   libsystem_platform.dylib      	0x00007fff6ba66f49 _platform_memmove$VARIANT$Haswell + 41
1   libGLImage.dylib              	0x00007fff4d1ccd53 glgProcessPixelsWithProcessor + 2307
2   com.apple.AMDRadeonX4000GLDriver	0x00000001134b28f6 glrATIModifyTexSubImageCPU + 1399
3   com.apple.AMDRadeonX4000GLDriver	0x00000001134ccc5b glrWriteTextureData + 552
4   GLEngine                      	0x00007fff4dd4f130 glTexSubImage2D_Exec + 1567
5   libGL.dylib                   	0x00007fff4d1bc74f glTexSubImage2D + 59
6   libSDL2-2.0.0.dylib           	0x000000010ad0b230 GL_UpdateTexture + 424
7   libSDL2-2.0.0.dylib           	0x000000010ad05d3f SDL_UpdateTexture_REAL + 594
8   PlayFFmpeg                    	0x00000001090678f3 main + 2851 (main.cpp:111)
9   libdyld.dylib                 	0x00007fff6b755015 start + 1

Thread 1:
0   libsystem_kernel.dylib        	0x00007fff6b8a5a16 __psynch_cvwait + 10
1   libsystem_pthread.dylib       	0x00007fff6ba6e589 _pthread_cond_wait + 732
2   libavcodec.58.dylib           	0x000000010948e09b 0x10906e000 + 4325531
3   ???                           	0x0000206000000000 0 + 35596688949248

Thread 2:
0   libsystem_kernel.dylib        	0x00007fff6b8a5a16 __psynch_cvwait + 10
1   libsystem_pthread.dylib       	0x00007fff6ba6e589 _pthread_cond_wait + 732
2   libavcodec.58.dylib           	0x000000010948e09b 0x10906e000 + 4325531
3   ???                           	0x0000206000000000 0 + 35596688949248

Thread 3:
0   libsystem_platform.dylib      	0x00007fff6ba67c69 _platform_bzero$VARIANT$Haswell + 41
1   libavutil.56.dylib            	0x000000010ae14f46 av_buffer_allocz + 30

Thread 4:
0   libsystem_kernel.dylib        	0x00007fff6b8a5a16 __psynch_cvwait + 10
1   libsystem_pthread.dylib       	0x00007fff6ba6e589 _pthread_cond_wait + 732
2   libavcodec.58.dylib           	0x000000010948e09b 0x10906e000 + 4325531
3   ???                           	0x0000206000000000 0 + 35596688949248

Thread 5:
0   libsystem_kernel.dylib        	0x00007fff6b8a5a16 __psynch_cvwait + 10
1   libsystem_pthread.dylib       	0x00007fff6ba6e589 _pthread_cond_wait + 732
2   libavcodec.58.dylib           	0x000000010948e09b 0x10906e000 + 4325531
3   ???                           	0x0000206000000000 0 + 35596688949248

Thread 6:
0   libsystem_kernel.dylib        	0x00007fff6b8a5a16 __psynch_cvwait + 10
1   libsystem_pthread.dylib       	0x00007fff6ba6e589 _pthread_cond_wait + 732
2   libavcodec.58.dylib           	0x000000010948e09b 0x10906e000 + 4325531
3   ???                           	0x0000206000000000 0 + 35596688949248

Thread 7:
0   libsystem_kernel.dylib        	0x00007fff6b8a5a16 __psynch_cvwait + 10
1   libsystem_pthread.dylib       	0x00007fff6ba6e589 _pthread_cond_wait + 732
2   libavcodec.58.dylib           	0x000000010948e09b 0x10906e000 + 4325531
3   ???                           	0x0000206000000000 0 + 35596688949248

Thread 8:
0   libsystem_kernel.dylib        	0x00007fff6b8a5a16 __psynch_cvwait + 10
1   libsystem_pthread.dylib       	0x00007fff6ba6e589 _pthread_cond_wait + 732
2   libavcodec.58.dylib           	0x000000010948e09b 0x10906e000 + 4325531
3   ???                           	0x0000206000000000 0 + 35596688949248

Thread 9:: SDLTimer
0   libsystem_kernel.dylib        	0x00007fff6b8a5a16 __psynch_cvwait + 10
1   libsystem_pthread.dylib       	0x00007fff6ba6e589 _pthread_cond_wait + 732
2   libSDL2-2.0.0.dylib           	0x000000010ad8904e SDL_CondWaitTimeout_REAL + 144
3   libSDL2-2.0.0.dylib           	0x000000010ad88cad SDL_SemWaitTimeout_REAL + 75
4   libSDL2-2.0.0.dylib           	0x000000010ad28e9c SDL_TimerThread + 350
5   libSDL2-2.0.0.dylib           	0x000000010ad28929 SDL_RunThread + 60
6   libSDL2-2.0.0.dylib           	0x000000010ad88a1b RunThread + 9
7   libsystem_pthread.dylib       	0x00007fff6ba6d661 _pthread_body + 340
8   libsystem_pthread.dylib       	0x00007fff6ba6d50d _pthread_start + 377
9   libsystem_pthread.dylib       	0x00007fff6ba6cbf9 thread_start + 13

Thread 10:
0   libsystem_kernel.dylib        	0x00007fff6b8a628a __workq_kernreturn + 10
1   libsystem_pthread.dylib       	0x00007fff6ba6d20e _pthread_wqthread + 1552
2   libsystem_pthread.dylib       	0x00007fff6ba6cbe9 start_wqthread + 13

Thread 11:
0   libsystem_kernel.dylib        	0x00007fff6b8a628a __workq_kernreturn + 10
1   libsystem_pthread.dylib       	0x00007fff6ba6d009 _pthread_wqthread + 1035
2   libsystem_pthread.dylib       	0x00007fff6ba6cbe9 start_wqthread + 13

Thread 12:
0   libsystem_kernel.dylib        	0x00007fff6b8a628a __workq_kernreturn + 10
1   libsystem_pthread.dylib       	0x00007fff6ba6d20e _pthread_wqthread + 1552
2   libsystem_pthread.dylib       	0x00007fff6ba6cbe9 start_wqthread + 13

Thread 13:
0   libsystem_kernel.dylib        	0x00007fff6b89c25e semaphore_timedwait_trap + 10
1   libdispatch.dylib             	0x00007fff6b72b94f _dispatch_sema4_timedwait + 72
2   libdispatch.dylib             	0x00007fff6b72394e _dispatch_semaphore_wait_slow + 58
3   libdispatch.dylib             	0x00007fff6b71ee53 _dispatch_worker_thread + 251
4   libsystem_pthread.dylib       	0x00007fff6ba6d661 _pthread_body + 340
5   libsystem_pthread.dylib       	0x00007fff6ba6d50d _pthread_start + 377
6   libsystem_pthread.dylib       	0x00007fff6ba6cbf9 thread_start + 13

Thread 14:
0   libsystem_kernel.dylib        	0x00007fff6b8a628a __workq_kernreturn + 10
1   libsystem_pthread.dylib       	0x00007fff6ba6d20e _pthread_wqthread + 1552
2   libsystem_pthread.dylib       	0x00007fff6ba6cbe9 start_wqthread + 13

Thread 15:
0   libsystem_kernel.dylib        	0x00007fff6b8a628a __workq_kernreturn + 10
1   libsystem_pthread.dylib       	0x00007fff6ba6d20e _pthread_wqthread + 1552
2   libsystem_pthread.dylib       	0x00007fff6ba6cbe9 start_wqthread + 13

Thread 16:
0   libsystem_kernel.dylib        	0x00007fff6b8a628a __workq_kernreturn + 10
1   libsystem_pthread.dylib       	0x00007fff6ba6d009 _pthread_wqthread + 1035
2   libsystem_pthread.dylib       	0x00007fff6ba6cbe9 start_wqthread + 13

Thread 0 crashed with X86 Thread State (64-bit):
  rax: 0x0000000110b53000  rbx: 0x00007ffee6b9b1b0  rcx: 0x0000000000004500  rdx: 0x000000000000e100
  rdi: 0x0000000110b5cc00  rsi: 0x0000000113e3d000  rbp: 0x00007ffee6b9b070  rsp: 0x00007ffee6b9b070
   r8: 0x00007fff4d1cdadd   r9: 0x0000000000000000  r10: 0x000000000000a000  r11: 0xfffffffffcd1fc00
  r12: 0x0000000000000140  r13: 0x0000000000000000  r14: 0x00007ffee6b9b1c4  r15: 0x0000000000000002
  rip: 0x00007fff6ba66f49  rfl: 0x0000000000010206  cr2: 0x0000000113e3d000
  
Logical CPU:     0
Error Code:      0x00000004
Trap Number:     14


Binary Images:
       0x109064000 -        0x109068fff +PlayFFmpeg (0) <C9120D12-7D1C-3BB0-8451-22A6E462929D> /Users/USER/*/PlayFFmpeg
       0x10906e000 -        0x10a0fdfa7 +libavcodec.58.dylib (0) <51938E5C-D0DA-32AF-B5E4-61555C741119> /usr/local/opt/ffmpeg/lib/libavcodec.58.dylib
       0x10a8c0000 -        0x10a8d5ff7 +libavdevice.58.dylib (0) <19B63CC2-1824-37A9-A3F5-88C62E86F497> /usr/local/opt/ffmpeg/lib/libavdevice.58.dylib
       0x10a8e2000 -        0x10aa93fe7 +libavfilter.7.dylib (0) <CC9936B3-8E85-31E0-ADD6-CA69D609A79F> /usr/local/opt/ffmpeg/lib/libavfilter.7.dylib
       0x10ab17000 -        0x10ac98ff7 +libavformat.58.dylib (0) <52016529-99A9-35B7-8B4B-B9529468C749> /usr/local/opt/ffmpeg/lib/libavformat.58.dylib
       0x10acde000 -        0x10adb3ffb +libSDL2-2.0.0.dylib (0) <7D108060-C5B4-3F7C-91D8-3480EA57C1BF> /usr/local/opt/sdl2/lib/libSDL2-2.0.0.dylib
       0x10adf1000 -        0x10ae08fff +libswresample.3.dylib (0) <BE6B58B4-12FD-3AAA-8754-DBDDB98A74EB> /usr/local/Cellar/ffmpeg/4.1.3_1/lib/libswresample.3.dylib
       0x10ae0f000 -        0x10ae4efe7 +libavutil.56.dylib (0) <F51E959E-EC10-3DE3-BAD8-2358A62F29B0> /usr/local/Cellar/ffmpeg/4.1.3_1/lib/libavutil.56.dylib
       0x10ae6a000 -        0x10ae85ff7 +liblzma.5.dylib (0) <423B98CF-7AF0-325D-AB6A-3F44B56B90C2> /usr/local/opt/xz/lib/liblzma.5.dylib
       0x10ae8e000 -        0x10ae9fff7 +libopencore-amrwb.0.dylib (0) <C06D2282-9D81-3A6C-BF06-E6C6B748EBF3> /usr/local/opt/opencore-amr/lib/libopencore-amrwb.0.dylib
       0x10aea9000 -        0x10aeadfef +libsnappy.1.dylib (0) <6D5AE5B9-3F43-39E0-AD7C-0EFD2B5F8573> /usr/local/opt/snappy/lib/libsnappy.1.dylib
       0x10aeb9000 -        0x10aef0ffb +libmp3lame.0.dylib (0) <B69319FA-C9B7-3A7C-A150-E511F8D792BF> /usr/local/opt/lame/lib/libmp3lame.0.dylib
       0x10af2e000 -        0x10af4fffb +libopencore-amrnb.0.dylib (0) <C188B0CF-1693-384D-85E7-61E2A23B0D38> /usr/local/opt/opencore-amr/lib/libopencore-amrnb.0.dylib
       0x10af57000 -        0x10af8fffb +libopenjp2.7.dylib (0) <4559F881-D6A7-3F37-B260-5DC0071D6802> /usr/local/opt/openjpeg/lib/libopenjp2.7.dylib
       0x10af9c000 -        0x10afe4ffb +libopus.0.dylib (0) <D4D0F610-E34D-3208-9C4E-F147AACF0A03> /usr/local/opt/opus/lib/libopus.0.dylib
       0x10afef000 -        0x10b000ffb +libspeex.1.dylib (0) <375602FA-4F12-3F9E-9B5B-ADEFD40349DA> /usr/local/opt/speex/lib/libspeex.1.dylib
       0x10b00b000 -        0x10b032ff7 +libtheoraenc.1.dylib (0) <E29E2177-A794-3FC6-952C-FA28A2E19D12> /usr/local/opt/theora/lib/libtheoraenc.1.dylib
       0x10b03d000 -        0x10b04afff +libtheoradec.1.dylib (0) <2CD18A86-268E-3577-A1C2-14A425F619E6> /usr/local/opt/theora/lib/libtheoradec.1.dylib
       0x10b055000 -        0x10b058fff +libogg.0.dylib (0) <70E116A3-5EA5-3B75-AD66-35B0F0298639> /usr/local/opt/libogg/lib/libogg.0.dylib
       0x10b05d000 -        0x10b07ffff +libvorbis.0.dylib (0) <17074AE1-B95D-3BBE-A50A-739F32F95413> /usr/local/opt/libvorbis/lib/libvorbis.0.dylib
       0x10b088000 -        0x10b0fffff +libvorbisenc.2.dylib (0) <346B6C8D-D8FF-3D3E-8C47-AEA47C0BE159> /usr/local/opt/libvorbis/lib/libvorbisenc.2.dylib
       0x10b133000 -        0x10b2c7fdf +libx264.155.dylib (0) <3B71F212-8A91-37CE-B0DA-2BBAF200E6CF> /usr/local/opt/x264/lib/libx264.155.dylib
       0x10b3ee000 -        0x10c26afcb +libx265.169.dylib (0) <FBB1A8F7-B918-3DB3-9E39-7F81D80FBFE4> /usr/local/opt/x265/lib/libx265.169.dylib
       0x10c487000 -        0x10c4acff3 +libsoxr.0.dylib (0) <E8E57FC0-4561-30FF-97E8-4D5F6C285CA1> /usr/local/opt/libsoxr/lib/libsoxr.0.dylib
       0x10c4eb000 -        0x10c55cff7 +libswscale.5.dylib (0) <25CF236C-8F6D-3A28-A18D-1E9D3FFC6B4C> /usr/local/Cellar/ffmpeg/4.1.3_1/lib/libswscale.5.dylib
       0x10c56c000 -        0x10c588fff +libpostproc.55.dylib (0) <A143323A-4FC9-38C9-99E6-DCB5BFBF0FDF> /usr/local/Cellar/ffmpeg/4.1.3_1/lib/libpostproc.55.dylib
       0x10c590000 -        0x10c5a9ff7 +libavresample.4.dylib (0) <59F08A4C-948F-3EAF-BA36-A00E41C90FFE> /usr/local/Cellar/ffmpeg/4.1.3_1/lib/libavresample.4.dylib
       0x10c5b2000 -        0x10c5dcfff +librubberband.2.dylib (2.1.1) <D1434B63-6507-3725-85E8-DC7374F06121> /usr/local/opt/rubberband/lib/librubberband.2.dylib
       0x10c5f6000 -        0x10c7d3feb +libtesseract.4.dylib (0) <A81C3055-B927-3DF4-B703-6C08D831D3CD> /usr/local/opt/tesseract/lib/libtesseract.4.dylib
       0x10caee000 -        0x10cb16fe7 +libass.9.dylib (0) <C7EE5547-FDFC-37F2-91B9-01F777DC7197> /usr/local/opt/libass/lib/libass.9.dylib
       0x10cb25000 -        0x10cb54fff +libfontconfig.1.dylib (0) <DF644C1F-4C8C-372F-A0B4-B03BCB76AEDE> /usr/local/opt/fontconfig/lib/libfontconfig.1.dylib
       0x10cb67000 -        0x10cbe3ff7 +libfreetype.6.dylib (0) <DB64FA8D-F889-37BD-9A18-04083CD6B0FA> /usr/local/opt/freetype/lib/libfreetype.6.dylib
       0x10cc03000 -        0x10cc3bff7 +libbluray.2.dylib (0) <B2BB1E4C-FF04-36D4-978B-5F83568A0789> /usr/local/opt/libbluray/lib/libbluray.2.dylib
       0x10cc4d000 -        0x10cd86fef +libgnutls.30.dylib (0) <ED9C71BF-1C10-3CE3-9343-6FC53316EE39> /usr/local/opt/gnutls/lib/libgnutls.30.dylib
       0x10cdca000 -        0x10cdddfff +librtmp.1.dylib (0) <1A6EF3AE-2A09-3500-B7A3-EABBA1921969> /usr/local/opt/rtmpdump/lib/librtmp.1.dylib
       0x10cdea000 -        0x10ce2aff7 +libssl.1.0.0.dylib (0) <D2C19917-C050-3BBD-AD06-7596BC8D807B> /usr/local/opt/openssl/lib/libssl.1.0.0.dylib
       0x10ce4f000 -        0x10cfa2e9f +libcrypto.1.0.0.dylib (0) <7755BA8D-2054-35D0-B0B8-4A860B8C2129> /usr/local/opt/openssl/lib/libcrypto.1.0.0.dylib
       0x10d01d000 -        0x10d03ffff +libpng16.16.dylib (0) <211A7B4C-219C-3929-A2CF-D796E3A5F40A> /usr/local/opt/libpng/lib/libpng16.16.dylib
       0x10d04a000 -        0x10d0f1ff3 +libp11-kit.0.dylib (0) <E129E2F0-88F4-3DF0-B9A7-C5B98C92E471> /usr/local/opt/p11-kit/lib/libp11-kit.0.dylib
       0x10d13d000 -        0x10d2a0fff +libunistring.2.dylib (0) <A4545916-E2F4-3D6A-862B-528A6806E9FC> /usr/local/opt/libunistring/lib/libunistring.2.dylib
       0x10d2b5000 -        0x10d2c0fff +libtasn1.6.dylib (0) <A39E91B7-03B4-3674-9F21-00CED14D080E> /usr/local/opt/libtasn1/lib/libtasn1.6.dylib
       0x10d2c4000 -        0x10d2e9ff7 +libnettle.6.dylib (0) <057A74F3-9008-3D9D-AD5C-676747FB3AF5> /usr/local/opt/nettle/lib/libnettle.6.dylib
       0x10d2f5000 -        0x10d31effb +libhogweed.4.dylib (0) <8A53F23D-4D30-39C8-8C26-D10A8960A309> /usr/local/opt/nettle/lib/libhogweed.4.dylib
       0x10d328000 -        0x10d384fcf +libgmp.10.dylib (0) <7D2A1AB0-B206-3196-954C-5A0E17049998> /usr/local/opt/gmp/lib/libgmp.10.dylib
       0x10d393000 -        0x10d397fff +libffi.6.dylib (0) <47F6B233-3552-3D42-A3EC-1917E141AC53> /usr/local/opt/libffi/lib/libffi.6.dylib
       0x10d39d000 -        0x10d559ff7 +liblept.5.dylib (0) <3A1CB4F9-C6B0-3DED-B9CF-0A96D43E7B3F> /usr/local/opt/leptonica/lib/liblept.5.dylib
       0x10d5aa000 -        0x10d5d6ff7 +libjpeg.9.dylib (0) <C76CAB50-100A-3873-9E2E-7861B0C9D8C4> /usr/local/opt/jpeg/lib/libjpeg.9.dylib
       0x10d5e2000 -        0x10d5e7ffb +libgif.7.dylib (0) <BED1A4FB-CB35-3897-AE66-EA8219CC312D> /usr/local/opt/giflib/lib/libgif.7.dylib
       0x10d5ed000 -        0x10d647ff3 +libtiff.5.dylib (0) <9D70E831-88DD-3D51-9F03-F26395AD5DDD> /usr/local/opt/libtiff/lib/libtiff.5.dylib
       0x10d65b000 -        0x10d6affff +libwebp.7.dylib (0) <41532BC1-E0E7-35D2-9C48-8817CBA0F274> /usr/local/opt/webp/lib/libwebp.7.dylib
       0x10d6c3000 -        0x10d6dbff3 +libfribidi.0.dylib (0) <D9D77A1A-5EAB-34F0-98E4-A4D6C9134F7E> /usr/local/opt/fribidi/lib/libfribidi.0.dylib
       0x10d6e2000 -        0x10d781ffb +libharfbuzz.0.dylib (0) <518AC6EF-42B4-3832-8AA4-7B27344B1F12> /usr/local/opt/harfbuzz/lib/libharfbuzz.0.dylib
       0x10d7c3000 -        0x10d8b2ff3 +libglib-2.0.0.dylib (0) <CBDE35B2-D068-3A05-A4E5-5C33A028029F> /usr/local/opt/glib/lib/libglib-2.0.0.dylib
       0x10d90c000 -        0x10d914ff7 +libintl.8.dylib (0) <2BE4B1C7-92C9-39E9-B4A7-F880907047DB> /usr/local/opt/gettext/lib/libintl.8.dylib
       0x10d91f000 -        0x10d934ff3 +libgraphite2.3.dylib (0) <902BA2A7-0936-317E-A158-67ED52F736EE> /usr/local/opt/graphite2/lib/libgraphite2.3.dylib
       0x10d941000 -        0x10d9abfff +libpcre.1.dylib (0) <9C2F8523-710D-3BF3-8DDC-40CA9CA4FA7C> /usr/local/opt/pcre/lib/libpcre.1.dylib
       0x110aed000 -        0x110afaff7  com.apple.iokit.IOHIDLib (2.0.0 - 2.0.0) <3044C23E-A258-376F-A7F6-74F6CA8770FA> /System/Library/Extensions/IOHIDFamily.kext/Contents/PlugIns/IOHIDLib.plugin/Contents/MacOS/IOHIDLib
       0x110b0d000 -        0x110b11ffb  com.apple.audio.AppleHDAHALPlugIn (281.52 - 281.52) <23C7DDE6-A44B-3BE4-B47C-EB3045B267D9> /System/Library/Extensions/AppleHDA.kext/Contents/PlugIns/AppleHDAHALPlugIn.bundle/Contents/MacOS/AppleHDAHALPlugIn
       0x1122d8000 -        0x113055ff7  com.apple.driver.AppleIntelKBLGraphicsGLDriver (10.36.23 - 10.3.6) <96449C1B-CC52-371A-B04D-3AB1C75E1D46> /System/Library/Extensions/AppleIntelKBLGraphicsGLDriver.bundle/Contents/MacOS/AppleIntelKBLGraphicsGLDriver
       0x113493000 -        0x113555ffb  com.apple.AMDRadeonX4000GLDriver (1.68.21 - 1.6.8) <90BAA864-5DB7-30C8-BE74-D80070F22FAD> /System/Library/Extensions/AMDRadeonX4000GLDriver.bundle/Contents/MacOS/AMDRadeonX4000GLDriver
       0x116034000 -        0x11607eacf  dyld (551.5) <30B355CB-35BA-3112-AA76-4E46CD45F699> /usr/lib/dyld
    0x7fff3bd17000 -     0x7fff3bd26ffb  libSimplifiedChineseConverter.dylib (70) <79F6AF91-B369-3C30-8C52-19608D2566F9> /System/Library/CoreServices/Encodings/libSimplifiedChineseConverter.dylib
    0x7fff3c2e8000 -     0x7fff3cb3eff7  ATIRadeonX4000SCLib.dylib (1.68.21) <6B036072-A36F-3B82-9762-A6E13B4E282C> /System/Library/Extensions/AMDRadeonX4000GLDriver.bundle/Contents/MacOS/ATIRadeonX4000SCLib.dylib
    0x7fff3f974000 -     0x7fff3fb54ff3  com.apple.avfoundation (2.0 - 1536.36) <BB65ED51-CE44-31BD-A6EC-4B1EC5EADDD9> /System/Library/Frameworks/AVFoundation.framework/Versions/A/AVFoundation
    0x7fff3fb55000 -     0x7fff3fc0efff  com.apple.audio.AVFAudio (1.0 - ???) <ECE63BA3-4344-3522-904B-71F89677AC7D> /System/Library/Frameworks/AVFoundation.framework/Versions/A/Frameworks/AVFAudio.framework/Versions/A/AVFAudio
    0x7fff3fd14000 -     0x7fff3fd14fff  com.apple.Accelerate (1.11 - Accelerate 1.11) <8632A9C5-19EA-3FD7-A44D-80765CC9C540> /System/Library/Frameworks/Accelerate.framework/Versions/A/Accelerate
    0x7fff3fd15000 -     0x7fff3fd2bfef  libCGInterfaces.dylib (417.2) <E08ADB62-DF87-3BC8-81C6-60438991D4E1> /System/Library/Frameworks/Accelerate.framework/Versions/A/Frameworks/vImage.framework/Versions/A/Libraries/libCGInterfaces.dylib
    0x7fff3fd2c000 -     0x7fff4022afc3  com.apple.vImage (8.1 - ???) <A243A7EF-0C8E-3A9A-AA38-44AFD7507F00> /System/Library/Frameworks/Accelerate.framework/Versions/A/Frameworks/vImage.framework/Versions/A/vImage
    0x7fff4022b000 -     0x7fff40385fe3  libBLAS.dylib (1211.50.2) <62C659EB-3E32-3B5F-83BF-79F5DF30D5CE> /System/Library/Frameworks/Accelerate.framework/Versions/A/Frameworks/vecLib.framework/Versions/A/libBLAS.dylib
    0x7fff40386000 -     0x7fff403b4fef  libBNNS.dylib (38.1) <7BAEFDCA-3227-3E07-80D8-59B6370B89C6> /System/Library/Frameworks/Accelerate.framework/Versions/A/Frameworks/vecLib.framework/Versions/A/libBNNS.dylib
    0x7fff403b5000 -     0x7fff40774ff7  libLAPACK.dylib (1211.50.2) <40ADBA5F-8B2D-30AC-A7AD-7B17C37EE52D> /System/Library/Frameworks/Accelerate.framework/Versions/A/Frameworks/vecLib.framework/Versions/A/libLAPACK.dylib
    0x7fff40775000 -     0x7fff4078aff7  libLinearAlgebra.dylib (1211.50.2) <E8E0B7FD-A0B7-31E5-AF01-81781F71EBBE> /System/Library/Frameworks/Accelerate.framework/Versions/A/Frameworks/vecLib.framework/Versions/A/libLinearAlgebra.dylib
    0x7fff4078b000 -     0x7fff40790ff3  libQuadrature.dylib (3) <3D6BF66A-55B2-3692-BAC7-DEB0C676ED29> /System/Library/Frameworks/Accelerate.framework/Versions/A/Frameworks/vecLib.framework/Versions/A/libQuadrature.dylib
    0x7fff40791000 -     0x7fff40811fff  libSparse.dylib (79.50.2) <0DC25CDD-F8C1-3D6E-B472-8B060708424F> /System/Library/Frameworks/Accelerate.framework/Versions/A/Frameworks/vecLib.framework/Versions/A/libSparse.dylib
    0x7fff40812000 -     0x7fff40825fff  libSparseBLAS.dylib (1211.50.2) <722573CC-31CC-34B2-9032-E4F652A9CCFE> /System/Library/Frameworks/Accelerate.framework/Versions/A/Frameworks/vecLib.framework/Versions/A/libSparseBLAS.dylib
    0x7fff40826000 -     0x7fff409d3fc3  libvDSP.dylib (622.50.5) <40690941-CF89-3F90-A0AC-A4D200744A5D> /System/Library/Frameworks/Accelerate.framework/Versions/A/Frameworks/vecLib.framework/Versions/A/libvDSP.dylib
    0x7fff409d4000 -     0x7fff40a85fff  libvMisc.dylib (622.50.5) <BA2532DF-2D68-3DD0-9B59-D434BF702AA4> /System/Library/Frameworks/Accelerate.framework/Versions/A/Frameworks/vecLib.framework/Versions/A/libvMisc.dylib
    0x7fff40a86000 -     0x7fff40a86fff  com.apple.Accelerate.vecLib (3.11 - vecLib 3.11) <54FF3B43-E66C-3F36-B34B-A2B3B0A36502> /System/Library/Frameworks/Accelerate.framework/Versions/A/Frameworks/vecLib.framework/Versions/A/vecLib
    0x7fff40d7a000 -     0x7fff41bd8fff  com.apple.AppKit (6.9 - 1561.60.100) <6ADB4EAD-58E8-3C18-9062-A127601F86DB> /System/Library/Frameworks/AppKit.framework/Versions/C/AppKit
    0x7fff41c2a000 -     0x7fff41c2afff  com.apple.ApplicationServices (48 - 50) <3E227FC4-415F-3575-8C9C-8648301782C0> /System/Library/Frameworks/ApplicationServices.framework/Versions/A/ApplicationServices
    0x7fff41c2b000 -     0x7fff41c91fff  com.apple.ApplicationServices.ATS (377 - 445.5) <703CE7E4-426A-35C0-A229-F140F30F5340> /System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/ATS.framework/Versions/A/ATS
    0x7fff41d2a000 -     0x7fff41e4cfff  libFontParser.dylib (222.1.6) <6CEBACDD-B848-302E-B4B2-630CB16E663E> /System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/ATS.framework/Versions/A/Resources/libFontParser.dylib
    0x7fff41e4d000 -     0x7fff41e97ff7  libFontRegistry.dylib (221.5) <8F68EA59-C8EE-3FA3-BD19-0F1A58441440> /System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/ATS.framework/Versions/A/Resources/libFontRegistry.dylib
    0x7fff41f3c000 -     0x7fff41f6fff7  libTrueTypeScaler.dylib (222.1.6) <9147F859-8BD9-31D9-AB54-8E9549B92AE9> /System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/ATS.framework/Versions/A/Resources/libTrueTypeScaler.dylib
    0x7fff41fd9000 -     0x7fff41fddff3  com.apple.ColorSyncLegacy (4.13.0 - 1) <A5FB2694-1559-34A8-A3D3-2029F68A63CA> /System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/ColorSyncLegacy.framework/Versions/A/ColorSyncLegacy
    0x7fff4207d000 -     0x7fff420cfffb  com.apple.HIServices (1.22 - 624.1) <66FD9ED2-9630-313C-86AE-4C2FBCB3F351> /System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/HIServices.framework/Versions/A/HIServices
    0x7fff420d0000 -     0x7fff420defff  com.apple.LangAnalysis (1.7.0 - 1.7.0) <B65FF7E6-E9B5-34D8-8CA7-63D415A8A9A6> /System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/LangAnalysis.framework/Versions/A/LangAnalysis
    0x7fff420df000 -     0x7fff4212bfff  com.apple.print.framework.PrintCore (13.4 - 503.2) <B90C67C1-0292-3CEC-885D-F1882CD104BE> /System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/PrintCore.framework/Versions/A/PrintCore
    0x7fff4212c000 -     0x7fff42166fff  com.apple.QD (3.12 - 404.2) <38B20AFF-9D54-3B52-A6DC-C0D71380AA5F> /System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/QD.framework/Versions/A/QD
    0x7fff42167000 -     0x7fff42173fff  com.apple.speech.synthesis.framework (7.8.1 - 7.8.1) <A08DE016-C8F2-3B0E-BD34-15959D13DBF0> /System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/SpeechSynthesis.framework/Versions/A/SpeechSynthesis
    0x7fff42174000 -     0x7fff42402ff7  com.apple.audio.toolbox.AudioToolbox (1.14 - 1.14) <E0B8B5D8-80A0-308B-ABD6-F8612102B5D8> /System/Library/Frameworks/AudioToolbox.framework/Versions/A/AudioToolbox
    0x7fff42404000 -     0x7fff42404fff  com.apple.audio.units.AudioUnit (1.14 - 1.14) <ABF8778E-4F9D-305E-A528-DE406A1A2B68> /System/Library/Frameworks/AudioUnit.framework/Versions/A/AudioUnit
    0x7fff42727000 -     0x7fff42ac2ff7  com.apple.CFNetwork (902.3.1 - 902.3.1) <4C012538-BB8A-32F3-AACA-011092BEFAB1> /System/Library/Frameworks/CFNetwork.framework/Versions/A/CFNetwork
    0x7fff42ad7000 -     0x7fff42ad7fff  com.apple.Carbon (158 - 158) <F8B370D9-2103-3276-821D-ACC756167F86> /System/Library/Frameworks/Carbon.framework/Versions/A/Carbon
    0x7fff42ad8000 -     0x7fff42adbffb  com.apple.CommonPanels (1.2.6 - 98) <2391761C-5CAA-3F68-86B7-50B37927B104> /System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/CommonPanels.framework/Versions/A/CommonPanels
    0x7fff42adc000 -     0x7fff42de1fff  com.apple.HIToolbox (2.1.1 - 911.10) <BF7F9C0E-C732-3FB2-9BBC-362888BDA57B> /System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/HIToolbox.framework/Versions/A/HIToolbox
    0x7fff42de2000 -     0x7fff42de5ffb  com.apple.help (1.3.8 - 66) <DEBADFA8-C189-3195-B0D6-A1F2DE95882A> /System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/Help.framework/Versions/A/Help
    0x7fff42de6000 -     0x7fff42debfff  com.apple.ImageCapture (9.0 - 9.0) <23B4916F-3B43-3DFF-B956-FC390EECA284> /System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/ImageCapture.framework/Versions/A/ImageCapture
    0x7fff42dec000 -     0x7fff42e81ffb  com.apple.ink.framework (10.9 - 221) <5206C8B0-22DA-36C9-998E-846EDB626D5B> /System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/Ink.framework/Versions/A/Ink
    0x7fff42e82000 -     0x7fff42e9cff7  com.apple.openscripting (1.7 - 174) <1B2A1F9E-5534-3D61-83CA-9199B39E8708> /System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/OpenScripting.framework/Versions/A/OpenScripting
    0x7fff42ebd000 -     0x7fff42ebefff  com.apple.print.framework.Print (12 - 267) <3682ABFB-2561-3419-847D-02C247F4800D> /System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/Print.framework/Versions/A/Print
    0x7fff42ebf000 -     0x7fff42ec1ff7  com.apple.securityhi (9.0 - 55006) <C1406B8D-7D05-3959-808F-9C82189CF57F> /System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/SecurityHI.framework/Versions/A/SecurityHI
    0x7fff42ec2000 -     0x7fff42ec8fff  com.apple.speech.recognition.framework (6.0.3 - 6.0.3) <2ED8643D-B0C3-3F17-82A2-BBF13E6CBABC> /System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/SpeechRecognition.framework/Versions/A/SpeechRecognition
    0x7fff42fe9000 -     0x7fff42fe9fff  com.apple.Cocoa (6.11 - 22) <80C1AABB-FC32-3833-A53B-0E36E70EBABA> /System/Library/Frameworks/Cocoa.framework/Versions/A/Cocoa
    0x7fff42ff7000 -     0x7fff430b0fff  com.apple.ColorSync (4.13.0 - 3325) <D283C285-447D-3258-A7E4-59532123B8FF> /System/Library/Frameworks/ColorSync.framework/Versions/A/ColorSync
    0x7fff4323d000 -     0x7fff432d0ff7  com.apple.audio.CoreAudio (4.3.0 - 4.3.0) <EB35D3EC-56EA-33E6-98DC-BDC3A5FA8ACE> /System/Library/Frameworks/CoreAudio.framework/Versions/A/CoreAudio
    0x7fff43337000 -     0x7fff43360ffb  com.apple.CoreBluetooth (1.0 - 1) <E1335074-9D07-370E-8440-61C4874BAC56> /System/Library/Frameworks/CoreBluetooth.framework/Versions/A/CoreBluetooth
    0x7fff43361000 -     0x7fff436b7fef  com.apple.CoreData (120 - 851) <A2B59780-FB16-36A3-8EE0-E0EF072454E0> /System/Library/Frameworks/CoreData.framework/Versions/A/CoreData
    0x7fff436b8000 -     0x7fff4379bfff  com.apple.CoreDisplay (99.14 - 99.14) <A1B91ADD-828D-33A0-8A92-CC3F83DF89D0> /System/Library/Frameworks/CoreDisplay.framework/Versions/A/CoreDisplay
    0x7fff4379c000 -     0x7fff43c3dff7  com.apple.CoreFoundation (6.9 - 1454.93) <BA2165CA-0860-34A5-9238-75260E06E94B> /System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation
    0x7fff43c3f000 -     0x7fff4424ffef  com.apple.CoreGraphics (2.0 - 1161.21.2) <B3ECE8F4-92B5-3E04-A30D-72540D3B93CB> /System/Library/Frameworks/CoreGraphics.framework/Versions/A/CoreGraphics
    0x7fff44251000 -     0x7fff44540fff  com.apple.CoreImage (13.0.0 - 579.5) <AAE2DFD0-9B0A-3D56-8A3E-C460BAF70394> /System/Library/Frameworks/CoreImage.framework/Versions/A/CoreImage
    0x7fff447cf000 -     0x7fff448c5ffb  com.apple.CoreMedia (1.0 - 2276.80.3) <CCB8A012-28DA-3CCC-A3BC-AA60E3FDBF0E> /System/Library/Frameworks/CoreMedia.framework/Versions/A/CoreMedia
    0x7fff448c6000 -     0x7fff44914fff  com.apple.CoreMediaIO (814.0 - 4995) <665836FB-2193-3769-996F-D5644A98E92C> /System/Library/Frameworks/CoreMediaIO.framework/Versions/A/CoreMediaIO
    0x7fff44915000 -     0x7fff44915fff  com.apple.CoreServices (822.37 - 822.37) <264305C4-BB1C-3D3B-A2B1-D4EAA02669BF> /System/Library/Frameworks/CoreServices.framework/Versions/A/CoreServices
    0x7fff44916000 -     0x7fff4498affb  com.apple.AE (735.1 - 735.1) <08EBA184-20F7-3725-AEA6-C314448161C6> /System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/AE.framework/Versions/A/AE
    0x7fff4498b000 -     0x7fff44c62fff  com.apple.CoreServices.CarbonCore (1178.4 - 1178.4) <0D5E19BF-18CB-3FA4-8A5F-F6C787C5EE08> /System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/CarbonCore.framework/Versions/A/CarbonCore
    0x7fff44c63000 -     0x7fff44c97fff  com.apple.DictionaryServices (1.2 - 284.2) <6505B075-41C3-3C62-A4C3-85CE3F6825CD> /System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/DictionaryServices.framework/Versions/A/DictionaryServices
    0x7fff44c98000 -     0x7fff44ca0ffb  com.apple.CoreServices.FSEvents (1239.50.1 - 1239.50.1) <3637CEC7-DF0E-320E-9634-44A442925C65> /System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/FSEvents.framework/Versions/A/FSEvents
    0x7fff44ca1000 -     0x7fff44e5fff7  com.apple.LaunchServices (822.37 - 822.37) <6AA93307-220A-3417-BBC2-FE1C0DE0B652> /System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/LaunchServices.framework/Versions/A/LaunchServices
    0x7fff44e60000 -     0x7fff44f10fff  com.apple.Metadata (10.7.0 - 1191.7) <3DD530A7-E104-3469-98BB-20449834B2FE> /System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/Metadata.framework/Versions/A/Metadata
    0x7fff44f11000 -     0x7fff44f71fff  com.apple.CoreServices.OSServices (822.37 - 822.37) <4AD2FC98-C6CB-392A-A22F-196A723D7FAE> /System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/OSServices.framework/Versions/A/OSServices
    0x7fff44f72000 -     0x7fff44fe0fff  com.apple.SearchKit (1.4.0 - 1.4.0) <3662545A-B1CF-3079-BDCD-C83855CEFEEE> /System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/SearchKit.framework/Versions/A/SearchKit
    0x7fff44fe1000 -     0x7fff45005ffb  com.apple.coreservices.SharedFileList (71.21 - 71.21) <1B5228EF-D869-3A50-A373-7F4B0289FADD> /System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/SharedFileList.framework/Versions/A/SharedFileList
    0x7fff452a6000 -     0x7fff453f6fff  com.apple.CoreText (352.0 - 578.22) <6129F39D-284D-3BBF-8999-7854AB61C01C> /System/Library/Frameworks/CoreText.framework/Versions/A/CoreText
    0x7fff453f7000 -     0x7fff45431fff  com.apple.CoreVideo (1.8 - 0.0) <86CCC036-51BB-3DD1-9601-D93798BCCD0F> /System/Library/Frameworks/CoreVideo.framework/Versions/A/CoreVideo
    0x7fff45432000 -     0x7fff454bdff3  com.apple.framework.CoreWLAN (13.0 - 1350.2) <53966601-3913-3027-92AC-D79506F4DB6E> /System/Library/Frameworks/CoreWLAN.framework/Versions/A/CoreWLAN
    0x7fff45738000 -     0x7fff4573dfff  com.apple.DiskArbitration (2.7 - 2.7) <A975AD56-4CD3-3A89-8732-858CA9BD3DAA> /System/Library/Frameworks/DiskArbitration.framework/Versions/A/DiskArbitration
    0x7fff458fb000 -     0x7fff458fdff7  com.apple.ForceFeedback (1.0.6 - 1.0.6) <87FB3F31-DF4C-3172-BF86-45EFC331BDD8> /System/Library/Frameworks/ForceFeedback.framework/Versions/A/ForceFeedback
    0x7fff458fe000 -     0x7fff45cc4ff3  com.apple.Foundation (6.9 - 1454.93) <4780F1E1-7F46-3028-B2D1-C7960A8BD5AB> /System/Library/Frameworks/Foundation.framework/Versions/C/Foundation
    0x7fff45d35000 -     0x7fff45d65ff3  com.apple.GSS (4.0 - 2.0) <F0458628-964B-3B96-8C84-6FACB03CA63D> /System/Library/Frameworks/GSS.framework/Versions/A/GSS
    0x7fff45e77000 -     0x7fff45f7bffb  com.apple.Bluetooth (6.0.7 - 6.0.7f12) <1ADBFD1B-B92E-37FE-8F2D-BDB100DB77E1> /System/Library/Frameworks/IOBluetooth.framework/Versions/A/IOBluetooth
    0x7fff45fdb000 -     0x7fff46077fff  com.apple.framework.IOKit (2.0.2 - 1445.71.4) <52A5F1FF-BBAA-3088-933A-BD4A8AF6F6B1> /System/Library/Frameworks/IOKit.framework/Versions/A/IOKit
    0x7fff46079000 -     0x7fff46080fff  com.apple.IOSurface (211.15 - 211.15) <9FD406F1-6BF2-35B0-8339-DF83A1A661EB> /System/Library/Frameworks/IOSurface.framework/Versions/A/IOSurface
    0x7fff460d7000 -     0x7fff46251ff7  com.apple.ImageIO.framework (3.3.0 - 1739.3) <7C579D3F-AE0B-31C9-8F80-67F2290B8DE0> /System/Library/Frameworks/ImageIO.framework/Versions/A/ImageIO
    0x7fff46252000 -     0x7fff46256ffb  libGIF.dylib (1739.3) <7AA44C9D-48E8-3090-B044-61FE6F0AEF38> /System/Library/Frameworks/ImageIO.framework/Versions/A/Resources/libGIF.dylib
    0x7fff46257000 -     0x7fff4633efef  libJP2.dylib (1739.3) <AEBF7260-0C10-30C0-8F0F-8B347DEE78B3> /System/Library/Frameworks/ImageIO.framework/Versions/A/Resources/libJP2.dylib
    0x7fff4633f000 -     0x7fff46362ff7  libJPEG.dylib (1739.3) <D8C966AD-A00C-3E8B-A7ED-D7CC7ECB3224> /System/Library/Frameworks/ImageIO.framework/Versions/A/Resources/libJPEG.dylib
    0x7fff4663e000 -     0x7fff46664feb  libPng.dylib (1739.3) <1737F680-99D1-3F03-BFA5-5CDA30EB880A> /System/Library/Frameworks/ImageIO.framework/Versions/A/Resources/libPng.dylib
    0x7fff46665000 -     0x7fff46667ffb  libRadiance.dylib (1739.3) <21746434-FCC7-36DE-9331-11277DF66AA8> /System/Library/Frameworks/ImageIO.framework/Versions/A/Resources/libRadiance.dylib
    0x7fff46668000 -     0x7fff466b6fef  libTIFF.dylib (1739.3) <C4CB5C1D-20F2-3BD4-B0E6-629FDB3EF8E8> /System/Library/Frameworks/ImageIO.framework/Versions/A/Resources/libTIFF.dylib
    0x7fff47570000 -     0x7fff47589ff7  com.apple.Kerberos (3.0 - 1) <F86DCCDF-93C1-38B3-82C2-477C12E8EE6D> /System/Library/Frameworks/Kerberos.framework/Versions/A/Kerberos
    0x7fff47848000 -     0x7fff4784ffff  com.apple.MediaAccessibility (1.0 - 114) <9F72AACD-BAEB-3646-BD0F-12C47591C20D> /System/Library/Frameworks/MediaAccessibility.framework/Versions/A/MediaAccessibility
    0x7fff478ff000 -     0x7fff47f69ff7  com.apple.MediaToolbox (1.0 - 2276.80.3) <5AEB0117-CD84-3174-9671-579597592AFF> /System/Library/Frameworks/MediaToolbox.framework/Versions/A/MediaToolbox
    0x7fff47f6b000 -     0x7fff47fecfff  com.apple.Metal (125.30 - 125.30) <6B9EBDEE-C64C-3C1C-922A-0363B642C9BC> /System/Library/Frameworks/Metal.framework/Versions/A/Metal
    0x7fff48009000 -     0x7fff48024fff  com.apple.MetalPerformanceShaders.MPSCore (1.0 - 1) <AD754E8F-CA00-3878-9AF3-208C224A230B> /System/Library/Frameworks/MetalPerformanceShaders.framework/Frameworks/MPSCore.framework/Versions/A/MPSCore
    0x7fff48025000 -     0x7fff48094fef  com.apple.MetalPerformanceShaders.MPSImage (1.0 - 1) <338B7779-E608-3D68-8A07-2ACC11299744> /System/Library/Frameworks/MetalPerformanceShaders.framework/Frameworks/MPSImage.framework/Versions/A/MPSImage
    0x7fff48095000 -     0x7fff480b9fff  com.apple.MetalPerformanceShaders.MPSMatrix (1.0 - 1) <9CE072D7-853B-3939-9645-7EB951376B87> /System/Library/Frameworks/MetalPerformanceShaders.framework/Frameworks/MPSMatrix.framework/Versions/A/MPSMatrix
    0x7fff480ba000 -     0x7fff481a1ff7  com.apple.MetalPerformanceShaders.MPSNeuralNetwork (1.0 - 1) <0DE891AD-27E5-38FF-AEC8-4A95356C4357> /System/Library/Frameworks/MetalPerformanceShaders.framework/Frameworks/MPSNeuralNetwork.framework/Versions/A/MPSNeuralNetwork
    0x7fff481a2000 -     0x7fff481a2ff7  com.apple.MetalPerformanceShaders.MetalPerformanceShaders (1.0 - 1) <2D2D261C-50B0-32F9-BF9A-5C01382BB528> /System/Library/Frameworks/MetalPerformanceShaders.framework/Versions/A/MetalPerformanceShaders
    0x7fff491a1000 -     0x7fff491adffb  com.apple.NetFS (6.0 - 4.0) <471DD96F-FA2E-3FE9-9746-2519A6780D1A> /System/Library/Frameworks/NetFS.framework/Versions/A/NetFS
    0x7fff4bf96000 -     0x7fff4bf9efef  libcldcpuengine.dylib (2.8.7) <EF9A91AC-029C-300A-99E7-4952C15DA09F> /System/Library/Frameworks/OpenCL.framework/Versions/A/Libraries/libcldcpuengine.dylib
    0x7fff4bf9f000 -     0x7fff4bff9ff7  com.apple.opencl (2.8.24 - 2.8.24) <058114A8-6825-31D7-976E-0612B6A273E9> /System/Library/Frameworks/OpenCL.framework/Versions/A/OpenCL
    0x7fff4bffa000 -     0x7fff4c016ffb  com.apple.CFOpenDirectory (10.13 - 207.50.1) <29F55F7B-379F-3053-8FF3-5C6675A3DD4D> /System/Library/Frameworks/OpenDirectory.framework/Versions/A/Frameworks/CFOpenDirectory.framework/Versions/A/CFOpenDirectory
    0x7fff4c017000 -     0x7fff4c022fff  com.apple.OpenDirectory (10.13 - 207.50.1) <F895547D-4915-353F-9C1E-E95172BA803B> /System/Library/Frameworks/OpenDirectory.framework/Versions/A/OpenDirectory
    0x7fff4d1a1000 -     0x7fff4d1a3fff  libCVMSPluginSupport.dylib (16.7.4) <F9270AE0-CC3B-3E3E-BA32-CC1068DD8F27> /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libCVMSPluginSupport.dylib
    0x7fff4d1a4000 -     0x7fff4d1a9ffb  libCoreFSCache.dylib (162.9) <DD9DD721-2957-3D05-B361-70AF9EBFB280> /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libCoreFSCache.dylib
    0x7fff4d1aa000 -     0x7fff4d1aefff  libCoreVMClient.dylib (162.9) <4E4F3EA2-5B53-31A3-8256-54EFAA94E4D6> /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libCoreVMClient.dylib
    0x7fff4d1af000 -     0x7fff4d1b8ff3  libGFXShared.dylib (16.7.4) <EB2BF8A0-E10D-35EA-8F46-B2E3C62C12A8> /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGFXShared.dylib
    0x7fff4d1b9000 -     0x7fff4d1c4fff  libGL.dylib (16.7.4) <2BB333D3-5C61-33DF-8545-06DF2D08B83D> /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib
    0x7fff4d1c5000 -     0x7fff4d200fe7  libGLImage.dylib (16.7.4) <4DA003CE-0B74-3FE4-808C-B2FBCE517EB4> /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGLImage.dylib
    0x7fff4d201000 -     0x7fff4d36eff3  libGLProgrammability.dylib (16.7.4) <ECC9D79B-C0B1-33F9-A9BB-097EF12D9E13> /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGLProgrammability.dylib
    0x7fff4d36f000 -     0x7fff4d3adffb  libGLU.dylib (16.7.4) <BCB09CD8-EB0E-38FA-8B5A-9E29532EE364> /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGLU.dylib
    0x7fff4dd25000 -     0x7fff4dd34ff3  com.apple.opengl (16.7.4 - 16.7.4) <9BDE8FF9-5418-3C70-8D1C-09656884CE48> /System/Library/Frameworks/OpenGL.framework/Versions/A/OpenGL
    0x7fff4dd35000 -     0x7fff4deccff3  GLEngine (16.7.4) <E2EE1D9C-826E-3DA9-9DCA-2FF371CDE5BB> /System/Library/Frameworks/OpenGL.framework/Versions/A/Resources/GLEngine.bundle/GLEngine
    0x7fff4decd000 -     0x7fff4def5ffb  GLRendererFloat (16.7.4) <3B51AC1B-0A3C-30E5-80EB-F64EBB1B1F77> /System/Library/Frameworks/OpenGL.framework/Versions/A/Resources/GLRendererFloat.bundle/GLRendererFloat
    0x7fff4eb83000 -     0x7fff4edcfff7  com.apple.QuartzCore (1.11 - 584.64.2) <F9FFB507-48B3-381A-8BCC-617C54157590> /System/Library/Frameworks/QuartzCore.framework/Versions/A/QuartzCore
    0x7fff4f605000 -     0x7fff4f930fff  com.apple.security (7.0 - 58286.70.9) <2F4537D6-9FA7-32BE-8D3D-31E5992D096F> /System/Library/Frameworks/Security.framework/Versions/A/Security
    0x7fff4f931000 -     0x7fff4f9bdff7  com.apple.securityfoundation (6.0 - 55185.50.5) <67B7E8AD-2C9A-35B0-B05E-88ED346FC02B> /System/Library/Frameworks/SecurityFoundation.framework/Versions/A/SecurityFoundation
    0x7fff4f9ef000 -     0x7fff4f9f3ffb  com.apple.xpc.ServiceManagement (1.0 - 1) <85D84D87-E387-3422-A788-FD17F7180A2C> /System/Library/Frameworks/ServiceManagement.framework/Versions/A/ServiceManagement
    0x7fff4fd98000 -     0x7fff4fe08ff3  com.apple.SystemConfiguration (1.17 - 1.17) <8532B8E9-7E30-35A3-BC4A-DDE8E0614FDA> /System/Library/Frameworks/SystemConfiguration.framework/Versions/A/SystemConfiguration
    0x7fff4ffbd000 -     0x7fff50338fff  com.apple.VideoToolbox (1.0 - 2276.80.3) <546452DF-73CC-3C43-B391-FE532FEE5E31> /System/Library/Frameworks/VideoToolbox.framework/Versions/A/VideoToolbox
    0x7fff52d05000 -     0x7fff52d98fff  com.apple.APFS (1.0 - 1) <6BBB3988-1C91-314F-A77A-4E093A1B18F0> /System/Library/PrivateFrameworks/APFS.framework/Versions/A/APFS
    0x7fff539c4000 -     0x7fff539ecfff  com.apple.framework.Apple80211 (13.0 - 1370.4) <21593061-5389-3689-BBC2-F9F0E9D929F0> /System/Library/PrivateFrameworks/Apple80211.framework/Versions/A/Apple80211
    0x7fff539ee000 -     0x7fff539fdfef  com.apple.AppleFSCompression (96.60.1 - 1.0) <A7C875C4-F5EE-3272-AFB6-57C9FD5352B3> /System/Library/PrivateFrameworks/AppleFSCompression.framework/Versions/A/AppleFSCompression
    0x7fff53afc000 -     0x7fff53b07ff7  com.apple.AppleIDAuthSupport (1.0 - 1) <2FAF5567-CDB3-33EF-AB71-05D37F2248B7> /System/Library/PrivateFrameworks/AppleIDAuthSupport.framework/Versions/A/AppleIDAuthSupport
    0x7fff53b41000 -     0x7fff53b89ff3  com.apple.AppleJPEG (1.0 - 1) <8DD410CB-76A1-3F22-9A9F-0491FA0CEB4A> /System/Library/PrivateFrameworks/AppleJPEG.framework/Versions/A/AppleJPEG
    0x7fff53bc4000 -     0x7fff53becfff  com.apple.applesauce (1.0 - ???) <CCA8B094-1BCE-3AE3-A0A7-D544C818DE36> /System/Library/PrivateFrameworks/AppleSauce.framework/Versions/A/AppleSauce
    0x7fff53cb9000 -     0x7fff53d09ff7  com.apple.AppleVAFramework (5.0.41 - 5.0.41) <14E91E09-C345-3C5F-8D3C-4BC291FAF796> /System/Library/PrivateFrameworks/AppleVA.framework/Versions/A/AppleVA
    0x7fff5403f000 -     0x7fff542d8ffb  com.apple.AuthKit (1.0 - 1) <6CA71A11-91C5-307C-B933-9FCDEDCB580A> /System/Library/PrivateFrameworks/AuthKit.framework/Versions/A/AuthKit
    0x7fff5440f000 -     0x7fff54416ff7  com.apple.coreservices.BackgroundTaskManagement (1.0 - 57.1) <51A41CA3-DB1D-3380-993E-99C54AEE518E> /System/Library/PrivateFrameworks/BackgroundTaskManagement.framework/Versions/A/BackgroundTaskManagement
    0x7fff54417000 -     0x7fff5449eff7  com.apple.backup.framework (1.9.6 - 1.9.6) <3C96FD26-C7F2-3F37-885A-5A71372FA8F4> /System/Library/PrivateFrameworks/Backup.framework/Versions/A/Backup
    0x7fff55e58000 -     0x7fff55e61ff3  com.apple.CommonAuth (4.0 - 2.0) <230E9C02-9A73-3ED5-BD3D-9E04CAC5F74F> /System/Library/PrivateFrameworks/CommonAuth.framework/Versions/A/CommonAuth
    0x7fff5619d000 -     0x7fff565a5fff  com.apple.CoreAUC (259.0.0 - 259.0.0) <1E0FB2C7-109E-3924-8E7F-8C6ACD78AF26> /System/Library/PrivateFrameworks/CoreAUC.framework/Versions/A/CoreAUC
    0x7fff565a6000 -     0x7fff565d6ff7  com.apple.CoreAVCHD (5.9.0 - 5900.4.1) <E9FF9574-122A-3966-AA2B-546E512ACD06> /System/Library/PrivateFrameworks/CoreAVCHD.framework/Versions/A/CoreAVCHD
    0x7fff56975000 -     0x7fff56985ff7  com.apple.CoreEmoji (1.0 - 69.3) <A4357F5C-0C38-3A61-B456-D7321EB2CEE5> /System/Library/PrivateFrameworks/CoreEmoji.framework/Versions/A/CoreEmoji
    0x7fff57119000 -     0x7fff57121ff3  com.apple.CorePhoneNumbers (1.0 - 1) <A5D41251-9F38-3AB9-9DE7-F77023FAAA44> /System/Library/PrivateFrameworks/CorePhoneNumbers.framework/Versions/A/CorePhoneNumbers
    0x7fff572ac000 -     0x7fff572ddff3  com.apple.CoreServicesInternal (309.1 - 309.1) <4ECD14EA-A493-3B84-A32F-CF928474A405> /System/Library/PrivateFrameworks/CoreServicesInternal.framework/Versions/A/CoreServicesInternal
    0x7fff57657000 -     0x7fff576e8fff  com.apple.CoreSymbolication (9.3 - 64026.2) <A8D4315F-5DD5-3164-8672-ECDAF2766644> /System/Library/PrivateFrameworks/CoreSymbolication.framework/Versions/A/CoreSymbolication
    0x7fff5776b000 -     0x7fff578a0fff  com.apple.coreui (2.1 - 494.1) <B2C515C3-FCE8-3B28-A225-05AD917F509B> /System/Library/PrivateFrameworks/CoreUI.framework/Versions/A/CoreUI
    0x7fff578a1000 -     0x7fff579d2fff  com.apple.CoreUtils (5.6 - 560.11) <1A02D6F0-8C65-3FAE-AD63-56477EDE4773> /System/Library/PrivateFrameworks/CoreUtils.framework/Versions/A/CoreUtils
    0x7fff57a27000 -     0x7fff57a8bfff  com.apple.framework.CoreWiFi (13.0 - 1350.2) <373AD7DB-3947-300A-8B8C-9BABC6D1AE12> /System/Library/PrivateFrameworks/CoreWiFi.framework/Versions/A/CoreWiFi
    0x7fff57a8c000 -     0x7fff57a9cff7  com.apple.CrashReporterSupport (10.13 - 1) <A909F468-0648-3F51-A77E-3F9ADBC9A941> /System/Library/PrivateFrameworks/CrashReporterSupport.framework/Versions/A/CrashReporterSupport
    0x7fff57b1b000 -     0x7fff57b2aff7  com.apple.framework.DFRFoundation (1.0 - 191.7) <3B8ED6F7-5DFF-34C3-BA90-DDB85679684C> /System/Library/PrivateFrameworks/DFRFoundation.framework/Versions/A/DFRFoundation
    0x7fff57b2d000 -     0x7fff57b31ffb  com.apple.DSExternalDisplay (3.1 - 380) <901B7F6D-376A-3848-99D0-170C4D00F776> /System/Library/PrivateFrameworks/DSExternalDisplay.framework/Versions/A/DSExternalDisplay
    0x7fff57bb3000 -     0x7fff57c29fff  com.apple.datadetectorscore (7.0 - 590.3) <B4706195-CBE6-320D-A0E1-A9D4BDF52791> /System/Library/PrivateFrameworks/DataDetectorsCore.framework/Versions/A/DataDetectorsCore
    0x7fff57c77000 -     0x7fff57cb7ff7  com.apple.DebugSymbols (181.0 - 181.0) <299A0238-ED78-3676-B131-274D972824AA> /System/Library/PrivateFrameworks/DebugSymbols.framework/Versions/A/DebugSymbols
    0x7fff57cb8000 -     0x7fff57de7fff  com.apple.desktopservices (1.12.5 - 1.12.5) <7739C9A5-64D9-31A5-899B-5FFA242AD70D> /System/Library/PrivateFrameworks/DesktopServicesPriv.framework/Versions/A/DesktopServicesPriv
    0x7fff58c01000 -     0x7fff5902ffff  com.apple.vision.FaceCore (3.3.2 - 3.3.2) <B574FE33-4A41-3611-9738-388EBAF03E37> /System/Library/PrivateFrameworks/FaceCore.framework/Versions/A/FaceCore
    0x7fff5c2eb000 -     0x7fff5c2f6ff7  libGPUSupportMercury.dylib (16.7.4) <A4D6F79C-1DFA-3E96-8F76-4882FBEDE9CF> /System/Library/PrivateFrameworks/GPUSupport.framework/Versions/A/Libraries/libGPUSupportMercury.dylib
    0x7fff5c2f7000 -     0x7fff5c2fcfff  com.apple.GPUWrangler (3.20.0 - 3.20.0) <54EC174A-C5E6-3CA2-8A8D-9DA6ACE68C3D> /System/Library/PrivateFrameworks/GPUWrangler.framework/Versions/A/GPUWrangler
    0x7fff5d072000 -     0x7fff5d081fff  com.apple.GraphVisualizer (1.0 - 5) <B993B8A2-5700-3DFC-9EB7-4CCEE8F959F1> /System/Library/PrivateFrameworks/GraphVisualizer.framework/Versions/A/GraphVisualizer
    0x7fff5d104000 -     0x7fff5d178fff  com.apple.Heimdal (4.0 - 2.0) <A5C34322-456F-3AF0-9B53-A32862C8B4E3> /System/Library/PrivateFrameworks/Heimdal.framework/Versions/A/Heimdal
    0x7fff5da88000 -     0x7fff5da8fff7  com.apple.IOAccelerator (378.26 - 378.26) <2274BE11-18DE-3B13-BCDB-C488C9BB19AD> /System/Library/PrivateFrameworks/IOAccelerator.framework/Versions/A/IOAccelerator
    0x7fff5da93000 -     0x7fff5daaafff  com.apple.IOPresentment (1.0 - 35.1) <7C6332FF-6535-3064-B437-1E9F70671927> /System/Library/PrivateFrameworks/IOPresentment.framework/Versions/A/IOPresentment
    0x7fff5de75000 -     0x7fff5de9bffb  com.apple.IconServices (97.6 - 97.6) <A56D826D-20D2-34BE-AACC-A80CFCB4E915> /System/Library/PrivateFrameworks/IconServices.framework/Versions/A/IconServices
    0x7fff5dfa8000 -     0x7fff5dfabff3  com.apple.InternationalSupport (1.0 - 1) <5AB382FD-BF81-36A1-9565-61F1FD398ECA> /System/Library/PrivateFrameworks/InternationalSupport.framework/Versions/A/InternationalSupport
    0x7fff5e019000 -     0x7fff5e029ffb  com.apple.IntlPreferences (2.0 - 227.5.2) <7FC3783F-C96A-3CD6-BBA8-2BC682BC7109> /System/Library/PrivateFrameworks/IntlPreferences.framework/Versions/A/IntlPreferences
    0x7fff5e120000 -     0x7fff5e133ff3  com.apple.security.KeychainCircle.KeychainCircle (1.0 - 1) <D919DF09-908E-34F8-99D0-28E7E548D932> /System/Library/PrivateFrameworks/KeychainCircle.framework/Versions/A/KeychainCircle
    0x7fff5e134000 -     0x7fff5e229ff7  com.apple.LanguageModeling (1.0 - 159.5.3) <7F0AC200-E3DD-39FB-8A95-00DD70B66A9F> /System/Library/PrivateFrameworks/LanguageModeling.framework/Versions/A/LanguageModeling
    0x7fff5e22a000 -     0x7fff5e26cfff  com.apple.Lexicon-framework (1.0 - 33.5) <DC94CF9E-1EB4-3C0E-B298-CA1190885276> /System/Library/PrivateFrameworks/Lexicon.framework/Versions/A/Lexicon
    0x7fff5e270000 -     0x7fff5e277ff7  com.apple.LinguisticData (1.0 - 238.3) <49A54649-1021-3DBD-99B8-1B2EDFFA5378> /System/Library/PrivateFrameworks/LinguisticData.framework/Versions/A/LinguisticData
    0x7fff5ea76000 -     0x7fff5ea79fff  com.apple.Mangrove (1.0 - 1) <27D6DF76-B5F8-3443-8826-D25B284331BF> /System/Library/PrivateFrameworks/Mangrove.framework/Versions/A/Mangrove
    0x7fff5ef89000 -     0x7fff5eff2ff7  com.apple.gpusw.MetalTools (1.0 - 1) <B4217182-B2AA-3AA3-80E8-F4C80B71BDDC> /System/Library/PrivateFrameworks/MetalTools.framework/Versions/A/MetalTools
    0x7fff5f181000 -     0x7fff5f19afff  com.apple.MobileKeyBag (2.0 - 1.0) <52760DF8-D921-3E61-9A54-447F9F7E08A0> /System/Library/PrivateFrameworks/MobileKeyBag.framework/Versions/A/MobileKeyBag
    0x7fff5f226000 -     0x7fff5f250ffb  com.apple.MultitouchSupport.framework (1614.1 - 1614.1) <A0709B43-FA9E-3617-8E7A-D68CDFAB2167> /System/Library/PrivateFrameworks/MultitouchSupport.framework/Versions/A/MultitouchSupport
    0x7fff5f4b7000 -     0x7fff5f4c2fff  com.apple.NetAuth (6.2 - 6.2) <6B8932DD-ABC2-334C-BEA2-20F049634167> /System/Library/PrivateFrameworks/NetAuth.framework/Versions/A/NetAuth
    0x7fff60d5f000 -     0x7fff60d6fffb  com.apple.PerformanceAnalysis (1.194 - 194) <8022ED1F-CE70-388E-B37B-6FB1E5F90CF2> /System/Library/PrivateFrameworks/PerformanceAnalysis.framework/Versions/A/PerformanceAnalysis
    0x7fff62b2e000 -     0x7fff62b4cfff  com.apple.ProtocolBuffer (1 - 260) <40704740-4A53-3010-A49B-08D1D69D1D5E> /System/Library/PrivateFrameworks/ProtocolBuffer.framework/Versions/A/ProtocolBuffer
    0x7fff62d1a000 -     0x7fff62d26fff  com.apple.xpc.RemoteServiceDiscovery (1.0 - 1205.70.10) <178A00AF-E9DA-3AA8-9311-7D6087240AAC> /System/Library/PrivateFrameworks/RemoteServiceDiscovery.framework/Versions/A/RemoteServiceDiscovery
    0x7fff62d27000 -     0x7fff62d4affb  com.apple.RemoteViewServices (2.0 - 125) <592323D1-CB44-35F1-9921-4C2AB8D920A0> /System/Library/PrivateFrameworks/RemoteViewServices.framework/Versions/A/RemoteViewServices
    0x7fff62d4b000 -     0x7fff62d60ff3  com.apple.xpc.RemoteXPC (1.0 - 1205.70.10) <A9C84936-30E2-3E8C-9151-305E4BDFE35E> /System/Library/PrivateFrameworks/RemoteXPC.framework/Versions/A/RemoteXPC
    0x7fff6466e000 -     0x7fff64783ff7  com.apple.Sharing (1050.22.2 - 1050.22.2) <4E3CCDF2-EA26-334F-8EBA-79BD28486C9D> /System/Library/PrivateFrameworks/Sharing.framework/Versions/A/Sharing
    0x7fff647ae000 -     0x7fff647afff7  com.apple.performance.SignpostNotification (1.2.6 - 2.6) <8F04800F-3570-3392-A24D-B229FF03F7F9> /System/Library/PrivateFrameworks/SignpostNotification.framework/Versions/A/SignpostNotification
    0x7fff6550c000 -     0x7fff657a8fff  com.apple.SkyLight (1.600.0 - 312.103.2) <E5B27C32-3AFB-31FC-9379-4A28D79309A1> /System/Library/PrivateFrameworks/SkyLight.framework/Versions/A/SkyLight
    0x7fff65f71000 -     0x7fff65f7efff  com.apple.SpeechRecognitionCore (4.6.1 - 4.6.1) <87EE7AB5-6925-3D21-BE00-F155CB457699> /System/Library/PrivateFrameworks/SpeechRecognitionCore.framework/Versions/A/SpeechRecognitionCore
    0x7fff66b24000 -     0x7fff66badfc7  com.apple.Symbolication (9.3 - 64033) <FAA17252-6378-34A4-BBBB-22DF54EC1626> /System/Library/PrivateFrameworks/Symbolication.framework/Versions/A/Symbolication
    0x7fff6711d000 -     0x7fff67125ff7  com.apple.TCC (1.0 - 1) <E1EB7272-FE6F-39AB-83CA-B2B5F2A88D9B> /System/Library/PrivateFrameworks/TCC.framework/Versions/A/TCC
    0x7fff67332000 -     0x7fff673efff7  com.apple.TextureIO (3.7 - 3.7) <F8BAC954-405D-3CC3-AB7B-048C866EF980> /System/Library/PrivateFrameworks/TextureIO.framework/Versions/A/TextureIO
    0x7fff67499000 -     0x7fff6749afff  com.apple.TrustEvaluationAgent (2.0 - 31) <39F533B2-211E-3635-AF47-23F27749FF4A> /System/Library/PrivateFrameworks/TrustEvaluationAgent.framework/Versions/A/TrustEvaluationAgent
    0x7fff674a0000 -     0x7fff6764ffff  com.apple.UIFoundation (1.0 - 547.5) <86A2FBA7-2709-3894-A3D5-A00C19BAC48D> /System/Library/PrivateFrameworks/UIFoundation.framework/Versions/A/UIFoundation
    0x7fff68dcb000 -     0x7fff68dcdffb  com.apple.loginsupport (1.0 - 1) <D1232C1B-80EA-3DF8-9466-013695D0846E> /System/Library/PrivateFrameworks/login.framework/Versions/A/Frameworks/loginsupport.framework/Versions/A/loginsupport
    0x7fff68f34000 -     0x7fff68f67ff7  libclosured.dylib (551.5) <112BC241-6626-3848-8DD8-B34B5B6F7ABC> /usr/lib/closure/libclosured.dylib
    0x7fff69021000 -     0x7fff6905aff7  libCRFSuite.dylib (41) <FE5EDB68-2593-3C2E-BBAF-1C52D206F296> /usr/lib/libCRFSuite.dylib
    0x7fff6905b000 -     0x7fff69066fff  libChineseTokenizer.dylib (28) <53633C9B-A3A8-36F7-A53C-432D802F4BB8> /usr/lib/libChineseTokenizer.dylib
    0x7fff690f8000 -     0x7fff690f9ff3  libDiagnosticMessagesClient.dylib (104) <9712E980-76EE-3A89-AEA6-DF4BAF5C0574> /usr/lib/libDiagnosticMessagesClient.dylib
    0x7fff69130000 -     0x7fff692faff3  libFosl_dynamic.dylib (17.8) <C58ED77A-4986-31C2-994C-34DDFB8106F0> /usr/lib/libFosl_dynamic.dylib
    0x7fff69332000 -     0x7fff69332fff  libOpenScriptingUtil.dylib (174) <610F0242-7CE5-3C86-951B-B646562694AF> /usr/lib/libOpenScriptingUtil.dylib
    0x7fff69469000 -     0x7fff6946dffb  libScreenReader.dylib (562.18.4) <E239923D-54C9-3BBF-852F-87C09DEF4091> /usr/lib/libScreenReader.dylib
    0x7fff6946e000 -     0x7fff6946fffb  libSystem.B.dylib (1252.50.4) <FE429C40-31DB-39A9-8B98-A8E688F58478> /usr/lib/libSystem.B.dylib
    0x7fff69502000 -     0x7fff69502fff  libapple_crypto.dylib (109.50.14) <48BA2E76-BF2F-3522-A54E-D7FB7EAF7A57> /usr/lib/libapple_crypto.dylib
    0x7fff69503000 -     0x7fff69519ff7  libapple_nghttp2.dylib (1.24) <01402BC4-4822-3676-9C80-50D83F816424> /usr/lib/libapple_nghttp2.dylib
    0x7fff6951a000 -     0x7fff69544ff3  libarchive.2.dylib (54) <8FC28DD8-E315-3C3E-95FE-D1D2CBE49888> /usr/lib/libarchive.2.dylib
    0x7fff69545000 -     0x7fff695c6fdf  libate.dylib (1.13.1) <178ACDAD-DE7E-346C-A613-1CBF7929AC07> /usr/lib/libate.dylib
    0x7fff695ca000 -     0x7fff695caff3  libauto.dylib (187) <A05C7900-F8C7-3E75-8D3F-909B40C19717> /usr/lib/libauto.dylib
    0x7fff695cb000 -     0x7fff69683ff3  libboringssl.dylib (109.50.14) <E6813F87-B5E4-3F7F-A725-E6A7F2BD02EC> /usr/lib/libboringssl.dylib
    0x7fff69684000 -     0x7fff69694ff3  libbsm.0.dylib (39) <6BC96A72-AFBE-34FD-91B1-748A530D8AE6> /usr/lib/libbsm.0.dylib
    0x7fff69695000 -     0x7fff696a2ffb  libbz2.1.0.dylib (38) <0A5086BB-4724-3C14-979D-5AD4F26B5B45> /usr/lib/libbz2.1.0.dylib
    0x7fff696a3000 -     0x7fff696f9fff  libc++.1.dylib (400.9) <7D3DACCC-3804-393C-ABC1-1A580FD00CB6> /usr/lib/libc++.1.dylib
    0x7fff696fa000 -     0x7fff6971eff7  libc++abi.dylib (400.8.2) <EF5E37D7-11D9-3530-BE45-B986612D13E2> /usr/lib/libc++abi.dylib
    0x7fff69720000 -     0x7fff69730fff  libcmph.dylib (6) <890DEC4C-4334-393C-8B56-7C8560BBED9D> /usr/lib/libcmph.dylib
    0x7fff69731000 -     0x7fff69748fcf  libcompression.dylib (47.60.2) <543F07BF-2F2F-37D5-9866-E84BF659885B> /usr/lib/libcompression.dylib
    0x7fff699f3000 -     0x7fff69a0bff7  libcoretls.dylib (155.50.1) <D350052E-DC4D-3185-ADBA-BA48EDCEE955> /usr/lib/libcoretls.dylib
    0x7fff69a0c000 -     0x7fff69a0dff3  libcoretls_cfhelpers.dylib (155.50.1) <B297F5D8-F2FE-3566-A752-E9D998B9C039> /usr/lib/libcoretls_cfhelpers.dylib
    0x7fff69ba6000 -     0x7fff69d37fff  libcrypto.35.dylib (22.50.3) <6E609F99-59BC-3AEA-9DB3-FD360A2C50CC> /usr/lib/libcrypto.35.dylib
    0x7fff69ede000 -     0x7fff69f34ff3  libcups.2.dylib (462.2.5) <EA944DD1-0B60-32E9-8FB4-BE642D2E7352> /usr/lib/libcups.2.dylib
    0x7fff6a074000 -     0x7fff6a074fff  libenergytrace.dylib (16) <A92AB8B8-B986-3CE6-980D-D55090FEF387> /usr/lib/libenergytrace.dylib
    0x7fff6a075000 -     0x7fff6a08effb  libexpat.1.dylib (16.1.1) <5E1796FA-4041-3187-B5C2-8E6B03D1D72A> /usr/lib/libexpat.1.dylib
    0x7fff6a0ab000 -     0x7fff6a0b0ff3  libheimdal-asn1.dylib (520.50.7) <BC22EC07-A701-3B8F-B075-E29BB437E6CF> /usr/lib/libheimdal-asn1.dylib
    0x7fff6a0dc000 -     0x7fff6a1cdff7  libiconv.2.dylib (51.50.1) <2FEC9707-3FAF-3828-A50D-8605086D060F> /usr/lib/libiconv.2.dylib
    0x7fff6a1ce000 -     0x7fff6a3f5ffb  libicucore.A.dylib (59181.0.1) <2CE6205F-D375-3BF5-AA0A-3254BC4773D0> /usr/lib/libicucore.A.dylib
    0x7fff6a442000 -     0x7fff6a443fff  liblangid.dylib (128) <39C39393-0D05-301D-93B2-F224FC4949AA> /usr/lib/liblangid.dylib
    0x7fff6a444000 -     0x7fff6a45dffb  liblzma.5.dylib (10) <3D419A50-961F-37D2-8A01-3DC7AB7B8D18> /usr/lib/liblzma.5.dylib
    0x7fff6a45e000 -     0x7fff6a474ff7  libmarisa.dylib (9) <D6D2D55D-1D2E-3442-B152-B18803C0ABB4> /usr/lib/libmarisa.dylib
    0x7fff6a525000 -     0x7fff6a74dff7  libmecabra.dylib (779.7.6) <F462F170-E872-3D09-B219-973D5E99C09F> /usr/lib/libmecabra.dylib
    0x7fff6a925000 -     0x7fff6aaa0fff  libnetwork.dylib (1229.70.2) <E185D902-AC7F-3044-87C0-AE2887C59CE7> /usr/lib/libnetwork.dylib
    0x7fff6ab27000 -     0x7fff6af157e7  libobjc.A.dylib (723) <DD9E5EC5-B507-3249-B700-93433E2D5EDF> /usr/lib/libobjc.A.dylib
    0x7fff6af28000 -     0x7fff6af2cfff  libpam.2.dylib (22) <7B4D2CE2-1438-387A-9802-5CEEFBF26F86> /usr/lib/libpam.2.dylib
    0x7fff6af2f000 -     0x7fff6af63fff  libpcap.A.dylib (79.20.1) <FA13918B-A247-3181-B256-9B852C7BA316> /usr/lib/libpcap.A.dylib
    0x7fff6afe2000 -     0x7fff6affeffb  libresolv.9.dylib (65) <E8F3415B-4472-3202-8901-41FD87981DB2> /usr/lib/libresolv.9.dylib
    0x7fff6b04d000 -     0x7fff6b04eff3  libspindump.dylib (252) <D8E27057-E3CC-3D7F-A010-4A87830F6A83> /usr/lib/libspindump.dylib
    0x7fff6b04f000 -     0x7fff6b1e2ff7  libsqlite3.dylib (274.8.1) <FCAD6A57-829E-3701-B16E-1833D620E0E8> /usr/lib/libsqlite3.dylib
    0x7fff6b3b6000 -     0x7fff6b416ff3  libusrtcp.dylib (1229.70.2) <1E065228-D0E3-3808-9405-894056C6BEC0> /usr/lib/libusrtcp.dylib
    0x7fff6b417000 -     0x7fff6b41affb  libutil.dylib (51.20.1) <216D18E5-0BAF-3EAF-A38E-F6AC37CBABD9> /usr/lib/libutil.dylib
    0x7fff6b41b000 -     0x7fff6b428fff  libxar.1.dylib (400) <0316128D-3B47-3052-995D-97B4FE5491DC> /usr/lib/libxar.1.dylib
    0x7fff6b42c000 -     0x7fff6b513fff  libxml2.2.dylib (31.13) <8C12B82A-66FD-330C-9BEA-AAC090C7076A> /usr/lib/libxml2.2.dylib
    0x7fff6b514000 -     0x7fff6b53cfff  libxslt.1.dylib (15.12) <4A5E011D-8B29-3135-A52B-9A9070ABD752> /usr/lib/libxslt.1.dylib
    0x7fff6b53d000 -     0x7fff6b54fffb  libz.1.dylib (70) <48C67CFC-940D-3857-8DAD-857774605352> /usr/lib/libz.1.dylib
    0x7fff6b5eb000 -     0x7fff6b5efff7  libcache.dylib (80) <092479CB-1008-3A83-BECF-E115F24D13C1> /usr/lib/system/libcache.dylib
    0x7fff6b5f0000 -     0x7fff6b5faff3  libcommonCrypto.dylib (60118.50.1) <029F5985-9B6E-3DCB-9B96-FD007678C6A7> /usr/lib/system/libcommonCrypto.dylib
    0x7fff6b5fb000 -     0x7fff6b602fff  libcompiler_rt.dylib (62) <968B8E3F-3681-3230-9D78-BB8732024F6E> /usr/lib/system/libcompiler_rt.dylib
    0x7fff6b603000 -     0x7fff6b60cffb  libcopyfile.dylib (146.50.5) <3885083D-50D8-3EEC-B481-B2E605180D7F> /usr/lib/system/libcopyfile.dylib
    0x7fff6b60d000 -     0x7fff6b692ffb  libcorecrypto.dylib (562.70.2) <495BACA2-67D7-369D-ABB1-FE67FAF63A6A> /usr/lib/system/libcorecrypto.dylib
    0x7fff6b71a000 -     0x7fff6b753ff7  libdispatch.dylib (913.60.3) <BF368549-2DFB-3530-B4CB-31D5EDAC4F2F> /usr/lib/system/libdispatch.dylib
    0x7fff6b754000 -     0x7fff6b771ff7  libdyld.dylib (551.5) <49BF9E96-8297-30CF-8AA6-128CC14054B2> /usr/lib/system/libdyld.dylib
    0x7fff6b772000 -     0x7fff6b772ffb  libkeymgr.dylib (28) <E34E283E-90FA-3C59-B48E-1277CDB9CDCE> /usr/lib/system/libkeymgr.dylib
    0x7fff6b773000 -     0x7fff6b77fff3  libkxld.dylib (4570.71.22) <A549EF48-67A7-323B-9211-E4CCA518760E> /usr/lib/system/libkxld.dylib
    0x7fff6b780000 -     0x7fff6b780ff7  liblaunch.dylib (1205.70.10) <ACB92462-EDA9-39E6-BB4E-635D47D30D58> /usr/lib/system/liblaunch.dylib
    0x7fff6b781000 -     0x7fff6b785ffb  libmacho.dylib (906) <1902A611-081A-3452-B11E-EBD1B166E831> /usr/lib/system/libmacho.dylib
    0x7fff6b786000 -     0x7fff6b788ff3  libquarantine.dylib (86) <26C0BA22-8F93-3A07-9A4E-C8D53D2CE42E> /usr/lib/system/libquarantine.dylib
    0x7fff6b789000 -     0x7fff6b78aff3  libremovefile.dylib (45) <711E18B2-5BBE-3211-A916-56740C27D17A> /usr/lib/system/libremovefile.dylib
    0x7fff6b78b000 -     0x7fff6b7a2fff  libsystem_asl.dylib (356.70.1) <39E46A6F-B228-3E78-B83E-1779F9707A39> /usr/lib/system/libsystem_asl.dylib
    0x7fff6b7a3000 -     0x7fff6b7a3fff  libsystem_blocks.dylib (67) <17303FDF-0D2D-3963-B05E-B4DF63052D47> /usr/lib/system/libsystem_blocks.dylib
    0x7fff6b7a4000 -     0x7fff6b82dff7  libsystem_c.dylib (1244.50.9) <1187BFE8-4576-3247-8177-481554E1F9E7> /usr/lib/system/libsystem_c.dylib
    0x7fff6b82e000 -     0x7fff6b831ffb  libsystem_configuration.dylib (963.50.8) <DF6B5287-203E-30CB-9947-78DF446C72B8> /usr/lib/system/libsystem_configuration.dylib
    0x7fff6b832000 -     0x7fff6b835ffb  libsystem_coreservices.dylib (51) <486000D3-D8CB-3BE7-8EE5-8BF380DE6DF7> /usr/lib/system/libsystem_coreservices.dylib
    0x7fff6b836000 -     0x7fff6b837fff  libsystem_darwin.dylib (1244.50.9) <09C21A4A-9EE0-388B-A9D9-DFF8F6758791> /usr/lib/system/libsystem_darwin.dylib
    0x7fff6b838000 -     0x7fff6b83eff7  libsystem_dnssd.dylib (878.70.3) <7C4C39D5-3642-3049-B309-7ACF2F3CE0DA> /usr/lib/system/libsystem_dnssd.dylib
    0x7fff6b83f000 -     0x7fff6b888ff7  libsystem_info.dylib (517.30.1) <AB634A98-B8AA-3804-8436-38261FC8EC4D> /usr/lib/system/libsystem_info.dylib
    0x7fff6b889000 -     0x7fff6b8afff7  libsystem_kernel.dylib (4570.71.22) <6BFAF4C2-FF7B-301C-8D1C-3ED5E090B0CE> /usr/lib/system/libsystem_kernel.dylib
    0x7fff6b8b0000 -     0x7fff6b8fbfcb  libsystem_m.dylib (3147.50.1) <17570F46-566C-39FC-BEF6-635A355DD549> /usr/lib/system/libsystem_m.dylib
    0x7fff6b8fc000 -     0x7fff6b91bfff  libsystem_malloc.dylib (140.50.6) <7FD43735-9DDD-300E-8C4A-F909A74BDF49> /usr/lib/system/libsystem_malloc.dylib
    0x7fff6b91c000 -     0x7fff6ba4cff7  libsystem_network.dylib (1229.70.2) <5E86B2DE-9E15-3354-8714-4094ED5F698D> /usr/lib/system/libsystem_network.dylib
    0x7fff6ba4d000 -     0x7fff6ba57ffb  libsystem_networkextension.dylib (767.70.2) <9DC03712-552D-3AEE-9519-B5ED70980B70> /usr/lib/system/libsystem_networkextension.dylib
    0x7fff6ba58000 -     0x7fff6ba61ff3  libsystem_notify.dylib (172) <08012EC0-2CD2-34BE-BF93-E7F56491299A> /usr/lib/system/libsystem_notify.dylib
    0x7fff6ba62000 -     0x7fff6ba69ff7  libsystem_platform.dylib (161.50.1) <6355EE2D-5456-3CA8-A227-B96E8F1E2AF8> /usr/lib/system/libsystem_platform.dylib
    0x7fff6ba6a000 -     0x7fff6ba75fff  libsystem_pthread.dylib (301.50.1) <0E51CCBA-91F2-34E1-BF2A-FEEFD3D321E4> /usr/lib/system/libsystem_pthread.dylib
    0x7fff6ba76000 -     0x7fff6ba79fff  libsystem_sandbox.dylib (765.70.1) <553DFCC6-9D31-3B9C-AB7C-30F6F265786D> /usr/lib/system/libsystem_sandbox.dylib
    0x7fff6ba7a000 -     0x7fff6ba7bff3  libsystem_secinit.dylib (30) <DE8D14E8-A276-3FF8-AE13-77F7040F33C1> /usr/lib/system/libsystem_secinit.dylib
    0x7fff6ba7c000 -     0x7fff6ba83ff7  libsystem_symptoms.dylib (820.60.3) <441C6CA0-5711-3BB1-8420-DDAC3D5272E1> /usr/lib/system/libsystem_symptoms.dylib
    0x7fff6ba84000 -     0x7fff6ba97fff  libsystem_trace.dylib (829.70.1) <3A6CB706-8CA6-3616-8AFC-14AAD7FAF187> /usr/lib/system/libsystem_trace.dylib
    0x7fff6ba99000 -     0x7fff6ba9eff7  libunwind.dylib (35.3) <BEF3FB49-5604-3B5F-82B5-332B80023AC3> /usr/lib/system/libunwind.dylib
    0x7fff6ba9f000 -     0x7fff6baccff7  libxpc.dylib (1205.70.10) <903AB944-964B-3E73-89AE-A55F5424BD9A> /usr/lib/system/libxpc.dylib

External Modification Summary:
  Calls made by other processes targeting this process:
    task_for_pid: 0
    thread_create: 0
    thread_set_state: 0
  Calls made by this process:
    task_for_pid: 0
    thread_create: 0
    thread_set_state: 0
  Calls made by all processes on this machine:
    task_for_pid: 68537
    thread_create: 0
    thread_set_state: 5970

VM Region Summary:
ReadOnly portion of Libraries: Total=438.3M resident=0K(0%) swapped_out_or_unallocated=438.3M(100%)
Writable regions: Total=182.2M written=0K(0%) resident=0K(0%) swapped_out=0K(0%) unallocated=182.2M(100%)
 
                                VIRTUAL   REGION 
REGION TYPE                        SIZE    COUNT (non-coalesced) 
===========                     =======  ======= 
Activity Tracing                   256K        2 
CoreGraphics                         8K        2 
CoreUI image data                  148K        2 
CoreUI image file                  136K        4 
Dispatch continuations            16.0M        2 
Foundation                           4K        2 
IOKit                             15.5M        2 
Kernel Alloc Once                    8K        2 
MALLOC                           137.1M       39 
MALLOC guard page                   48K       10 
OpenGL GLSL                        256K        4 
STACK GUARD                       56.1M       18 
Stack                             16.1M       18 
VM_ALLOCATE                        396K       13 
__DATA                            39.7M      326 
__FONT_DATA                          4K        2 
__GLSLBUILTINS                    2588K        2 
__LINKEDIT                       202.1M       63 
__TEXT                           236.2M      314 
__UNICODE                          560K        2 
mapped file                      113.6M       13 
shared memory                      724K       14 
===========                     =======  ======= 
TOTAL                            837.4M      834 

Model: MacBookPro14,3, BootROM 185.0.0.0.0, 4 processors, Intel Core i7, 2.9 GHz, 16 GB, SMC 2.45f0
Graphics: Intel HD Graphics 630, Intel HD Graphics 630, Built-In
Graphics: Radeon Pro 560, Radeon Pro 560, PCIe
Memory Module: BANK 0/DIMM0, 8 GB, LPDDR3, 2133 MHz, 0x802C, 0x4D5435324C31473332443450472D30393320
Memory Module: BANK 1/DIMM0, 8 GB, LPDDR3, 2133 MHz, 0x802C, 0x4D5435324C31473332443450472D30393320
AirPort: spairport_wireless_card_type_airport_extreme (0x14E4, 0x173), Broadcom BCM43xx 1.0 (7.77.37.31.1a9)
Bluetooth: Version 6.0.7f12, 3 services, 18 devices, 1 incoming serial ports
Network Service: Wi-Fi, AirPort, en0
USB Device: USB 3.0 Bus
USB Device: iBridge
Thunderbolt Bus: MacBook Pro, Apple Inc., 33.1
Thunderbolt Bus: MacBook Pro, Apple Inc., 33.1
```

