# Native Handler

> AOSP Version: Oreo 8.0.0_r4

所谓的Native Handler，只是我自己臆想出来的名词（或许也有前人创造了，但我没发现也说不定），用来对Android媒体框架中消息通信部分进行描述。之所以这么命名，主要基于如下几点：
1. Android 媒体框架涉及到消息通信部分，主要由ALooper、AHandler、AMessage三个类组成，但并没有一个官方的名称，这对于写作来说，很不方便，总不能提到相关部分内容，就三个名词一起上吧。
2. 之所以是Native，是因为这个框架涉及到的类，都位于Native层，实际上ALooper、AHandler、AMessage三个都是C++代码编写，起名Native名副其实。
3. 至于Handler，借鉴Android 上层代码中的Handler机制名称，因为它们之间的逻辑，区别真的很小。

该系列文章，会分为三个部分：
* 介绍AHandler、ALooper源码。
* 介绍AMessage源码：AMessage值得一说的地方太多了，就和其它两个类分开了。
* 以Nuplayer为例，分析Native Handler在Android底层起到的作用。
## 简介

来张图说明一下handler-looper-message之间的关系。

![](/Users/heli/Desktop/NativeHandler.png)

## AHandler

> 源码路径参见文末最后一小节

AHandler，顾名思义，在这个框架中，是消息处理者的角色。

代码片段如下:

```c++
namespace android {
struct AMessage;
struct AHandler : public RefBase {
    AHandler()
        : mID(0),
          mVerboseStats(false),
          mMessageCounter(0) {
    }
    
    sp<ALooper> looper() const {
        return mLooper.promote();
    }
	//......
protected:
    // 纯虚函数，由每个继承了AHandler接口的类实现，是消息处理的关键
    virtual void onMessageReceived(const sp<AMessage> &msg) = 0;

private:
    friend struct AMessage;      // deliverMessage()
    friend struct ALooperRoster; // setID()
	// 每一个handler都有自己的唯一id，可以通过id()函数别获取
    ALooper::handler_id mID;
    // handler需要注册在looper中才会起作用，可以通过get方法获取，通过setID初始化
    wp<ALooper> mLooper; 
    
    inline void setID(ALooper::handler_id id, const wp<ALooper> &looper) {
        mID = id;
        mLooper = looper;
    }

    bool mVerboseStats;
    uint32_t mMessageCounter; // 当前handler处理消息的计数器
    KeyedVector<uint32_t, uint32_t> mMessages; // 和message相关的消息计数器
	// 当需要发送消息时，Looper线程会调用该函数，进而触发子类实现的onMessageReceived函数回调
    void deliverMessage(const sp<AMessage> &msg);

    DISALLOW_EVIL_CONSTRUCTORS(AHandler);
};
}  // namespace android
```

很简单，来看一下具体实现，也就是AHandler.cpp。

## deliverMessage

```c++
namespace android {

void AHandler::deliverMessage(const sp<AMessage> &msg) {
    onMessageReceived(msg); // 直接调用纯虚函数（子类实现）onMessageReceived
    mMessageCounter++; // 消息计数器+1
	  // 如果mVerboseStats为true，则会在mMessages中记录通过Handler处理的每一个Message的数量。
    // 这功能对阅读代码来说，基本没什么用。构造函数默认初始化列表直接把mVerboseStats设置成了false
    if (mVerboseStats) { 
        uint32_t what = msg->what();
        ssize_t idx = mMessages.indexOfKey(what);
        if (idx < 0) {
            mMessages.add(what, 1); // 添加新的消息
        } else {
            mMessages.editValueAt(idx)++; // 消息计数+1
        }
    }
}

}  // namespace android
```

可以看到，AHandler的实现非常简单，头文件中，除了一个虚函数外，就只有deliverMessage函数没有实现。所以，在cpp文件中，只需要实现deliverMessage函数即可。
> KeyedVector<key, value> 相关函数：
>
> * indexOfKey(key)：获取key对应的value值
> * add(key, value)：添加一个键值对
> * editValueAt(value)：返回value的地址
### 小结一下

简单总结一下deliverMessage函数：

1. 函数体第一行就直接调用了子类的onMessageReceived函数，处理消息去了。
2. 为mMessageCounter消息计数器加1。
3. mVerboseStats控制的*if*代码块，无关紧要，不说也罢。

小结一下AHandler：

* 拥有一个mID，区别其它AHandler。

* 持有一个ALooper的引用，//TODO 后面一定能够知道这个引用是用来做什么的。

* 维护了一个mMessageCounter计数器，记录ahandler已经处理过的Message数。

* deliverMessage函数直接调用子类的onMessageReceived函数，交给子类处理。

## ALooper

> 源码路径参见文末最后一小节

继续顾名思义，是一个循环器，如果是循环的话，多半有线程了。先看一下它的头文件：

