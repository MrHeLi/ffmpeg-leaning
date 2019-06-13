[TOC]

# 数组

占据了内存中一段连续的内存空间，并顺序存储，因此可以通过数组下标直接访问，时间复杂度为$O_{(1)}$，时间效率高。在申明一个数组时，即使不往数组内存数据，也需要给定数组的空间大小。数组的这种存储方式，造成了内存空间的浪费，经常会有空间没有得到有效利用。

为了解决数组空间利用率的问题，在各大高级语言中，设计了动态数组。

所谓的动态数组指的是，在初始状态下会有一个默认长度的数组，往动态数组中添加数据时，如果超过了默认数组的容量，动态数组会重新申请一个数组（该数组的大小通常是当前数组容量的两倍），然后将已有数据拷贝（频繁的拷贝会影响效率）到新数组，最后释放旧数组。以此达到提高空间利用率的目的。

java中底层是动态数组机制的有：ArrayList，Vector和Stack。

## 面试题 3：二维数组中的查找

### 题目

在一个二维数组中，每一行都按照从左到右递增的顺序排序，每一列都按照从上到下递增的顺序排序。请完成一个函数，输入这样一个二维数组和一个整数，判断数组中是否含有该整数。

下面数组就是每行、每列递增排序，如果查找数字$7$，能找到，函数返回`true`，如果查找数字$5$，因为数组中没有该数字，无法找到，所以函数返回`false`。
$$
\begin{matrix}
1& 2& 8& 9 \\
2& 4& 9& 12 \\
4& 7& 10& 13 \\
6& 8& 11& 15
\end{matrix}
$$

### 分析

不知道为啥，刚看完题目，最直接的反应是直接双循环遍历不就好了。甚至觉得这个题出得没有必要。直到看完案例分析才意识到图样图森破。

该题考察的是算法逻辑，如果无脑遍历的话，确实可以实现功能，但效率会变得很低。最糟糕的情况，会将数组矩阵中所有元素都遍历比较一次才能得到结果。

提高效率的解法是从**行和列都是递增着手**，首先明确一点就是，**每行的最后一个元素，是当前行的最大值，是当前列的最小值**。

那么核心逻辑便是：首先取第一行最后一列元素（后面称为数组元素），与目标值(以后称为target_number)作对比。

如果数组元素大于target_number，因为数组元素是该列第一个元素，且矩阵行列递增，数组元素为该列最小值，说明数组元素所在列不可能出现target_number，删掉当前列（在后续的查找过程中，不考虑该列）。

如果数组元素小于target_number，因为数组元素是该列第一个元素，且矩阵行列递增，数组元素为该行最大值，说明数组元素所在行不可能出现target_number，删掉当前行（在后续的查找过程中，不考虑该行）。

比如在示例矩阵中，首先比较右上角的数字$9$，目标数字为$4$，那么应该删除当前列，我们需要考虑的矩阵将变成这样：
$$
\begin{matrix}
1& 2& 8 \\
2& 4& 9 \\
4& 7& 10 \\
6& 8& 11
\end{matrix}
$$
接着用目标数字$4$和右上角数组元素$8$比较，数组元素还是大于目标数字，应该删掉当前列，矩阵变为：
$$
\begin{matrix}
1& 2 \\
2& 4 \\
4& 7 \\
6& 8
\end{matrix}
$$
继续用目标数字$4$和右上角数组元素$2$比较，数组元素还是小于目标数字，应该删除当前行，矩阵变为：
$$
\begin{matrix}
2& 4 \\
4& 7 \\
6& 8
\end{matrix}
$$
最后，右上角数组元素是$4$，和目标数字相等，返回true。

### 解：C++

```c++
#include <iostream>

using namespace std;

const int maxRow = 4;
const int maxColumn = 4;

bool searchValue(int number, int array[maxRow][maxColumn]) {
    int row = 0;
    int column = maxColumn - 1;
    while (row < maxRow && column >= 0) {
        if (array[row][column] > number) {
            column--;
        } else if (array[row][column] < number) {
            row++;
        } else {
            return true;
        }
    }
    return false;
}

int main() {
    int array[maxRow][maxColumn] = {{1, 2, 8,  9},
                                    {2, 4, 9,  12},
                                    {4, 7, 10, 13},
                                    {6, 8, 11, 15}};
    bool result = searchValue(14, array);
    cout << "result = " << (result ? "true" : "false") << endl;
    return 0;
}
```

