---
title: "fanchai请你喝茶！题目复现"
date: 2026-09-25
categories: ["reverse"]
tags: ["IDA", "TEA", "XTEA", "学习笔记"]
description: 记录对chall6的逆向分析过程，从IDA中提取TEA/XTEA密钥和密文，并通过逆向轮函数完成解密。
cover: /imgs/20260925-tea6-cover.png
password: "8888"
abstract: 本文已设置访问密码，请输入密码后查看完整题解。
message: 请输入访问密码后查看本文。
wrong_pass_message: 密码错误，请重新输入。
toc: true
comments: true
---

# fanchai请你喝茶！题目复现

## 前言

题目给出了一个64位 ELF 可执行文件 `chall6`，以及一份带有空缺的 `tea6solve.c`。我们需要根据 IDA 中的反编译结果，找出密文、密钥和具体的加密逻辑，最后补全解密脚本。

这道题将长度为32字节的 Flag 分成了两部分：

```text
前16字节 → TEA加密
后16字节 → XTEA加密
```

完整分析路线如下：

```text
分析main函数
  ↓
确定前后两段的检查方式
  ↓
提取TEA/XTEA密文和密钥
  ↓
还原两个加密函数
  ↓
逆向运算顺序编写解密函数
  ↓
得到Flag
```

---

## 一、分析main函数

将 `chall6` 拖入 IDA，等待自动分析完成，然后进入 `main` 函数并按 `F5` 生成伪代码：

![main函数伪代码](/imgs/20260925-tea6-main.png)

关键逻辑可以整理为：

```c
if (fgets(s, 34, stdin))
{
    len = strcspn(s, "\r\n");
    s[len] = 0;

    if (len == 32 && check_tea_part(s))
    {
        if (check_xtea_part(s + 16))
            puts("Correct!");
        else
            puts("Wrong!");
    }
}
```

`fgets(s, 34, stdin)` 最多读取33个字符，`strcspn(s, "\r\n")` 找到换行符的位置，也就是实际输入长度。程序随后判断：

```c
len == 32
```

因此 Flag 必须恰好是32字节。

两个检查函数的参数分别是：

```c
check_tea_part(s);
check_xtea_part(s + 16);
```

`s` 是输入首地址，因此 `check_tea_part` 从第0字节开始读取。`s + 16` 则指向第16字节，所以 `check_xtea_part` 检查后半部分。

```text
s[0]                         s[16]                       s[31]
 ↓                            ↓                           ↓
┌────────────────────────────────┬────────────────────────────────┐
│          前16字节              │           后16字节             │
│      check_tea_part(s)         │ check_xtea_part(s + 16)       │
└────────────────────────────────┴────────────────────────────────┘
                 TEA                              XTEA
```

---

## 二、确定两部分的分组方式

### 1. TEA检查函数

双击 `check_tea_part` 进入函数：

![check_tea_part伪代码](/imgs/20260925-tea6-check-tea.png)

函数通过4次 `load32_le` 读取了4个32位整数：

```c
v2 = load32_le(a1);
v3 = load32_le(a1 + 4);
v4 = load32_le(a1 + 8);
v5 = load32_le(a1 + 12);
```

一个32位整数占4字节，4个整数刚好对应16字节。接下来分别从 `v2` 和 `v4` 的地址开始加密：

```c
tea_encrypt(&v2, tea_key_address);
tea_encrypt(&v4, tea_key_address);
```

TEA 每次处理两个32位整数，即8字节：

```text
第一组：v2 + v3 = 8字节
第二组：v4 + v5 = 8字节
```

因此 `check_tea_part` 总共检查前16字节。

### 2. XTEA检查函数

`check_xtea_part` 的结构几乎一样：

![check_xtea_part伪代码](/imgs/20260925-tea6-check-xtea.png)

IDA 此时把 `a1` 识别成了 `unsigned __int16 *`，所以伪代码显示：

```c
v2 = load32_le(a1);
v3 = load32_le(a1 + 2);
v4 = load32_le(a1 + 4);
v5 = load32_le(a1 + 6);
```

这不是每次只向后移动2字节。因为 `a1` 指向的 `unsigned __int16` 占2字节，所以实际偏移为：

| 伪代码 | 计算 | 实际字节偏移 |
|---|---:|---:|
| `a1` | `0 × 2` | 0 |
| `a1 + 2` | `2 × 2` | 4 |
| `a1 + 4` | `4 × 2` | 8 |
| `a1 + 6` | `6 × 2` | 12 |

所以它仍然是连续读取16字节，并拆成两个8字节分组进行 XTEA 加密。

