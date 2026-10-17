---
title: "re复现"
date: 2026-10-17
categories: ["逆向工程"]
tags: ["CTF", "IDA", "ARM64", "Mach-O", "UPX", "动态调试", "Unicorn", "SM4变体"]
description: "从 ARM64 Mach-O 的主函数出发，分析密钥生成、分组解密与字节重排，通过 IDA 远程调试在内存中恢复 flag。"
cover: /imgs/20261017-cover-re.jpg
password: "8888"
abstract: 本文已设置访问密码，请输入密码后查看完整题解。
message: 请输入访问密码后查看本文。
wrong_pass_message: 密码错误，请重新输入。
toc: true
comments: true
---

本题的关键是：程序会先在内存中恢复正确答案，再读取输入并比较。因此，可以先分析数据处理流程，在最后一次变换完成后暂停程序，直接查看内存中的明文。

本文使用 Windows 上的 IDA，通过 Remote GDB 连接本机 ARM64 模拟后端完成调试，并收录解题过程中提供的全部 5 张本题截图。

<!-- more -->

> 本文根据已有解题记录整理，保留原调试结果与截图。本次整理未重新执行附件；文中的地址与运行状态以原复现环境为准。

## 1. 识别附件格式

附件包含两个主要文件：

| 文件 | 用途 |
| --- | --- |
| `FragmentsRejoined-final` | 待分析的可执行程序 |
| `gen_key.py` | 生成程序解密时使用的密钥 |

先使用 Exeinfo PE 查看程序：

![图 1：Exeinfo PE 识别题目文件](/imgs/20261017-fragments-rejoined/01-exeinfo.png)

**图 1：底部的 `NOT Win EXE` 表明它不是 Windows PE 程序。**

截图中的工具同时给出了 `Mac OS X Mach-O 64bit` 提示，但其中的 `Intel executable` 架构提示并不准确。本题脱壳文件的 Mach-O 头中，`cputype` 为 `0x0100000C`，对应 ARM64；IDA 中的 `X0`、`X29`、`BL`、`ADRP` 等寄存器和指令也与 AArch64 一致。

因此，应将其作为 **macOS ARM64 的 Mach-O 程序**分析，不能选择 Local Windows debugger 直接运行。

原始程序带有 UPX 压缩壳。先用 UPX 的 `-d` 参数脱壳，得到 `FragmentsRejoined-unpacked`，再交给 IDA 分析。本文后续地址均以脱壳文件的原始加载基址 `0x100000000` 为准。

## 2. 打开 main，确定校验方式

用 IDA 打开脱壳后的程序，等待分析结束，定位 `main`，按 **F5** 查看伪代码。

![图 2：main 函数整体逻辑与局部变量](/imgs/20261017-fragments-rejoined/02-main-overview.png)

**图 2：程序先获取密钥，再处理内置密文，随后才提示输入 flag。**

主要变量可以整理为：

| 变量 | 含义 |
| --- | --- |
| `v15` | 16 字节密钥 |
| `v14` | 48 字节数据缓冲区，依次保存混淆密文、解密结果和重排结果 |
| `v13` | 32 个轮常量 |
| `v12` | 32 个轮密钥 |
| `v11` | 一个 16 字节分组的解密输出 |
| `v7` | 从处理结果中筛选出的正确答案字符串 |
| `v5` | 用户输入的字符串 |

这些 `vXX` 名称是反编译器生成的临时名称，换数据库或重新反编译后可能变化。分析时应根据变量的数据来源和用途判断。

先看主函数的前半段：

![图 3：读取密钥、异或、分组解密与字节旋转](/imgs/20261017-fragments-rejoined/03-decrypt-flow.png)

**图 3：恢复答案所需的处理步骤已经集中出现在 main 中。**

有效逻辑可简化为：

```python
key = read_key_from_gen_key()

# 第一步：去掉额外的异或混淆。
buf = bytearray(x ^ 0x5A for x in ENCRYPTED_BYTES_XOR)

# 第二步：每 16 字节解密一次，共处理三个分组。
ck = init_CK()
for offset in range(0, 48, 16):
    rk = key_schedule(key, ck)
    buf[offset:offset + 16] = block_decrypt(buf[offset:offset + 16], rk)

# 第三步：整个 48 字节数组循环右移 7 个字节。
buf = buf[-7:] + buf[:-7]

# 第四步：只留下可打印 ASCII 字符。
expected = bytes(x for x in buf if 0x20 <= x <= 0x7E)
```

注意，`rotate_right_bytes(v14, 0x30, 7)` 是对**整段数组右移 7 个字节**，不是把每个字节的二进制位循环右移 7 位。

