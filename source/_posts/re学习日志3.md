---
title: "re学习日志3"
date: 2026-05-06
categories: ["逆向工程"]
tags: ["学习笔记"]
description: 记录re的学习过程。
cover: /img/5036443ba2bb03030effc43b3b1673f0.png
toc: true  
comments: true  
---
<!-- 正文从这里开始 -->




#re学习日志3：从 OLLVM 迷雾到 TEA 算法还原

一、 程序初步分析1. 基础信息探测首先使用 file 指令确定文件属性：Bashfile ollvm
# 输出：ELF 64-bit LSB pie executable, x86-64, stripped
由于程序是 Stripped（剔除了符号表），我们无法直接看到函数名。接着利用 strings 寻找提示字符：输入提示：Enter the flag :输入限制：%32s（暗示 Flag 长度为 32 字节）结果反馈：Right!!! 或 Sorry...2. 逻辑结构推测虽然函数名丢失，但通过交叉引用字符串可以锁定 main 函数逻辑：获取 32 字节输入。调用核心校验函数 check。根据 check 的布尔返回值决定输出结果。

二、 混淆识别：OLLVM进入校验函数后，代码呈现出极其典型的 OLLVM (Obfuscator-LLVM) 混淆特征：控制流平坦化 (Flattening)：逻辑块被分散，由一个中心分发器和状态变量（如 v13）驱动循环跳转。虚假分支 (Bogus Control Flow)：代码中充斥着永远为真或为假的条件判断。
 自动化去混淆方案手动修复平坦化极其耗时，这里推荐使用 IDA 插件 D810：加载规则：在 D810 配置中选中 default_unflattening_ollvm.json。执行修复：对目标函数执行 Unflattening，它会自动识别并剔除虚假跳转。刷新伪代码：按下 F5 后，逻辑会由“面条状”回归为线性结构。

三、 算法识别：TEA 加密在还原后的 sub_1170 函数中，我们能清晰识别出加密核心。1. 核心常量识别函数中出现了一个关键的负数常量 -1640531527。将其转换为十六进制无符号数：$$-1640531527 \rightarrow 0x9E3779B9$$这是 TEA (Tiny Encryption Algorithm) 的标志性黄金分割常数（Delta）。2. 加密逻辑公式在经过 D810 还原后，复杂的 MBA 混淆被还原为标准的位运算：Sum 累加：$sum += delta$更新 $v_0$：$$v_0 += ((v_1 \ll 4) + k_0) \oplus (v_1 + sum) \oplus ((v_1 \gg 5) + k_1)$$更新 $v_1$：$$v_1 += ((v_0 \ll 4) + k_2) \oplus (v_0 + sum) \oplus ((v_1 \gg 5) + k_3)$$

四、 
数据提取
1. 密钥提取在内存或数据块中找到 16 字节密钥，按 小端序 排列如下：C++uint32_t key[4] = {0x11121314, 0x22232425, 0x33343536, 0x41424344};
2. 密文提取提取程序最终用于 memcmp 的 32 字节密文。这些数据通常存储在 .data 或 .rodata 段。

五、 解密原理与实现TEA 是对称结构。解密时，我们只需逆转加密的每一轮操作：初始 Sum：$$sum = delta \times 32 = 0xC6EF3720$$逆向还原：在每一轮迭代中，必须先解 $v_1$，再解 $v_0$（与加密顺序相反）。符号取反：将加密过程中所有的 += 操作改为 -=。解密核心逻辑 (C++)C++#include <iostream>
#include <cstdint>

void decrypt(uint32_t* v, uint32_t* k) {
    uint32_t v0 = v[0], v1 = v[1];
    uint32_t delta = 0x9e3779b9;
    uint32_t sum = 0xC6EF3720; // delta * 32
    
    for (int i = 0; i < 32; i++) {
        // 先还原最后被加密的 v1
        v1 -= ((v0 << 4) + k[2]) ^ (v0 + sum) ^ ((v0 >> 5) + k[3]);
        // 再还原 v0
        v0 -= ((v1 << 4) + k[0]) ^ (v1 + sum) ^ ((v1 >> 5) + k[1]);
        // sum 递减
        sum -= delta;
    }
    v[0] = v0; v[1] = v1;
}

int main() {
    // 接下来根据提取的密文循环调用 decrypt 即可
    return 0;
}
 总结：在处理 OLLVM 题目时，核心挑战在于通过 D810 还原控制流。一旦识别出标志性的 0x9E3779B9，即可锁定 TEA 算法并编写对应的解密脚本。
