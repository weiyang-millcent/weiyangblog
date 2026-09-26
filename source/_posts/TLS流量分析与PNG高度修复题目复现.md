---
title: "TLS流量分析与PNG高度修复题目复现"
date: 2026-09-26
categories: ["misc"]
tags: ["流量分析", "Wireshark", "TLS", "ZIP弱口令", "PNG"]
description: 从流量包中提取TLS密钥日志并解密HTTPS，导出加密ZIP、爆破四位弱口令，最后修复PNG高度得到Flag。
cover: /imgs/20260926-tls-zip-cover.jpg
password: "8888"
abstract: 本文已设置访问密码，请输入密码后查看完整题解。
message: 请输入访问密码后查看本文。
wrong_pass_message: 密码错误，请重新输入。
toc: true
comments: true
---

# TLS流量分析与PNG高度修复题目复现

## 前言

题目给出一个 `capture.pcapng` 流量包，以及一段看似语无伦次的提示：

```text
xing：阿巴阿巴，要来点特仑苏牛奶吗。歪四比位巴密码。
大狗嚼，嚼图片，不好吃，吐了 ~~被哄睡了~~

CloverDay'ssss：他在说什么？总不能是提示吧……
```

实际上，这段话给出了几个关键方向：

- “特仑苏”谐音指向 `TLS`；
- “四位……密码”提示压缩包使用四位弱口令；
- “图片不好吃，吐了”提示最后的图片文件存在结构异常；
- “被哄睡了”对应修复后看到的睡觉小猫。

完整解题流程为：

```text
分析流量包
  ↓
提取 tls.log
  ↓
配置 Wireshark 解密 TLS
  ↓
导出加密的 ZIP 文件
  ↓
爆破四位弱口令 #9eA
  ↓
解压得到 cat.png
  ↓
分析并修复 PNG 的 IHDR 高度
  ↓
显示隐藏在底部的 Flag
```

---

## 一、初步分析流量包

使用 Wireshark 打开 `capture.pcapng`，先观察“协议”一列，并尝试使用以下过滤器：

```wireshark
http || tls
```

可以发现，绝大多数通信都被 TLS 加密，无法直接看到 HTTP 请求和响应。不过，在流量末尾还能找到少量明文 HTTP 数据，其中包含对 `tls.log` 的请求。

![Wireshark中发现对tls.log的明文HTTP请求](/imgs/20260926-tls-log-http-request.png)

截图中的第89帧为：

```http
GET /tls.log HTTP/1.1
```

紧接着的第90帧返回 `HTTP/1.0 200 OK`，因此可以判断服务器确实将 `tls.log` 作为HTTP对象传输了出来。

`tls.log` 是 TLS 会话密钥日志，记录了用于解密 TLS 连接的流量密钥。只要日志与抓包中的连接对应，就可以让 Wireshark还原加密后的应用层内容。

---

## 二、从流量中导出 tls.log

在 Wireshark 中依次选择：

```text
文件（File）
  → 导出对象（Export Objects）
  → HTTP
```

在对象列表中找到 `tls.log`，选中后保存到本地。

![从流量中导出的tls.log](/imgs/20260926-tls-zip-tls-log.png)

也可以使用 Tshark 导出 HTTP 对象：

```powershell
tshark -r capture.pcapng --export-objects http,.
```

执行后，在当前目录中检查导出的文件，并找到 `tls.log`。

---

## 三、使用密钥日志解密 TLS

在 Wireshark 中打开：

```text
编辑（Edit）
  → 首选项（Preferences）
  → Protocols
  → TLS
```

找到：

```text
(Pre)-Master-Secret log filename
```

将刚才导出的 `tls.log` 文件路径填入，然后确认并重新载入流量包。

此时再次使用过滤器：

```wireshark
http
```

原本被 TLS 加密的 HTTP 内容已经可以正常解析，并能看到如下请求：

```http
GET /flag.zip HTTP/1.1
```

如果配置密钥日志后依然不能正常解析，可能是抓包中存在乱序数据包。可以先使用 Wireshark 自带的 `reordercap` 按时间戳重新排列：

```powershell
reordercap capture.pcapng capture_ordered.pcapng
```

然后重新打开 `capture_ordered.pcapng` 并配置同一个 `tls.log`。

---

## 四、导出加密的 ZIP 文件

TLS 解密成功后，再次选择：

```text
文件（File）
  → 导出对象（Export Objects）
  → HTTP
```

找到类型为 ZIP、请求路径为 `/flag.zip` 的对象并保存。

![从解密流量中导出的ZIP文件](/imgs/20260926-tls-zip-archive.png)

尝试解压时会被要求输入密码，说明下一步需要进行 ZIP 弱口令爆破。

---

