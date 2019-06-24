# 基本概念

**ES流（Elementary Stream）**：基本码流，经过编码后的连续码流。

**PES流（Pakckaged Elementary Stream）**：将ES流分割成段，加上相应的头文件打包后的码流。PES包的长度可变，包头中最重要的是PTS（Presentation Time Stamp）、和DTS（Decode Time Stamp）时间，再加上参考PCR参考时钟，播放器便能从PES流中重建音视频。

**TS流（Transport Stream）**：固定包长度为188B，将一个或多个PES包组合到一起用于传输的单一码流。

# TS分析工具

TSR：不仅可以查看16进制数据信息，且将所有关键表都自动解析出来，如PAT、PMT等。

# TS简介

TS的名称实际上是**MPEG2-TS**，在iso13818-1文档中定义，TS是Transport Stream的缩写，意为传输流，用于传输混合后的多媒体数据流。在MPEG2标准中，有两种不同类型个码流标准，一是节目码流（PS即 Program Stream），另外一个就是TS流。在MPEG2标准中，给出两种不同类型码流的形成过程。

![ps_ts_mux](.\ps_ts_mux.JPG)

1. Video或者Audio原始数据，通过音视频编码器编码后形成ES流。
2. 通过打包器（Packatizer）添加上PTS、DTS等信息后，打包成PES包。 
3. 分别打包好后的音频PES包和视频PES包经过PS/TS复用处理后，分别形成**PS流**和**TS流**。

## TS流包含的内容

我们之所以要解析TS流，目的就是从流中获取我们需要的编码数据、时钟等信息从而在接收设备中重建音视频。而所谓的TS流，实际上就是基于Packet形成的Byte位流，由一个个包形成。

要完成音视频的重建，首先需要获取TS流中的Packet，TS的Packet一般固定长度为188字节（也有204字节的版本，该版本在188字节后加上17字节的CRC校验数据）。其中包头4字节，负载184字节。下图表示一个TS流和包结构信息图。

![packet_structure](.\packet_structure.JPG)

一段PS流，必然包含PAT包、PTM包、多个音频...

## PS流和TS流的主要区别

TS流的包结构长度固定为188字节，而PS流的包长度可变。

TS和PS的包结构差异，使对误码的抵抗程度不同，因此适用于不同的环境。TS码流因为采用了固定长度的包结构，传输误码时，接收设备依然可以从固定的位置检测同步信息，恢复同步。

而PS流如果误码，接收设备将无法的值下一个包的同步位置，无法同步甚至无法解码。

因为MPEG2-TS流的任何一个片段都可以独立解码，因此在传输信道不稳定时，传输MPEG-2码流基本都采用TS流，如电视节目。而PS流因为长度可变，节约空间，通常用在没有误码场景的DVD播放上。

以下是TS流的语法定义：

```c++
MPEG_transport_stream() {
	do {
        transport_packet()
    } while (nextbits() == sync_byte)
}
```

# TS头解析

TS的包头中，包含关于传输相关信息。虽然TS包长度固定，但包头长度并不固定，除去前4个字节的包头信息外，可能还会跟有自适应空间，最后才是包数据。下面看一下在MPEG-2中的定义：

```c++
transport_packet(){
    sync_byte                    // 8 bit
    transport_error_indicator    // 1 bit
    payload_unit_start_indicator // 1 bit
    transport_priority           // 1 bit                 包头部分
    PID                          // 13 bit
    transport_scrambling_control // 2 bit
    adaptation_field_control     // 2 bit
    continuity_counter           // 4 bit
    ===========================================================================
    if(adaptation_field_control == '10' || adaptation_field_control == '11'){
    	adaptation_field()
    }
    if(adaptation_field_control == '01' || adaptation_field_control == '11') {
        for (i = 0; i < N; i++){
        	data_byte            // 8 bit
        }
    }
}
```

上述注释部分表示了对应字段做占据的大小。其中if条件语句之前32bit是TS Packet的包头部分，包头各字段含义如下：

* **同步字节（sync_byte）**：固定的8bit字段，其值为“0100 0111”（0x47）。 应避免在为其他常规字段（例如PID）选择值时使用sync_byte的固定， 该字段是MPEG-2 TS传送包标识符。

* **传输误差指示符（transport_error_indicator）**：长度为1bit， 设置为“1”时，表示相关传输流数据包中至少存在1个不可纠正的位错误。 该位可以由传输层外部的实体设置为“1”。 设置为“1”时，除非纠正了错误的位值，否则该位不应复位为“0”。

