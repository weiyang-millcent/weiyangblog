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

## 最终 Flag

```text
flag{wakuwaKu_yoU_know-z3ro_width_chaRact3r_stegaNography_and_b64_paddiNg_stego}
```
