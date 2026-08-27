---
title: "Linux GNU编译环境与Pwndbg配置记录"
date: 2026-08-27
categories: ["逆向工程"]
tags: ["学习笔记"]
description: 记录在Kali虚拟机中配置GNU编译环境、安装Pwndbg以及排查VMware网络问题的过程。
cover: /img/20260827-linux-pwndbg-cover.png
toc: true
comments: true
---

# Linux GNU编译环境与Pwndbg配置记录

## 前言

最近准备在 Kali Linux 虚拟机中搭建一个用于 C 语言编译和二进制调试的环境，主要需要配置 Vim、GNU 编译工具链以及 Pwndbg。

本来安装过程并不复杂，但在克隆 Pwndbg 仓库时遇到了 GitHub 连接超时的问题。经过排查，最后发现 VMware 的 VMnet8 虚拟网卡也存在异常。本篇博客记录完整的安装和解决过程。

## 一、安装Vim

首先安装用于编辑代码的 Vim：

```bash
apt install vim
```

![安装Vim](/imgs/20260827-vim-install.png)

系统提示将升级以下三个软件包：

```text
vim
vim-tiny
xxd
```

输入 `y` 后继续安装，最终升级到 `2:9.2.0524-1+b1`，安装过程没有出现报错。

## 二、安装GNU编译环境

接下来安装 Linux 中常用的基础编译工具：

```bash
apt install build-essential
```

![安装build-essential](/imgs/20260827-build-essential.png)

这里系统提示：

```text
build-essential 已经是最新版 (12.12)
```

说明当前 Kali 中已经安装了基础编译环境。`build-essential` 通常包含 GCC、G++、Make 等常用工具，可以使用下面的命令分别检查：

```bash
gcc --version
g++ --version
make --version
```

## 三、安装Pwndbg

Pwndbg 是一个对 GDB 进行增强的调试插件，在逆向分析和 Pwn 题目中非常实用。

最开始尝试从 GitHub 克隆项目：

```bash
git clone https://github.com/pwndbg/pwndbg.git
```

但是等待一段时间后出现了报错：

```text
Failed to connect to github.com port 443
Could not connect to server
```

### 1.检查网络状态

首先检查 Kali 能否解析并访问 GitHub：

```bash
ping -c 4 github.com
```

测试结果中，`github.com` 被成功解析为 `20.205.243.166`，四个数据包全部收到回复，没有丢包。

接着测试 HTTPS 连接：

```bash
curl -I --connect-timeout 10 https://github.com
```

结果为：

```text
curl: (28) Connection timed out after 10005 milliseconds
```

最后检查域名解析：

```bash
getent hosts github.com
```

可以正常得到：

```text
20.205.243.166  github.com
```

由此可以判断，DNS 解析和基础网络正常，问题主要出现在 GitHub 的 HTTPS 连接上。

## 四、排查VMware NAT网络

虚拟机使用的是 VMware NAT 模式，并且网络适配器已经勾选“已连接”和“启动时连接”。

![VMware NAT配置](/imgs/20260827-vmware-nat.png)

在 Kali 中查看网络信息：

```bash
ip route
ip addr
```

得到的主要信息为：

```text
Kali IP：192.168.44.129
默认网关：192.168.44.2
网段：192.168.44.0/24
```

按照 VMware NAT 的常见配置，Windows 宿主机的 VMnet8 地址应该是 `192.168.44.1`。但是测试时出现了异常：

```bash
ping -c 4 192.168.44.1
```

```text
Destination Host Unreachable
No route to host
```

### 1.检查VMnet8状态

在 VMware 的虚拟网络编辑器中可以看到：

- VMnet8 使用 NAT 模式；
- 子网为 `192.168.44.0`；
- DHCP 已启用；
- 已勾选将主机虚拟适配器连接到此网络。

但是在 Windows PowerShell 中进一步检查：

```powershell
Get-NetAdapter -IncludeHidden |
Where-Object {$_.InterfaceDescription -like "*VMware*"} |
Format-Table Name, InterfaceDescription, Status
```

最开始显示 VMnet8 和 VMnet1 均为 `Not Present`，说明 Windows 仍保留对应的配置，但虚拟网卡设备没有正常加载。

### 2.重启VMware网络组件

以管理员身份打开 PowerShell，执行：

