---
title: "Asara Escape 1 脱壳分析"
date: 2026-10-06
categories: ["逆向工程"]
tags: ["UPX", "IDA", "脱壳", "学习笔记"]
description: "记录 Asara Escape 1 的 UPX 脱壳和输入校验函数定位过程。"
cover: /imgs/20261007-cover-asara.png
password: "8888"
abstract: 本文已设置访问密码，请输入密码后查看完整题解。
message: 请输入访问密码后查看本文。
wrong_pass_message: 密码错误，请重新输入。
toc: true
comments: true
---

# Asara Escape 1 脱壳分析

## 脱壳

程序启动时提示 `unpack the box first`，说明入口处仍是 UPX 壳。先运行 UPX 脱壳，或在 OEP 处转储进程并修复导入表，再将脱壳文件重新载入 IDA。

## 校验函数

主函数读取 credential 后依次调用：

```text
sub_1400014FF：处理输入并返回状态
sub_14000153C：检查环境/前置条件
sub_1400015A8：生成 24 字节中间结果
sub_140001629：比较最终结果
```

`sub_1400015A8` 的核心循环从输入偏移 `a1 + 5` 取字节，并使用 `byte_140004000` 进行异或和位置索引混合：

```c
for (i = 0; i < 24; i++) {
    v6 = (3 + 5 * i) % 24;
    v7 = i % 6;
    out[i] = i + (table[v7] ^ input[v6]);
}
```

后续只需从 `sub_140001629` 提取 24 字节目标常量，再按相反顺序还原输入。当前记录保留了完整的脱壳和函数定位过程；比较函数的目标常量尚未从样本中确认，因此不在文章中臆造 Flag。
