



# Git 使用过程中出现的BUG

## 卡住时的日志打印

```shell
Superli-2:ffmpeg-leaning heli$ git push origin master
Username for 'https://github.com': MrHeLi
Password for 'https://MrHeLi@github.com': 
Counting objects: 11, done.
Delta compression using up to 8 threads.
Compressing objects: 100% (11/11), done.
Writing objects: 100% (11/11), 25.92 MiB | 26.84 MiB/s, done.
Total 11 (delta 1), reused 0 (delta 0)
```



出现这样的日志，并且卡住不动很久，可能有三种原因：

1、网络问题：提交代码到github上，毕竟是外网，所以有所怀疑。不过从GitHub clone 项目速度很快，所以这点排除了。

2、git缓存空间不够：这是网络上大量提供的思路，解决方式如下：

```shell
## 设置http缓存为1000M（大小可以根据需要自行更改）
git config --global http.postBuffer 1048576000 
## 设置https缓存为1000M
git config --global https.postBuffer 1048576000
```

如果你提交的文件确实比较大，并且修改缓存后，还是卡在上述界面，建议你等等，大文件传输毕竟比较耗时间。

3、SSL认证问题：

上述日志打印后，等了半天出现了后续打印，完整log如下：

```shell
Superli-2:ffmpeg-leaning heli$ git push origin master
Username for 'https://github.com': MrHeLi
Password for 'https://MrHeLi@github.com': 
Counting objects: 11, done.
Delta compression using up to 8 threads.
Compressing objects: 100% (11/11), done.
Writing objects: 100% (11/11), 25.92 MiB | 26.84 MiB/s, done.
Total 11 (delta 1), reused 0 (delta 0)
error: RPC failed; curl 56 OpenSSL SSL_read: SSL_ERROR_SYSCALL, errno 60
fatal: The remote end hung up unexpectedly
fatal: The remote end hung up unexpectedly
Everything up-to-date
```

原因是，本地SSL没有经过第三方机构认证，所以报错，解决方式如下：

1、克隆仓库时使用evn命令忽略本次SSL检查

```shell
env GIT_SSL_NO_VERIFY=true git clone https://.......
```



2、克隆完毕后，进入仓库，设置该仓库忽略SSL证书检查：

```shell
git config http.sslVerify "false"
```



只有当以上顺序得到保证的时候，才能生效，有尝试过不执行第一步，在提交代码到git服务器时执行`env GIT_SSL_NO_VERIFY = true git push origin master`，但是无效。

参考：https://blog.csdn.net/m0_37052320/article/details/77799413