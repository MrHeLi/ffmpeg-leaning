# Binder

```c++
int main(int argc, char **argv)
{
    struct binder_state *bs;
    void *svcmgr = BINDER_SERVICE_MANAGER;
 
    bs = binder_open(128*1024);//通过binder 驱动程序打开设备文件
 
    if (binder_become_context_manager(bs)) {//申明该server是binder 上下文的管理者
        LOGE("cannot become context manager (%s)\n", strerror(errno));
        return -1;
    }
 
    svcmgr_handle = svcmgr;
    binder_loop(bs, svcmgr_handler);//循环，充当server的角色   等待client 端请求
    return 0;
}

struct binder_state
{
    int fd;//  dev/binder 设备文件的标识符
    void *mapped; //设备地址空间映射到进程空间的首地址
    unsigned mapsize;//地址空间的大小
};


```