```c++
namespace android {

struct AHandler;
struct AMessage;
struct AReplyToken;

struct ALooper : public RefBase {
    typedef int32_t event_id;
    typedef int32_t handler_id;

    ALooper();

	// 注册handler
    handler_id registerHandler(const sp<AHandler> &handler);
    void unregisterHandler(handler_id handlerID); // 根据handlerID注销handler
	// 启动looper的循环线程，开始工作
    status_t start(
            bool runOnCallingThread = false,
            bool canCallJava = false,
            int32_t priority = PRIORITY_DEFAULT
            );
	// 停止线程
    status_t stop();
	
    static int64_t GetNowUs();

protected:
    virtual ~ALooper(); // 需析构函数，子类可以在复写函数中释放资源

private:
    friend struct AMessage;       // post()消息本身，下一个小结会讲
 
    struct Event { // 将发送时间和Message封装在一个结构体重
        int64_t mWhenUs;
        sp<AMessage> mMessage;
    };
 
    Mutex mLock;
    Condition mQueueChangedCondition;
 	// Looper的名字，可以通过对应的set/get函数设置/获取该字段值
    AString mName; 
 
    List<Event> mEventQueue; // 事件列表，所有的消息都会放进来，应该就是所谓的消息队列// TODO
 
    struct LooperThread; // 循环线程
    sp<LooperThread> mThread;
    bool mRunningLocally;
 
    // use a separate lock for reply handling, as it is always on another thread
    // use a central lock, however, to avoid creating a mutex for each reply
    Mutex mRepliesLock;
    Condition mRepliesCondition;
 
    // START --- 函数只会被AMessage使用
 
    // 在给定的超时时间内从当前looper上发送一个消息
    void post(const sp<AMessage> &msg, int64_t delayUs);
    // 创建一个和当前looper一起使用的回复令牌
    sp<AReplyToken> createReplyToken();
    // 等待消息发送后的回复令牌。如果状态OK,回复信息会被存储于参数提供的变量中。否则，参数无变化
    status_t awaitResponse(const sp<AReplyToken> &replyToken, sp<AMessage> *response);
    // 发送回复令牌的响应。假如该响应发送成功，返回OK。否则，返回一个错误码
    status_t postReply(const sp<AReplyToken> &replyToken, const sp<AMessage> &msg);

    // END --- 函数只会被AMessage使用
	// 从消息队列中取消息，执行
    bool loop();

    DISALLOW_EVIL_CONSTRUCTORS(ALooper);
};

} // namespace android
```

简单了解了ALooper的定义，接下来看看一些函数的具体实现。

### post()

```c++
void ALooper::post(const sp<AMessage> &msg, int64_t delayUs) { 
    // 参数接收一个Message，和一个消息发送的时延
    int64_t whenUs; // 消息发送的真实时间：根据时延值，判断是延迟发送还是立即发送。
    if (delayUs > 0) {
        whenUs = GetNowUs() + delayUs;
    } else {
        whenUs = GetNowUs();
    }
	// 遍历消息队列，找到一个队列中Event的时延大于whenUs的位置
    List<Event>::iterator it = mEventQueue.begin();
    while (it != mEventQueue.end() && (*it).mWhenUs <= whenUs) {
        ++it;
    }
	// 根据入参和计算得到的时延，封装消息事件（Event）
    Event event;
    event.mWhenUs = whenUs;
    event.mMessage = msg;

    if (it == mEventQueue.begin()) {
        mQueueChangedCondition.signal();
    }
	// 将封装好的消息事件，插到消息队列中。
    mEventQueue.insert(it, event);
}
```

一句话总结post函数的功能：*post()函数将Message和时延值封装成Event后，插入到消息队列mEventQueue中*。

> 从函数中的消息队列的遍历算法来推断，消息队列是按照时延值的大小，从小到大排列的。

### AReplyToken::createReplyToken

```c++
// 只有AMessage::postAndAwaitResponse函数会调用
sp<AReplyToken> ALooper::createReplyToken() {
    return new AReplyToken(this); 
}
```

createReplyToken函数直接创建了 一个AReplyToken对象返回，来看看是怎么的定义：

> AReplyToken是定义在AMessage.h中的，具体代码路径请查看本文最后一小节

```c++
struct AReplyToken : public RefBase {
    explicit AReplyToken(const sp<ALooper> &looper)
        : mLooper(looper), // 将looper保存在mLooper成员中
          mReplied(false) { // 将mReplied初始化为false，刚刚创建回复令牌时状态当然是尚未回复的
    }

private:
    friend struct AMessage;
    friend struct ALooper;
    // 回复令牌中，保存了消息来源于那个looper，一个程序中looper可不止一个。它们彼此应该是靠mName区别TODO
    wp<ALooper> mLooper; 
    sp<AMessage> mReply; // 记录回复消息
    bool mReplied; // 本回复令牌的状态
	
    // 如果未设置回复，则返回false; 否则，它检索回复并返回true
    bool retrieveReply(sp<AMessage> *reply) {
        if (mReplied) {
            *reply = mReply;
            mReply.clear();
        }
        return mReplied;
    }
    // 设置此令牌的回复。 返回OK或错误
    status_t setReply(const sp<AMessage> &reply);
};
```

