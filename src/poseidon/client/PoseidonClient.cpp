#include "PoseidonClient.h"
#include "../../core/primitive.h"
#include "PoseidonClientStateMemory.h"
#include "relic/relic_ep.h"
#include <iostream>
#include <set>
#include <string>
#include <climits>

extern "C" {
#include <openssl/rand.h>
}

using namespace std;

PoseidonClient::PoseidonClient() {
  bn_new(K1);
  bn_new(K2);
  bn_new(Kx);
  bn_new(Ky);
  bn_new(Kz);

  state = std::make_unique<PoseidonClientStateMemory>();
}

PoseidonClient::~PoseidonClient() {
  bn_clean(K1);
  bn_clean(K2);
  bn_clean(Kx);
  bn_clean(Ky);
  bn_clean(Kz);
}

int PoseidonClient::Setup() {
  bn_t ord; // 大数类型，椭圆曲线的阶

  bn_new(ord);
  ep_curve_get_ord(ord); // 获取当前椭圆曲线的阶
  bn_rand_mod(K1, ord);
  bn_rand_mod(K2, ord);
  bn_rand_mod(Kx, ord);
  bn_rand_mod(Ky, ord);
  bn_rand_mod(Kz, ord);

  this->state->Clear();

  bn_clean(ord);

  return 0;
}

int PoseidonClient::DataUpdate(Metadata &meta, PoseidonOp op,
                               const std::string &keyword,
                               const std::string &id) {
  if (id.length() > 18) {
    cerr << "Error: ID too long (max 18 bytes)" << endl;
    return -1;
  }

  StateCell cell;
  unsigned char buf1[64];
  string tk1, rand1, s;
  ep_t e_addr, e_val, e_lastAddr, e_alpha, e_L, e_D, e_C, e_tmp, e_tmp1;

  ep_new(e_addr);
  ep_new(e_val);
  ep_new(e_lastAddr);
  ep_new(e_alpha);
  ep_new(e_L);
  ep_new(e_D);
  ep_new(e_C);
  ep_new(e_tmp);
  ep_new(e_tmp1);

  if (!this->state->Get(cell, keyword)) {
    RAND_bytes(buf1, 16);
    cell.tk.assign((char *)buf1, 16);
    RAND_bytes(buf1, 16);
    cell.rand.assign((char *)buf1, 16);
    cell.cntw = 0;
  }
  cell.cntw += 1;

  RAND_bytes(buf1, 16);
  tk1.assign((char *)buf1, 16);
  RAND_bytes(buf1, 16);
  rand1.assign((char *)buf1, 16);

  Hash_H1(e_addr, tk1);
  ep_mul(e_addr, e_addr, this->K1);

  if (op == Poseidon_add)
    s = "1" + id;
  else
    s = "0" + id;

  pi(e_val, s);
  ep_mul(e_val, e_val, this->K2);
  Hash_G1(e_tmp, tk1);
  ep_mul(e_tmp, e_tmp, this->K1);
  ep_add(e_val, e_val, e_tmp);

  pi(e_lastAddr, cell.tk);
  Hash_H2(e_tmp, tk1);
  ep_add(e_lastAddr, e_lastAddr, e_tmp);
  ep_mul(e_lastAddr, e_lastAddr, this->K1);

  Hash_H1(e_tmp, s);
  ep_mul(e_tmp, e_tmp, this->Ky);
  string w_cntw = keyword + to_string(cell.cntw);
  Hash_H1(e_tmp1, w_cntw);
  ep_mul(e_tmp1, e_tmp1, this->Kz);
  ep_sub(e_tmp, e_tmp, e_tmp1);
  Hash_G2(e_tmp1, tk1);
  ep_mul(e_tmp1, e_tmp1, this->K1);
  ep_add(e_alpha, e_tmp, e_tmp1);

  Hash_H1(e_C, keyword);
  ep_mul(e_C, e_C, this->Kx);
  Hash_H1(e_tmp, s);
  ep_mul(e_tmp, e_tmp, this->Ky);
  ep_add(e_C, e_C, e_tmp);
  Hash_G1(e_tmp, rand1);
  ep_mul(e_tmp, e_tmp, this->K1);
  ep_add(e_C, e_C, e_tmp);

  Hash_H1(e_L, rand1);
  ep_mul(e_L, e_L, this->K1);

  pi(e_D, cell.rand);
  Hash_H2(e_tmp, rand1);
  ep_add(e_D, e_tmp, e_D);
  ep_mul(e_D, e_D, this->K1);

  ep_write_bin(buf1, 33, e_addr, 1);
  meta.addr.assign((char *)buf1, 33);

  ep_write_bin(buf1, 33, e_val, 1);
  meta.val.assign((char *)buf1, 33);

  ep_write_bin(buf1, 33, e_lastAddr, 1);
  meta.lastAddr.assign((char *)buf1, 33);

  ep_write_bin(buf1, 33, e_alpha, 1);
  meta.alpha.assign((char *)buf1, 33);

  ep_write_bin(buf1, 33, e_L, 1);
  meta.L.assign((char *)buf1, 33);

  ep_write_bin(buf1, 33, e_D, 1);
  meta.D.assign((char *)buf1, 33);

  ep_write_bin(buf1, 33, e_C, 1);
  meta.C.assign((char *)buf1, 33);

  cell.tk = tk1;
  cell.rand = rand1;
  this->state->Put(cell, keyword);
  ep_free(e_addr);
  ep_free(e_val);
  ep_free(e_lastAddr);
  ep_free(e_alpha);
  ep_free(e_L);
  ep_free(e_D);
  ep_free(e_C);
  ep_free(e_tmp);
  ep_free(e_tmp1);

  return 0;
}

