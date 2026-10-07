---
title: "LSB 彩色平面隐写题目复现"
date: 2026-10-06
categories: ["misc"]
tags: ["LSB", "隐写", "zsteg", "学习笔记"]
description: "从 PNG 的 Alpha、Red 和 Blue 平面最低有效位中恢复分段 Flag。"
cover: /imgs/20261007-cover-lsb.png
password: "8888"
abstract: 本文已设置访问密码，请输入密码后查看完整题解。
message: 请输入访问密码后查看本文。
wrong_pass_message: 密码错误，请重新输入。
toc: true
comments: true
---

# LSB 彩色平面隐写题目复现

## 分析思路

题目提示“四个维度缺一不可”，说明不能只查看 RGB 三个通道，还要检查 Alpha 通道和最低有效位。使用 `zsteg` 扫描 PNG：

```bash
zsteg -a audio.png
```

重点查看 `b1,*,lsb` 的文本结果，并分别打开 Alpha、Red、Blue 的 bit plane。

## 分段结果

三个平面得到的内容分别是：

```text
Alpha: flag{foRgeR_fAmily_
Red:   PHOtO_hiDES_53cre7s_
Blue:  cOlOr_P1ANe5}
```

按图片中的通道顺序拼接即可。大小写保持原样，不要在中间添加空格。

## 具体操作

### 1. 查看图片基本信息

先确认文件确实是 PNG，并检查是否带有 Alpha 通道：

```bash
file audio.png
identify -verbose audio.png
```

如果图片模式是 RGBA，那么每个像素有四个独立分量：Red、Green、Blue、Alpha。题目提示“四个维度缺一不可”，说明不能只用普通 RGB 图片隐写的思路。

### 2. 使用 zsteg 筛选结果

```bash
zsteg -a audio.png
```

重点关注 `b1`、`lsb`、`xy` 和 `alpha` 相关结果。真正的文本通常会同时满足三个条件：输出能读、包含 `flag` 或下划线结构、长度与其他片段能够拼接。诸如连续的 `U`、`o`、`%`，往往是把错误的通道或错误位序当作 ASCII 后产生的假阳性。

也可以分别导出 bit plane 进行确认：

```bash
zsteg -E b1,r,lsb,xy audio.png > red.bin
zsteg -E b1,g,lsb,xy audio.png > green.bin
zsteg -E b1,b,lsb,xy audio.png > blue.bin
```

### 3. 为什么大小写不能修改

隐写得到的是字节流，程序或平台比较 Flag 时通常按字符串逐字节比较。因此 `foRgeR`、`PHOtO`、`cOlOr` 中的大写字母都是数据，不是排版。把它们统一改成小写会导致校验失败。

最终三段分别对应 Alpha、Red、Blue 平面，按题目给出的通道顺序拼接，不要根据视觉上出现的先后顺序自行调整。

## 常见误区

- 只扫描 RGB，遗漏 Alpha 平面；
- 看到一段可打印字符就直接认定为答案；
- 把 LSB 和 MSB 的结果混在一起；
- 使用 `XY` 与 `xy` 的错误扫描顺序；
- 拼接时修改题目刻意保留的大小写。

## Flag

```text
flag{foRgeR_fAmily_PHOtO_hiDES_53cre7s_cOlOr_P1ANe5}
```

## 考点

本题考查 PNG 通道拆分、LSB 隐写识别以及 `zsteg` 输出筛选。大量重复的 `U`、`o` 等结果通常是错误通道或错误位序，真正有意义的结果应当同时满足可读文本和 Flag 格式。
