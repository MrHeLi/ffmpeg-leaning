# Android Native报错定位

今天调试Android stagefright模块，修改MediaCodec.cpp文件时，一不小心在代码里写了个空指针进去。

于是得到了下面这个报错日志：

```shell
--------- beginning of crash
libc    : Fatal signal 11 (SIGSEGV), code 1, fault addr 0x0 in tid 5104 (MediaCodec_loop)
DEBUG   : *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***
DEBUG   : Build fingerprint: '*******************************************'
DEBUG   : Revision: '0'
DEBUG   : ABI: 'arm'
DEBUG   : pid: 5050, tid: 5104, name: MediaCodec_loop  >>> com.google.android.exoplayer2.demo <<<
DEBUG   : signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 0x0
DEBUG   : Cause: null pointer dereference
DEBUG   :     r0 ********  r1 ********  r2 ********  r3 ********
DEBUG   :     r4 ********  r5 ********  r6 ********  r7 ********
DEBUG   :     r8 ********  r9 ********  sl ********  fp ********
DEBUG   : 
DEBUG   : backtrace:
DEBUG   :     #00 pc 00018e90  /system/lib/libc.so (__memcpy_base+185)
DEBUG   :     #01 pc 000446e9  /system/lib/libc.so (__sfvwrite+310)
DEBUG   :     #02 pc 00044943  /system/lib/libc.so (fwrite+122)
DEBUG   :     #03 pc 000dc169  /system/lib/libstagefright.so (_ZN7android10MediaCodec21onReleaseOutputBufferERKNS_2spINS_8AMessageEEE+1432)
DEBUG   :     #04 pc 000d7a59  /system/lib/libstagefright.so (_ZN7android10MediaCodec17onMessageReceivedERKNS_2spINS_8AMessageEEE+156)
```

想着平时分析日志时，经常会遇到这样的打印，因为看不懂，通常都是直接绕过。索性今天悠闲，清扫一下知识上的战争迷雾吧。

## 源码部分：

```c++
status_t MediaCodec::onReleaseOutputBuffer(const sp<AMessage> &msg) {
	// 这是我添加的代码 也是报空指针的代码，代码在MediaCodec.cpp文件中的3077行
	size_t count = fwrite(heliBuf, sizeof(heliBuf) , 1, fp);
	// *****************
}
```

## 日志分析

简单看看logcat，能够提取到`Cause: null pointer dereference`。

我们知道，是一个空指针异常导致了crash。

那么这个空指针是在哪一行代码调用的呢？

可以看到`backtrace:`下面有crash时的调用栈信息。格式为：

> \#编号 pc 异常地址 共享库路径 （函数名）

 例1.0：

> \#00 pc 00018e90  /system/lib/libc.so (__memcpy_base+185)

所以，示例1.0，可以翻译为：在共享库 */system/lib/libc.so*的*__memcpy_base*函数中报了一个空指针。

logcat中打印的调用顺序，都是从下到上看的。例如，这里是先调用#04这一行，再调用#03... ，最后调用#00。所以，严格的说，#00这一行，是本次报错的代码。

但通常，我们编码都会直接或间接的使用系统现有库，而这些库久经沙场，基本不会有什么问题，如果\#00编号的调用指向的是一个系统库，不要犹豫，一定是你非法调用了它。

### 线索一

回到backtrace，\#00、\#01、\#02行的共享库都是libc.so，这就是一个系统库。在\#02行的函数名中，能够得知是调用fwrite时出现了异常。

### 线索二

\#03 pc 000dc169  /system/lib/libstagefright.so (_ZN7android10MediaCodec21onReleaseOutputBufferERKNS_2spINS_8AMessageEEE+1432)

从这里，终于看到了我修改的代码所属模块:libstagefright.so

> MediaCodec.cpp 代码目录就位于：/frameworks/av/media/libstagefright目录下

这里能给我们提供的信息有：

1. 异常出现在libstagefright.so库中。

2. 异常出现在_ZN7android10MediaCodec21onReleaseOutputBufferERKNS_2spINS_8AMessageEEE函数中。

从第二点那串乱码中，大致已经可以看到发生异常的函数名称了。但为什么是乱码！！！

google了一把，原来编译器在编译时，对函数名做了一定优化。

既然是编译器的事情，那一定是有一定规则，想想这规则应该也不会简单。有没有简单的方法，能把这堆乱码还原？

当然有，国外有大兄弟做了个网站专门“翻译”这堆乱码：<https://demangler.com/>

*DEMANGLE* 了一下，得到了这串乱码的实际函数名：`android::MediaCodec::onReleaseOutputBuffer(android::sp<android::AMessage> const&)`

