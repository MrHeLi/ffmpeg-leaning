# Android JNI 详解

## 简介

JNI 应该是所有Android老鸟都绕不过的“坎”吧，之所以说是“坎”是因为它比较难，因为他不仅涉及Android开发者的“本命”语言—Java，还要求开发者对C/C++有相当的基础，同时如何协调两种语言的运行时也是重难点之一。

难度是有点，不过，一旦掌握，无疑会给开发者打开一扇通往新世界的大门——openGL、openSL、OpenCV等一系列优秀成熟的库，任君采摘。既可以为你的应用插上高效的翅膀，也为你装逼提供了基础，哇哈哈哈，人生在世，何处不装逼！

本文详细简介JNI在Android中的使用。主要参考https://www3.ntu.edu.sg/home/ehchua/programming/java/JavaNativeInterface.html。不过里边都是英文，	不喜看本文即可。

本文工程目录地址：https://github.com/MrHeLi/JNItest 

## 环境

任何没有环境就说程序的文章都是耍流氓（我估计耍了不少,^_^）。

```wiki
Android Studio 3.1.2

Build #AI-173.4720617, built on April 14, 2018

JRE: 1.8.0_152-release-1024-b02 amd64

JVM: OpenJDK 64-Bit Server VM by JetBrains s.r.o

Windows 7 6.1

```

## 使用Android Studio创建JNI工程

相信想要学习JNI知识的都是些老鸟，创建步骤就简略一些：

File ----->  New  -----> New Project （在这一步需要如下图所示一样勾上C++ 支持）----> Next  ----> Next  ----> Next。

![](E:\study\jni_project_create.PNG)

和普通工程创建不同的是，创建成功后，支持C/C++会多 native-lib.cpp 和 CMakeLists.txt两个文件。关于CMakeLists.txt会另起一篇讲。

TODO: CMakeLists.txt讲解

本文的重点集中在native-lib.cpp所代表的C/C++代码和MainActivity.java代表的java代码之间的交互。

> 现在的开发工具真的是强大，Android studio 在创建工程时就可以将一些JNI的工具加入工程中，再也不用执行一些琐碎的javah、gcc等命令，为Android开发人员解放了双手（也可以说解放了恐惧），节约了时间

### JNI下的MainActivity分析

```java
public class MainActivity extends AppCompatActivity {

    /* 使用静态代码块加载'native-lib'库,该库即为C/C++代码编译后的共享库，
    * 加载后才能让java层调用C/C++的代码。
    * 库名称由CMakeLists.txt文件中的add_library指定。通常此处名称是native-lib的话，
    * 那么编译成功的共享库名称为libnative-lib.so。
    **/
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        TextView tv = (TextView) findViewById(R.id.sample_text);
        tv.setText(stringFromJNI());//本地方法调用和java函数调用毫无二致

    }

    /**
     * 本地'native-lib'库中实现了的本地方法，共享库会打包到本应用中
     * 区别于普通java函数，在函数申明中多了个native字段，以及没有函数体
     */
    public native String stringFromJNI();
```



### JNI下的C/C++代码分析

```c++
#include <jni.h>
#include <iostream>
#include <string>
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "In C/C++:", __VA_ARGS__);
using namespace std;

extern "C" 
JNIEXPORT jstring JNICALL
Java_com_dali_jnitest_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
```



JNI中C/C++代码对应于java中的函数，命令有一定的规则。

我们看一下本例中，java函数和C/C++函数的对应关系：

java部分：

类名：`com.dali.jnitest.MainActivity`

方法名：`public native String stringFromJNI();`

c/c++部分：

`JNIEXPORT jstring JNICALL Java_com_dali_jnitest_MainActivity_stringFromJNI(JNIEnv *env, jobject /* this */)`



* JNIEXPORT : 可以当做JNI方法的函数申明关键字。
* jstring  ： 函数返回值，jstring 对应的是java的String对象。
* JNICALL ： 可以认为是JNI访问的关键字，固定格式啦。
* Java_com_dali_jnitest_MainActivity_stringFromJNI： 在C函数中的方法名格式为**Java_{package_and_classname}_{function_name}(JNI_arguments) **，只需要将包名中的点换成下横线就行。
* JNIEnv *env： JNI的环境引用，一个非常有用的变量，可以通过它调用所有JNI函数。
* jobject /* this */：函数调用者的对象，相当于java层中的**this**



## JNI 基础

上面做了点环境铺垫，接下来开始上正菜。

我们都知道，java的数据类型和C/C++的数据类型并不一致，典型的例子是：java中的String是一个引用数据类型，但在C语言中的String是以NULL结尾的字符串数组。所以协调数据类型，是JNI的重点内容。

JNI 定义了如下的JNI类型用于本地代码中，对应java的数据类型：

1. java 基础数据类型：

   下表是对应关系

   | JNI数据类型 | java数据类型 |
   | ----------- | ------------ |
   | jint        | int          |
   | jbyte       | byte         |
   | jshort      | short        |
   | jlong       | long         |
   | jfloat      | float        |
   | jdouble     | double       |
   | jchar       | char         |
   | jboolean    | boolean      |

   

