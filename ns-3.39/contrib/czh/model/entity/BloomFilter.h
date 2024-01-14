#ifndef _BloomFilter_H
#define _BloomFilter_H
#include "../tools/GLOBAL.h"

namespace ns3
{

class BloomFilter
{
  public:
    BloomFilter(int m);
    BloomFilter();

    uint RSHash(const char* str, int seed);
    void SetKey(const char* str);
    int VaryExist(const char* str);

  private:
    int m_k, m_m, arrays[5];
    // k:number of the hash functions
    // m:the size of bitset
    // n:number of strings to hash (k = [m/n]*ln2)
    std::vector<int> bit;
};
} // namespace ns3
#endif