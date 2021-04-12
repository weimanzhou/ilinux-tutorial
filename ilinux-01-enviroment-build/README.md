# 01环境搭建

[toc]

工欲善其事，必先利其器。我们开始来配置工具链吧。

## 一、实验目的

为了后续的开发，需要安装一些必要的软件

## 二、实验内容与要求

安装软件，列表如下：

- nasm汇编编译器
- qemu虚拟机
- bochs用于调试代码
- gcc工具链，用于编译c代码
- MobaXterm用于ssh连接

## 三、实验步骤

### Linux平台

#### 安装nasm汇编编译器

1. 简介

   This is the project webpage for the Netwide Assembler (NASM), an asssembler for the x86 CPU architecture portable to nearly every modern platform, and with code generation for many platforms old and new.

2. 下载

   ```bash
   $ wget https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/linux/nasm-2.15.05-0.fc31.x86_64.rpm
   ```

3. 安装

   ```bash
   $ # 如果没有 rpm 可以先安装 rpm，或者依照自己所使用的平台的包管理器进行安装
   $ rpm --install nasm-2.15.05-0.fc31.x86_64.rpm
   ```

4. 测试

   ```bash
   $ whereis nasm
   $ # 出现路径即可
   nasm: /usr/bin/nasm /usr/share/man/man1/nasm.1.gz
   ```

#### 安装qemu

1. 简介

   QEMU is a generic and open source machine emulator and virtualizer.

2. 安装（Ubuntu18.04）：其它平台参考： https://www.qemu.org/download/

   ```bash
   $ apt-get install qemu
   ```

3. 测试

   ```bash
   $ whereis qemu-system-x86_64
   $ # 出现路径即可
   qemu-system-x86_64: /usr/bin/qemu-system-x86_64 /usr/share/man/man1/qemu-system-x86_64.1.gz
   ```

#### 安装bochs

1. 简介

   Bochs is a highly portable open source IA-32 (x86) PC emulator written in C++, that runs on most popular platforms. It includes emulation of the Intel x86 CPU, common I/O devices, and a custom BIOS. Bochs can be compiled to emulate many different x86 CPUs, from early 386 to the most recent x86-64 Intel and AMD processors which may even not reached the market yet.

2. 下载: https://sourceforge.net/projects/bochs/files/bochs/2.6.11/

3. 安装

   ```bash
   $ wget https://sourceforge.net/projects/bochs/files/bochs/2.6.11/bochs-2.6.11.tar.gz/download -O bochs-2.6.11.tar.gz
   
   $ wget https://sourceforge.net/projects/bochs/files/bochs/2.6.11/bochs-2.6.11-1.x86_64.rpm/download -O bochs-2.6.11.tar.gz
   ```

4. 测试

   ```bash
   $ whereis bochs
   $ # 出现路径即可
   bochs: /usr/bin/bochs /usr/lib/bochs /usr/share/bochs /usr/share/man/man1/bochs.1.gz
   ```

#### 安装gcc工具链

1. 下载

   ```bash
   $ wget http://mirrors.concertpass.com/gcc/releases/gcc-7.5.0/
   ```

2. 安装（安装过程依机器可能**长达数小时**，请耐心等待）

   ```bash
   $ tar -zxvf gcc-7.5.0.tar.gz
   $ cd gcc-7.5.0
   $ ./contrib/download_prerequisites
   $ mkdir gcc-build-7.5.0
   $ cd gcc-build-7.5.0
   $ ../configure --enable-checking=release --enable-languages=c,c++ --disable-multilib
   $ make
   $ make install
   ```

3. 测试

   ```bash
   $ gcc -v
   Using built-in specs.
   COLLECT_GCC=gcc
   COLLECT_LTO_WRAPPER=/usr/lib/gcc/x86_64-linux-gnu/7/lto-wrapper
   OFFLOAD_TARGET_NAMES=nvptx-none
   OFFLOAD_TARGET_DEFAULT=1
   Target: x86_64-linux-gnu
   Configured with: ...
   Thread model: posix
   gcc version 7.5.0 (Ubuntu 7.5.0-3ubuntu1~18.04)
   ```

