#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include <string>
#include <gmpxx.h>

extern "C" {
#include<relic/relic.h>
};

#define A_MAX (500000)

enum NetworkOp
{
    OP_SETUP,
    OP_SAVE_CIPHER,
    OP_SRCH_QRY,
    OP_KEY_UPDT,
    OP_ECDH,
    OP_BACKUP_EDB,
    OP_LOAD_EDB,
    OP_SAVE_BATCH
};

void pi(ep_t out, const std::string &in);

void pi_inv(std::string &out, ep_t in);

void Hash_H1(ep_t out, const std::string &in);

void Hash_H2(ep_t out, const std::string &in);

void Hash_G(ep_t out, const std::string &in);

void print_hex(const void *data, int len);

void recv_data(int sock, unsigned char *buf, int length);
void recv_bytes(int sock, std::string &str_out);
void send_bytes(int sock, const std::string &str_in);

#endif
