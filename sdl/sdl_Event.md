# SDL2常用函数&结构分析:SDL_Event&SDL_PollEvent

## SDL_Event

`SDL_Event`是个联合体，是SDL中所有事件处理的核心。 `SDL_Event`是SDL中使用的所有事件结构的并集。 只要知道了那个事件类型对应`SDL_Event`结构的那个成员，使用它是一个简单的事情。

下表罗列了所有`SDL_Event`的所有成员和对应类型。

| Uint32                                                       | **type**     | event type, shared with all events |
| ------------------------------------------------------------ | ------------ | ---------------------------------- |
| SDL_CommonEvent                                              | **common**   | 常见事件数据                       |
| [SDL_WindowEvent](http://wiki.libsdl.org/SDL_WindowEvent)    | **window**   |                                    |
| [SDL_KeyboardEvent](http://wiki.libsdl.org/SDL_KeyboardEvent) | **key**      | 键盘事件数据                       |
| [SDL_TextEditingEvent](http://wiki.libsdl.org/SDL_TextEditingEvent) | **edit**     | 文本编辑事件数据                   |
| [SDL_TextInputEvent](http://wiki.libsdl.org/SDL_TextInputEvent) | **text**     | 文本输入事件数据                   |
| [SDL_MouseMotionEvent](http://wiki.libsdl.org/SDL_MouseMotionEvent) | **motion**   | 鼠标运动事件数据                   |
| [SDL_MouseButtonEvent](http://wiki.libsdl.org/SDL_MouseButtonEvent) | **button**   | 鼠标按钮事件数据                   |
| [SDL_MouseWheelEvent](http://wiki.libsdl.org/SDL_MouseWheelEvent) | **wheel**    | 鼠标滚轮事件数据                   |
| [SDL_JoyAxisEvent](http://wiki.libsdl.org/SDL_JoyAxisEvent)  | **jaxis**    | 操纵杆轴事件数据                   |
| [SDL_JoyBallEvent](http://wiki.libsdl.org/SDL_JoyBallEvent)  | **jball**    | 操纵杆球事件数据                   |
| [SDL_JoyHatEvent](http://wiki.libsdl.org/SDL_JoyHatEvent)    | **jhat**     | 操纵杆帽子事件数据                 |
| [SDL_JoyButtonEvent](http://wiki.libsdl.org/SDL_JoyButtonEvent) | **jbutton**  | 操纵杆按钮事件数据                 |
| [SDL_JoyDeviceEvent](http://wiki.libsdl.org/SDL_JoyDeviceEvent) | **jdevice**  | 操纵杆设备事件数据                 |
| [SDL_ControllerAxisEvent](http://wiki.libsdl.org/SDL_ControllerAxisEvent) | **caxis**    | 游戏控制器轴事件数据               |
| [SDL_ControllerButtonEvent](http://wiki.libsdl.org/SDL_ControllerButtonEvent) | **cbutton**  | 游戏控制器按钮事件数据             |
| [SDL_ControllerDeviceEvent](http://wiki.libsdl.org/SDL_ControllerDeviceEvent) | **cdevice**  | 游戏控制器设备事件数据             |
| [SDL_AudioDeviceEvent](http://wiki.libsdl.org/SDL_AudioDeviceEvent) | **adevice**  | 音频设备事件数据（> = SDL 2.0.4）  |
| [SDL_QuitEvent](http://wiki.libsdl.org/SDL_QuitEvent)        | **quit**     | 退出请求事件数据                   |
| [SDL_UserEvent](http://wiki.libsdl.org/SDL_UserEvent)        | **user**     | 自定义事件数据                     |
| [SDL_SysWMEvent](http://wiki.libsdl.org/SDL_SysWMEvent)      | **syswm**    | 系统相关的窗口事件数据             |
| [SDL_TouchFingerEvent](http://wiki.libsdl.org/SDL_TouchFingerEvent) | **tfinger**  | 触摸手指事件数据                   |
| [SDL_MultiGestureEvent](http://wiki.libsdl.org/SDL_MultiGestureEvent) | **mgesture** | 多指手势数据                       |
| [SDL_DollarGestureEvent](http://wiki.libsdl.org/SDL_DollarGestureEvent) | **dgesture** | 多指手势数据                       |
| [SDL_DropEvent](http://wiki.libsdl.org/SDL_DropEvent)        | **drop**     | 拖拽事件数据                       |

`SDL_Event`联合体包含了外界操作SDL的几乎所有操作时间，所以成员稍微有点多。选了几个简单的联合体成员分析一下：

```c
/**
 *  \brief Fields shared by every event
 */
typedef struct SDL_CommonEvent
{
    Uint32 type;        // 事件类型
    Uint32 timestamp;   // 以毫秒为单位，使用SDL_GetTicks()填充
} SDL_CommonEvent;
/**
 *  键盘按钮事件结构（event.key）
 */
typedef struct SDL_KeyboardEvent
{
    Uint32 type;        // 事件类型：按下按键，按键弹起（SDL_KEYDOWN or SDL_KEYUP）
    Uint32 timestamp;   // 以毫秒为单位，使用SDL_GetTicks()填充
    Uint32 windowID;    // 具有键盘焦点的窗口id
    Uint8 state;        // SDL_PRESSED or SDL_RELEASED
    Uint8 repeat;       // 如果这是重复键，则非零
    Uint8 padding2;
    Uint8 padding3;
    SDL_Keysym keysym;  // 按下或释放的键
} SDL_KeyboardEvent;
```

SDL的所有事件都是存储在一个队列中，而`SDL_Event`的常规操作，就是从这个队列中读取事件或者写入事件。

## SDL_PollEvent

`SDL_PollEvent`函数便是从事件队列中，读取事件的常用函数。

函数原型为：

```c
int SDL_PollEvent(SDL_Event * event);
```

函数的作用是，对当前待处理事件进行轮询。

返回值：如果时间队列中有待处理事件，返回1;如果没有可处理事件，则返回0。

参数event：如果不为NULL，则从队列中删除下一个事件并将其存储在该区域中。

### 示例代码

鼠标移动时间的简单处理代码：

```c
SDL_Event test_event;
while (SDL_PollEvent(&test_event)) {
  switch (test_event.type) {
    case SDL_MOUSEMOTION:
      printf("We got a motion event.\n");
      printf("Current mouse position is: (%d, %d)\n", test_event.motion.x, test_event.motion.y);
      break;
    default:
      printf("Unhandled Event!\n");
      break;
  }
}
printf("Event queue empty.\n");
```

1. 首先需要申明一个`SDL_Event`变量，方便轮巡事件队列时使用。
2. 通过`SDL_PollEvent()`函数获取指向要填充事件信息的`SDL_Event`结构的指针。 我们知道,如果`SDL_PollEvent()`从队列中删除了一个事件，那么事件信息将放在我们的test_event结构中。
3. 为了单独处理每个事件类型，我们使用switch语句。
4. 我们通常需要知道正在寻找什么样的事件以及这些事件的类型。 例如，在示例代码中，我们想要检测用户在我们的应用程序中移动鼠标指针的位置。 我们会查看我们的事件类型，并注意到`SDL_MOUSEMOTION`很可能是我们正在寻找的事件。 查看下表，知道`SDL_MOUSEMOTION`事件是在`SDL_MouseMotionEvent`结构中处理的，然后我们就可以通过`SDL_MouseMotionEvent`的结构，获得我们想要的数据，例如鼠标移动的位置x and y。

