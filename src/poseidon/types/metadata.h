#ifndef METADATA_H
#define METADATA_H

#include <string>
#include <vector>

struct Metadata {
  std::string addr;
  std::string val;
  std::string lastAddr;
  std::string alpha;
  std::string L;
  std::string D;
  std::string C;
};

struct ResMetadata {
  std::string val;
  int cnt;
};

struct TKLMetadata {
  std::string L;
  std::string TD;
  std::string TC;
};

struct TrapdoorMetadata {
  std::string K1;
  std::vector<TKLMetadata> TKL;
  std::vector<std::string> STKL;
  std::vector<std::vector<std::string>> XTKL;
};

#endif