| 缩写       | 全称                                                         | 中文                 | 说明                                                         |
| ---------- | ------------------------------------------------------------ | -------------------- | ------------------------------------------------------------ |
| PSI        | Program Specific Information                                 | 节目引导信息         | 对单一码流的描述                                             |
| SI         | Service Information                                          | 业务信息             | 对系统中所有码流的描述                                       |
| TS         | Transport Stream                                             | 传输流（常称为TS流） | 一个频道（多个节目及业务）的TS包复用后称TS流                 |
| TS包       | Transport Packet                                             | 传输包               | 数字视音频、图文数据打包成TS包                               |
| PAT        | Program Association Table                                    | 节目关联表           | 将节目号码和节目映射表PID相关联，获取数据的开始              |
| PMT        | Program Map Table                                            | 节目映射表           | 指定一个或多个节目的PID                                      |
| CAT        | Conditional Access Table                                     | 条件接收表           | 将一个或多个专用EMM流分别与唯一的PID相关联                   |
| NIT        | Network Information Table                                    | 网络信息表           | 描述整个网络，如多少TS流、频点和调制方式等信息               |
| SDT        | Service Description Table                                    | 业务描述表           | 包含业务数据（如业务名称、起始时间、持续时间等）             |
| EIT        | Event Information Table                                      | 事件信息表           | 包含事件或节目相关数据，是生成EPG的主要表                    |
| BAT        | Bouquet Association Table                                    | 业务群关联表         | 给出业务群的名称及其业务列表等信息                           |
| TDT        | Time&Date Table                                              | 时间和日期表         | 给出当前事件和日期相关信息，更新频繁                         |
| TOT        | Time Offset Table                                            | 时间偏移表           | 给出了当前时间日期与本地时间偏移的信息                       |
| RST        | Running Status Table                                         | 运行状态表           | 给出事件的状态（运行/非运行）                                |
| ST         | Stuffing Table                                               | 填充表               | 用于使现有的段无效，如在一个传输系统的边界                   |
| SIT        | Selection Information Table                                  | 选择信息表           | 仅用于码流片段中，如记录的一段码流，包含描述该码流片段业务信息段的地方 |
| DIT        | Discontinuity Information   Table DVB                        | 间断信息表           | 仅用于码流片段，如记录的一段码流中，它将插入到码流片段业务信息间断的地方 |
| Descriptor |                                                              | 描述子，描述符       |                                                              |
| AC-3       | Dolby AC-3 audio coding   (ITU-R Recommendation BS.1196.1 [45]) | 杜比DD               | 可称为AC3、DD，升级版为杜比+或称为DD+、AC3+                  |
| BCD        | Binary Coded Decimal                                         | BCD码                | 一组二进制数1000 0010   1001，用BCD码显示为829(Hex)；即1000->8，0010->2，1001->9。这里的每四位二进制数分别用十六进制数来显示，但它并不是真正的十六进制数，且只显示0-9共10个数。 |
| bslbf      | bit string, left bit first                                   | 左对齐的位字符串     | 表和描述子结构的助记符(Identifier)                           |
| CA         | Conditional Access                                           | 条件接收             |                                                              |
| CLUT       | Colour Look-Up Table                                         |                      |                                                              |
| CRC        | Cyclic Redundancy Check                                      | 循环冗赘校验         | 一种数据传输检错功能，对数据进行多项式计算，并将得到的结果附在帧的后面，接收设备也执行类似的算法，以保证数据传输的正确性和完整性 |
| DAB        | Digital Audio Broadcasting                                   | 数字音频广播         | 数字音频广播是继调幅和调频广播之后的第三代广播，它全部采用最新的数字处理方式进行音频广播，有杜比降噪功能，具有失真小、噪音低、音域定位准的特点。如果用户配合功放、音箱等设备便可真正地带来高保真立体声享受。 |
| DSNG       | Digital Satellite News Gathering                             | 数字卫星新闻采集     | DSNG数字卫星新闻采集的缩写，是指把采集到的信号，进行压缩处理成数字信号再进行传送的卫星新闻。 |
| DVB        | Digital Video Broadcasting                                   | 数字视频广播         | 由DVB项目维护的一系列国际承认的数字电视公开标准。DVB项目是一个由300多个成员组成的工业组织，它是由欧洲电信标准化组织European Telecommunications Standards Institute   (ETSI),欧洲电子标准化组织European Committee for Electrotechnical Standardization   (CENELEC)和欧洲广播联盟European Broadcasting Union (EBU)联合组成的联合专家组Joint Technical Committee (JTC)发起的。 |
| DVD        | Digital Versatile Disc                                       | 数字多功能光盘       | 是一种光盘存储器，通常用来播放标准电视机清晰度的电影，高质量的音乐与作大容量存储数据用途。 |
| ETS        | European Telecommunication   Standard                        | 欧洲电信标准         |                                                              |
| ETSI       | European Telecommunication   Standard Institute              | 欧洲电信标准委员会   |                                                              |
| EBU        | European Broadcasting Union                                  | 欧洲广播联盟         | 以西欧和地中海沿岸国家广播电视机构为主的国际性广播电视组织。简称欧广联。英文简称EBU。 |
| EMM        | Entitlement Management   Message                             | 授权管理信息         |                                                              |
| EPG        | Electronic Programme Guide                                   | 电子节目指南         |                                                              |
| ES         | Elementary Stream                                            | 基本码流             | 基本码流，不分段的音频、视频或其他信息的连续码流。           |
| FEC        | Forward Error Correction                                     | 前向错误更正         |                                                              |
| HD         | High Definition (Video)                                      | 高清晰度             | 俗称“高清”                                                   |
| HP         | High Priority                                                | 高优先级             |                                                              |
| IEC        | International   Electrotechnical Commission                  | 国际电工委员会       |                                                              |
| IRD        | Integrated Receiver Decoder                                  | 综合接收解码器       | 机顶盒是其中一种。在Irdeto(爱迪德)等CA认证的文档中对接收机的描述就是IRD |
| ISO        | International Organization   for Standardization             | 国际标准化组织       |                                                              |
| LP         | Low Priority                                                 | 低优先级             |                                                              |
| LSB        | Least Significant Bit                                        | 最低有效位           |                                                              |
| MJD        | Modified Julian Date                                         | 儒略日期             | 由公元前4713年1月1日，协调世界时中午12时开始所经过的天数，多为天文学家采用，用以作为天文学的单一历法，把不同历法的年表统一起来。 |
| MPEG       | Moving Pictures Expert Group                                 | 运动图像专家组       |                                                              |
| MSB        | Most Significant Bit                                         | 最高有效位           |                                                              |
| NBC-BS     | Non Backwards Compatible   Broadcast Services (DVB-S2)       |                      |                                                              |
| NDA        | Non Disclosure Agreement                                     | 非公开协议           |                                                              |
| NVOD       | Near Video On Demand                                         | 准视频点播           | 就是根据观众的要求播放节目的视频点播系统，把用户所点击或选择的视频内容，传输给所请求的用户。 |
| OFDM       | Orthogonal Frequency Division Multiplex                      | 正交频分复用         | 将信道分成若干正交子信道，将高速数据信号转换成并行的低速子数据流，调制到在每个子信道上进行传输。 |
| PDC        | Programme Delivery Control                                   |                      |                                                              |
| PID        | Packet IDentifier                                            | 包标识符             | PID是当前TS流的Packet区别于其他Packet类型的唯一识别符，通过读取每个包的Packet Header，我们可以知道这个Packet的数据属于何种类型。 |
| PIL        | Programme Identification   Label                             |                      |                                                              |
| PSTN       | Public Switched Telephone   Network                          | 公用交换电话网       |                                                              |
| QAM        | Quadrature Amplitude Modulation                              | 正交振幅调制         | QAM是用两路独立的基带信号对两个相互正交的同频载波进行抑制载波双边带调幅，利用这种已调信号的频谱在同一带宽内的正交性，实现两路并行的数字信息的传输。该调制方式通常有二进制QAM（4QAM）、四进制QAM（l6QAM）、八进制QAM（64QAM）… |
| QPSK       | Quaternary Phase Shift Keying                                | 正交相移键控         | 在数字信号的调制方式中QPSK是最常用的一种卫星数字信号调制方式，它具有较高的频谱利用率、较强的抗干扰性，在电路上实现也较为简单。 |
| rpchof     | remainder polynomial   coefficients, highest order first     |                      |                                                              |
| RDS        | Radio Data System                                            | 无线电数据系统       |                                                              |
| RS         | Reed-Solomon                                                 | 里德－所罗门码       | BCH码的子集                                                  |
| ScF        | Scale Factor                                                 | 比例因子             |                                                              |
| SD         | Standard Definition (Video)                                  | 标准清晰度           | 俗称“标清”                                                   |
| SMI        | Storage Media   Interoperability                             | 存储媒体互操作性     |                                                              |
| TPS        | Transmission Parameter   Signalling                          | 传输参数信令         |                                                              |
| TSDT       | Transport Stream Description   Table                         | 传送流描述表         |                                                              |
| UECP       | Universal Encoder   Communication Protocol (RDS)             | 通用编码器通信协议   |                                                              |
| uimsbf     | unsigned integer most   significant bit first                | 无符号整数，高位在先 |                                                              |
| UTC        | Universal Time, Co-ordinated                                 |                      | 为了方便， 通常记成Universal Time   Coordinated。同样为了方便，在不需要精确到秒的情况下，通常也将GMT 和UTC 视作等同。尽管UTC 更加科学更加精确 |
| GMT        | Greenwich Mean Time                                          | 格林尼治标准时       | 指位于伦敦郊区的皇家格林尼治天文台的标准时间，因为本初子午线被定义在通过那里的经线。 |
| VBI        | Vertical Blanking Interval                                   | 场消隐期             | VBI原指电视广播中的场消隐期，它是Vertical Blanking Interval（场消隐期）的英文缩写，现在特指利用场逆程叠加传递图文数据信息的一种数据广播。 |
| VPS        | Video Programme System                                       | 视频节目系统         |                                                              |
| WSS        | Wide Screen Signalling                                       | 宽屏幕信号           |                                                              |
| -          | Pan & Scan                                                   | 平移与扫描           | 简单来说，就是以固定宽高比裁剪原画面，以填充满整个屏幕       |
| -          | LetterBox                                                    | 宽银幕式的           | 简单来说，就是以固定宽高比填充原画面，在保证原画面完整的前提下充满整个屏幕 |