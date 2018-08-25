# MediaPlayer

前记，本文涉及java、c/c++、JNI、智能指针等知识，还是有一定门槛的。不过，如果你有一颗坚持不懈心，本文也非常适合阅读，除了java之外的知识，都有解释或者浅显易懂的外链，不是特别小白的程序员都能看懂。

## MediaPlayer示例代码

```java
public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback, MediaPlayer.OnPreparedListener {
    SurfaceView surfaceView;
    SurfaceHolder holder;
    String path = "/sdcard/Movies/skateboard.mp4";
    MediaPlayer player;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        surfaceView = new SurfaceView(this);
        setContentView(surfaceView);
        holder = surfaceView.getHolder();
        holder.addCallback(this);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        player = new MediaPlayer();
        try {
            player.setDataSource(path);
            player.setDisplay(holder);//如果这个参数为NULL的话，那么就只有声音播放
            /**
             * 阻塞式准备：
             * player.prepare();
             * player.start();
             */
            player.prepareAsync();//异步准备， 两种准备方式二选其一即可
            player.setOnPreparedListener(this);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
    @Override
    public void onPrepared(MediaPlayer mp) {
        player.start();
    }
}
```

此外在不使用MediaPlayer的时候，记得释放：

```java
player.release();//一般在Surface的回调函数surfaceDestroyed中释放
```

> SurfaceView 的三个重要的回调函数分别为：
>
> * public void surfaceCreated(SurfaceHolder holder)；
> * public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)；
> * public void surfaceDestroyed(SurfaceHolder holder)；

我们先来看看MediaPlayer创建对象时都做了啥。

## new MediaPlayer()

我们在看MediaPlayer的构造之前，先来看看它的静态代码块：

```java
static {
    System.loadLibrary("media_jni"); //加载libmedia_jni.so库
    native_init();
}
private static native final void native_init();
```

首先加载了一个libmedia_jni.so库，这个库不仅包含了MediaPlayer的代码，还有其它媒体播放相关的基本代码，以及相关JNI代码。关于这段代码，能说的不多，但很重要。

接下来我们去jni中对应的本地方法android_media_MediaPlayer_native_init(JNIEnv *env)：

```c++
void android_media_MediaPlayer_native_init(JNIEnv *env)
{
    jclass clazz;//类的句柄
    clazz = env->FindClass("android/media/MediaPlayer");//通过jni的env句柄调用Java层的MediaPlayer对象
    fields.context = env->GetFieldID(clazz, "mNativeContext", "J");//获取MediaPlayer对象中的mNativeContext字段
    fields.post_event = env->GetStaticMethodID(clazz, "postEventFromNative",
                                               "(Ljava/lang/Object;IIILjava/lang/Object;)V");//调用MediaPlayer中的postEventFromNative	
    fields.surface_texture = env->GetFieldID(clazz, "mNativeSurfaceTexture", "J");
    env->DeleteLocalRef(clazz);
    
    clazz = env->FindClass("android/net/ProxyInfo");
    if (clazz == NULL) {
        return;
    }
    fields.proxyConfigGetHost =
       env->GetMethodID(clazz, "getHost", "()Ljava/lang/String;");
    fields.proxyConfigGetPort =
        env->GetMethodID(clazz, "getPort", "()I");
    fields.proxyConfigGetExclusionList =
        env->GetMethodID(clazz, "getExclusionListAsString", "()Ljava/lang/String;");
    env->DeleteLocalRef(clazz);
}
```

> 对应代码路径为：Z:\work\20170630\platform\Nougat\frameworks\base\media\jni\android_media_MediaPlayer.cpp