#### 安装MobaXterm软件

1. 下载（下载免费版即可）

   https://mobaxterm.mobatek.net/

   根据个人喜欢安装相应版本（Windows，这里以安装版本为例演示安装过程）

   ![image-20210401115654356](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210401115656.png)

2. 安装

   右键解压

   ![image-20210401115940398](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210401115942.png)

   双击即可开始安装

   ![image-20210401120011829](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210401120013.png)

   安装步骤

   ![image-20210401120031559](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210401120033.png)

   接收许可

   ![image-20210401120047583](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210401120049.png)

   选择安装路径

   ![image-20210401120136012](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210401120137.png)

   点击安装即可

   ![image-20210401120202064](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210401120203.png)

3. 安装成功

   ![image-20210401115446914](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210401115448.png)

#### 远程连接使用vscode开发（依据个人喜好选择开发工具）

1. 下载VsCode软件

   https://code.visualstudio.com/

2. 安装

3. 安装ssh-remote工具

   ![image-20210330103606133](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210330103825.png)

4. 连接远程服务器

   ![image-20210330104208467](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210330104210.png)![image-20210330104248184](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210330104250.png)

   信息输入正确后，左侧列表会出现新添加的远程主机

   ![image-20210330104341284](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210330104343.png)

   ![image-20210330104444094](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210330104446.png)

   选择继续

   ![image-20210330104541908](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210330104544.png)

   然后按照提示输入密码

   ![image-20210330104633763](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210330104635.png)

5. 打开项目文件夹

   ![image-20210330104102689](https://gitee.com/wei_manzhou/blog_picture/raw/master/20210330104104.png)

至此开发所需环境已经配置好了，接下来可以愉快的开发啦。

### 其它平台也类似

### 创建目录结构

```bash
$ tree ../ -L 1
../
├── 01-ilinux-bootsect
├── 02-ilinux-print-method
├── 03-ilinux-gdt
├── 04-ilinux-protect-mode-advanced
├── 06-ilinux-fat-fs
├── 07-ilinux-loader
├── 08-ilinux-makefile
├── 09-ilinux-get-mm-info
├── 10-ilinux-elf
├── 10-ilinux-load-kernel
├── 11-ilinux-continue-protect-mode
├── 12-ilinux-print-mm-info
├── 13-ilinux-paging
├── 14-ilinux-start-kernel
├── 15-ilinux-start-with-c
├── 16-ilinux-idt-tss
├── 17-ilinux-k-printf
├── 18-ilinux-hardware-interrupt
├── 19-ilinux-multiprocess-framework
├── 20-ilinux-multiprocess
├── 21-ilinux-halt-process
├── 22-ilinux-process-scheduling
├── 23-ilinux-ipc
├── 23-ilinux-ipc-new
├── 24-ilinux-prefect-clock
├── 25-ilinux-realtime
├── 25-ilinux-realtime-new
├── 26-ilinux-delay-time
├── 26-ilinux-delay-time-new
├── 26-ilinux-delay-time-new-new
├── 27-ilinux-keyboard
├── 30-ilinux-elf
├── LICENSE
└── README.md
```

## 四、实验结果与分析

## 五、实验总结

## 六、总结与回顾

经过上面的努力，我们已经将实验所需的全部工具都安装好了，至于为什么要安装这些工具，只是为了让开发、调试更加顺手而已，读者可以根据自己的习惯来使用工具。

对笔者而言，比较习惯如下的开发流程：

1. 使用`VsCode`远程连接服务器进行编写代码
2. 使用`MobaXterm`远程连接服务器，进行编译以及生成操作系统镜像
3. 使用`qemu`工具运行系统镜像
4. 使用`bochs`工具调试内核代码
5. 使用`git`进行版本控制工具

至于为什么没有使用VsCode自带的终端，是因为`qemu`和`bochs`运行之后，是要在`Windows`下打开窗口的，而`VsCode`的终端没有这样的功能。

