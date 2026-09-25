#include <stdio.h>
/**
 * @brief 这是一个简短的 TEA 解密指南，依据 IDA 反编译的结果，
 *        正确填充代码中的空缺处，运行后就可以获得 flag！
 * 
 * @attention 请先移步 main 函数，阅读此题拆解！
 */

#define uint unsigned int

/**
 * @brief TEA 解密一个 64 位数据块
 *
 * @param cipher 待解密数据（2×32 bit）
 * @param key    128 位密钥（4×32 bit）
 * @param delta  常量值
 */
void tea_decrypt(uint cipher[2], uint key[4])
{
    /**
     * @brief 解密是加密的逆过程。如果我们已知最终的密文，
     *        并可以通过某种方式恢复上一轮的状态，
     *        那么一直恢复到最初状态，就可以得到明文。
     *        在此例中，加密的逻辑是这样的：
     *     -> 先将 delta 减去 1640531527（或为加上 0x9E3779B9）
     *     -> 更新 cipher[0]
     *     -> 更新 cipher[1]
     *     -> 重复以上操作，循环 32 次
     *        由于更新某一个数据时，另外两个数据和密钥的值都是固定的，
     *        于是在解密时，我们只需要：
     *     -> 反向更新 cipher[1]
     *     -> 反向更新 cipher[0]
     *     -> 将 delta 加上 1640531527（或为减去 0x9E3779B9）
     *     -> 重复以上操作，循环 32 次
     * 
     * @attention 根据 IDA 反编译展示的逻辑，修改下面代码中填 0x0 的部分。 
     */
    uint delta = 0xC6EF3720U;

    for(int i = 1; i <= 32; i++)
    {
        cipher[1] -= (cipher[0] + delta) ^ (16 * cipher[0] + key[2]) ^ ((cipher[0] >> 5) + key[3]);
        cipher[0] -= (cipher[1] + delta) ^ (16 * cipher[1] + key[0]) ^ ((cipher[1] >> 5) + key[1]);
        delta += 1640531527;
    }
}

/**
 * @brief XTEA 解密一个 64 位数据块
 *
 * @param cipher 待解密数据（2×32 bit）
 * @param key    128 位密钥（4×32 bit）
 * @param delta  常量值
 */
void xtea_decrypt(uint cipher[2], uint key[4])
{
    /**
     * @brief 这里的反编译可能会出现 *(_DWORD *)。
     *        不用害怕，以 (4LL * (v4 & 3) + xtea_key_address) 为例，
     *        由于 uint 的存储空间为 4 字节，乘上几个 4 就代表偏移量是几。
     * 
     * @attention 根据 IDA 反编译的结果，修改下面代码中填 0x0 的部分。
     */
    uint delta = 0xC6EF3720U;

    for(int i = 1; i <= 32; i++)
    {
        cipher[1] -= (((cipher[0] << 4) ^ (cipher[0] >> 5)) + cipher[0])
                     ^ (key[(delta >> 11) & 3] + delta);
        delta += 1640531527;
        cipher[0] -= (((cipher[1] << 4) ^ (cipher[1] >> 5)) + cipher[1])
                     ^ (key[delta & 3] + delta);
    }
}

int main()
{
    /**
     * @brief 你看懂 main 函数的逻辑了吗？
     *        本题分为两个加密部分：
     *        第一，tea_encrypt 函数对 flag 前 16 字节进行加密。
     *        第二，xtea_encrypt 函数对 flag 后 16 字节进行加密。
     *        密文分别存储在 tea_cipher 和 xtea_cipher 中。
     * 
     * @attention 让我们先来解密 tea_encrypt 部分！
     *            请双击 check_tea_part 函数，找到这部分的密文以及密钥。
     */

     /**
      * @brief 密文数组由 4 个 32 位无符号整数构成。
      * 
      * @attention 双击 tea_cipher，填充以下数据！我已经帮你填好一个了。
      */
    uint cipher1[4] = {
        0xB3E7E33E,
        0xB4114672,
        0x8E088C0C,
        0x14B2C329
    };
    /**
     * @brief 密钥数组同样由 4 个 32 位无符号整数构成。
     *        双击 get_tea_key_address，再双击 tea_key，填充...
     *        等一下，我的 32 位整数呢？！实际上，呈现在你面前的 16 字节，
     *        是密钥数组在内存中原始的存储形式。按每四个字节划分，
     *        就可以得到 4 个密钥。那么，第一个密钥是 0x78563412 吗？
     *        并不是！x86-64 机器一般采用小端序存储。记住一点：
     *        低位字节存储在低地址，高位字节存储在高地址。因此，
     *        第一个密钥应该是 0x12345678。
     * 
     * @attention 完成剩下的填充！
     */
    uint key1[4] = {
        0x12345678,
        0x87654321,
        0x13572468,
        0x24681357
    };
    /**
     * @brief 加密算法 TEA 要求加密密钥为 128 比特，密文块分组长度为 64 比特。
     *        这里密文长度为 128 比特，因此要分为两个块分别加密。
     * 
     * @attention 请移步 tea_decrypt 函数，完成解密逻辑编写！
     */
    tea_decrypt(cipher1, key1);
    tea_decrypt(cipher1 + 2, key1);

    /**
     * @brief 祝贺你成功完成第一部分的解密！第二部分是 xtea 解密，
     *        整体思路和 tea 差不多，不过在加密流程中有些许出入。
     * 
     * @attention 填充对应密文。
     */
    uint cipher2[4] = {
        0x60EC68AB,
        0x940251CE,
        0xB427534C,
        0xDF435416
    };
    /**
     * @attention 填充对应密钥。注意：别忘记小端序！
     */
    uint key2[4] = {
        0xDEADBEEF,
        0xCAFEBABE,
        0x11223344,
        0x55667788
    };
    /**
     * @attention 移步 xtea_decrypt，继续完成解密逻辑编写。
     */
    xtea_decrypt(cipher2, key2);
    xtea_decrypt(cipher2 + 2, key2);
    /**
     * @attention 补全所有部分后，运行程序。如果解密正确，就能看到输出的 flag 啦！
     */
    printf("%.16s%.16s\n", (char *)cipher1, (char *)cipher2);

    return 0;
}
