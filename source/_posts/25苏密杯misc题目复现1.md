---
title: "25苏密杯misc题目复现1"
date: 2026-09-09
categories: ["misc"]
tags: ["学习笔记"]
description: 记录25苏密杯两道Misc题目的复现过程，包括多层编码识别和利用固定前缀进行已知明文攻击。
cover: /imgs/20260909-sumicup-misc-cover.jpg
toc: true
comments: true
---

# 25苏密杯misc题目复现1

## 前言

本篇博客记录 2025 苏密杯中两道 Misc 题目的复现过程。

第一道题主要考查 Base64 和 Hex 的识别与解码，第二道题则需要阅读题目给出的 Python 加密代码，利用固定明文前缀恢复初始密钥，再逐组解密得到结果。

虽然两道题使用的算法并不复杂，但都需要先根据题目特征判断下一步操作，尤其是第二题，需要把看起来比较复杂的位运算拆开理解。

---

# 第一题：熊猫特工的千层饼

## 一、查看题目附件

题目给出一个 RAR 压缩包：

```text
熊猫特工的千层饼附件.rar
```

将压缩包解压后，得到一个 TXT 文档：

```text
熊猫特工的千层饼附件.txt
```

![题目附件](/imgs/20260909-pancake-files.png)

## 二、查看文档内容

打开 TXT 文档，可以看到一段很长的字符串：

![打开文档](/imgs/20260909-pancake-base64.png)

观察这段字符串可以发现：

- 字符主要由大小写字母和数字组成；
- 字符串长度较长；
- 末尾存在 `==`；
- 字符组成符合 Base64 的特征。

因此初步判断第一层使用的是 Base64 编码。

## 三、Base64解码

将字符串放入 CyberChef，使用：

```text
From Base64
```

得到下面的内容：

```text
The flag is base64 encoded. But don't stop there.
This is only the first layer. Flag:
6d756c74696c617965725f656e636f64696e67
```

![Base64解码结果](/imgs/20260909-pancake-base64-result.png)

提示中明确说明这只是第一层，不能在这里停止。

观察最后一段字符串：

```text
6d756c74696c617965725f656e636f64696e67
```

其中只包含 `0-9` 和 `a-f`，并且长度为偶数，因此可以判断下一层是十六进制编码。

## 四、Hex解码

继续在 CyberChef 中使用：

```text
From Hex
```

解码结果为：

```text
multilayer_encoding
```

![Hex解码结果](/imgs/20260909-pancake-hex-result.png)

这里的 `5f` 对应 ASCII 字符 `_`，因此中间是下划线，而不是斜杠。

根据题目的 Flag 格式，最终得到：

```text
flag{multilayer_encoding}
```

## 五、使用Python完成两层解码

除了 CyberChef，也可以使用 Python 一次完成两层处理：

```python
import base64
import re


with open(
    "熊猫特工的千层饼附件.txt",
    "r",
    encoding="utf-8",
) as f:
    data = f.read().strip()

# 第一层：Base64
layer1 = base64.b64decode(data).decode("utf-8")
print("[+] Base64 result:")
print(layer1)

# 从英文提示中提取Hex字符串
hex_data = re.search(
    r"[0-9a-fA-F]{20,}",
    layer1,
).group()

# 第二层：Hex
layer2 = bytes.fromhex(hex_data).decode("utf-8")
print("[+] Hex result:")
print(layer2)

print("[+] Flag:")
print(f"flag{{{layer2}}}")
```

运行后会输出：

```text
[+] Hex result:
multilayer_encoding

[+] Flag:
flag{multilayer_encoding}
```

## 六、第一题总结

这道题的完整解码流程为：

```text
解压RAR附件
      |
      v
打开TXT文档
      |
      v
根据字符范围和末尾“==”识别Base64
      |
      v
Base64解码得到英文提示和Hex字符串
      |
      v
根据0-9、a-f和偶数长度识别Hex
      |
      v
Hex解码得到Flag内容
      |
      v
补充Flag格式
```

这类题目的重点是识别编码特征。遇到一层解码结果后，不要看到可读文本就直接停止，还要继续判断其中是否包含新的编码数据或提示。

---

# 第二题：窃听风云

## 一、查看附件

题目附件中包含两个文件：

```text
附件.txt
题目脚本.py
```

`附件.txt` 中给出了密文：

```text
3d67f186c0897b06b5a5bbce26355fed
2f4ea19384892d06c0b1bec8286607e0
284ba390828b2f01c9b4eacb763f0ce5
1d2a94f5b3e81c63c3b3e0cf7a3a3e84
```

去掉换行后，密文一共有 128 个十六进制字符：

```text
128 Hex = 64 Bytes
```

题目每 16 字节处理一组，因此一共有4个密文分组。

## 二、分析明文预处理