* **有效载荷单元起始符（payload_unit_start_indicator）**：长度为1bit，对于携带PES包或PSI数据的传输流包具有规范含义。

  **当TS包的有效载荷包含PES数据时**，payload_unit_start_indicator具有以下含义：“1”表示该TS包的有效载荷从PES的第一个字节开始，“0”则表示PES包的起始地址并非有效载荷的开始。 如果payload_unit_start_indicator被设置为'1'，则在该TS包中有且仅有一个PES包。

  **当TS包的有效载荷包含PSI数据时**，payload_unit_start_indicator具有以下含义：如果TS包携带PSI部分的第一个字节，则payload_unit_start_indicator值应为'1'，表示TS包有效载荷的第一个字节包含pointer_field。 如果TS包不携带PSI部分的第一个字节，则payload_unit_start_indicator值应为'0'，表示有效载荷中没有pointer_field。

  **对于空包**，payload_unit_start_indicator应设置为'0'。这意味着，该TS包只包含了MPEG2-TS规范未定义的用户自定义数据。

* **传输优先级（transport_priority）**：长度为1bit。 当设置为“1”时，它表示相关包（Packet）的优先级高于具有相同PID但没有将该位设置为“1”的其他包。 传输优先级机制可以再基本流中对数据进行优先级排序。

* **PID**：长度为13bit，指示存储在Packet有效载荷中的数据的类型。 PID值0x0000保留给程序关联表、0x0001保留用于条件访问表、0x0002  -  0x000F保留。 PID值0x1FFF保留用于空包。对应的PID表如下：

  | Value         | Description                                                  |
  | ------------- | ------------------------------------------------------------ |
  | 0x0000        | Program Association Table                                    |
  | 0x0001        | Conditional Access Table                                     |
  | 0x0002        | Transport Stream Description Table                           |
  | 0x0003~0x000F | Reserved                                                     |
  | 0x0010~0x1FFE | May be assigned as network_PID, Program_map_PID, elementary_PID, or for other purposes |
  | 0x1FFF        | Null Packet                                                  |
  |               | 注 - 允许PID值为0x0000,0x0001和0x0010-0x1FFE的传输包携带PCR  |

* **传输加扰控制（transport_scrambling_control）**：长度为2bit，表示TS包有效载荷是否是加扰模式。 在加扰模式下，TS包头和可能存在的适配字段不应被加扰。 在空包的情况下，transport_scrambling_control字段的值应设为'00'。加扰表如下：

  | Value | Description   |
  | ----- | ------------- |
  | 00    | Not scrambled |
  | 01    | User-defined  |
  | 10    | User-defined  |
  | 11    | User-defined  |

* **自适应字段控制（adaptation_field_control）**：长度为2bit，决定TS包头，后面是跟适配字段还是负载数据。

  | Value | Description                          |
  | ----- | ------------------------------------ |
  | 00    | Reserved for future use by ISO/IEC   |
  | 01    | No adaptation_field, payload only    |
  | 10    | Adaptation_field only, no payload    |
  | 11    | Adaptation_field followed by payload |

  当adaptation_field_control字段值为'00'是，解码器应该丢弃该Packet。 空包时，该字段的值应设为'01'。

* **连续计数器（continuity_counter）**：长度为4bit，连续计数器表示，有多少个TS包具有相同的PID。 当Packet的adaptation_field_control等于'00'或'10'时，continuity_counter不会递增。空包时，continuity_counter是未定义的。

* **数据字节（data_byte）**：载荷部分的有效字节数据，数据字节应是来自PES包、PSI部分的数据的连续字节，PSI部分之后的包填充字节，或者不在这些结构中的私有数据。 在具有PID值0x1FFF的空包的情况下，可以为data_bytes分配任何值。 data_bytes的数量N由184减去adaptation_field（）中的字节数来指定。

## TS包调整字段（adaptation_field）

在TS包中，打包后的PES数据，通常并不能满足188字节的长度，为了补足188字节，以及在系统层插入插入参考时钟（program clock reference， PCR），需要在TS包中插入长度可变的调整字段。

调整字段的作用之一是解决编解码器的音视频同步问题。一般在视频帧TS包的调整字段中，每隔一段时间，传送系统时钟27MHz的一个抽样值给接收机，作为解码器的时钟参考即PCR。PCR通常每隔100ms至少被传输一次。PCR的数值所表示的是解码器在读完这个抽样值的最后那个字节时，解码器本地时钟所应处的状态。通常情况下，PCR不直接改变解码器的本地时钟，而是作为参考基准来调整本地时钟，使之与PCR趋于一致。

# 节目关联表（Program Association Table，PAT）

下图通过TSR分析真实TS文件的截图：

![PAT](.\PAT.JPG)

