继续错误的代价有别人承担，而承认错误的代价由自己承担——Joseph Stiglitz

# 任务

线程可以驱动任务，描述任务由Runnable接口提供。定义任务只需要实现Runnable接口，重写run()函数即可。需要注意的是，这个run()函数并不会产生任何内在线程的能力，要实现线程行为，必须显示地将任务附着到线程上。

Thread.yield()的调用时对线程调度器的一种建议。

# Executor

java.util.concurrent包中的执行器（Executor），用于管理Thread对象，简化并发编程。它允许你管理异步任务的执行，Executor在Java SE5/6是启动任务的优选方法。

## ExecutorService

具有服务生命周期的Executor，例如关闭。它知道如何构建恰当的上下文来执行Runnable对象。

```java
ExecutorService exec = Executors.newCachedThreadPool();
exec = Executors.newFixedThreadPool(5);
exec.shutdown(); // 表示防止新任务被提交给这个Executor；
```

* CachedThreadPool: 创建与所需数量相等的线程，然后在回收旧线程时停止创建新线程，是合理的Executor的首选。
* FixedTheadPool:只有上述发生问题时，才考虑选择FixedTheadPool，一次性预先执行代价高昂的线程分配，因而也就可以限制线程的数量。
* SingleThreadPool: 就像线程数为1的FixedTheadPool。如果提交了多个任务，那么这些任务将排队，每个任务都会在下一个任务开始之前结束运行，所有的任务将使用相同的线程。

## 从任务中产生返回值

Runnable接口时执行工作的独立任务，并不返回任何值。如果你希望任务在完成时能够返回一个值，那么可以实现Callable接口而不是Runnable接口。Callable是一种具有范型的接口，它的类型参数表示的是从方法call()中返回的值。

```java
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

public class Main {
    static class TaskWaitResult implements Callable<String> {
        @Override
        public String call() throws Exception {
            System.out.println("this is call invoke");
            return "method return";
        }
    }
  
    public static void main(String[] args) {
        ExecutorService exec = Executors.newCachedThreadPool();
        Future<String> future = exec.submit(new TaskWaitResult());
        try {
            System.out.println("main result = " + future.get());
        } catch (InterruptedException e) {
            e.printStackTrace();
        } catch (ExecutionException e) {
            e.printStackTrace();
        }
    }
}
```

submit()方法会产生Future对象，它用Callable返回结果的特定类型进行了参数化。可以用isDone()函数来查询Future是否已经完成。当任务完成时，它具有一个结果，可以调用get()（阻塞）函数获取结果。

# 休眠

sleep是影响任务行为的一种简单的方法，它会产生中断异常，该异常需要在run()中被捕获，不能跨线程传播回mian()函数。

# 优先级

可以通过setPriority()或者getPriority()函数来设置或者获取优先级，高优先级的线程仅仅是执行的频率高，调度器倾向于让优先级高的线程先执行。注意，优先级是在run()的开始部分设置，在构造器中设置它们不会有任何好处，因为Executor在此时还没呕开始执行任务。

尽管JDK有10个优先级，但它与多数操作系统都不能映射的很好。如，Windows有7个优先级且并不固定，所以这种映射关系也不是确定的。Sun的Solaris有$2^{31}$个优先级。唯一可移植的方法是当调整优先级时，只是用MAX_PRIORITY、NORM_PRIORITY、MIN_PRIORITY三种。

# 让步

yield()函数可以给线程调度机制一个暗示：你的工作已经做得差不多了，可以让别的线程使用CPU了。但这只是一个暗示，并没有任何机制可以保证它将被采纳。

当yield()被调用时，你也是在建议具有相同优先级的其他线程可以运行。

实际上，对于任何重要的控制，都不能依赖于yield()。

# 后台线程

所谓的后台线程（daemon），是指在程序运行的时候在后台提供一种通用服务的线程，当所有非后台线程结束时，程序也就终止了，同时会杀死进程中的所有线程。

必须在线程启动之前调用setDaemon()方法，才能把它设置为后台线程。

# 异常

由于线程的本质特性，使得你不能捕获从线程中逃逸的异常。一旦异常逃出任务的run()方法，他就会想外传播到控制台。

Thread.UncaughtExceptionHandler是Java SE5中的新接口，允许你在每个Thread对象上都附着一个异常处理器。Thread.UncaughtExceptionHandler.uncaughtException()会在线程因未捕获的异常而临近死亡时被调用。

```java
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;

public class Main {
    static class ExceptionThread implements Runnable {
        @Override
        public void run() {
            Thread t = Thread.currentThread();
            throw new RuntimeException();
        }
    }
  
    static class MyUncaughtExceptionHandler implements Thread.UncaughtExceptionHandler {
        @Override
        public void uncaughtException(Thread t, Throwable e) {
            System.out.println("caught " + e);
        }
    }

    static class HandlerThreadFactory implements ThreadFactory {
        @Override
        public Thread newThread(Runnable r) {
            Thread t = new Thread(r);
            t.setUncaughtExceptionHandler(new MyUncaughtExceptionHandler());
            return t;
        }
    }
  
    public static void main(String[] args) {
        ExecutorService exec = Executors.newCachedThreadPool(new HandlerThreadFactory());
        exec.execute(new ExceptionThread());
    }
}
```

# 共享资源和竞争

线程使用时的一个基本的问题是：你永远不知道一个线程何时在运行。想象一下，你坐在桌边手拿叉子，正要去叉盘中最后一块肉，当你的叉子就要够着它时，这便食物突然消失了，因为你的线程被挂起，另一个进餐者吃掉了它。