只有一个未实现的函数

```c++
status_t AReplyToken::setReply(const sp<AMessage> &reply) {
    if (mReplied) { // 已经设置过的回复不可再设置
        ALOGE("trying to post a duplicate reply");
        return -EBUSY;
    }
    CHECK(mReply == NULL);
    mReply = reply;
    mReplied = true;
    return OK;
}
```

setReply函数将回复的Message设置到回复令牌的mReply字段中，并将回复状态改为true。

#### 小结AReplyToken

1. AReplyToken：意味消息的回复令牌
2. AReplyToken中包含消息是否已经被处理过的字段mReplied，如果处理过，mReplied字段被置为true。
3. AReplyToken中包含了回复消息本身，体现在mReply字段。

### awaitResponse

```c++
// 只会被AMessage::postAndAwaitResponse函数调用
status_t ALooper::awaitResponse(const sp<AReplyToken> &replyToken, sp<AMessage> response) {
   // return status in case we want to handle an interrupted wait
   Mutex::Autolock autoLock(mRepliesLock);
   CHECK(replyToken != NULL);
   while (!replyToken->retrieveReply(response)) {
       {
           Mutex::Autolock autoLock(mLock);
           if (mThread == NULL) {
               return -ENOENT;
           }
       }
       mRepliesCondition.wait(mRepliesLock); // 等待mRepliesLock，相关知识，请查看Condition小结
   }
   return OK;
}
```

awaitResponse函数，通过无限循环+同步锁的方式，让当前线程检索出一个可回复的回复令牌后通过入参，将回复消息反馈给调用线程。

> 在ALooper中，mRepliesCondition.wait等待后，只有当ALooper::stop()和ALooper::postReply函数调用后，才会结束等待，继续下一次循环。

#### Condition

这是一个用于同步的对象，为Android中特有。它的函数有：

| 函数名                                      | 功能                                                |
| ------------------------------------------- | --------------------------------------------------- |
| wait(Mutex& mutex)                          | 当前线程等待唤醒                                    |
| waitRelative(Mutex& mutex, nsecs_t reltime) | 当前线程等待唤醒，如果等待时间超过reltime，退出等待 |
| signal()                                    | 触发唤醒通知，但只能唤醒一个等待的线程              |
| broadcast()                                 | 发送唤醒广播，唤醒所有等待线程                      |

###postReply

```c++
status_t ALooper::postReply(const sp<AReplyToken> &replyToken, const sp<AMessage> &reply) {
    Mutex::Autolock autoLock(mRepliesLock);
    status_t err = replyToken->setReply(reply); // 将回复消息设置到回复令牌中
    if (err == OK) {
        mRepliesCondition.broadcast(); // 通过Condition唤醒awaitResponse函数
    }
    return err;
}
```

postReply的主要作用，就是将回复令牌和回复消息绑定，并唤醒awaitResponse函数，处理回复消息。

### loop

```c++
bool ALooper::loop() {
    Event event;
    {
        Mutex::Autolock autoLock(mLock);
        if (mThread == NULL && !mRunningLocally) { 
            // 如果没有初始化线程，或者不在本地运行，返回false
            return false;
        }
        if (mEventQueue.empty()) { // 消息队列是空的，当前线程等待，直到被唤醒发回true
            mQueueChangedCondition.wait(mLock);
            return true;
        }
        int64_t whenUs = (*mEventQueue.begin()).mWhenUs; // 获取消息队列第一条消息的发送时间
        int64_t nowUs = GetNowUs();

        if (whenUs > nowUs) { 
            // 如果第一条消息还没有到发送时间，则等待whenUs - nowUs后唤醒线程返回true
            int64_t delayUs = whenUs - nowUs;
            mQueueChangedCondition.waitRelative(mLock, delayUs * 1000ll);
            return true;
        }
		// 如果发现消息的发送时间过期，做两件事情：1. 删掉该条消息。2. 发送该条消息
        event = *mEventQueue.begin();
        mEventQueue.erase(mEventQueue.begin()); // 1. 删掉该条消息。
    }
    event.mMessage->deliver(); // 2. 发送该条消息
    return true; // 返回true
}
```

loop函数，总共做了以下几件事情：

* 条件判断：判断是否初始化线程，并且线程是否在本地运行，如果否则返回false，使可能存在的循环停止。
* 消息队列判断：判断消息队列中是否有消息，没有的话，让线程进入等待，直到有消息入队后被唤醒。
* 消息发送判断：判断队列中，第一条小时发送时间是否满足，满足则发送消息，并将消息移出队列。否则让线程等待，一定时间（当前时间和发送时间的时间差）后，自动唤醒线程。

