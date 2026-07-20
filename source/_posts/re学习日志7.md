---
title: "re学习日志7"
date: 2026-07-20
categories: ["逆向工程"]
tags: ["学习笔记"]
description: 记录re的学习过程。
cover: /img/4.jpg
toc: true  
comments: true  

---

# re学习日志7

## 1.检查文件信息


![exeinfo](imgs/20267201.png)



## 2.使用ida进行初步分析

这里使用ida64先看一下：


![ida](imgs/20267202.png)



反汇编就直接看到了提示

大致是说：

1.需要动态调试出flag

2.flag在函数x0r()中



## 3.动态调试x0r()函数

我们在函数入口处设下断点：


![ida](imgs/20267203.png)



多次让函数循环得到flag:

**flag{It3_D3bugG_T11me!_le3_play}**








