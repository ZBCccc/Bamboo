#include "primitive.h"

#include <string>
#include <cstring>
#include <cstdlib>
#include <iostream>

extern "C"
{
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

    len = (char) in.size();

    memcpy(buf1, &len, sizeof(char));
    memcpy(buf1 + sizeof(char), in.c_str(), len);

    do
    {
        ep_rand(out);
        ep_write_bin(buf2, 33, out, 1);
        memcpy(buf2 + 4, buf1, len + sizeof(char));

    } while (ep_read_bin(out, buf2, 33) != 1);
}

void pi_inv(string &out, ep_t in)
{
    unsigned char buf1[128];
    char len;

    ep_write_bin(buf1, 33, in, 1);
    memcpy(&len, buf1 + 4, sizeof(char));
    out.assign((char *) buf1 + 4 + sizeof(char), len);
}

void Hash_H1(ep_t out, const std::string &in)
{
    unsigned char buf[64];
    SHA256((const unsigned char *) in.c_str(), in.length(), buf);

    ep_map(out, buf, 32);
}

void Hash_H2(ep_t out, const std::string &in)
{
    unsigned char buf[64];
    SHA384((const unsigned char *) in.c_str(), in.length(), buf);

    ep_map(out, buf, 48);
}

void Hash_G(ep_t out, const std::string &in)
{
    unsigned char buf[64];
    SHA512((const unsigned char *) in.c_str(), in.length(), buf);

    ep_map(out, buf, 64);
}

void print_hex(const void *data, int len)
{
    unsigned char *p = (unsigned char *) data;
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

    recv_data(sock, (unsigned char*)&buf_len, sizeof(int));
    recv_data(sock, buf, buf_len);
    str_out.assign((const char*)buf, buf_len);
}

void send_bytes(int sock, const std::string &str_in)
{
    int len;

    len = str_in.size();
    send(sock, &len, sizeof(int), 0);
    send(sock, str_in.c_str(), len, 0);
}