![](https://github.com/MrHeLi/ffmpeg-leaning/blob/master/image/demangler.PNG)

直白的表明了，这是MediaCodec.cpp中的onReleaseOutputBuffer函数。xia! xia!

这个时候，我选择去茶水间，冲一杯...好吧，我没有咖啡。

让我们来理一理：

* 异常出现在libstagefright.so库中

* 异常是一个空指针。
* 发生在`fwrite`调用中。
* fwrite又被onReleaseOutputBuffer调用。

离真相已经很近了。我去代码里看了一下onReleaseOutputBuffer函数，居然只有我插入的代码里调用了fwrite。

为了让文档继续下去，我们来假装onReleaseOutputBuffer函数中，有无数的fwrite调用、或者报错的是一个第三方库，我们并不知道源码。怎么知道报错的代码，具体在文件的哪一行呢？

## addr2line

从名字上就可以看出，这玩意儿是将一个地址转换成行号的工具。

### 位置

如果你有在用Android Studio，那么恭喜你，可以很方便在自带NDK目录下找到它。

比如我的是这样：

> \Android\Sdk\ndk-bundle\toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\bin\arm-linux-androideabi-addr2line.exe

没有装Android Studio的同学，可以自行到官网下载NKD，解压后就可以看到了，路径类似。

> <https://developer.android.com/ndk/downloads?hl=zh-cn>

### 使用

```shell
arm-linux-androideabi-addr2line [option(s)] [addr(s)]
```

简单来说就是arm-linux-androideabi-addr2line + 可选项  + 异常地址

先来看看可选项都有哪些：

| [option(s)] | 介绍                                                         |
| ----------- | ------------------------------------------------------------ |
| @<file>     | 从文件中读取options（暂时不研究）                            |
| -a          | 在结果中显示地址addr（ps：没啥用，因为这个地址本来就是我们输入命令中的） |
| -b          | 设置二进制文件的格式（暂不研究）                             |
| -e          | 设置输入文件（常用：选项后面需要跟报错的共享库，用于addr2line程序分析） |
| -i          | unwind inline function（暂不研究）                           |
| -j          | Read section-relative offsets instead of addresses（暂不研究） |
| -p          | 让输出更易读（亲测，没啥区别，可能其它选项已经很易懂了）     |
| -s          | 在输出中，剥离文件夹名称（这个还不错，前面有一长串目录确实恶心） |
| -f          | 显示函数名称（也比较有用，可以直观的看出是在哪个函数中报的错） |
| -C(大写的)  | 将输出的函数名demangle（类似于demangler.com这个网站的功能）  |
| -h          | 输出帮助                                                     |
| -v          | 输出版本信息                                                 |

那么多选项，在这里能用上（也是常用）的，也就是 -e、 -s、-C、-f了。

### 实战addr2line出现“??:?”

铺垫了那么多，是时候试验一下了：

```shell
C:\SuperLi>arm-linux-androideabi-addr2line -e \system\lib\libstagefright.so -s -f -C 000dc169
_ZN7android10MediaCodec21onReleaseOutputBufferERKNS_2spINS_8AMessageEEE
??:?
```

eum~ 出现：_ZN7android10MediaCodec17onMessageReceivedERKNS_2spINS_8AMessageEEE，加个-C选项再试试：

```shell
C:\SuperLi>arm-linux-androideabi-addr2line -e \system\lib\libstagefright.so -s -f -C 000dc169
android::MediaCodec::onReleaseOutputBuffer(android::sp<android::AMessage> const&)
??:?
```

**??:?** 虾米，看到这个问号，我不由想起黑人问号的表情包。

只能google了：

> 在使用 addr2line 过程中经常会遇到 “??:?” 或 “??:0” 这种情况，原因就是一般 C/C++ SDK 都会进行添加 map 混淆，在编译配置选项中不生成符号表 symbolic 信息，不过 AndroidStudio 会默认为 so 文件添加符号表。

也就是说，我使用了一个不带符号表的库。

google说了，如果是aosp编译的话，在out/target/product/[productname]/symbols/system/lib/****.so下面会自动生成带了符号表的共享库。找了找，真的有。于是我改了一下addr2line输入文件的位置：

```shell
C:\SuperLi>arm-linux-androideabi-addr2line -e \symbols\system\lib\libstagefright.so -s -f -C 000dc169
fwrite(void const*, unsigned int pass_object_size0, unsigned int, __sFILE*)
stdio.h:375
```

Bingo！！！成功的去掉了讨厌的黑人问号（??:?）。但是为什么函数名称不一样了？（哪位大神知道原因，求解惑啊）索性把和libstagefright.so相关的addr都加进来看看吧：

```shell
C:\SuperLi>arm-linux-androideabi-addr2line -e \symbols\system\lib\libstagefright.so -s -f -C 000dc169
fwrite(void const*, unsigned int pass_object_size0, unsigned int, __sFILE*)
stdio.h:375
android::MediaCodec::onMessageReceived(android::sp<android::AMessage> const&)
MediaCodec.cpp:2508
```

结果中可以看出，报错的地方在MediaCodec.cpp文件的2508行。

推了推鼻梁上不存在的眼镜，嘴角扯出一抹弧度：小样，我找到你了。

然鹅，在MediaCodec.cpp文件中ctrl + g 了一把。我擦，2508行代码居然只是这个：

```c++
 我是2508行：   status_t err = onReleaseOutputBuffer(msg);
```

而我们真实报错的地方是在3077行。

因为fwrite在onReleaseOutputBuffer函数中只有一处调用，我们能通过阅读onReleaseOutputBuffer函数的代码，将异常锁定在3077行，但这只是妥协的方法。如果onReleaseOutputBuffer函数中有无数的fwrite怎么搞？暂时能想到的只有加log一步步调试了。

好吧，目前也只能做到这一步了。

### 小结

1、通过实验addrline + symbolic版本的so，只能将异常定位到库中的函数级别，而不能具体到行号

2、要精准定位，还需要结合上下文才能确定。

那么，所有的库都需要这么麻烦吗？

准备使用Android studio来测试一下。

## android studio

打开Android studio，通过File > New > New Project > Native C++ ... 创建一个Test 工程。

在自动生成的native-lib.cpp 文件的Java_com_dali_myapplication_MainActivity_stringFromJNI函数中，故意加入了一个`a = b / c`的除零异常。

native-lib.cpp 代码如下：

```c++
#include <jni.h>
#include <string>
using namespace std;

extern "C" JNIEXPORT jstring JNICALL
Java_com_dali_myapplication_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    string hello = "Hello from C++";
    int b = 3, c = 0;
    int a = b / c;
    return env->NewStringUTF(hello.c_str());
}
```

Activity代码如下：

```java
public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("native-lib");
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        TextView tv = findViewById(R.id.sample_text);
        tv.setText(stringFromJNI());
    }
    public native String stringFromJNI();
}
```

运行后，立马报错，关键日志如下：

```shell
 DEBUG   : signal 8 (SIGFPE), code -6 (SI_TKILL), fault addr -------- 
 DEBUG   :     r0 00000000  r1 000016aa  r2 00000008  r3 8efbcbe4 
 DEBUG   :     r4 8efbcbe4  r5 00000001  r6 00000000  r7 0000010c
 DEBUG   :     r8 0000004e  r9 8dc10000  sl 9eadb790  fp 8dc10000
 DEBUG   :     ip 9eadb768  sp 9eadb6c8  lr 79a1ee9b  pc 8ef4c964  cpsr 00000010 
 DEBUG   :                                       
 DEBUG   : backtrace:
 DEBUG   :     #00 pc 0004a964  /system/lib/libc.so (tgkill+12) 
 DEBUG   :     #01 pc 0001ce97  /data/app/com.dali.myapplication-H8qoqUb6cjv0hUDfGYRtRQ==/lib/arm/libnative-lib.so
 DEBUG   :     #02 pc 000064e1  /data/app/com.dali.myapplication-H8qoqUb6cjv0hUDfGYRtRQ==/lib/arm/libnative-lib.so (Java_com_dali_myapplication_MainActivity_stringFromJNI+108)                  
```

使用addr2line运行一下：

```shell
C:\SuperLi>arm-linux-androideabi-addr2line -e libnative-lib.so 000064e1 -s 
native-lib.cpp:11
```

可以看到，Android studio中编译出来的so，完全没有各种困扰，很直接的指出的异常发生的行号。

原因是，Android studio会在编译时，自动生成附带有符号表的so。

## 其它

在对应库的编译目录下的Android.mk文件中，增加gcc警告和调试标志：

```makefile
LOCAL_CFLAGS := -Wl,-Map=test.map -g  
```

可能有不一样的效果，为啥说不一样的效果呢？因为俺们没验证，暂时感觉没需求。

## 总结

1、使用addr2line需要附带符号表的so文件。

2、addr2line工具并不是一定能将异常位置定位到行，还需要其它调试手段辅助。

3、如果c/c++库程序是使用Android studio开发，因为生成的so自带符号表，所以使用addr2line工具可以直接定位。