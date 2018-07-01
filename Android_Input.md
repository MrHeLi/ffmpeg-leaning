# Android Input命令

## input

`input`是Android系统中的一个特殊的命令，用于模拟遥控器、键盘、鼠标的各种按键操作。我们都知道Android是阉割版本的Linux系统，Linux中很多命令在Android系统中是没有的。但是他们之间并没有包含的关系，Android系统中有些特有的东西（命令、属性）在Linux中也是没有的。



## 命令路径

可以通过`which` 命令查看该命令的位置：

```shell
130|superli:/ # which -a input        
/system/bin/input
superli:/ # 
```

## 命令概要

### Android 4.4以前

使用`help`命令查看命令如何使用：

```shell
root@hwH30-U10:/ # input --help
Error: Unknown command: --help
usage: input ...
       input text <string>
       input keyevent <key code number or name>
       input [touchscreen|touchpad] tap <x> <y>
       input [touchscreen|touchpad] swipe <x1> <y1> <x2> <y2>
       input trackball press
       input trackball roll <dx> <dy>
root@hwH30-U10:/ #
```



### Android 4.4+

使用`help`命令查看命令如何使用：

```shell
130|superli:/ # input --help
Error: Unknown command: --help	## 未知命令？为什么会出现这个？？？算了，不重要，可能系统抽风了。
Usage: input [<source>] <command> [<arg>...]

The sources are: ##模拟的输入设备类型
      keyboard
      mouse
      joystick
      touchnavigation
      touchpad
      trackball
      dpad
      stylus
      gamepad
      touchscreen

The commands and default sources are: ## 不指定source时，命令的默认输入设备类型为：
      text <string> (Default: touchscreen)
      keyevent [--longpress] <key code number or name> ... (Default: keyboard)
      tap <x> <y> (Default: touchscreen)
      swipe <x1> <y1> <x2> <y2> [duration(ms)] (Default: touchscreen)
      press (Default: trackball)
      roll <dx> <dy> (Default: trackball)
superli:/ # 
```

## 命令解读和使用

后续内容主要分析Android 4.4+版本关于input命令的使用，当然4.4以前的版本也可以参考，毕竟差别并不是很大。



命令格式：`input [<source>] <command> [<arg>...]`

命令格式（中文版）：`input [<设备类型（可选）>] <命令> [<参数（可选）>...]`

命令格式中可以看到有两个可选的部分：

* 设备类型：设备类型不输入时，使用命令的默认设备类型。默认类型见下文。
* 参数：当命令没有参数时，可不输入（想了半天，想不出来什么命令不需要参数）

### 设备类型



|     sources     |          模拟的输入设备类型          |
| :-------------: | :----------------------------------: |
|    keyboard     |                 键盘                 |
|      mouse      |                 鼠标                 |
|    joystick     | 操纵杆（玩过游戏手柄的同学应该懂吧） |
| touchnavigation |           ？？触摸导航？？           |
|    touchpad     |                触摸板                |
|    trackball    |          轨迹球（啥意思？）          |
|      dpad       |                什么鬼                |
|     stylus      |          触控笔（styluses）          |
|     gamepad     |               游戏手柄               |
|   touchscreen   |                触摸屏                |

设备类型还挺丰富的，好多普通人根本接触不到，哎，我就是普通人。

### 命令列表和默认设备类型

| 命令                                             | 默认设备类型 | 示例                              |
| ------------------------------------------------ | ------------ | --------------------------------- |
| text <string>                                    | touchscreen  | `input text "hello"`              |
| keyevent [--longpress] <key code number or name> | keyboard     | `input keyevent 4`                |
| tap <x> <y>                                      | touchscreen  | `input tap 500 500`               |
| swipe <x1> <y1> <x2> <y2> [duration(ms)]         | touchscreen  | `input swipe 500 500 600 500 200` |
| press                                            | trackball    | `input press`                     |
| roll <dx> <dy>                                   | trackball    | `input roll 500 500`              |



## 命令演练与解释

命令列表中，关于`press`和`roll`都是针对触控球的设备设计的，因为手中没有设备，而且这种设备现实中使用的比较少，所以就不做演示

### text

该命令用于模拟触摸屏的虚拟键盘输入字符串。

比如，下面使用命令输入“hello”：

```shell
HWVKY:/ $ input text hello
```

text命令后面的字符都会以字符串的形式输入，如果恰好设备的焦点在一个可输入控件（通常为`EditText`）中，那么可输入控件中就会出现“hello”字样。

> 如果命令执行后，“hello”并没有显示，请检查一下控件是否有限定输入类型。

### keyevent

该命令用以默认按键输入，对应的输入设备场景可能是遥控器、键盘等。

例如，下面使用命令模拟遥控器的返回键：

```shell
HWVKY:/ $ input keyevent 4
```

命令执行后，设备中能很明显的看到有回退动作。命令中的参数“4”，对应的是keyevent中的返回键。下面列出部分常用keyevent事件的键值列表。

| Keyevent            | value | 备注   |
| ------------------- | ----- | ------ |
| KEYCODE_BACK        | 4     | 返回键 |
| KEYCODE_HOME        | 3     | HOME键 |
| KEYCODE_MENU        | 82    | 菜单键 |
| KEYCODE_DPAD_UP     | 19    | 上     |
| KEYCODE_DPAD_DOWN   | 20    | 下     |
| KEYCODE_DPAD_LEFT   | 21    | 左     |
| KEYCODE_DPAD_RIGHT  | 22    | 右     |
| KEYCODE_DPAD_CENTER | 23    | OK键   |
| KEYCODE_VOLUME_UP   | 24    | 音量+  |
| KEYCODE_VOLUME_DOWN | 25    | 音量-  |

如果列表中不满足你的需求，可以去`android.view.KeyEvent.java`中查看

### tap

该命令用于模拟触摸操作，感觉就是点击一下指定位置，可以让指定的点变相的获取焦点。

例如，下面的命令，如果位于首页，500*500的坐标处恰好有一个应用图标，那么命令执行后，会打开该应用：

```shell
HWVKY:/ $ input tap 500 500
```

### swipe

该命令用于模拟手势滑动操作

例如，下面的命令模拟，从`500*500`的坐标滑动到`600*500`的位置：

```shell
HWVKY:/ $ input swipe 500 500 600 500
```

上边的命令执行后，屏幕瞬间就会滑动，那么我要慢慢的滑动怎么办呢，看下面的命令:

```shell
HWVKY:/ $ input swipe 500 500 600 500 500
```

两个命令的区别在于第二个多了一个参数，该参数表示该滑动需要执行的时长为500ms。

值得一提的是，当参数中的两个坐标点使用一个点时，秒变长按事件：

```shell
HWVKY:/ $ input swipe 500 500 500 500 1000
```

该命令表示：长按坐标为`500*500`的点1秒钟。