int PoseidonClient::Trapdoor(TrapdoorMetadata &td,
                             const std::vector<std::string> &keywords) {
  unsigned char buf[64];
  StateCell cell;
  string s;
  int mid;
  ep_t e_L, e_xtk, e_TD, e_TC, e_addr, e_val, e_alpha, e_last, e_tmp;

  ep_new(e_L);
  ep_new(e_xtk);
  ep_new(e_TD);
  ep_new(e_TC);
  ep_new(e_addr);
  ep_new(e_val);
  ep_new(e_alpha);
  ep_new(e_last);
  ep_new(e_tmp);

  string w1 = keywords[0];
  int cnt_w1 = INT_MAX;

  // 找到出现次数最少的关键词
  int cnt;
  string tk, rand;
  for (auto &w : keywords) {
    if (!this->state->Get(cell, w)) {
      return -1;
    }

    cnt = cell.cntw;
    tk = cell.tk;
    rand = cell.rand;

    Hash_H1(e_L, rand);
    ep_mul(e_L, e_L, this->K1);

    Hash_H2(e_TD, rand);
    ep_mul(e_TD, e_TD, this->K1);

    Hash_G1(e_TC, rand);
    ep_mul(e_TC, e_TC, this->K1);

    string L, TD, TC;
    ep_write_bin(buf, 33, e_L, 1);
    L.assign((char *)buf, 33);

    ep_write_bin(buf, 33, e_TD, 1);
    TD.assign((char *)buf, 33);

    ep_write_bin(buf, 33, e_TC, 1);
    TC.assign((char *)buf, 33);

    TKLMetadata tkl;
    tkl.L = L;
    tkl.TD = TD;
    tkl.TC = TC;
    td.TKL.push_back(tkl);

    if (cnt_w1 > cell.cntw) {
      cnt_w1 = cell.cntw;
      w1 = w;
    }
  }

  this->state->Get(cell, w1);
  string tkw1 = cell.tk;

  Hash_H1(e_addr, tkw1);
  ep_mul(e_addr, e_addr, this->K1);

  Hash_H2(e_last, tkw1);
  ep_mul(e_last, e_last, this->K1);

  Hash_G1(e_val, tkw1);
  ep_mul(e_val, e_val, this->K1);

  Hash_G2(e_alpha, tkw1);
  ep_mul(e_alpha, e_alpha, this->K1);

  bn_write_bin(buf, 32, this->K1);
  td.K1.assign((char *)buf, 32);

  string addr, val, last, alpha;
  ep_write_bin(buf, 33, e_addr, 1);
  addr.assign((char *)buf, 33);
  td.STKL.push_back(addr);

  ep_write_bin(buf, 33, e_val, 1);
  val.assign((char *)buf, 33);
  td.STKL.push_back(val);

  ep_write_bin(buf, 33, e_alpha, 1);
  alpha.assign((char *)buf, 33);
  td.STKL.push_back(alpha);

  ep_write_bin(buf, 33, e_last, 1);
  last.assign((char *)buf, 33);
  td.STKL.push_back(last);

  int cntw1 = cell.cntw;
  td.XTKL.reserve(cntw1);
  int n = keywords.size();
  for (int j = cntw1; j > 0; j--) {
    string w1j = w1 + to_string(j);
    std::vector<std::string> inner;
    inner.reserve(n);
    for (int k = 2; k <= n; k++) {
      Hash_H1(e_xtk, keywords[k - 1]);
      ep_mul(e_xtk, e_xtk, this->Kx);

      Hash_H1(e_tmp, w1j);
      ep_mul(e_tmp, e_tmp, this->Kz);

      ep_add(e_xtk, e_xtk, e_tmp);

      string xtk;
      ep_write_bin(buf, 33, e_xtk, 1);
      xtk.assign((char *)buf, 33);
      inner.push_back(xtk);
    }
    td.XTKL.push_back(std::move(inner));
  }

  ep_free(e_L);
  ep_free(e_xtk);
  ep_free(e_TD);
  ep_free(e_TC);
  ep_free(e_addr);
  ep_free(e_val);
  ep_free(e_alpha);
  ep_free(e_last);
  ep_free(e_tmp);

  return 0;
}

