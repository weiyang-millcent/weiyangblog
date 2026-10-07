---
title: "galactic-bin 安卓逆向题解"
date: 2026-10-06
categories: ["逆向工程"]
tags: ["Android", "APK", "Kotlin", "学习笔记"]
description: "反编译 APK，恢复 sorting_order、sorting_mask 和 Base64 校验流程。"
cover: /imgs/20261007-cover-galactic.jpg
password: "8888"
abstract: 本文已设置访问密码，请输入密码后查看完整题解。
message: 请输入访问密码后查看本文。
wrong_pass_message: 密码错误，请重新输入。
toc: true
comments: true
---

# galactic-bin 安卓逆向题解

## 定位校验逻辑

使用 JADX 打开 APK，在 `BinVerifier.check()` 中找到输入校验。输入要求长度为 30，格式为 `flag{...}`，花括号内部 24 字节，并且只能包含可打印 ASCII。

程序从 assets 读取两个关键文件：

```text
sorting_order.txt: 5 12 19 2 9 16 23 6 13 20 3 10 17 0 7 14 21 4 11 18 1 8 15 22
sorting_mask.txt:  B1nKey
```

校验过程是先按 `order` 置换 24 字节，再与循环使用的 `B1nKey` 异或，最后 Base64 编码并和常量比较：

```text
IVQHKgkWY1AdJRZJMEUAFAIRNEUcFBZY
```

## 逆向恢复

先 Base64 解码目标值，再异或 mask，得到置换后的明文；随后根据 `order` 的逆置换还原原始顺序：

```python
target = base64.b64decode("IVQHKgkWY1AdJRZJMEUAFAIRNEUcFBZY")
mixed = bytes(x ^ b"B1nKey"[i % 6] for i, x in enumerate(target))
plain = bytearray(24)
for i, pos in enumerate(order):
    plain[pos] = mixed[i]
```

## Flag

```text
flag{trashcan_l0ves_sorting!!}
```
