---
title: "misc学习日志1"
date: 2026-07-24
categories: ["misc"]
tags: ["学习笔记"]
description: 记录misc的学习过程。
cover: /img/789.png
toc: true  
comments: true  

---

# **misc学习日志1**

## 前有文字，所以搜索很有用

### Track 1

![](imgs/20267241.png)


根据提示判断本体考察零宽字符隐写，我们先选择合适的文本编辑器打开文件，确保零宽字节能正确被复制：

![](imgs/20267242.png)


接下来进行零宽字节隐写解密，先要根据文本提示选择正确配置：

![](imgs/20267243.png)


揭秘结果为：

![](imgs/20267244.png)


经过base64解码得到第一部分：

![](imgs/20267245.png)


### Track 2

![](imgs/20267246.png)


根据提示其中的txt文件需要进行brainfuck解密：

![](imgs/20267247.png)


根据咏雪这一提示可以判断出下一个doc文件用了snow隐写，我们像将其内容复制成txt文件，再进行解密操作：

![](imgs/20267248.png)


将得到的莫斯密码再进行解密得到的二部分：


![](imgs/20267249.png)



### Track 3

本题需要对出现词数进行统计排序：


通过工具得到第三部分：

`cH@1LenG3s}`



将三部分连接得到最终flag:



<mark>**flag{you_0V3RC4ME_cH@1LenG3s}**</mark>