## 五、爆破四位 ZIP 弱口令

题目中的“四位……密码”已经提示了密码长度，因此不需要无范围地爆破。设置以下条件即可：

```text
密码长度：4
字符集：数字、小写字母、大写字母、特殊字符
```

使用压缩包密码恢复工具进行穷举，最终得到：

```text
#9eA
```

这里的 `#` 是密码的一部分，完整密码一共四个字符，输入时不能省略。

如果使用 John the Ripper，也可以先提取 ZIP 哈希，再限定四位 ASCII 字符爆破：

```powershell
zip2john flag.zip > zip.hash
john --incremental=ASCII --min-length=4 --max-length=4 zip.hash
john --show zip.hash
```

使用密码 `#9eA` 解压后得到：

```text
cat.png
```

---

## 六、分析无法正常打开的 cat.png

直接打开 `cat.png` 时，部分图片查看器会提示文件无效。检查文件头可以确认它仍然具有正常的 PNG 签名：

```text
89 50 4E 47 0D 0A 1A 0A
```

接下来的 `IHDR` 块内容为：

```text
00 00 00 0D 49 48 44 52
00 00 06 A4 00 00 04 00
08 06 00 00 00 21 B5 33 5D
```

其中：

```text
00 00 06 A4 = 宽度 1700
00 00 04 00 = 高度 1024
08             = 每个通道8位
06             = RGBA颜色类型
```

使用 `pngcheck` 检查时，会发现 `IHDR` 的 CRC 校验异常。进一步分析解压后的图像数据，还能发现正常的 1024 行之后存在额外像素数据。

---

## 七、计算图片的真实高度

图片宽度是 1700，颜色类型是 RGBA，因此每个像素需要4字节：

```text
R + G + B + A = 4字节
```

PNG 每行像素前还有一个过滤器字节，所以每行解压后的长度为：

```text
1700 × 4 + 1 = 6801字节
```

图像数据中多出的内容共有：

```text
1741056字节
```

用额外数据长度除以每行长度：

```text
1741056 ÷ 6801 = 256行
```

因此，文件中实际上还隐藏了256行，真实高度应该是：

```text
1024 + 256 = 1280
```

转换成十六进制：

```text
1024 = 0x400
1280 = 0x500
```

也就是说，出题人将高度字段从 `00 00 05 00` 篡改成了 `00 00 04 00`，但没有同步修改 CRC。原文件保存的 CRC `21 B5 33 5D`，实际上正好对应高度为1280时的 IHDR 数据。

---

## 八、使用 HxD 修复图片

使用 HxD 打开 `cat.png`，定位到文件偏移：

```text
0x16
```

将该位置的：

```text
04
```

修改为：

```text
05
```

高度字段就会从：

```text
00 00 04 00
```

恢复为：

```text
00 00 05 00
```

保存为新的 PNG 文件即可。由于原文件中的 CRC 本来就是按照真实高度计算的，因此这里不需要再修改 CRC。

也可以在图片目录中使用 PowerShell 一键生成修复副本：

```powershell
$data = [IO.File]::ReadAllBytes(".\cat.png")
$data[22] = 0x05
[IO.File]::WriteAllBytes(".\cat_fixed.png", $data)
```

数组下标22对应十六进制文件偏移 `0x16`。

---

## 九、得到 Flag

打开修复后的图片，可以在原图底部看到被隐藏的文字：

![修复高度后显示的Flag](/imgs/20260926-tls-zip-flag.png)

最终 Flag 为：

```text
moectf{5133p_ca7_5ay5_r1gh7_h1n7}
```

其中内容使用了简单的 Leet 写法：

```text
5133p → sleep
ca7   → cat
5ay5  → says
r1gh7 → right
h1n7  → hint
```

整体可以理解为：

```text
sleep cat says right hint
```

也与题面中的“被哄睡了”相呼应。

---

## 总结

这道题将多个常见 Misc 知识点串联在一起：

1. 从明文 HTTP 流量中导出 TLS 会话密钥日志；
2. 在 Wireshark 中配置 `tls.log` 解密 HTTPS；
3. 从解密后的 HTTP 流量中导出 ZIP 文件；
4. 根据题目提示限定四位密码并进行弱口令爆破；
5. 分析 PNG 的 IHDR、CRC 和解压后像素数据长度；
6. 恢复被篡改的图片高度，显示藏在底部的 Flag。

遇到“PNG 签名正常但图片无法打开”的情况时，不要只检查文件尾是否附加了内容，还应重点检查：

```text
IHDR中的宽高和颜色类型
各数据块的CRC
IDAT解压后的实际数据长度
声明尺寸与真实像素行数是否一致
```

这类题的关键不是复杂隐写算法，而是发现文件头声明与真实图像数据之间的不一致。
