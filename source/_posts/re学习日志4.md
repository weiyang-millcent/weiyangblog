---
title: "re学习日志4"
date: 2026-07-11
categories: ["逆向工程"]
tags: ["学习笔记"]
description: 记录re的学习过程。
cover: /img/7d9292cfe64bb23acf95ebbd947df5ad.png
toc: true  
comments: true  

---

[Reverse]  数据脱敏算法逆向
=========================

题目说明
----

题目给出了三个文件：
    题目说明.docx
    ds.exe
    fake_data.xlsx

要求分析 `ds.exe` 的数据脱敏方式，并还原 `fake_data.xlsx` 中姓名为 `丁**` 的出生日期。
![题干](imgs/2026-07-12-095039.png)

打开 Excel 后，可以找到目标记录，其 `birthday` 字段为：
    b57e1b10af2f3bce1e4a374a49ed538023f461802227e74169a822b56ed1b23b



最终目标就是还原这串数据对应的原始日期。

* * *

初步分析
----

目标字符串长度为 64 个十六进制字符，也就是 32 字节。
    64 hex = 32 bytes = 256 bit

* * *

确定考点
------

将exe文件载入 IDA 后，从数据处理相关函数开始向下分析。

在一个初始化函数中发现了一组常量：
![IDA截图](/imgs/2026-07-12-101355.png)

将其转为十六进制后为:

7380166F
4914B2B9
172442D7
DA8A0600
A96F30BC
163138AA
E38DEE4D
B0FB0E4E

经过查询发现这组值是 SM3 的标准初始向量。

继续查看函数内部，还可以看到：
    79CC4519
    7A879D8A

这两个值同样是 SM3 的轮常量。

因此可以确定该函数实现的是 SM3。

于是最开始的数据处理流程可以确定为：
    birthday->SM3

SM3 输出长度正好为 32 字节，与目标字段长度一致。

* * *

继续追踪 SM3 输出
-----------

接下来查看 SM3 函数的交叉引用。

在调用 SM3 后，程序将 32 字节摘要继续传递到另一个数据处理函数中。

这个函数有反调试处理，经过相关插件处理后可以正常查看

继续分析该函数，可以看到下面几组明显的常量：
    A3B1BAC6
    56AA3350
    677D9197
    B27022DC

这是 SM4 的系统参数 FK。

同时还能找到 SM4 的 CK 常量以及 32 轮循环结构。

因此确定第二层算法为 SM4。

现在确认数据流程为：
    birthday->SM3-> SM4

* * *

判断 SM4 工作模式
-----------

SM3 输出固定为：
    32 bytes

SM4 的分组长度为：
    16 bytes

因此 SM3 的摘要正好可以分成两个 SM4 Block：
    32 bytes = 16 bytes + 16 bytes

目标密文同样是 32 字节。

如果使用 PKCS#7 Padding，由于原数据已经是 16 字节的整数倍，还需要额外填充 16 字节，最终密文应该是 48 字节。

但实际结果仍然是 32 字节，因此这里没有 Padding。

继续查看加密函数，可以在sub_1400015E0 这个函数中看到当前明文块在进入 SM4 加密前，会先与一个 16 字节缓冲区进行 XOR：

![IDA截图](/imgs/2026-07-12-112708.png)
    tmp[i] = input[i] ^ iv[i];

第一块处理完成后，输出密文又被作为下一块的 XOR 数据。

这个结构就是典型的 CBC：
    C1 = E(P1 XOR IV)
    C2 = E(P2 XOR C1)

因此完整算法已经可以确定为：
    SM3 -> SM4-CBC

数据处理逻辑为：
    birthday
        |
        v
    SM3
        |
        v
    32-byte digest
        |
        v
    SM4-CBC Encrypt
        |
        v
    32-byte ciphertext
        |
        v
    Hex Encode

* * *

分析 Key 和 IV
-----------

确定 SM4-CBC 后，下一步就是找到 Key 和 IV。

继续向上追踪 SM4 初始化参数，可以发现程序调用了：

![IDA截图](/imgs/2026-07-12-113701.png)
    srand(0xCC);

随后调用多次：
    rand();

一开始我以为程序是随机生成 Key 或 IV，但 Seed 被固定为：
    0xCC

这意味着随机序列实际上是固定的。

