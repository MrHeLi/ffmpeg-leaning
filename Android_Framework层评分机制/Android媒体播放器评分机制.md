

> 本文涉及源码版本为：Oreo 8.0.0_r4
>
> /frameworks/av/media/libmedia/mediaplayer.cpp
>
> /frameworks/av/include/media/mediaplayer.h
>
> /frameworks/av/media/libmediaplayerservice/MediaPlayerFactory.cpp
>
> /frameworks/av/media/libmediaplayerservice/MediaPlayerFactory.h

# 什么是评分机制

就媒体播放器而言，Android上层通常能够接触到的接口有两个：
1. `MediaCodec`: 对`MediaCodec`来说，它只向调用者提供编解码的接口。这意味着喂给它的数据，必须经过解封装处理才行。同时解码后的数据并没有音视频同步功能（tunnel模式除外）。
2. `MediaPlayer`：`MediaPlayer`则没有这么麻烦，使用时，只需要把媒体源（媒体文件路径或者Uri）直接设置给`MediaPlayer`配合prepare、start等函数调用就可以直接播放了，完全不需要关系同步、解封装等。

通常，我们用得比较多的是`MediaPlayer`，因为它简单方便，不需要开发者做太多的工作就能正常使用。和`MediaCodec`相比，它的缺点是不太灵活，扩展性不强。

实际上，`MediaCodec`和`MediaPlayer`并不是两种隔离的播放接口，`MediaPlayer`最终的解码还是会用到`MediaCodec`。你需要知道如下几点:

* `MediaCodec`是Android媒体数据相关编解码的基础接口。
* `MediaPlayer`是对`MediaCodec`解码能力的一种封装和扩展。
* `MediaPlayer`拥有`MediaCodec`不具备的解协议、同步方面的功能。

那么`MediaPlayer`和`MediaCodec`直接构成调用关系吗？

答案是否定的，它们之间还隔了一层framework的播放器，我想用一张图来说明这其中的关系。

