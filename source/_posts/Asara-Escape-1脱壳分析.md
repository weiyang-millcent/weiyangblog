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

## 详细操作记录

### 1. 判断是否为 UPX

先用 `strings`、Detect It Easy 或 `upx -t` 检查文件。若能看到 UPX 相关节名，或者入口点位于一段明显的解压循环中，就不要直接在壳代码里寻找输入校验。

常见的脱壳流程是：

```text
upx -t asara_escape_1.exe
upx -d asara_escape_1.exe -o asara_escape_1_unpacked.exe
```

如果 UPX 被修改过导致命令失败，就在调试器中运行到 OEP，转储内存映像并修复导入表。重新载入 IDA 后，应该能看到 `Asara Escape 1: unpack the box first.` 等正常字符串。

### 2. 从主函数追踪参数

主函数的伪代码可以整理成：

```c
print("evac credential> ");
fgets(Buffer, 160, stdin);
state = sub_1400014FF(Buffer);
if (sub_14000167F(Buffer, state))
    puts("evac request accepted");
else
    puts("not accepted");
```

`sub_14000167F` 先调用 `sub_14000153C` 检查前置状态，成功后把 `Buffer + 5` 传给 `sub_1400015A8`。这里的 `+5` 很重要：输入前五个字符通常是固定前缀，真正参与 24 字节变换的是后面的 credential。

### 3. 还原中间变换

`sub_1400015A8` 的循环可以逐项对应：

```c
for (i = 0; i < 24; i++) {
    index = (3 + 5 * i) % 24;
    table_index = i % 6;
    out[i] = i + (byte_140004000[table_index] ^ input[index]);
}
```

逆向时要先减去 `i`，再与表异或，最后根据 `index` 把结果写回原位置：

```python
input_data = bytearray(24)
for i, value in enumerate(target):
    index = (3 + 5 * i) % 24
    input_data[index] = (value - i) ^ table[i % 6]
```

如果结果不对，应优先检查输入是否包含换行、`a1 + 5` 是否理解正确，以及 C 语言无符号字节运算是否需要 `& 0xff`。

## 脱壳题的通用经验

先恢复 OEP，再定位用户输入和成功字符串，最后沿参数流进入校验函数。不要在 UPX 解压循环中浪费时间，也不要把所有常量都当作密文；很多常量只是表、索引或比较目标的一部分。