2. java 引用数据类型：

   | JNI数据类型  | java数据类型            |
   | ------------ | ----------------------- |
   | `jobject`    | `java.lang.Object `     |
   | ` jclass `   | `java.lang.Class`       |
   | `jstring`    | `java.lang.String`      |
   | `jthrowable` | ` java.lang.Throwable ` |

   在java中，数组中的数据类型和JNI的数组类型对应定义：

   | JNI数据类型      | java数据类型 |
   | ---------------- | ------------ |
   | ` jintArray `    | int []       |
   | ` jbyteArray `   | byte []      |
   | ` jshortArray `  | short []     |
   | `jlongArray`     | long []      |
   | `jfloatArray`    | float []     |
   | `jdoubleArray`   | double []    |
   | `jcharArray`     | char []      |
   | `jbooleanArray`  | boolean []   |
   | ` jobjectArray ` | Object []    |

   

### 本地程序调用基本顺序

1. 使用JNI数据类型接收参数（该参数通过java程序调用传递）
2. 对于JNI引用数据类型，将参数转换或者复制为本地类型。比如：jstring 转为 C-string， jintArray转为C's int[]等等。基本数据类型，例如jint, jdouble可以直接使用而不需要转换。
3. 使用本地数据类型执行程序。
4. 创建一个JNI类型的对象，用作返回（return），将程序运行的结果复制到返回对象中。
5. 函数返回（return）。

> 在JNI程序开发过程中，比较困难而极具挑战的是JNI引用类型(例如 `jstring`, `jobject`, `jintArray`, `jobjectArray`) 和C本地数据类型（例如`C-string`, `int[]` ）之间的转换。幸好，JNI环境提供了大量的函数来处理这种转换。

## Java & Native 程序之间参数传递

### 基本数据类型传递

java中的8种基本数据类型可以直接被传递和使用，因为这些类型都在`jni.h`中申明了：

```c
/* Primitive types that match up with Java equivalents. */
typedef uint8_t  jboolean; /* unsigned 8 bits */
typedef int8_t   jbyte;    /* signed 8 bits */
typedef uint16_t jchar;    /* unsigned 16 bits */
typedef int16_t  jshort;   /* signed 16 bits */
typedef int32_t  jint;     /* signed 32 bits */
typedef int64_t  jlong;    /* signed 64 bits */
typedef float    jfloat;   /* 32-bit IEEE 754 */
typedef double   jdouble;  /* 64-bit IEEE 754 */
```

#### 示例

Java 层：

```java
   public class MainActivity extends AppCompatActivity {
   // 在程序开始时使用静态代码块加载'native-lib'库
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
            Log.i("In java", String.valueOf(average(3, 4)));
    }
    //基本数据类型在c/java之间的传递
    public native double average(int arg1, int arg2);
```

C 层：

```c
extern "C"
JNIEXPORT jdouble JNICALL
Java_com_dali_jnitest_MainActivity_average(JNIEnv *env, jobject instance, jint arg1,
                                           jint arg2) {
    jdouble result;//基本数据类型无需变化，在jni.h中已经设置了类型别名
    /**
    *想要使用该打印，请在C文件头增加下列代码：
    *include <android/log.h>
    *define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "In C/C++:", __VA_ARGS__);
    */
    LOGI("arg1: %d, ar2: %d", arg1, arg2);
    result = (arg1 + arg2) / 2;
    return result;
}
```

运行程序：

```shell
com.dali.jnitest I/In C/C++: arg1: 3, ar2: 4
com.dali.jnitest I/In java: 3.0
```



### 字符串传递

#### 示例

java层：

```java
public class MainActivity extends AppCompatActivity {
   // 在程序开始时使用静态代码块加载'native-lib'库
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        Log.i("In java", testString("hello"));
    }
    //字符串在c/java之间的传递
    public native String testString(String str);
```

c层：

```c
extern "C"
JNIEXPORT jstring JNICALL
Java_com_dali_jnitest_MainActivity_testString(JNIEnv *env, jobject instance, jstring str_) {
    //在C中的String是以NULL结尾的字符串数组，需要通过特定方法转换，但在C++中有对应的String，是否可以不     //转换?
    const char *str = env->GetStringUTFChars(str_, 0);
    LOGI("str: %s", str);
    char* returnValue = "hehe ,wo lai le";
    env->ReleaseStringUTFChars(str_, str);
    return env->NewStringUTF(returnValue);
}
```

JNI 定义了`jstring`类型来代表`java`的`String`。C层函数的最后一个参数（JNI类型的`jstring`）是Java层的`String`传递到C层的引用。该程序的返回值同样也是`jstring`类型。

传递字符串远比基本数据类型复杂，因为Java层的`String`是一个对象（引用数据类型），然而C层中的`string`是一个以`NULL`结尾的`char`数组。所以，使用时需要在Java层的`String`（以JNI 的`jstring`表示）和C层的`string`(`char*`)之间转换。

JNI环境（通过参数`JNIENV*`调用）提供了这种转换的函数：

1. 使用`const char* GetStringUTFChars(JNIEnv*, jstring, jboolean*)`将JNI`string`（`jstring`）类型转换为C层的`string`（`char*`）。
2. 使用 `jstring NewStringUTF(JNIEnv*, char*)`将C层的`string`（`char*`）转为JNI`string`（`jstring`）类型。

