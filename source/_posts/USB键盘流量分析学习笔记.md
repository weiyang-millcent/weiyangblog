---
title: "USB键盘流量分析学习笔记"
date: 2026-09-03
categories: ["misc"]
tags: ["学习笔记"]
description: 记录USB键盘流量分析的通用思路，包括设备定位、HID报告解析、按键映射和编辑操作还原。
cover: /img/20260903-usb-keyboard-cover.jpg
toc: true
comments: true
---

# USB键盘流量分析学习笔记

## 前言

在 CTF 的 Misc 和流量分析题目中，有时会得到一个由 USBPcap 抓取的 `pcap` 文件。与普通的网络流量不同，这类文件中记录的是 USB 设备与主机之间的通信，其中可能包含键盘输入、鼠标移动以及设备初始化信息。

USB 键盘流量题的核心并不是直接在数据包中搜索 Flag，而是先找到真正的键盘设备，再按照 HID 键盘报告的格式，将每一个按键码还原为字符。

如果输入过程中还使用了 Shift、Caps Lock、退格和方向键，只按数据包顺序拼接字符就会得到错误结果。因此，这类题最后还需要模拟一个简单的文本编辑器，才能恢复用户真正输入完成的内容。

## 一、确认抓包类型

首先使用 `file` 查看文件类型：

```bash
file keyboard.pcap
```

也可以使用 `capinfos` 查看抓包的基本信息：

```bash
capinfos keyboard.pcap
```

如果文件来自 USBPcap，Wireshark 打开后通常能够看到 `USB`、`USBPcap` 或 `USBHID` 等协议。经典 PCAP 文件头的前四个字节一般为：

```text
d4 c3 b2 a1
```

需要注意，这只能说明它采用小端序的经典 PCAP 格式。还要继续查看链路层类型，USBPcap 对应的 LinkType 为 `249`。

## 二、观察USB设备

一个 USB 抓包中可能同时存在多个设备，例如：

- 键盘；
- 鼠标；
- USB 集线器；
- 存储设备；
- 设备初始化时产生的控制传输。

因此，第一步不能直接提取所有 `usb.capdata`，而是要先按照总线号、设备地址和端点进行分类。

在 Wireshark 中可以先使用下面的过滤器观察存在数据的 USB 包：

```text
usb.capdata
```

部分 Wireshark 版本会把 HID 数据解析为：

```text
usbhid.data
```

然后重点观察以下字段：

```text
usb.bus_id
usb.device_address
usb.endpoint_address
usb.transfer_type
usb.data_len
```

也可以使用 TShark 导出关键信息：

```bash
tshark -r keyboard.pcap \
  -Y "usb.capdata || usbhid.data" \
  -T fields \
  -e frame.number \
  -e usb.bus_id \
  -e usb.device_address \
  -e usb.endpoint_address \
  -e usb.data_len \
  -e usb.capdata
```

不同版本的 Wireshark 字段可能略有区别。如果某个字段提示不存在，可以在数据包详情中选中对应字段，右键选择“Apply as Column”，查看当前版本实际使用的字段名。

## 三、定位键盘流量

USB 键盘一般通过中断传输周期性地向主机发送 HID 报告。常见特征如下：

```text
Transfer Type：Interrupt
Endpoint：0x81一类的IN端点
Data Length：8 bytes
```

其中，端点最高位为 `1` 时表示数据方向为设备到主机，也就是 IN 方向。

可以尝试使用下面的过滤思路：

```text
usb.transfer_type == 0x01 && usb.data_len == 8
```

然后继续按照 `usb.device_address` 区分设备。

### 如何区分键盘和鼠标

键盘和鼠标都有可能使用中断传输，并且报告长度也可能相同。因此不能只根据“8 字节”判断设备类型，还需要观察数据变化规律。

标准键盘报告通常类似：

```text
00 00 0b 00 00 00 00 00
```

按键松开时会出现：

```text
00 00 00 00 00 00 00 00
```

键盘按键码通常出现在第 3～8 字节，同一个按键按下后会保持不变，松开后再归零。

鼠标报告则通常表现为某几个字节连续小幅变化，用于记录 X、Y 方向的移动量、滚轮和按键状态。若数据不断出现 `01`、`ff` 等相对位移，而不是稳定的 HID 键码，就更可能是鼠标。

如果仍然无法确定，可以查看设备初始化阶段的 HID Report Descriptor。描述符会说明设备包含 Keyboard、Mouse 或其他 Usage Page。

## 四、理解8字节键盘报告

常见的 USB Boot Keyboard 报告结构如下：

| 字节位置 | 含义 |
| --- | --- |
| Byte 0 | 修饰键状态，如 Ctrl、Shift、Alt、GUI |
| Byte 1 | 保留字节，通常为 `00` |
| Byte 2～7 | 同时按下的最多六个普通按键 |

例如：

```text
00 00 0b 00 00 00 00 00
```

第 1 字节为 `00`，表示没有按下修饰键；第 3 字节为 `0b`，按照 HID 键码表对应字母 `h`。

