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

## 安卓分析过程

### 1. 从 APK 结构入手

APK 本质上是 ZIP，可以先查看目录：

```bash
unzip -l galactic-bin.apk
```

如果校验逻辑位于 Kotlin/Java 层，优先用 JADX 打开 `classes.dex`；如果入口只负责调用 native 方法，再继续检查 `lib/` 目录下的 `.so` 文件。`assets/` 中的文本文件通常不是普通资源，而是题目作者故意留下的 key、排列或校验常量。

### 2. 找到真正的校验点

在 JADX 中搜索 `check`、`Base64`、`sorting_order` 和成功提示，沿调用关系进入 `BinVerifier.check()`。伪代码可概括为：

```kotlin
fun check(input: String): Boolean {
    if (!input.startsWith("flag{") || !input.endsWith("}")) return false
    val body = input.substring(5, input.length - 1)
    if (body.length != 24) return false

    val shuffled = ByteArray(24)
    for (i in order.indices) shuffled[i] = body[order[i]].code.toByte()
    for (i in shuffled.indices) shuffled[i] =
        (shuffled[i].toInt() xor mask[i % mask.size].code).toByte()

    return Base64.encodeToString(shuffled, NO_WRAP) == target
}
```

这里最容易误判的是 Base64。它只是把二进制比较值变成可打印字符串，不是额外的加密层；先解码目标值，才能进行后续异或。

### 3. 置换方向

正向代码使用 `shuffled[i] = body[order[i]]`，因此恢复时要执行 `body[order[i]] = shuffled[i]`。如果直接写成 `body[i] = shuffled[order[i]]`，只有排列恰好是自逆置换时才会正确。本题的 `order` 不是自逆置换，所以必须按赋值语句逐字翻译。

### 4. 长度与字符集

Java/Kotlin 的 `String.length` 以 UTF-16 code unit 计数，而题目限制的是可打印 ASCII。脚本中使用 `bytes` 后，所有元素都应位于 `0x20` 到 `0x7e` 范围内。恢复出的中间结果为：

```text
trashcan_l0ves_sorting!!
```

## 考点总结

这道题综合了 APK 解包、JADX 调用图定位、assets 资源读取、Base64 解码、循环异或和排列逆运算。关键是不要把每一层都当成加密：先区分编码、置换和异或，再按照实际执行顺序逐层还原。

## Flag

```text
flag{trashcan_l0ves_sorting!!}
```