C层函数的实现步骤为：

1. 从JNI的`jstring`接收数据，并通过`GetStringUTFChars()`转为C层的`string` (`char*`)类型。
2. 然后执行程序，显示接收到的参数数据，并返回另外一个字符串。
3. 将C层的`string` (`char*`)类型通过`NewStringUTF()`函数转换为JNI的`jstring`类型并返回。

运行程序：

```shell
com.dali.jnitest I/In C/C++: str: hello
com.dali.jnitest I/In java: hehe ,wo lai le
```

#### JNI本地String函数

JNI支持Unicode(16字节字符串)和UTF-8(1-3字节编码)不同格式字符串之间的转换。UTF-8编码的字符串和C语言中的字符串一样是以`NULL`结尾的`char`数组，用于C/C++程序中。

这些JNI字符串（`jstring`）为：

```C
/** UTF-8 String (encoded to 1-3 byte, backward compatible with 7-bit ASCII)
* 获取以NULL结尾的字符数组，也就是C-string
*/
// 返回表示UTF-8编码字符串的数组指针
const char * GetStringUTFChars(jstring string, jboolean *isCopy);
// 通知VM 本地代码不再需要UTF引用。
void ReleaseStringUTFChars(jstring string, const char *utf);
// 根据字符串数组，构造一个UTF-8编码的java String新对象
jstring NewStringUTF(const char *bytes);
// 返回UTF-8编码字符串的长度
jsize GetStringUTFLength(jstring string);
// 将从偏移量start开始的length长度的Unicode字符转换为UTF-8编码，并将结果放在给定的缓冲区buf中。
void GetStringUTFRegion(jstring str, jsize start, jsize length, char *buf);

  
// Unicode Strings (16-bit character)
// 返回指向Unicode字符数组的指针
const jchar * GetStringChars(jstring string, jboolean *isCopy);
// 通知VM本机代码不再需要访问字符。
void ReleaseStringChars(jstring string, const jchar *chars);
// 从Unicode字符数组构造一个新的java.lang.String对象。
jstring NewString(const jchar *unicodeChars, jsize length);
// 返回Java字符串的长度（Unicode字符数）。
jsize GetStringLength(jstring string);
// 将从偏移量=start开始的length长度的Unicode字符数复制到给定的缓冲区buf。
void GetStringRegion(jstring str, jsize start, jsize length, jchar *buf);
```

#### UTF-8 strings & C-strings

`GetStringUTFChars（）`函数可用于从给定的Java的`jstring`创建新的C字符串（`char *`）。 如果无法分配内存，则该函数返回`NULL`。 检查`NULL`是一个好习惯。

第三个参数`isCopy（of jboolean *）`，它是一个“in-out”参数，如果返回的字符串是原始`java.lang.String`实例的副本，则将设置为`JNI_TRUE`。 如果返回的字符串是指向原始`String`实例的直接指针，则它将设置为`JNI_FALSE `- 在这种情况下，本机代码不应修改返回的字符串的内容。 如果可能，JNI运行时将尝试返回直接指针; 否则，它返回一份副本。 尽管如此，我们很少对修改底层字符串感兴趣，并且经常传递NULL指针。

不使用使用`GetStringUTFChars（）`返回的字符串时，需要来释放内存和引用以便可以对其进行垃圾回收时，始终调用`ReleaseStringUTFChars（）`。

`NewStringUTF（）`函数使用给定的C字符串创建一个新的JNI字符串（`jstring`）。

JDK 1.2引入了`GetStringUTFRegion（）`，它将`jstring`（或从长度开始的一部分）复制到“预分配”的C的`char`数组中。 可以使用它们代替`GetStringUTFChars（）`。 由于预先分配了C的数组，因此不需要`isCopy`。

JDK 1.2还引入了`Get / ReleaseStringCritical（）`函数。 与`GetStringUTFChars（）`类似，如果可能，它返回一个直接指针; 否则，它返回一份副本。 本机方法不应阻止（对于IO或其他）一对`GetStringCritical（）`和`ReleaseStringCritical（）`调用。

有关详细说明，请始终参阅“Java Native Interface Specification”@ http://docs.oracle.com/javase/7/docs/technotes/guides/jni/index.html。



### 基本数据类型数组传递

#### 示例

java层：

```java
    public class MainActivity extends AppCompatActivity {

    // 在程序开始时使用静态代码块加载'native-lib'库
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        int[] numbers = {22, 33, 33};
        double[] results = sumAndAverage(numbers);
        Log.i("In java, the sum is ", ""+results[0]);
        Log.i("In java, the average is", " "+results[1]);
    }
    //基础数据类型的数组传递
    public native double[] sumAndAverage(int arr[]);
```



C层：

在Java中，数组（`array`）和类（`class`）一样，是引用数据类型。总共有9中数组类型，其中，8种基本数据类型数组和一个`java.lang.Object`数组类型。对于8中基本数据类型，JNI分别定义了以下数据类型与之对应`jintArray`, `jbyteArray`, `jshortArray`, `jlongArray`, `jfloatArray`, `jdoubleArray`, `jcharArray`, `jbooleanArray`。

