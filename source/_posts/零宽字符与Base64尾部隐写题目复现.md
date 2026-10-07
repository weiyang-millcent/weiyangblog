---
title: "零宽字符与 Base64 尾部隐写题目复现"
date: 2026-10-07
categories: ["misc"]
tags: ["零宽字符", "Base64", "隐写", "学习笔记"]
description: "从零宽字符和 Base64 尾部低位中恢复完整 Flag。"
cover: /imgs/20261007-cover-zero-width.png
password: "8888"
abstract: 本文已设置访问密码，请输入密码后查看完整题解。
message: 请输入访问密码后查看本文。
wrong_pass_message: 密码错误，请重新输入。
toc: true
comments: true
---

# 零宽字符与 Base64 尾部隐写题目复现

## 题目一：零宽字符隐写

附件表面上是一段段 Base64 字符串，但其中夹杂了肉眼不可见的 Unicode 字符。

使用 `detect_zero_width.py` 检测得到 `U+200B` 和 `U+200D`，将它们分别映射为 `0` 和 `1`，提取全部零宽字符，按 8 位分组并按 UTF-16BE 解码：

```python
raw = open("attachment_28aebf.txt", encoding="utf-8").read()
bits = "".join("0" if ch == "\u200b" else "1"
                for ch in raw if ch in "\u200b\u200d")
data = bytes(int(bits[i:i + 8], 2) for i in range(0, len(bits), 8))
print(data.decode("utf-16-be"))
```

得到：

```text
part1:flag{wakuwaKu_yoU_know-z3ro_width_chaRact3r_stegaNography_
```

## 题目二：Base64 尾部隐写

提示中的“等号的尾巴尖上”指向每行 `==` 前的最后一个 Base64 字符。查出该字符在 Base64 表中的索引，取低 4 位；每两个 4 位数据组成一个字节：

```python
alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
nibbles = []

for line in open("attachment_28aebf.txt", encoding="utf-8"):
    visible = "".join(ch for ch in line if ch in alphabet + "=")
    last = visible.rstrip("=")[-1]
    nibbles.append(alphabet.index(last) & 0xF)

result = bytes((nibbles[i] << 4) | nibbles[i + 1]
               for i in range(0, len(nibbles) - 1, 2))
print(result.decode())
```

输出：

```text
part2:and_b64_paddiNg_stego}
```

### 如何确认字符类型

不要直接凭肉眼复制或删除不可见字符。可以使用 Python 的 Unicode 类别扫描：

```python
import unicodedata
from collections import Counter

text = open("attachment_28aebf.txt", encoding="utf-8").read()
hidden = [c for c in text if unicodedata.category(c) == "Cf"]
print(Counter(f"U+{ord(c):04X}" for c in hidden))
```

本题统计到 `U+200B` 和 `U+200D`。零宽字符序列本身不能直接按 ASCII 解码：输出中每个可见字符前都有一个 `0` 字节，因此应该按 UTF-16BE 解释，而不是按 UTF-8 解释。

另外，复制内容时要从原始文本文件完整复制。某些网页、聊天软件或富文本编辑器会自动过滤零宽字符，导致数量减少，脚本得到的比特流也会错位。

### 为什么取 Base64 低 4 位

每行都以 `==` 结尾，说明这行 Base64 编码的最后一个量子只携带一个原始字节。最后一个 Base64 字符的高位由正常 Base64 数据决定，而低 4 位有可利用的冗余空间。作者没有改变可见的日文内容，只修改了这些低位，所以逐行查看最后字符本身通常看不出异常。

例如 Base64 表中某字符的索引为 `0x2d`，取低 4 位就是 `0xd`。两个 nibble 拼成一个字节：

```python
high = nibbles[i]
low = nibbles[i + 1]
byte = (high << 4) | low
```

若输出出现大量不可打印字符，通常是把整个 Base64 索引当成 6 位数据，或者误取了倒数第二个字符。

## 完整流程总结

第一层利用零宽字符传输 UTF-16BE 文本，第二层利用 Base64 尾部低位补充后半段。两层都不需要破解 Base64 本身；Base64 只是载体，真正的隐藏数据在字符间隙和编码冗余位中。

## 最终 Flag

```text
flag{wakuwaKu_yoU_know-z3ro_width_chaRact3r_stegaNography_and_b64_paddiNg_stego}
```
