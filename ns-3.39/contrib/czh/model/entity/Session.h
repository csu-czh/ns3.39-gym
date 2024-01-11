#ifndef _SESSION_H
#define _SESSION_H
#include "../tools/GLOBAL.h"
#include "Topolopy.h"
#include "MNode.h"
#include "MPacket.h"

namespace ns3
{
    class MPacket;
    class Session{
    public:
        Session();
        Session(int src, std::vector<int> dst, Topolopy* topolopy);
        bool isPositiveLink(int, int);
        int getBFIndex(Mnode* node);

        bool isPostiveNode[MAX_NODE];
        int distance[MAX_NODE];

        std::set<PairII> m_links; // node,interfaceId
        std::set<PairII> postiveLink; // node,node
        Mnode* sender;
        Topolopy* topolopy;
        std::set<int> receivers;
        MPacket *mpacket;
    };
}
#endif