---
title: "misc出题日志1"
date: 2026-07-27
categories: ["misc"]
tags: ["学习笔记"]
description: 记录misc的学习过程。
cover: /img/777.jpg
toc: true  
comments: true  

---

# misc出题日志1

自己出了道题。

![](imgs/20267271.png)

本题将flag分为三部分，我们一步步来。

![](imgs/20267272.png)

第一部分是一个图片的steghide隐写，没有设置密码，解密得到：

![](imgs/20267273.png)

再base64解密得到：

### **flag{L1v3_**

第二部分是音频的隐写，我们将其拖入Audacity并将视窗切换至频谱图：

![](imgs/20267274.png)

得到flag的第二部分。



第三部分是一个snow隐写，我们先来找密码：

![](imgs/20267275.png)

根据提示可知这是一个w型的四栏栅栏密码：

![](imgs/20267276.png)

再进行snow隐写解密：

![](imgs/20267277.png)

拼凑出最终flag:

## **<mark>flag{L1v3_Th1s_L1f3_S3ri0usly}</mark>**


