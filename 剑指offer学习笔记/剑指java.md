@[toc]
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






