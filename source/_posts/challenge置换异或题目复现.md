---
title: "challenge 置换异或题目复现"
date: 2026-10-06
categories: ["逆向工程"]
tags: ["IDA", "异或", "置换", "学习笔记"]
description: "从伪随机密钥生成、循环移位和索引置换中逆推出 Flag。"
cover: /imgs/20261007-cover-permutation.jpg
password: "8888"
abstract: 本文已设置访问密码，请输入密码后查看完整题解。
message: 请输入访问密码后查看本文。
wrong_pass_message: 密码错误，请重新输入。
toc: true
comments: true
---

# challenge 置换异或题目复现

## 校验流程

程序对 24 字节输入执行三步变换：先用生成的 24 字节 key 异或并循环左移一位，再按 `(i + 19) % 24` 做置换，最后和常量密文比较。生成 key 的种子来自 32 字节表的混合，并经过 32 位移位异或扩散。

## 逆向顺序

异或和循环移位都可逆。由于正向输出是 `tmp[(i + 19) % 24]`，解密时先按相反索引取回，再对每个字节执行循环右移一位并异或 key。脚本验证结果为：

```text
middle = m4nb0_cat_runs_at_boot!!
```

## 伪代码拆解

程序的正向过程可以写成：

```python
for i in range(24):
    x = input[i]
    x ^= key[i]
    x = rol8(x, 1)
    middle[i] = x

for i in range(24):
    cipher[(i + 19) % 24] = middle[i]
```

其中 `rol8` 是 8 位循环左移。它和普通左移的区别是不会丢失最高位，因此逆操作必须使用循环右移，而不是简单的 `>> 1`。

### 生成 key 的注意事项

IDA 中的 key 生成函数通常会把一个 32 位状态变量拆成多个阶段处理：先与固定表异或，再减去 TEA 风格常量，之后通过左移和右移混合，最后取低 8 位作为 key 字节。Python 实现时必须模拟 C 的 32 位无符号溢出：

```python
MASK32 = 0xffffffff
state = seed
for i in range(32):
    value = state ^ (table1[i] << (8 * (i & 3)))
    state = (value + table2[31 - i] - 1640531527) & MASK32

key = bytearray()
for j in range(24):
    state = (((state << 13) ^ state) >> 17) ^ (state << 13) ^ state
    state = ((state << 5) ^ state) & MASK32
    key.append(state & 0xff)
```

### 解密时为什么要反着做

正向置换是 `cipher[(i + 19) % 24] = middle[i]`，所以解密时应执行：

```python
for i in range(24):
    middle[i] = cipher[(i + 19) % 24]

for i in range(24):
    x = ror8(middle[i], 1)
    plain[i] = x ^ key[i]
```

异或的逆仍然是异或，循环左移的逆是循环右移，置换的逆则是按照正向索引重新取值。三个逆操作必须按照相反顺序执行。

## 验证方法

恢复后不要只看字符串是否像 Flag，还要把结果重新送回程序。若校验失败，重点检查置换方向、循环移位是否限制在 8 位，以及 key 生成过程中是否遗漏 `& 0xffffffff`。

## Flag

```text
flag{m4nb0_cat_runs_at_boot!!}
```
