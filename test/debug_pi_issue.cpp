#include "primitive.h"
#include <iostream>
#include <string>

extern "C"
{
#include <relic/relic.h>
}

using namespace std;

void print_point_hex(const char *label, ep_t point)
{
    unsigned char buf[33];
    ep_write_bin(buf, 33, point, 1);

    cout << label << ": ";
    for (int i = 0; i < 33; i++)
    {
        printf("%02X ", buf[i]);
    }
    cout << endl;

    // 特别关注第5字节（索引4）
    cout << "  Length byte (index 4): " << static_cast<int>(buf[4]) << " (0x" << hex << static_cast<int>(buf[4]) << dec << ")" << endl;
}

void debug_pi_operations()
{
    cout << "=== Debugging pi and pi_inv operations ===" << endl;

    core_init();
    ep_param_set(NIST_P256);

    // 测试字符串
    string test_str = "1file-0"; // 这是实际使用的格式
    cout << "Original string: \"" << test_str << "\" (length: " << test_str.size() << ")" << endl;

    // 1. 通过pi函数映射到椭圆曲线点
    ep_t original_point;
    ep_new(original_point);

    pi(original_point, test_str);
    print_point_hex("After pi()", original_point);

    // 2. 测试pi_inv是否能正确恢复
    string recovered1;
    pi_inv(recovered1, original_point);
    cout << "Recovered by pi_inv: \"" << recovered1 << "\"" << endl;
    cout << "Match: " << (test_str == recovered1 ? "YES" : "NO") << endl
         << endl;

    // 3. 模拟BambooClient::DataUpdate中的完整流程
    cout << "=== Simulating BambooClient::DataUpdate ===" << endl;

    // 模拟密钥
    bn_t K, K1, ord;
    bn_new(K);
    bn_new(K1);
    bn_new(ord);
    ep_curve_get_ord(ord);
    bn_rand_mod(K, ord);
    bn_rand_mod(K1, ord);

    // 模拟token
    string token = "randomtoken12345"; // 16字节token

    // 步骤1: pi(e_D, token)
    ep_t e_D, e_tmp;
    ep_new(e_D);
    ep_new(e_tmp);

    pi(e_D, token);
    print_point_hex("e_D after pi(token)", e_D);

    // 步骤2: Hash_H2(e_tmp, tk1) + ep_add + ep_mul
    string tk1 = "newtokenabcdefgh"; // 新token
    Hash_H2(e_tmp, tk1);
    ep_add(e_D, e_tmp, e_D);
    ep_mul(e_D, e_D, K);
    print_point_hex("e_D after encryption operations", e_D);

    // 步骤3: 序列化存储（模拟网络传输和数据库存储）
    unsigned char buf[33];
    ep_write_bin(buf, 33, e_D, 1);
    string stored_data;
    stored_data.assign((char *)buf, 33);

    // 步骤4: 从存储中读取（模拟服务器端）
    ep_t loaded_point;
    ep_new(loaded_point);
    ep_read_bin(loaded_point, (unsigned char *)stored_data.c_str(), 33);
    print_point_hex("Loaded from storage", loaded_point);

    // 步骤5: 模拟BambooServer::Search中的解密过程
    cout << "=== Simulating BambooServer::Search ===" << endl;

    // 计算MskD (这里简化，实际中MskD是通过Hash_H2(token)*K计算的)
    ep_t MskD;
    ep_new(MskD);
    Hash_H2(MskD, token);
    ep_mul(MskD, MskD, K);
    print_point_hex("MskD", MskD);

    // 执行 ele1 = loaded_point - MskD
    ep_t ele1;
    ep_new(ele1);
    ep_sub(ele1, loaded_point, MskD);
    print_point_hex("After subtraction (loaded_point - MskD)", ele1);

    // 计算逆标量 d
    bn_t c, d, e;
    bn_new(c);
    bn_new(d);
    bn_new(e);
    bn_gcd_ext(c, d, e, K, ord);

    // 执行 ele1 = ele1 * d
    ep_mul(ele1, ele1, d);
    print_point_hex("After multiplication by inverse", ele1);

    // 尝试恢复token
    string recovered_token;
    pi_inv(recovered_token, ele1);
    cout << "Recovered token: \"" << recovered_token << "\"" << endl;
    cout << "Expected token: \"" << token << "\"" << endl;
    cout << "Match: " << (token == recovered_token ? "YES" : "NO") << endl;

    // 清理
    ep_free(original_point);
    ep_free(e_D);
    ep_free(e_tmp);
    ep_free(loaded_point);
    ep_free(MskD);
    ep_free(ele1);
    bn_free(K);
    bn_free(K1);
    bn_free(ord);
    bn_free(c);
    bn_free(d);
    bn_free(e);

    core_clean();
}

int main()
{
    debug_pi_operations();
    return 0;
}