题目首先定义了一个固定前缀：

```python
PREFIX = "Original Message:"
```

加密时，无论用户输入什么内容，程序都会先在消息前面添加这个前缀：

```python
message_with_prefix = PREFIX + message_str
message = message_with_prefix.encode("utf-8")
```

因此所有密文对应的明文都具有相同开头：

```text
Original Message:
```

这给我们提供了一段已知明文，是本题恢复密钥的关键。

## 三、理解16字节分组

分组代码为：

```python
blocks_num = (len(message) + 15) // 16

blocks = [
    message[i * 16:(i + 1) * 16]
    for i in range(blocks_num)
]
```

其中：

```python
message[i * 16:(i + 1) * 16]
```

表示每次从消息中取出16字节。

Python 切片包含开始位置，但不包含结束位置，因此：

| `i` | 开始位置 | 结束位置 | 切片 |
| --- | ---: | ---: | --- |
| 0 | 0 | 16 | `message[0:16]` |
| 1 | 16 | 32 | `message[16:32]` |
| 2 | 32 | 48 | `message[32:48]` |
| 3 | 48 | 64 | `message[48:64]` |

因为：

```text
1字节 = 8位
16字节 = 128位
```

所以每一个明文分组都被转换为一个128位整数进行处理。

如果最后一组不足16字节，程序使用零字节补齐：

```python
blocks[-1] = blocks[-1].ljust(16, b"\x00")
```

## 四、分析关键加密代码

题目的核心加密代码为：

```python
block_int = int.from_bytes(
    block,
    byteorder="big",
)

encrypted_block = (
    ((block_int & 0xFFFFFFFFFFFFFFFF) << 64)
    | (block_int >> 64)
) ^ k
```

这段代码看起来比较复杂，但实际上只完成两个操作：

```text
交换分组的高低64位
        ↓
与密钥k进行异或
```

### 1.将16字节转换为整数

代码：

```python
block_int = int.from_bytes(
    block,
    byteorder="big",
)
```

使用大端序将16字节转换为128位整数。

对于一个16字节分组，可以将它表示为：

```text
前8字节 | 后8字节
高64位  | 低64位
```

第一组固定明文为：

```text
Original Message
```

正好可以拆成：

```text
Original |  Message
高64位   | 低64位
```

### 2.理解64位掩码

代码中的：

```python
0xFFFFFFFFFFFFFFFF
```

由16个十六进制的 `F` 组成。一个十六进制位表示4个二进制位，因此它一共表示64个二进制 `1`。

它相当于：

```python
MASK64 = (1 << 64) - 1
```

### 3.取出低64位

代码：

```python
block_int & 0xFFFFFFFFFFFFFFFF
```

使用按位与保留原整数的低64位，并将高64位清零：

```text
原高64位 | 原低64位
    AND
全零      | 全一
-----------------
全零      | 原低64位
```

### 4.将低64位移动到高位

代码：

```python
(block_int & 0xFFFFFFFFFFFFFFFF) << 64
```

取出低64位后，再整体左移64位：

```text
全零      | 原低64位
              << 64
原低64位 | 全零
```

这样，原来的低64位就被放到了高64位。

### 5.取出原来的高64位

代码：

```python
block_int >> 64
```

将128位整数向右移动64位：

```text
原高64位 | 原低64位
              >> 64
全零      | 原高64位
```

低64位被移出，原高64位被移动到低位。

### 6.使用按位或组合

代码：

```python
((block_int & MASK64) << 64) | (block_int >> 64)
```

前一部分为：

```text
原低64位 | 全零
```

后一部分为：

```text
全零 | 原高64位
```

使用按位或组合后得到：

```text
原低64位 | 原高64位
```

因此这段位运算的作用就是交换高低64位。

可以将原代码改写成更容易理解的形式：

```python
MASK64 = 0xFFFFFFFFFFFFFFFF

low = block_int & MASK64
high = block_int >> 64

swapped = (low << 64) | high
encrypted_block = swapped ^ k
```

## 五、使用实际明文观察换位

第一组固定明文为：

```text
Original Message
```

前8字节：

```text
Original
```

对应十六进制：

```text
4f726967696e616c
```

后8字节：

```text
 Message
```

对应十六进制：

```text
204d657373616765
```

交换前：

```text
4f726967696e616c | 204d657373616765
```

交换后：

```text
204d657373616765 | 4f726967696e616c
```

转换回文本就是：

```text
 MessageOriginal
```

## 六、理解异或加密

高低位交换完成后，程序执行：

```python
encrypted_block = swapped ^ k
```

异或最重要的性质为：

```text
A XOR B XOR B = A
```

因为：

```text
B XOR B = 0
A XOR 0 = A
```

所以，如果加密为：

```text
C = Swap(P) XOR K
```