### 解：java

```java
public class InterviewQuestion3 {
    public static void main(String[] args) {
        int[][] array = {
            {1, 2, 8, 9},
            {2, 4, 9, 12},
            {4, 7, 10, 13},
            {6, 8, 11, 15}};
        boolean result = searchValue(7, array);
        System.out.println("result = " + result);
    }

    private static boolean searchValue(int number, int[][] array) {
        int maxRow = array.length;
        int maxColumn = array[0].length;
        int row = 0;
        int column = maxColumn - 1;
        while (row < maxRow && column > 0) {
            if (array[row][column] > number) {
                column--;
            } else if (array[row][column] < number) {
                row++;
            } else {
                return true;
            }
        }
        return false;
    }
}
```

 # 字符串

字符串是若干字符组成的序列，因为使用频率较高在各语言中都做了特殊处理。

C\C++中的字符串，是以'\0'字符结尾的数组。Java中的字符串指的是一个包含了字符数组的类：`String`。

在Java中，除了`String`外和字符串相关的类还有`StringBuffer`和`StringBuilder`。搞清楚`String`、`StringBuffer`和`StringBuilder`的应用和区别，对于Java基础的掌握非常重要，面试中往往也会出现相关问题。

## String的重要特性

1. String对象的主要作用是对字符串创建、查找、比较等基础操作。

2. **`String`的底层数据结构是固定长度的字符数组**（`char []`），该类的基础操作是和数组下标`index`相关的查找操作，如`indexOf`系列函数。该类的特点是不可变，任何拼接、截取、添加、替换等修改操作都将导致新`String`对象的诞生，**频繁的对象创建，将降低效率**。

3. 在代码中，类似于`"hello"`的字符串，都代表了一个String对象，它通常也被成为字面量，区别于`new String()`保存在堆内存，它被保存在虚拟机的常量池中。

   如果你这样申明一个变量：`String str = "hello";`，程序会首先在常量池中查看是否已有`"hello"`对象，如果有则直接将该对象的引用赋值给`str`变量，这种情况下，不会有对象被创建。当在常量池中找不到`"hello"`对象时，会创建一个`"hello"`对象，保存在常量池中，然后再将引用赋值给`str`变量。这一点比较重要，以前经常在面试中被考察。

## StringBuilder的重要特性

1. 和`String`对象关注字符串的基础操作不同，`StringBuffer`和`StringBuilder`一样，主要关注字符串拼接。

   为什么String已经可以实现拼接操作，还专门设计`StringBuilder`来做拼接的工作呢？还记得String对象的修改操作都会导致新对象的创建吧，在程序中，我们常常将字符串拼接的工作放在循环中执行，这将对系统资源造成极大的负担，而StringBuilder就是来解决这种问题的。

2. `StringBuilder`继承了`AbstractStringBuilder`，**`AbstractStringBuilder`的底层数据结构也是一个数组**，通过一定的扩容机制，更改数组容量（创建新的更大的数组）。

## StringBuffer和StringBuilder的区别

之所以没有单独说明`StringBuffer`，是因为它们太像了，不仅API类似（也意味着功能类似），也同样继承`AbstractStringBuilder`，该类实现了主要功能，同样是维护了一个动态数组。

区别在于：

* `StringBuffer`是线程安全，效率较低
* `StringBuilder`是线程不安全的，效率较高

线程安全与否，全在于一个`synchronized`关键字的使用。在`StringBuffer`的函数申明中，都有`synchronized`关键字，而`StringBuilder`则没有。在代码中看看他们的区别：

`StringBuffer` 的 append函数：

```java
@Override
public synchronized StringBuffer append(Object obj) {
    toStringCache = null;
    super.append(String.valueOf(obj));
    return this;
}
```

`StringBuilder`的 append函数：

```java
@Override
public StringBuilder append(Object obj) {
    return append(String.valueOf(obj));
}
```

