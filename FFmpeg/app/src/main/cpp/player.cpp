#include <jni.h>
#include <string>
#include <android/log.h>
//#define LOGW(...) _android_log_print(ANDROID_LOG_WARN, "player jni", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"testff",__VA_ARGS__)
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

static double r2d(AVRational r) {
    return r.den == 0 || r.num == 0 ? 0 : r.num / r.den;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_kiven_heli_ffmpeg_FFmpegPlayer_open(JNIEnv *env, jclass type, jstring url_,
                                             jobject handle) {
    const char *url = env->GetStringUTFChars(url_, 0);
    LOGI("url : %s", url);
    //初始解封装
    av_register_all();
    //初始化网络
    avformat_network_init();
    //打开文件
    AVFormatContext *ac = NULL;
    //初始化解码器
    avcodec_register_all();
    int result = avformat_open_input(&ac, url, 0, 0);
//    int re = avformat_find_stream_info(ac, 0);
//    if (re == 0) {
//        LOGI("duration = %lld nbstream = %d", ac->duration, ac->nb_streams);
//    }
    if (result == 0) {
        LOGI("avformat_open_input %s success!", url);
        LOGI("duration = %lld nbstream = %d", ac->duration, ac->nb_streams);
        for (int i = 0; i < ac->nb_streams; ++i) {
            AVStream *avStream = ac->streams[i];
            if (avStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                LOGI("%d 这是一个视频", i);
                int fps = r2d(avStream->avg_frame_rate);
                LOGI("fps : %d width : %d height = %d codecid = %d pixFormat = %d",
                     fps ,
                     avStream->codecpar->width,
                     avStream->codecpar->height,
                     avStream->codecpar->codec_id,
                     avStream->codecpar->format);
            } else if (avStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                LOGI("%d 这是一个音频", i);
                LOGI("sampleTate = %d channels = %d format = %d",
                     avStream->codecpar->sample_rate,
                     avStream->codecpar->channels,
                     avStream->codecpar->format);
            }
        }
        int audioStream = av_find_best_stream(ac, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, -1);
        LOGI("audioStream id = %d", audioStream);
        int videoStream = av_find_best_stream(ac, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, -1);
        LOGI("videoStream id = %d", videoStream);

        //start 解码模块
        AVCodec *codec = avcodec_find_decoder(ac->streams[videoStream]->codecpar->codec_id);
        if(!codec) {
            LOGI("avcodec find failed");
            return true;
        }
        AVCodecContext *avCodecContext = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(avCodecContext, ac->streams[videoStream]->codecpar);
        avCodecContext->thread_count = 1;

        //打开解码器
        result = avcodec_open2(avCodecContext, codec, 0);
        if (result != 0) {
            LOGI("avcodec_open2 failed");
            return true;
        }


        AVCodec *acodec = avcodec_find_decoder(ac->streams[audioStream]->codecpar->codec_id);
        if(!acodec) {
            LOGI("avcodec find failed");
            return true;
        }
        AVCodecContext *aCodecCtx = avcodec_alloc_context3(acodec);
        avcodec_parameters_to_context(avCodecContext, ac->streams[videoStream]->codecpar);
        aCodecCtx->thread_count = 1;

        //打开解码器
        result = avcodec_open2(avCodecContext, acodec, 0);
        if (result != 0) {
            LOGI("acodec_open2 failed");
            return true;
        }
        //end   解码模块


        //读取视频流
        AVPacket *packet = av_packet_alloc();
        for(;;) {
            int result = av_read_frame(ac, packet);
            if (result != 0) {
                LOGI("已经读取到结尾");
                int pos = 5 * r2d(ac->streams[videoStream]->time_base);
                av_seek_frame(ac, videoStream, pos,AVSEEK_FLAG_BACKWARD|AVSEEK_FLAG_FRAME);
                continue;
            }
            LOGI("streadId = %d, size = %d, pts = %lld, flag = %d",
            packet->stream_index, packet->size, packet->pts, packet->flags);
            av_packet_unref(packet);
        }

        avformat_close_input(&ac);
    } else {
        LOGI("avformat_open_input failed: %s",av_err2str(result));
    }
    env->ReleaseStringUTFChars(url_, url);
    return true;
}


extern "C"
JNIEXPORT jstring JNICALL
Java_com_kiven_heli_ffmpeg_FFmpegPlayer_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    char info[10000] = {0};
    sprintf(info, "%s\n", avcodec_configuration());
    return env->NewStringUTF(info);
}