可以看到，loop函数会根据实际情况，判断是否让线程等待。防止函数不断执行的无意义死循环，造成CPU资源的浪费。

## AMessage

AMessage可以算的上市整个消息系统中的核心接口了。自然，它的接口也比其它两个结构体复杂得多。

```c++
namespace android {

struct ABuffer;
struct AHandler;
struct AString;
class Parcel;

struct AMessage : public RefBase {
    AMessage();
    AMessage(uint32_t what, const sp<const AHandler> &handler);

    /**
     * 根据parcel构建一个AMessage
     *
     * @param parcel 数据包
     * @param maxNestingLevel 最大嵌套层级
     * @return AMessage是可以嵌套存在的，但如果嵌套层级大于maxNestingLevel，将产生异常，并返回NULL
     * 如果消息类型无法被函数识别也会产生异常，并返回NULL。支持的消息类型有：
     * int32_t Int32 int64_t Int64 size_t Size float Float double Double
     * AString String AMessage Message
     */
    static sp<AMessage> FromParcel(const Parcel &parcel,
                                   size_t maxNestingLevel = 255);

    /** 将当前AMessage写入数据包parcel。AMessage中所有的items类型必须能被识别（见FromParcel部 
     *  分），否则会产生异常。
     */
    void writeToParcel(Parcel *parcel) const;
    void clear();
	/**
	 * 设置Int32类型的item,类似的函数还有setInt64、setSize、setFloat、setDouble、setPointer、
	 * setString、setRect、setBuffer、setObject等。并通过findXXX获取每个item的值。
	 * 
	 * @param name 每个item都会有一个独立的key
	 * @param value 每个item的值
	 */
    void setInt32(const char *name, int32_t value);
    bool contains(const char *name) const; // 判断是否包含名为name的item
    status_t post(int64_t delayUs = 0); // 发送消息
    // 将消息post到目标AHandler,并等待回复或者异常。
    status_t postAndAwaitResponse(sp<AMessage> *response);

    // If this returns true, the sender of this message is synchronously
    // awaiting a response and the reply token is consumed from the message
    // and stored into replyID. The reply token must be used to send the response
    // using "postReply" below.
    bool senderAwaitsResponse(sp<AReplyToken> *replyID);

    // 将message作为回复令牌发送，一个回复令牌只能使用一次，如何可以被发送，返回true，否则返回false
    status_t postReply(const sp<AReplyToken> &replyID);

    // 执行当前对象的深拷贝。警告：RefBase类型item值，不会被拷贝，只会让引用计数加+1。
    sp<AMessage> dup() const;

    /**
     * 对当前对象进行深/浅拷贝，并返回一个包含差异的AMessage。
     * 警告：RefBase类型item值，不会被拷贝，只会让引用计数加+1。
     *
     * @param other 用于和当前对象比较的AMessage对象。
     * @param deep 是否进行深比较。
     * @return AMessage 返回一个差异的AMessage对象。
     */
    sp<AMessage> changesFrom(const sp<const AMessage> &other, bool deep = false) const;

    AString debugString(int32_t indent = 0) const;// TODO 这是干啥的？
    enum Type { // 消息中，item的类型
        kTypeInt32,
        kTypeInt64,
        kTypeSize,
        kTypeFloat,
        kTypeDouble,
        kTypePointer,
        kTypeString,
        kTypeObject,
        kTypeMessage,
        kTypeRect,
        kTypeBuffer,
    };

    size_t countEntries() const;
    const char *getEntryNameAt(size_t index, Type *type) const;
protected:
    virtual ~AMessage(); // 虚析构函数

private:
    friend struct ALooper; // deliver()
    // 通过setWhat函数设置该值，通过what函数获取该值。该值用于区分不同的消息，可以认为是一种消息类型
    uint32_t mWhat; 

    // used only for debugging
    ALooper::handler_id mTarget;
    // 消息的处理器：指定该消息最终由那个AHandler处理。通过setTarget函数初始化。
    wp<AHandler> mHandler;
    wp<ALooper> mLooper;
    struct Rect { // 矩形结构体, 用于保存视频帧的显示尺寸的
        int32_t mLeft, mTop, mRight, mBottom;
    };

    struct Item { // item结构体
        union { // item 的值
            int32_t int32Value;
            int64_t int64Value;
            size_t sizeValue;
            float floatValue;
            double doubleValue;
            void *ptrValue;
            RefBase *refValue;
            AString *stringValue;
            Rect rectValue;
        } u;
        const char *mName; // item的key
        size_t      mNameLength; // item key的长度
        Type mType; // item的消息类型
        void setName(const char *name, size_t len);
    };

    enum {
        kMaxNumItems = 64 // 最大item数
    };
    Item mItems[kMaxNumItems]; // item 数组
    size_t mNumItems; // 实际item数

    Item *allocateItem(const char *name);
    void freeItemValue(Item *item);
    const Item *findItem(const char *name, Type type) const;

    void setObjectInternal(
            const char *name, const sp<RefBase> &obj, Type type);

    size_t findItemIndex(const char *name, size_t len) const;

    void deliver();

    DISALLOW_EVIL_CONSTRUCTORS(AMessage);
};

}  // namespace android
```

