#include "primitive.h"

#include <cassert>
w#include <string>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>

extern "C"
{
#include <relic/relic.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <sys/socket.h>
}

using namespace std;

void pi(ep_t out, const string &in)
{
    unsigned char buf1[128], buf2[128];
    char len;

    if (in.size() > 20 - sizeof(char))
    {
        cerr << "The length of input to mapping pi is illegal" << endl;
        return;
    }
    len = (char)in.size();

    memcpy(buf1, &len, sizeof(char));
    memcpy(buf1 + sizeof(char), in.c_str(), len);

    int attempt_count = 0;
    const int max_attempts = 1000; // 防止无限循环

    while (true)
    {
        attempt_count++;
        if (attempt_count > max_attempts)
        {
            std::cerr << "Max attempts reached in pi function, setting to infinity" << std::endl;
            ep_set_infty(out);
            break;
        }

        ep_rand(out);
        ep_write_bin(buf2, 33, out, 1);
        memcpy(buf2 + 4, buf1, len + sizeof(char));

        err_t error;          // 定义变量存储错误代码
        bool success = false; // 添加成功标志

        RLC_TRY
        {
            // 临时重定向 stderr 以抑制 RELIC 库的错误输出
            int stderr_backup = dup(STDERR_FILENO);
            int devnull = open("/dev/null", O_WRONLY);
            dup2(devnull, STDERR_FILENO);
            close(devnull);

            ep_read_bin(out, buf2, 33);

            // 恢复 stderr
            dup2(stderr_backup, STDERR_FILENO);
            close(stderr_backup);

            if (ep_is_infty(out))
            {
                // std::cout << "Point is infinity, retrying..." << std::endl;
                // 不设置 success，继续循环
            }
            else
            {
                // std::cout << "Success: Valid point read" << std::endl;
                success = true; // 设置成功标志
            }
        }
        RLC_CATCH(error)
        {
            if (error == ERR_NO_VALID)
            {
                // std::cout << "Caught ERR_NO_VALID, retrying..." << std::endl;
            }
            else
            {
                // 其他错误
                // std::cerr << "Unexpected error: " << error << std::endl;
            }
        }

        // 在 TRY-CATCH 块外部检查是否成功
        if (success)
        {
            break;
        }
    }
}

void pi_inv(string &out, ep_t in)
{
    unsigned char buf1[128];
    char len;

    ep_write_bin(buf1, 33, in, 1);
    memcpy(&len, buf1 + 4, sizeof(char));
    out.assign((char *)buf1 + 4 + sizeof(char), len);
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