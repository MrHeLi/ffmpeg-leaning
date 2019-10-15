# Android音视频框架总结

## 音视频采集

### 音频

#### Android 内置的应用程序

```java
Intent intent=new Intent(MediaStore.Audio.Media.RECORD_SOUND_ACTION);
startActivityForResult(intent,0); //通过startActivityForResult获取音频录制的结果的路径
```

#### MediaRecorder



#### AudioRecord



### 视频

#### Camera



#### Camera2

## 音视频播放

### 播放器

* 自带播放器：

  * dolby音频无声，或者干脆视频无法播放。
  * 通过intent打开的是系统自带播放器应用，例如华为手机的`com.huawei.himovie`、nexus的`com.google.android.apps.photos`
  * 不支持rtmp

  ```java
  Intent intent = new Intent(Intent.ACTION_VIEW);
  String path = "/sdcard/Movies/economics.mp4"; // 可以播放本地文件
  // String path = "http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4"; // 也可以播放http流
  Uri uri = Uri.parse(path);
  intent.setDataAndType(uri, "video/*");
  startActivity(intent);
  ```

* VideoView

  * 播放实际上依赖MediaPlayer，VideoView相当于将SurfaceView+MediaPlayer封装在一起，给开发者使用。

  ```java
  public class VideoView extends SurfaceView
          implements MediaPlayerControl, SubtitleController.Anchor {
              // All the stuff we need for playing and showing a video
      private SurfaceHolder mSurfaceHolder = null;
      private MediaPlayer mMediaPlayer = null;
      private MediaController mMediaController;
  }
  ```

  布局文件：

  ```java
  <VideoView
      android:id="@+id/vv_player"
      android:layout_width="match_parent"
      android:layout_height="match_parent"
      app:layout_constraintTop_toBottomOf="@id/bt_rtmp_video"
      app:layout_constraintBottom_toBottomOf="parent"/>
  ```

  播放代码：

  ```java
  String url = "http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4";
  VideoView videoView = findViewById(R.id.vv_player);
  videoView.setVideoPath(url);
  videoView.requestFocus();
  videoView.start();
  ```

* MediaPlayer
  * SurfaceView + MediaPlayer方案
  * 博客：[MediaPlayer源码分析](https://blog.csdn.net/qq_25333681/article/details/82056184)
* NuPlayer
  * 博客：[NuPlayer源码分析](https://blog.csdn.net/qq_25333681/article/details/90354268)

### 解封装

## 音频

### 解码

* MediaCodec

## 视频

### 解码

* MediaCodec

## 音视频同步