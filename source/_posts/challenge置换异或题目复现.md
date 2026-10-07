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

## Flag

```text
flag{m4nb0_cat_runs_at_boot!!}
```