同样的，你在编码过程中需要在JNI数据和本地数组之间转换，例如：`jintArray` 与 C的`jint[]`,`jdoubleArray` 与 C的`jdouble[]`。JNI环境提供了支持这些转换的函数如下：

1. JNI ——》C本地代码：

   使用`jint* GetIntArrayElements(JNIEnv *env, jintArray a, jboolean *iscopy)`，从JNI的`jintArray`获取C语言的本地数组`jint[]`  。

2. C本地代码 ——》JNI：

   * 首先使用`jintArray NewIntArray(JNIEnv *env, jsize len)`申请一片内存
   * 然后使用`void SetIntArrayRegion(JNIEnv *env, jintArray a, jsize start, jsize len, const jint *buf)`将数据从C本地代码的`jint[]` 复制到JNI层的`jintArray`。

> 如上面的两个函数特定用于int类型，与此类似，JNI中总共有8组类似函数对应于8中基本数据类型。

回到示例代码，基本数据类型数组传递的典型本地代码步骤如下：

1. 从JNI参数接收数组数据，转换为C代码的本地数据（例如，`jint[]`）。
2. 执行程序。
3. 将本地C代码的数据（例如，`jdouble[]`）转换为JNI数组（例如，`jdoubleArray`）,并且返回JNI数据。

代码如下：

```c
extern "C"
JNIEXPORT jdoubleArray JNICALL
Java_com_dali_jnitest_MainActivity_sumAndAverage(JNIEnv *env, jobject instance,
                                                               jintArray arr_) {
    //从JNI参数接收数组数据，转换为C代码的本地数据（例如，jint[]）。
    jint *arr = env->GetIntArrayElements(arr_, NULL);
    if (NULL == arr) return NULL;
    jsize length = env->GetArrayLength(arr_);
    // 执行程序
    jint sum = 0;
    int i;
    for (i = 0; i < length; i++) {
        sum += arr[i];
    }
    jdouble average = (jdouble)sum / length;
    env->ReleaseIntArrayElements(arr_, arr, 0); // release resources

    jdouble outCArray[] = {(jdouble)sum, average};
    // 将本地C代码的数据（例如，jdouble[]）转换为JNI数组（例如，jdoubleArray）,并且返回JNI数据。
    jdoubleArray outJNIArray = env->NewDoubleArray(2);  // allocate
    if (NULL == outJNIArray) return NULL;
    env->SetDoubleArrayRegion(outJNIArray, 0 , 2, outCArray);  // copy
    return outJNIArray;
}
```

执行结果：

```shell
com.dali.jnitest I/In java, the sum is: 88.0
com.dali.jnitest I/In java, the average is:  29.333333333333332
```

#### JNI 基本数据类型数组相关函数

```c
// ArrayType: jintArray, jbyteArray, jshortArray, jlongArray, jfloatArray, jdoubleArray, jcharArray, jbooleanArray
// PrimitiveType: int, byte, short, long, float, double, char, boolean
// NativeType: jint, jbyte, jshort, jlong, jfloat, jdouble, jchar, jboolean
NativeType * Get<PrimitiveType>ArrayElements(ArrayType array, jboolean *isCopy);
void Release<PrimitiveType>ArrayElements(ArrayType array, NativeType *elems, jint mode);
void Get<PrimitiveType>ArrayRegion(ArrayType array, jsize start, jsize length, NativeType *buffer);
void Set<PrimitiveType>ArrayRegion(ArrayType array, jsize start, jsize length, const NativeType *buffer);
ArrayType New<PrimitiveType>Array(jsize length);
void * GetPrimitiveArrayCritical(jarray array, jboolean *isCopy);
void ReleasePrimitiveArrayCritical(jarray array, void *carray, jint mode);
```

总结如下：

* `GET|Release<*PrimitiveType*>ArrayElements()` 函数可用于根据java的`jxxxArray`创建C代码的本地`jxxx[]数组。`
* `GET | Set <PrimitiveType> ArrayRegion()`可用于将`jxxxArray`（或从长度开始的一部分）复制到预分配的C本地数组`jxxx []`。
* `New <PrimitiveType> Array()`可用于分配给定大小的新`jxxxArray`。 然后，您可以使用`Set <PrimitiveType> ArrayRegion()`函数从本机数组`jxxx []`中填充其内容。
* `Get | ReleasePrimitiveArrayCritical()`函数不允许在`get`和`elease`之间阻塞调用。

### 访问对象的变量和函数回调

#### 访问对象的内部变量

##### 示例

java层：

```java
public class MainActivity extends AppCompatActivity {
    // 在程序开始时使用静态代码块加载'native-lib'库
    static {
        System.loadLibrary("native-lib");
    }
    // 实例的内部变量
    private int number = 88;
    private String message = "帅呆了！";
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        modifyInstanceVariable();
        System.out.println("In Java, int is " + number);
        System.out.println("In Java, String is " + message);
    }
    
    public native void modifyInstanceVariable();
}
```

C层：