接着看主函数后半段：

![图 4：读取输入，并与恢复出的字符串比较](/imgs/20261017-fragments-rejoined/04-flag-comparison.png)

**图 4：`v7` 是程序计算出的答案，`v5` 是输入，比较结果决定输出 `Correct!` 还是 `Wrong!`。**

程序会去掉输入末尾可能存在的 `\r`，然后执行字符串比较。由此可确定：正确答案在输入比较之前已经以明文形式存在于内存中。

## 3. 分析 gen_key.py，得到密钥

程序通过以下命令获取密钥：

```text
python3 gen_key.py
```

脚本中虽然有异或、移位、加减等操作，但最终可以抵消。例如，第一个字节的过程为：

```python
hidden0 = ord('S') ^ m0
hidden0 = hidden0 ^ 0xAA
b0 = (hidden0 ^ 0xAA) ^ m0
```

因为同一个值连续异或两次会恢复原值，所以 `b0` 就是字符 `S`。另外两个字节分别恢复为 `M` 和 `4`，其余位置补零。

最终 16 字节密钥为：

```text
534d3400000000000000000000000000
```

用 Python 表示为：

```python
key = b"SM4" + b"\x00" * 13
```

这里补的是二进制零字节 `00`，不是字符 `0`。

## 4. Windows 上的调试环境

本次使用的环境为：

| 组成 | 作用 |
| --- | --- |
| IDA 9.3 | 显示反汇编、设置断点、单步、查看寄存器和内存 |
| Remote GDB debugger | 通过 GDB 远程协议控制后端 |
| Unicorn ARM64 模拟器 | 执行题目原本的 ARM64 指令 |
| 本题 GDB 服务 | 连接 IDA 与模拟器，监听 `127.0.0.1:23949` |

实际连接关系是：

```text
Windows 上的 IDA
        ↓ GDB 远程协议，本机连接
127.0.0.1:23949
        ↓
Unicorn 执行原始 ARM64 解密指令
```

本后端提前准备密钥、栈、堆以及必要的运行库操作，从 `0x100000694` 开始执行，到 `0x100000768` 停止。异或循环、密钥扩展、分组解密和字节旋转执行的是题目原本的机器指令。

这是为本题准备的解密核心模拟环境，不是完整 macOS；后面的标准输入与最终比较未在该后端中完整运行。IDA 中的断点、单步与内存观察方法则可以迁移到其他调试目标。

连接时，在 IDA 中选择：

```text
Debugger → Switch debugger → Remote GDB debugger
```

然后打开 `Debugger → Process options`，设置：

| 配置项 | 本次使用的值 |
| --- | --- |
| Hostname | `127.0.0.1` |
| Port | `23949` |
| 目标架构 | ARM64 / AArch64，小端序 |

本次连接时取消了 GDB 专用选项中的强制 `No-Ack mode`，也关闭了与 AArch64 无关的 `Update CPSR to match Thumb mode`。其他服务的选项应按其实际协议能力配置。

这里的 `127.0.0.1:23949` 不是公共调试服务。需要先在本机运行原解题环境中的专用 GDB 后端；仅安装 IDA 或填写端口并不能自动创建这一环境。本文没有附上该后端脚本，不能把上述配置当作独立、完整的启动教程。

确认服务启动后，执行：

```text
Debugger → Attach to process
```

选择 `<attach to the process started on target>`。连接成功后，关闭提示框，当前 PC 应为 `0x100000694`。

## 5. 选择断点：在答案生成后暂停

动态调试的重点是根据数据处理流程选断点，而不是随便找一个地址停下。

本题有两个很合适的观察位置：

| 地址 | 执行进度 | 可以看到的内容 |
| --- | --- | --- |
| `0x100000758` | 三个分组已经解密，尚未进行最后的旋转 | 字符已恢复，但顺序还需要调整 |
| `0x100000768` | `rotate_right_bytes` 已返回 | 完整 flag 已出现在缓冲区中 |

先在第一个位置暂停：

1. 点击反汇编窗口，按 **G**，输入 `100000758`。
2. 按 **F2** 设置断点。
3. 按 **F9** 继续运行，等待命中断点。

**按 G 只是浏览代码；只有当前 PC 到达该位置，才表示程序执行到了那里。**

此时主函数缓冲区的位置可从栈变量确定：

```text
buf = X29 - 0x58
```

本模拟环境中，主函数帧指针和缓冲区地址分别为：

```text
X29 = 0x20001E200
buf = 0x20001E1A8
```