解密时再次与同一个密钥异或：

```text
C XOR K
= Swap(P) XOR K XOR K
= Swap(P)
```

再将高低64位交换一次：

```text
Swap(Swap(P)) = P
```

即可恢复原始明文。

## 七、通过已知明文恢复密钥

第一组加密满足：

```text
C0 = Swap(P0) XOR K0
```

第一组密文为：

```text
3d67f186c0897b06b5a5bbce26355fed
```

第一组明文固定为：

```text
Original Message
```

交换后的十六进制为：

```text
204d6573736167654f726967696e616c
```

因为异或可以抵消，所以：

```text
K0 = C0 XOR Swap(P0)
```

计算后得到初始密钥：

```text
1d2a94f5b3e81c63fad7d2a94f5b3e81
```

## 八、恢复后续轮密钥

题目每加密完一个分组都会执行：

```python
k += 1
```

因此4个分组使用的密钥分别为：

```text
第1组：K0
第2组：K0 + 1
第3组：K0 + 2
第4组：K0 + 3
```

只要恢复初始密钥，后续所有轮密钥也就全部确定，不需要爆破。

## 九、编写解密脚本

最终解密脚本如下：

```python
PREFIX = b"Original Message:"

cipher_hex = (
    "3d67f186c0897b06b5a5bbce26355fed"
    "2f4ea19384892d06c0b1bec8286607e0"
    "284ba390828b2f01c9b4eacb763f0ce5"
    "1d2a94f5b3e81c63c3b3e0cf7a3a3e84"
)

ciphertext = bytes.fromhex(cipher_hex)

MASK64 = 0xFFFFFFFFFFFFFFFF


def swap_high_low_64(value):
    low = value & MASK64
    high = value >> 64
    return (low << 64) | high


# 第一组已知明文
known_plaintext = PREFIX[:16]

p0 = int.from_bytes(
    known_plaintext,
    byteorder="big",
)

c0 = int.from_bytes(
    ciphertext[:16],
    byteorder="big",
)

# C0 = Swap(P0) XOR K0
# K0 = C0 XOR Swap(P0)
initial_key = c0 ^ swap_high_low_64(p0)

print("[+] Initial key:")
print(hex(initial_key))

plaintext = b""

for offset in range(0, len(ciphertext), 16):
    block = ciphertext[offset:offset + 16]

    block_int = int.from_bytes(
        block,
        byteorder="big",
    )

    block_number = offset // 16
    current_key = initial_key + block_number

    # 撤销异或
    decrypted = block_int ^ current_key

    # 再次交换高低64位
    decrypted = swap_high_low_64(decrypted)

    plaintext += decrypted.to_bytes(
        16,
        byteorder="big",
    )

# 删除末尾零填充
plaintext = plaintext.rstrip(b"\x00")

print("[+] Full plaintext:")
print(plaintext.decode("utf-8"))

if plaintext.startswith(PREFIX):
    message = plaintext[len(PREFIX):]

    print("[+] Message:")
    print(message.decode("utf-8"))
```

运行后得到：

```text
[+] Initial key:
0x1d2a94f5b3e81c63fad7d2a94f5b3e81

[+] Full plaintext:
Original Message:flag=9b2d5f7a1e3c8b9d2f5a7e1c3b9d2f5a

[+] Message:
flag=9b2d5f7a1e3c8b9d2f5a7e1c3b9d2f5a
```

最终答案为：

```text
flag=9b2d5f7a1e3c8b9d2f5a7e1c3b9d2f5a
```

## 十、第二题总结

这道题的完整分析流程为：

```text
阅读题目加密代码
        |
        v
发现固定前缀Original Message:
        |
        v
获得第一组16字节已知明文
        |
        v
拆解位运算，确认高低64位交换
        |
        v
使用C0 XOR Swap(P0)恢复初始密钥
        |
        v
根据k += 1恢复后续轮密钥
        |
        v
逐组异或并再次交换高低64位
        |
        v
拼接明文并去除零填充
        |
        v
得到最终结果
```

这道题的关键并不是完整爆破一个128位密钥，而是利用程序主动添加的固定前缀实施已知明文攻击。

只要理解异或运算可以抵消，并认识到高低64位交换执行两次会恢复原状，就可以直接逆向整个加密过程。

---

# 总结

本次复现的两道题分别考查了编码识别和简单密码算法逆向。

第一题需要根据字符串的字符范围、长度和填充符判断 Base64 与 Hex；第二题则需要从代码中寻找固定明文，并拆解掩码、移位、按位或和异或操作。

通过这两道题也可以发现，分析题目时不应该只关注最终使用了什么工具，更重要的是理解数据在每一步发生了什么变化。只要能够把复杂表达式拆成多个简单操作，题目的整体思路就会清晰很多。