只要使用相同的 `rand()` 实现，就可以完全复现程序运行时得到的结果。

由于目标程序是 Windows PE，因此这里需要按照 MSVC CRT 的 `rand()` 实现进行计算。

 MSVC `rand()` 逻辑如下：
    state = state * 214013 + 2531011;
    return (state >> 16) & 0x7fff;

Python 复现：
    def msvc_rand(state):
        state = (
            state * 214013 + 2531011
        ) & 0xffffffff
        value = (
            state >> 16
        ) & 0x7fff
        return state, value

从：
    seed = 0xCC

开始计算，即可得到程序中的固定随机数序列。

继续看调用逻辑，可以发现程序先准备了一组 Key 和 IV，然后使用 `rand()` 的结果对其中部分字节进行修改。

所以直接从程序字符串中提取 Key 和 IV 是不够的，必须还原运行时修改后的最终结果。

最终得到：
    Key = j8dadef565d9a0ad
    IV = 4ed112dd3B275564

* * *

* * *

还原整体算法
------

最终得到程序处理生日的完整逻辑：
    Result =
    HEX(
        SM4-CBC-ENC(
            Key,
            IV,
            SM3(Birthday)
        )
    )

其中：
    Key = j8dadef565d9a0ad
    IV  = 4ed112dd3B275564

日期格式根据程序中的字符串长度和处理方式，可以确定为：
    YYYY-MM-DD

目标密文：
    b57e1b10af2f3bce1e4a374a49ed538023f461802227e74169a822b56ed1b23b

* * *

利用思路
----

此时虽然已经得到了 Key 和 IV，但直接解密 SM4 并不能得到生日。

原因是原始数据先经过了 SM3：
    Birthday
        |
        v
    SM3
        |
        v
    SM4-CBC

SM4 解密后得到的只是：
    SM3(Birthday)

SM3 是哈希算法，不能直接逆运算。

不过生日的数据范围非常小。

假设出生日期范围从：
    1900-01-01

到：
    2025-12-31

总共只有四万多个合法日期。

因此可以先解密目标密文得到目标 SM3 摘要，然后枚举全部合法日期进行比较。

流程如下：
    Target Cipher
          |
          v
    SM4-CBC Decrypt
          |
          v
    Target SM3 Digest
          |
          v
    枚举合法日期
          |
          v
    SM3(date)
          |
          v
    Compare

对应逻辑：
    target_digest = sm4_cbc_decrypt(
        target,
        KEY,
        IV
    )
    for birthday in all_dates:
        digest = sm3(
            birthday.encode()
        )
        if digest == target_digest:
            print(birthday)
            break

日期使用 `datetime` 枚举，避免产生 `2024-02-31` 之类的非法日期。
    from datetime import date, timedelta
    current = date(1900, 1, 1)
    end = date(2025, 12, 31)
    while current <= end:
        birthday = current.strftime("%Y-%m-%d")

        # SM3 compare

        current += timedelta(days=1)

运行后得到：
    [+] FOUND: 1958-04-13

* * *

解题思路总结
------

整个分析过程如下：
    发现 birthday 为 64 位 Hex
            |
            v
    猜测 256 位 Hash
            |
            v
    SHA-256 枚举失败
            |
            v
    逆向 ds.exe
            |
            v
    通过固定常量识别 SM3
            |
            v
    直接 SM3 枚举失败
            |
            v
    追踪 SM3 输出
            |
            v
    通过 FK / CK 常量识别 SM4
            |
            v
    分析 XOR 链确认 CBC
            |
            v
    追踪 Key 和 IV
            |
            v
    发现 srand(0xCC)
            |
            v
    复现 MSVC rand()
            |
            v
    处理 BeingDebugged 干扰
            |
            v
    恢复最终 Key / IV
            |
            v
    SM4-CBC 解密得到 SM3 Digest
            |
            v
    枚举合法日期
            |
            v
    1958-04-13

这道题的关键不在于完整阅读 SM3 或 SM4 的每一轮实现，而是先通过标准算法常量快速识别算法，再沿着数据流继续追踪。

确认处理链为：
    SM3 -> SM4-CBC

后，只需要恢复 Key、IV 和输入格式。

最后空间进行枚举。
Flag

----

    1958-04-13

<!-- 正文从这里开始 -->
