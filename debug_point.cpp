#include <iostream>
#include <cstring>
extern "C" {
#include <relic/relic.h>
}

int main() {
    core_init();
    ep_param_set(NIST_P256);
    
    ep_t point;
    ep_new(point);
    ep_rand(point);
    
    unsigned char buf[33];
    ep_write_bin(buf, 33, point, 1);
    
    std::cout << "椭圆曲线点二进制格式 (33字节):" << std::endl;
    for (int i = 0; i < 33; i++) {
        printf("buf[%2d] = 0x%02X\n", i, buf[i]);
    }
    
    ep_free(point);
    core_clean();
    return 0;
}
