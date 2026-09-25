---
title: "Emoji多层编码题目复现"
date: 2026-09-25
categories: ["misc"]
tags: ["学习笔记"]
description: 记录一道Emoji多层编码题的复现过程，通过Base100、Base64、Base58和Base32逐层解码得到Flag。
cover: /imgs/20260925-emoji-cover.png
password: "8888"
abstract: 本文已设置访问密码，请输入密码后查看完整题解。
message: 请输入访问密码后查看本文。
wrong_pass_message: 密码错误，请重新输入。
toc: true
comments: true
---

# Emoji多层编码题目复现

## 前言

这道题给出的附件是一个名为 `flag2.txt` 的文本文件。

使用文本编辑器打开后，发现文件中没有普通的字母或数字，而是由一长串连续的 Emoji 组成：

```text
👄👱👞👱👑👢🐨👤👍👥👚🐫👛👏🐹👫……
```

这种没有正常语义、由大量 Emoji 连续组成的文本，很容易让人联想到 Base100 编码。

本题的完整解码路线如下：

```text
Emoji文本
   ↓ Base100
Base64字符串
   ↓ Base64
Base58字符串
   ↓ Base58
Base32字符串
   ↓ Base32
Flag
```

---

## 一、查看题目附件

`flag2.txt` 本质上仍然是一个普通的文本文件，可以使用以下软件打开：

```text
Windows记事本
Notepad++
Visual Studio Code
Sublime Text
```

如果打开后出现方框、问号或乱码，需要将文本编码切换为：

```text
UTF-8
```

Emoji 属于 Unicode 字符，使用不正确的文本编码打开时可能无法正常显示。

---

## 二、识别Base100编码

Base100 是一种使用 Emoji 表示数据的编码方式。它会把原始字节转换成特定 Unicode 区间中的 Emoji，因此编码结果通常具有以下特征：

1. 文本由连续的 Emoji 组成；
2. Emoji 本身无法构成有意义的句子；
3. 文件实际仍然是 UTF-8 文本；
4. 每个 Emoji 对应一个原始字节。

本题的内容完全符合这些特征，因此首先尝试 Base100 解码。

将 `flag2.txt` 中的全部 Emoji 复制到支持 Base100 的解码工具中，选择解码：

![Base100解码结果](/imgs/20260925-emoji-base100.png)

得到：

```text
MzgzZk1mVnc4dXBtbWJ3ejZGc2JkcnJhYUFQN1R4cXVlNXZ6YnFZMkJSUjVRODloWWtNeHRoQ05kVHZ4b2s4b3hVS0dVeEpGUGRiVEo=
```

---

## 三、识别Base64编码

观察第一层的输出，可以发现它具有以下特点：

```text
只包含大小写字母、数字
末尾存在等号“=”
字符串长度是4的倍数
```

这些都是 Base64 的典型特征。

Base64 使用的字符范围通常是：

```text
A-Z  a-z  0-9  +  /
```

末尾的 `=` 是补位字符，所以接下来进行 Base64 解码。

![Base64解码结果](/imgs/20260925-emoji-base64.png)

得到：

```text
383fMfVw8upmmbwz6FsbdrraaAP7Txque5vzbqY2BRR5Q89hYkMxthCNdTvxok8oxUKGUxJFPdbTJ
```

这一层的结果仍然不是 Flag，说明还需要继续判断编码类型。

---

## 四、识别Base58编码

第二层结果是一串由大小写字母和数字组成的文本，但没有 Base64 常见的 `+`、`/` 和 `=`。

仔细观察还会发现，它避开了几个容易混淆的字符：

```text
0  O  I  l
```

这正是 Base58 字符表的典型特征。Base58 为了方便人工抄写，主动删除了这些容易看错的字符。

Base58 使用的字符表为：

```text
123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz
```

因此对上一层结果进行 Base58 解码：

![Base58解码结果](/imgs/20260925-emoji-base58.png)

得到：

```text
NVXWKY3UMZ5UK3JQNIYV6MJVL5ZTAX3DOV2DGXZSGMZTGMZTGN6Q====
```

---

## 五、识别Base32编码

第三层结果具有非常明显的 Base32 特征：

```text
只包含大写字母A-Z
只出现数字2-7
末尾使用等号补位
```

Base32 的标准字符表为：

```text
ABCDEFGHIJKLMNOPQRSTUVWXYZ234567
```

本题得到的字符串完全符合这个范围，因此最后进行 Base32 解码。

![Base32解码得到Flag](/imgs/20260925-emoji-base32.png)

得到：

```text
moectf{Em0j1_15_s0_cut3_2333333}
```

---

## 六、Flag

最终 Flag 为：

```text
moectf{Em0j1_15_s0_cut3_2333333}
```

其中使用了 Leetspeak 替换：

```text
Em0j1 → Emoji
15    → is
s0    → so
cut3  → cute
```

整体内容可以读作：

```text
Emoji is so cute 2333333
```

---

## 七、总结

这道题本身没有使用复杂的加密算法，重点是根据每一层输出的字符特征判断下一种编码。

完整过程如下：

```text
大量连续Emoji
→ Base100

大小写字母、数字，末尾有“=”
→ Base64

避开0、O、I、l等易混淆字符
→ Base58

大写字母、数字2-7，末尾有“=”
→ Base32
```

在处理多层编码题时，不应该只进行一次解码。每完成一层后，都需要继续观察新结果的：

```text
字符范围
字符串长度
补位符号
是否存在固定前缀
是否已经出现Flag格式
```

只要熟悉常见 Base 编码的字符特征，就可以按照结果逐层判断，直到得到最终 Flag。
