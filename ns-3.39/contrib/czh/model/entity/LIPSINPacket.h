#ifndef _LIPSINPacket_H
#define _LIPSINPacket_H

#include "../tools/GLOBAL.h"
#include "MNode.h"
#include "Session.h"
#include "Topolopy.h"
namespace ns3
{

class Session;

class LIPSINPacket
{
  public:
    BloomFilter bf;
    std::set<PairII> erroredge;
    LIPSINPacket(Session* session);
    void generateLabelFattree(Session* session);
    void generateLabelLeafspine();
    void init(Session* session);
    bool doForward(int nodeId, int interfaceId, Topolopy* topology, Session* session);
};
} // namespace ns3
#endif