int PoseidonClient::DecryptResult(std::vector<std::string> &plain_out,
                                  const std::vector<ResMetadata> &res_in,
                                  const std::string &keyword, const int n) {
  if (res_in.empty()) {
    return 0; // 无结果可解密
  }

  StateCell cell;
  if (!this->state->Get(cell, keyword)) {
    cerr << "Error: Keyword not found in state" << endl;
    return -1;
  }

  // 如果计数为0，提前返回空结果
  if (cell.cntw <= 0) {
    return 0;
  }

  ep_t ele;
  bn_t c, d, e, ord;
  ep_new(ele);
  bn_new(c);
  bn_new(e);
  bn_new(d);
  bn_new(ord);

  // 计算K2的模逆，用于解密
  ep_curve_get_ord(ord);
  bn_gcd_ext(c, d, e, this->K2, ord);

  std::set<string> tmp;
  string s1, s2;

  for (int i = 1; i <= cell.cntw; i++) {
    const string &val = res_in[i].val;
    int cnt = res_in[i].cnt;

    // 读取椭圆曲线点
    ep_read_bin(ele, (const unsigned char *)val.c_str(), 33);

    // 解密：乘以K2的模逆
    ep_mul(ele, ele, d);

    // 反向映射获取原始数据
    pi_inv(s1, ele);

    // 检查pi_inv是否成功返回有效字符串
    if (s1.empty() || s1.size() < 2) {
      // 至少需要1字节操作符 + 1字节数据
      cerr << "Warning: pi_inv returned invalid string at index " << i
           << ", skipping entry" << endl;
      continue;
    }

    // 提取操作符和数据
    char op = s1[0];
    s2.assign(s1.begin() + 1, s1.end());

    // 根据操作符处理结果
    if (op == '1' && cnt == n) {
      // 添加操作且计数匹配
      tmp.emplace(s2);
    } else if (op == '0' && cnt > 0) {
      // 删除操作
      tmp.erase(s2);
    }
    // 其他情况：忽略
  }

  // 将结果转换为输出向量
  plain_out.reserve(tmp.size());
  for (auto &itr : tmp) {
    plain_out.emplace_back(itr);
  }

  // 清理资源
  ep_free(ele);
  bn_free(c);
  bn_free(e);
  bn_free(d);
  bn_free(ord);

  return 0;
}

int PoseidonClient::KeyUpdate(std::string &Delta) {
  bn_t delta, ord;
  unsigned char buf[64];

  bn_new(delta);
  bn_new(ord);

  ep_curve_get_ord(ord);
  bn_rand_mod(delta, ord);

  bn_write_bin(buf, 32, delta);
  Delta.assign((char *)buf, 32);

  bn_mul(this->K1, delta, this->K1);
  bn_mod(this->K1, this->K1, ord);

  bn_mul(this->K2, delta, this->K2);
  bn_mod(this->K2, this->K2, ord);

  bn_mul(this->Kx, delta, this->Kx);
  bn_mod(this->Kx, this->Kx, ord);

  bn_mul(this->Ky, delta, this->Ky);
  bn_mod(this->Ky, this->Ky, ord);

  bn_mul(this->Kz, delta, this->Kz);
  bn_mod(this->Kz, this->Kz, ord);

  bn_free(delta);
  bn_free(ord);

  return 0;
}

void PoseidonClient::DumpData(const std::string &filename) {
  unsigned char buf[64];
  FILE *fkey;

  this->state->DumpData(filename + ".db");

  fkey = fopen((filename + "_priv_key").c_str(), "wb");
  bn_write_bin(buf, 32, K1);
  fwrite(buf, sizeof(char), 32, fkey);
  bn_write_bin(buf, 32, K2);
  fwrite(buf, sizeof(char), 32, fkey);
  bn_write_bin(buf, 32, Kx);
  fwrite(buf, sizeof(char), 32, fkey);
  bn_write_bin(buf, 32, Ky);
  fwrite(buf, sizeof(char), 32, fkey);
  bn_write_bin(buf, 32, Kz);
  fwrite(buf, sizeof(char), 32, fkey);

  fclose(fkey);
}

void PoseidonClient::LoadData(const std::string &filename) {
  unsigned char buf[64];
  FILE *fkey;

  this->state->LoadData(filename + ".db");

  fkey = fopen((filename + "_priv_key").c_str(), "rb");
  fread(buf, sizeof(char), 32, fkey);
  bn_read_bin(K1, buf, 32);
  fread(buf, sizeof(char), 32, fkey);
  bn_read_bin(K2, buf, 32);
  fread(buf, sizeof(char), 32, fkey);
  bn_read_bin(Kx, buf, 32);
  fread(buf, sizeof(char), 32, fkey);
  bn_read_bin(Ky, buf, 32);
  fread(buf, sizeof(char), 32, fkey);
  bn_read_bin(Kz, buf, 32);

  fclose(fkey);
}