AMessage的函数接口相对较多，这里就挑一个我认为重要的展开一下：

###  构造函数

```c++
AMessage::AMessage(void)
    : mWhat(0),
      mTarget(0),
      mNumItems(0) {
}

AMessage::AMessage(uint32_t what, const sp<const AHandler> &handler)
    : mWhat(what),
      mNumItems(0) {
    setTarget(handler);
}
```

1. 无参构造没什么好说的，将mWhat、mTarget、mNumItems设置为零。
2. 有参构造：将mNumItems设置为零，参数中的handler设置给mHandler。

这里出现了setTarget函数，索性先看看这个函数呗。

###  setTarget

```c++
void AMessage::setTarget(const sp<const AHandler> &handler) {
    if (handler == NULL) { // 如果参数为NULL，AMessage对象回到初始化的状态
        mTarget = 0;
        mHandler.clear();
        mLooper.clear();
    } else { // 将handler中的相关应用赋值给AMessage对象
        mTarget = handler->id(); 
        mHandler = handler->getHandler();
        mLooper = handler->getLooper();
    }
}
```

为什么取名叫做`setTarget`呢？ 因为AMessage对象，最终需要AHandler对象处理，这两个原本孤立的对象，通过各自内部对彼此的引用持有，达到这样的目的。所以，该函数可以理解为，为该条消息（AMessage）设定目标处理对象。

### setXXX/findXXX

在看基本数据类型的setXXX/findXXX函数之前，先介绍一下几个比较重要的函数，allocateItem、findItem、findItemIndex。

####  findItemIndex

findItemIndex函数中，有大量的DUMP_STATS的宏定义，对本文来说没什么意义。去掉之后的代码就像这个样子：

```c++
inline size_t AMessage::findItemIndex(const char *name, size_t len) const {
    size_t i = 0;
    for (; i < mNumItems; i++) {
        if (len != mItems[i].mNameLength) { // 如果入参len和itemName长度不一致，继续下一次判断
            continue;
        }
        if (!memcmp(mItems[i].mName, name, len)) { // 长度一致，且名字相同，找到了目标item，跳出循环
            break;
        }
    }
    return i;
}
```

在AMessage.h的代码分析部分，已经说明了：

* mNumItems：是当前AMessage中的Item计数。
* mItems：是当前AMessage的Item数组。

这个函数的功能从名字上就很直观：findItemIndex可以直接看出是“找到指定item的数组index”的意思，这种自注释的命名，值得我们学习。

另外，方法实现上也很简单：

1. 编译mItems数组。
2. 通过比较入参len的值，和每个item名称长度作对比，如果相等进入下一个判断逻辑，如果不相等进入下一步循环。
3. 如果名称长度相等，且名称通过memcmp判断相同，则找到和入参name匹配的item，返回找到的item在数组中的index。如果没有找到，则返回数组mItems的长度。

#### findItem

```c++
const AMessage::Item *AMessage::findItem(
        const char *name, Type type) const {
    size_t i = findItemIndex(name, strlen(name));
    if (i < mNumItems) {
        const Item *item = &mItems[i];
        return item->mType == type ? item : NULL;
    }
    return NULL;
}
```

findItem 函数主要功能：

1. 通过findItemIndex函数，找到对应入参name在mItems数组中的索引。
2. 通过上一步的索引，从mItems数组中获取Item指针，并返回。
3. 没找到，返回NULL。

#### allocateItem

我们都知道AMessage中有很多的Item，Amessage为创建这些Item，专门编写了一个函数：allocateItem

```c++
AMessage::Item *AMessage::allocateItem(const char *name) {
    size_t len = strlen(name);
    size_t i = findItemIndex(name, len); // 查看需要新建的item是否已近存在
    Item *item;
    if (i < mNumItems) { // 如果需要新建的item已存在，将已存在的item清空
        item = &mItems[i];
        freeItemValue(item);
    } else { // 需要新建的item不存在
        CHECK(mNumItems < kMaxNumItems);
        i = mNumItems++; // item计数加1
        item = &mItems[i]; // 将mItems中第i个位置留给新的item
        item->setName(name, len); // 为item设置名称
    }
    return item;
}
```

allocateItem函数，并不会真的去alloc内存，因为mItems数组中已经将内存分配好了，只需要将对应数组中index的位置指针赋值给一个临时变量，然后通过临时指针变量来初始化数组中item的值就可以了。这也就是allocateItem函数的逻辑。

#### setXXX/findXXX