---

## 三、在IDA中寻找密钥

以 XTEA 为例，在 `check_xtea_part` 中可以看到：

```c
xtea_key_address = get_xtea_key_address();
```

双击 `get_xtea_key_address`，再按 `F5`：

![get_xtea_key_address函数](/imgs/20260925-tea6-key-function.png)

该函数直接返回 `xtea_key` 的地址。继续双击 `xtea_key`，IDA 会跳转到 `.rodata` 数据段：

![XTEA密钥与密文](/imgs/20260925-tea6-xtea-data.png)

密钥的原始字节为：

```text
EF BE AD DE BE BA FE CA 44 33 22 11 88 77 66 55
```

XTEA 密钥是128位，由4个32位整数组成。所以每4字节划分一组：

```text
EF BE AD DE
BE BA FE CA
44 33 22 11
88 77 66 55
```

x86-64 采用小端序，多字节整数的低位字节保存在低地址。因此每组都要在组内逆序读取：

```text
EF BE AD DE → 0xDEADBEEF
BE BA FE CA → 0xCAFEBABE
44 33 22 11 → 0x11223344
88 77 66 55 → 0x55667788
```

得到 XTEA 密钥：

```c
uint key2[4] = {
    0xDEADBEEF,
    0xCAFEBABE,
    0x11223344,
    0x55667788
};
```

用相同方法追踪 `get_tea_key_address`，或直接在数据段中查看 `0x402010`：

![TEA与XTEA密钥](/imgs/20260925-tea6-keys.png)

TEA 的原始密钥字节为：

```text
78 56 34 12 21 43 65 87 68 24 57 13 57 13 68 24
```

每4字节按小端序还原：

```c
uint key1[4] = {
    0x12345678,
    0x87654321,
    0x13572468,
    0x24681357
};
```

---

## 四、提取两组密文

在数据段中，`tea_cipher` 和 `xtea_cipher` 已经被 IDA 识别为 `dd`，也就是32位整数：

```text
tea_cipher:
0xB3E7E33E  0xB4114672  0x8E088C0C  0x14B2C329

xtea_cipher:
0x60EC68AB  0x940251CE  0xB427534C  0xDF435416
```

因此脚本中填入：

```c
uint cipher1[4] = {
    0xB3E7E33E,
    0xB4114672,
    0x8E088C0C,
    0x14B2C329
};

uint cipher2[4] = {
    0x60EC68AB,
    0x940251CE,
    0xB427534C,
    0xDF435416
};
```

需要注意：

```text
db = 1字节，需要自己每4字节分组并考虑小端序
dd = 4字节，IDA已经将它显示为32位整数，直接抄数值即可
```

---

## 五、还原TEA加密与解密

TEA 把一个8字节分组看成两个32位整数 `left` 和 `right`。本题的加密主体可以整理为：

```c
sum += 0x9E3779B9;

left += (right + sum)
        ^ ((right << 4) + key[0])
        ^ ((right >> 5) + key[1]);

right += (left + sum)
         ^ ((left << 4) + key[2])
         ^ ((left >> 5) + key[3]);
```

加密一轮的顺序是：

```text
更新sum → 更新left → 更新right
```

解密必须以相反顺序撤销：

```text
恢复right → 恢复left → 恢复sum
```

由于加密使用 `+=`，撤销时使用 `-=`。32轮后的 `sum` 为：

```text
0x9E3779B9 × 32 mod 2^32 = 0xC6EF3720
```

因此 TEA 解密函数为：

```c
void tea_decrypt(uint cipher[2], uint key[4])
{
    uint sum = 0xC6EF3720U;

    for (int i = 0; i < 32; i++)
    {
        cipher[1] -= (cipher[0] + sum)
                     ^ ((cipher[0] << 4) + key[2])
                     ^ ((cipher[0] >> 5) + key[3]);

        cipher[0] -= (cipher[1] + sum)
                     ^ ((cipher[1] << 4) + key[0])
                     ^ ((cipher[1] >> 5) + key[1]);

        sum += 1640531527;
    }
}
```

`1640531527` 的十六进制是 `0x61C88647`。在32位无符号运算中：

```c
sum += 0x61C88647;
```

等价于撤销一次 `sum += 0x9E3779B9`。

---

## 六、还原XTEA加密逻辑

IDA 中的 XTEA 加密函数可以整理为：

