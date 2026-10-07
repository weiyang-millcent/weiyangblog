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

## Flag

```text
flag{foRgeR_fAmily_PHOtO_hiDES_53cre7s_cOlOr_P1ANe5}
```

## 考点

本题考查 PNG 通道拆分、LSB 隐写识别以及 `zsteg` 输出筛选。大量重复的 `U`、`o` 等结果通常是错误通道或错误位序，真正有意义的结果应当同时满足可读文本和 Flag 格式。