```c
extern "C"
JNIEXPORT void JNICALL
Java_com_dali_jnitest_MainActivity_modifyInstanceVariable(JNIEnv *env, jobject instance) {

    // 获得调用java层对象的引用
    jclass thisClass = env->GetObjectClass(instance);

    // int
    // 获取调用java层对象中number变量的fieldID
    jfieldID fidNumber = env->GetFieldID(thisClass, "number", "I");
    if (NULL == fidNumber) return;

    // 通过fieldID获取number变量中的值
    jint number = env->GetIntField(instance, fidNumber);
    LOGI("In C, the int is %d\n", number);

    // 修改变量
    number = 99;
    env->SetIntField(instance, fidNumber, number);

    // 获取调用java层对象中message变量的fieldID
    jfieldID fidMessage = env->GetFieldID(thisClass, "message", "Ljava/lang/String;");
    if (NULL == fidMessage) return;

    // String
    // 通过fieldID获取object变量中的值
    jstring message = static_cast<jstring>(env->GetObjectField(instance, fidMessage));

    // 通过JNI字符串创建C代码字符串
    const char *cStr = env->GetStringUTFChars(message, NULL);
    if (NULL == cStr) return;

    LOGI("In C, the string is %s\n", cStr);
    env->ReleaseStringUTFChars(message, cStr);

    //创建C代码字符串，并分配给JNI字符串
    message = env->NewStringUTF("C：不你很蠢");
    if (NULL == message) return;

    // 修改实例变量
    env->SetObjectField(instance, fidMessage, message);
}
```

执行：

```shell
com.dali.jnitest I/In C/C++: In C, the int is 88
com.dali.jnitest I/In C/C++: In C, the string is java：帅呆了！
com.dali.jnitest I/In Java, int is: 99
com.dali.jnitest I/In Java, String is: C：不你很蠢
```

##### 访问实例变量的基本步骤

