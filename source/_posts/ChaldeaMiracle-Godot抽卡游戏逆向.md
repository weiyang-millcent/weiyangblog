---
title: "ChaldeaMiracle：Godot 抽卡游戏逆向与 CTR 解密"
date: 2026-10-14
categories: ["逆向工程"]
tags: ["CTF", "Godot", "GDRE Tools", "SM4变体", "CTR", "Python"]
description: "从 Godot PCK 提取明文脚本，计算十连印章并派生密钥，复现修改后的24轮分组算法与CTR模式，恢复抽卡游戏隐藏的Flag。"
cover: /imgs/20261014-cover-chaldea.jpg
password: "8888"
abstract: 本文已设置访问密码，请输入密码后查看完整题解。
message: 请输入访问密码后查看本文。
wrong_pass_message: 密码错误，请重新输入。
toc: true
comments: true
---

十连抽全部出金，才能看到隐藏奖励——这是 ChaldeaMiracle 给出的通关条件。读到抽卡脚本后才发现，单次出金概率只有万分之一，一次十连全金的理论概率是 `10^-40`。

这道题的突破口在游戏资源里：Godot 的 PCK 包保存了明文脚本，十个印章由固定字符串和槽位编号生成。顺着奖励函数往下追踪，就能重现密钥派生和 CTR 密钥流，解开 `record.dat` 中的 Flag。

本文从资源提取讲到完整解密脚本，重点解释源码里的字节操作如何对应 Python。需要准备 GDRE Tools 和 Python 3.9 或更高版本，脚本仅使用 Python 标准库。

<!-- more -->

## 解题路线

先提取 PCK 资源，从 `ritual_engine.gd` 定位奖励条件，再进入 `astral_cipher.gd` 分析印章、密钥和文件格式。最后按源码复现修改过的 24 轮分组算法，通过 CTR 异或恢复明文，并使用 `record.dat` 自带的 SHA-256 摘要检查结果。

这题主要涉及 Godot 资源提取、固定输入的密钥派生、大端读写、32 位循环移位，以及 CTR 模式下的密钥流生成。

## 1. 检查附件，确定分析目标

解压题目附件 `challenge (2).zip`，得到：

```text
ChaldeaMiracle.exe
ChaldeaMiracle.pck
```

`.pck` 是 Godot 的资源包，可以包含游戏脚本、场景、图片等。先查看资源包，比直接分析整个游戏 EXE 更容易定位题目的逻辑。

本题 PCK 开头四字节是：

```text
47 44 50 43  →  GDPC
```

资源包的格式版本为 4，引擎版本字段为 4.7.2。目录和文件均未加密，共有 70 个资源文件。

## 2. 用 GDRE Tools 提取资源

