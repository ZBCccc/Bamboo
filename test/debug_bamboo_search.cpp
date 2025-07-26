#include "primitive.h"
#include <iostream>
#include <string>

extern "C" {
#include <relic/relic.h>
}

using namespace std;

void debug_bamboo_search_process() {
    cout << "=== Debugging Bamboo Search Process ===" << endl;
    
    core_init();
    ep_param_set(NIST_P256);
    
    // 模拟 BambooClient::DataUpdate 中的过程
    cout << "1. Simulating DataUpdate process..." << endl;
    
    string doc_id = "file-0";
    string s = "1" + doc_id;  // "1file-0"
    
    cout << "   Document ID: " << doc_id << endl;
    cout << "   String to encrypt: " << s << endl;
    
    // 步骤1: pi(e_tmp, s)
    ep_t e_tmp;
    ep_new(e_tmp);
    pi(e_tmp, s);
    
    cout << "   After pi(e_tmp, s): ";
    unsigned char buf[33];
    ep_write_bin(buf, 33, e_tmp, 1);
    for (int i = 0; i < 10; i++) {
        printf("%02X ", buf[i]);
    }
    cout << endl;
    
    // 步骤2: 模拟加密过程 ep_mul(e_tmp, e_tmp, K1)
    bn_t K1, ord;
    bn_new(K1);
    bn_new(ord);
    ep_curve_get_ord(ord);
    bn_rand_mod(K1, ord);
    
    ep_mul(e_tmp, e_tmp, K1);
    
    cout << "   After ep_mul(e_tmp, e_tmp, K1): ";
    ep_write_bin(buf, 33, e_tmp, 1);
    for (int i = 0; i < 10; i++) {
        printf("%02X ", buf[i]);
    }
    cout << endl;
    
    // 步骤3: 模拟 BambooClient::DecryptResult 中的解密过程
    cout << "\n2. Simulating DecryptResult process..." << endl;
    
    // 计算 K1 的逆元
    bn_t d, c, e;
    bn_new(d);
    bn_new(c);
    bn_new(e);
    bn_gcd_ext(c, d, e, K1, ord);
    
    // 执行解密: ep_mul(e_tmp, e_tmp, d)
    ep_mul(e_tmp, e_tmp, d);
    
    cout << "   After ep_mul(e_tmp, e_tmp, d): ";
    ep_write_bin(buf, 33, e_tmp, 1);
    for (int i = 0; i < 10; i++) {
        printf("%02X ", buf[i]);
    }
    cout << endl;
    
    // 步骤4: 尝试恢复字符串
    string recovered;
    pi_inv(recovered, e_tmp);
    
    cout << "   pi_inv result: \"" << recovered << "\"" << endl;
    cout << "   Expected: \"" << s << "\"" << endl;
    cout << "   Match: " << (s == recovered ? "YES" : "NO") << endl;
    
    // 步骤5: 如果不匹配，尝试分析原因
    if (s != recovered) {
        cout << "\n3. Analyzing why recovery failed..." << endl;
        
        // 重新创建原始点
        ep_t original_point;
        ep_new(original_point);
        pi(original_point, s);
        
        cout << "   Original point: ";
        ep_write_bin(buf, 33, original_point, 1);
        for (int i = 0; i < 10; i++) {
            printf("%02X ", buf[i]);
        }
        cout << endl;
        
        cout << "   Decrypted point: ";
        ep_write_bin(buf, 33, e_tmp, 1);
        for (int i = 0; i < 10; i++) {
            printf("%02X ", buf[i]);
        }
        cout << endl;
        
        bool points_equal = (ep_cmp(original_point, e_tmp) == RLC_EQ);
        cout << "   Points equal: " << (points_equal ? "YES" : "NO") << endl;
        
        if (points_equal) {
            cout << "   Points are equal but pi_inv failed - lookup table issue" << endl;
        } else {
            cout << "   Points are different - encryption/decryption issue" << endl;
        }
        
        ep_free(original_point);
    }
    
    // 清理
    ep_free(e_tmp);
    bn_free(K1);
    bn_free(ord);
    bn_free(d);
    bn_free(c);
    bn_free(e);
    
    core_clean();
}

int main() {
    debug_bamboo_search_process();
    return 0;
}