```c++
#define BASIC_TYPE(NAME,FIELDNAME,TYPENAME)                             
void AMessage::set##NAME(const char *name, TYPENAME value) {            
    Item *item = allocateItem(name);                                    
                                                                        
    item->mType = kType##NAME;                                          
    item->u.FIELDNAME = value;                                          
}                                                                       
                                                                        
/* NOLINT added to avoid incorrect warning/fix from clang.tidy */       
bool AMessage::find##NAME(const char *name, TYPENAME *value) const {  /* NOLINT */ 
    const Item *item = findItem(name, kType##NAME);                     
    if (item) {                                                         
        *value = item->u.FIELDNAME;                                     
        return true;                                                    
    }                                                                   
    return false;                                                       
}

BASIC_TYPE(Int32,int32Value,int32_t)
BASIC_TYPE(Int64,int64Value,int64_t)
BASIC_TYPE(Size,sizeValue,size_t)
BASIC_TYPE(Float,floatValue,float)
BASIC_TYPE(Double,doubleValue,double)
BASIC_TYPE(Pointer,ptrValue,void *)

#undef BASIC_TYPE
```

AMessage中，基本数据类型的set/find方法定义就在上边了。通过宏定义，设计一个同样算法的函数，表达N个函数的做法，真的是绝了，如果java里也可以这么做的话，要少多少set/get函数，少多少重复代码啊。我代表自己，强烈建议java9引入这种机制。

在这里，通过宏替换，总共可以展开12个不同函数。篇幅起见，就只展开Int32类型的分析分析。

> set##NAME中的“##”符号的作用是，将相邻的两个宏连接起来。于是就有了setInt32、setInt64等函数。

#### setInt32

```c++
void AMessage::setInt32(const char *name, int32_t value) {            
    Item *item = allocateItem(name); // 前面已经对这个函数解释过了，忘记的小朋友可以回头去看看
    item->mType = kTypeInt32;                                          
    item->u.int32Value = value;                                          
}                                                 
```

set系列函数都比较简单：

1. 通过allocateItem函数，获取mItems数组中，对应name的空item的指针。
2. 通过上一步获取的指针，修改mItems数组对应item的mType值。
3. 通过第一步获取的指针，修改mItems数组对应item的值（该值的内存使用联合体表示）。

简单直白又透彻。完美的代码

#### findInt32

```c++
/* NOLINT added to avoid incorrect warning/fix from clang.tidy */       
bool AMessage::findInt32(const char *name, int32_t *value) const {  /* NOLINT */ 
    const Item *item = findItem(name, kTypeInt32); // 前面已经对这个函数解释过了，忘记的小朋友可以回头去看看                    
    if (item) {                                                         
        *value = item->u.int32Value;                                     
        return true;                                                    
    }                                                                   
    return false;                                                       
}
```

find系列函数，同样直白：

1. 通过findItem找到name相同且type相同的item，返回它的指针。没找到的话，会返回一个NULL。
2. 如果第一步找到了对应指针，则通过指针，将对应item的值赋值给`*value`，并返回true。表示找到了对应item，函数调用线程可以根据`*value` 指向的内存，获取对应item的值。
3. 如果第一步没有找到，返回false。

### 其它常见的set/find函数

#### setMessage

```c++
void AMessage::setMessage(const char *name, const sp<AMessage> &obj) {
    Item *item = allocateItem(name); // 前面已经对这个函数解释过了，忘记的小朋友可以回头去看看
    item->mType = kTypeMessage;

    if (obj != NULL) { obj->incStrong(this); }
    item->u.refValue = obj.get();
}
```

如果你记性足够好的话，应该记得AMessage的源码介绍的时候，FromParcel函数的注释部分提示：AMessage是可以**嵌套**存在的，但如果嵌套层级大于maxNestingLevel，将产生异常。

这个函数，就可以实现AMessage嵌套，只需将item类型指定为`kTypeMessage`即可，剩下的操作和其它set函数也没啥区别了。

我发现，set/find系列方法，都不是很复杂：

1. 通过allocateItem找到合适的item内存指针。
2. 通过指针，设置对应item的mType类型，在这个函数中，类型自然是`kTypeMessage`。
3. 将被嵌套的AMessage对象引用计数加1,后，将AMessage对象的值放到当前item中去。

#### findMessage

```c++
bool AMessage::findMessage(const char *name, sp<AMessage> *obj) const {
    const Item *item = findItem(name, kTypeMessage);
    if (item) {
        *obj = static_cast<AMessage *>(item->u.refValue);
        return true;
    }
    return false;
}
```

和基础类型的find函数类似：

1. 通过findItem找到对应的item，如果没找到`*item`为NULL。
2. 如果找到了对应item，通过`*obj`将找到的AMessage提供给调用线程使用，并返回true。
3. 没找到，返回false。

其它的setBuffer、setObject以及与之对应的find函数，都和set/findMessage相关函数类似，就不多说了。