本次使用的是 **GDRE Tools v2.7.4**。下载入口为 [官方发布页](https://github.com/GDRETools/gdsdecomp/releases/tag/v2.7.4)，Windows 包名为 `GDRE_tools-v2.7.4-windows.zip`。

图形界面操作：

1. 解压工具，运行 `gdre_tools.exe`。
2. 选择左上角 `RE Tools → Recover Project...`。
3. 在文件选择窗口中打开 `ChaldeaMiracle.pck`。
4. 在资源列表中点击 `Check all`，选中全部文件。
5. 将底部 `Extract to...` 设置为自己的输出目录，例如题目目录下的 `manual_extracted`。
6. 选择 `Extract Only`，点击 `Extract`。

这里选择“仅提取”就足够了，因为关键脚本直接以明文 `.gd` 文件保存在资源包中。

如果使用命令行，也可以直接调用软件自己的提取功能：

```powershell
.\gdre-tools\gdre_tools.exe --headless --extract=".\ChaldeaMiracle.pck" --output=".\manual_extracted"
```

这条命令由 GDRE Tools 完成提取，不需要自己编写 PCK 解析脚本。命令中的相对路径以当前终端目录为准。

提取后重点查看：

```text
manual_extracted/
├─ scripts/
│  ├─ ritual_engine.gd
│  ├─ astral_cipher.gd
│  ├─ main.gd
│  └─ ether_fx.gd
└─ assets/
   └─ record.dat
```

| 文件 | 本题中负责的内容 |
|---|---|
| `ritual_engine.gd` | 抽卡、印章收集、奖励触发 |
| `astral_cipher.gd` | 印章生成、密钥派生、加密运算和结果检查 |
| `record.dat` | 保存加密后的 flag |

提取完成后，先确认上述两个关键脚本和 `record.dat` 都存在。原提取记录中的日志显示 `Verified 70 files, no errors detected!` 和 `Extracted 70 files, no errors detected!`，说明资源包可以正常读取。

## 3. 先找到游戏什么时候展示奖励

在 `ritual_engine.gd` 中查看 `roll_rarity()`：

```gdscript
func roll_rarity() -> int:
    var roll := rng.randi_range(0, 9999)
    if roll == 0:
        return GOLD
    if roll < 1001:
        return SILVER
    return BRONZE
```

`randi_range(0, 9999)` 从 0 到 9999 之间取随机数，共有 10000 个可能值。只有结果为 0 才返回 `GOLD`，所以单次出金概率是 `1/10000`。

继续查看 `summon()` 中与印章和奖励有关的部分：

```gdscript
if rarity == GOLD:
    golds += 1
    if count == 10:
        seals.append(CIPHER.golden_seal(slot))
```

每抽到一张金卡，就增加出金计数。在十连模式下，还会生成这个槽位的印章，并放进 `seals` 数组。

奖励入口是：

```gdscript
var complete := count == 10 and golds == 10 and seals.size() == 10
var reward := ""
if complete:
    reward = CIPHER.open_record(seals)
```

三个条件同时成立才进入 `open_record()`：进行十连、十张全部出金、拥有十个印章。

按独立抽取计算，一次十连全金的概率是：

```text
(1 / 10000)^10 = 10^-40
```

接下来追踪的重点是：印章如何生成，以及 `open_record()` 如何还原奖励。

## 4. 印章可以直接计算

先补充一个基础概念：**字节是一个范围在 0～255 的数字**。程序读取文件时，得到的是字节序列。为了方便查看，工具常用十六进制显示它们，例如 `ff` 表示十进制 255。

`PackedByteArray` 就是 Godot 中保存这些字节的数组。

印章生成源码：

```gdscript
static func golden_seal(slot: int) -> PackedByteArray:
    var phrase := "CHALDEA/SEAL/v1|%d|LUMEN-2731" % slot
    return _sha256(phrase.to_utf8_buffer()).slice(4, 12)
```

第一行把槽位编号填进 `%d`。例如槽位 0 的完整字符串是：

```text
CHALDEA/SEAL/v1|0|LUMEN-2731
```

第二行包含三个动作：

1. `to_utf8_buffer()`：把字符串转换为 UTF-8 字节。
2. `_sha256()`：给这些字节计算 SHA-256 摘要，也可以理解为计算数据的“指纹”。同样的输入会产生同样的 32 字节结果。
3. `.slice(4, 12)`：取摘要中下标 4 到 11 的八字节；右端下标 12 不包含在结果中。

Python 对应写法：

```python
phrase = f"CHALDEA/SEAL/v1|{slot}|LUMEN-2731"
seal = hashlib.sha256(phrase.encode("utf-8")).digest()[4:12]
```

这里要用 `.digest()` 得到原始字节。`.hexdigest()` 返回的是用于展示的十六进制文本，两者不能用相同下标直接替代。

槽位 0 的印章为：

```text
74 34 02 42 42 2e 1e 52
```

这个函数只需要槽位编号，不需要抽卡随机数，也不依赖具体抽到哪个角色。因此我们可以直接计算槽位 0 到 9 的十个合法印章。

## 5. 从十个印章得到密钥

密钥派生源码：

```gdscript
static func _derive_key(seals: Array[PackedByteArray]) -> PackedByteArray:
    var buf := "CHALDEA/TENFOLD/MNEMOSYNE|".to_utf8_buffer()
    for i in range(10):
        buf.append(i)
        buf.append_array(seals[i])
    buf.append_array("|STARS-ARE-WITNESSES".to_utf8_buffer())
    return _sha256(buf).slice(0, 16)
```

把 `buf` 理解为一个逐步追加数据的字节列表。

首先放入固定前缀：

```text
CHALDEA/TENFOLD/MNEMOSYNE|
```

随后每个槽位依次追加“一个编号字节”和“八字节印章”。例如前两个槽位加入的是：

```text
00 74 34 02 42 42 2e 1e 52
01 d4 30 02 aa 0e 10 60 be
```

`buf.append(i)` 加入的是编号这个数字本身，不是编号的文字。例如：

```python
bytes([1])  # 字节01，符合源码
b"1"        # 字符'1'对应的字节31，不符合源码
```

同样，印章要加入八个原始字节，而不是十六个十六进制字符。

最后追加固定后缀：

```text
|STARS-ARE-WITNESSES
```

整个哈希输入的排列为：

```text
前缀 || byte(0) || seal[0] || ... || byte(9) || seal[9] || 后缀
```

这里 `||` 只是表示拼接，不是实际加入的字符。输入总长度是：

```text
26字节前缀 + 10 × (1字节编号 + 8字节印章) + 20字节后缀
= 136字节
```

对这 136 字节计算 SHA-256，再取前 16 字节，得到密钥：

```text
2ee1b3dd048a55a9d4372037afe59442
```

这些字符串和步骤是作者规定的生成方法。解题时需要保留它们的每个字符、原始字节和排列顺序，才能重现同一把密钥。

## 6. 阅读解密入口，解析 record.dat

`open_record()` 完整源码如下：

```gdscript
static func open_record(seals: Array[PackedByteArray]) -> String:
    if seals.size() != 10:
        return ""
    for i in range(10):
        if seals[i] != golden_seal(i):
            return ""
    var data := FileAccess.get_file_as_bytes("res://assets/record.dat")
    if data.size() < 30:
        return ""
    if data.slice(0, 4) != "CLD1".to_ascii_buffer():
        return ""
    var count := (int(data[16]) << 8) | int(data[17])
    if data.size() != 18 + count + 8:
        return ""
    var nonce := data.slice(4, 16)
    var encrypted := data.slice(18, 18 + count)
    var output := _ctr(encrypted, _derive_key(seals), nonce)
    if _sha256(output).slice(0, 8) != data.slice(18 + count, data.size()):
        return ""
    var text := output.get_string_from_utf8()
    if not text.begins_with("flag{") or not text.ends_with("}"):
        return ""
    return text
```

前面的两段检查要求印章数量为 10，并逐个比较 `seals[i]` 与 `golden_seal(i)`。我们按上一节的方法计算印章，就能满足这些检查。

随后程序读取 `res://assets/record.dat`。`res://` 表示 Godot 的资源根目录，对应提取目录中的 `assets/record.dat`。

本题文件共 77 字节，排列如下。所有偏移从 0 开始计数：

| 偏移范围 | 长度 | 内容 |
|---|---:|---|
| 0～3 | 4 | 文件标识 `CLD1` |
| 4～15 | 12 | nonce |
| 16～17 | 2 | 大端密文长度 |
| 18～68 | 51 | 密文 |
| 69～76 | 8 | 正确明文 SHA-256 的前八字节 |

程序通过固定位置读取数据，并非自动猜测密文的位置。这是写入文件和读取文件时共同遵守的格式。

长度读取代码：

```gdscript
var count := (int(data[16]) << 8) | int(data[17])
```

可以理解为：

```text
count = 第一个字节 × 256 + 第二个字节
```

一个字节只能表示 0～255，两个字节组合时，前者表示“有多少组256”，后者表示剩余值。这种把高位字节放在前面的方式叫“大端”。

本题这两字节是 `00 33`。十六进制 `33` 等于十进制 51，所以密文长度为 51。

程序还检查文件总长度是否为 `18 + count + 8`，防止长度与数据不一致。

解析出的 nonce 为：

```text
4ab126d3ff82e4b72f3994ac
```

解析出的密文为：

```text
50064966874a6a84c0908f5f48f48be5588abcde2f83c09a85a91bfd3ebfdd9f4ee62f795fbd6b947fa66939eb319d6493fc1a
```

之后的关键调用是：

```gdscript
var output := _ctr(encrypted, _derive_key(seals), nonce)
```

它先从印章计算密钥，再把密文、密钥和 nonce 交给 `_ctr()`，得到明文字节。

## 7. CTR 为什么能解密：异或两次会还原

先认识源码中的 `^`：它表示按位异或。把数字写成二进制，每一位都按“相同得0、不同得1”计算。

| 左边的位 | 右边的位 | 异或结果 |
|---|---|---|
| 0 | 0 | 0 |
| 0 | 1 | 1 |
| 1 | 0 | 1 |
| 1 | 1 | 0 |

用四位数字演示：

```text
  1010             0110
^ 1100           ^ 1100
------           ------
  0110             1010
```

原数字与同一个数字异或两次，会恢复原值。因此：

```text
密文 = 明文 XOR 密钥流
明文 = 密文 XOR 同一段密钥流
```

密钥流是一段由算法计算出的字节。解密的任务就是重现加密时使用的那段密钥流。

`_ctr()` 完整源码：

```gdscript
static func _ctr(cipher: PackedByteArray, key: PackedByteArray, nonce: PackedByteArray) -> PackedByteArray:
    var plain := PackedByteArray()
    for start in range(0, cipher.size(), 16):
        var iv := nonce.duplicate()
        _put_u32(iv, (start / 16) + 1)
        var stream := _block(iv, key)
        for i in range(mini(16, cipher.size() - start)):
            plain.append(cipher[start + i] ^ stream[i])
    return plain
```

每次处理 16 字节密文。51 字节被分成四段，长度分别是 `16、16、16、3`。

对于每一段，先复制十二字节 nonce，再追加四字节大端计数器。计数器从 1 开始：

```text
第一段：4ab126d3ff82e4b72f3994ac00000001
第二段：4ab126d3ff82e4b72f3994ac00000002
第三段：4ab126d3ff82e4b72f3994ac00000003
第四段：4ab126d3ff82e4b72f3994ac00000004
```

nonce 是文件附带的一组输入数字；计数器则使每一段的输入发生变化，避免重复处理完全相同的输入块。

`_block(iv, key)` 将十六字节计数器块和密钥交给分组算法，输出十六字节密钥流。

然后执行：

```gdscript
plain.append(cipher[start + i] ^ stream[i])
```

把密文和密钥流逐字节异或，追加到明文数组中。最后一段只有三字节，`mini()` 会把循环限制在三次，所以无需额外填充。

第一段的实际计算结果：

```text
密文：   50064966874a6a84c0908f5f48f48be5
密钥流： 366a2801fc3e59ea9ff7bf332cc7e5ba
异或后： 666c61677b74336e5f67306c64336e5f
文字：   flag{t3n_g0ld3n_
```

例如第一个字节：`0x50 XOR 0x36 = 0x66`，而 `0x66` 正是字符 `f` 的编码。

这里容易误解的一点是：CTR 解密仍然调用分组算法的**加密运算**。它加密的是计数器块，产生密钥流；随后才将密钥流与密文异或。我们不需要反向执行分组算法，也不需要对密文块直接做分组解密。

## 8. 复现分组算法

`astral_cipher.gd` 使用 SM4 的 S 盒与 FK 常量，但定义了修改过的 24 轮运算。不能直接把它交给普通 SM4 库，需要按源码实现以下规则。

### 8.1 字节顺序和32位运算

```gdscript
static func _load_u32(b: PackedByteArray, at: int) -> int:
    return (int(b[at]) << 24) | (int(b[at + 1]) << 16) | (int(b[at + 2]) << 8) | int(b[at + 3])

static func _put_u32(out: PackedByteArray, value: int) -> void:
    value &= MASK
    out.append((value >> 24) & 255)
    out.append((value >> 16) & 255)
    out.append((value >> 8) & 255)
    out.append(value & 255)

static func _rol(x: int, n: int) -> int:
    x &= MASK
    return ((x << n) | (x >> (32 - n))) & MASK
```

`_load_u32()` 把四个字节合成一个整数。例如 `12 34 56 78` 合成 `0x12345678`，顺序为大端。

`_put_u32()` 做相反的事，把整数写成四个大端字节。因此计数器 1 会写成 `00 00 00 01`。

`MASK` 是 `0xffffffff`，二进制为 32 个 1。`& MASK` 只保留结果的低 32 位，模拟32位整数运算。

`_rol()` 实现32位循环左移：向左移出的位重新进入右侧。用四位演示一次循环左移，`1001` 会变成 `0011`；本题实际宽度是32位。

### 8.2 S盒替换和线性变换

```gdscript
static func _tau(w: int) -> int:
    if _box.is_empty():
        _box = SBOX_HEX.hex_decode()
    var y := 0
    for j in range(4):
        var shift := 24 - 8 * j
        y |= int(_box[(w >> shift) & 255]) << shift
    return y & MASK

static func _l(w: int) -> int:
    return (w ^ _rol(w, 3) ^ _rol(w, 9) ^ _rol(w, 17) ^ _rol(w, 25)) & MASK

static func _key_l(w: int) -> int:
    return (w ^ _rol(w, 13) ^ _rol(w, 23)) & MASK
```

`SBOX_HEX` 是原脚本开头保存的一张256项替换表。`hex_decode()` 把十六进制文字转换成实际字节。完整表保留在原脚本中，文末解密程序会自动读取它。

`_tau()` 从一个32位整数中取出四个字节，分别查表，再按原位置组合回去。例如表中 `0、1、2、3` 对应 `d6、90、e9、fe`，所以：

```text
tau(0x00010203) = 0xd690e9fe
```

`_l()` 把原整数与循环左移3、9、17、25位的四个版本异或；`_key_l()` 使用13和23位。它们分别用于数据运算和密钥扩展，不能混用。

这些旋转距离是源码规定的算法参数。复现时必须保持一致，并不需要从字符串含义中推导它们。

### 8.3 生成24个轮密钥

轮密钥是从原始密钥计算出的、每一轮使用的32位整数。源码使用的 FK 常量为：

```gdscript
const FK := [0xa3b1bac6, 0x56aa3350, 0x677d9197, 0xb27022dc]
```

生成过程：

```gdscript
static func _round_keys(key: PackedByteArray) -> Array[int]:
    var k: Array[int] = []
    var keys: Array[int] = []
    for i in range(4):
        k.append(_load_u32(key, i * 4) ^ int(FK[i]))
    for rnd in range(24):
        var ck := 0
        for j in range(4):
            ck = (ck << 8) | (((rnd * 4 + j) * 7) & 255)
        var mix := k[rnd + 1] ^ k[rnd + 2] ^ k[rnd + 3] ^ ck
        var rk := (k[rnd] ^ _key_l(_tau(mix))) & MASK
        k.append(rk)
        keys.append(rk)
    return keys
```

先将16字节密钥拆成四个32位整数，分别与 FK 异或，得到初始状态 `K0～K3`。

每轮的四字节常量由 `((rnd * 4 + j) * 7) & 255` 计算。例如第0轮得到 `00 07 0e 15`，合成 `0x00070e15`；第1轮得到 `1c 23 2a 31`，合成 `0x1c232a31`。

随后按以下规则计算新状态：

```text
mix = K[r+1] XOR K[r+2] XOR K[r+3] XOR CK[r]
K[r+4] = K[r] XOR key_l(tau(mix))
rk[r] = K[r+4]
```

新增的状态会继续参与后面的计算。24轮结束后，得到24个轮密钥。

### 8.4 24轮分组运算

```gdscript
static func _block(block: PackedByteArray, key: PackedByteArray) -> PackedByteArray:
    var x: Array[int] = []
    for i in range(4):
        x.append(_load_u32(block, i * 4))
    for rk in _round_keys(key):
        var v := x[x.size() - 3] ^ x[x.size() - 2] ^ x[x.size() - 1] ^ rk
        var next := (x[x.size() - 4] ^ _l(_tau(v))) & MASK
        x.append(next)
    var result := PackedByteArray()
    for j in range(4):
        _put_u32(result, x[x.size() - 1 - j])
    return result
```

先把16字节输入拆成四个32位整数 `X0～X3`。第一轮计算：

```text
v = X1 XOR X2 XOR X3 XOR rk0
X4 = X0 XOR L(tau(v))
```

追加 `X4` 后，第二轮使用末尾四个状态字 `X1、X2、X3、X4`：

```text
v = X2 XOR X3 XOR X4 XOR rk1
X5 = X1 XOR L(tau(v))
```

一般公式为：

```text
X[r+4] = X[r] XOR L(tau(X[r+1] XOR X[r+2] XOR X[r+3] XOR rk[r]))
```

经过24轮后，状态数组有28个整数。最后反序取末尾四个字，以大端序写出：

```text
X27 || X26 || X25 || X24
```

输出共16字节，即 CTR 当前段所需的密钥流。

复现时重点核对：24轮、S盒、两种旋转参数、32位掩码、大端字节顺序、末尾四字反序，以及CTR计数器从1开始。

## 9. 用原题的检查确认结果

解密后，`open_record()` 计算明文 SHA-256 的前八字节，并与文件末尾的八字节比较。这里的摘要用于检查结果，它本身不是解密密钥，也不是可逆的密文。

本题实际得到：

```text
SHA-256(plaintext)[:8] = b107fdb8897e58b3
```

与原始 `record.dat` 保存的值一致。解码后的文本也满足源码检查的 `flag{` 前缀和 `}` 后缀。

最终 flag：

```text
flag{t3n_g0ld3n_summ0ns_unl0ck_th3_ch4ld34_4rch1v3}
```

## 10. 完整解密脚本和运行方法

将下面代码保存为 `solve_extracted.py`，放在与 `manual_extracted` 同级的目录中。目录排列为：

```text
题目目录/
├─ solve_extracted.py
└─ manual_extracted/
   ├─ scripts/astral_cipher.gd
   └─ assets/record.dat
```

打开题目目录下的 PowerShell，执行：

```powershell
py .\solve_extracted.py
```

如果自己的资源输出目录名称不同，可以把路径作为参数传入：

```powershell
py -3.9 .\solve_extracted.py ".\自己的提取目录"
```

以下程序只使用 Python 标准库，直接读取提取出的脚本和密文；不会运行游戏，不需要联网，也不需要安装第三方加密库。

```python
"""Decrypt ChaldeaMiracle using resources extracted with GDRE Tools.

Python 3.9+, standard library only.
Usage: py -3.9 solve_extracted.py [resource_directory]
"""
from pathlib import Path
import hashlib
import re
import struct
import sys

MASK = 0xffffffff
FK = (0xa3b1bac6, 0x56aa3350, 0x677d9197, 0xb27022dc)


def sha256(data):
    return hashlib.sha256(data).digest()


def golden_seal(slot):
    phrase = f"CHALDEA/SEAL/v1|{slot}|LUMEN-2731"
    return sha256(phrase.encode("utf-8"))[4:12]


def derive_key(seals):
    material = bytearray(b"CHALDEA/TENFOLD/MNEMOSYNE|")
    for slot in range(10):
        material.append(slot)       # One raw byte, not the character '0'/'1'.
        material.extend(seals[slot])
    material.extend(b"|STARS-ARE-WITNESSES")
    return sha256(material)[:16]


def rotate_left(word, bits):
    word &= MASK
    return ((word << bits) | (word >> (32 - bits))) & MASK


def substitute(word, sbox):
    result = 0
    for shift in (24, 16, 8, 0):
        byte = (word >> shift) & 0xff
        result |= sbox[byte] << shift
    return result & MASK


def data_linear(word):
    return (word ^ rotate_left(word, 3) ^ rotate_left(word, 9)
            ^ rotate_left(word, 17) ^ rotate_left(word, 25)) & MASK


def key_linear(word):
    return (word ^ rotate_left(word, 13) ^ rotate_left(word, 23)) & MASK


def expand_key(key, sbox):
    master_words = struct.unpack(">4I", key)  # Four big-endian uint32 values.
    state = [master_words[i] ^ FK[i] for i in range(4)]
    round_keys = []
    for round_number in range(24):
        constant_bytes = bytes(
            ((round_number * 4 + j) * 7) & 0xff for j in range(4)
        )
        constant = int.from_bytes(constant_bytes, "big")
        mixed = (state[round_number + 1] ^ state[round_number + 2]
                 ^ state[round_number + 3] ^ constant)
        next_word = (state[round_number]
                     ^ key_linear(substitute(mixed, sbox))) & MASK
        state.append(next_word)
        round_keys.append(next_word)
    return round_keys


def encrypt_block(block, round_keys, sbox):
    state = list(struct.unpack(">4I", block))
    for round_key in round_keys:
        mixed = state[-3] ^ state[-2] ^ state[-1] ^ round_key
        next_word = (state[-4]
                     ^ data_linear(substitute(mixed, sbox))) & MASK
        state.append(next_word)
    # Reverse the last four words, then serialize each in big-endian order.
    return struct.pack(">4I", *state[-4:][::-1])


def decrypt_ctr(ciphertext, nonce, round_keys, sbox):
    plaintext = bytearray()
    for start in range(0, len(ciphertext), 16):
        counter = start // 16 + 1
        counter_block = nonce + counter.to_bytes(4, "big")
        keystream = encrypt_block(counter_block, round_keys, sbox)
        chunk = ciphertext[start:start + 16]
        plaintext.extend(a ^ b for a, b in zip(chunk, keystream))
    return bytes(plaintext)


def main(resource_directory):
    source = (resource_directory / "scripts" / "astral_cipher.gd").read_text(
        encoding="utf-8"
    )
    # Read the exact S-box from the original script; no third-party crypto library.
    match = re.search(r"const\s+SBOX_HEX\s*:=\s*\((.*?)\)", source, re.S)
    assert match is not None, "SBOX_HEX not found"
    pieces = re.findall(r'"([0-9a-fA-F]+)"', match.group(1))
    sbox = bytes.fromhex("".join(pieces))
    assert len(sbox) == 256

    seals = [golden_seal(slot) for slot in range(10)]
    key = derive_key(seals)
    round_keys = expand_key(key, sbox)

    record = (resource_directory / "assets" / "record.dat").read_bytes()
    assert len(record) >= 30 and record[:4] == b"CLD1", "Invalid record header"
    nonce = record[4:16]
    length = int.from_bytes(record[16:18], "big")
    assert len(record) == 18 + length + 8, "Invalid record length"
    ciphertext = record[18:18 + length]
    expected_digest = record[18 + length:]

    plaintext = decrypt_ctr(ciphertext, nonce, round_keys, sbox)
    actual_digest = sha256(plaintext)[:8]
    assert actual_digest == expected_digest, "Original SHA-256 check failed"
    flag = plaintext.decode("utf-8")
    assert flag.startswith("flag{") and flag.endswith("}")

    print("key =", key.hex())
    print("nonce =", nonce.hex())
    print("ciphertext length =", length)
    print("SHA-256[:8] =", actual_digest.hex(), "(verified)")
    print(flag)
    return flag


if __name__ == "__main__":
    default_directory = Path(__file__).resolve().parent / "manual_extracted"
    directory = Path(sys.argv[1]) if len(sys.argv) > 1 else default_directory
    main(directory)
```

实际运行输出：

```text
key = 2ee1b3dd048a55a9d4372037afe59442
nonce = 4ab126d3ff82e4b72f3994ac
ciphertext length = 51
SHA-256[:8] = b107fdb8897e58b3 (verified)
flag{t3n_g0ld3n_summ0ns_unl0ck_th3_ch4ld34_4rch1v3}
```

复现中最常见的错误是：把编号字节写成数字字符、对十六进制文本做哈希切片、使用普通SM4算法参数、计数器从0开始，以及把大端写成小端。遇到哈希检查失败时，可以先比较上面的密钥、nonce和第一段密钥流，判断错误发生在哪一步。

## 11. 这道题值得记住的几个点

- **从资源包定位业务逻辑。** 面对游戏题，先检查脚本和资源格式，往往比从引擎可执行文件的入口开始分析更直接。
- **分清触发条件和数据依赖。** 十连全金是界面展示奖励的条件，但印章只依赖固定字符串与槽位编号，能够在游戏外重建。
- **按源码参数复现算法。** 使用 SM4 的 S 盒并不代表整个算法就是标准 SM4。本题的轮数和线性变换都需要逐项核对。
- **CTR 的解密不需要逆向分组运算。** 只要生成相同密钥流，就可以通过异或还原密文。
- **使用原程序的检查验证答案。** 文件末尾的八字节摘要给出了明确的验证依据，不能只凭结果看起来像 Flag 就结束分析。

## 参考

- [GDRE Tools 官方项目与操作说明](https://github.com/GDRETools/gdsdecomp)
- [Godot 官方 PCK 读取实现](https://github.com/godotengine/godot/blob/master/core/io/file_access_pack.cpp)
- 本题附件中的 `ritual_engine.gd`、`astral_cipher.gd` 与 `record.dat`。算法和常量均以附件源码为依据。