1. 通过`GetObjectClass()`函数class对象的引用。
2. 从类引用中通过`GetFieldID（）`获取要访问的实例变量的字段ID。 参数中，需要提供变量名称及其字段描述符（或签名）。 对于Java类，字段描述符的形式为“L <fully-qualified-name>”，点用正斜杠（/）替换，例如，String的类描述符是“Ljava / lang / String;”。 对于基本数据类型，使用“I”表示int，“B”表示字节，“S”表示short，“J”表示long，“F”表示float，“D”表示double，“C”表示char，以及“Z”表示 boolean。 对于数组，需要加上前缀“[”，例如“[Ljava / lang / Object;” 表示一个Object数组; “[I”代表一个int数组。
3. 基于Field ID，通过`GetObjectField（）`或`Get <primitive-type> Field（）`函数检索实例变量。
4. 根据提供字段ID，并通过`SetObjectField（）`或`Set <primitive-type> Field（）`函数，更新实例变量。



用于访问实例变量的JNI函数如下：

```c
//返回java层调用者实例class
jclass GetObjectClass(jobject obj);
//返回访问者实例的field ID   
jfieldID GetFieldID(jclass cls, const char *name, const char *sig);
//Get/Set 实例中变量的值，
//<type> 包含8中基本数据类型，加上一个Object类型。
NativeType Get<type>Field(jobject obj, jfieldID fieldID);
void Set<type>Field(jobject obj, jfieldID fieldID, NativeType value);
```

#### 访问对象的静态变量

访问静态变量类似于访问实例变量，只是调用函数改为`GetStaticFieldID()`，`Get | SetStaticObjectField()`，`Get | SetStatic <Primitive-type> Field()`之类的。

##### 示例

java层：

```java
public class MainActivity extends AppCompatActivity {

    // 在程序开始时使用静态代码块加载'native-lib'库
    static {
        System.loadLibrary("native-lib");
    }
    // 静态变量
    private static double number = 55.66;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        modifyStaticVariable();
        Log.i("In Java, double is ", "" + number);
    }

    private native void modifyStaticVariable();
}
```



C 层：

```c
extern "C"
JNIEXPORT void JNICALL
Java_com_dali_jnitest_MainActivity_modifyStaticVariable(JNIEnv *env, jobject instance) {

    jclass cls = env->GetObjectClass(instance);
    jfieldID fieldID = env->GetStaticFieldID(cls, "number", "D");
    if (fieldID == NULL) return;
    jdouble number = env->GetStaticDoubleField(cls, fieldID);
    LOGI("In C, the double is %f\n", number);
    number = 77.88;
    env->SetStaticDoubleField(cls, fieldID, number);

}
```



执行：

```shell
com.dali.jnitest I/In C/C++: In C, the double is 55.660000
com.dali.jnitest I/In Java, double is: 77.88
```

用于访问静态变量的JNI函数如下：

```c
//返回类的静态变量的字段ID。
jfieldID GetStaticFieldID(jclass cls, const char *name, const char *sig);
//Get/Set 实例中静态变量的值，
//<type> 包含8中基本数据类型，加上一个Object类型。
NativeType GetStatic<type>Field(jclass clazz, jfieldID fieldID);
void SetStatic<type>Field(jclass clazz, jfieldID fieldID, NativeType value);
```

#### 回调java函数和静态函数

##### 示例

java层：

```java
public class MainActivity extends AppCompatActivity {

    // 在程序开始时使用静态代码块加载'native-lib'库
    static {
        System.loadLibrary("native-lib");
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        nativeMethod();
    }
    // 申明本地函数
    private native void nativeMethod();
    // 用于测试C代码调用
    private void callback() {
        Log.i("In Java", "");
    }
    private void callback(String message) {
        Log.i("In Java with ", message);
    }
    // 用于测试C代码调用
    private double callbackAverage(int n1, int n2) {
        return ((double)n1 + n2) / 2.0;
    }
    // Static method to be called back
    private static String callbackStatic() {
        return "From static Java method";
    }
}
```

C 层：

java类声明一个名为`nativeMethod()`的本地方法，并调用此`nativeMethod()`。 反过来，在C层`nativeMetho()`回调此类中定义的各种实例和静态方法。

```c
extern "C"
JNIEXPORT void JNICALL
Java_com_dali_jnitest_MainActivity_nativeMethod(JNIEnv *env, jobject instance) {

    // 获取java层调用的class对象的引用
    jclass thisClass = env->GetObjectClass(instance);

    // 获取Method ID 用于调用“callback”函数，该函数没有参数，无返回值
    jmethodID midCallBack = env->GetMethodID(thisClass, "callback", "()V");
    if (NULL == midCallBack) return;
    LOGI("In C, call back Java's callback()\n");
    // 根据Method ID 调用该函数（无返回值）
    env->CallVoidMethod(instance, midCallBack);

    jmethodID midCallBackStr = env->GetMethodID(thisClass, "callback", "(Ljava/lang/String;)V");
    if (NULL == midCallBackStr) return;
    LOGI("In C, call back Java's called(String)\n");
    jstring message = env->NewStringUTF("Hello from C");
    env->CallVoidMethod(instance, midCallBackStr, message);

    jmethodID midCallBackAverage = env->GetMethodID(thisClass, "callbackAverage", "(II)D");
    if (NULL == midCallBackAverage) return;
    jdouble average = env->CallDoubleMethod(instance, midCallBackAverage, 2, 3);
    LOGI("In C, the average is %f\n", average);

    jmethodID midCallBackStatic = env->GetStaticMethodID(thisClass, "callbackStatic", "()Ljava/lang/String;");
    if (NULL == midCallBackStatic) return;
    jstring resultJNIStr = static_cast<jstring>(env->CallStaticObjectMethod(thisClass, midCallBackStatic));
    const char *resultCStr = env->GetStringUTFChars(resultJNIStr, NULL);
    if (NULL == resultCStr) return;
    LOGI("In C, the returned string is %s\n", resultCStr);
    env->ReleaseStringUTFChars(resultJNIStr, resultCStr);

}
```

执行代码：

```shell
com.dali.jnitest I/In C/C++: In C, call back Java's callback() In C, call back Java's  called(String)
com.dali.jnitest I/In Java with: Hello from C
com.dali.jnitest I/In C/C++: In C, the average is 2.500000 In C, the returned string is From static Java method
```

##### 在本地代码中回调java对象中的函数的基本步骤

1. 通过`GetObjectClass() `获取class对象的引用。

2. 通过`GetMethodID()`以及class对象引用获取Method ID。该函数参数中，需要提供函数名和签名。签名的格式为： "`(parameters)return-type`" 。太复杂了么，别着急，你可以通过`javap`工具 (Class File Disassembler)  列出java代码中的函数签名，`-s`用于打印签名，`-p`用于显示私有成员（函数或变量），具体如下：

   ```shell
     private void callback();
       Signature: ()V
    
     private void callback(java.lang.String);
       Signature: (Ljava/lang/String;)V
    
     private double callbackAverage(int, int);
       Signature: (II)D
    
     private static java.lang.String callbackStatic();
       Signature: ()Ljava/lang/String;
   ```

3. 基于Method ID, 你可以调用`Call<Primitive-type>Method()` 或者 `CallVoidMethod()` 亦或 `CallObjectMethod()`, 这些函数分别返回的类型为： `<*Primitive-type*>`, `void` and `Object`。 在参数列表之前附加参数（如果有）。 对于非void返回类型，该方法返回一个值。

访问实例函数和实例的静态函数的JNI函数如下：

```c
// 返回java层实例(类或接口)函数的method ID
jmethodID GetMethodID(jclass cls, const char *name, const char *sig);
// 调用类的method
// <type> 包含8种基本数据类型
NativeType Call<type>Method(jobject obj, jmethodID methodID, ...);
NativeType Call<type>MethodA(jobject obj, jmethodID methodID, const jvalue *args);
NativeType Call<type>MethodV(jobject obj, jmethodID methodID, va_list args);
// 返回Java层实例静态函数的method ID  
jmethodID GetStaticMethodID(jclass cls, const char *name, const char *sig);
// 调用对象的函数
// <type> 包含8种基本数据类型
NativeType CallStatic<type>Method(jclass clazz, jmethodID methodID, ...);
NativeType CallStatic<type>MethodA(jclass clazz, jmethodID methodID, const jvalue *args);
NativeType CallStatic<type>MethodV(jclass clazz, jmethodID methodID, va_list args);
```

#### 访问父类的重载函数

JNI提供了一组`CallNonvirtual <Type> Method（）`函数来调用已在此类中重写的超类'实例方法（类似于Java子类中的super。* methodName *（）调用）：

1. 通过`GetMethodID()`函数获取Method ID。
2. 基于Method ID，使用object, superclass, 和arguments 访问`CallNonvirtual<Type>Method()`函数。

访问超累的JNI函数如下：

```
NativeType CallNonvirtual<type>Method(jobject obj, jclass cls, jmethodID methodID, ...);
NativeType CallNonvirtual<type>MethodA(jobject obj, jclass cls, jmethodID methodID, const jvalue *args);
NativeType CallNonvirtual<type>MethodV(jobject obj, jclass cls, jmethodID methodID, va_list args);
```

### 创建对象和对象数组

#### 创建对象

可以通过`NewObject（）`和`newObjectArray（）`函数在本机代码中构造`jobject`和`jobjectArray`，并将它们传递回Java程序。

##### 示例

java代码：

```java
public class MainActivity extends AppCompatActivity {

    // 在程序开始时使用静态代码块加载'native-lib'库
    static {
        System.loadLibrary("native-lib");
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Log.i("In Java,the number is :", "" + getIntegerObject(9999));
    }
    // 本地函数，调用之后返回创建完毕的java对象实例
    // 使用给定的int返回Integer对象
    private native Integer getIntegerObject(int number);
```

C 代码：

```c
extern "C"
JNIEXPORT jobject JNICALL
Java_com_dali_jnitest_MainActivity_getIntegerObject(JNIEnv *env, jobject instance, jint number) {
    // 获取java.lang.Integer类的引用
    jclass cls = env->FindClass("java/lang/Integer");
    // 获取Integer的构造函数Method ID,使用int参数
    jmethodID midInit = env->GetMethodID(cls, "<init>", "(I)V");
    if (NULL == midInit) return NULL;
    // 使用jint参数调用构造函数，分配新的实例
    jobject newObj = env->NewObject(cls, midInit, number);

    // 调用String的toString()方法调用打印
    jmethodID midToString = env->GetMethodID(cls, "toString", "()Ljava/lang/String;");
    if (NULL == midToString) return NULL;
    jstring resultStr = static_cast<jstring>(env->CallObjectMethod(newObj, midToString));
    const char *resultCStr = env->GetStringUTFChars(resultStr, NULL);
    LOGI("In C: the number is %s\n", resultCStr);

    return newObj;

}
```

代码执行：

```shell
com.dali.jnitest I/In C/C++: In C: the number is 9999
com.dali.jnitest I/In Java,the number is :: 9999用于创建对象（jobject）的JNI函数是：
```

####  用于创建对象（jobject）的JNI函数是：

```c
jclass FindClass(JNIEnv *env, const char *name);
// 构造一个新的Java对象。 method ID指定要调用的构造方法 
jobject NewObject(JNIEnv *env, jclass cls, jmethodID methodID, ...);
jobject NewObjectA(JNIEnv *env, jclass cls, jmethodID methodID, const jvalue *args);
jobject NewObjectV(JNIEnv *env, jclass cls, jmethodID methodID, va_list args);
// 在不调用对象的任何构造函数的情况下分配新的Java对象。 
jobject AllocObject(JNIEnv *env, jclass cls);
```



#### 对象数组

##### 示例

java层：

```java
public class MainActivity extends AppCompatActivity {

    // 在程序开始时使用静态代码块加载'native-lib'库
    static {
        System.loadLibrary("native-lib");
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Integer[] numbers = {11, 22, 32};  // auto-box
        Double[] results = sumAndAverage2(numbers); // auto-unbox
        Log.i("In Java,the sum is ","" + results[0]);
        Log.i("In Java,the average is ","" + results[1]);
    }

    // 本地函数：接收一个Integer[], 返回Double[2]，[0]为和，[1]为平均值
    private native Double[] sumAndAverage2(Integer[] numbers);
```



C层：

```C
extern "C"
JNIEXPORT jobjectArray JNICALL
Java_com_dali_jnitest_MainActivity_sumAndAverage2(JNIEnv *env,jobject instance,jobjectArray numbers) {
    // 获取java.lang.Integer类的引用
    jclass classInteger = env->FindClass("java/lang/Integer");
    // 使用Integer.intValue() 函数
    jmethodID midIntValue = env->GetMethodID(classInteger, "intValue", "()I");
    if (NULL == midIntValue) return NULL;

    // 获取数据中每个元素Get the value of each Integer object in the array
    jsize length = env->GetArrayLength(numbers);
    jint sum = 0;
    int i;
    for (i = 0; i < length; i++) {
        jobject objInteger = env->GetObjectArrayElement(numbers, i);
        if (NULL == objInteger) return NULL;
        jint value = env->CallIntMethod(objInteger, midIntValue);
        sum += value;
    }
    double average = (double)sum / length;
    LOGI("In C, the sum is %d\n", sum);
    LOGI("In C, the average is %f\n", average);

    // 获取java.lang.Double类的引用
    jclass classDouble = env->FindClass("java/lang/Double");

    // 创建一个长度为2的java.lang.Double类型数组（）jobjectArray
    jobjectArray outJNIArray = env->NewObjectArray(2, classDouble, NULL);

    // 通过构造函数构造两个Double类型对象
    jmethodID midDoubleInit = env->GetMethodID(classDouble, "<init>", "(D)V");
    if (NULL == midDoubleInit) return NULL;
    jobject objSum = env->NewObject(classDouble, midDoubleInit, (double)sum);
    jobject objAve = env->NewObject(classDouble, midDoubleInit, average);
    // 初始化jobjectArray
    env->SetObjectArrayElement(outJNIArray, 0, objSum);
    env->SetObjectArrayElement(outJNIArray, 1, objAve);

    return outJNIArray;
}
```

代码执行：

```shell
com.dali.jnitest I/In C/C++: In C, the sum is 65
                             In C, the average is 21.666667
com.dali.jnitest I/In Java,the sum is: 65.0
com.dali.jnitest I/In Java,the average is: 21.666666666666668
```

与可以批量处理的原始数组不同，对于对象数组，你需要使用`Get | SetObjectArrayElement（）`来处理每个元素。

用于创建和操作对象数组（jobjectArray）的JNI函数如下：

```c
// 构造一个包含elementClass对象的数组
//所有元素都设置为initialElement。
jobjectArray NewObjectArray(jsize length, jclass elementClass, jobject initialElement);
// 返回一个对象数组元素 
jobject GetObjectArrayElement(jobjectArray array, jsize index);
// 设置一个对象数组元素 
void SetObjectArrayElement(jobjectArray array, jsize index, jobject value);
```

### 本地和全局引用

管理引用对于编写高效的程序至关重要。 例如，我们经常使用`FindClass（）`，`GetMethodID（）`，`GetFieldID（）`来检索本机函数中的`jclass`，`jmethodID`和`jfieldID`。 应该获取一次并缓存以供后续使用的值，而不是执行重复调用，以减少开销。

JNI将C代码使用的对象引用（对于`jobject`）分为两类：本地引用和全局引用：

1. 在C函数中创建本地引用，并在方法退出后释放。 它在本地方法的持续时间内有效。 你还可以使用JNI函数`DeleteLocalRef（）`显式地使本地引用无效，以便可以在中间进行垃圾回收。 对象作为本地引用传递给本地方法。 JNI函数返回的所有Java对象（`jobject`）都是本地引用。
2. 在程序员通过`DeleteGlobalRef（）`JNI函数显式释放它之前，全局引用仍然存在。 您可以通过JNI函数`NewGlobalRef（）`从本地引用创建新的全局引用。

##### 示例：

java代码：

```java
public class MainActivity extends AppCompatActivity {

    // 在程序开始时使用静态代码块加载'native-lib'库
    static {
        System.loadLibrary("native-lib");
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Log.i("java:", ""+getIntegerObject2(1));
        Log.i("java:", ""+getIntegerObject2(2));
        Log.i("java:", ""+anotherGetIntegerObject(11));
        Log.i("java:", ""+anotherGetIntegerObject(12));
        Log.i("java:", ""+getIntegerObject2(3));
        Log.i("java:", ""+anotherGetIntegerObject(13));
    }
    // 返回给定int值的java.lang.Integer的本地方法。
    private native Integer getIntegerObject2(int number);
    // 另一个本地方法也返回带有给定int值的java.lang.Integer。
    private native Integer anotherGetIntegerObject(int number);
}
```



C代码：

```c
// java类"java.lang.Integer"的全局引用
static jclass classInteger;
static jmethodID midIntegerInit;

jobject getInteger(JNIEnv *env, jobject instance, jint number) {

    // 如果classInteger为NULL，获取java.lang.Integer的类引用
    if (NULL == classInteger) {
        LOGI("Find java.lang.Integer\n");
        classInteger = env->FindClass("java/lang/Integer");
        // 不加下面这行会出现报错：jni error (app bug): accessed stale local reference
        classInteger = static_cast<jclass>(env->NewGlobalRef(classInteger));
    }
    if (NULL == classInteger) return NULL;

    // 如果midIntegerInit为空，获取Integer构造函数的method ID
    if (NULL == midIntegerInit) {
        LOGI("Get Method ID for java.lang.Integer's constructor\n");
        midIntegerInit = env->GetMethodID(classInteger, "<init>", "(I)V");
    }
    if (NULL == midIntegerInit) return NULL;

    // 调用构造函数以使用int参数分配新实例
    jobject newObj = env->NewObject(classInteger, midIntegerInit, number);
    LOGI("In C, constructed java.lang.Integer with number %d\n", number);
    return newObj;
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_dali_jnitest_MainActivity_getIntegerObject2(JNIEnv *env, jobject instance, jint number) {
    return getInteger(env, instance, number);
}
extern "C"
JNIEXPORT jobject JNICALL
Java_com_dali_jnitest_MainActivity_anotherGetIntegerObject(JNIEnv *env, jobject instance,jint number) {
    return getInteger(env, instance, number);
}
```

在上面的程序中，我们使用`classInteger = static_cast<jclass>(env->NewGlobalRef(classInteger));`将`classInteger `变为全局引用，记得要使用`env->DeleteLocalRef(classInteger);`取消引用。

> 请注意，jmethodID和jfieldID不是jobject，并且无法创建全局引用。

### Debug JNI 程序

TODO：