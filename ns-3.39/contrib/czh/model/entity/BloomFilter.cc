#include "BloomFilter.h"

#include <cmath>
#include <ctime>
#include <iostream>
#include <random>
#include <sstream>

namespace ns3
{

uint
BloomFilter::RSHash(const char* str, int seed)
{
    // unsigned int b = 378551;
    uint a = 63689;
    uint hash = 0;
    while (*str)
    {
        hash = hash * a + (*str++);
        a *= seed;
    }
    return (hash & 0x7FFFFFFF);
}

void
BloomFilter::SetKey(const char* str)
{
    for (int i = 0; i < m_k; ++i)
    {
        int pos = static_cast<int>(RSHash(str, arrays[i])) % m_m;
        bit[pos] = 1;
    }
}

int
BloomFilter::VaryExist(const char* str)
{
    for (int i = 0; i < m_k; ++i)
    {
        int pos = static_cast<int>(RSHash(str, arrays[i])) % m_m;
        if (bit[pos] == 0)
        {
            return 0;
        }
    }
    return 1;
}

BloomFilter::BloomFilter()
{
}

BloomFilter::BloomFilter(int m)
{
    m_m = m;
    m_k = 3;
    int a[] = {31, 5, 11, 13, 31};
    for (int i = 0; i < m_k; i++)
    {
        arrays[i] = a[i];
    }
    bit.resize(m);
    for (int i = 0; i < m; i++)
    {
        bit[i] = 0;
    }
}
} // namespace ns3