```c
left = data[0];
right = data[1];
sum = 0;

for (int i = 0; i < 32; i++)
{
    left += (((right << 4) ^ (right >> 5)) + right)
            ^ (key[sum & 3] + sum);

    sum += 0x9E3779B9;

    right += (((left << 4) ^ (left >> 5)) + left)
             ^ (key[(sum >> 11) & 3] + sum);
}
```

### 1. 理解IDA中的密钥指针

IDA 伪代码中可能显示：

```c
*(_DWORD *)(4LL * (sum & 3) + key_address)
```

`sum & 3` 的结果只可能是0、1、2、3。每个 `uint32_t` 占4字节，所以乘以4就是计算数组元素的字节偏移。

```text
key_address + 0  → key[0]
key_address + 4  → key[1]
key_address + 8  → key[2]
key_address + 12 → key[3]
```

因此整个表达式就是：

```c
key[sum & 3]
```

同理：

```c
*(_DWORD *)(4LL * ((sum >> 11) & 3) + key_address)
```

等价于：

```c
key[(sum >> 11) & 3]
```

`sum` 不是密钥，它是每轮变化的累计值。它既用于从4个密钥元素中选择一个，又会与选中的密钥相加：

```c
key[index] + sum
```

### 2. TEA与XTEA的主要区别

TEA 在每轮中固定使用密钥位置：

```text
更新left：key[0]、key[1]
更新right：key[2]、key[3]
```

XTEA 则根据 `sum` 动态选择：

```c
key[sum & 3]
key[(sum >> 11) & 3]
```

另外，TEA 在一轮开始时更新 `sum`，XTEA 则在更新 `left` 和 `right` 之间更新 `sum`。这些区别决定了两个解密函数不能混用。

---

## 七、编写XTEA解密函数

XTEA 一轮加密的顺序是：

```text
1. 使用旧sum更新left
2. 更新sum
3. 使用新sum更新right
```

解密时必须从最后一步开始撤销：

```text
1. 使用当前sum恢复right
2. 将sum恢复到上一轮
3. 使用恢复后的sum恢复left
```

对应代码为：

```c
void xtea_decrypt(uint cipher[2], uint key[4])
{
    uint sum = 0xC6EF3720U;

    for (int i = 0; i < 32; i++)
    {
        cipher[1] -= (((cipher[0] << 4) ^ (cipher[0] >> 5)) + cipher[0])
                     ^ (key[(sum >> 11) & 3] + sum);

        sum += 1640531527;

        cipher[0] -= (((cipher[1] << 4) ^ (cipher[1] >> 5)) + cipher[1])
                     ^ (key[sum & 3] + sum);
    }
}
```

---

## 八、完成解密脚本

将两组密文、密钥和解密函数填入 `tea6solve.c`。由于每个算法都要处理16字节，需要分别解密两个8字节分组：

```c
tea_decrypt(cipher1, key1);
tea_decrypt(cipher1 + 2, key1);

xtea_decrypt(cipher2, key2);
xtea_decrypt(cipher2 + 2, key2);
```

`cipher1 + 2` 不是向后移动2字节。`cipher1` 是 `uint *`，每个元素占4字节，所以：

```text
cipher1 + 2 → 向后移动2 × 4 = 8字节
```

最后将两段明文连续输出：

```c
printf("%.16s%.16s\n", (char *)cipher1, (char *)cipher2);
```

完整解密脚本可以在此下载：

{% btn '/files/tea6solve.c', '下载 tea6solve.c', 'fas fa-download', 'blue larger' %}

在 Linux 中编译运行：

```bash
gcc tea6solve.c -o tea6solve
./tea6solve
```

得到：

```text
moectf{Wh4t_a_n1ce_cup_0f_TEA!!}
```

---

## 九、Flag

```text
moectf{Wh4t_a_n1ce_cup_0f_TEA!!}
```

---

## 十、总结

这道题的难点不在于记忆 TEA 和 XTEA 的标准代码，而在于把 IDA 中的伪代码翻译成容易理解的数组和指针操作。

本题需要掌握以下几点：

```text
1. 通过函数参数 s 和 s+16 判断输入的分段。
2. 根据指针类型判断 a1+2 的真实字节偏移。
3. 从 .rodata 数据段提取密钥和密文。
4. 区分 db 原始字节和 dd 32位整数的显示方式。
5. 按小端序将每4字节恢复成 uint32_t。
6. 理解XTEA中 sum 与密钥下标的关系。
7. 解密时严格按照加密步骤的相反顺序撤销。
```

在后续遇到类似题目时，可以先定位“输入如何分组”、“加密结果与哪些常量比较”和“密钥地址从哪里获取”，再进入加密函数还原轮结构，整体思路会清晰很多。
