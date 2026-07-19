---
title: "re学习日志6"
date: 2026-07-19
categories: ["逆向工程"]
tags: ["学习笔记"]
description: 记录re的学习过程。
cover: /img/1760094194226.webp
toc: true  
comments: true  

---

# re学习日志6

## X0r

### 1.查看程序信息

![exeinfo](imgs/20267191.png)


### **2.使用ida进行初步分析**

这里将题目附件用ida打开查看，反编译后先看主函数

![ida](imgs/20267192.png)


大概逻辑是对输入做两次xor操作



### 3.分析两次xor操作的具体逻辑

![ida](imgs/20267193.png)

这里可以得出两次xor的具体操作：

第一次：每三位一个循环，每位都被不同的常量异或一次。

三个常量分别为0x11u，0x45u，0x14u;

第二次则以19，19，81循环异或

这两次每次都进行八轮循环。



密文为：

![ida](imgs/20267194.png)

也就是说flag经过两轮异或得到"anu`ym7wKLl$P]v3q%D]lHpi"，而我们要逆向进行这个操作。



### 4.解密脚本

```python

encrypted = "anu`ym7wKLl$P]v3q%D]lHpi"
flag = ""

for i in range(len(encrypted)):
    value = ord(encrypted[i])
   if i % 3 == 0:
        value ^=19
    elif i % 3 == 1:
        value ^=19
    else:
        value ^=81
    if i % 3 == 0:
        value ^= 0x14
    elif i % 3 == 1:
        value ^= 0x11
    else:
        value ^= 0x45
    flag += chr(value)

print("flag：", flag)
```

**最终flag为：flag{y0u_Kn0W_b4s1C_xOr}**

### 