从**16进制数据区**中，我们可以看到，该TS包原始16进制数据为：47 40 00 10 00 00 b0 0d 00 01 c1 00 00 00 01 ef ff 36 90 e2 3d ff ff ff ff ff ff ff ff ff ff ff  ........，先来分析包头。

我们都知道，Packet Header占每个Packet的头四个字节，对应**47 40 00 10**，换成二进制表示：**01000111 0100001 00000000 00010000**。使用包头格式解析如下表：

| 标识                         | 位数    | 说明                                                       |
| ---------------------------- | ------- | ---------------------------------------------------------- |
| sync_byte                    | 8 bits  | 固定是0x47                                                 |
| transport_error_indicator    | 1 bits  | 值为0，表示当前包没有发生传输错误。                        |
| payload_unit_start_indicator | 1 bits  | 值为1，负载数据从包头数据1字节后开始                       |
| transport_priority           | 1 bits  | 值为0，表示当前包是低优先级。传输优先级标志（1：优先级高） |
| PID                          | 13 bits | PID=0x0000,说明是PAT表。                                   |
| transport_scrambling_control | 2 bits  | 值为0x00，表示节目没有加密。                               |
| adaptation_field_control     | 2 bits  | 值为0x01,改Packet区域含控制调整字段                        |
| continuity_counter           | 4 bits  | 值为0x00,表示当前传送的相同类型的包是第0个。               |

接下来是PAT表数据部分：

00 00 b0 0d 00 01 c1 00 00 00 01 ef ff 36 90 e2 3d ff ff ff ff ff ff ff ff ff ff ff  ........

因为payload_unit_start_indicator值为1字节开始，所以实际上PAT包所含数据为：

**00 b0 0d 00 01 c1 00 00** 00 01 ef ff 36 90 e2 3d ff ff ff ff ff ff ff ff ff ff ff  ........

在这部分数据中，PAT表固定的前8个字节为头信息，其它数据组成固定长度的块，通过循环读取这些数据块，可以获得TS流中的节目和对应的PMT表的PID。

# 节目映射表（Program Map Table， PMT）

节目映射表，该表的PID是由PAT解析得出。通过该表可以得到一路节目中包含的信息，例如，该路节目由哪些流构成和这些流的类型（视频，音频，数据），指定节目中各流对应的PID，以及该节目的PCR所对应的PID。
PMT表中包含的数据如下：

* 当前节目中所有Video ES流的PID
* 当前节目所有Audio ES流的PID
* 当前节目PCR的PID等。

结构如下：

```c++
TS_program_map_section() {
    table_id \\ 8 uimsbf
    section_syntax_indicator \\1 bslbf
    '0' \\1 bslbf
    reserved \\2 bslbf
    section_length \\12 uimsbf
    program_number \\16 uimsbf
    reserved \\2 bslbf
    version_number \\5 uimsbf
    current_next_indicator \\1 bslbf
    section_number \\8 uimsbf
    last_section_number \\8 uimsbf
    reserved \\3 bslbf
    PCR_PID \\13 uimsbf
    reserved \\4 bslbf
    program_info_length \\12 uimsbf
    for (i = 0; i < N; i++) {
    	descriptor()
    }
    for (i = 0; i < N1; i++) {
        stream_type \\8 uimsbf
        reserved \\3 bslbf
        elementary_PID \\13 uimsbf
        reserved \\4 bslbf
        ES_info_length \\12 uimsbf
        for (i = 0; i < N2; i++) {
        	descriptor()
        }
    }
    CRC_32 \\32 rpchof
}
```

解析完PMT后，表可以得到所有包含了编码数据的音视频原始数据和同步信息，接受者通过这些同步信息便可以重建视频。

# 其它基本概念

**scrambling (system):** 加扰，以改变视频，音频或编码数据流的特性，防止以明文形式传输，使未经授权者接收明文数据。 

**PCR (system)**: Program Clock Reference

**PTS（system）**：presentation time-stamp 显示时间戳，可能存在于PES包头中的字段，用于指示在系统目标解码器中显示帧数据的时间。

**DTS（system）**：decoding time-stamp 解码时间戳，可能存在于PES包头中的字段，用于指示在系统目标解码器中解码访问帧数据的时间。

**CRC**：Cyclic Redundancy Check 循环冗余检查，以验证数据的正确性。

**PSI**：Program Specific Information 程序特定信息，提供了单个TS流的信息，使接收方能够对单个TS流中的不同节目进行解码，这些信息都存在用表的形式提供给，如PAT、PMT、CAT等。但它无法提供多个TS流的相关业务，也不能提供节目的类型、节目名称、开始时间、节目简介等信息。

**SI**：Specific Information 特定信息，PSI中无法提供的相关信息，SI定义了NIT、SDT、EIT和TDT等9张表，方便用户查看多种信息。



