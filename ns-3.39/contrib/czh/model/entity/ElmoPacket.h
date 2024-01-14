#ifndef _ElmoPacket_H
#define _ElmoPacket_H

#include "../tools/GLOBAL.h"
#include "MNode.h"
#include "Session.h"
#include "Topolopy.h"

namespace ns3
{

class Session;

class Rule
{
public:
    std::vector<std::pair<int, unsigned long long >> nodes;
    unsigned long long  bitmap;
    Rule()
    {
        bitmap = 0;
        nodes.clear();
    }
};
class ElmoPacket
{
  public:
    int sumBits;
    unsigned long long  leafUp; // len is the number of leaf downports. It is no error
    // unsinged unsigned long long  coreDown; // len is the number of pods. It is no error
    std::vector<Rule> aggrPRules; // maxlen is the number of pods
    std::vector<Rule> edgePRules;  // maxlen is the number of leaf node
    std::set<PairII> erroredge;
    ElmoPacket(Session* session);
    void generateLabelFattree(Session* session);
    void generateLabelLeafspine();
    void init(Session* session);
    bool doForward(int nodeId, int interfaceId, Topolopy* topology, Session* session);
};
} // namespace ns3
#endif