# NuPlayer源码分析一：创建

源码环境：Oreo 8.0.0_r4

需要编译  /frameworks/av/media/libmediaplayerservice，生成libmediaplayerservice.so

该系列播放将详细分析NuPlayer源码，经过该系列博客之后，你应该就可以对底层播放器有了一个具体而全面的认识，系列文章分为下面几个部分：
* NuPlayer播放器创建
* NuPlayer解封装模块
* NuPlayer解码模块
* NuPlayer渲染和同步模块

## 简介

看过或者没看过[MediaPlayer源码分析](https://blog.csdn.net/qq_25333681/article/details/82056184)一文的朋友，都应该知道，严格意义上讲，MediaPlayer并不是播放器本身，它只是Android框架层众多媒体播放器（包括ROM厂商自定义的播放器）的“壳“。

MediaPlayer的所有主要操作（包括但不限于播放、暂停、快进等）都将通过调用链条，到达框架层的播放器，如NuPlayer。那么，这里就来分析一下，Android框架层真正的播放器，NuPlayer。

MediaPlayer和NuPlayer的关系如图：

![Android媒体解码关系](/Users/heli/github/ffmpeg-leaning/image/Android媒体解码关系.png)

## NuPlayer前情提要

既然`MediaPlayer`只是`NuPlayer`等底层播放器的壳，那么是不是`new MediaPlayer()` 的时候，底层就会`new NuPlayer()呢？`

结论是否定的，对于NuPlayer播放器来说，NuPlayer类中，实现了播放相关函数，但播放器的控制流程却是在NuPlayerDriver类中实现。

虽然如此，但这也不意味着`new MediaPlayer()`会导致`new NuPlayerDriver()`的函数调用。

不管是`NuPlayer`还是`NuPlayerDriver`实例的创建，都是在`MediaPlayer`实例化后的`setDataSource()`函数执行过程中实现的。

`MediaPlayer`的调用代码如下：

```C
player = new MediaPlayer();
player.setDataSource(path);
```

至于具体调用过程就不分析了， 有兴趣或者感到迷惑的同学可以去看一看[MediaPlayer源码分析](https://blog.csdn.net/qq_25333681/article/details/82056184)就全明白了。来看一下`setDataSource`部分的时序图:TODO：看有没有现成的，有的话就拿来用。

我们从最开始接触到的NuPlayer世界的代码开始，也就是`status_t MediaPlayerService::Client::setDataSource(...)`函数。

```c++
status_t MediaPlayerService::Client::setDataSource(int fd, int64_t offset, int64_t length) {
	// 略掉一些对资源的判断，剩下和可能和NuPlayer有关的部分
    player_type playerType = MediaPlayerFactory::getPlayerType(this,
                                                               fd,
                                                               offset,
                                                               length);
    sp<MediaPlayerBase> p = setDataSource_pre(playerType);
    // now set data source
    return mStatus = setDataSource_post(p, p->setDataSource(fd, offset, length));
}
```

留下了三个比较重要的调用，先依次说一下它们的作用，然后再展开：

1. `MediaPlayerFactory::getPlayerType`：该函数涉及Android底层媒体播放器的评分机制。通过评分，获得一个最优的播放器类型，具体怎么得到播放器类型，请阅：[Android Framework层播放器评分机制](https://blog.csdn.net/qq_25333681/article/details/89715957)，因为AndroidO只剩下了`NuPlayer`和`TestPlayer`两种播放器，`TestPlayer`并未正式启用。所以，函数调用返回的是`NuPlayer`对应的播放器类型`NU_PLAYER`。
2. `setDataSource_pre`：该函数的作用，是根据前面获得的播放器类型创建播放器对象。
3. `setDataSource_post`：将媒体资源设置给播放器，这才是真正的`setDataSource`操作。

接下来展开：`setDataSource_pre`和`setDataSource_post`函数。

为了让小伙伴们看代码片段的时候，可以有效的形成上下文逻辑，每部分代码，都会配一张图，以说明当前代码所处位置，比如：现在我们在这里（注意红色箭头，表示当前函数位置）。

![nuplayer_setdatasource_1](/Users/heli/github/ffmpeg-leaning/NuPlayer\nuplayer_setdatasource_1.JPG)

## NuPlayer播放器创建

![nuplayer_setdatasource_3](/Users/heli/github/ffmpeg-leaning/NuPlayer\nuplayer_setdatasource_3.JPG)

前面已经提到，NuPlayer的创建过程，是在`setDataSource_pre`函数中实现，我们接下来就展开一下吧：

```c++
sp<MediaPlayerBase> MediaPlayerService::Client::setDataSource_pre(
        player_type playerType)
{
    // create the right type of player
    sp<MediaPlayerBase> p = createPlayer(playerType);
	// 删掉了大量注册服务监听的代码，包括extractor、IOMX
    if (!p->hardwareOutput()) { // 播放器音频是否通过硬件直接输出，NuPlayer是不需要的。
        Mutex::Autolock l(mLock);
        mAudioOutput = new AudioOutput(mAudioSessionId, IPCThreadState::self()->getCallingUid(), mPid, mAudioAttributes);
        static_cast<MediaPlayerInterface*>(p.get())->setAudioSink(mAudioOutput);
    }
    return p;
}
```

### createPlayer

![nuplayer_setdatasource_4](/Users/heli/github/ffmpeg-leaning/NuPlayer\nuplayer_setdatasource_4.JPG)

```c++
sp<MediaPlayerBase> MediaPlayerService::Client::createPlayer(player_type playerType)
{
    // 检查当前进程，是否已经有一个播放器不同类型的播放器了，如果有，干掉它
    sp<MediaPlayerBase> p = mPlayer;
    if ((p != NULL) && (p->playerType() != playerType)) {
        ALOGV("delete player");
        p.clear();
    }
    if (p == NULL) { // 创建对应类型的播放器。
        p = MediaPlayerFactory::createPlayer(playerType, this, notify, mPid);
    }
    if (p != NULL) {
        p->setUID(mUid);
    }
    return p;
}
```

这个函数最重要的部分是 `MediaPlayerFactory::createPlayer`:

```c++
sp<MediaPlayerBase> MediaPlayerFactory::createPlayer(
        player_type playerType,
        void* cookie,
        notify_callback_f notifyFunc,
        pid_t pid) {
    sp<MediaPlayerBase> p;
    IFactory* factory;
    status_t init_result;
	// 略掉一些非关键代码
    factory = sFactoryMap.valueFor(playerType);
    p = factory->createPlayer(pid);
	// 略掉一些非关键代码
    init_result = p->initCheck();
    return p;
}
```

这个函数体实现的也比较简单，逻辑如下：

* `sFactoryMap.valueFor`：通过sFactoryMap和playerType获取播放器工厂对象。
* `factory->createPlayer`：调用播放器工厂对象创建播放器对象。
* `p->initCheck`：对播放器做初始化检查。

#### sFactoryMap.valueFor

sFactoryMap是个什么东西呢，看一下它的申明：

```c++
typedef KeyedVector<player_type, IFactory*> tFactoryMap;
static tFactoryMap sFactoryMap;
```

它是一个`KeyedVector`的结构，以播放器类型为键，对应的播放器工厂为值。在`MediaPlayerService`服务启动时，会通过`MediaPlayerFactory::registerBuiltinFactories()`函数调用，将所有的播放器工厂添加到这个Map结构中。这部分逻辑，在[Android Framework层播放器评分机制](https://blog.csdn.net/qq_25333681/article/details/89715957)一文中的**注册播放器工厂**小节中详细分析过，就不再赘述了。

我们已经知道此时的播放器类型为NU_PLAYER，`sFactoryMap.valueFor(playerType);`可以等价于：

`sFactoryMap.valueFor(NU_PLAYER)`，所以，factory是NuPlayer播放器对应的工厂对象。简单看一下类图结构。

![播放器工厂](/Users/heli/github/ffmpeg-leaning/image/播放器工厂.png)

#### factory->createPlayer

![5](/Users/heli/github/ffmpeg-leaning/NuPlayer\5.JPG)

通过类图，和前面的分析，到这里我们已经知道`NuPlayer`的播放器工厂是`NuPlayerFactory`类：

```c++
class NuPlayerFactory : public MediaPlayerFactory::IFactory {
  public:
	// 删掉了评分机制的代码
    virtual sp<MediaPlayerBase> createPlayer(pid_t pid) {
        ALOGV(" create NuPlayer");
        return new NuPlayerDriver(pid);
    }
};
```

说好的创建`NuPlayer`播放器呢，怎么冒出来一个`NuPlayerDriver`？

其实，虽然播放器叫`NuPlayer`，但并意味着“播放器”只有`NuPlayer`对象。实际上，`NuPlayer`播放器由`NuPlayerDriver`和`NuPlayer`两部分组成，`NuPlayer`对象负责播放、暂停等功能函数的实现，`NuPlayerDriver`则负责功能的调度，和`MediaPlayerSerivce`等外界沟通。

回到代码。

`NuPlayerFactory::createPlayer`函数只new了一个`NuPlayerDriver`，我们来看一下`NuPlayerDriver`的初始化过程：

```c++
NuPlayerDriver::NuPlayerDriver(pid_t pid)
    : mState(STATE_IDLE),
      mLooper(new ALooper),
      mPlayer(new NuPlayer(pid)),
      mLooping(false),
      mAutoLoop(false) {
    mLooper->setName("NuPlayerDriver Looper");
    mLooper->start(
            false, /* runOnCallingThread */
            true,  /* canCallJava */
            PRIORITY_AUDIO);

    mLooper->registerHandler(mPlayer);
    mPlayer->setDriver(this);
}
```

为了简洁，代码依然删掉了不少暂时并不重要的。这部分代码，其实我在[Android媒体底层通信框架Native Handler(三):NuPlayer](https://blog.csdn.net/qq_25333681/article/details/89436181)一文中已经讲过。

有所不同的是，当时侧重点放在媒体通信部分，也就是`NativeHandler`逻辑部分。

`NuPlayerDriver`的构造函数部分，除了`NativeHandler`逻辑外，最重要的就是以下三个操作了：

1. `mState(STATE_IDLE)`：将播放器状态设置为`STATE_IDLE`（空闲）。
2. `new NuPlayer(pid)`：创建一个`NuPlayer`对象，并让`NuPlayerDriver`持有`NuPlayer`的引用。这里稍后展开。
3. `setDriver(this)`：将`NuPlayerDriver`设置给`NuPlayer`，让`NuPlayer`持有`NuPlayerDriver`的引用。

第二和第三点，让`NuPlayerDriver`和`NuPlayer`相互持有引用，目的是在后续的流程控制中，方便彼此回调，配合工作。

到这里，`NuPlayer`的创建过程，算是明白了。`NuPlayer`的构造函数没什么好看的，就是给一堆成员赋初值的过程。

#### initCheck()

因为`createPlayer`函数创建并返回的是`NuPlayerDriver`对象，所以调用的是`NuPlayerDriver::initCheck`函数:

```c++
status_t NuPlayerDriver::initCheck() {
    return OK;
}
```

啥也没干，直接返回了`OK`，有点浪费时间的感觉^kugualian^

代码到哪儿了？

![6](/Users/heli/github/ffmpeg-leaning/NuPlayer\6.JPG)

### setAudioSink

![7](/Users/heli/github/ffmpeg-leaning/NuPlayer\7.JPG)

`AudioOutput`对象是音频输出的抽象层，在不支持硬件驱动直接输出的接口下，需要手动设置音频输出的抽象层接口。

`MediaPlayerBase`和它子类的结构图如下：

![MediaPlayerBase](/Users/heli/github/ffmpeg-leaning/NuPlayer/MediaPlayerBase.png)

通过`createPlayer`函数，返回的是`NuPlayerDriver`对象。

```c++
void NuPlayerDriver::setAudioSink(const sp<AudioSink> &audioSink) {
    mPlayer->setAudioSink(audioSink);
    mAudioSink = audioSink;
}
```

这个调用的`mPlayer`，在`NuPlayerDriver`构造函数的初始化列表中，已经新建了一个`NuPlayer`对象，并赋值给`mPlayer`。所以，来看一下`NuPlayer`的`setAudioSink`：

```c++
void NuPlayer::setAudioSink(const sp<MediaPlayerBase::AudioSink> &sink) {
    sp<AMessage> msg = new AMessage(kWhatSetAudioSink, this);
    msg->setObject("sink", sink);
    msg->post();
}
```

```c++
case kWhatSetAudioSink:
        {
            sp<RefBase> obj;
            CHECK(msg->findObject("sink", &obj));
            mAudioSink = static_cast<MediaPlayerBase::AudioSink *>(obj.get());
            break;
        }
```

> 关于`AMessage`、`msg->findObject`等代码和为什么这么调用，可以去快速看一下[Android媒体底层通信框架Native Handler(三):NuPlayer](https://blog.csdn.net/qq_25333681/article/details/89436181)的总结部分。

可以看出，不管是`NuPlayerDriver`还是`NuPlayer`的`setAudioSink`代码，都是将新建的`AudioOutput`对象存在对应的`mAudioSink`字段中了，方便以后播放音频做准备。

## setDataSource

当前代码位置：

![8](/Users/heli/github/ffmpeg-leaning/NuPlayer\8.JPG)

在前一个流程中，创建了`NuPlayer`和`NuPlayerDriver`对象，并将`NuPlayerDriver`对象指针保存在了p中，接着，通过p调用了`NuPlayerDriver`的`setDataSource`函数。

```c++
status_t NuPlayerDriver::setDataSource(int fd, int64_t offset, int64_t length) {
    if (mState != STATE_IDLE) { // NuPlayerDriver构造中mState被设置成了STATE_IDLE。
        return INVALID_OPERATION;
    }
    mState = STATE_SET_DATASOURCE_PENDING; // 将播放器状态设置为STATE_SET_DATASOURCE_PENDING
    mPlayer->setDataSourceAsync(fd, offset, length); // 调用NuPlayer，设置媒体源
    while (mState == STATE_SET_DATASOURCE_PENDING) {
        mCondition.wait(mLock); // 加锁，直到被通知唤醒
    }
    return mAsyncResult;
}
```

该函数主要作用：

* **mState = STATE_SET_DATASOURCE_PENDING**: 设置播放器状态，和流程控制有关，比较重要，后面很多流程都需要判断当前状态，上一个状态是`NuPlayerDriver`构造中设置的`STATE_IDLE`状态。
* **mPlayer->setDataSourceAsync**：实际上`NuPlayerDriver`并没有处理资源的逻辑，前面也提到，它就是一层壳，需要将具体的动作交给`NuPlayer`对象去做。
* **while (mState == STATE_SET_DATASOURCE_PENDING)**：因为上一步的`setDataSourceAsync`流程中会用到`NativeHandler`机制，是异步的，所以在while循环体中加了一个锁，让当前线程阻塞。直到`setDataSourceAsync`流程执行完毕后，唤醒。

### setDataSourceAsync

![9](/Users/heli/github/ffmpeg-leaning/NuPlayer\9.JPG)

继续跟踪`setDataSourceAsync`函数：

```c++
void NuPlayer::setDataSourceAsync(int fd, int64_t offset, int64_t length) {
    sp<AMessage> msg = new AMessage(kWhatSetDataSource, this); // 新建消息，这属于常规操作了
    sp<AMessage> notify = new AMessage(kWhatSourceNotify, this); // 新建消息，用于和解封装模块通信，类似于一种listener的功能。

    sp<GenericSource> source = new GenericSource(notify, mUIDValid, mUID); // 创建解封装器
    status_t err = source->setDataSource(fd, offset, length);  // 为GenericSource设置媒体源

    msg->setObject("source", source);
    msg->post(); // 将创建并设置好的setDataSource，post给下一个流程处理
    mDataSourceType = DATA_SOURCE_TYPE_GENERIC_FD;
}
```

该函数主要逻辑如下：

* **new AMessage**：构建了两个消息对象，`msg`用于向下一个流程发送消息和当前函数执行的成果（`source`）。`notify`用于在构建`GenericSource`的结果回调。
*  **new GenericSource**：只是一个解封装格式的类，同样的类还有`RTSPSource`、`HTTPLiveSource`等，是媒体流信息的直接处理者。媒体源信息也将被设置到该对象中。这会在一下篇文章进行展开，这里就先留个疑问。
* **source->setDataSource**：将媒体流（源）设置给解封装格式的解析器，这个也在下一篇文章中展开。
* **msg->post()**：通过`NativeHandler`机制，将函数执行结果，也就是新创建的`source`对象发送给下一个函数执行`onMessageReceived`，这个过程是异步的，当前函数执行到这里就会退栈。

创建了一个解封装格式的解析器后，将结果`post`到`NuPlayer::onMessageReceived`函数处理：

```c++
void NuPlayer::onMessageReceived(const sp<AMessage> &msg) {
    switch (msg->what()) {
        case kWhatSetDataSource:
        {
            status_t err = OK;
            sp<RefBase> obj;
            CHECK(msg->findObject("source", &obj));
            if (obj != NULL) {
                Mutex::Autolock autoLock(mSourceLock);
                mSource = static_cast<Source *>(obj.get()); // 将新创建的GenericSource对象，赋值给mSource
            } else {
                err = UNKNOWN_ERROR;
            }
            sp<NuPlayerDriver> driver = mDriver.promote();
            if (driver != NULL) {
                driver->notifySetDataSourceCompleted(err); // 通知NuPlayerDriver，任务完成
            }
            break;
        }
        // 略去一万行代码
	}
}
```

这段代码的重点在于：

* **mSource =**：将之前创建的**GenericSource**对象赋值给了**mSource**字段。
* **driver->notifySetDataSourceCompleted**：到这里，整个setDataSource的流程已经执行完毕，函数调用回到NuPlayerDriver中。

###NuPlayerDriver::notifySetDataSourceCompleted

![10](/Users/heli/github/ffmpeg-leaning/NuPlayer\10.JPG)

```c++
void NuPlayerDriver::notifySetDataSourceCompleted(status_t err) { // err = OK;
    CHECK_EQ(mState, STATE_SET_DATASOURCE_PENDING); // 当前mState为STATE_SET_DATASOURCE_PENDING
    mAsyncResult = err;
    mState = (err == OK) ? STATE_UNPREPARED : STATE_IDLE; // 将状态设置为STATE_UNPREPARED
    mCondition.broadcast(); // 唤醒mCondition.wait(mLock);锁，完成setDataSource函数调用
}
```

如果没出以外，这里的入参值应该是`OK`的。所以，该函数的主要操作有：

* 将当前状态设置成STATE_UNPREPARED。上一个状态未STATE_SET_DATASOURCE_PENDING。
* **mCondition.broadcast()**：发出唤醒mCondition锁广播。

释放锁后，`NuPlayerDriver::setDataSource`会将执行的结果`mAsyncResult`返回给调用者。`setDataSource`流程执行完毕。

# NuPlayer解封装模块

解封装模块的重要作用，是将封装好的音视频源文件，通过不同的封装协议，解析成码流后，送到解码器解码。

NuPlayer中和解封装相关的类有：

* NuPlayer::Source：解封装模块的基类，定义了解封装的基本接口。
* GenericSource：本地文件相关。
* HTTPLiveSource：HLS流媒体使用的解封装类。
* RTSPSource：SDP协议媒体流使用的解封装类。

此外，还需要DataSource等配合操作。类图如下：

![10](/Users/heli/github/ffmpeg-leaning/NuPlayer/sources_class_diagraph.png)

篇幅有限，本文主要介绍本地媒体文件的例子。也就是GenericSource播放本地文件为例。

## Android播放器的一般步骤

一个Android播放器典型的播放步骤一般是：

1. 播放器创建。
2. 设置媒体源文件（本地文件路径、或者Uri）。
3. 准备媒体数据。
4. 播放视频。
5. 停止播放。

为了方便分析解封装模块，也从该顺序逐步分析解封装过程。对应的播放器调用接口如下：

1. GenericSource：创建

2. setDataSource：设置媒体源数据

3. prepareAsync：准备媒体数据

4. start：播放视频

5. stop&pause：停止播放

## GenericSource：创建

先来分析第一个步骤：**GenericSource的创建**，即**播放器创建**部分。在流程中的位置是：

1. **GenericSource的创建**

2. setDataSource：设置媒体源数据

3. prepareAsync：准备媒体数据

4. start：启动

5. stop&pause&resume：停止&暂停&恢复

上一篇文章中，我们提到，在`NuPlayer`的`setDataSourceAsync`函数中创建了`GenericSource`对象。并调用了`setDataSource`函数。用一张图回忆一下：

![9](/Users/heli/github/ffmpeg-leaning/NuPlayer\9.JPG)

再来看看对应代码：

```c++
void NuPlayer::setDataSourceAsync(int fd, int64_t offset, int64_t length) {
    sp<AMessage> msg = new AMessage(kWhatSetDataSource, this); // 新建消息，这属于常规操作了
    sp<AMessage> notify = new AMessage(kWhatSourceNotify, this); // 新建消息，用于和解封装模块通信，类似于一种listener的功能。

    sp<GenericSource> source = new GenericSource(notify, mUIDValid, mUID); // 创建解封装器
    status_t err = source->setDataSource(fd, offset, length);  // 为GenericSource设置媒体源
    msg->setObject("source", source);
    msg->post(); // 将创建并设置好的setDataSource，post给下一个流程处理
    mDataSourceType = DATA_SOURCE_TYPE_GENERIC_FD;
}
```

这段代码中，首次创建了一个GenericSource实例，先来看看实例化过程。

```c++
NuPlayer::GenericSource::GenericSource(
        const sp<AMessage> &notify,
        bool uidValid,
        uid_t uid)
    : Source(notify), // 将一个AMessage对象存放在父类Source的mNotify字段中，这是个通用操作，用来通知调用者，当前资源状态的。
      mAudioTimeUs(0),
      mAudioLastDequeueTimeUs(0),
      mVideoTimeUs(0),
      mVideoLastDequeueTimeUs(0),
      mFetchSubtitleDataGeneration(0),
      mFetchTimedTextDataGeneration(0),
      mDurationUs(-1ll),
      mAudioIsVorbis(false), // 音频是否为Vorbis压缩格式，默认为false
      mIsSecure(false),
      mIsStreaming(false),
      mFd(-1), // 文件句柄
      mBitrate(-1ll), // 比特率
      mPendingReadBufferTypes(0) {
    mBufferingMonitor = new BufferingMonitor(notify); // 新建一个BufferingMonitor实例
    resetDataSource(); // 重置一些DataSource数据到初始状态。
}
```

从构造函数默认初始化列表中的字段含义来看，GenericSource包含了除了Buffer以外几乎所有的解封装相关数据，如文件句柄（mFd）、媒体时长（mDurationUs）等。

而关于Buffer状态的管理和监听使用的是BufferingMonitor类来实现。

* **BufferingMonitor**：协助监控Buffer的状态，每秒轮询一次，必要时会将Buffer的状态通过AMessage通知Player。

可见其重要性，来简单看一下该结构体和部分函数，间接感受一下它的功能：

```c++
struct BufferingMonitor : public AHandler {
    public:
    explicit BufferingMonitor(const sp<AMessage> &notify);
    // 重新启动监视任务。
    void restartPollBuffering();
    // 停止缓冲任务并发送相应的事件。
    void stopBufferingIfNecessary();
    // 确保数据源正在获取数据。
    void ensureCacheIsFetching();
    // 更新从DataSource刚刚提取的缓冲区的媒体时间。
    void updateQueuedTime(bool isAudio, int64_t timeUs);
    // 更新发送到解码器的最后出队缓冲区的媒体时间。
    void updateDequeuedBufferTime(int64_t mediaUs);
    protected:
    virtual ~BufferingMonitor();
    virtual void onMessageReceived(const sp<AMessage> &msg);
}
```

##  setDataSource：设置媒体源数据

setDataSource在播放流程中的位置为：

1. GenericSource的创建

2. **setDataSource：设置媒体源数据**

3. prepareAsync：准备媒体数据

4. start：启动

5. stop&pause&resume：停止&暂停&恢复

```c++
status_t NuPlayer::GenericSource::setDataSource(int fd, int64_t offset, int64_t length) {
    ALOGV("setDataSource %d/%lld/%lld", fd, (long long)offset, (long long)length);
    resetDataSource(); // 重置一些DataSource数据到初始状态。
    mFd = dup(fd); // 将文件的句柄复制一份给mFd字段
    mOffset = offset; // 数据的偏移量
    mLength = length; // 文件长度

    // delay data source creation to prepareAsync() to avoid blocking
    // the calling thread in setDataSource for any significant time.
    return OK;
}
```

> `dup(fd)`是什么？该函数定义在/frameworks/base/core/java/android/os/ParcelFileDescriptor.java中，函数原型为：`public static ParcelFileDescriptor dup(FileDescriptor orig)`
>
> 作用：创建一个新的ParcelFileDescriptor，它是现有FileDescriptor的副本。 这遵循标准POSIX语义，其中新文件描述符共享状态，例如文件位置与原始文件描述符。

可以看到，setDataSource除了将媒体文件相关参数保存下来外，并没有做其他的工作。顺便看一看`resetDataSource`函数吧：

```c++
void NuPlayer::GenericSource::resetDataSource() {
   mUri.clear();
   mUriHeaders.clear();
   if (mFd >= 0) {
       close(mFd);
       mFd = -1;
   }
   mOffset = 0;
   mLength = 0;
   mStarted = false;
   mStopRead = true;

   if (mBufferingMonitorLooper != NULL) { // 让BufferingMonitor停止循环监听buffer
       mBufferingMonitorLooper->unregisterHandler(mBufferingMonitor->id());
       mBufferingMonitorLooper->stop();
       mBufferingMonitorLooper = NULL;
   }
   mBufferingMonitor->stop();
   mMimes.clear();
}
```

主要有两个方面作用：

1. 将一些媒体资源文件相关索引（值），以及解析器状态重置为默认状态。
2. 停止使用让BufferingMonitor停止循环监听buffer。

下面来看看如何准备资源的

## prepareAsync：准备媒体数据

prepareAsync在播放流程中的位置为：

1. GenericSource的创建

2. setDataSource：设置媒体源数据

3. **prepareAsync：准备媒体数据**

4. start：启动

5. stop&pause&resume：停止&暂停&恢复

```c++
void NuPlayer::GenericSource::prepareAsync() {
    ALOGV("prepareAsync: (looper: %d)", (mLooper != NULL));
    if (mLooper == NULL) { // 创建looper并启动AHandler循环
        mLooper = new ALooper;
        mLooper->setName("generic");
        mLooper->start();
        mLooper->registerHandler(this);
    }
    sp<AMessage> msg = new AMessage(kWhatPrepareAsync, this);
    msg->post();
}
```

虽然代码少，但这是一个很重要的调用：创建ALooper并且让Looper 循环起来了。这个信息告诉我们，GenericSource本身组成了一个NativeHandler体系，用于传递自身消息。

GenericSource类通过继承`NuPlayer::Source`间接继承了AHandler，用于处理消息。

这些，都说明GenericSource的函数会有部分是异步的，函数名中`prepareAsync`中的Async也表明了这一点。

不熟悉的朋友可以翻一翻这篇文章：[Android媒体底层通信框架Native Handler](https://blog.csdn.net/qq_25333681/article/details/89289411)

启动了looper循环处理消息后，发送了一个kWhatPrepareAsync的消息，给looper线程来处理。

熟悉NativeHandler的朋友应该知道，GenericSource函数作为AHandler，必然要重写onMessageReceived函数，用于处理数据：

```c++
void NuPlayer::GenericSource::onMessageReceived(const sp<AMessage> &msg) {
    switch (msg->what()) {
      case kWhatPrepareAsync:
      {
          onPrepareAsync();
          break;
      }
      case kWhatStart:
      case kWhatResume:
      {
          mBufferingMonitor->restartPollBuffering();
          break;
      }
      // .......省略一万行.......
    }
}
```

AMessage的标志是kWhatPrepareAsync，在onMessageReceived并没有做什么处理，直接调用了onPrepareAsync函数。

```c++
void NuPlayer::GenericSource::onPrepareAsync() { // 该函数运行在looper所在的子线程中
    // delayed data source creation
    if (mDataSource == NULL) { // 第一次进来，mDataSource肯定为空
        mIsSecure = false; // 先设置为false，如果extractor返回为安全，再设置为true.
        if (!mUri.empty()) { // 因为是本地文件，所以mUri不用初始化，自然为空。
			// 略掉网络媒体源创建DataSource相关代码。
        } else { // 处理本地媒体文件源
            // media.stagefright.extractremote属性一般不会设置，
            if (property_get_bool("media.stagefright.extractremote", true) &&
                    !FileSource::requiresDrm(mFd, mOffset, mLength, nullptr /* mime */)) {
                sp<IBinder> binder =
                        defaultServiceManager()->getService(String16("media.extractor"));
                if (binder != nullptr) {
                    ALOGD("FileSource remote");
                    sp<IMediaExtractorService> mediaExService(
                            interface_cast<IMediaExtractorService>(binder));
                    sp<IDataSource> source =
                            mediaExService->makeIDataSource(mFd, mOffset, mLength);
                    ALOGV("IDataSource(FileSource): %p %d %lld %lld",
                            source.get(), mFd, (long long)mOffset, (long long)mLength);
                    if (source.get() != nullptr) {
                        mDataSource = DataSource::CreateFromIDataSource(source);
                        if (mDataSource != nullptr) { // 过河拆迁，初始化mDataSource成功后
                            // Close the local file descriptor as it is not needed anymore.
                            close(mFd);
                            mFd = -1;	
                        }
                    }
                }
            }
            if (mDataSource == nullptr) { // 如果没有从extractor服务中成功获取DataSource就自己创建
                ALOGD("FileSource local");
                mDataSource = new FileSource(mFd, mOffset, mLength);
            }
            mFd = -1;
        }

        if (mDataSource == NULL) { // 到这里基本上是不可能为NULL了
            ALOGE("Failed to create data source!");
            notifyPreparedAndCleanup(UNKNOWN_ERROR);
            return;
        }
    }

    if (mDataSource->flags() & DataSource::kIsCachingDataSource) {
        mCachedSource = static_cast<NuCachedSource2 *>(mDataSource.get());
    }

    // For cached streaming cases, we need to wait for enough
    // buffering before reporting prepared.
    mIsStreaming = (mCachedSource != NULL);

    // init extractor from data source
    status_t err = initFromDataSource();
	// ...
    finishPrepareAsync();
    ALOGV("onPrepareAsync: Done");
}

```

从函数代码中可以看出，该函数唯一的目的就是为了初始化mDataSource，主要的初始化方式有两个：

1. 从MediaExtractorService服务中获取。
2. 如果第一步未能初始化成功，直接自己创建一个`new FileSource`。

这里没有想到的是，Android底层框架为了解封装的通用性，直接提供了一个解封装相关的服务：MediaExtractorService，服务名称为："media.extractor"，NuPlayer作为众多播放器的一种，也是可以直接享受该服务的。在这里就通过该服务，创建了一个DataSource对象。

这里有个问题，最终NuPLayer使用的到底是通过ExtractorService获取DataSource对象，还是直接自己`new FileSource`呢。

我们当然可以通过日志来判断，但这会失去对播放逻辑的学习。所以，我一般都通过代码来判断。

因为代码调用的先后顺序，我们先来看通过服务获取的过程。

### MediaExtractor服务获取DataSource

```c++
// media.stagefright.extractremote属性一般不会设置，
if (property_get_bool("media.stagefright.extractremote", true) &&
    !FileSource::requiresDrm(mFd, mOffset, mLength, nullptr /* mime */)) {
    // 通过Binder机制，获取"media.extractor"服务的远程代理
    sp<IBinder> binder =
        defaultServiceManager()->getService(String16("media.extractor"));
    if (binder != nullptr) { // 获取失败时为空指针
        ALOGD("FileSource remote");
        // 强转为IMediaExtractorService对象指针
        sp<IMediaExtractorService> mediaExService(
            interface_cast<IMediaExtractorService>(binder));
        // 调用服务的代理对象接口，获取IDataSource对象指针
        sp<IDataSource> source =
            mediaExService->makeIDataSource(mFd, mOffset, mLength);
        ALOGV("IDataSource(FileSource): %p %d %lld %lld",
              source.get(), mFd, (long long)mOffset, (long long)mLength);
        if (source.get() != nullptr) {
            // 通过获取IDataSource对象指针初始化mDataSource
            mDataSource = DataSource::CreateFromIDataSource(source);
            if (mDataSource != nullptr) { // 过河拆迁，初始化mDataSource成功后
                // Close the local file descriptor as it is not needed anymore.
                close(mFd);
                mFd = -1;	
            }
        }
    }
}
```

这一段代码，比较重要的函数调用，都加上了注释，这里再啰嗦得总结一下吧：

* `getService(String16("media.extractor"))`：熟悉binder机制的同学都知道，这是Binder远端获取指定服务的基本操作了。有时间整理一份文章出来，敬请期待吧。
* `mediaExService->makeIDataSource`：调用服务接口，创建IDataSource对象。
* `DataSource::CreateFromIDataSource`：调用CreateFromIDataSource通过前面创建的IDataSource初始化mDataSource。

基本上就这么回事儿。第一点就不说了，东西太多。这里稍微展开一下第二、第三点的调用。

#### makeIDataSource

该函数是通过Binder的远端调用，最终会调用到服务端的代码，也就是MediaExtractorService中：

> 代码路径：/frameworks/av/services/mediaextractor/MediaExtractorService.cpp

```c++
sp<IDataSource> MediaExtractorService::makeIDataSource(int fd, int64_t offset, int64_t length)
{
    sp<DataSource> source = DataSource::CreateFromFd(fd, offset, length);
    return source.get() != nullptr ? source->asIDataSource() : nullptr;
}
```

再看CreateFromFd干了啥:

```c++
sp<DataSource> DataSource::CreateFromFd(int fd, int64_t offset, int64_t length) {
    sp<FileSource> source = new FileSource(fd, offset, length); // 也是直接new了FileSource
    return source->initCheck() != OK ? nullptr : source; // 检查是否有sp时候为有效指针，有效把指针丢回去
}
```

咦~这代码看着耳熟啊！！！也是直接`new FileSource`。

> 那个`initCheck()`函数，就不说了，说多了又是长篇大论。

#### CreateFromIDataSource

```c++
sp<DataSource> DataSource::CreateFromIDataSource(const sp<IDataSource> &source) {
    return new TinyCacheSource(new CallbackDataSource(source));
}
```

我去，又来了两个陌生的类，其实他们都和DataSource有千丝万缕的联系，看看一下类图：

![nuplayer_datasource_classgraph](/Users/heli/github/ffmpeg-leaning/NuPlayer\nuplayer_datasource_classgraph.JPG)

有关`CallbackDataSource`和`TinyCacheSource`的定义，都在CallbackDataSource.h头文件中。它们的定义都比较简单，就不贴代码了，有兴趣自己去看，源码路径如下：

> \frameworks\av\include\media\stagefright\CallbackDataSource.h
>
> \frameworks\av\media\libstagefright\CallbackDataSource.cpp

下面来稍微总结一下前面的类图：

* `DataSource`：该类规定了媒体源文件基本的操作接口。

* `IDataSource`：它是实现远程调用stagefright DataSource的Binder接口。Android媒体相关的各种服务中，创建的`DataSource`对象，都通过这个client的远程接口句柄来调用。

* `CallbackDataSource`：实现了DataSource接口（实现关系），但它的私有字段`mIDataSource`中，保留了`IDataSource`（服务端DataSource）的引用（组合关系），让Client端程序可以回调到server端的`DataSource`对象，从而具备了”回调“功能。
* `TinyCacheSource`：该类实现了`DataSource`接口（实现关系），在私有字段`mSource`中可以持有`DataSource`的引用，这个引用通常是用来存放`CallbackDataSource`对象的，所以和`CallbackDataSource`形成了组合关系。另外，该类中还有一个用于缓存的数组`mCache[kCacheSize]`，对于小于`kCacheSize`的读取，它将提前读取并缓存在`mCache`中，这不仅极大减少了Client端到Server端的数据读取操作，对提高数据类型嗅探和元数据（metadata）的提取也有较高效率。

回头来看代码：

```c++
return new TinyCacheSource(new CallbackDataSource(source));
```

也就稀松平常了，不过是将server端的`FileSource`对象，通过`IDataSource`接口传递到client端后，依次通过`CallbackDataSource`、`TinyCacheSource`对象包起来，已达到后续可以通过`IDataSource`对象调用远端`FileSource`对象的目的。

### new FileSource

整个`onPrepareAsync`函数执行的前一部分，都在想法设法的通过"media.extractor"服务，获取初始化`mDataSource`字段，如果初始化失败，那么这个这个逻辑不会执行，如果失败，那么`mDataSource`的值为`NULL`。



```c++
if (mDataSource == nullptr) { // 如果没有从extractor服务中成功获取DataSource就自己创建
    ALOGD("FileSource local");
    mDataSource = new FileSource(mFd, mOffset, mLength);
}
mFd = -1;
```

这段代码就比较简洁，直接创建一个FileSource，将文件句柄和偏移量，长度等信息作出构造参数传递过去。这里就先不展开FileSource源码的分析，后面涉及到的时候再聊。

### 小结

所以，总结一下prepareAsync函数：

* 该函数是异步执行的，整整的prepare动作，是在子线程执行的`onPrepareAsync`函数中。
* `onPrepareAsync`函数主要的作用就是初始化`mDataSource`字段。共有两种方式，首相尝试通过"media.extractor"服务获取server端`DataSource`，失败后尝试直接自己`new FileSource`。
* 远端服务实例化`mDataSource`能否成功，主要看该服务在系统中是否启用（一般来说都是正常运行的）。
* 如果无法通过"media.extractor"初始化`mDataSource`，就直接自己创建（`new FileSource`）。
* 不管通过server端还是自己new的方式，`mDataSource`最终关联的对象都是`FileSource`的实例。

###  initFromDataSource

想方设法将mDataSource字段初始化后，接着往下看（不考虑初始化失败的场景）。

```c++
if (mDataSource->flags() & DataSource::kIsCachingDataSource) { // 16 & 4 = 0
   mCachedSource = static_cast<NuCachedSource2 *>(mDataSource.get());
}
mIsStreaming = (mCachedSource != NULL); // mIsStreaming = false

// 通过data source初始化extractorinit
status_t err = initFromDataSource();
```

`mDataSource->flags()`这段代码，会经历漫长的路程，大概是这样：

mDataSource->flags() ==>> TinyCacheSource::flags() ==>> CallbackDataSource::flags() ==>> FileSource::flags()。

```c++
virtual uint32_t flags() {
    return kIsLocalFileSource;
}
```
最终返回一个固定的值`kIsLocalFileSource`也就是16。该值定义在一个DataSource的结构体中：

```c++
enum Flags {
    kWantsPrefetching      = 1,
    kStreamedFromLocalHost = 2,
    kIsCachingDataSource   = 4,
    kIsHTTPBasedSource     = 8,
    kIsLocalFileSource     = 16,
};
```
而`DataSource::kIsCachingDataSource`的值为4,16&4 = 0。结果可想而知，哎，走了步闲棋。

该干正事了，分析一下这段调用中最重要的函数之一：`initFromDataSource`。

```c++
status_t NuPlayer::GenericSource::initFromDataSource() {
    sp<IMediaExtractor> extractor;
    extractor = MediaExtractor::Create(mDataSource, NULL); // 创建

    mFileMeta = extractor->getMetaData();
    if (mFileMeta != NULL) {
        int64_t duration;
        if (mFileMeta->findInt64(kKeyDuration, &duration)) {
            mDurationUs = duration;
        }
    }

    int32_t totalBitrate = 0;
    size_t numtracks = extractor->countTracks();

    mMimes.clear();
    for (size_t i = 0; i < numtracks; ++i) {
        sp<IMediaSource> track = extractor->getTrack(i);
        sp<MetaData> meta = extractor->getTrackMetaData(i);
        const char *mime;
        CHECK(meta->findCString(kKeyMIMEType, &mime));
        ALOGV("initFromDataSource track[%zu]: %s", i, mime);

        if (!strncasecmp(mime, "audio/", 6)) {
            if (mAudioTrack.mSource == NULL) {
                mAudioTrack.mIndex = i;
                mAudioTrack.mSource = track;
                mAudioTrack.mPackets =
                    new AnotherPacketSource(mAudioTrack.mSource->getFormat());

                if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_VORBIS)) {
                    mAudioIsVorbis = true;
                } else {
                    mAudioIsVorbis = false;
                }

                mMimes.add(String8(mime));
            }
        } else if (!strncasecmp(mime, "video/", 6)) {
            if (mVideoTrack.mSource == NULL) {
                mVideoTrack.mIndex = i;
                mVideoTrack.mSource = track;
                mVideoTrack.mPackets =
                    new AnotherPacketSource(mVideoTrack.mSource->getFormat());

                // video always at the beginning
                mMimes.insertAt(String8(mime), 0);
            }
        }

        mSources.push(track);
        int64_t durationUs;
        if (meta->findInt64(kKeyDuration, &durationUs)) {
            if (durationUs > mDurationUs) {
                mDurationUs = durationUs;
            }
        }

        int32_t bitrate;
        if (totalBitrate >= 0 && meta->findInt32(kKeyBitRate, &bitrate)) {
            totalBitrate += bitrate;
        } else {
            totalBitrate = -1;
        }
    }

    ALOGV("initFromDataSource mSources.size(): %zu  mIsSecure: %d  mime[0]: %s", mSources.size(),
            mIsSecure, (mMimes.isEmpty() ? "NONE" : mMimes[0].string()));

    mBitrate = totalBitrate;
    return OK;
}
```

#### IMediaExtractor创建

代码挺长，先来看看`MediaExtractor::Create(mDataSource, NULL)`:

```c++
// static
sp<IMediaExtractor> MediaExtractor::Create(
        const sp<DataSource> &source, const char *mime) {
    if (!property_get_bool("media.stagefright.extractremote", true)) {
        // 本地 extractor
        ALOGW("creating media extractor in calling process");
        return CreateFromService(source, mime);
    } else { // 使用远程extractor
        ALOGV("get service manager");
        sp<IBinder> binder = defaultServiceManager()->getService(String16("media.extractor"));
        if (binder != 0) {
            sp<IMediaExtractorService> mediaExService(interface_cast<IMediaExtractorService>(binder));
            sp<IMediaExtractor> ex = mediaExService->makeExtractor(source->asIDataSource(), mime);
            return ex;
        } else {
            ALOGE("extractor service not running");
            return NULL;
        }
    }
    return NULL;
}
```

第一个条件判断，获取系统属性“media.stagefright.extractremote”，通常该属性都是默认未设置的。

> media.stagefright.extractremote系统属性，用于判断是否之处远程extractor服务。

所以`property_get_bool`调用返回默认值`true`，`!true`则条件不成立。所以通过远程服务“media.extractor”创建一个MediaExtractor返回。

##### MediaExtractorService::makeExtractor

```c++
sp<IMediaExtractor> MediaExtractorService::makeExtractor(
        const sp<IDataSource> &remoteSource, const char *mime) {
    ALOGV("@@@ MediaExtractorService::makeExtractor for %s", mime);
    sp<DataSource> localSource = DataSource::CreateFromIDataSource(remoteSource);
    sp<IMediaExtractor> ret = MediaExtractor::CreateFromService(localSource, mime);
    ALOGV("extractor service created %p (%s)", ret.get(), ret == NULL ? "" : ret->name());
    if (ret != NULL) {
        registerMediaExtractor(ret, localSource, mime);
    }
    return ret;
}
```

`DataSource::CreateFromIDataSource`前面已经详细说明了，这里就不赘述了。来看看更重要的函数

###### MediaExtractor::CreateFromService

```c++
sp<MediaExtractor> MediaExtractor::CreateFromService(
        const sp<DataSource> &source, const char *mime) {
    ALOGV("MediaExtractor::CreateFromService %s", mime);
    RegisterDefaultSniffers();

    sp<AMessage> meta;
    String8 tmp;
    if (mime == NULL) {
        float confidence;
        if (!sniff(source, &tmp, &confidence, &meta)) {
            ALOGW("FAILED to autodetect media content.");
            return NULL;
        }
        mime = tmp.string();
    }

    MediaExtractor *ret = NULL;
    if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG4)
            || !strcasecmp(mime, "audio/mp4")) {
        ret = new MPEG4Extractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_MPEG)) {
        ret = new MP3Extractor(source, meta);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_NB)
            || !strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_WB)) {
        ret = new AMRExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_FLAC)) {
        ret = new FLACExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_WAV)) {
        ret = new WAVExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_OGG)) {
        ret = new OggExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MATROSKA)) {
        ret = new MatroskaExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG2TS)) {
        ret = new MPEG2TSExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AAC_ADTS)) {
        ret = new AACExtractor(source, meta);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG2PS)) {
        ret = new MPEG2PSExtractor(source);
    } else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_MIDI)) {
        ret = new MidiExtractor(source);
    }
	// 略掉了一些跟踪信息的代码
    return ret;
}
```

天哪噜，感觉又捅了一个马蜂窝，哎，源码真是轻易看不得啊啊啊啊啊啊。内容是真的多，感觉又可以单独拎出来，另起一篇了。

来看看MediaExtractor的类图吧。

###### ![nuplayer_datasource_classgraph](/Users/heli/github/ffmpeg-leaning/NuPlayer/mediaextractor_class_diagraph.png)

###### MediaExtractor::RegisterDefaultSniffers

```c++
// static
void MediaExtractor::RegisterDefaultSniffers() {
    Mutex::Autolock autoLock(gSnifferMutex);
    if (gSniffersRegistered) { // 只注册一次
        return;
    }
    RegisterSniffer_l(SniffMPEG4);
    RegisterSniffer_l(SniffMatroska);
    RegisterSniffer_l(SniffOgg);
    RegisterSniffer_l(SniffWAV);
    RegisterSniffer_l(SniffFLAC);
    RegisterSniffer_l(SniffAMR);
    RegisterSniffer_l(SniffMPEG2TS);
    RegisterSniffer_l(SniffMP3);
    RegisterSniffer_l(SniffAAC);
    RegisterSniffer_l(SniffMPEG2PS);
    RegisterSniffer_l(SniffMidi);
    gSniffersRegistered = true;
}
```

`snif`f的意思是“嗅、用鼻子吸“，`sniffer`可以翻译为”嗅探器“。所以，该函数是**注册默认嗅探器**的意思。

在媒体框架中，**嗅探器**又是个什么概念呢？

这里的**嗅探**，实际上是对媒体输入源进行文件头的读取，根据文件头内容嗅探出需要什么样的解封装组件，也就是不同的`MediaExtractor`实现。通过类图，我们也可以知道MediaExtractor有大量的实现，分别针对MP3、AAC、OGG、WAV、MPEG4等格式输入的解封装操作。

回到代码上，函数体中，通过`RegisterSniffer_l`函数，将不同解封装器的**嗅探函数指针**保存到了列表`gSniffers`中。显然，针对于不同封装格式的解封装器，嗅探函数也是不一样的，当然需要对应的解封装器自己实现。

具体**注册嗅探器**的代码如下：

```c++
List<MediaExtractor::SnifferFunc> MediaExtractor::gSniffers;
// static
void MediaExtractor::RegisterSniffer_l(SnifferFunc func) {
    for (List<SnifferFunc>::iterator it = gSniffers.begin();
         it != gSniffers.end(); ++it) {
        if (*it == func) {
            return;
        }
    }
    gSniffers.push_back(func);
}
```

###### MediaExtractor::sniff

```c++
// static
bool MediaExtractor::sniff(
        const sp<DataSource> &source, String8 *mimeType, float *confidence, sp<AMessage> *meta) {
    *mimeType = "";
    *confidence = 0.0f;
    meta->clear();
    {
        Mutex::Autolock autoLock(gSnifferMutex);
        if (!gSniffersRegistered) { // 在“嗅探”之前必须已经注册了嗅探器
            return false;
        }
    }
    for (List<SnifferFunc>::iterator it = gSniffers.begin();
         it != gSniffers.end(); ++it) { // 遍历所有嗅探器
        String8 newMimeType;
        float newConfidence;
        sp<AMessage> newMeta;
        if ((*it)(source, &newMimeType, &newConfidence, &newMeta)) { // 调用嗅探器的嗅探函数
            if (newConfidence > *confidence) {
                *mimeType = newMimeType;
                *confidence = newConfidence;
                *meta = newMeta;
            }
        }
    }
    return *confidence > 0.0;
}
```

嗅探器的实现原理基本上都是读取媒体源文件的头信息，不同格式都会有自己的特征，嗅探器就是根据这些特征，来判断是否是需要找的类型。

**嗅探函数的目的**，就是判断源文件(码流)类型是否和当前格式匹配。

说到**嗅探函数**的实现，必然会涉及到各种编码格式的特征，鉴于这部分内容实在太多，就不进一步详细分析了。

这里就简单的说一下几个参数的意义：

* source：这是个`DataSource`类型的指针，该类型通过层层包裹包含了一系列读取媒体源文件的功能。嗅探函数通过该指针通源文件中读取头信息，来判断源文件的类型。

* newMimeType：`String8`类型的指针，一旦嗅探函数通过头信息探测出源文件属于当前类型，该变量会通过指针赋值。这些类型定义在`MediaDefs.cpp`中如：

  ```c++
  const char *MEDIA_MIMETYPE_IMAGE_JPEG = "image/jpeg";
  const char *MEDIA_MIMETYPE_VIDEO_HEVC = "video/hevc";
  const char *MEDIA_MIMETYPE_VIDEO_MPEG2 = "video/mpeg2";
  
  const char *MEDIA_MIMETYPE_AUDIO_MPEG = "audio/mpeg";
  const char *MEDIA_MIMETYPE_AUDIO_FLAC = "audio/flac";
  const char *MEDIA_MIMETYPE_AUDIO_AC3 = "audio/ac3";
  const char *MEDIA_MIMETYPE_AUDIO_EAC3 = "audio/eac3";
  
  const char *MEDIA_MIMETYPE_CONTAINER_MPEG4 = "video/mp4";
  const char *MEDIA_MIMETYPE_CONTAINER_AVI = "video/avi";
  // ......................此处略去一万字
  ```

* newConfidence：`float`类型指针，一旦嗅探函数通过头信息探测出源文件属于当前类型，该变量会通过指针赋值。该值的意思是**“信心”**，每个判断了是自己类型的函数，都会给出对于源文件类型的判断的**信心值**，然后通过比较，信心值最大的类型判断获胜，该源文件便会被判定为该类型。例如：SniffAAC对自己的信心值为：0.2、SniffMPEG4：0.4、SniffMatroska：0.6、SniffOgg：0.2等。

* newMeta：这是一个AMessage对象，用于将嗅探结果（一些和格式相关的头信息）传递给调用者。对于不同格式传递的类型会不一样。

  SniffMPEG4传递的是：文件头结束的偏移量

  ```c++
  if (moovAtomEndOffset >= 0) { mpeg4
      *meta = new AMessage;
      (*meta)->setInt64("meta-data-size", moovAtomEndOffset);
  }
  ```

  SniffAAC传递：也是文件头结束的偏移位置。

  ```c++
  *meta = new AMessage; aac
  (*meta)->setInt64("offset", pos);
  ```

  SniffMP3：有文件头结束的偏移量，也有特有的格式位置信息。

  ```c++
  *meta = new AMessage;
  (*meta)->setInt64("offset", pos);
  (*meta)->setInt32("header", header);
  (*meta)->setInt64("post-id3-offset", post_id3_pos);
  ```

最后说一下整个嗅探函数的返回值`return *confidence > 0.0;`只需要信息之大于0.0就返回`true`，说明已经嗅探到格式信息。

一般来说，支持嗅探的格式越多，失败的可能越小。Android默认支持的类型已经足够普通使用了，所以，分析的时候我就当它返回`true`了。

回到`MediaExtractor::CreateFromService`中，经过嗅探函数对源文件进行嗅探后，基本能够确定源文件的类型，并把嗅探出来的`newMimeType`字符串的指针赋值给`mime`，最终通过在CreateFromService对该类型进行比较，创建对应类型的XXXExtractor。代码如下：

###### 创建指定格式的Extractor

```c++
MediaExtractor *ret = NULL;
if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG4)
    || !strcasecmp(mime, "audio/mp4")) {
    ret = new MPEG4Extractor(source);
} else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_MPEG)) {
    ret = new MP3Extractor(source, meta);
} else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_NB)
           || !strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AMR_WB)) {
    ret = new AMRExtractor(source);
} else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_FLAC)) {
    ret = new FLACExtractor(source);
} else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_WAV)) {
    ret = new WAVExtractor(source);
} else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_OGG)) {
    ret = new OggExtractor(source);
} else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MATROSKA)) {
    ret = new MatroskaExtractor(source);
} else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG2TS)) {
    ret = new MPEG2TSExtractor(source);
} else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_AAC_ADTS)) {
    ret = new AACExtractor(source, meta);
} else if (!strcasecmp(mime, MEDIA_MIMETYPE_CONTAINER_MPEG2PS)) {
    ret = new MPEG2PSExtractor(source);
} else if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_MIDI)) {
    ret = new MidiExtractor(source);
}
```

创建完指定的`MediaExtractor`之后，还需要将刚刚创建的`XXXExtractor`注册一下：

```c++
if (ret != NULL) {
    registerMediaExtractor(ret, localSource, mime);
}
```
###### registerMediaExtractor

```c++
void IMediaExtractor::registerMediaExtractor(
        const sp<IMediaExtractor> &extractor,
        const sp<DataSource> &source,
        const char *mime) {
    ExtractorInstance ex;
    ex.mime = mime == NULL ? "NULL" : mime;
    ex.name = extractor->name();
    ex.sourceDescription = source->toString();
    ex.owner = IPCThreadState::self()->getCallingPid();
    ex.extractor = extractor;
    // ...
        sExtractors.push_front(ex);
    // ...
}
```

该函数很短，作用也很简单，直接将闯入的`XXXExtractor`实例用`ExtractorInstance`包装一下，存放在`sExtractors`（一个`vector`）队首中。方便以后查询。

至此`IMediaExtractor`创建工作才算完成。

###### 小结

简单小结一下创建过程中都做了那些值得注意的事：

1. 将之前流程中创建的`FileSource`对象，通过包装成了一个`DataSource`对象。
2. 注册各种格式`Extractor`的嗅探函数。
3. 通过调用嗅探函数，利用`DataSource`读取媒体文件头，并分析媒体文件是何种格式。
4. 根据媒体文件格式，创建对应格式的`XXXExtractor`。

####初始化媒体源基本参数：mFileMeta、mDurationUs

`initFromDataSource`函数通过千辛万苦创建了`Extractor`后，任务其实已经完成了一大半了。后面都是一些从`Extractor`实例后的对象中拿数据进行填充的过程。

接着看看后面初始化`mFileMeta`和`mDurationUs`的调用

```c++
mFileMeta = extractor->getMetaData();
if (mFileMeta != NULL) {
    int64_t duration;
    if (mFileMeta->findInt64(kKeyDuration, &duration)) {
        mDurationUs = duration;
    }
}
```

第一行代码，通过extractor调用getMetaData获取文件的元数据（metadata）。对于getMetaData函数的实现，不同类型的Extractor会有不同的实现手段，例如：

```c++
sp<MetaData> MPEG4Extractor::getMetaData() {
    status_t err;
    if ((err = readMetaData()) != OK) { // 从源文件中读取MetaData信息，初始化mFileMetaData
        return new MetaData;
    }
    return mFileMetaData; // 将MetaData 返回给调用者
}
sp<MetaData> MP3Extractor::getMetaData() {
    sp<MetaData> meta = new MetaData; // 直接new
    if (mInitCheck != OK) {
        return meta;
    }
	// ...
    meta->setCString(kKeyMIMEType, "audio/mpeg");
    // ...
        meta->setCString(kMap[i].key, s);
    // ...
        meta->setData(kKeyAlbumArt, MetaData::TYPE_NONE, data, dataSize);
        meta->setCString(kKeyAlbumArtMIME, mime.string());
    return meta;
}
```

`MPEG4Extractor`和`MP3Extractor`的`getMetaData`函数实现就大为不同，不能再展开了。这里只顺便提及一下什么是元数据（MetaData）：

对于媒体文件而言，元数据一般有：音频采样率、视频帧率、视频尺寸、比特率、编解码、播放时长等基本信息，此外也可能含有其它杂七杂八的信息：名称、版权、专辑、时间、艺术家等。

#### 初始化媒体源基本参数：mMimes、mSources、mBitrate

下面这些代码就不一一解读了，啥都不用说，都在注释里。

```c++
int32_t totalBitrate = 0;
size_t numtracks = extractor->countTracks(); // 获取媒体源中的轨道数量，通常为三个，音频、视频、字幕各一个
mMimes.clear(); // 清理掉mMime信息，准备装新的。
for (size_t i = 0; i < numtracks; ++i) { // 遍历轨道，将音视频轨道信息的mime添加到mMimes中
    sp<IMediaSource> track = extractor->getTrack(i); // 获取各轨道
    sp<MetaData> meta = extractor->getTrackMetaData(i); // 获取各轨道的元数据
    const char *mime;
    CHECK(meta->findCString(kKeyMIMEType, &mime));
    ALOGV("initFromDataSource track[%zu]: %s", i, mime);
    if (!strncasecmp(mime, "audio/", 6)) { // 音频轨道
        if (mAudioTrack.mSource == NULL) { // 初始化各种音频轨道信息
            mAudioTrack.mIndex = i;
            mAudioTrack.mSource = track;
            mAudioTrack.mPackets =
                new AnotherPacketSource(mAudioTrack.mSource->getFormat());
            if (!strcasecmp(mime, MEDIA_MIMETYPE_AUDIO_VORBIS)) {
                mAudioIsVorbis = true;
            } else {
                mAudioIsVorbis = false;
            }
            mMimes.add(String8(mime)); // 将音频轨道mime信息，添加到mMimes中
        }
    } else if (!strncasecmp(mime, "video/", 6)) { // 视频轨道
        if (mVideoTrack.mSource == NULL) { // 初始化各种视频轨道信息
            mVideoTrack.mIndex = i;
            mVideoTrack.mSource = track;
            mVideoTrack.mPackets =
                new AnotherPacketSource(mVideoTrack.mSource->getFormat());
            // video always at the beginning
            mMimes.insertAt(String8(mime), 0); // 将视频轨道mime信息，添加到mMimes队首
        }
    }
    mSources.push(track); // 将各轨道信息统一保存在保存在mSources中
    int64_t durationUs;
    if (meta->findInt64(kKeyDuration, &durationUs)) { // 获取媒体播放时长
        if (durationUs > mDurationUs) { // 将个轨道中最大的播放时长作为媒体文件的播放时长
            mDurationUs = durationUs;
        }
    }
	// 通比特率为各轨道比特率之和
    int32_t bitrate;
    if (totalBitrate >= 0 && meta->findInt32(kKeyBitRate, &bitrate)) {
        totalBitrate += bitrate;
    } else {
        totalBitrate = -1;
    }
}
mBitrate = totalBitrate; // 初始化比特率
```

扯了这么多，才把`initFromDataSource`搞完，赶紧看一下最后一个函数：

### finishPrepareAsync

```c++
void NuPlayer::GenericSource::finishPrepareAsync() {
    ALOGV("finishPrepareAsync");
    status_t err = startSources(); // 启动各资源对象
    // ....
    if (mIsStreaming) { // 通常为false
		// ....
    } else {
        notifyPrepared(); // 该函数几乎啥都没做。
    }
}
```

```c++
status_t NuPlayer::GenericSource::startSources() {
    // 在我们开始缓冲之前，立即启动所选的A / V曲目。
    // 如果我们将它延迟到start()，那么在准备期间缓冲的所有数据都将被浪费。
    // (并不是在start()开始执行后，才开始读取数据)
    if (mAudioTrack.mSource != NULL && mAudioTrack.mSource->start() != OK) { // 启动音频
        ALOGE("failed to start audio track!");
        return UNKNOWN_ERROR;
    }

    if (mVideoTrack.mSource != NULL && mVideoTrack.mSource->start() != OK) { // 启动视频
        ALOGE("failed to start video track!");
        return UNKNOWN_ERROR;
    }
    return OK;
}
```

除了注释部分的信息，关于`mSource->start()`我也没什么准备在本文补充了，如果以后有机会，我会写n篇来尽未尽之事。

### 小结prepareSync

好了，`prepareSync`函数算是告一段落。花了巨大的篇幅来描述，总要总结一波的。

1. 不管是通过直接创建，还是通过服务创建，总之拐弯抹角的创建了一个`FileSource`对象。并通过各种封装，达到不一样的用途。
2. 通过各种封装好的`FileSource`对象，读取并**嗅探**源文件的头信息，判断文件格式，并创建对应格式的`XXXExtractor`。
3. 通过创建好的`XXXExtractor`，初始化各种媒体相关字段，如：`mFileMeta`、`mDurationUs`、`mMimes`、`mSources`、`mBitrate`。
4. `mSources`中包含所有`track`信息，`track`中又包含了对应的流信息，通过这些信息，启动了音视频数据的读取。

## start：启动

 start在播放流程中的位置为：

1. GenericSource的创建

2. setDataSource：设置媒体源数据

3. prepareAsync：准备媒体数据

4. **start：启动**

5. stop&pause&resume：停止&暂停&恢复

现在我们来看看GenericSource是怎么参与到播放流程中的。

```c++
void NuPlayer::GenericSource::start() {
    ALOGI("start");
    mStopRead = false; // 启动播放时，自然要不暂停读取数据false掉
    if (mAudioTrack.mSource != NULL) { // 在prepareAsync中，已经赋值，自然不能为空
        postReadBuffer(MEDIA_TRACK_TYPE_AUDIO); 
    }
    if (mVideoTrack.mSource != NULL) { // 在prepareAsync中，已经赋值，自然不能为空
        postReadBuffer(MEDIA_TRACK_TYPE_VIDEO);
    }
    mStarted = true;
    (new AMessage(kWhatStart, this))->post();
}
```

postReadBuffer函数其实做的事情不多，就是把trackType一路向下异步传递，最后让`NuPlayer::GenericSource::readBuffer`摘桃子，调用链如下：

postReadBuffer ==> onMessageReceived ==> onReadBuffer ==> readBuffer

基本上没啥看头，跳过，直接来readBuffer

### NuPlayer::GenericSource::readBuffer

```c++
void NuPlayer::GenericSource::readBuffer(
        media_track_type trackType, int64_t seekTimeUs, MediaPlayerSeekMode mode,
        int64_t *actualTimeUs, bool formatChange) 
    if (mStopRead) {
        return;
    }
    Track *track;
    size_t maxBuffers = 1;
    switch (trackType) { // 根据track类型分配最大buffer，并初始化track
        case MEDIA_TRACK_TYPE_VIDEO: // 音频
            track = &mVideoTrack;
            maxBuffers = 8;  // 最大buffer值为64，太大的buffer值会导致不能流畅的执行seek操作。
            break;
        case MEDIA_TRACK_TYPE_AUDIO: // 视频
            track = &mAudioTrack;
            maxBuffers = 64; // 最大buffer值为64
            break;
        case MEDIA_TRACK_TYPE_SUBTITLE: // 字幕
            track = &mSubtitleTrack;
            break;
        // 篇幅有限，能省一行是一行
    }
	// 篇幅有限，能省一行是一行
    for (size_t numBuffers = 0; numBuffers < maxBuffers; ) {
        Vector<MediaBuffer *> mediaBuffers;
        status_t err = NO_ERROR;
		// 从文件中读取媒体数据，用于填充mediaBuffers
        if (couldReadMultiple) { // 这个值一般为true
            err = track->mSource->readMultiple(
                    &mediaBuffers, maxBuffers - numBuffers, &options);
        } else { // read函数其实最终也是调用了readMultiple，只是read的最大buffer数为1
            MediaBuffer *mbuf = NULL;
            err = track->mSource->read(&mbuf, &options); 
            if (err == OK && mbuf != NULL) {
                mediaBuffers.push_back(mbuf);
            }
        }
        size_t id = 0;
        size_t count = mediaBuffers.size();
        for (; id < count; ++id) { // 将所有刚才读到的MediaBuffer中的数据摘出来封装到mPackets中
            int64_t timeUs;
            MediaBuffer *mbuf = mediaBuffers[id];
            // 根据类型，通过mBufferingMonitor监视器更新状态
            if (trackType == MEDIA_TRACK_TYPE_AUDIO) { 
                mAudioTimeUs = timeUs;
                mBufferingMonitor->updateQueuedTime(true /* isAudio */, timeUs);
            } else if (trackType == MEDIA_TRACK_TYPE_VIDEO) {
                mVideoTimeUs = timeUs;
                mBufferingMonitor->updateQueuedTime(false /* isAudio */, timeUs);
            }
            // 根据类型，将MediaBuffer转换为ABuffer
            sp<ABuffer> buffer = mediaBufferToABuffer(mbuf, trackType);
			// 篇幅有限，能省一行是一行
            track->mPackets->queueAccessUnit(buffer); // 将buffer入队，等待播放
            formatChange = false;
            seeking = false;
            ++numBuffers;
        }
    }
}
```

除了`trackType`参数外，其它都是有默认参数的，在start调用链中，`readBuffer`只传入了这个参数。其它参数可以控制seek功能。代码其实比这个长多了，我删掉了些暂时不重要的seek、异常中断等逻辑。

能说的代码注释里说了，继续看一下`start`函数接下来发送的`kWhatResume`消息干了啥

```c++
case kWhatResume:
{
    mBufferingMonitor->restartPollBuffering(); // 只是让buffer监视器重新循环起来
    break;
}
```

### start小结

* start函数调用链中，最终的的就是readBuffer函数，该函数最终要的功能就是将各种类型的数据读取并解析到track的buffer队列中，等待播放。
* 需要注意的是：解封装模块的start函数和NuPlayer的start功能并不相同，NuPlayer的start函数是播放，而解封装模块的start函数则是加载数据，后者是前者的子集。

## stop&pause&resume：停止&暂停&恢复

 start在播放流程中的位置为：

1. GenericSource的创建

2. setDataSource：设置媒体源数据

3. prepareAsync：准备媒体数据

4. start：播放视频

5. **stop&pause&resume：停止&暂停&恢复**

```c++
void NuPlayer::GenericSource::stop() { // 停止
    mStarted = false;
}

void NuPlayer::GenericSource::pause() { // 暂停
    mStarted = false;
}

void NuPlayer::GenericSource::resume() { // 恢复
    mStarted = true;
    (new AMessage(kWhatResume, this))->post();
}
```

停止、暂停、恢复几个动作，相关函数中仅是改变mStarted，其它几乎什么事情都没做。

这有提醒了我解封装模块和播放器的区别：

* 播放器的暂停：表示的是暂停播放
* 解封装模块的暂停：表示暂停将读取并缓存好的数据提供给播放器，这一点同样适用于停止，回复和start则相反。

所以，不管是停止、暂停还是回复的函数，关键都不在函数本身，而在于mStarted变量对于向外提供数据的函数的影响，也就是`dequeueAccessUnit`。

```c++
status_t NuPlayer::GenericSource::dequeueAccessUnit(
        bool audio, sp<ABuffer> *accessUnit) {
    if (audio && !mStarted) { // 如果是音频，并且mStarted为false，则不提供数据，返回block
        return -EWOULDBLOCK;
    }
    // ...
}
```

该函数用于为播放器提供原始媒体数据，audio表示是否为音频，accessUnit则是需要填充的buffer指针。

可以看到，如果`GenericSource::stop()`或者`GenericSource::pause() `函数调用后，mStarted变为了false，那么播放器将无法得到媒体数据，也就无法播放了。

那么，有人问，如果是视频不就可以了么。是的，视频还是可以从该函数中获取数据，但对于播放器而言，视频和音频肯定是同时播放，如果没了音频，视频也不会**独活**的。

好了，解封装模块终于搞定了。妈呀！

# 解码模块

NuPlayer的解码模块相对比较简单，统一使用了一个基类NuPlayerDecoderBase管理，该类中包含了一个MediaCodec的对象，实际解码工作全靠MediaCodec。

> 如果你不会知道MediaCodec是什么，推荐去官网看看：[MediaCodec](https://developer.android.com/reference/android/media/MediaCodec?hl=en)

尽管解码工作都被`MediaCodec`接管，我还是会按照播放器的一般步骤，来分析一下`NuPlayerDecoderBase`。步骤如下：

一个Android播放器典型的播放步骤一般是：

1. 播放器创建。
2. 设置媒体源文件（本地文件路径、或者Uri）。
3. 准备媒体数据。
4. 播放视频。
5. 停止播放。

对应于解码模块，会稍微简单一些：

1. `NuPlayerDecoderBase`：解码器创建
2. `onInputBufferFetched`：填充数据到解码队列
3. `onRenderBuffer`：渲染解码后的数据
4. `~Decoder`：释放解码器

## 解码器创建：NuPlayerDecoderBase 

当前位置：

1. **`NuPlayerDecoderBase`：解码器创建**
2. `onInputBufferFetched`：填充数据到解码队列
3. `onRenderBuffer`：渲染解码后的数据
4. `~Decoder`：释放解码器

解码器创建的入口在NuPlayer的`NuPlayer::instantiateDecoder`函数调用时。NuPlayer在执行start函数后，会通过一系列调用链，触发该函数。来具体分析一下该函数。

```c++
// 参数部分：audio true调用者想要创建音频解码器， false 想要创建视频解码器
// 参数部分：*decoder 该函数最终会创建指定解码器，使用该函数将解码器对象地址提供给调用者
status_t NuPlayer::instantiateDecoder(
        bool audio, sp<DecoderBase> *decoder, bool checkAudioModeChange) {
    sp<AMessage> format = mSource->getFormat(audio); // 其实就是GenericSource中的MetaData
    format->setInt32("priority", 0 /* realtime */);

    if (!audio) { // 视频
        // 总要丢掉一些代码的
            mCCDecoder = new CCDecoder(ccNotify); // 创建字幕解码器
    }
	// 创建音/视频解码器
    if (audio) { // 音频
        // ...
            *decoder = new Decoder(notify, mSource, mPID, mUID, mRenderer);
        // ...
    } else { // 视频
		// ...
        *decoder = new Decoder(
                notify, mSource, mPID, mUID, mRenderer, mSurface, mCCDecoder);
		// ...
    }
    (*decoder)->init();
    (*decoder)->configure(format);

    if (!audio) { // 视频
        sp<AMessage> params = new AMessage();
        float rate = getFrameRate();
        if (rate > 0) {
            params->setFloat("frame-rate-total", rate);
        }
		// ...
        if (params->countEntries() > 0) {
            (*decoder)->setParameters(params);
        }
    }
    return OK;
}
```

删删减减去掉了大量代码，留下来我感兴趣的。

先来说说，Decoder实际上是继承于DecoderBase的。

Decoder前者的定义如下：

```c++
struct NuPlayer::Decoder : public DecoderBase {
    Decoder(const sp<AMessage> &notify,
            const sp<Source> &source,
            pid_t pid,
            uid_t uid,
            const sp<Renderer> &renderer = NULL,
            const sp<Surface> &surface = NULL,
            const sp<CCDecoder> &ccDecoder = NULL);
    // 自然又是删掉很多代码
protected:
    virtual ~Decoder();
```

DecoderBase的定义如下：

```c++
struct ABuffer;
struct MediaCodec;
class MediaBuffer;
class MediaCodecBuffer;
class Surface;
struct NuPlayer::DecoderBase : public AHandler {
    explicit DecoderBase(const sp<AMessage> &notify);
    void configure(const sp<AMessage> &format);
    void init();
    void setParameters(const sp<AMessage> &params);
protected:
    virtual ~DecoderBase();
    void stopLooper();
    virtual void onMessageReceived(const sp<AMessage> &msg);
    virtual void onConfigure(const sp<AMessage> &format) = 0;
    virtual void onSetParameters(const sp<AMessage> &params) = 0;
    virtual void onSetRenderer(const sp<Renderer> &renderer) = 0;
    virtual void onResume(bool notifyComplete) = 0;
    virtual void onFlush() = 0;
    virtual void onShutdown(bool notifyComplete) = 0;
};
```

可以从`DecoderBase`的实现看到，它包含了所有解码相关的接口，这些接口往往都和`MediaCodec`的接口直接相关。可见，它是处在解码的前沿阵地上的。

在`instantiateDecoder`函数中，创建音频和视频的解码器，参数略有不同，创建视频解码器是会多出一个`mSurface`，提供给`MediaCodec`以显示视频，`mCCDecoder`则是字幕相关解码器。来看一下解码器构建函数：

```c++
NuPlayer::Decoder::Decoder(
        const sp<AMessage> &notify,
        const sp<Source> &source,
        pid_t pid,
        uid_t uid,
        const sp<Renderer> &renderer,
        const sp<Surface> &surface,
        const sp<CCDecoder> &ccDecoder)
    : DecoderBase(notify),
      mSurface(surface), // 视频播放的surface实体
      mSource(source),
      mRenderer(renderer), // 渲染器
      mCCDecoder(ccDecoder), // 字幕解码器
      mIsAudio(true) { // 是否为音频
    mCodecLooper = new ALooper;
    mCodecLooper->setName("NPDecoder-CL");
    mCodecLooper->start(false, false, ANDROID_PRIORITY_AUDIO);
    mVideoTemporalLayerAggregateFps[0] = mFrameRateTotal;
}
```

构造函数基本上就是将传递进来的参数，直接保存到自己的各类变量中，功能后续使用。继续看一下接下来对解码器来说比较重要的调用。

再来看看关于解码器的第二各操作：`(*decoder)->init();`。

直接在`Decoder`类中查找`init()`并不能找到，因为`Decoder`继承与`DecoderBase`，所以这里执行的应该是`DecoderBase`的`init`函数：

```c++
void NuPlayer::DecoderBase::init() {
    mDecoderLooper->registerHandler(this);
}
```

`DecoderBase`的构造函数中，已经创建了一套`NativeHandler`体系，并将`Looper`启动，只是没有将`AHandler`的子类对象和`ALooper`绑定，知道`init()`函数执行后，这种绑定关系才算结束。也只有这样，`DecoderBase`中的`NativeHandle`r体系才能够正常工作。有关NativeHandler的详细信息，请参考：[NativeHandler系列（一）](https://blog.csdn.net/qq_25333681/article/details/89289411)

接下来看看和解码相关的最重要的一步操作：`(*decoder)->configure(format);`

`configure`函数实际上是在`DecoderBase`中实现，最终调用了`DecoderBase`的纯虚构函数：`onConfigure`，让它的子类去实现具体的配置方法：

```c++
void NuPlayer::Decoder::onConfigure(const sp<AMessage> &format) {
    AString mime;
    CHECK(format->findString("mime", &mime));
	// 根据需要创建的解码器类型创建解码器
    mCodec = MediaCodec::CreateByType(
            mCodecLooper, mime.c_str(), false /* encoder */, NULL /* err */, mPid, mUid);
    err = mCodec->configure(format, mSurface, crypto, 0 /* flags */); // 配置解码器
    sp<AMessage> reply = new AMessage(kWhatCodecNotify, this);
    mCodec->setCallback(reply); // 设置解码器回调
    err = mCodec->start(); // 启动解码器
}
```

从简化后的代码可以看到， 在`onConfigure`函数中，有关`MediaCodec`的调用都是比较经典的调用方式。分别有，`MediaCodec`的创建、配置、设置回调通知、启动解码器。

关于`MediaCodec`还有`buffer`的入队和出队以及释放函数，相信不久后在其它地方可以见到。

解码器创建部分暂时就这么多，小结一下：

### 小结解码器创建

* `Decoder`继承于`DecoderBase`，`DecoderBas`e基本上接管了解码工作所有的操作，通过纯虚构（抽象）函数来让子类，也就是`Decoder`来实现一些具体的操作。
* `DecoderBase`解码体系，都是通过`MediaCodec`来实现解码流程。有鉴于此，它的基本操作函数都是为了`MediaCodec`的服务。

## 填充数据到解码队列:onInputBufferFetched

当前位置：

1. `NuPlayerDecoderBase`：解码器创建
2. **`onInputBufferFetched`：填充数据到解码队列**
3. `onRenderBuffer`：渲染解码后的数据
4. `~Decoder`：释放解码器

当`MediaCode`创建并执行了`start`函数后，就已经在通过`mCodec->setCallback(reply)`提供的回调，不断地调用填充数据有关的逻辑，最后实现数据填充的地方是在onInputBufferFetched函数中：

```c++
bool NuPlayer::Decoder::onInputBufferFetched(const sp<AMessage> &msg) {
   size_t bufferIx;
   CHECK(msg->findSize("buffer-ix", &bufferIx));
   CHECK_LT(bufferIx, mInputBuffers.size());
   sp<MediaCodecBuffer> codecBuffer = mInputBuffers[bufferIx];

   sp<ABuffer> buffer;
   bool hasBuffer = msg->findBuffer("buffer", &buffer); // 填充通解封装模块获取的数据
   bool needsCopy = true; // 是否需要将数据拷贝给MediaCodec

   if (buffer == NULL /* includes !hasBuffer */) { // 如果已经没有buffer可以提供了。
       status_t err = mCodec->queueInputBuffer(bufferIx, 0, 0, 0, MediaCodec::BUFFER_FLAG_EOS);
		// ...
   } else { // 还有buffer
        if (needsCopy) { // 拷贝给MediaCodec
            // ...
            if (buffer->data() != NULL) {
                codecBuffer->setRange(0, buffer->size());
                // 拷贝到MediaCodec的buffer中
                memcpy(codecBuffer->data(), buffer->data(), buffer->size()); 
            }
        } // needsCopy

        status_t err;
        AString errorDetailMsg;
		// ...
            err = mCodec->queueInputBuffer( // 将buffer加入到MediaCodec的待解码队列中
                    bufferIx,
                    codecBuffer->offset(),
                    codecBuffer->size(),
                    timeUs,
                    flags,
                    &errorDetailMsg);
        // ...
    }   // buffer != NULL
    return true;
}
```

这个函数的核心，就是调用`MediaCodec`的`queueInputBuffer`函数，将填充好的`MediaCodecBuffer`添加到`MediaCodec`的输入队列中，等待解码。解释都放在注释里了。来看一下如何取数据的。

## 渲染解码后的数据:onRenderBuffer

当前位置：

1. `NuPlayerDecoderBase`：解码器创建
2. `onInputBufferFetched`：填充数据到解码队列
3. **`onRenderBuffer`：渲染解码后的数据**
4. `~Decoder`：释放解码器

`onRenderBuffer`的执行时机，和`onInputBufferFetched`几乎是同时的，当`MediaCodec`的解码`outputBuffer`队列中有数据时，就会通过回调通知播放器，执行对应的回调函数渲染数据。在NuPlayer这样的回调函数执行链条为：`NuPlayer::Decoder::onMessageReceived` ==> `handleAnOutputBuffer` ==> `NuPlayer::Decoder::onRenderBuffer`

最终执行取出解码数据并渲染的函数便是`onRenderBuffer`：

```c++
void NuPlayer::Decoder::onRenderBuffer(const sp<AMessage> &msg) {
    int32_t render;
    size_t bufferIx;
    CHECK(msg->findSize("buffer-ix", &bufferIx));
	ALOGE("onRenderBuffer");

    if (msg->findInt32("render", &render) && render) {
        int64_t timestampNs;
        CHECK(msg->findInt64("timestampNs", &timestampNs));
        // 触发播放音频数据
        err = mCodec->renderOutputBufferAndRelease(bufferIx, timestampNs);
    } else { // 播放视频数据
        mNumOutputFramesDropped += !mIsAudio;
        err = mCodec->releaseOutputBuffer(bufferIx);
    }
	// ...
}
```

##释放解码器:~Decoder

当前位置：

1. `NuPlayerDecoderBase`：解码器创建
2. `onInputBufferFetched`：填充数据到解码队列
3. `onRenderBuffer`：渲染解码后的数据
4. **`~Decoder`：释放解码器**

```c++
NuPlayer::Decoder::~Decoder() {
    // Need to stop looper first since mCodec could be accessed on the mDecoderLooper.
    stopLooper(); // 停止looper
    if (mCodec != NULL) {
        mCodec->release(); // release掉MediaCodec
    }
    releaseAndResetMediaBuffers(); // 清理buffer
}
```

# 渲染模块&音视频同步

渲染模块的作用是，将音频、视频数据安装一定的同步策略通过对应的设备输出。这是所有的播放器都不可或缺的模块。

`NuPlayer`的渲染类为`Renderer`，定义在NuPlayerRenderer.h文件中。它的主要功能有：

* 缓存数据
* 音频设备初始化&数据播放
* 视频数据播放
* 音视频同步功能

## 缓存数据

在表明缓存逻辑之前，先介绍一下`NuPlayerRenderer`缓存数据的结构：

```c++
struct QueueEntry {
    sp<MediaCodecBuffer> mBuffer; // 如果该字段不为NULL，则包含了真实数据
    sp<AMessage> mMeta; 
    sp<AMessage> mNotifyConsumed; // 如果该字段为NULL，则表示当前QueueEntry是最后一个（EOS）。
    size_t mOffset;
    status_t mFinalResult;
    int32_t mBufferOrdinal;
};
List<QueueEntry> mAudioQueue; // 用以缓存音频解码数据的队列，队列实体为QueueEntry
List<QueueEntry> mVideoQueue; // 用以缓存视频解码数据的队列，队列实体为QueueEntry
```
来看看逻辑部分，看两个队列是如何被填满的。

`NuPlayerRenderer`渲染器的创建是在解码模块初始化之前实现的，解码模块在实例化并启动（start）后，如果已经有了解码数据，通过一些列调用后，会调用到`NuPlayer::Renderer::onQueueBuffer`，将解码后的数据存放到缓存队列中去，调用链条如下：`NuPlayer::Decoder::onMessageReceived` ==> `handleAnOutputBuffer` ==> `NuPlayer:::Renderer::queueBuffer` ==> `NuPlayer::Renderer::onQueueBuffer`。

```c++
void NuPlayer::Renderer::onQueueBuffer(const sp<AMessage> &msg) {
    int32_t audio;
    CHECK(msg->findInt32("audio", &audio));
    if (audio) {
        mHasAudio = true; // 需要缓存的是解码后的音频数据
    } else {
        mHasVideo = true; // 需要缓存的是解码后的视频数据
    }
    if (mHasVideo) {
        if (mVideoScheduler == NULL) {
            mVideoScheduler = new VideoFrameScheduler(); // 用于调整视频渲染计划
            mVideoScheduler->init();
        }
    }
    sp<RefBase> obj;
    CHECK(msg->findObject("buffer", &obj));
    // 获取需要被缓存的解码数据
    sp<MediaCodecBuffer> buffer = static_cast<MediaCodecBuffer *>(obj.get());

    QueueEntry entry; // 创建队列实体对象，并将解码后的buffer传递进去
    entry.mBuffer = buffer;
    entry.mNotifyConsumed = notifyConsumed;
    entry.mOffset = 0;
    entry.mFinalResult = OK;
    entry.mBufferOrdinal = ++mTotalBuffersQueued; // 当前队列实体在队列中的序号

    if (audio) { // 音频
        Mutex::Autolock autoLock(mLock);
        mAudioQueue.push_back(entry); // 将包含了解码数据的队列实体添加到音频队列队尾。
        postDrainAudioQueue_l(); // 刷新/播放音频
    } else { // 视频
        mVideoQueue.push_back(entry); // 将包含了解码数据的队列实体添加到音频队列队尾。
        postDrainVideoQueue(); // 刷新/播放视频
    }

    sp<MediaCodecBuffer> firstAudioBuffer = (*mAudioQueue.begin()).mBuffer;
    sp<MediaCodecBuffer> firstVideoBuffer = (*mVideoQueue.begin()).mBuffer;
	// ...
    int64_t firstAudioTimeUs;
    int64_t firstVideoTimeUs;
    CHECK(firstAudioBuffer->meta()->findInt64("timeUs", &firstAudioTimeUs));
    CHECK(firstVideoBuffer->meta()->findInt64("timeUs", &firstVideoTimeUs));
	// 计算队列中第一帧视频和第一帧音频的时间差值
    int64_t diff = firstVideoTimeUs - firstAudioTimeUs; 
    ALOGV("queueDiff = %.2f secs", diff / 1E6);
    if (diff > 100000ll) {
        // 如果音频播放比视频播放的时间超前大于0.1秒，则丢弃掉音频数据
        (*mAudioQueue.begin()).mNotifyConsumed->post();
        mAudioQueue.erase(mAudioQueue.begin()); // 从音频队列中删掉队首音频数据
        return;
    }
    syncQueuesDone_l(); // 刷新/播放音视频数据
}
```

这里，对于音视频设备刷新和播放的函数并没有做太多的解读，留到下节来说。

##音频设备初始化&数据播放

###音频设备初始化

对于Android系统来说，音频的播放最终都绕不开AudioSink对象。NuPlayer中的AudioSink对象早在NuPlayer播放器创建时就已经创建，并传入NuPlayer体系中，可以回过头去看看**NuPlayer播放器创建**一节。

接下来在创建解码器的过程中，也就是NuPlayer::instantiateDecoder函数调用创建音频解码器的同时，会触发一系列对`AudioSink`的初始化和启动动作。调用链如下：

`NuPlayer::instantiateDecoder` ==> `NuPlayer::determineAudioModeChange` ==> `NuPlayer::tryOpenAudioSinkForOffload` ==> `NuPlayer::Renderer::openAudioSink` ==> `NuPlayer::Renderer::onOpenAudioSink`

```c++
status_t NuPlayer::Renderer::onOpenAudioSink(
        const sp<AMessage> &format,
        bool offloadOnly,
        bool hasVideo,
        uint32_t flags,
        bool isStreaming) {
    ALOGV("openAudioSink: offloadOnly(%d) offloadingAudio(%d)",
            offloadOnly, offloadingAudio());
    bool audioSinkChanged = false;
    int32_t numChannels;
    CHECK(format->findInt32("channel-count", &numChannels)); // 获取声道数

    int32_t sampleRate;
    CHECK(format->findInt32("sample-rate", &sampleRate)); // 获取采样率

    if (!offloadOnly && !offloadingAudio()) { // 非offload模式打开AudioSink
        audioSinkChanged = true;
        mAudioSink->close();
        mCurrentOffloadInfo = AUDIO_INFO_INITIALIZER;
        status_t err = mAudioSink->open( // 打开AudioSink
                    sampleRate, // 采样率
                    numChannels, // 声道数
                    (audio_channel_mask_t)channelMask,
                    AUDIO_FORMAT_PCM_16_BIT, // 音频格式
                    0 /* bufferCount - unused */,
                    mUseAudioCallback ? &NuPlayer::Renderer::AudioSinkCallback : NULL,
                    mUseAudioCallback ? this : NULL,
                    (audio_output_flags_t)pcmFlags,
                    NULL,
                    doNotReconnect,
                    frameCount);
       
        mCurrentPcmInfo = info;
        if (!mPaused) { // for preview mode, don't start if paused
            mAudioSink->start(); // 启动AudioSink
        }
    }
    mAudioTornDown = false;
    return OK;
}
```

在这个函数执行完启动AudioSink的操作后，只需要往AudioSink中写数据，音频数据便能够得到输出。

## 音频数据输出

音频数据输出的触发函数是`postDrainAudioQueue_l`，在**缓存数据**一节中分析`NuPlayer::Renderer::onQueueBuffer`函数执行时，当数据被缓存在音频队列后，`postDrainAudioQueue_l`便会执行，让数据最终写入到`AudioSink`中播放。而`postDrainAudioQueue_l`函数简单处理后，就通过Nativehandler机制，将调用传递到了`NuPlayer::Renderer::onMessageReceived`的`kWhatDrainAudioQueue ` case中：

```c++
 case kWhatDrainAudioQueue:
        {
            if (onDrainAudioQueue()) { // 真正往AudioSink中写数据的函数
                uint32_t numFramesPlayed;
                CHECK_EQ(mAudioSink->getPosition(&numFramesPlayed), (status_t)OK);
                uint32_t numFramesPendingPlayout = mNumFramesWritten - numFramesPlayed;

                // AudioSink已经缓存的可用于播放数据的时间长度
                int64_t delayUs = mAudioSink->msecsPerFrame()
                    * numFramesPendingPlayout * 1000ll;
                if (mPlaybackRate > 1.0f) {
                    delayUs /= mPlaybackRate; // 计算当前播放速度下的可播放时长
                }

                // 计算一半播放时长的延迟，刷新数据
                delayUs /= 2;
                postDrainAudioQueue_l(delayUs); // 重新调用刷新数据的循环
            }
            break;
        }
```

下面重点照顾一下真正往`AudioSink`中写数据的函数：

```c++
bool NuPlayer::Renderer::onDrainAudioQueue() {
	// ...
    uint32_t prevFramesWritten = mNumFramesWritten;
    while (!mAudioQueue.empty()) { // 如果音频的缓冲队列中还有数据，循环就不停止
        QueueEntry *entry = &*mAudioQueue.begin(); // 取出队首队列实体
		// ...
        mLastAudioBufferDrained = entry->mBufferOrdinal;
        size_t copy = entry->mBuffer->size() - entry->mOffset;
        // 写入AudioSink，此时应该能可以听到声音了。
        ssize_t written = mAudioSink->write(entry->mBuffer->data() + entry->mOffset,
                                            copy, false /* blocking */);
		// ...
            entry->mNotifyConsumed->post(); // 通知解码器数据已经消耗
            mAudioQueue.erase(mAudioQueue.begin()); // 从队列中删掉已经播放的数据实体
		// ...
    }

    // 计算我们是否需要重新安排另一次写入。
    bool reschedule = !mAudioQueue.empty() && (!mPaused
                || prevFramesWritten != mNumFramesWritten); // permit pause to fill 
    return reschedule;
}
```

函数看着很短，其实很长，有需要的，可以自己去研究一下。

##视频数据播放

视频数据输出的时机几乎和音频数据输出是一样的，即在播放器创建完成并启动后便开始了。区别只是，音频执行了`postDrainAudioQueue_l`，而视频执行的是：`postDrainVideoQueue`。

```c++
void NuPlayer::Renderer::postDrainVideoQueue() {
    QueueEntry &entry = *mVideoQueue.begin(); // 从队列中取数据
    sp<AMessage> msg = new AMessage(kWhatDrainVideoQueue, this);
    msg->post(delayUs > twoVsyncsUs ? delayUs - twoVsyncsUs : 0);
    mDrainVideoQueuePending = true;
}
```

这里的代码自然不会这么简单，我几乎全部删掉，这些被删掉的代码基本都是同步相关，我准备留在下一步讲。

回来看代码执行到哪儿了：

```c++
void NuPlayer::Renderer::onDrainVideoQueue() {
    QueueEntry *entry = &*mVideoQueue.begin();

    entry->mNotifyConsumed->setInt64("timestampNs", realTimeUs * 1000ll);
    entry->mNotifyConsumed->setInt32("render", !tooLate);
    entry->mNotifyConsumed->post(); // 通知解码器已经消耗数据
    mVideoQueue.erase(mVideoQueue.begin()); // 删掉已经处理的数据
    entry = NULL;

    if (!mPaused) {
        if (!mVideoRenderingStarted) {
            mVideoRenderingStarted = true;
            notifyVideoRenderingStart();
        }
        Mutex::Autolock autoLock(mLock);
        notifyIfMediaRenderingStarted_l(); // 向上层（播放器）通知渲染开始
    }
}
```

同样有删除了和同步相关的代码

可能有人有疑问，这里并没有类似于向AudioSink中写数据的操作啊！怎么就渲染了？

相较于音频而言，显示视频数据的设备(`Surface`)和`MediaCodec`高度绑定，这个函数能做的，只是将数据实体通过NativeHandler消息的机制，通过mNotifyConsumed传递给MediaCodec，告诉解码器就可以了。所以，在`entry->mNotifyConsumed->post()`函数执行后，回调函数将最终执行到`NuPlayer::Decoder::onRenderBuffer`随后便会播放。

##音视频同步功能

**音视频同步的目的是**：让音频数据和视频数据能够在同一时间输出到对应设备中去。

音视频同步对于任何一个播放器而言，都是重中之重，在实际环境中，音视频同步问题的Bug，也是音视频项目中出现的一类大问题。

在本小结，将从原理讲起，同时分析NuPlayer中关于同步部分的代码。

在音频和视频输出的相关部分，删除了很多有关音视频同步的代码，在这一节都会补上。

### 时间戳

因为音频、视频等数据在漫长的处理流程中，无法保证同时到达输出设备。为了达到**同时**的目的，就出现了**时间戳**的概念：标定一段数据流的解码、和在设备上的显示时间。接下来我会重点分析在设备上的显示时间，也就是通常所说的PTS时间。

### 参考时钟

**参考时钟**是一条线性递增的时间线，通常选择系统时钟来作为参考时钟。

在制作音频视频数据时，会根据参考时钟上的时间为每个数据块打上时间戳，以便在播放时可以再指定的时间输出。

在播放时，会从数据块中取出时间戳，对比当前参考时钟，进行策略性播放。这种策略可能是音频为基准、也可能是视频为基准。

### Android NuPlayer同步方案

音视频同步方案有很多，NuPlayer选择了最常用的一种：**音频同步**

**音频同步**的意思是：以音频数据的播放时间为参考时钟，视频数据根据音频数据的播放时间做参考，如果视频超前将会被延迟播放，如果落后将会被快速播放或者丢弃。

当然音视频同步只有在既有音频也有视频的情况下才成立，如果仅有其中一方，NuPlayer会按照它们自己的时间播放的。

接下来，我们回到NuPlayer的源码，来分析NuPlayer是如何做好音频同步方案的。

### NuPlayer同步实现

在分析音视频同步代码之前，先来看看一个比较重要的类`MediaClock`，它完成了参考时钟的功能。

#### MediaClock::媒体时钟

```c++
struct MediaClock : public RefBase {
    // 在暂停状态下，需要使用刚渲染帧的时间戳作为锚定时间。
    void updateAnchor(
            int64_t anchorTimeMediaUs,
            int64_t anchorTimeRealUs,
            int64_t maxTimeMediaUs = INT64_MAX);
    // 查询与实时| realUs |对应的媒体时间，并将结果保存到| outMediaUs |中。
    status_t getMediaTime(
            int64_t realUs,
            int64_t *outMediaUs,
            bool allowPastMaxTime = false) const;
	// 查询媒体时间对应的实时时间| targetMediaUs |。 结果保存在| outRealUs |中
    status_t getRealTimeFor(int64_t targetMediaUs, int64_t *outRealUs) const;
private:
    status_t getMediaTime_l(
            int64_t realUs,
            int64_t *outMediaUs,
            bool allowPastMaxTime) const;
    int64_t mAnchorTimeMediaUs; // 锚定媒体时间:数据块中的媒体时间
    int64_t mAnchorTimeRealUs; // 锚定显示时间：数据块的实时显示时间
    int64_t mMaxTimeMediaUs; // 最大媒体时间
    int64_t mStartingTimeMediaUs; // 开始播放时的媒体时间
    float mPlaybackRate; // 播放速率
    DISALLOW_EVIL_CONSTRUCTORS(MediaClock);
};
```

其中比较重要的就是几个时间、和处理时间的函数。下面逐个分析一下这几个函数。

#####updateAnchor

函数的作用是，将当前正在播放的时间更新的`MediaClock`中。

```c++
void MediaClock::updateAnchor(
        int64_t anchorTimeMediaUs, // 数据流的时间戳
        int64_t anchorTimeRealUs, // 计算出的媒体数据显示真实时间
        int64_t maxTimeMediaUs) { // 最大媒体时间

    int64_t nowUs = ALooper::GetNowUs(); // 获取当前系统时间
    int64_t nowMediaUs = // 重新计算数据显示的真实时间
        anchorTimeMediaUs + (nowUs - anchorTimeRealUs) * (double)mPlaybackRate;
    if (nowMediaUs < 0) { // 如果时间已经超过当前系统时间就不更新时间了
        ALOGW("reject anchor time since it leads to negative media time.");
        return;
    }
    if (maxTimeMediaUs != -1) {
        mMaxTimeMediaUs = maxTimeMediaUs;
    }
    if (mAnchorTimeRealUs != -1) {
        int64_t oldNowMediaUs =
            mAnchorTimeMediaUs + (nowUs - mAnchorTimeRealUs) * (double)mPlaybackRate;
        if (nowMediaUs < oldNowMediaUs
                && nowMediaUs > oldNowMediaUs - kAnchorFluctuationAllowedUs) {
            return;
        }
    }
    mAnchorTimeRealUs = nowUs; // 以当前时间更新播放时间
    mAnchorTimeMediaUs = nowMediaUs; // 以数据流的时间戳更新锚定媒体时间
}
```

#####getMediaTime

查询与实时| realUs |对应的媒体时间，并将结果保存到| outMediaUs |中。

```c++
status_t MediaClock::getMediaTime(
        int64_t realUs, int64_t *outMediaUs, bool allowPastMaxTime) const {
    if (outMediaUs == NULL) {
        return BAD_VALUE;
    }

    Mutex::Autolock autoLock(mLock);
    return getMediaTime_l(realUs, outMediaUs, allowPastMaxTime);
}

status_t MediaClock::getMediaTime_l(
        int64_t realUs, int64_t *outMediaUs, bool allowPastMaxTime) const {
    if (mAnchorTimeRealUs == -1) {
        return NO_INIT;
    }

    int64_t mediaUs = mAnchorTimeMediaUs
            + (realUs - mAnchorTimeRealUs) * (double)mPlaybackRate;
    if (mediaUs > mMaxTimeMediaUs && !allowPastMaxTime) {
        mediaUs = mMaxTimeMediaUs;
    }
    if (mediaUs < mStartingTimeMediaUs) {
        mediaUs = mStartingTimeMediaUs;
    }
    if (mediaUs < 0) {
        mediaUs = 0;
    }
    *outMediaUs = mediaUs;
    return OK;
}
```

##### getRealTimeFor

查询媒体时间对应的实时时间| targetMediaUs |。 结果保存在| outRealUs |中，通常被视频播放时调用查询视频数据真实的显示时间。

```c++
status_t MediaClock::getRealTimeFor(
        int64_t targetMediaUs, int64_t *outRealUs) const {
    int64_t nowUs = ALooper::GetNowUs();
    int64_t nowMediaUs;
    // 获取当前系统时间对应音频流的显示时间戳即当前音频流的真实播放位置
    status_t status = getMediaTime_l(nowUs, &nowMediaUs, true /* allowPastMaxTime */);
    if (status != OK) {
        return status;
    }
    // 视频流的显示时间 = （视频流的媒体时间 - 音频流的显示时间） * 播放速度 + 系统时间
    *outRealUs = (targetMediaUs - nowMediaUs) / (double)mPlaybackRate + nowUs;
    return OK;
}
```

```c++
status_t MediaClock::getMediaTime_l(
        int64_t realUs, int64_t *outMediaUs, bool allowPastMaxTime) const {
    // 媒体时间 = 锚点媒体时间 + （系统时间 - 锚点媒体时间）*播放速度
    int64_t mediaUs = mAnchorTimeMediaUs + (realUs - mAnchorTimeRealUs) * (double)mPlaybackRate;
    // 媒体时间，不能超过mMaxTimeMediaUs
    if (mediaUs > mMaxTimeMediaUs && !allowPastMaxTime) { 
        mediaUs = mMaxTimeMediaUs;
    }
    // 媒体时间，不能小于mMaxTimeMediaUs
    if (mediaUs < mStartingTimeMediaUs) {
        mediaUs = mStartingTimeMediaUs;
    }
    if (mediaUs < 0) {
        mediaUs = 0;
    }
    *outMediaUs = mediaUs;
    return OK;
}
```

#### 音视同步-音频

音频数据对音视同步中的贡献，就是提供自己的播放时间，用以更新`MediaClock`。

而音频数据播放的时间已经在**渲染模块—音频数据**输出一节中讲到，是在`NuPlayer::Renderer::onDrainAudioQueue()`函数中完成的。

```c++
bool NuPlayer::Renderer::onDrainAudioQueue() {
	// ...
    uint32_t prevFramesWritten = mNumFramesWritten;
    while (!mAudioQueue.empty()) { // 如果音频的缓冲队列中还有数据，循环就不停止
        QueueEntry *entry = &*mAudioQueue.begin(); // 取出队首队列实体
		// ...
        mLastAudioBufferDrained = entry->mBufferOrdinal;

        // ignore 0-sized buffer which could be EOS marker with no data
        if (entry->mOffset == 0 && entry->mBuffer->size() > 0) {
            int64_t mediaTimeUs; // 获取数据块的时间
            CHECK(entry->mBuffer->meta()->findInt64("timeUs", &mediaTimeUs));
            ALOGV("onDrainAudioQueue: rendering audio at media time %.2f secs",
                    mediaTimeUs / 1E6);
            onNewAudioMediaTime(mediaTimeUs); // 将新的媒体时间更新到MediaClock中
        }
        size_t copy = entry->mBuffer->size() - entry->mOffset;
        // 写入AudioSink，此时应该能可以听到声音了。
        ssize_t written = mAudioSink->write(entry->mBuffer->data() + entry->mOffset,
                                            copy, false /* blocking */);
		// ...
            entry->mNotifyConsumed->post(); // 通知解码器数据已经消耗
            mAudioQueue.erase(mAudioQueue.begin()); // 从队列中删掉已经播放的数据实体
		// ...
    }

    // 计算我们是否需要重新安排另一次写入。
    bool reschedule = !mAudioQueue.empty() && (!mPaused
                || prevFramesWritten != mNumFramesWritten); // permit pause to fill 
    return reschedule;
}
```

该函数中，关于播放的大部分内容已经在音频输出模块讲过了，现在重点关注一下音视频同步相关的函数：

```c++
void NuPlayer::Renderer::onNewAudioMediaTime(int64_t mediaTimeUs) {
    if (mediaTimeUs == mAnchorTimeMediaUs) {
        return;
    }
    setAudioFirstAnchorTimeIfNeeded_l(mediaTimeUs); // 通过第一次的媒体时间更新第一帧锚点媒体时间
    // 如果我们正在等待音频接收器启动，则mNextAudioClockUpdateTimeUs为-1
    if (mNextAudioClockUpdateTimeUs == -1) {
        AudioTimestamp ts;
        if (mAudioSink->getTimestamp(ts) == OK && ts.mPosition > 0) {
            mNextAudioClockUpdateTimeUs = 0; // 开始我们的时钟更新
        }
    }
    int64_t nowUs = ALooper::GetNowUs();
    if (mNextAudioClockUpdateTimeUs >= 0) { // 此时mNextAudioClockUpdateTimeUs = 0
        if (nowUs >= mNextAudioClockUpdateTimeUs) {
            // 将当前播放音频流时间戳、系统时间、音频流当前媒体时间戳更新到MediaClock
            int64_t nowMediaUs = mediaTimeUs - getPendingAudioPlayoutDurationUs(nowUs);
            mMediaClock->updateAnchor(nowMediaUs, nowUs, mediaTimeUs);
            mUseVirtualAudioSink = false;
            mNextAudioClockUpdateTimeUs = nowUs + kMinimumAudioClockUpdatePeriodUs;
        }
    }
    mAnchorNumFramesWritten = mNumFramesWritten;
    mAnchorTimeMediaUs = mediaTimeUs;
}
```

这部分的内容还是比较简单的。

#### 音视同步-视频

同样，涉及到同步的代码，和视频数据播放是放在一起的，在**渲染模块—视频数据播放**中已经提到过。重新拿出来分析音视同步部分的代码。

```c++
void NuPlayer::Renderer::postDrainVideoQueue() {
    QueueEntry &entry = *mVideoQueue.begin();

    sp<AMessage> msg = new AMessage(kWhatDrainVideoQueue, this);

    bool needRepostDrainVideoQueue = false;
    int64_t delayUs;
    int64_t nowUs = ALooper::GetNowUs();
    int64_t realTimeUs;
	if (mFlags & FLAG_REAL_TIME) {
        // ...
    } else {
        int64_t mediaTimeUs;
        CHECK(entry.mBuffer->meta()->findInt64("timeUs", &mediaTimeUs)); // 获取媒体时间
        {
            Mutex::Autolock autoLock(mLock);
             // mAnchorTimeMediaUs 该值会在onNewAudioMediaTime函数中，随着音频播放而更新
             // 它的值如果小于零的话，意味着没有音频数据
            if (mAnchorTimeMediaUs < 0) { // 没有音频数据，则使用视频将以系统时间为准播放
                // 只有视频的情况，使用媒体时间和系统时间更新MediaClock
                mMediaClock->updateAnchor(mediaTimeUs, nowUs, mediaTimeUs);
                mAnchorTimeMediaUs = mediaTimeUs;
                realTimeUs = nowUs;
            } else if (!mVideoSampleReceived) { // 没有收到视频帧 
                // 显示时间为当前系统时间，意味着一直显示第一帧
                realTimeUs = nowUs;
            } else if (mAudioFirstAnchorTimeMediaUs < 0
                || mMediaClock->getRealTimeFor(mediaTimeUs, &realTimeUs) == OK) { 
                // 一个正常的音视频数据，通常都走这里
                realTimeUs = getRealTimeUs(mediaTimeUs, nowUs); // 获取视频数据的显示事件
            } else if (mediaTimeUs - mAudioFirstAnchorTimeMediaUs >= 0) {
              	// 其它情况，视频的显示时间就是系统时间
                needRepostDrainVideoQueue = true;
                realTimeUs = nowUs;
            } else {
                realTimeUs = nowUs; // 其它情况，视频的显示时间就是系统时间
            }
        }
        if (!mHasAudio) { // 没有音频流的情况下，
            // 平滑的输出视频需要 >= 10fps, 所以，以当前视频流的媒体时间戳+100ms作为maxTimeMedia
            mMediaClock->updateMaxTimeMedia(mediaTimeUs + 100000);
        }

        delayUs = realTimeUs - nowUs; // 计算视频播放的延迟
        int64_t postDelayUs = -1;
        if (delayUs > 500000) { // 如果延迟超过500ms
            postDelayUs = 500000; // 将延迟时间设置为500ms
            if (mHasAudio && (mLastAudioBufferDrained - entry.mBufferOrdinal) <= 0) {、
                // 如果有音频，并且音频队列的还有未消耗的数据又有新数据增加，则将延迟时间设为10ms
                postDelayUs = 10000;
            }
        } else if (needRepostDrainVideoQueue) {
            postDelayUs = mediaTimeUs - mAudioFirstAnchorTimeMediaUs;
            postDelayUs /= mPlaybackRate;
        }

        if (postDelayUs >= 0) { // 以音频为基准，延迟时间通常都大于零
            msg->setWhat(kWhatPostDrainVideoQueue);
            msg->post(postDelayUs); // 延迟发送，播放视频数据
            mVideoScheduler->restart();
            mDrainVideoQueuePending = true;
            return;
        }
    }
    // 依据Vsync机制调整计算出两个Vsync信号之间的时间
    realTimeUs = mVideoScheduler->schedule(realTimeUs * 1000) / 1000;
    int64_t twoVsyncsUs = 2 * (mVideoScheduler->getVsyncPeriod() / 1000);
    delayUs = realTimeUs - nowUs;
    // 将Vsync信号的延迟时间考虑到视频播放指定的延迟时间中去
    msg->post(delayUs > twoVsyncsUs ? delayUs - twoVsyncsUs : 0);
    mDrainVideoQueuePending = true;
}
```

代码已经挺详细的了，其中提到了Vsync机制的概念。

在Android中，这是一种**垂直同步机制**，用于处理两个处理速度不同的模块存在。

为了使显示的数据正确且稳定，在视频播放过程中，有两种buffer的概念，一种是处理数据的buffer，一种是专门用于显示的buffer，前者由我们的程序提供，后者往往需要驱动程序支持。因为两者的处理速度不同，所以就使用了Vsync机制。详细的，请大家Google吧。

当执行msg->post之后，消息会在指定的延迟时间后，触发解码器给显示器提供视频数据。音视频也就完了。