```powershell
Disable-NetAdapter -Name "VMware Network Adapter VMnet8" -Confirm:$false
Start-Sleep -Seconds 2
Enable-NetAdapter -Name "VMware Network Adapter VMnet8" -Confirm:$false
Restart-Service -Name "VMnetDHCP" -Force
Restart-Service -Name "VMware NAT Service" -Force
```

然后回到 Kali，重新连接网卡：

```bash
nmcli device disconnect eth0
nmcli device connect eth0
```

重新测试：

```bash
ping -c 4 192.168.44.1
```

此时四个数据包全部成功返回，平均延迟约为 `0.375 ms`。至此，Kali 与 Windows 宿主机之间的 VMware NAT 内部通信恢复正常。

不过再次执行：

```bash
curl -I --connect-timeout 10 https://github.com
```

GitHub 仍然连接超时。由此可以确认，VMware 内部网络问题已经修复，但 GitHub 的 HTTPS 访问是另一个独立问题。

## 五、使用官方脚本安装Pwndbg

由于无法正常克隆 GitHub 仓库，最后改用 Pwndbg 官方提供的便携安装入口：

```bash
curl -qsL https://install.pwndbg.re | sh -s -- -t pwndbg-gdb
```

安装完成后显示：

```text
Installing... pwndbg-gdb in /usr/local/lib/pwndbg-gdb
Creating... symlink in /usr/local/bin/pwndbg
Installation complete.
Run binary with: pwndbg
```

![Pwndbg安装完成](/imgs/20260827-pwndbg-install.png)

这说明 Pwndbg 被安装到 `/usr/local/lib/pwndbg-gdb`，同时在 `/usr/local/bin/pwndbg` 创建了命令链接，可以在任意目录直接启动。

## 六、启动并测试Pwndbg

在终端中执行：

```bash
pwndbg
```

启动后显示：

```text
pwndbg: loaded 199 pwndbg commands.
pwndbg: created 12 GDB functions.
```

说明 Pwndbg 已经成功载入 GDB。需要注意，Pwndbg 并不是一个独立的图形化软件，而是增强后的 GDB 终端调试界面。

进入后通常会看到：

```text
pwndbg>
```

可以使用下面的命令查看当前上下文：

```gdb
context
```

退出调试器：

```gdb
quit
```

## 七、加载chall1进行测试

最后使用 Pwndbg 加载一个 ELF 程序 `chall1`：

```bash
pwndbg ./chall1
```

输出如下：

```text
Reading symbols from ./chall1...
Downloading separate debug info for /home/kali/Desktop/chall1
Download failed: Invalid argument. Continuing without separate debug info.
(No debugging symbols found in ./chall1)
```

虽然下载独立调试信息失败，但程序已经成功载入。`No debugging symbols found` 表示程序编译时没有加入调试信息，并不影响我们通过反汇编继续分析。

可以先关闭自动下载调试信息：

```gdb
set debuginfod enabled off
```

然后查看程序保护和基本信息：

```gdb
checksec
info file
info functions main
disassemble main
```

如果能够找到 `main` 函数，可以设置断点并运行：

```gdb
break main
run
context
```

如果程序没有保留函数符号，则可以从入口点开始：

```gdb
starti
x/10i $pc
info registers
```

通过文件头检查可以确认，`chall1` 是一个 64 位小端序 x86-64 ELF 文件，入口地址为 `0x4010d0`。程序保留了 `main` 符号，并导入了 `fgets`、`printf`、`puts`、`strcspn` 和 `strncmp` 等函数，为后续动态调试提供了方向。

## 总结

本次主要完成了下面几个工作：

1. 安装并升级 Vim；
2. 确认 `build-essential` 和 GNU 基础编译环境已经安装；
3. 排查 GitHub 的 DNS、ICMP 和 HTTPS 连接；
4. 修复 VMware VMnet8 虚拟网卡无法与宿主机通信的问题；
5. 使用 Pwndbg 官方脚本完成安装；
6. 成功启动 Pwndbg 并加载 `chall1` 进行测试。

这次配置过程中遇到的问题比较多，但也让我对 VMware NAT、VMnet8、DNS、HTTPS 连接以及 Pwndbg 的安装方式有了更清楚的认识。

目前 GNU 编译环境和 Pwndbg 已经可以正常使用，接下来就可以继续进行 ELF 程序的动态调试和逆向分析。