![](https://github.com/MrHeLi/ffmpeg-leaning/blob/master/image/Android%E5%AA%92%E4%BD%93%E8%A7%A3%E7%A0%81%E5%85%B3%E7%B3%BB.png)

嗯，没错，大概就是这样了。

你看到的`MediaCodec`和`MediaPlayer`都是成对出现的，应用层的，都只是一层壳，方便让应用层直接调用framework层的`MediaPlayer`和`MediaCodec`。

上层和底层之间，通过JNI调用链接。PS：如果对Android中的JNI有想法，可以转到：[Android JNI 详解](<https://blog.csdn.net/qq_25333681/article/details/80919590>)阅读。

图中，不管是上层的`MediaPlayer`还是framework层的`MediaPlayer`都不是不能说是真正的播放器。真正播放器是底层的`NuPlayer`和`TestPlayer`。

>  Android的历史上还出现过`StagefrightPlayer`等其它的播放器，但在Android7.0后，这些播放器代码都被删掉了。

那么图中的评分机制是什么意思呢？

当应用层的`MediaPlayer`通过JNI调用了framework层的`MediaPlayer`，最终调用到`MediaPlayerService`。`MediaPlayerService`根据**评分机制**，比较`NuPlayer`和`TestPlayer`的得分，实例化得分最高的。这便是最终使用的底层播放器。

现在可以回答标题问题了：

评分机制用于`MediaPlayer`播放中，选择最终使用哪个底层播放器。

# 上层MediaPlayer实例

为了不使后面的内容显得突兀，先放一点上层MediaPlayer使用的实例代码：

> 代码来自[MediaPlayer源码分析](https://blog.csdn.net/qq_25333681/article/details/82056184)，对于MediaPlayer整体调用逻辑感兴趣的朋友，建议详读。

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

从代码中，可以提炼出以下关键调用点：

1. player = new MediaPlayer()
2. player.setDataSource(path)
3. player.start()

结合前文，总结一下[MediaPlayer源码分析](https://blog.csdn.net/qq_25333681/article/details/82056184)对这一点的分析：

1. new MediaPlayer()：只是创建了一个MediaPlayer对象，遗憾的是MediaPlayer并不是一个真正的播放器。
2. setDataSource(path)：这里才是创建了具体播放器的地方。
3. start()：播放。

后文主要分析播放器创建过程中，**评分机制**的逻辑。

# 播放器选择

从应用层`MediaPlayer`创建开始，到`MediaPlayer`调用`setDataSource`函数，最终调用到`MediaPlayerService::Client::setDataSource`函数，整个过程都是一个参数透传的过程。这里就不细讲了，确实想了解的可以去[MediaPlayer源码分析](https://blog.csdn.net/qq_25333681/article/details/82056184)看看。

这一小节，主要分析`setDataSource`中关于**评分机制**的部分。

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

看到代码中的`player_type`了么，它就是要分析的重点。这里涉及调用的是`MediaPlayerFactory::getPlayerType`来获取最终类型。

考虑到扩展性，google工程师们使用了工厂模式，来管理播放器的创建和**评分机制**。这样，出了Android自己的内置播放器，各大Rom厂商也可以实现自己的播放器。我们来看看播放器相关工厂类的类图。

![](https://github.com/MrHeLi/ffmpeg-leaning/blob/master/image/%E6%92%AD%E6%94%BE%E5%99%A8%E5%B7%A5%E5%8E%82.png)

播放器的创建，会使用`MediaPlayerFactory`的`createPlayer`函数。`MediaPlayerFactory`的`createPlayer`函数中，会根据片源情况，调用注册在`tFactoryMap`中的各个`IFactory`实现，获取对应的分值，获得最大分值的工厂类，调用该工厂类的`createPlayer`函数，最终创建出指定的播放器。

让我们从注册工厂类开始说起吧。

## 注册播放器工厂

注册播放器工厂的目的，是初始化`tFactoryMap`，以便通过打分机制比较出最优的播放器，最终创建出对应的播放器。

初次调用注册播放器工厂的函数是在`MediaPlayerService`的构造器中，`MediaPlayerService`作为基础服务，在Android世界启动的时候，便会调用。

```c++
MediaPlayerService::MediaPlayerService()
{
    ALOGV("MediaPlayerService created");
    mNextConnId = 1;
    MediaPlayerFactory::registerBuiltinFactories();
}
```

看一下具体实现：

```c++
void MediaPlayerFactory::registerBuiltinFactories() {
	...
    registerFactory_l(new NuPlayerFactory(), NU_PLAYER);
    registerFactory_l(new TestPlayerFactory(), TEST_PLAYER);
	...
}
```

先来看看参数部分的播放器类型定义：

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

我们前面提到过stagefrightplayer，在Android7.0后源码就被删掉了，但是在`player_type`中依然保留了它对应的类型。另一个有意思的地方是`TEST_PLAYER`的注释部分，简单翻译一下是这样的：

测试播放器只能在'test'和'eng'版本中使用。 当setDataSource参数中带有'test：'开头的url将使用带有测试播放器的共享库。

所以，当你的参数以'test:'开头时，会不会最终使用到`testplayer`呢？(#^.^#)

最后，来看`registerFactory_l`的实现。

```c++
typedef KeyedVector<player_type, IFactory*> tFactoryMap;
static tFactoryMap sFactoryMap;
--------------------------------↑头文件中|↓cpp文件中----------------------------------------
status_t MediaPlayerFactory::registerFactory_l(IFactory* factory,
                                               player_type type) {
	//.... 略去一些入参校验逻辑
    if (sFactoryMap.add(type, factory) < 0) {
        ALOGE("Failed to register MediaPlayerFactory of type %d, failed to add"
              " to map.", type);
        return UNKNOWN_ERROR;
    }
    return OK;
}
```

参数部分：

* factory：是调用时创建的`NuPlayerFactory`和`TestPlayerFactory`。
* type：`NuPlayerFactory`对应的player_type是NU_PLAYER， `TestPlayerFactory`对应的是TEST_PLAYER。

而`registerFactory_l`函数实际上就是将播放器类型作为key，具体的播放器工厂作为值存储在`tFactoryMap`中。

## MediaPlayerFactory::getPlayerType

回到工厂方法的`getPlayerType`函数：

```c++
player_type MediaPlayerFactory::getPlayerType(const sp<IMediaPlayer>& client,
                                              const char* url) {
    GET_PLAYER_TYPE_IMPL(client, url);
}
player_type MediaPlayerFactory::getPlayerType(const sp<IMediaPlayer>& client,
                                              int fd,
                                              int64_t offset,
                                              int64_t length) {
    GET_PLAYER_TYPE_IMPL(client, fd, offset, length);
}
player_type MediaPlayerFactory::getPlayerType(const sp<IMediaPlayer>& client,
                                              const sp<IStreamSource> &source) {
    GET_PLAYER_TYPE_IMPL(client, source);
}
player_type MediaPlayerFactory::getPlayerType(const sp<IMediaPlayer>& client,
                                              const sp<DataSource> &source) {
    GET_PLAYER_TYPE_IMPL(client, source);
}
```

该函数有多个重载类型，后面会展开第一和第二个，另外两个有兴趣可以自己研究一下。因为他们都使用了同一个宏展开，原理类似。先看一下缩减版本的宏定义。

```c++
#define GET_PLAYER_TYPE_IMPL(a...)                      \
    player_type ret = STAGEFRIGHT_PLAYER;               \
    float bestScore = 0.0;                              \
    for (size_t i = 0; i < sFactoryMap.size(); ++i) {   \
        IFactory* v = sFactoryMap.valueAt(i);           \
        float thisScore;                                \
        CHECK(v != NULL);                               \
        thisScore = v->scoreFactory(a, bestScore);      \
        if (thisScore > bestScore) {                    \
            ret = sFactoryMap.keyAt(i);                 \
            bestScore = thisScore;                      \
        }                                               \
    }                                                   \
    if (0.0 == bestScore) {                             \
        ret = getDefaultPlayerType();                   \
    }                                                   \
    return ret;
```

对第一个`getPlayerType`函数进行展开：

```c++
player_type MediaPlayerFactory::getPlayerType(const sp<IMediaPlayer>& client,
                                              const char* url) {
    player_type ret = STAGEFRIGHT_PLAYER; // 默认播放器类型为STAGEFRIGHT_PLAYER，真是这样吗？ 
    float bestScore = 0.0; // 最高得分                              
    for (size_t i = 0; i < sFactoryMap.size(); ++i) { // 遍历sFactoryMap
        IFactory* v = sFactoryMap.valueAt(i);           
        float thisScore; // 当前分数                                
        CHECK(v != NULL);                               
        thisScore = v->scoreFactory(a, bestScore); // 这里的a是一个参数数组，包含所有getPlayerType的入参      
        if (thisScore > bestScore) { // 如果当前分数比最高得分大，则替换掉最高得分 
            ret = sFactoryMap.keyAt(i); // 同时将播放器类型交给ret以便返回              
            bestScore = thisScore;                      
        }                                               
    }                                                   
    if (0.0 == bestScore) { // 如果前面打分全部为0.0，默认类型靠getDefaultPlayerType获得，所以说前面那个STAGEFRIGHT_PLAYER并没什么用
        ret = getDefaultPlayerType();                   
    }                                                   
    return ret;
}
```

虽然这里只展开了第一个函数，但因为使用了同样的宏定义，并且参数数组a并没有在该函数中处理，而是透传到具体的工厂函数scoreFactory中去了。所以，就目前而言，四个重载函数并没有什么区别。

### 小结一下getPlayerType

该函数有两个比较重要调用：

1. `v->scoreFactory(a, bestScore)`：调用对应工厂方法中的scoreFactory函数，返回工厂的分数。
2. `getDefaultPlayerType`：获取默认的播放器类型。

所以，总结一下就是：通过循环`sFactoryMap`中的播放器工厂，调用工厂方法`scoreFactory`后，得到一个分数。取得分最大的类型，这就是我们要找的类型。如果到最后，得分还是为0.0则使用默认的类型。

在这里，工厂函数`scoreFactory`是核心，我们来看看它。

## scoreFactory

回顾一下相关类图：

![](https://github.com/MrHeLi/ffmpeg-leaning/blob/master/image/%E6%92%AD%E6%94%BE%E5%99%A8%E5%B7%A5%E5%8E%82.png)

`scoreFactory`函数是抽象类`IFactory`中的非抽象函数：

```c++
class IFactory {
    public:
    virtual ~IFactory() { }
    virtual float scoreFactory(const sp<IMediaPlayer>& /*client*/,
                               const char* /*url*/,
                               float /*curScore*/) { return 0.0; }
    virtual float scoreFactory(const sp<IMediaPlayer>& /*client*/,
                               int /*fd*/,
                               int64_t /*offset*/,
                               int64_t /*length*/,
                               float /*curScore*/) { return 0.0; }
    virtual float scoreFactory(const sp<IMediaPlayer>& /*client*/,
                               const sp<IStreamSource> &/*source*/,
                               float /*curScore*/) { return 0.0; }
    virtual float scoreFactory(const sp<IMediaPlayer>& /*client*/,
                               const sp<DataSource> &/*source*/,
                               float /*curScore*/) { return 0.0; }
    virtual sp<MediaPlayerBase> createPlayer(pid_t pid) = 0;
};
```

对应于`MediaPlayerFactory`中的四个`getPlayerTyp`e函数，`IFactory`也有四个重载的`scoreFactory`函数。并且每个重载函数都已经有了现成实现，咳咳……虽然直接返回0.0，好歹也是个实现不是。这意味着继承类可以不用重写`scoreFactory`便可直接调用。

`NuPlayerFactory`和`TestPlayerFactory`都继承了`IFactory` ，来看看他们继承过来后都实现了啥。

## NuPlayerFactory

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
    virtual float scoreFactory(const sp<IMediaPlayer>& /*client*/,
                               const sp<IStreamSource>& /*source*/,
                               float /*curScore*/) {
        return 1.0;
    }
    virtual float scoreFactory(const sp<IMediaPlayer>& /*client*/,
                               const sp<DataSource>& /*source*/,
                               float /*curScore*/) {
        // 只有NuPlayer直接支持设置一个DataSource的播放源
        return 1.0;
    }
    virtual sp<MediaPlayerBase> createPlayer(pid_t pid) {
        ALOGV(" create NuPlayer");
        return new NuPlayerDriver(pid);
    }
};
```

这么长的代码，这简直就是罪过啊啊啊啊

但还是一个个分析一下吧:

NuPlayerFactory重写了三个scoreFactory函数，先来看看最简单的两个：

```c++
virtual float scoreFactory(const sp<IMediaPlayer>&, const sp<DataSource>&, float) 
virtual float scoreFactory(const sp<IMediaPlayer>&, const sp<IStreamSource>&, float)
```

这两哥们连形参都不要了，直接在函数体里返回一个1.0。显得很是草率，对于入参中包含**char* url**类型的函数重写，却显得格外“尊重”。

```c++
virtual float scoreFactory(const sp<IMediaPlayer>& /*client*/,
                               const char* url,
                               float curScore) { // 入参部分的curScore为调用函数传入的最大分数
        static const float kOurScore = 0.8; // 我们的分数是0.8分
        if (kOurScore <= curScore) // 如果自己的分数小于调用者的最大分数，直接返回0.0不和别人抢机会
            return 0.0;
        if (!strncasecmp("http://", url, 7)
                || !strncasecmp("https://", url, 8)
                || !strncasecmp("file://", url, 7)) { // 判断各种条件是否满足，满足则返回自己的分数
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
        if (!strncasecmp("rtsp://", url, 7)) { // 前面都不满足，最后挣扎一把，看是不是"rtsp://"
            return kOurScore;
        }
        return 0.0;
    }
```

代码中的注释，可以看个大概，用文字整理一下：

* `NuPlayerFactory`有个自己的分数`kOurScore = 0.8`如果url满足一定的条件就把这个分数返回给调用者。
* url需要满足的条件，可以视为NuPlayer所能支持的能力。
  1. url如果是以字符串"rtsp://"开头。
  2. url以字符串"http://"、"https://"、"file://"开头，且以".m3u8"结尾。
  3. url以字符串"http://"、"https://"、"file://"开头，且包含".m3u8"。
  4. url以字符串"http://"、"https://"、"file://"开头，且以".sdp"结尾，或者url中包含".sdp?"。
  5. 只要满足上述四种条件，那么得分都为0.8，否则为0.0。

用一个图来描述一下这个过程。

![](https://github.com/MrHeLi/ffmpeg-leaning/blob/master/image/%E8%AF%84%E5%88%86%E6%9C%BA%E5%88%B6.png)

## TestPlayerFactory

```c++
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

`TestPlayerFactory`的代码让人看起来，舒服了很多。

也是通过url判断是否返回自己的分值，这里分值为1.0。

在TestPlayer的类型注释里，也提到了，只有在测试版本中才可能用到这个播放器，所以这里也不展开研究了。

# 总结

* 整个打分机制设计的核心类有`MediaPlayerFactory`、`NuPlayerFactory`、`TestPlayerFactory`，涉及的核心函数为`scoreFactory`、`MediaPlayerFactory::getPlayerType`。
* 在`getPlayerType`函数中，会遍历保存了全部`XXPlayerFactory`和它对应类型的`sFactoryMap`,并调用`XXPlayerFactory`的`scoreFactory`函数获得一个分数，分数最高者的类型将会作为`getPlayerType`的函数返回值。
* 媒体框架会根据`getPlayerType`函数的返回类型，创建出一个对应的播放器。
* `NuPlayerFactory`的`scoreFactory`函数的筛选过程实际上表明了`NuPlayer`所支持的媒体源类型。
* 整个播放器框架扩展性非常强，有利于Rom产生定制自己的播放器。