#### setRect

不得不说一下Rect 相关的函数（Rect结构体，前面代码中已介绍）。Rect结构体是用来保存视频帧的显示尺寸的。所以，就Android多媒体框架来说，非常重要。

```c++
void AMessage::setRect(
        const char *name,
        int32_t left, int32_t top, int32_t right, int32_t bottom) {
    Item *item = allocateItem(name); // 拿到一个空的item
    item->mType = kTypeRect; // 设置item类型
	// 左上右下的边界值
    item->u.rectValue.mLeft = left;
    item->u.rectValue.mTop = top;
    item->u.rectValue.mRight = right;
    item->u.rectValue.mBottom = bottom;
}
```

#### findRect

再看一下find函数：

```c++
bool AMessage::findRect(
        const char *name,
        int32_t *left, int32_t *top, int32_t *right, int32_t *bottom) const {
    const Item *item = findItem(name, kTypeRect); // 找到item
    if (item == NULL) {
        return false;
    }
	// 将上下左右值，通过指针，放到不同的内存地址去。
    *left = item->u.rectValue.mLeft; 
    *top = item->u.rectValue.mTop;
    *right = item->u.rectValue.mRight;
    *bottom = item->u.rectValue.mBottom;
    return true;
}
```

和其他find函数也没啥区别，一定要有的话，大概就是：findRect的参数比较多 .^~^. 。

### 消息交互相关函数

#### deliver

```c++
void AMessage::deliver() {
    sp<AHandler> handler = mHandler.promote(); // 获取当前AMessage绑定的Handler对象
    if (handler == NULL) {
        ALOGW("failed to deliver message as target handler %d is gone.", mTarget);
        return;
    }

    handler->deliverMessage(this); // 将自己作为参数，调用目标handler的deliverMessage
}
```

deliver 也不复杂，咦，为啥说也？

哎，简单的说一下需要注意的地方：

* 关于mHandler：mHandler对象是AMessage中的私有字段，该字段唯一初始化的地方在前面讲过的AMessage::setTarget函数中。虽然，到现在，还没有分析整个消息机制工作流程，但我们可以大胆的猜想一下：AMessage实际只是消息的载体，消息只有发送出去了，被处理了才有意义。但是在AMessage的deliver（发送）函数中，却用到了一个mHandler对象。那么，可以考虑，mHandler对象，在消息创建之初便已经被初始化。换句话说，setTarget函数，在AMessage创建之初就会调用。这点，我希望在下一篇文章得到验证。TODO
* 关于handler->deliverMessage：看到这里，该函数应该已经熟悉了，AHandler::deliverMessage函数什么都没做，直接将入参原封不动的通过onMessageReceived纯虚函数，交给子类处理了。子类怎么处理，关我屁事！！！！ 好吧，不急，后面分析具体事例的时候肯定会稍微提一下。

#### post

```c++
status_t AMessage::post(int64_t delayUs) {
    sp<ALooper> looper = mLooper.promote(); // 获取当前AMessage绑定的mLooper对象
    if (looper == NULL) {
        ALOGW("failed to post message as target looper for handler %d is gone.", mTarget);
        return -ENOENT;
    }

    looper->post(this, delayUs); // 将自己和发送时间传递给looper处理。
    return OK;
}
```

哇哇哇~，这函数简直就是`AMessage::deliver`函数的翻版，已经在deliver函数中说过的就不再说了。说一下不同的地方：

* looper： mLooper赋值的地方，也在`AMessage::setTarget`函数中，和mHandler赋值的时机是一样的，在AMessage创建时。
* `AMessage::post`直接将自己和消息的发送时间，通过AMessage创建时赋值的`ALooper::post`函数，交给looper去处理，处理结果就是将`AMessage`对象自己，和发送时间(`delayUs`)包装一下，放到一个消息队列中区。不知道`ALooper::post`函数的帅哥(应该没有仙女会看这种文章吧！)可以去ALooper一小节回顾一下。

#### postAndAwaitResponse

```c++
status_t AMessage::postAndAwaitResponse(sp<AMessage> *response) {
    sp<ALooper> looper = mLooper.promote(); // 获取当前AMessage绑定的mLooper对象
    if (looper == NULL) {
        ALOGW("failed to post message as target looper for handler %d is gone.", mTarget);
        return -ENOENT;
    }

    sp<AReplyToken> token = looper->createReplyToken();
    if (token == NULL) {
        ALOGE("failed to create reply token");
        return -ENOMEM;
    }
    setObject("replyID", token);

    looper->post(this, 0 /* delayUs */);
    return looper->awaitResponse(token, response);
}
```

从函数名可以看出，该函数除了和`AMessage::post`函数一样，会将消息放到`ALooper`中的消息队列中外，还会等待消息的返回。而在整个Native Handler 消息机制中，消息的返回都是通过回复令牌体现。

