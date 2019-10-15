# OpenGL使用Bug
> 系统：MacOS 10.13.6
> openGL 版本: 4.1 ATI-1.68.21
> opengl 函数管理库：glew2.1.0
> opengl 窗口库：glfw3.3
> IDE：CLion 2018.1
## 编译时openGL 相关函数找不到
**报错日志：**
```shell
Undefined symbols for architecture x86_64:
  "_glClear", referenced from:
      _main in main.cpp.o
  "_glClearColor", referenced from:
      _main in main.cpp.o
  "_glGetString", referenced from:
      _main in main.cpp.o
  "_glViewport", referenced from:
      framebuffer_size_callback(GLFWwindow*, int, int) in main.cpp.o
```

报错的原因是代码中所有涉及到opengl函数调用的地方，都不被x86_64架构系统识别，这点在日志中写的很明白。
**原因和解决方案:**
学习的时候，代码参考网络博客，CMakeLists.txt文件如下：

```shell
cmake_minimum_required(VERSION 3.10)
project(OpenGL)

# 添加头文件
set(CMAKE_CXX_STANDARD 11)
set(GLFW_H /usr/local/Cellar/glfw/3.3/include/GLFW)
set(GLEW_H /usr/local/Cellar/glew/2.1.0/include/GL)
include_directories(${GLFW_H} ${GLEW_H})

# 添加目标链接
set(GLFW_LINK /usr/local/Cellar/glfw/3.3/lib/libglfw.3.3.dylib)
set(GLEW_LINK /usr/local/Cellar/glew/2.1.0/lib/libGLEW.2.1.0.dylib)
link_libraries(${OPENGL} ${GLFW_LINK} ${GLEW_LINK})

# 执行编译命令
set(SOURCE_FILES main.cpp)
add_executable(OpenGL ${SOURCE_FILES})
```
其中`${OPENGL}`原本应该将系统的OpenGL相关库链接到项目中的，但这个写法可能在当前环境不支持。将这种写法换成`"-framework OpenGL"`即可。
替换后，完整的CMakeLists.txt文件如下：
```shell
cmake_minimum_required(VERSION 3.10)
project(OpenGL)

# 添加头文件
set(CMAKE_CXX_STANDARD 11)
set(GLFW_H /usr/local/Cellar/glfw/3.3/include/GLFW)
set(GLEW_H /usr/local/Cellar/glew/2.1.0/include/GL)
include_directories(${GLFW_H} ${GLEW_H})

# 添加目标链接
set(GLFW_LINK /usr/local/Cellar/glfw/3.3/lib/libglfw.3.3.dylib)
set(GLEW_LINK /usr/local/Cellar/glew/2.1.0/lib/libGLEW.2.1.0.dylib)
link_libraries("-framework OpenGL" ${GLFW_LINK} ${GLEW_LINK})

# 执行编译命令
set(SOURCE_FILES main.cpp)
add_executable(OpenGL ${SOURCE_FILES})
```

## 运行时EXC_BAD_ACCESS

**报错日志：**

```shell
Exception Type:        EXC_BAD_ACCESS (SIGSEGV)
Exception Codes:       KERN_INVALID_ADDRESS at 0x0000000000000000
Exception Note:        EXC_CORPSE_NOTIFY

Termination Signal:    Segmentation fault: 11
Termination Reason:    Namespace SIGNAL, Code 0xb
Terminating Process:   exc handler [0]

VM Regions Near 0:
--> 
    __TEXT                 00000001026c2000-00000001026c5000 [   12K] r-x/rwx SM=COW  
 [/Users/**/CLionProjects/OpenGL/cmake-build-debug/OpenGL]

Thread 0 Crashed:: Dispatch queue: com.apple.main-thread
0   ???                           	000000000000000000 0 + 0
1   OpenGL                        	0x00000001026c3d71 main + 465 (main.cpp:55)
2   libdyld.dylib                 	0x00007fff7efbd015 start + 1
```

main.cpp:55 这行的代码是：

```c++
unsigned int VBO;
glGenBuffers(1, &VBO); // main.cpp:55
```

**原因和解决方案:**

出现问题的**客观原因**是：在跟随教程[你好，三角形](https://learnopengl-cn.github.io/01%20Getting%20started/04%20Hello%20Triangle/)练习时，教程中的代码并不完成适用于当前场景，可能是系统方面的原因。**直接原因**：教程的代码中，没有执行`glewInit()`函数，造成随后调用的OpenGL函数无法正确的找到对应函数，从而导致段错误。

来看看GLEW的官方解释：

> The OpenGL Extension Wrangler Library (GLEW) is a cross-platform open-source C/C++ extension loading library. GLEW provides efficient run-time mechanisms for determining which OpenGL extensions are supported on the target platform. OpenGL core and extension functionality is exposed in a single header file. GLEW has been tested on a variety of operating systems, including Windows, Linux, Mac OS X, FreeBSD, Irix, and Solaris.

OpenGL的使用，最基本的两个步骤是：

1. 初始化OpenGL上下文：上下文环境用于记录所有OpenGL相关的状态，位于操作系统的某一个进程中，每个进程可以拥有多个上下文，每个上下文可以描述系统中的一个窗口。简单的说，就是在当前操作系统中创建一个窗口。
2. 定位所有需要使用的OpenGL函数：因为OpenGL只是一个标准，具体的实现由显卡驱动厂商实现，但OpenGL驱动版本居多，无法在编译时制定。这个工作就只能转移到运行时，由开发者指定了。GLEW的作用就是在运行时，获取显卡驱动支持的函数，保存在对应的函数指针中供运行时调用。

所以，对应上述两个步骤，我们应该分别执行：

1. `glfwInit`：以创建上下文。
2. `glewInit`：以加载系统OpenGL函数。

所以，本次未捕获异常只需要在执行任何OpenGL函数调用之前，调用`glewInit`即可。