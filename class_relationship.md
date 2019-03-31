# Java类与类之间的关系

>  已经远行的人，再不可能回来了。

本文主要讲述Java中，类与类之间的关系。

这些关系有实现、依赖、继承或泛化、关联。其中前三个概念较好理解，难的是关联。

因为关联关系中，又分为一般关联、聚合、组合。它们在代码中，都是通过成员属性的方式呈现，只是在逻辑上有所差别。所以，必须在了解整个系统的基础上，才能准确的区分和理解。

## 实现（Realization） 

- 发生在接口和类之间
- Uml示例：一个指向超类的空心虚线空心三角箭头。如Teacher实现People可以这样表示：

![](https://github.com/MrHeLi/ffmpeg-leaning/blob/master/image/class_realization.png)

## 泛化或继承（Generalization）

- 发生在类与类之间，也称为*is a like of*关系，父类也称基类或超类，子类也称作派生类。
- Java中，使用*extend*关键字实现，在C++中，使用*：*来实现。
- Java代码示例：

```java
class People {
	String name;
}
class Teacher extends People {
}
```

- Uml示例：实线空心三角箭头

  ![class_generalization](https://github.com/MrHeLi/ffmpeg-leaning/blob/master/image/class_generalization.png)

## 依赖 （dependency）

- 依赖表示一个类中要完成某种任务，需要用到另外一个类，是一种使用关系。
- 在java中，依赖关系表现为局部变量、参数、返回类型或者静态函数的调用。如果A类依赖B类，那么A类的成员函数参数、返回值等相关位置有B类参与。

```java
class A {
  public void getContent(B b) { // 类B作为类A的函数参数，被A类依赖
    System.out.println("B: " + b.content);
  }
}
class B {
  private String content = "B_content";
}
```

- 关系线形状：虚线箭头

![dependency](https://github.com/MrHeLi/ffmpeg-leaning/blob/master/image/dependency.png)

## 关联

关联分为一般关联、聚合、组合，耦合程度由低到高。

### 一般关联（directAssociation）:

- 类与类之间的链接，使一个类知道另一个类的成员。关联关系可以试单向的，也可以是双向的。
- 在Java中，关联关系一般通过成员变量来实现。

```java
class A {
  private B b = new B(); // 使用成员变量实现关联
  public void getContent() {
    System.out.println("B: " + b.content);
  }
}
class B {
  private String content = "B_content";
}
```

- 关系线形状：单向关联使用实线箭头，双向关联使用实线链接即可。下图为单向关联：

![Directed Association](https://github.com/MrHeLi/ffmpeg-leaning/blob/master/image/Directed%20Association.png)

### 聚合

- 是一种更强的关联关系，聚合关系表达的是一种整体和个体的关系。就像汽车和轮胎、发动机之间一样。通常，在定义一个整体类后，再去定义这个整体类的组成。
- 在Java中，聚合关系也是通过成员变量实现。区别与关联关系：关联表达的是在逻辑同一层次的相互关系，而聚合表示的是逻辑上两个层级之间的关系，一个是整体，一个是组成整体的零件。

```java
class Car {
  private Tyre tyre;
  private Engine engine;
}
class Tyre {
}
class Engine {
}
```

本例中，Tyre 和 Engine分别都是Car的一部分，Car在逻辑上代表的是一个整体，Tyre和Engine都是Car这个整体的组成部分。所以可以说，Tyre和Engine聚合于Car。

- 关系线形状：空心菱形实线，由整体类指向个体类。

![aggregation](https://github.com/MrHeLi/ffmpeg-leaning/blob/master/image/aggregation.png)

### 组合

- 最强关系的关联，同样表示的是局部和整体之间的关系，却别于聚合关系的是，组合强调生命周期强关联，要求整体对象管理局部对象的生命周期，整体对象的生命周期结束，局部对象也会消失。在聚合关系中，代表局部的对象可以被不同的整体对象所共享，这在组合关系中是不存在的。如公司和公司里的一个部门。
- 在代码中，也是通过成员变量来实现组合关系。

```java
class Human {
  private Head head;
  private Heart heart;
}
class Head {
}
class Heart {
}
```

Human作为一个整体，由Head和Heart组成，一旦Human的生命周期结束，Head和Heart对象也将不复存在。

- 关系线形状：实心棱形实线，由整体指向个体类。

![composition](https://github.com/MrHeLi/ffmpeg-leaning/blob/master/image/composition.png)