如果对JNI熟悉的话（如果不熟悉可以看一下这篇博文[Andorid JNI 详解](https://blog.csdn.net/qq_25333681/article/details/80919590)中的***访问对象的变量和函数回调***），这部分代码倒也简单，就是给一个叫`fields`的结构体赋值，赋值分为两个部分：

1. 获取java层中MediaPlayer的相关字段和函数的ID：
   * FieldID :   mNativeContext
   * FiledID： mNativeSurfaceTexture   这个厉害了，视频播放强相关啊。
   * MethodID：void postEventFromNative(Object, int, int ,int ,Object )   看名字应该是用来和java层事件传递的函数。
2. 获取java层网络相关ProxyInfo类中的相关函数ID，看看，网络部分MediaPlayer也会负责。
   * MethodID：String getHost()
   * MethodID：int getPort()
   * MethodID：String getExclusionListAsString()

这些初始化后的ID都赋值给了一个叫`fields`的结构体，我们来看看它：

```c++
struct fields_t {
    jfieldID    context;
    jfieldID    surface_texture;
    jmethodID   post_event;
//  MediaPlayer相关ID
//----------------------------------------
//  网络相关ID
    jmethodID   proxyConfigGetHost;
    jmethodID   proxyConfigGetPort;
    jmethodID   proxyConfigGetExclusionList;
};
static fields_t fields;
```

很简单，就这么点儿东西。

看完静态函数，接下来再看看构造函数：

```java
public MediaPlayer() {
    //...
    /* Native setup requires a weak reference to our object.
     * It's easier to create it here than in C++.
     */
    native_setup(new WeakReference<MediaPlayer>(this));
}
private native final void native_setup(Object mediaplayer_this);
```

执行了本地代码：

> MediaPlayer对应的jni源码路径：\frameworks\base\media\jni\android_media_MediaPlayer.cpp

```c++
static void
android_media_MediaPlayer_native_setup(JNIEnv *env, jobject thiz, jobject weak_this)
{
    ALOGV("native_setup");
    sp<MediaPlayer> mp = new MediaPlayer();//在C++层创建一个MediaPlayer对象
    if (mp == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
        return;
    }

    // 创建JNIMediaPlayerListener监听器，并设置给MediaPlayer
    sp<JNIMediaPlayerListener> listener = new JNIMediaPlayerListener(env, thiz, weak_this);
    mp->setListener(listener);

    // 将C++中的MediaPlayer设置给java对象
    setMediaPlayer(env, thiz, mp);
}
static sp<MediaPlayer> setMediaPlayer(JNIEnv* env, jobject thiz, const sp<MediaPlayer>& player)
{
    Mutex::Autolock l(sLock);
    //通过JNI获取java层MediaPlayer中的mNativeContext字段的值，并把它强转为一个类指针
    sp<MediaPlayer> old = (MediaPlayer*)env->GetLongField(thiz, fields.context);
    if (player.get()) {//判断MediaPlayer如果为空，就让智能指针的引用计数+1
        player->incStrong((void*)setMediaPlayer);
    }
    if (old != 0) {//如果mNativeContext字段的值已经存在过了，就让智能指针的引用计数-1
        old->decStrong((void*)setMediaPlayer);
    }
    //通过JNI将C++层创建的MediaPlayer的指针，设置给java层MediaPlayer的mNativeContext字段
    env->SetLongField(thiz, fields.context, (jlong)player.get());
    return old;
}
```

整个构造函数中的逻辑也比较简单，在C++层创建一个C++ 的MediaPlayer对象，并将这个对象的指针通过JNI设置给java层MediaPlayer对象的mNativeContext。

> mNativeContext在java中是一个long类型，对应8个字节，和Linux平台的C++指针表达的内存大小一致，正好可以存下。
>
> C++层的MediaPlayer定义路径：/frameworks/av/media/libmedia/mediaplayer.cpp

### 小结一下：

1. MediaPlayer在构造函数执行之前，会首先执行静态代码块中加载媒体库，并执行本地`native_init()`函数。`native_init()`函数主要的作用是初始化`fields_t`结构体，在这个结构体中保存一系列相关的字段ID和函数ID。
2. 接着在MediaPlayer的构造函数中，通过本地代码和JNI调用，创建了一个C++层的MediaPlayer对象，并将这个对象的指针设置给了Java层MediaPlayer对象中的mNativeContext字段。

MediaPlayer初始化的部分，差不多就这样。

## setDataSource()

> MediaPlayer的源码路径：\frameworks\base\media\java\android\media\MediaPlayer.java

首先来看看`player.setDataSource(path);`做了啥

```java
public void setDataSource(String path)
    throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
    setDataSource(path, null, null);//调用了重载函数
}
```

继续跟进：

```java
private void setDataSource(String path, String[] keys, String[] values)
    throws IOException, IllegalArgumentException, SecurityException, IllegalStateException {
    final Uri uri = Uri.parse(path);
    //如果path为本地文件，函数返回值为null，如果是网络地址，返回值为"http"
    final String scheme = uri.getScheme();
    if ("file".equals(scheme)) {
        path = uri.getPath();
    } else if (scheme != null) {//如果是网络地址，调用本地方法nativeSetDataSource
        // handle non-file sources
        nativeSetDataSource(
            MediaHTTPService.createHttpServiceBinderIfNecessary(path),
            path,
            keys,
            values);
        return;
    }
    //后面的处理path为本地文件的情况
    final File file = new File(path);
    if (file.exists()) {
        FileInputStream is = new FileInputStream(file);
        FileDescriptor fd = is.getFD();//得到文件标识符
        setDataSource(fd);
        is.close();
    } else {
        throw new IOException("setDataSource failed.");
    }
}
```

我们先来分析本地文件的情况：`setDataSource(fd)`：

```java
public void setDataSource(FileDescriptor fd)
    throws IOException, IllegalArgumentException, IllegalStateException {
    // intentionally less than LONG_MAX
    setDataSource(fd, 0, 0x7ffffffffffffffL);
}
```

然后一路跟进，调用链如下：

```java
public void setDataSource(FileDescriptor fd, long offset, long length);
private native void _setDataSource(FileDescriptor fd, long offset, long length)
```

代码走读到这里，出现了一个native方法，显然需要去看看MediaPlayer对应的c++代码了。显然，根据jni的语法，MediaPlayer的包名为`android.media.MediaPlayer`我们应该去找一个名称为：`android_media_MediaPlayer_setDataSource`的接口。

> Android对应的JNI的源码路径：\frameworks\base\media\jni
>
> MediaPlayer对应的jni源码路径：\frameworks\base\media\jni\android_media_MediaPlayer.cpp

然而，在android_media_MediaPlayer.cpp中搜索，并不能找到一个叫`android_media_MediaPlayer_setDataSource`的接口。搜索一下**_setDataSource**可以发现如下代码：

```c++
static const JNINativeMethod gMethods[] = {
    //将java中的_setDataSource 映射为C++中的android_media_MediaPlayer_setDataSourceFD函数
    {"_setDataSource",      "(Ljava/io/FileDescriptor;JJ)V",    (void *)android_media_MediaPlayer_setDataSourceFD}
}

```

浓浓的JNI风格代码，熟悉JNI的同学肯定知道。不熟悉的同志可以参看[Andorid JNI 详解](https://blog.csdn.net/qq_25333681/article/details/80919590)。这段代码的主要作用是将java的代码映射为C++中的代码，如代码片段中的注释，我们应该继续去看`android_media_MediaPlayer_setDataSourceFD`函数。

```c++
static void
android_media_MediaPlayer_setDataSourceFD(JNIEnv *env, jobject thiz, jobject fileDescriptor, jlong offset, jlong length)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    if (fileDescriptor == NULL) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return;
    }
    int fd = jniGetFDFromFileDescriptor(env, fileDescriptor);
    process_media_player_call( env, thiz, mp->setDataSource(fd, offset, length), "java/io/IOException", "setDataSourceFD failed." );
}
```

看起来函数一开始就创建了一个MediaPlayer对象，并且用弱智针(如果不知道弱智针为何物，可以[戳这里](https://blog.csdn.net/qq_25333681/article/details/81516781))来管理。但是想想，不大可能吧，我在java层不是已经创建了一个MediaPlayer对象吗，为啥c++层还去创建一个？有疑问，看代码：

```c++
static sp<MediaPlayer> getMediaPlayer(JNIEnv* env, jobject thiz)
{
    MediaPlayer* const p = (MediaPlayer*)env->GetLongField(thiz, fields.context);
    return sp<MediaPlayer>(p);
}
```

fields.context 是什么？还记得在new MediaPlayer()的时候做的工作吗？执行本地`native_init()`函数。`native_init()`函数主要的作用是初始化`fields_t`结构体（如果忘了可以回去看看）。而fields.context 的值便是在`native_init()`调用时赋值的，回顾一下代码：

```c++
fields.context = env->GetFieldID(clazz, "mNativeContext", "J");//获取MediaPlayer对象中的mNativeContext字段
```

fields.context记录了java层的mNativeContext的字段ID，下面是Java层代码：

```java
private long mNativeContext; // accessed by native methods
```

扯远了，回来看看`getMediaPlayer`函数。函数体中的代码还挺奇怪的，`GetLongField(thiz, fields.context);`这段JNI函数返回是java层中一个long类型变量`mNativeContext`的地址，把它强转成了一个MediaPlayer指针，如果你还记得前面的内容，我想你肯定知道，`mNativeContext`的值是在什么时候被设置的。

总结一下`getMediaPlayer`这个函数，就是将Java层的`mNativeContext`的地址值拿到，函数调用出栈时返回给调用者。

回到`android_media_MediaPlayer_setDataSourceFD`调用，`getMediaPlayer`给mp赋值后，判断调用是否成功，不成功抛出非法状态异常，程序结束。

我们显现不想让程序就此结束，那么继续往下：

```c++
int fd = jniGetFDFromFileDescriptor(env, fileDescriptor);
```

该函数的主要作用是通过JNI接口获取java.io.FileDescriptor类中的descriptor字段。这个代码比较难找，至少单纯的在Android源码中是找不到的，我这边零零总总搜罗了一些信息。jniGetFDFromFileDescriptor 是在/[libnativehelper](http://androidxref.com/8.0.0_r4/xref/libnativehelper/)/[include](http://androidxref.com/8.0.0_r4/xref/libnativehelper/include/)/[nativehelper](http://androidxref.com/8.0.0_r4/xref/libnativehelper/include/nativehelper/)/[JNIHelp.h](http://androidxref.com/8.0.0_r4/xref/libnativehelper/include/nativehelper/JNIHelp.h) 中申明：

```c++
/*
 * Sets the int fd in a java.io.FileDescriptor.
*/
void jniSetFileDescriptorOfFD(C_JNIEnv* env, jobject fileDescriptor, int value);
```

所以，可以再各种源码中调用。但它的定义是在https://android.googlesource.com/platform/libcore/+/2fb02ef3025449e24e756a7f645ea6eab7a1fd4f/luni/src/main/native/java_io_FileDescriptor.c这个文件中。

```c
/* 
 * For JNIHelp.c
 * Get an int file descriptor from a java.io.FileDescriptor
 */
int jniGetFDFromFileDescriptor (JNIEnv* env, jobject fileDescriptor) {
    /* should already be initialized if it's an actual FileDescriptor */
    assert(fileDescriptor != NULL);
    assert(gCachedFields.clazz != NULL);
    return getFd(env, fileDescriptor);
}
/*
 * Internal helper function.
 * Get the file descriptor.
 */
static inline int getFd(JNIEnv* env, jobject obj)
{
    return (*env)->GetIntField(env, obj, gCachedFields.descriptor);
}
```

根据这些定义可以看出，确实，该函数的作用，只是获取了java.io.FileDescriptor类中的descriptor字段，这个字段的作用大概是一个文件的描述符（唯一性），这个字段值的设置也是在java_io_FileDescriptor.c中设置，这里就不展开了，有兴趣的可以自己探索一番。

扯远了，回到正题，接着看最后一段函数调用`process_media_player_call`。

```c++
// If exception is NULL and opStatus is not OK, this method sends an error
// event to the client application; otherwise, if exception is not NULL and
// opStatus is not OK, this method throws the given exception to the client
// application.
static void process_media_player_call(JNIEnv *env, jobject thiz, status_t opStatus, const char* exception, const char *message)
{
    if (exception == NULL) {  // Don't throw exception. Instead, send an event.
        if (opStatus != (status_t) OK) {
            sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
            if (mp != 0) mp->notify(MEDIA_ERROR, opStatus, 0);
        }
    } else {  // Throw exception!
        if ( opStatus == (status_t) INVALID_OPERATION ) {
            jniThrowException(env, "java/lang/IllegalStateException", NULL);
        } else if ( opStatus == (status_t) BAD_VALUE ) {
            jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        } else if ( opStatus == (status_t) PERMISSION_DENIED ) {
            jniThrowException(env, "java/lang/SecurityException", NULL);
        } else if ( opStatus != (status_t) OK ) {
            if (strlen(message) > 230) {
               // if the message is too long, don't bother displaying the status code
               jniThrowException( env, exception, message);
            } else {
               char msg[256];
                // append the status code to the message
               sprintf(msg, "%s: status=0x%X", message, opStatus);
               jniThrowException( env, exception, msg);
            }
        }
    }
}
```

process_media_player_call函数主要将`mp->setDataSource(fd, offset, length)`的返回值作出判断，如果设置成功，那么就通知MediaPlayer设置成功，如果失败则抛出对应异常。

具体看看`mp->setDataSource(fd, offset, length)`，这名字看起来像是关键调用了：

> 代码路径： /frameworks/av/media/libmedia/mediaplayer.cpp

```c++
status_t MediaPlayer::setDataSource(int fd, int64_t offset, int64_t length)
{
    ALOGV("setDataSource(%d, %" PRId64 ", %" PRId64 ")", fd, offset, length);
    status_t err = UNKNOWN_ERROR;
    // 获取 MediaPlayerService 接口
    const sp<IMediaPlayerService> service(getMediaPlayerService());
    if (service != 0) {
        // 获取 MediaPlayer 接口
        sp<IMediaPlayer> player(service->create(this, mAudioSessionId));
        // 设置数据源
        if ((NO_ERROR != doSetRetransmitEndpoint(player)) ||
            (NO_ERROR != player->setDataSource(fd, offset, length))) {
            player.clear();
        }
        err = attachNewPlayer(player);//根据当前的一些播放器状态判断播放器是否创建并设置data_source成功
    }
    return err;
}
```

这段代码是真真的关键了，前方高能，喝杯水冷静一下吧。

继续：先来看看setDataSource获取MediaPlayerService的代码，首先执行的是getMediaPlayerService：

> 下面的代码位于IMediaDeathNotifier.cpp中，代码路径为：/frameworks/av/media/libmedia/IMediaDeathNotifier.cpp

```c++
// establish binder interface to MediaPlayerService
/*static*/const sp<IMediaPlayerService> IMediaDeathNotifier::getMediaPlayerService()
{
    ALOGV("getMediaPlayerService");
    Mutex::Autolock _l(sServiceLock);
    if (sMediaPlayerService == 0) {
        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder;
        do {
            binder = sm->getService(String16("media.player"));
            if (binder != 0) {
                break;
            }
            ALOGW("Media player service not published, waiting...");
            usleep(500000); // 0.5 s
        } while (true);

        if (sDeathNotifier == NULL) {
            sDeathNotifier = new DeathNotifier();
        }
        binder->linkToDeath(sDeathNotifier);
        sMediaPlayerService = interface_cast<IMediaPlayerService>(binder);
    }
    ALOGE_IF(sMediaPlayerService == 0, "no media player service!?");
    return sMediaPlayerService;
}
```

这段代码中，涉及了binder操作，都说aidl是典型的binder的应用。所以，来稍回忆一下，Android中的aidl是如何使用的：

1. 在A工程（作为服务端）中创建一个MyService.aidl文件，手动编译生成一个对应的java文件：MyService.java。
2. 在MyService.java文件中实现你的服务，其中包含一个有一个IBinder对象。
3. 将刚才的MyService.aidl文件拷贝到client端，也就是B工程中使用。
4. 在B工程中，通过bind启动服务，通过参数中的ServiceConnection的onServiceConnected回调，获取到Server端的IBinder对象，这个函数通过asInterface接口，就可以转换成MyService对象，于是，我们就可以调用这个对象的各种函数了。

回忆了上层的binder使用，再来看看本地的binder使用，也是类似，只是表达方式不同。

通过defaultServiceManager();函数返回的IServiceManager对象，通过"media.player"参数，获取了一个服务的binder对象。这一段逻辑对应了aidl中的通过onServiceConnected函数获取binder的部分。而asInterface函数类似的操作是`interface_cast<IMediaPlayerService>(binder)`，他返回了一个MediaPlayerService的对象，这段代码实体在/frameworks/native/include/binder/IInterface.h中：

```C++
template<typename INTERFACE>
inline sp<INTERFACE> interface_cast(const sp<IBinder>& obj)
{
    return INTERFACE::asInterface(obj);
}
```

可以看到，该函数直接反回了一个INTERFACE（在这里就是IMediaPlayerService）类型的对象。

### 总结一下getMediaPlayerService：

通过defaultServiceManager调用获得ServiceManager，再通过**media.player**参数，获取了MediaPlayer服务的binder对象，接着通过interface_cast将binder转为MediaPlayerService并返回。

继续看`service->create(this, mAudioSessionId)`：

这里的service便是刚才通过IPC机制获取的MediaPlayerService，当前代码作为client端，通过IPC远程调用MediaPlayerService中的create函数，来看看具体实现：

```c++
sp<IMediaPlayer> MediaPlayerService::create(const sp<IMediaPlayerClient>& client,
        audio_session_t audioSessionId)
{
    pid_t pid = IPCThreadState::self()->getCallingPid();
    int32_t connId = android_atomic_inc(&mNextConnId);

    sp<Client> c = new Client(
            this, pid, connId, client, audioSessionId,
            IPCThreadState::self()->getCallingUid());

    ALOGV("Create new client(%d) from pid %d, uid %d, ", connId, pid,
         IPCThreadState::self()->getCallingUid());

    wp<Client> w = c;
    {
        Mutex::Autolock lock(mLock);
        mClients.add(w);
    }
    return c;
}
```

整个函数其实就是把各种相关参数（服务，pid，audioSessionId...)存到一个Client类中，方便以后Client端通过IPC调用，调用时，参数列表中的client传递的实参是MediaPlayer的this指针，之所以这么传递是因为它继承了BnMediaPlayerClient 。简单看看相关定义：

```c++
MediaPlayerService::Client::Client(
        const sp<MediaPlayerService>& service, pid_t pid,
        int32_t connId, const sp<IMediaPlayerClient>& client,
        audio_session_t audioSessionId, uid_t uid)
{
    ALOGV("Client(%d) constructor", connId);
    mPid = pid;
    mConnId = connId;
    mService = service;
    mClient = client;
    mLoop = false;
    mStatus = NO_INIT;
    mAudioSessionId = audioSessionId;
    mUid = uid;
    mRetransmitEndpointValid = false;
    mAudioAttributes = NULL;

#if CALLBACK_ANTAGONIZER
    ALOGD("create Antagonizer");
    mAntagonizer = new Antagonizer(notify, this);
#endif
}
class Client : public BnMediaPlayer {
    mutable     Mutex                       mLock;
                sp<MediaPlayerBase>         mPlayer;
                sp<MediaPlayerService>      mService;
                sp<IMediaPlayerClient>      mClient;
                sp<AudioOutput>             mAudioOutput;
                pid_t                       mPid;
                status_t                    mStatus;
                bool                        mLoop;
                int32_t                     mConnId;
                audio_session_t             mAudioSessionId;
                audio_attributes_t *        mAudioAttributes;
                uid_t                       mUid;
                sp<ANativeWindow>           mConnectedWindow;
                sp<IBinder>                 mConnectedWindowBinder;
                struct sockaddr_in          mRetransmitEndpoint;
                bool                        mRetransmitEndpointValid;
                sp<Client>                  mNextClient;
}; // Client                    
```

创建完Client之后，将它的强指针返回给调用者，返回类型是sp<IMediaPlayer>。这样返回显然他们之间是有联系的。代码中我们可以看到，Client继承了BnMediaPlayer， 而BnMediaPlayer有继承了BnInterface<IMediaPlayer>：

```c++
class BnMediaPlayer: public BnInterface<IMediaPlayer>
{
 //...
};
```

好了，花了大量时间获取 MediaPlayerService 接口、 获取 MediaPlayer 接口，到了设置真正设置数据源的地方了。也就是这段`player->setDataSource(fd, offset, length)`调用。看看具体实现：

```c++
status_t MediaPlayerService::Client::setDataSource(int fd, int64_t offset, int64_t length)
{
    //.... 大量log和异常参数检查
	//获取播放器类型
    player_type playerType = MediaPlayerFactory::getPlayerType(this, fd, offset, length);
    //创建播放器
    sp<MediaPlayerBase> p = setDataSource_pre(playerType);
    if (p == NULL) {
        return NO_INIT;
    }
    // 为播放器设置数据源
    setDataSource_post(p, p->setDataSource(fd, offset, length));
    return mStatus;
}
```

逻辑，主要分为三个部分：

1. 获取播放器类型
2. 根据播放器类型创建播放器
3. 调用播放器接口，设置数据源

我们分开来分析，首先来第一个部分，获取播放器类型：

```c++

#define GET_PLAYER_TYPE_IMPL(a...)                      \
    Mutex::Autolock lock_(&sLock);                      \
                                                        \
    player_type ret = STAGEFRIGHT_PLAYER;               \   //mo
    float bestScore = 0.0;                              \
                                                        \
    for (size_t i = 0; i < sFactoryMap.size(); ++i) {   \
                                                        \
        IFactory* v = sFactoryMap.valueAt(i);           \
        float thisScore;                                \
        CHECK(v != NULL);                               \
        thisScore = v->scoreFactory(a, bestScore);      \
        if (thisScore > bestScore) {                    \
            ret = sFactoryMap.keyAt(i);                 \
            bestScore = thisScore;                      \
        }                                               \
    }                                                   \
                                                        \
    if (0.0 == bestScore) {                             \
        ret = getDefaultPlayerType();                   \
    }                                                   \
                                                        \
    return ret;

player_type MediaPlayerFactory::getPlayerType(const sp<IMediaPlayer>& client,
                                              int fd,
                                              int64_t offset,
                                              int64_t length) {
    GET_PLAYER_TYPE_IMPL(client, fd, offset, length);
}
#undef GET_PLAYER_TYPE_IMPL
```

上面这段代码就是传说中的打分机制，分析代码之前，我们先看看播放器类型都有哪些，定义在MediaPlayerInterface.h中：

```c++
enum player_type {
    STAGEFRIGHT_PLAYER = 3,
    NU_PLAYER = 4,
    // Test players are available only in the 'test' and 'eng' builds.
    // The shared library with the test player is passed passed as an
    // argument to the 'test:' url in the setDataSource call.
    TEST_PLAYER = 5,
};
```

这是AndroidO的源码，所以我们看到的只有三种播放器类型，如果提及，你一定会好奇，为什么他们的值是从3开始，而不是从0或者1。其实在早期版本，播放器类型还有PV_PLAYER = 1,    SONIVOX_PLAYER = 2两种，但在后续的版本中被Google无情抛弃，当前，STAGEFRIGHT_PLAYER主要用于DRM播放，NU_PLAYER用于网络和文件播放。

回到打分机制的代码，来看看sFactoryMap是个什么：

```c++
typedef KeyedVector<player_type, IFactory*> tFactoryMap;
static tFactoryMap sFactoryMap;
```

可以看出，sFactoryMap就是一个以player_type为值，IFactory指针为值得键值对容器。看看它是如何初始化的：

```c++
status_t MediaPlayerFactory::registerFactory_l(IFactory* factory, player_type type) {
    //...
    if (sFactoryMap.add(type, factory) < 0) {
        ALOGE("Failed to register MediaPlayerFactory of type %d, failed to add"
              " to map.", type);
        return UNKNOWN_ERROR;
    }
    return OK;
}
void MediaPlayerFactory::registerBuiltinFactories() {
    Mutex::Autolock lock_(&sLock);
    if (sInitComplete)
        return;
    registerFactory_l(new NuPlayerFactory(), NU_PLAYER);
    registerFactory_l(new TestPlayerFactory(), TEST_PLAYER);
    sInitComplete = true;
}
```

在registerBuiltinFactories函数中，分别创建NU_PLAYER、TEST_PLAYER对应的工厂类，并通过registerFactory_l调用存入sFactoryMap中。在这里，我们惊奇的发现，我们的播放器类型有三种，而在这里只创建了两种播放器类型的工厂类，理应还有一个StagefrightPlayerFactory类才对。结合前面提到STAGEFRIGHT_PLAYER主要用于DRM播放，我们可以大胆猜测一下，这个播放器类型应该是Google交给厂商去定制的播放器类型，以便厂商支持DRM视频播放，所以在这里就没有初始化。

接着看看registerBuiltinFactories的调用时机：

```c++
MediaPlayerService::MediaPlayerService()
{
    ALOGV("MediaPlayerService created");
    mNextConnId = 1;

    MediaPlayerFactory::registerBuiltinFactories();
}
```

可以看到，是在MediaPlayerService的构造函数中调用的。

好了，让我们回到打分机制的代码中，关键之处在于不同播放器分数的获得，也就是scoreFactory，其实，在这本次调用，不管是NuPlayerFactory还是TestPlayerFactory的scoreFactory都是调用的它们的父类MediaPlayerFactory:: IFactory的scoreFactory，返回值都是0.0：

```c++
virtual float scoreFactory(const sp<IMediaPlayer>& /*client*/,
                           int /*fd*/,
                           int64_t /*offset*/,
                           int64_t /*length*/,
                           float /*curScore*/) { return 0.0; }
```

当然，NuPlayerFactory和TestPlayerFactory还有有重写scoreFactory函数，将会在其它场景下调用。代码如下：

```c++
class NuPlayerFactory : public MediaPlayerFactory::IFactory {
  public:
    virtual float scoreFactory(const sp<IMediaPlayer>& /*client*/,
                               const char* url,
                               float curScore) {
        static const float kOurScore = 0.8;

        if (kOurScore <= curScore)
            return 0.0;

        if (!strncasecmp("http://", url, 7)
                || !strncasecmp("https://", url, 8)
                || !strncasecmp("file://", url, 7)) {
            size_t len = strlen(url);
            if (len >= 5 && !strcasecmp(".m3u8", &url[len - 5])) {
                return kOurScore;
            }

            if (strstr(url,"m3u8")) {
                return kOurScore;
            }

            if ((len >= 4 && !strcasecmp(".sdp", &url[len - 4])) || strstr(url, ".sdp?")) {
                return kOurScore;
            }
        }

        if (!strncasecmp("rtsp://", url, 7)) {
            return kOurScore;
        }

        return 0.0;
    }

    virtual float scoreFactory(const sp<IMediaPlayer>& /*client*/,const sp<IStreamSource>& /*source*/, float /*curScore*/) {
        return 1.0;
    }

    virtual float scoreFactory(const sp<IMediaPlayer>& /*client*/, const sp<DataSource>& /*source*/,float /*curScore*/) {
        return 1.0;
    }
};

class TestPlayerFactory : public MediaPlayerFactory::IFactory {
  public:
    virtual float scoreFactory(const sp<IMediaPlayer>& /*client*/,
                               const char* url,
                               float /*curScore*/) {
        if (TestPlayerStub::canBeUsed(url)) {
            return 1.0;
        }

        return 0.0;
    }
};
```

综上，scoreFactory函数返回的值是0.0，代入到getPlayerType函数中，可得，最终的返回值是getDefaultPlayerType()的返回值：

```c++
static player_type getDefaultPlayerType() {
    return NU_PLAYER;
}
```

### 总结一下：获取播放器类型

MediaPlayerService::Client在获取播放器类型时，会根据scoreFactory（姑且称为打分机制）返回的值，返回对应的播放器类型给调用者。本例中的调用属于默认情况，返回NU_PLAYER；

我们要干嘛来着？经过打分机制的长篇大论之后，我都快忘了初衷了。之前我们说到setDataSource，来回顾一下：

```c++
status_t MediaPlayerService::Client::setDataSource(int fd, int64_t offset, int64_t length)
{
    //.... 大量log和异常参数检查
	//获取播放器类型
    player_type playerType = MediaPlayerFactory::getPlayerType(this, fd, offset, length);
    //创建播放器
    sp<MediaPlayerBase> p = setDataSource_pre(playerType);
    if (p == NULL) {
        return NO_INIT;
    }
    // 为播放器设置数据源
    setDataSource_post(p, p->setDataSource(fd, offset, length));
    return mStatus;
}
```

刚才，我们已经获取的播放器类型为：NU_PLAYER

接着开始根据获取的播放器类型，开始真正的创建我们的播放器，也就是这段调用：setDataSource_pre

```c++
sp<MediaPlayerBase> MediaPlayerService::Client::setDataSource_pre(
        player_type playerType)
{
    //...
    // create the right type of player
    sp<MediaPlayerBase> p = createPlayer(playerType);
    if (p == NULL) {
        return p;
    }
    //...
    return p;
}
sp<MediaPlayerBase> MediaPlayerService::Client::createPlayer(player_type playerType)
{
    // determine if we have the right player type
    sp<MediaPlayerBase> p = mPlayer;//mPlayer sp<MediaPlayerBase>         mPlayer;
    if ((p != NULL) && (p->playerType() != playerType)) {
        ALOGV("delete player");
        p.clear();
    }
    if (p == NULL) {
        p = MediaPlayerFactory::createPlayer(playerType, this, notify, mPid);
    }

    if (p != NULL) {
        p->setUID(mUid);
    }
    return p;
}
sp<MediaPlayerBase> MediaPlayerFactory::createPlayer(
        player_type playerType,
        void* cookie,
        notify_callback_f notifyFunc,
        pid_t pid) {
    sp<MediaPlayerBase> p;
    IFactory* factory;
    
    p = factory->createPlayer(pid);

    return p;
}
virtual sp<MediaPlayerBase> createPlayer(pid_t pid) {
    ALOGV(" create NuPlayer");
    return new NuPlayerDriver(pid);
}
NuPlayerDriver::NuPlayerDriver(pid_t pid):mPlayer(new NuPlayer(pid)){
}
```

这段代码逻辑比较简单：

* MediaPlayerService::Client::setDataSource_pre:函数中直接调用createPlayer函数根据传递的播放器类型，创建播放器。包括media.extractor 、media.codec在内的多个服务的获取（代码已略）。
* MediaPlayerService::Client::createPlayer：函数中的mPlayer存放的是已有的播放器，接下来检查这个已有的播放器类型是否和参数中指定的播放器类型一直，如果一致则直接返回已有播放器，如果不一致重新创建。假定本次操作为MediaPlayer的第一次初始化，mPlayer的值为NULL，函数调用会走向MediaPlayerFactory::createPlayer。
* MediaPlayerFactory::createPlayer：该函数中做了很多事情，类型判断啊，设置notify回调啥的。核心为使用指定的播放器类型的createPlayer函数创建一个NuPlayerDriver。
* new NuPlayerDriver(pid)：接着在NuPlayerDriver构造函数的初始化列表中，我们看到创建了一个NuPlayer对象（播放器），并将这个对象设置给了mPlayer。至此，播放器的创建流程结束。

看看最后一段逻辑，为播放器设置数据源：

应该也是最重要的一段：p->setDataSource(fd, offset, length)，从前面我们可以知道，到这里创建的播放器为：NuPlayerDriver，应该调用NuPlayerDriver的setDataSource：

```c++
status_t NuPlayerDriver::setDataSource(int fd, int64_t offset, int64_t length) {
    ALOGV("setDataSource(%p) file(%d)", this, fd);
    Mutex::Autolock autoLock(mLock);

    if (mState != STATE_IDLE) {
        return INVALID_OPERATION;
    }

    mState = STATE_SET_DATASOURCE_PENDING;
	//前面提到过了，NuPlayerDriver在构造函数的初始化列表中创建了NuPlayer，并设置给mPlayer，
    //所以这里的调用为NuPlayer::setDataSourceAsync
    mPlayer->setDataSourceAsync(fd, offset, length);

    while (mState == STATE_SET_DATASOURCE_PENDING) {
        mCondition.wait(mLock);
    }

    return mAsyncResult;
}

void NuPlayer::setDataSourceAsync(int fd, int64_t offset, int64_t length) {
    sp<AMessage> msg = new AMessage(kWhatSetDataSource, this);
    sp<AMessage> notify = new AMessage(kWhatSourceNotify, this);
    //创建了一个GenericSource对象，调用它的setDataSource函数设置数据源
    sp<GenericSource> source = new GenericSource(notify, mUIDValid, mUID);
    status_t err = source->setDataSource(fd, offset, length);
}
```

GenericSource是NuPlayer播放器中的一个关于资源的对象，本文就不继续跟踪了。至此，MediaPlayer的setDataSource便说完了。

是时候总结一波了：

### setDataSource总结：

整个设置数据源的过程简单的来说就是：

1. 通过IPC机制，获取MediaPlayerService服务。
2. 通过MediaPlayerService创建它的Client端。
3. Client通过打分机制判断应该使用的播放器类型，并根据该类型创建对应播放器。
4. 调用播放器的设置资源函数，设置资源。

下面是setDataSource的类图:

![](https://github.com/MrHeLi/ffmpeg-leaning/blob/master/image/MediaPlayer_Class.png)

时序图:

![](https://github.com/MrHeLi/ffmpeg-leaning/blob/master/image/setDataSource.png)

## setDisplay()

```java
public void setDisplay(SurfaceHolder sh) {
    mSurfaceHolder = sh;//SurfaceHolder是一个接口，作用是持有、调整Surface大小、像素等。
    Surface surface;//这个类是一个用于显示的原始缓冲区的句柄（Handle onto a raw buffer）
    if (sh != null) {
        surface = sh.getSurface();
    } else {
        surface = null;
    }
    _setVideoSurface(surface);//调用本地函数设置句柄
    updateSurfaceScreenOn();//设置屏幕长亮
}
private native void _setVideoSurface(Surface surface);
```

走到本地代码，看看android_media_MediaPlayer.cpp怎么说：

```c++
static const JNINativeMethod gMethods[] = {
    {"_setVideoSurface",    "(Landroid/view/Surface;)V",        (void *)android_media_MediaPlayer_setVideoSurface},
}
static void
android_media_MediaPlayer_setVideoSurface(JNIEnv *env, jobject thiz, jobject jsurface)
{
    setVideoSurface(env, thiz, jsurface, true /* mediaPlayerMustBeAlive */);
}
static void
setVideoSurface(JNIEnv *env, jobject thiz, jobject jsurface, jboolean mediaPlayerMustBeAlive)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);//获取本地创建的MediaPlayer对象指针
    decVideoSurfaceRef(env, thiz);//该函数判断IGraphicBufferProducer是否已经设置过，如果有则取消引用
    sp<IGraphicBufferProducer> new_st;//发送数据的IPC接口，用于发送媒体frame数据
    if (jsurface) {
        sp<Surface> surface(android_view_Surface_getSurface(env, jsurface));
        if (surface != NULL) {
            new_st = surface->getIGraphicBufferProducer(); //获取IGraphicBufferProducer        
            new_st->incStrong((void*)decVideoSurfaceRef);//new_st引用计数+1
        } 
    }
    env->SetLongField(thiz, fields.surface_texture, (jlong)new_st.get());
    // This will fail if the media player has not been initialized yet. This
    // can be the case if setDisplay() on MediaPlayer.java has been called
    // before setDataSource(). The redundant call to setVideoSurfaceTexture()
    // in prepare/prepareAsync covers for this case.
    mp->setVideoSurfaceTexture(new_st);
}
static void
decVideoSurfaceRef(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    sp<IGraphicBufferProducer> old_st = getVideoSurfaceTexture(env, thiz);
    if (old_st != NULL) {
        old_st->decStrong((void*)decVideoSurfaceRef);
    }
}
sp<Surface> android_view_Surface_getSurface(JNIEnv* env, jobject surfaceObj) {
    //该函数是一个JNI 接口用于将java层中的Surface
    sp<Surface> sur;
    jobject lock = env->GetObjectField(surfaceObj,
            gSurfaceClassInfo.mLock);
    if (env->MonitorEnter(lock) == JNI_OK) {
        sur = reinterpret_cast<Surface *>(
                env->GetLongField(surfaceObj, gSurfaceClassInfo.mNativeObject));
        env->MonitorExit(lock);
    }
    env->DeleteLocalRef(lock);
    return sur;
}
status_t MediaPlayer::setVideoSurfaceTexture(
        const sp<IGraphicBufferProducer>& bufferProducer)
{
    ALOGV("setVideoSurfaceTexture");
    Mutex::Autolock _l(mLock);
    if (mPlayer == 0) return NO_INIT;
    return mPlayer->setVideoSurfaceTexture(bufferProducer);//将bufferProducer设置到了具体的播放器中处理
}
```

这里得说明一下关键的类和步骤：

* IGraphicBufferProducer： 这是一个用于管理媒体数据队列的IPC机制中的Client端，主要的作用在于将准备好的frame（媒体帧数据）发送给解码队列。
* android_view_Surface_getSurface：该函数定义在android_view_Surface.cpp中，该类是沟通java端的Surface类和本地Surface类的中间JNI接口，该函数的作用在于，获取java端的mNativeObject成员的值，该值其实就是c++Surface对象的地址值（在前面的逻辑中已经初始化）。
* setVideoSurfaceTexture：最终会调用在之前通过打分机制创建的播放器中，这个就不继续跟进了。



另外，关于播放的状态的控制还有如下一下关键的调用：

1. prepare();
2. start();
3. pause();
4. stop();
5. seekTo();
6. release();

根据前面的分析，MediaPlayer真正的播放器是通过打分机制创建的播放器，所以这些函数的调用最终都会调用到对应的播放器上去，这个就需要针对不同的播放器来分析了。

以一张图结束：

![](https://github.com/MrHeLi/ffmpeg-leaning/blob/master/image/setDisplay.png)



