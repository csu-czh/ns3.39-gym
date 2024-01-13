#ifndef _MPACKET_H
#define _MPACKET_H

#include "../tools/GLOBAL.h"
#include "MNode.h"
#include "Session.h"
#include "Topolopy.h"

namespace ns3
{
class BloomFilter
{
  public:
    BloomFilter(int m)
    {
        m_m = m;
        m_k = 3;
        int a[] = {31, 5, 11, 13, 31};
        for (int i = 0; i < m_k; i++)
        {
            arrays[i] = a[i];
        }
        for (int i = 1; i <= m; i++)
        {
            bit.push_back(0);
        }
    }

    BloomFilter()
    {
    }

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

class Session;

class MPacket
{
  public:
    BloomFilter bfs[3];
    std::string multicastProtocol;
    std::set<PairII> erroredge;
    MPacket(Session* session, std::string _multicastProtocol);
    void generateLabelFattree(Session* session);
    void generateLabelLeafspine();
    void initRSBF(Session* session);
    bool doForwardRSBF(int nodeId, int interfaceId, Topolopy* topology, Session* session);
};
} // namespace ns3
#endif