void PoseidonClient::BatchDataUpdate(vector<Metadata> &Metadatas,
                                     const string &keyword,
                                     const vector<std::string> &ids,
                                     PoseidonOp op) {
  StateCell cell;
  unsigned char buf1[64];
  string tk1, s, rand1;
  ep_t e_addr, e_val, e_lastAddr, e_alpha, e_L, e_D, e_C, e_tmp, e_tmp1;

  ep_new(e_addr);
  ep_new(e_val);
  ep_new(e_lastAddr);
  ep_new(e_alpha);
  ep_new(e_L);
  ep_new(e_D);
  ep_new(e_C);
  ep_new(e_tmp);
  ep_new(e_tmp1);

  if (!this->state->Get(cell, keyword)) {
    RAND_bytes(buf1, 16);
    cell.tk.assign((char *)buf1, 16);
    RAND_bytes(buf1, 16);
    cell.rand.assign((char *)buf1, 16);
    cell.cntw = 0;
  }
  cell.cntw += 1;

  for (const auto &id : ids) {
    RAND_bytes(buf1, 16);
    tk1.assign((char *)buf1, 16);
    RAND_bytes(buf1, 16);
    rand1.assign((char *)buf1, 16);

    Hash_H1(e_addr, tk1);
    ep_mul(e_addr, e_addr, this->K1);

    if (op == Poseidon_add)
      s = "1" + id;
    else
      s = "0" + id;

    pi(e_val, s);
    ep_mul(e_val, e_val, this->K2);
    Hash_G1(e_tmp, tk1);
    ep_mul(e_tmp, e_tmp, this->K1);
    ep_add(e_val, e_val, e_tmp);

    pi(e_lastAddr, cell.tk);
    Hash_H2(e_tmp, tk1);
    ep_add(e_lastAddr, e_lastAddr, e_tmp);
    ep_mul(e_lastAddr, e_lastAddr, this->K1);

    Hash_H1(e_tmp, s);
    ep_mul(e_tmp, e_tmp, this->Ky);
    string w_cntw = keyword + to_string(cell.cntw);
    Hash_H1(e_tmp1, w_cntw);
    ep_mul(e_tmp1, e_tmp1, this->Kz);
    ep_sub(e_tmp, e_tmp, e_tmp1);
    Hash_G2(e_tmp1, tk1);
    ep_mul(e_tmp1, e_tmp1, this->K1);
    ep_add(e_alpha, e_tmp, e_tmp1);

    Hash_H1(e_C, keyword);
    ep_mul(e_C, e_C, this->Kx);
    Hash_H1(e_tmp, s);
    ep_mul(e_tmp, e_tmp, this->Ky);
    ep_add(e_C, e_C, e_tmp);
    Hash_G1(e_tmp, rand1);
    ep_mul(e_tmp, e_tmp, this->K1);
    ep_add(e_C, e_C, e_tmp);

    Hash_H1(e_L, rand1);
    ep_mul(e_L, e_L, this->K1);

    pi(e_D, cell.rand);
    Hash_H2(e_tmp, rand1);
    ep_add(e_D, e_tmp, e_D);
    ep_mul(e_D, e_D, this->K1);

    Metadata meta;
    ep_write_bin(buf1, 33, e_addr, 1);
    meta.addr.assign((char *)buf1, 33);

    ep_write_bin(buf1, 33, e_val, 1);
    meta.val.assign((char *)buf1, 33);

    ep_write_bin(buf1, 33, e_lastAddr, 1);
    meta.lastAddr.assign((char *)buf1, 33);

    ep_write_bin(buf1, 33, e_alpha, 1);
    meta.alpha.assign((char *)buf1, 33);

    ep_write_bin(buf1, 33, e_L, 1);
    meta.L.assign((char *)buf1, 33);

    ep_write_bin(buf1, 33, e_D, 1);
    meta.D.assign((char *)buf1, 33);

    ep_write_bin(buf1, 33, e_C, 1);
    meta.C.assign((char *)buf1, 33);

    Metadatas.emplace_back(meta);

    cell.tk = tk1;
    cell.cntw += 1;
  }

  this->state->Put(cell, keyword);

  ep_free(e_addr);
  ep_free(e_val);
  ep_free(e_lastAddr);
  ep_free(e_alpha);
  ep_free(e_L);
  ep_free(e_D);
  ep_free(e_C);
  ep_free(e_tmp);
  ep_free(e_tmp1);
}