如果报告为：

```text
02 00 0b 00 00 00 00 00
```

第 1 字节的值为 `02`，表示按下了左 Shift，因此结果应该是大写的 `H`。

## 五、修饰键的判断

第一个字节的每一位分别代表一个修饰键：

| 数值 | 按键 |
| --- | --- |
| `0x01` | Left Ctrl |
| `0x02` | Left Shift |
| `0x04` | Left Alt |
| `0x08` | Left GUI |
| `0x10` | Right Ctrl |
| `0x20` | Right Shift |
| `0x40` | Right Alt |
| `0x80` | Right GUI |

判断 Shift 时，可以使用：

```python
shift = bool(modifier & 0x22)
```

这里的 `0x22` 同时包含左 Shift 的 `0x02` 和右 Shift 的 `0x20`。

Shift 不仅影响字母大小写，也会影响数字和符号。例如：

```text
1 -> !
2 -> @
3 -> #
- -> _
[ -> {
] -> }
```

## 六、HID键码映射

最常见的字母键码范围为：

```text
0x04～0x1d -> a～z
```

数字键码范围为：

```text
0x1e～0x27 -> 1～0
```

一些常见控制键如下：

| HID键码 | 按键 |
| --- | --- |
| `0x28` | Enter |
| `0x29` | Esc |
| `0x2a` | Backspace |
| `0x2b` | Tab |
| `0x2c` | Space |
| `0x39` | Caps Lock |
| `0x4f` | Right Arrow |
| `0x50` | Left Arrow |
| `0x51` | Down Arrow |
| `0x52` | Up Arrow |

只处理字母和数字虽然能够应对最简单的题目，但遇到 Flag 中的下划线、花括号和特殊字符时，必须同时准备普通键表和 Shift 键表。

## 七、避免重复记录按键

USB 键盘会周期性发送当前按键状态。如果用户按住一个按键几十毫秒，抓包中可能连续出现多条相同报告。

例如：

```text
00 00 04 00 00 00 00 00
00 00 04 00 00 00 00 00
00 00 04 00 00 00 00 00
00 00 00 00 00 00 00 00
```

这些数据通常只代表按下了一次 `a`，并不一定是输入了三个 `a`。

正确的处理方法是比较当前报告与上一条报告，只记录本次新出现的键码：

```python
new_keys = [key for key in current_keys if key not in previous_keys]
```

当键码从非零变成全零时，只表示按键被松开，不应该输出字符。

## 八、处理Caps Lock

Caps Lock 和 Shift 的性质不同。

Shift 只在按住时生效，而 Caps Lock 每按下一次都会切换状态：

```python
if keycode == 0x39:
    caps_lock = not caps_lock
```

对于字母，最终是否大写可以通过异或判断：

```python
uppercase = shift ^ caps_lock
```

这意味着：

| Shift | Caps Lock | 字母状态 |
| --- | --- | --- |
| 未按 | 关闭 | 小写 |
| 按下 | 关闭 | 大写 |
| 未按 | 开启 | 大写 |
| 按下 | 开启 | 小写 |

对于数字和符号，Caps Lock 通常不生效，只需要判断 Shift。

## 九、处理退格和方向键

很多简单脚本遇到退格时，只会删除已经还原字符串的最后一个字符：

```python
text = text[:-1]
```

如果输入过程中没有移动光标，这样处理没有问题。但如果用户先按左方向键，再按退格，实际删除的是光标左侧的字符，而不是整行最后一个字符。

因此，完整还原时至少要维护两个变量：

```python
buffer = []
cursor = 0
```

输入普通字符：

```python
buffer.insert(cursor, char)
cursor += 1
```

按下左方向键：

```python
cursor = max(0, cursor - 1)
```

按下右方向键：

```python
cursor = min(len(buffer), cursor + 1)
```

按下退格：

```python
if cursor > 0:
    buffer.pop(cursor - 1)
    cursor -= 1
```

按下 Enter 时，将当前缓冲区保存为一行，并重新建立空缓冲区。

这样才能正确还原“先输入、再移动光标、删除并插入新字符”的情况。

## 十、导出键盘数据

在 Wireshark 中找到目标设备后，可以使用过滤器只保留它的中断数据。例如目标设备地址为 `2`：

```text
usb.device_address == 2 && usb.capdata
```

然后选择：

```text
文件 -> 导出分组解析结果 -> As CSV
```

也可以使用 TShark 直接导出数据部分：

```bash
tshark -r keyboard.pcap \
  -Y "usb.device_address == 2 && usb.capdata" \
  -T fields \
  -e usb.capdata > keyboard_data.txt
```

导出的数据可能带有冒号：

```text
00:00:0b:00:00:00:00:00
```

解析前可以统一删除冒号和空格。

## 十一、通用还原脚本

下面的脚本读取由 TShark 导出的 8 字节 HID 报告，并处理 Shift、Caps Lock、Enter、Backspace 和左右方向键。

