#include "primitive.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <mutex>

extern "C"
{
#include <relic/relic.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <sys/socket.h>
}

using namespace std;

// 全局映射表，用于存储字符串到椭圆曲线点的映射关系
static std::unordered_map<std::string, std::string> string_to_point_map;
static std::unordered_map<std::string, std::string> point_to_string_map;
static std::mutex mapping_mutex;

void pi(ep_t out, const string &in)
{
    std::lock_guard<std::mutex> lock(mapping_mutex);

    // 检查输入长度限制
    if (in.size() > 64)
    {
        cerr << "Error: Input string too long for pi mapping (max 64 bytes)" << endl;
        ep_set_infty(out);
        return;
    }

    // 检查是否已经存在映射
    auto it = string_to_point_map.find(in);
    if (it != string_to_point_map.end())
    {
        // 从存储的二进制数据恢复椭圆曲线点
        ep_read_bin(out, (const unsigned char *)it->second.c_str(), 33);
        return;
    }

    // 使用确定性哈希映射到椭圆曲线点
    unsigned char hash_input[128];
    memset(hash_input, 0, sizeof(hash_input));

    // 创建唯一的哈希输入：字符串长度 + 字符串内容 + 固定盐值
    uint32_t str_len = static_cast<uint32_t>(in.size());
    const char *salt = "BAMBOO_PI_SALT_2024";

    memcpy(hash_input, &str_len, sizeof(uint32_t));
    memcpy(hash_input + sizeof(uint32_t), in.c_str(), in.size());
    memcpy(hash_input + sizeof(uint32_t) + in.size(), salt, strlen(salt));

    // 使用SHA256哈希
    unsigned char hash[32];
    SHA256(hash_input, sizeof(uint32_t) + in.size() + strlen(salt), hash);

    // 将哈希映射到椭圆曲线点
    ep_map(out, hash, 32);

    // 存储映射关系
    unsigned char point_bin[33];
    ep_write_bin(point_bin, 33, out, 1);

    string point_str((char *)point_bin, 33);
    string_to_point_map[in] = point_str;
    point_to_string_map[point_str] = in;
}

void pi_inv(string &out, ep_t in)
{
    std::lock_guard<std::mutex> lock(mapping_mutex);

    // 将椭圆曲线点序列化为二进制字符串
    unsigned char point_bin[33];
    ep_write_bin(point_bin, 33, in, 1);
    string point_str((char *)point_bin, 33);

    // 首先在映射表中查找对应的字符串
    auto it = point_to_string_map.find(point_str);
    if (it != point_to_string_map.end())
    {
        out = it->second;
        return;
    }

    // 如果直接查找失败，尝试使用原始的解析方法作为后备
    // 这是为了兼容可能存在的旧数据或特殊情况
    unsigned char buf[33];
    ep_write_bin(buf, 33, in, 1);

    // 检查第5字节是否可能是有效的长度
    char len = buf[4];
    if (len >= 0 && len <= 19 && (4 + sizeof(char) + len <= 33))
    {
        try
        {
            string candidate((char *)buf + 4 + sizeof(char), len);
            // 验证这个候选字符串是否合理（只包含可打印字符）
            bool valid = true;
            for (char c : candidate)
            {
                if (c < 32 || c > 126)
                { // 不是可打印ASCII字符
                    valid = false;
                    break;
                }
            }
            if (valid && !candidate.empty())
            {
                out = candidate;
                return;
            }
        }
        catch (...)
        {
            // 忽略异常，继续下面的处理
        }
    }

    // 如果所有方法都失败，返回空字符串
    out.clear();
}

void Hash_H1(ep_t out, const std::string &in)
{
    unsigned char buf[64];
    SHA256((const unsigned char *)in.c_str(), in.length(), buf);

    ep_map(out, buf, 32);
}

void Hash_H2(ep_t out, const std::string &in)
{
    unsigned char buf[64];
    SHA384((const unsigned char *)in.c_str(), in.length(), buf);

    ep_map(out, buf, 48);
}

void Hash_G(ep_t out, const std::string &in)
{
    unsigned char buf[64];
    SHA512((const unsigned char *)in.c_str(), in.length(), buf);

    ep_map(out, buf, 64);
}

void print_hex(const void *data, int len)
{
    unsigned char *p = (unsigned char *)data;
    for (int i = 0; i < len; i++)
    {
        printf("%02X ", p[i]);
    }
    printf("\n");
}

void recv_data(int sock, unsigned char *buf, int length)
{
    int recv_len = 0;
    while (recv_len < length)
    {
        recv_len += recv(sock, buf + recv_len, length - recv_len, 0);
    }
}

void recv_bytes(int sock, std::string &str_out)
{
    int buf_len;
    unsigned char buf[512];

    recv_data(sock, (unsigned char *)&buf_len, sizeof(int));
    recv_data(sock, buf, buf_len);
    str_out.assign((const char *)buf, buf_len);
}

void send_bytes(int sock, const std::string &str_in)
{
    int len;

    len = str_in.size();
    send(sock, &len, sizeof(int), 0);
    send(sock, str_in.c_str(), len, 0);
}