切到 Hex View，在该窗口内按 **G** 跳转到 `20001E1A8`，查看 48 字节。

分组解密完成后的数据为：

```python
b'c_and_junk_rc4_reversed}\x01' + b'\x00' * 16 + b'flag{sm'
```

可以看到，`flag{sm` 位于数组末尾，说明还有最后一步重排。

接下来在 `100000768` 按 **F2** 设置第二个断点，按 **F9** 继续。程序执行完 `rotate_right_bytes` 后暂停，此时再次查看同一缓冲区，得到：

```python
b'flag{smc_and_junk_rc4_reversed}\x01' + b'\x00' * 16
```

数组末尾的 7 个字节正是 `flag{sm`，循环右移后被移动到开头，字符串便恢复为正确顺序。

## 6. 从寄存器指向的内存中看到 flag

实际调试过程中，还在寄存器窗口观察到了下面的结果：

![图 5：X0 指向的内存中出现完整 flag](/imgs/20261017-fragments-rejoined/05-flag-in-memory.png)

**图 5：IDA 对寄存器指向的内存进行解析，在右侧显示出字符串内容。**

截图中，`X0` 的数值为：

```text
0x20001DFC0
```

右侧可以读到：

```text
flag{smc_and_junk_rc4_reversed}
```

这里要区分两件事：

- `X0` 本身保存的是一个 64 位数值，此时被用作内存地址。
- 字符串保存在对应内存中，右侧是 IDA 自动解析后的提示。

实际操作时，可以对 X0 指向的地址使用 **Follow in dump**，或复制地址，在 Hex View 中按 **G** 跳转查看。

截图中的 `0x20001DFC0` 与前文主函数 `v14` 的地址 `0x20001E1A8` 不同。旋转函数还会使用临时存储，同一段明文可能在多个位置存在；仅凭这张寄存器截图不能确定暂停指令或内存对象的完整生命周期。因此，按前面的固定断点复现时，应优先依据当时的 `X29 - 0x58` 定位主函数缓冲区。

同样，X0 并不保证始终指向 flag。寄存器会随着后续指令变化，栈地址也可能因运行环境不同而改变。

## 7. 为什么这样能得到正确答案

程序的校验结构可以概括为：

```python
expected = recover_secret_from_builtin_ciphertext()
user_input = read_input()

if user_input == expected:
    print("Correct!")
else:
    print("Wrong!")
```

因为程序自己持有密文，也能获取解密所需的密钥，所以它能够独立恢复 `expected`。我们让程序执行到恢复完成的位置，再通过调试器读取这段数据，就能得到它准备拿来比较的正确答案。

断点的作用是**暂停执行，保留这一时刻的寄存器和内存状态**。明文来自程序的解密和重排运算。

最后，主函数只把 `0x20` 到 `0x7E` 范围内的字节加入答案字符串。因此，缓冲区末尾的 `\x01` 和零字节都不会参与最终字符串比较。

## 8. 补充：为什么不能直接套标准 SM4

若进一步静态分析 `init_CK`、`key_schedule` 和 `block_decrypt`，可以发现其结构接近 SM4，但有两处修改。

第一，轮常量按下面的方式生成：

```python
CK[i] = (0x01020304 + i * 0x01010101) & 0xFFFFFFFF
```

第二，密钥扩展也使用与数据轮相同的线性变换。设 S 盒替换结果为 `B`：

```text
T(x) = B XOR ROL32(B, 2) XOR ROL32(B, 10)
         XOR ROL32(B, 18) XOR ROL32(B, 24)
```

生成轮密钥的有效表达式为：

```python
rk[i] = K0 ^ T(K1 ^ K2 ^ K3 ^ CK[i])
```

数据解密时倒序使用 32 个轮密钥。这些修改意味着直接把密文交给标准 SM4 工具，不能复现本题结果。动态执行原始代码时，会自然执行到这些修改过的运算。

flag 中的 `rc4` 是答案文字的一部分，不能仅凭这个字符串认定分组解密算法是 RC4。

## 9. 最终 flag

```text
flag{smc_and_junk_rc4_reversed}
```

本题可迁移的解题思路是：先确认程序如何产生比较目标，再找到最终变换完成的位置，下断点并检查缓冲区。有些题只对输入计算摘要或执行约束校验，不会在内存中生成明文答案，需要根据实际校验逻辑继续分析。

参考：[IDA 调试操作与进程配置](https://docs.hex-rays.com/user-guide/user-interface/menu-bar/debugger/process-control/start-process)、[IDA 远程调试说明](https://docs.hex-rays.com/9.0/user-guide/debugger/remote-debugging)。