```python
LOWER = {}
UPPER = {}

for i in range(26):
    LOWER[0x04 + i] = chr(ord("a") + i)
    UPPER[0x04 + i] = chr(ord("A") + i)

for code, normal, shifted in zip(
    range(0x1e, 0x28),
    "1234567890",
    "!@#$%^&*()",
):
    LOWER[code] = normal
    UPPER[code] = shifted

SYMBOLS = {
    0x2c: (" ", " "),
    0x2d: ("-", "_"),
    0x2e: ("=", "+"),
    0x2f: ("[", "{"),
    0x30: ("]", "}"),
    0x31: ("\\", "|"),
    0x33: (";", ":"),
    0x34: ("'", '"'),
    0x35: ("`", "~"),
    0x36: (",", "<"),
    0x37: (".", ">"),
    0x38: ("/", "?"),
}

for code, (normal, shifted) in SYMBOLS.items():
    LOWER[code] = normal
    UPPER[code] = shifted


def read_reports(path):
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            raw = line.strip().replace(":", "").replace(" ", "")

            if len(raw) != 16:
                continue

            try:
                yield bytes.fromhex(raw)
            except ValueError:
                continue


lines = []
buffer = []
cursor = 0
caps_lock = False
previous_keys = set()

for report in read_reports("keyboard_data.txt"):
    modifier = report[0]
    shift = bool(modifier & 0x22)
    current_keys = [key for key in report[2:8] if key != 0]

    new_keys = [key for key in current_keys if key not in previous_keys]

    for key in new_keys:
        if key == 0x28:                 # Enter
            lines.append("".join(buffer))
            buffer = []
            cursor = 0

        elif key == 0x2a:               # Backspace
            if cursor > 0:
                buffer.pop(cursor - 1)
                cursor -= 1

        elif key == 0x39:               # Caps Lock
            caps_lock = not caps_lock

        elif key == 0x4f:               # Right Arrow
            cursor = min(len(buffer), cursor + 1)

        elif key == 0x50:               # Left Arrow
            cursor = max(0, cursor - 1)

        else:
            is_letter = 0x04 <= key <= 0x1d
            use_upper = (shift ^ caps_lock) if is_letter else shift
            char = UPPER.get(key) if use_upper else LOWER.get(key)

            if char is not None:
                buffer.insert(cursor, char)
                cursor += 1

    previous_keys = set(current_keys)

if buffer:
    lines.append("".join(buffer))

for line in lines:
    print(line)
```

运行脚本：

```bash
python3 usb_keyboard.py
```

### 脚本中的一个注意点

程序按照 HID 报告的六个按键槽位提取新按下的按键：

```python
new_keys = [
    key for key in report[2:8]
    if key != 0 and key not in previous_keys
]
```

这样既能过滤持续按住产生的重复报告，也能在多个普通按键同时出现时保留报告中的槽位顺序。

## 十二、常见错误

### 1.提取了鼠标设备

如果还原结果全部是不可见字符、方向键或无规律数字，首先检查设备地址是否选错，不要看到 8 字节报告就直接认为是键盘。

### 2.重复输出字符

如果结果中每个字符重复多次，说明脚本没有区分“按键保持”和“新按下”，需要比较前后报告。

### 3.花括号和下划线错误

这通常是没有处理 Shift。Flag 中常见的 `{`、`}` 和 `_` 都需要通过 Shift 组合得到。

### 4.大小写不正确

只判断 Shift 而没有维护 Caps Lock 状态，会导致部分字母大小写相反。

### 5.删除位置错误

如果题目中出现方向键，不能一直操作字符串末尾，必须维护光标位置。

### 6.同时处理提交包和完成包

USBPcap 中一次传输可能同时出现提交记录和完成记录。如果两种记录都含有相同数据，脚本可能重复解析。此时需要结合端点方向、数据长度和 USBPcap 的 `info` 字段，只保留真正携带键盘报告的一类数据包。

## 十三、完整分析流程

USB 键盘流量题可以按照下面的顺序处理：

```text
确认PCAP与USBPcap链路类型
        |
        v
统计总线、设备、端点和数据长度
        |
        v
定位Interrupt IN方向的HID报告
        |
        v
根据报告规律区分键盘与鼠标
        |
        v
提取8字节键盘报告
        |
        v
过滤重复按键和松开报告
        |
        v
映射普通键与Shift字符
        |
        v
维护Caps Lock状态
        |
        v
模拟退格、Enter与方向键
        |
        v
还原最终输入内容
```

## 总结

USB 键盘流量分析的关键不只是准备一张 HID 键码表，更重要的是理解键盘报告表达的是“当前按键状态”，而不是单纯的字符流。

面对这类题目时，首先要从多个 USB 设备中定位键盘，然后处理按键按下与松开、Shift、Caps Lock、特殊符号以及编辑操作。尤其是在出现退格和方向键时，只有模拟光标和文本缓冲区，才能恢复用户最终看到的内容。

掌握这一套流程后，即使题目更换设备地址、输入内容或按键顺序，也可以使用相同的方法完成分析。