## StringBuilder&StringBuffer的扩容逻辑

`StringBuilder`和`StringBuffer`都继承`AbstractStringBuilder`，扩容逻辑也都一样，在`AbstractStringBuilder`类中定义。

先看扩容代码：

```java
char[] value; // 保存字符的数组。
int count; // 当前字符数组中，已经保存的字符数。
private static final int MAX_ARRAY_SIZE = Integer.MAX_VALUE - 8;
public AbstractStringBuilder append(String str) {
    if (str == null)
    return appendNull();
    int len = str.length();
    ensureCapacityInternal(count + len); // 确保value数组长度足够，扩容逻辑函数。
    str.getChars(0, len, value, count);
    count += len;
    return this;
}
private void ensureCapacityInternal(int minimumCapacity) {
    // 参数minimumCapacity表示想要在旧数据中追加新数据所需要的最小数组容量。
    // overflow-conscious code
    if (minimumCapacity - value.length > 0) { // 如果当前数组容量小于最小容量，进入if代码块
        value = Arrays.copyOf(value, newCapacity(minimumCapacity)); // 创建新数组，
    }
}
private int newCapacity(int minCapacity) {
    // overflow-conscious code
    int newCapacity = (value.length << 1) + 2; // 新数组的容量，初始为原数组的2倍+2。
    if (newCapacity - minCapacity < 0) { // 如果新数组容量小于最小容量，则新数组容量改为最小容量
        newCapacity = minCapacity;
    }
    return (newCapacity <= 0 || MAX_ARRAY_SIZE - newCapacity < 0)
        ? hugeCapacity(minCapacity)
        : newCapacity;
}
private int hugeCapacity(int minCapacity) {
    if (Integer.MAX_VALUE - minCapacity < 0) { // overflow
        throw new OutOfMemoryError();
    }
    return (minCapacity > MAX_ARRAY_SIZE) ? minCapacity : MAX_ARRAY_SIZE;
}
```

整个逻辑还比较简单，梳理一下就是：

1. `AbstractStringBuilder`每次`append`字符串时，都会调用`ensureCapacityInternal`确保容量足够同时存放新老字符串。
2. 老的字符串的长度和新字符串长度之和被视为**最小容量**，如果最小容量大于当前数组数量，则需要扩容。
3. 扩容的新数组的容量为原数组容量的2倍+2（**新容量**），新容量如果比最小容量小，那么将新容量的值替换成最小容量。通常这就是新数组的容量了。
4. 例外情况是，新容量如果比整数的最大值都大，则会抛出内存溢出异常。如果是介于`MAX_ARRAY_SIZE`和`Integer.MAX_VALUE`之间，那么新容量的值变为最小容量。

## String、StringBuilder、StringBuffer之间的区别

* **在数据结构上**：String使用的是数组，而StringBuilder和StringBuffer是动态数组。
* **在使用上**：String主要用于字符串的查询操作，而StringBuilder和StringBuffer用于字符串的增、删、改。
* **效率上**：因为String的增、删、改操作会创建对象，而StringBuilder和StringBuffer不一定会，所以前者较后两者低。另外StringBuilder和StringBuffer相比，StringBuffer使用了同步，效率较StringBuilder低。

区别的主要原因前面已经说清楚了，这里就不重复了。来看看String相关的面试题。

## 面试题 4：替换空格

### 题目

请实现一个函数，把字符串中的每一个空格替换成“%20”。例如输入“We are happy.”，则输出“We%20are%20happy.”。

背景：在网络编程中，如果URL参数中包含特殊字符，如空格，#等，可能导致服务器无法或者正确的参数。我们需要将这些特殊字符转换为服务器可以识别的字符。转化规则是在‘%’后面跟上ASCII码的两位16进制表示。比如空格的ASCII码是32，即十六进制的0x20。因此空格需要被替换为`%20`。

### 分析

使用Java为开发语言的小伙伴肯定觉得这个题如此简单，调用String类的replace函数就可以实现。是的，这么说没错。但实际上这个题是给C/C++语言设计的面试题。就Java而言，该题中字符串应该改为字符数组。

