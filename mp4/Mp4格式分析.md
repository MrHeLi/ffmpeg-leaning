[TOC]

## MP4相关文档

[ISO标准文档](https://standards.iso.org/ittf/PubliclyAvailableStandards/c068960_ISO_IEC_14496-12_2015.zip)

[MPEG标准相关链接](https://mpeg.chiariglione.org/)

## MP4分析工具

MP4 Reader：展示MP4 Box的16进制数据，同时显示这些数据代表的信息。

Mp4Info ： 简单显示MP4 Box的结构信息。

Elecard HEVC Analyzer：一个及其强大的媒体解析工具，解析的内容非常详细，不仅限于MP4。

[分析Mp4结构](http://download.tsi.telecom-paristech.fr/gpac/mp4box.js)：一个在线解析MP4信息的网站

## 术语和缩略语

* **box**：由唯一的标志符和长度限定的面向对象的结构块，在某些规格中被称为**atom**，它们是一个意思。一个MP4文件是一个个box组成，一个Box中可以包含另一个Box，这种Box被称为**Container Box**。
* **track**：媒体文件中samples集合关于时间的序列。通常，一个track表示一个音频或者视频时间序列。
* **chunk**：一个track中，连续的sample集合。
* **hint track**：特殊的track，不包含媒体数据，而是包含了将一个或多个track打包成媒体流的指令信息。
* **hinter**：在仅包含媒体的文件上运行的工具，用于向文件添加一个或多个提示轨道，从而便于流式传输。
* **sample**：即媒体数据的采样，对于非hint track 而言 sample中包含一帧或多帧音频或视频数据。对于hint track来说，则定义了一个或者多个流媒体包格式。
* **sample** table：打包方式，规定track中sample的时序和物理布局。

## Mp4文件的组织结构

MP4文件由一系列的box对象组成，所有的数据都包含在这些box中，除此以外，文件中再无其它数据。此外，所有的Mp4文件中，首先有且仅有一个File Type 类型的Box。

## Box对象结构

Box由包含了**size**（Box大小）和**type**（Box 类型）的**Box Header**开始。**Header**允许紧凑或扩展的size（32位或64位）和**紧凑**和**扩展**的type（32位或者完整的通用唯一标识符，即UUID）。

大多数标准的Box使用的都是32位size和32位type，只有包含了媒体数据的Box需要使用64位size。

这里的**size**，指的是包括Header在内的整个Box占用的大小。在size指定的空间中，除了Box header占用的空间外，其它空间由真实的数据（BoxData）数据占据。这些数据，可能包括其它子Box、也可能是媒体数据。

我们可以利用该信息对MP4文件进行分析。需要注意的是，数据存储使用大端字节序。

在ISO标准文档中，对于Boxheader的定义伪代码是：

```c++
class Box (unsigned int(32) boxtype, optional unsigned int(8)[16]  extended_type) {
    unsigned int(32) size;
    unsigned int(32) type = boxtype;
    if (size==1) {
        unsigned int(64) largesize;
    } else if (size==0) {
        // box extends to end of file
    }
    if (boxtype==‘uuid’) {
        unsigned int(8)[16] usertype = extended_type;
    }
}
```

转化为下图：

![BoxHeader](.\BoxHeader.JPG)

* **size**字段：一般情况，BoxHeader只包含32位的size，除了BoxHeader占用的空间外，剩余的size保存了Box的数据。此外，有两种特殊情况：

1. 当size为1时，BoxHeader中将多出一个64位的largesize，此时Box的大小由largesize决定。
2. 当size为0时，表示当前Box是文件中最后一个Box，并且从该Box的Header开始直到文件末尾都是该Box的数据（通常用于保存Media Data）。

* **type**字段：type字段定义了Box的类型，通常该类型都是通过紧凑类型，也就是32位表示。当`type`的值等于`uuid`时，表示该Box的类型为用户扩展类型，用长度为16的`int`类型无符号数组表示该扩展类型。

**type字段**对于Box而言，非常重要，如果Box的type类型无法识别，该Box应该被忽略或者跳过不处理。

### FullBox对象结构

FullBox结构是对Box结构的一种扩展，除了包含所有Box结构的字段外，还包含了version和flags字段：

* **version字段**：version是一个整数值（int），用于表示该Box的版本。
* **flags字段**： 是一个map类型的标志。

FullBox的伪代码和结构图如下：

```c++
class FullBox(unsigned int(32) boxtype, unsigned int(8) v, bit(24) f) extends Box(boxtype) {
    unsigned int(8) version = v;
    bit(24) flags = f;
}
```

![BoxHeader](.\FullBoxHeader.JPG)

在读取或使用FullBox结构时应该注意，无法识别的`version`应该被忽略或者跳过不处理。

## 常见的Box

### File Type Box

> Box类型：'ftyp'
>
> 容器：	文件
>
> 是否强制：是
>
> 数量：	1个

ISO_IEC_14496-12_2015版本规范规定，Mp4格式文件中必须有且只有一个'ftyp'类型的Box。

为了与早期没有'ftype'类型的版本兼容。当文件中，包含FTYP类型的Box，且该Box包含范围`Major_brand='mp41'`, `minor_version=0`,以及单独指定兼容性compatible_brands='mp41'，则该文件需要按照有FTYP Box的方式读取。

**File Type Box**的语法定义如下：

```c++
class FileTypeBox extends Box('ftyp') {
    unsigned int(32) major_brand; // 一个品牌的标志符
    unsigned int(32) minor_version; // 是主要品牌的次要版本的信息，为一个整数
    unsigned int(32) compatible_brands[]; // 兼容的品牌列表
}
```

我们很少见到纯粹的`'ftyp'`类型的Box，而`FileTypeBox`这种为了兼容而存在的Box相对比较常见一些。	

一个典型的`FileTypeBox`的结构数据如下(用MP4 Reader查看)：

![BoxHeader](.\FTYP_Box.JPG)

### Movie 结构

### Movie Box

> Box类型：'moov'
>
> 容器：	文件
>
> 是否强制：是
>
> 数量：	1个

用于展示的媒体数据，都包含在`moov`中，包括音频、视频，同时也包含metedata数据。该Box一般紧跟在`ftyp`后面，`moov`占用了大部分的文件空间。'moov'类型的Box主要功能是包含其它类型的Box。

### Movie Header Box

> Box类型：'mvhd '
>
> 容器：	Movie Box ('moov')
>
> 是否强制：是
>
> 数量：	1个

该Box定了与媒体数据无关，与媒体文件相关的整体信息。定义语法为：

```c++
class MovieHeaderBox extends FullBox('mvhd', version, 0) {
    if (version==1) { // 当version==1时
        unsigned int(64) creation_time;
        unsigned int(64) modification_time;
        unsigned int(32) timescale;
        unsigned int(64) duration;
    } else { // version==0
        unsigned int(32) creation_time;
        unsigned int(32) modification_time;
        unsigned int(32) timescale;
        unsigned int(32) duration;
    }
    template int(32) rate = 0x00010000; // typically 1.0
    template int(16) volume = 0x0100; // typically, full volume
    const bit(16) reserved = 0;
    const unsigned int(32)[2] reserved = 0;
    template int(32)[9] matrix =
    { 0x00010000,0,0,0,0x00010000,0,0,0,0x40000000 };
    // Unity matrix
    bit(32)[6] pre_defined = 0;
    unsigned int(32) next_track_ID;
}
```

各字段含义：

| 字段                  | 含义                                                         |
| --------------------- | ------------------------------------------------------------ |
| **version**           | Box的版本，在FullBox中定义                                   |
| **creation_time**     | 整数，用于表示图像的创建时间（秒为单位，从1904.01.01 00:00开始） |
| **modification_time** | 整数，表示图像最近的修改时间（秒为单位，从1904.01.01 00:00开始） |
| **timescale**         | 整数，指定整体图像文件的时间尺度，表示一秒钟的时间单位数。例如时间坐标轴上，一秒钟分为60个单位，那么时间尺度为60。该值与duration一起计算媒体播放时长。 |
| **duration**          | 整数，申明媒体持续时长（在指定的时间尺度下），该值和最长的track持续时间的值保持一致。如果无法确定，值将被设置为1s。实际播放时间计算公式：duration/timescale秒。 |
| **rate**              | 播放速度，通常为1.0                                          |
| **volume**            | 播放音量，值为1.0时表示最高音量                              |
| **matrix**            | 视频变换矩阵（不明白是干啥用的）                             |
| **next_track_ID**     | 非零整数，用于表示继续往文件中追加track时使用的id值。该值肯定比已有trackID大 |

### Track Box

> Box类型：'trak '
>
> 容器：	Movie Box (‘moov’)
>
> 是否强制：是
>
> 数量：	1个或多个

Track Box表示包含了单个track媒体数据的容器Box(Container Box)。通常一个Mp4文件媒体数据中，包含一个或多个track，每个track都包含自己的时间和空间信息，且彼此独立。每个track中，也包含了与之相关的媒体信息。

在这里，**Track存在的目的有两个**：

1. 包含媒体数据（media track）。
2. 包含流协议的分组化信息（hint track）。

Track Box就是一个简单的Container Box，它会包含两个关键Box，**Track Header Box**和**Media Box**。

### Track Header Box

> Box类型：'tkhd'
>
> 容器：	Track Box ('trak')	 
>
> 是否强制：是
>
> 数量：	1个

该Box用于描述Track的基本属性，一个track中有且只有一个Track Header Box。

媒体轨道的flag标志的默认值为7（即111：track_enabled，track_in_movie，track_in_preview）。如果在媒体数据中所有轨道都没有设置track_in_movie和track_in_preview，则应将所有轨道视为在所有轨道上都设置了两个标志。

服务器提示轨道（hint track）应将track_in_movie和track_in_preview设置为0，以便在本地回放和预览时忽略它们。

定义语法：

```c++
class TrackHeaderBox extends FullBox('tkhd'， version， flags){
    if (version==1) {
        unsigned int(64) creation_time;
        unsigned int(64) modification_time;
        unsigned int(32) track_ID;
        const unsigned int(32) reserved = 0;
        unsigned int(64) duration;
    } else { // version==0
        unsigned int(32) creation_time;
        unsigned int(32) modification_time;
        unsigned int(32) track_ID;
        const unsigned int(32) reserved = 0;
        unsigned int(32) duration;
    }
    const unsigned int(32)[2] reserved = 0;
    template int(16) layer = 0;
    template int(16) alternate_group = 0;
    template int(16) volume = {if track_is_audio 0x0100 else 0};
    const unsigned int(16) reserved = 0;
    template int(32)[9] matrix=
    { 0x00010000,0,0,0,0x00010000,0,0,0,0x40000000 };
    // unity matrix
    unsigned int(32) width;
    unsigned int(32) height;
}
```

定义中，很多字段在前面都已经解释过了。下面将未解释和需要补充说明的列出：

| 字段                | 含义                                                         |
| ------------------- | ------------------------------------------------------------ |
| **track_ID**        | 当前track的ID，该值全局唯一，不能重复使用且不能为0           |
| **duration**        | 整数，申明媒体持续时长（在Movie Header Box指定的时间尺度下）。如果无法确定，值将被设置为1s。 |
| **layer**           | 指定视频track的前后顺序，该值通常为0。如果有-1和0两个track，那么-1所在track将在0所在track的前方显示。 |
| **alternate_group** | 是一个整数，指定一组或一组轨道，该值默认为0，表示没有和其它轨道关联。（不懂） |

需要特别说明的字段是**flag**，这是一个占用24bit空间的整数，用于定义以下属性：

* **Track_enabled** ：表示该track是否可用，Flag值为0x000001。一个不可用状态的track（Flag值为0x000000）数据会被当做不显示处理。
* **Track_in_movie** ：表示该track被用于播放，Flag值为0x000002。
* **Track_in_preview** ：表示该track用于预览媒体文件。Flag值为0x000004。
* **Track_size_is_aspect_ratio** ：表示track的宽高字段不是以像素为单位，且该值表示宽高比。Flag值为0x000008。

**width&height**：

* 对于文字或者字幕track，宽高取决于编码格式，用于描述推荐渲染区域的尺寸。对于这样的轨道，值0x0也可用于指示数据可以以任何大小呈现，并没有指定最优显示尺寸，它的实际大小可以通过外部上下文或通过重用另一个轨道的宽高来确定。对于这种轨道，也可以使用标志track_size_is_aspect_ratio。
* 对于可不见内容的track（例如音频文件）宽高都应该设置为0。
* 除此之外的其他track，width&height指定了可见track的宽高。

### Media Box

> Box类型：'mdia'
>
> 容器：	Track Box ('trak')	 
>
> 是否强制：是
>
> 数量：	1个

Media Box包含声明有关轨道中媒体数据信息的所有对象。这就是个简单的容器对象，没有别的需要说明了。

### Media Header Box 

> Box类型：'mdhd'
>
> 容器：	Media Box ('mdia')	 
>
> 是否强制：是
>
> 数量：	1个

Media Header Box 申明整体的媒体信息，这些信息也和track中的媒体特性相关。

定义语法：

```c++
class MediaHeaderBox extends FullBox('mdhd', version, 0) {
    if (version==1) {
        unsigned int(64) creation_time;
        unsigned int(64) modification_time;
        unsigned int(32) timescale;
        unsigned int(64) duration;
    } else { // version==0
        unsigned int(32) creation_time;
        unsigned int(32) modification_time;
        unsigned int(32) timescale;
        unsigned int(32) duration;
    }
    bit(1) pad = 0;
    unsigned int(5)[3] language; // ISO-639-2/T语言编码
    unsigned int(16) pre_defined = 0;
}
```

总共有三个额外字段：

* pad ：一个占位符，默认值为0，占用一个bit。
* language：当前track的语言，该字段总长为16bit，和pad字段组成两个字节。
* pre_defined：默认值为0。

### Handler Reference Box : 'hdlr'

> Box类型：'hdlr'
>
> 容器：	Media Box ('mdia')或者Meta Box('meta')	 
>
> 是否强制：是
>
> 数量：	1个

该Box申明了track的媒体类型，因此决定了track中媒体数据播放的处理过程。

例如，向解码器传送视频格式数据的媒体数据将被存储在video track中，这些数据将由视频处理程序处理。

当该Box位于Meta Box中时，该Box申明Meta Box内容的结构和格式。

定义语法：

```c++
class HandlerBox extends FullBox('hdlr', version = 0, 0) {
    unsigned int(32) pre_defined = 0;
    unsigned int(32) handler_type;
    const unsigned int(32)[3] reserved = 0;
    string name;
}
```

* handler_type：
  1. 当该Box位于Media Box中，表示当前track的处理类型。如：video、audio、hint track、meta track等。
  2. 当存在于Meta Box中时，包含指定Meta Box的内容格式。 可以在住Meta Box中使用值'null'来指示它
     仅用于保存资源。
* name：是一个以UTF-8字符结尾的以null结尾的字符串，它为轨道类型提供了一个人类可读的名称（用于调试和检查）。

handler_type的值，实际上就是字符串对应的16进制编码，如：

* 视频：vide       0x76, 0x69, 0x64, 0x65, 0x00, 0x00, 0x00, 0x00
* 音频：soun      0x73, 0x6F, 0x75, 0x6E, 0x00, 0x00, 0x00, 0x00

### Media Information Box : 'minf '

> Box类型：'minf '
>
> 容器：	Media Box ('mdia')
>
> 是否强制：是
>
> 数量：	1个

该box包含了track中申明了媒体特征信息的所有对象。这是一个普通的Box，定义如下：

```c++
class MediaInformationBox extends Box('minf') {}
```

### Video media header  : 'vmhd' & 'smhd'

####  'vmhd'，对应视频

> Box类型：'vmhd'
>
> 容器：	Media Information Box ('minf')	 
>
> 是否强制：是
>
> 数量：	1个

video track中的视频相关的媒体信息。定义如下：

```c++
aligned(8) class VideoMediaHeaderBox
    extends FullBox(‘vmhd’, version = 0, 1) { // flag = 1
    template unsigned int(16) graphicsmode = 0; // copy, see below
    template unsigned int(16)[3] opcolor = {0, 0, 0};
}
```

对于视频而言，flag字段值应该为1.

* graphicsmode ：指定此视频轨道的合成模式，来自以下枚举集，可以通过派生规范进行扩展：copy = 0复制现有图像
* opcolor ：是一组3原色值（红色，绿色，蓝色），供graphicsmode 使用。

####  'smhd'，对应音频

> Box类型：'smhd'
>
> 容器：	Media Information Box ('minf')	 
>
> 是否强制：是
>
> 数量：	1个

和视频类似，该Box包含音频相关的媒体信息。该Header对于所有包含了audio的track适用。定义如下：

```c++
aligned(8) class SoundMediaHeaderBox extends FullBox(‘smhd’, version = 0, 0) {
    template int(16) balance = 0;
    const unsigned int(16) reserved = 0;
}
```

* balance：是一个固定小数点表示单声道音轨（track）在立体声空间中的位置。0表示中间（通常都为0），-1.0表示声道，1.0表示右声道。

### Track Data Layout Structures : 'dinf'

> Box类型：'dinf'
>
> 容器：	Media Information Box ('minf')或者Meta Box ('meta')
>
> 是否强制：是
>
> 数量：	1个

该Box包含媒体信息在track中的位置信息。本身就是一个简单的Box，包含一个'dref'类型的Box：

```c++
aligned(8) class DataInformationBox extends Box('dinf') {}
```

### Data Reference Box : 'dref '

> Box类型：'dref '
>
> 容器：	Data Information Box ('dinf')
>
> 是否强制：是
>
> 数量：	1个

> Box类型：'url' 'urn'
>
> 容器：	Data Information Box ('dref ')
>
> 是否强制：是('dref'中，至少有一个'url'或者'urn')
>
> 数量：	1个或多个

Data Reference Box对象包含一个数据引用表（通常是URL），用于声明播放需要使用到的媒体数据的位置。

引用表中的数据索引（index）和track中的数据绑定。通过这种方式，一个track可能被分割为多个sources。

定义：

```c++
aligned(8) class DataEntryUrlBox (bit(24) flags) extends FullBox('url', version = 0, flags) {
    string location;
}
aligned(8) class DataEntryUrnBox (bit(24) flags) extends FullBox('urn', version = 0, flags) {
    string name;
    string location;
}
aligned(8) class DataReferenceBox extends FullBox('dref', version = 0, 0) {
    unsigned int(32) entry_count;
    for (i=1; i <= entry_count; i++) {
    	DataEntryBox(entry_version, entry_flags) data_entry;
    }
}
```

各字段的含义如下：

* entry_count：条目的数量，即DataEntryBox的数量。
* entry_flags：一个24bits表示的整数，如果值为0x000001意味着Media data 在当前文件中，并且Movie Box包含了数据的引用。
* data_entry ：是一个URL或者URN类型的条目，Name是URN，那么URN条目是必需的。 Location是一个URL，那么URL条目是必需的，在URN条目中是可选的，它提供了一个位置来查找具有给定名称的资源。 他们都是使用UTF-8字符的以null结尾的字符串。
  如果设置了自包含标志，则使用URL表单并且不存在字符串值; 该框终止于entry-flags字段。 URL类型应该是提供文件的服务（例如，类型为file，http，ftp等的URL），并且理想情况下哪些服务也允许随机访问。 相对URL是允许的，并且和包含了指定数据的Movie Box的文件相关。

### Sample Table Box : 'stbl'

> Box类型：'stbl' 
>
> 容器：	Media Information Box ('minf')	 
>
> 是否强制：是
>
> 数量：	1个

Sample Table Box可以翻译成**样本表容器**，是MP4规范中非常重要的容器。它包含了一个track中所有媒体样本的所有时间和数据的引用。使用容器中的sample信息，可以定位sample的媒体时间、定位其类型，决定其大小，以及在其它容器中，确定相邻sample的offset。

如果包含了Sample Table Box的track没有数据，那么该Sample Table Box不需要包含任何子Box。

如果Sample Table Box所在的track有数据引用，该Sample Table Box必须包含如下子Box：

* Sample Description
* Sample Size
* Sample To Chunk
* Chunk Offset 

此外，Sample Description Box应包含至少一个条目。Sample Description Box是必需的，因为它包含数据引用索引字段，该字段指示用于检索媒体样本的Data Reference Box 。
如果没有Sample Description Box，则无法确定媒体样本的存储位置。
Sync Sample Box 是可选的。 如果不存在Sync Sample Box，则所有样本都是同步样本。

### Sample Description Box : 'stsd'

> Box类型：'stsd' 
>
> 容器：	Sample Table Box ('stbl')
>
> 是否强制：是
>
> 数量：	1个

样本描述容器：提供了编码类型和用于编码初始化的详细信息。在ISO/IEC 14496-12:2015 标准中，相关的定义如下：

```c++
aligned(8) class SampleDescriptionBox (unsigned int(32) handler_type) extends FullBox('stsd', version, 0){
    int i ;
    unsigned int(32) entry_count; // entry 的数量
    for (i = 1 ; i <= entry_count ; i++){
    	SampleEntry(); // 这表示一个SampleEntry派生类的实例
    }
}
aligned(8) abstract class SampleEntry (unsigned int(32) format) extends Box(format){
    const unsigned int(8)[6] reserved = 0;
    unsigned int(16) data_reference_index;
}
class BitRateBox extends Box(‘btrt’){
    unsigned int(32) bufferSizeDB; // bufferSizeDB以字节为单位给出基本流的解码缓冲区的大小。
    unsigned int(32) maxBitrate; // 最大比特率
    unsigned int(32) avgBitrate; // 平均比特率
}
```

可以看到，**SampleDescriptionBox**中有几个**SampleEntry**的子类实例是通过**entry_count**确定。

* data_reference_index：是一个整数，包含用于检索与使用此样本描述容器的样本关联的数据的数据引用的索引。数据引用存储在Data Reference Boxes中。 索引的范围从1到数据引用的数量。

**SampleEntry**是一个抽象类，它的子类比较丰富，如：视频相关的VisualSampleEntry、音频相关的AudioSampleEntry 、字幕相关的SubtitleSampleEntry 等。这里简单展开解读一下VisualSampleEntry。它的定义为：

```c++
class VisualSampleEntry(codingname) extends SampleEntry (codingname){
    unsigned int(16) pre_defined = 0;
    const unsigned int(16) reserved = 0;
    unsigned int(32)[3] pre_defined = 0;
    unsigned int(16) width; // sample的宽
    unsigned int(16) height; // sample的高
    template unsigned int(32) horizresolution = 0x00480000; // 72 dpi 水平分辨率
    template unsigned int(32) vertresolution = 0x00480000; // 72 dpi 垂直分辨率
    const unsigned int(32) reserved = 0;
    template unsigned int(16) frame_count = 1; // 每一个sample中包含多少帧数据，默认为1
    string[32] compressorname; // 一个固定的32字节字段，第一个字节设置为要显示的字节数，然后是可显示数据的字节数，然后填充以完成总共32个字节（包括大小字节）。 该字段可以设置为0。
    template unsigned int(16) depth = 0x0018;
    int(16) pre_defined = -1;
    // other boxes from derived specifications
    CleanApertureBox clap; // optional
    PixelAspectRatioBox pasp; // optional
}
```

* codingname：表示的就是编码名称，这个名称是通过VisualSampleEntry的子类传递的，至于具体的子类，在ISO/IEC 14496-12:2015 标准中并没有定义，而是定义在其它标准文档中，例如ISO/IEC 14496-15标准中的AVCSampleEntry：

  ```c++
  class AVCSampleEntry() extends VisualSampleEntry ('avc1'){
      AVCConfigurationBox config;
      MPEG4BitRateBox (); // optional，可选
      MPEG4ExtensionDescriptorsBox (); // optional
  }
  ```

  它组合了其它的一些Box，如'mp4a'这里就不展开了。

在具体解析**stsd** Box 时，只需要按照定义和字段大小就可以解析出数据的编码信息了。

### Decoding Time to Sample Box ：'stts'

> Box类型：'stts' 
>
> 容器：	Sample Table Box ('stbl')
>
> 是否强制：是
>
> 数量：	1个

该Box包含了一个解码时间到sample索引的映射关系表（Table entries）。

entries表中的每个entry给出了具有相同时长的连续sample的数和这些sample的时间间隔值。将这些时间间隔相加在一起，就可以得到一个完整的time与sample之间的映射。将所有的时间间隔相加在一起，就可以得到该track的总时长。

Decoding Time to Sample Box包含解码时间增量，即每个sample的显示时间计算公式如下：

$$DT(n + 1) = DT(n) + STTS(n)$$

其中$STTS(n)$是sample n的时间间隔，$$DT(n)$$是sample n的显示时间。

sample 的entry都是按照解码的时间排序，所以时间增量$$DT(n + 1)$$都是非负的。Table entries根据每个sample在媒体流中的顺序和时长对它们进行描述。如果连续的sample时长相同，会被放在同一个table  entry中。

Box定义如下：

```c++
aligned(8) class TimeToSampleBox extends FullBox(’stts’, version = 0, 0) {
    unsigned int(32) entry_count; // 映射表中entry实体的数量
    int i;
    for (i=0; i < entry_count; i++) {
        unsigned int(32) sample_count; // 指定时长的样本数量
        unsigned int(32) sample_delta; // sample样本的时长
    }
}
```

![box_stts](.\box_stts.JPG)

如上图，表示1080.mp4文件的stts Box中有1500个时长为512的sample。

### Sync Sample Box：'stss'

> Box类型：'stss' 
>
> 容器：	Sample Table Box ('stbl')
>
> 是否强制：否
>
> 数量：	0个或1个

该Box提供流中同步样本的紧凑标记，标记了关键帧，提供随机访问点标记。 它包含了一个table， table的每个entry标记了一个sample，该sample是媒体关键帧。table按sample编号的严格递增顺序排列。如果不存在该Box，则每个sample都是关键帧。

定义如下：

```c++
aligned(8) class SyncSampleBox extends FullBox(‘stss’, version = 0, 0) {
    unsigned int(32) entry_count; // 表中entry的数量。如果为0意味着流中没有关键帧，table也为空
    int i;
    for (i=0; i < entry_count; i++) {
   		unsigned int(32) sample_number; // 流中关键帧的编号
    }
}
```

下图表示：该流有18个关键帧，对应的sample编号为1、151、211...

![box_stss](.\box_stss.JPG)

### Composition Time to Sample Box : 'ctts '

> Box类型：'ctts' 
>
> 容器：	Sample Table Box ('stbl')
>
> 是否强制：否
>
> 数量：	0个或1个

Composition Time to Sample Box也被称为时间合成偏移表，每个音视频的sample都有自己解码顺序和显示顺序。对每个sample而言，解码顺序和显示顺序往往都不相同。这时，就有了Composition Time to Sample Box。

有一种特殊情况，sample的解码顺序和显示顺序一致。那么mp4文件中将不会出现Composition Time to Sample Box，Decoding Time to Sample Box ：'stts'  将即提供解码顺序，也代表了显示顺序，同时可以根据时长计算每个sample开始和结束的时间。

一般而言，显示顺序和和解码顺序通常不一致（涉及到I、P、B帧的解码顺序）。此时Decoding Time to Sample Box提供解码顺序，Composition Time to Sample Box通过差值的形体，可以计算出显示时间。

Composition Time to Sample Box提供了从解码时间到显示时间的映射关系，如下：

$$CT_{(n)} = DT_{(n)} + CTTS{(n)}$$

其中，$DT_{(n)}$是sample n 的解码时间，通过Decoding Time to Sample Box计算获得，$CTTS_{(n)}$是sampe n在表中的entry，$CT_{(n)}$即为sample n的显示时间。

对于完成GOP的映射表示例如下：

![closed_gop_exsample](.\closed_gop_exsample.JPG)

对1080.mp4文件，使用MP4 Reader查看'ctts'如下：

![box_ctts](.\box_ctts.JPG)

定义：

```c++
aligned(8) class CompositionOffsetBox extends FullBox(‘ctts’, version, 0) {
    unsigned int(32) entry_count; // entry数
    int i;
    if (version==0) {
        for (i=0; i < entry_count; i++) {
            unsigned int(32) sample_count;
            unsigned int(32) sample_offset; // 不同版本 偏移量的类型不同
        }
    } else if (version == 1) {
        for (i=0; i < entry_count; i++) {
            unsigned int(32) sample_count;
            signed int(32) sample_offset;
        }
    }
}
```

### Sample To Chunk Box ：'stsc'

> Box类型：'stsc' 
>
> 容器：	Sample Table Box ('stbl')
>
> 是否强制：是
>
> 数量：	1个

媒体数据中的sample被分组为一个个chunk，一个chunk可以包含多个sample。 chunk可以具有不同的大小，并且chunk内的sample可以具有不同的大小。 

Sample To Chunk Box中有一个table，该table可用于查找包含样本的块，其位置以及关联的sample信息。

每个entry都给出具有相同特征的一组chunk的第一个chunk的索引，和这些具有相同特征的chunk中每个包含几个sample。 

定义：

```c++
aligned(8) class SampleToChunkBox extends FullBox(‘stsc’, version = 0, 0) {
    unsigned int(32) entry_count; // 表中 entry的数量
    for (i=1; i <= entry_count; i++) {
        unsigned int(32) first_chunk; 
        unsigned int(32) samples_per_chunk; // 每个chunk中的sample数量
        unsigned int(32) sample_description_index;
    }
}
```

* first_chunk：当前chunk组中，第一个chunk的index。该chunk组中，每个chunk都相同的samples‐per‐chunk和 sample‐description‐index 信息。在一个track中，的第一个chunk的index值为1(该Box的第一个记录中的first_chunk字段的值为1，表示第一个sample映射到第一个chunk)。
* sample_description_index：一个整数，它给出了描述此块中样本的样本条目的索引。 index的范围从1到Sample Description Box中的样本条目数。

对1080.mp4文件，使用MP4 Reader查看'stsc'如下：

![box_stsc](.\box_stsc.JPG)

### Sample Size Boxes ：'stsz' & 'stz2 '

> Box类型：'stsz' 、 'stz2 '
>
> 容器：	Sample Table Box ('stbl')
>
> 是否强制：是
>
> 数量：	1个

Sample Size Boxes给出了sample的总数，以及一张关于sample大小的表。该表显示了sample和sample size的映射关系。

Sample Size Boxes有两个版本：

第一个版本固定大小为32-bit的字段，用于显示每个sample的大小。该版本允许track中所有的sample拥有恒定的大小。

第二个版本允许更小的字段，一边在不同大小事节约空间。第一个版本是最佳兼容性的选择。

定义：

```java
aligned(8) class SampleSizeBox extends FullBox(‘stsz’, version = 0, 0) {
    unsigned int(32) sample_size; // 固定大小字段
    unsigned int(32) sample_count; // 总的sample数
    if (sample_size==0) { // 如果使用了固定大小的版本，没必要为每个sample指定大小了。
        for (i=1; i <= sample_count; i++) {
      	  unsigned int(32) entry_size; // 如果没有使用固定大小的版本，为每个sample指定大小。
        }
    }
}
```

对1080.mp4文件，使用MP4 Reader查看'stsz'如下：

![box_stsz](.\box_stsz.JPG)

### Chunk Offset Box : 'stco' & 'co64'

> Box类型：'stco' 、 'co64'
>
> 容器：	Sample Table Box ('stbl')
>
> 是否强制：是
>
> 数量：	1个

Chunk Offset Box 包含一个chunk的偏移表，将每个chunk的索引提供给包含文件。 有两种版本，允许使用32位或64位偏移。 后者在管理非常大的演示文稿时非常有用。 这些变体中的至多一个将出现在样本表的任何单个实例中。如果Box类型是'stco' ，则是32位， 'co64'为64位。

偏移是文件偏移，而不是文件中任何Box（例如Media Data Box）的偏移。 这允许在没有任何Box结构的文件中引用媒体数据。 它还意味着在构建一个自包含的ISO文件时必须小心，因为Movie Box的大小会影响媒体数据的块偏移。

定义：

```c++
aligned(8) class ChunkOffsetBox extends FullBox(‘stco’, version = 0, 0) {
    unsigned int(32) entry_count; // 表中entry的数量
    for (i=1; i <= entry_count; i++) {
    	unsigned int(32) chunk_offset;
    }
}
aligned(8) class ChunkLargeOffsetBox extends FullBox(‘co64’, version = 0, 0) {
    unsigned int(32) entry_count;
    for (i=1; i <= entry_count; i++) {
    	unsigned int(64) chunk_offset;
    }
}
```

* chunk_offset ：是一个32位或64位整数，它表示当前chunk在媒体文件中的起始偏移量。

对1080.mp4文件，使用MP4 Reader查看'stco'如下：

![box_stco](.\box_stco.JPG)

## 附：MP4文件结构列表

| Box类型 |      |      |      |      |      | 必须 | 描述                                                         |
| ------- | ---- | ---- | ---- | ---- | ---- | ---- | ------------------------------------------------------------ |
| ftyp    |      |      |      |      |      | √    | file type and compatibility                                  |
| pdin    |      |      |      |      |      |      | progressive download information                             |
| moov    |      |      |      |      |      | √    | container for all the metadata                               |
|         | mvhd |      |      |      |      | √    | movie header, overall declarations                           |
|         | trak |      |      |      |      | √    | container for an individual track or stream                  |
|         |      | tkhd |      |      |      | √    | track header, overall information about the track            |
|         |      | tref |      |      |      |      | track reference container                                    |
|         |      | edts |      |      |      |      | edit list container                                          |
|         |      |      | elst |      |      |      | an edit list                                                 |
|         |      | mdia |      |      |      | √    | container for the media information in a track               |
|         |      |      | mdhd |      |      | √    | media header, overall information about the media            |
|         |      |      | hdlr |      |      | √    | handler, declares the media (handler) type                   |
|         |      |      | minf |      |      | √    | media information container                                  |
|         |      |      |      | vmhd |      |      | video media header, overall information (video track only)   |
|         |      |      |      | smhd |      |      | sound media header, overall information (sound track only)   |
|         |      |      |      | hmhd |      |      | hint media header, overall information (hint track only)     |
|         |      |      |      | nmhd |      |      | Null media header, overall information (some tracks only)    |
|         |      |      |      | dinf |      | √    | data information box, container                              |
|         |      |      |      |      | dref | √    | data reference box, declares source(s) of media data in track |
|         |      |      |      | stbl |      | √    | sample table box, container for the time/space map           |
|         |      |      |      |      | stsd | √    | sample descriptions (codec types, initialization etc.)       |
|         |      |      |      |      | stts | √    | (decoding) time-to-sample                                    |
|         |      |      |      |      | ctts |      | (composition) time to sample                                 |
|         |      |      |      |      | stsc | √    | sample-to-chunk, partial data-offsetinformation              |
|         |      |      |      |      | stsz |      | sample sizes (framing)                                       |
|         |      |      |      |      | stz2 |      | compact sample sizes (framing)                               |
|         |      |      |      |      | stco | √    | chunk offset, partial data-offset information                |
|         |      |      |      |      | co64 |      | 64-bit chunk offset                                          |
|         |      |      |      |      | stss |      | sync sample table (random access points)                     |
|         |      |      |      |      | stsh |      | shadow sync sample table                                     |
|         |      |      |      |      | padb |      | sample padding bits                                          |
|         |      |      |      |      | stdp |      | sample degradation priority                                  |
|         |      |      |      |      | sdtp |      | independent and disposable samples                           |
|         |      |      |      |      | sbgp |      | sample-to-group                                              |
|         |      |      |      |      | sgpd |      | sample group description                                     |
|         |      |      |      |      | subs |      | sub-sample information                                       |
|         | mvex |      |      |      |      |      | movie extends box                                            |
|         |      | mehd |      |      |      |      | movie extends header box                                     |
|         |      | trex |      |      |      | √    | track extends defaults                                       |
|         | ipmc |      |      |      |      |      | IPMP Control Box                                             |
| moof    |      |      |      |      |      |      | movie fragment                                               |
|         | mfhd |      |      |      |      | √    | movie fragment header                                        |
|         | traf |      |      |      |      |      | track fragment                                               |
|         |      | tfhd |      |      |      | √    | track fragment header                                        |
|         |      | trun |      |      |      |      | track fragment run                                           |
|         |      | sdtp |      |      |      |      | independent and disposable samples                           |
|         |      | sbgp |      |      |      |      | sample-to-group                                              |
|         |      | subs |      |      |      |      | sub-sample information                                       |
| mfra    |      |      |      |      |      |      | movie fragment random access                                 |
|         | tfra |      |      |      |      |      | track fragment random access                                 |
|         | mfro |      |      |      |      | √    | movie fragment random access offset                          |
| mdat    |      |      |      |      |      |      | media data container                                         |
| free    |      |      |      |      |      |      | free space                                                   |
| skip    |      |      |      |      |      |      | free space                                                   |
|         | udta |      |      |      |      |      | user-data                                                    |
|         |      | cprt |      |      |      |      | copyright etc.                                               |
| meta    |      |      |      |      |      |      | metadata                                                     |
|         | hdlr |      |      |      |      | √    | handler, declares the metadata (handler) type                |
|         | dinf |      |      |      |      |      | data information box, container                              |
|         |      | dref |      |      |      |      | data reference box, declares source(s) of metadata items     |
|         | ipmc |      |      |      |      |      | IPMP Control Box                                             |
|         | iloc |      |      |      |      |      | item location                                                |
|         | ipro |      |      |      |      |      | item protection                                              |
|         |      | sinf |      |      |      |      | protection scheme information box                            |
|         |      |      | frma |      |      |      | original format box                                          |
|         |      |      | imif |      |      |      | IPMP Information box                                         |
|         |      |      | schm |      |      |      | scheme type box                                              |
|         |      |      | schi |      |      |      | scheme information box                                       |
|         | iinf |      |      |      |      |      | item information                                             |
|         | xml  |      |      |      |      |      | XML container                                                |
|         | bxml |      |      |      |      |      | binary XML container                                         |
|         | pitm |      |      |      |      |      | primary item reference                                       |
|         | fiin |      |      |      |      |      | file delivery item information                               |
|         |      | paen |      |      |      |      | partition entry                                              |
|         |      |      | fpar |      |      |      | file partition                                               |
|         |      |      | fecr |      |      |      | FEC reservoir                                                |
|         |      | segr |      |      |      |      | file delivery session group                                  |
|         |      | gitn |      |      |      |      | group id to name                                             |
|         |      | tsel |      |      |      |      | track selection                                              |
| meco    |      |      |      |      |      |      | additional metadata container                                |
|         | mere |      |      |      |      |      | metabox relation                                             |

