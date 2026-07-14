---

title: 初步学习利用bkcrack进行明文攻击
date: 2026-03-09
categories: ["misc"]
tags: ["学习笔记"]
description: 记录misc的学习过程。
cover: /img/b061cb2f4937e7abcf809fa5d20e9f4b.jpg  
toc: true  
comments: true  
---

<!-- 正文从这里开始 -->

# CTF中的ZIP明文攻击

## 前言

在 CTF 或数字取证中，我们常遇到加密的 ZIP 压缩包，而其中一种常见且高效的破解方式就是 **明文攻击（Known Plaintext Attack）**。

## 一、明文攻击的适用条件

### 必须满足的条件

| 条件       | 说明                                       |
| -------- | ---------------------------------------- |
| **加密算法** | 必须是 ZipCrypto（传统加密），对 AES-256 加密的 ZIP 无效 |
| **压缩方式** | 必须是 Store 或 Deflate                      |
| **已知明文** | 至少 12 字节的已知明文，且至少 8 字节连续                 |
| **数据来源** | 已知明文必须来自解密后的真实数据                         |
| **偏移位置** | 必须知道明文的正确偏移（-o 或 -x 中的偏移）                |

## 二、工具介绍：bkcrack

bkcrack 是目前最常用的 ZIP 明文攻击工具。

### 基本语法

```powershell
.\bkcrack -C <压缩包.zip> -c <文件名> -o <偏移量> -x <偏移> <十六进制数据>

参数说明
参数    说明
-C    指定加密的 ZIP 文件
-c    指定 ZIP 内的目标文件
-o    明文的起始偏移（相对于文件数据起始）
-x    指定已知的十六进制明文数据
三、bkcrack使用时的常见操作和指令
基于文件格式固定字段的通用模板
模板 1：明文对齐文件数据起始（-o 0）
.\bkcrack -C 压缩包.zip -c 文件名 -o 0 `
  -x 0 固定字段1 `
  -x 8 固定字段2 `
  -x 12 固定字段3
模板 2：明文对齐包含 12 字节加密头的开头（-o 12）
.\bkcrack -C 压缩包.zip -c 文件名 -o 12 `
  -x 0 固定字段1 `
  -x 8 固定字段2 `
  -x 12 固定字段3
四、常见文件格式的固定字段指令
1.PNG 图片

PNG 文件头（前 16 字节）：

89 50 4E 47 0D 0A 1A 0A 00 00 00 0D 49 48 44 52

攻击指令：

情况一：对齐文件数据起始
.\bkcrack -C secret.zip -c secret.png -o 0 `
  -x 0 89504e470d0a1a0a `
  -x 8 0000000d `
  -x 12 49484452
情况二：对齐包含加密头的开头
.\bkcrack -C secret.zip -c secret.png -o 12 `
  -x 0 89504e470d0a1a0a `
  -x 8 0000000d `
  -x 12 49484452
2.PDF 文件

PDF 文件头（前 8 字节）：

25 50 44 46 2D 31 2E 34  ( %PDF-1.4 )

攻击指令：

情况一
.\bkcrack -C secret.zip -c secret.pdf -o 0 `
  -x 0 25504446 `
  -x 4 2d312e34
情况二
.\bkcrack -C secret.zip -c secret.pdf -o 12 `
  -x 0 25504446 `
  -x 4 2d312e34
3.JPEG 图片

JPEG 文件头：

FF D8 FF E0 00 10 4A 46 49 46 00 01

攻击指令：

情况一
.\bkcrack -C secret.zip -c secret.jpg -o 0 `
  -x 0 ffd8ffe0 `
  -x 4 00104a46 `
  -x 8 49460001
情况二
.\bkcrack -C secret.zip -c secret.jpg -o 12 `
  -x 0 ffd8ffe0 `
  -x 4 00104a46 `
  -x 8 49460001
4.PCAP（非 pcapng）

传统 pcap 文件头（前 16 字节）：

D4 C3 B2 A1 02 00 04 00 00 00 00 00 00 00 00 00

攻击指令：

情况一
.\bkcrack -C secret.zip -c secret.pcap -o 0 `
  -x 0 d4c3b2a1 `
  -x 4 02000400 `
  -x 8 00000000
情况二
.\bkcrack -C secret.zip -c secret.pcap -o 12 `
  -x 0 d4c3b2a1 `
  -x 4 02000400 `
  -x 8 00000000
5.ZIP 压缩包

ZIP 文件头：

50 4B 03 04 14 00 00 00 00 00
情况一
.\bkcrack -C outer.zip -c inner.zip -o 0 `
  -x 0 504b0304 `
  -x 4 14000000
情况二
.\bkcrack -C outer.zip -c inner.zip -o 12 `
  -x 0 504b0304 `
  -x 4 14000000
五、实战步骤
步骤 1：确定文件类型

首先确定加密 ZIP 包内的文件类型（PNG、PDF、JPG 等）。

步骤 2：选择合适的模板

根据文件类型选择对应的固定字段。

步骤 3：运行 bkcrack
.\bkcrack -C target.zip -c encrypted_file.png -o 0 -x 0 89504e470d0a1a0a -x 8 0000000d -x 12 49484452
步骤 4：获取密钥

成功后会得到三个密钥：

Key: a1b2c3d4 e5f6a7b8 c9d0e1f2
步骤 5：解密或修改
解密整个压缩包
.\bkcrack -C target.zip -k a1b2c3d4 e5f6a7b8 c9d0e1f2 -D decrypted.zip
或者解密特定文件
.\bkcrack -C target.zip -c encrypted_file.png -k a1b2c3d4 e5f6a7b8 c9d0e1f2 -d decrypted.png
六、注意事项

偏移量要准确：错误偏移会导致攻击失败

加密算法确认：先用工具确认 ZIP 使用 ZipCrypto 而非 AES-256

压缩方式：Store 模式成功率最高，Deflate 也可行但需要更多计算

多尝试偏移：如果 -o 0 不行，试试 -o 12 或其他常见偏移

七、总结

明文攻击是破解 ZipCrypto 加密的高效方法，关键在于：

找对文件格式的固定字段

用对偏移量

至少有 12 字节已知明文

```