因为需要将一个空格替换成`%20`，替换一次空间会增加2。所需，需要考虑原始字符数组空间是否足够。如果空间不够，则需要重新创建字符数组，将原数组字符拷贝一次，并不存在优化空间。这里就不考虑重新创建字符数组的情况。

那么我们假设原数组空间足够，会有两种实现方式：

1. 从前到后遍历数组，遇到空格就存入`%20`,同时将后面的所有函数向后位移2。假设字符串长度为n，对于每个空格字符，需要移动后面$O_{n}$个字符，因此对于含有$O_{n}$个空格字符的字符串而言总的时间效率为$O_{n^2}$。这可不是能够拿到Offer的。

2. 这是一个时间复杂度为$O_{n}$的解法，它的思路是：

   因为每替换一个空格，实际长度会增长2，所以，最终数组的实际长度为

   $实际长度 = 原数组实际长度 + 空格数*2$

   所以，我们需要先遍历数组，以获得$原数组实际长度$值，以及数组中的$空格数$，计算出替换后的实际长度。

   然后从后遍历数组，提供两个指针，`p1`、`p2`，`p1`指向原字符串的末尾，`p2`指向替换后字符串的末尾。接下来逐步向前移动`p1`指针，将它指向的字符赋值到`p2`指向的位置，同时`p2`向前移动1格。碰到空格后，`p1`向前移动一格，并在`p2`前插入字符串`%20`，`%20`向前移动3格。最终`p1`和`p2`将指向同一个位置，替换完毕。

   从分析来看，所有的需要移动的字符都只会移动一次，因此这个算法的时间复杂度是$O_{n}$，比上一个快。后面将以这个思路实现算法。

### 解：java

```java
public class Main {
    public static void main(String args[]) {
        // 用0占位，使数组有充足的长度。
        char[] str = {'w', 'e', ' ', 'a', 'r', 'e', ' ', 'h', 'a', 'p', 'p', 'y',
            0, 0, 0, 0};
        int rawLength = 0; // 原字符串长度
        int emptyBlanks = 0; // 空格数量
        for (char cha : str) { // 遍历，获得原字符串长度和字符串中空格的数量
            if (cha == 0) {
                break;
            }
            rawLength++;
            if (cha == ' ') {
                emptyBlanks++;
            }
        }
        System.out.println(new String(str));
        int p1 = rawLength - 1; // 减1是为了让指针从0开始计算
        // 替换后的数组长度为原字符串长度加上空格数量的两倍，减1是为了让指针从0开始计算
        int p2 = rawLength + 2 * emptyBlanks - 1;
        while (p1 != p2 && p1 >= 0 && p2 >= 0) {
            if (str[p1] != ' ') {
                str[p2] = str[p1];
                p1--;
                p2--;
            } else {
                p1--;
                str[p2--] = '0';
                str[p2--] = '2';
                str[p2--] = '%';
            }
        }
        System.out.println(new String(str));
    }
}
```

运行结果：

```shell
we are happy
we%20are%20happy
```

解：C++

C++的代码和Java代码几乎一致：

```c++
int main() {
    char str[16] = {'w', 'e', ' ', 'a', 'r', 'e', ' ', 'h', 'a', 'p', 'p', 'y'};
    int rawLength = 0; // 原字符串长度
    int emptyBlanks = 0; // 空格数量
    for (char cha : str) { // 遍历，获得原字符串长度和字符串中空格的数量
        if (cha == 0) {
            break;
        }
        rawLength++;
        if (cha == ' ') {
            emptyBlanks++;
        }
    }
    cout << str << endl;
    int p1 = rawLength - 1; // 减1是为了让指针从0开始计算
    // 替换后的数组长度为原字符串长度加上空格数量的两倍，减1是为了让指针从0开始计算
    int p2 = rawLength + 2 * emptyBlanks - 1;
    while (p1 != p2 && p1 >= 0 && p2 >= 0) {
        if (str[p1] != ' ') {
            str[p2] = str[p1];
            p1--;
            p2--;
        } else {
            p1--;
            str[p2--] = '0';
            str[p2--] = '2';
            str[p2--] = '%';
        }
    }
    cout << str << endl;
    return 0;
}
```

运行结果：

```shell
we are happy
we%20are%20happy
```




