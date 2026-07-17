---
title: "re学习日志5"
date: 2026-07-17
categories: ["逆向工程"]
tags: ["学习笔记"]
description: 记录re的学习过程。
cover: /img/1755681953134.png
toc: true  
comments: true  

---

# re学习日记5

## Strange Base

### 1.查看程序信息

我们这里拿到可执行文件先用Exeinfo查看类型和是否有壳等信息。


![exeinfo](imgs/20267171.png)


### 2.使用ida进行初步分析

这里将题目附件用ida打开查看，反编译后先看主函数，

![ida](imgs/20267172.png)

这里可以发现关键字符串"T>6uTqOatL39aP!YIqruyv(YBA!8y7ouCa9="和关键函数”base64_encode“

猜测分别为目标字符串和加密函数。



再进行进一步分析，我们发现程序的逻辑如下：

1.输入字符串作为input

2.经过复制与计算长度等操作后传入函数”base64_encode“

3.将其执结果与字符串"T>6uTqOatL39aP!YIqruyv(YBA!8y7ouCa9="作比较，如果相同则输出flag

接下来查看关键函数”base64_encode“



### 3.寻找特殊加密表

函数加密本身没有什么区别问题在于base64的加密表

我们将汇编窗口与非汇编窗口同步显示，最终定位到加密表：

![ida](imgs/20267173.png)


db 'HElLo!A=CrQzy-B4S3|is',27h,'waITt1ng&Y0u^{/(>v<)*}GO~256789pPqWXV'
                db 'KJNMF',0 

将这两段拼在一起：

所以该base64加密对应的加密表为HElLo!A=CrQzy-B4S3|is'waITt1ng&Y0u^{/(>v<)*}GO~256789pPqWXVKJNMF'

接下来，我们只要对字符串"T>6uTqOatL39aP!YIqruyv(YBA!8y7ouCa9="进行对应的base64解码即可得到flag。

```python
import base64

custom_table = "HElLo!A=CrQzy-B4S3|is'waITt1ng&Y0u^{/(>v<)*}GO~256789pPqWXVKJNMF"

standard_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

cipher = "T>6uTqOatL39aP!YIqruyv(YBA!8y7ouCa9="

translate_table = str.maketrans( custom_table, standard_table)

normal_base64 = cipher.translate(translate_table)

print(normal_base64)

result = base64.b64decode(normal_base64)

print(result.decode())
```