这里重点看一下回复令牌和等待返回的代码。

1、回复令牌的创建

```c++
sp<AReplyToken> token = looper->createReplyToken();
    if (token == NULL) {
        ALOGE("failed to create reply token");
        return -ENOMEM;
}
setObject("replyID", token);
```

这段代码，直接通过mLooper获取一个回复令牌`AReplyToken`。并将回复令牌的指针当作自己的一个item存起来，key是“replyID”，值是刚刚获取的回复令牌。

来来来，我们这里刨一个坑先。再来联想一下，在一个需要等待回复的函数调用中，创建了一个回复令牌，并将该令牌作为消息的一部分（一个item）存储起来，但在不需要回复的另一个函数中，却没有这种动作。是不是可以大胆的判断：TODO

* 一个消息，如果有名为"replyID"的item，那么它就已经在准备返回了。
* 如果没有“replyID”的item，那么它应该是处在创建或者刚加入到消息队列中，而没有被线程处理。

大胆假设，我会在后面小心求证的。至于假设的意义？ 哎~ 想太多了， 消磨廉价的光阴罢了。我的发际线~

2、等待回复

```c++
looper->post(this, 0 /* delayUs */);
return looper->awaitResponse(token, response);
```

post函数就不多说了。简单聊一下awaitResponse：

两个参数：

* token：回复令牌实际上是记录当前消息是否已经回复等消息的。因为前面记录到当前消息item中的是token的指针，所以，这里传参的意义是通过指针，间接修改当前消息的token。
* response：外界传入的AMessage指针，用于回传消息处理的结果。

另外：因为ALooper::awaitResponse函数中存在while循环和Condition锁，所以该函数是个会阻塞的函数。直到有了回复消息，才会解除锁定状态。

#### postReply

```c++
status_t AMessage::postReply(const sp<AReplyToken> &replyToken) {
    if (replyToken == NULL) {
        ALOGW("failed to post reply to a NULL token");
        return -ENOENT;
    }
    sp<ALooper> looper = replyToken->getLooper();
    if (looper == NULL) {
        ALOGW("failed to post reply as target looper is gone.");
        return -ENOENT;
    }
    return looper->postReply(replyToken, this);
}
```

该函数的调用时机，暂时还不知道，通过“postReply”函数名称可以猜测，该函数是在消息接收端处理好消息和对应的回复令牌后，调用该函数，执行消息回复的逻辑。这里又挖了个坑，看后面能不能圆回来。

根据上面的假设：回复令牌已经处理好，准备回复消息，那么代码块前面的各种非空校验显然是能够通过的。

最终调用了`ALooper::postReply`函数，这里有两个参数：

* replyToken：即已经设置了回复的回复令牌本身
* this：AMessage消息对象自身。

好累啊，不过还是先来回顾一下，前面说过的`ALooper::postReply`函数吧：

> postReply的主要作用，就是将回复令牌和回复消息绑定，并唤醒awaitResponse函数，处理回复消息。



小结一下：`AMessage::postReply`函数也只是将传入的回复令牌和自己，交给`ALooper::postReply`函数处理，处理的结果是：将令牌与自身绑定后，通过广播，将所有等待mRepliesCondition锁的线程唤醒，告诉它们消息处理完了，起来干活。

#### senderAwaitsResponse

```c++
bool AMessage::senderAwaitsResponse(sp<AReplyToken> *replyToken) {
    sp<RefBase> tmp;
    bool found = findObject("replyID", &tmp);

    if (!found) {
        return false;
    }

    *replyToken = static_cast<AReplyToken *>(tmp.get());
    tmp.clear();
    setObject("replyID", tmp);
    // TODO: delete Object instead of setting it to NULL

    return *replyToken != NULL;
}
```

根据内心的指引，让我们畅想一下...... ，咳咳，不对。

应该是根据函数名，让我们继续猜一猜函数功能。sender Awaits Response = 发件人等待回应 。 什么鬼，我姑且理解为，这个函数是在消息回复的时候调用的吧，实在看不出其它含义了。还是看代码靠谱点。

* findObject： 找到当前AMessage对象的回复令牌，如果没有就返回false。
*  `*replyToken = static_cast<AReplyToken *>(tmp.get());`：如果有就将指针赋值给入参，供调用者差遣，最后将当前AMessage的回复令牌指针清掉，返回true。

小结一下：根据代码的意思，很简单，就是将当前AMessage的回复令牌拿给别人用，自己不要了。clear之后，又是一具白花花的身体。

## 源码相关路径

Android底层代码，一般*.h*文件和*.cpp*文件都存放在不同路径下。

### 头文件

/frameworks/av/include/media/stagefright/foundation/

AMessage.h

AHandler.h

ALooper.h

### .cpp文件

/frameworks/av/media/libstagefright/foundation/

AMessage.cpp

AHandler.cpp

ALooper.cpp