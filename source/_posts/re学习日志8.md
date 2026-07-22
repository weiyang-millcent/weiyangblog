---
title: "re学习日志8"
date: 2026-07-22
categories: ["逆向工程"]
tags: ["学习笔记"]
description: 记录re的学习过程。
cover: /img/202607220840247024.png
toc: true  
comments: true  

---

# re学习日志8

## OhNativeEnc

这是一道安卓逆向，题目给出提示：

![](imgs/20267221.png)


经过搜索我们得知：

![](imgs/20267222.png)

同样的，主函数也做了提示

![](imgs/20267223.png)

所以接下来我们先从so文件开始分析



### 1.利用apktool进行解包

我们已经知道需要分析so文件，而主函数又告诉我们so文件在lib文件中，而lib文件需要对apk文件进行解包。


![](imgs/20267224.png)


接下来用ida对这个so文件进行反编译：

![](imgs/20267225.png)

这里就是核心加密逻辑，单看这个是一个魔改版的TEA加密算法

结合前后内容可以分析出程序的完整逻辑：

1.从native中获得input

2.打印日志

`_android_log_print(4, "native", "input:%s", v3)`

3.复制前20字节的内容

`strncpy(dest, v3, 0x20u)`

4.分成四组再进行加密处理

5.与数组mm比较



找到密钥为：

![](imgs/20267226.png)

mm数组为：

![](imgs/20267227.png)

解密脚本：

```pyhton

import struct

DELTA = 0x1BF52

def mx(z, y, sum_, p, e, k):
    return (
        ((z >> 5 ^ y << 2) + 
         (y >> 3 ^ z << 4))
        ^
        ((sum_ ^ y) + 
         (k[(p & 3) ^ e] ^ z))
    )


def xxtea_decrypt(data, key):

    n = len(data)

    if n < 2:
        return data

    rounds = 12

    v = list(struct.unpack("<%dI" % (n // 4), data))

    k = list(struct.unpack("<4I", key))

    total = rounds * DELTA

    while total:

        e = (total >> 2) & 3

        for p in range(len(v)-1, 0, -1):

            z = v[p-1]
            y = v[p]

            v[p] -= mx(
                z,
                y,
                total,
                p,
                e,
                k
            )

            v[p] &= 0xffffffff


        z = v[-1]
        y = v[0]

        v[0] -= mx(
            z,
            y,
            total,
            0,
            e,
            k
        )

        v[0] &= 0xffffffff


        total -= DELTA


    return struct.pack(
        "<%dI" % len(v),
        *v
    )

if __name__ == "__main__":
    cipher_hex = (
        "b6536e4d775d08d2"
        "fb2c631ebb7b019b"
        "f5046af40e842747"
        "64a1e4d9ef124437"
    )

    cipher = bytes.fromhex(cipher_hex)

    key = b"ThisIsAXXteaKey"

    key = key.ljust(16, b"\x00")

    flag = xxtea_decrypt(
        cipher,
        key
    )


    print(
        flag.decode(
            "utf-8",
            errors="ignore"
        )
    )
```



### 最终flag:

**flag{Ur_G00d_@_n@tive_Func}**






