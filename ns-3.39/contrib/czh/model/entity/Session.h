#ifndef _SESSION_H
#define _SESSION_H
#include "../tools/GLOBAL.h"
#include "Topolopy.h"
#include "MNode.h"

namespace ns3
{
    class Session{
    public:
        Session();
        void addNewSession(int port, int src, std::vector<int> dst);
        bool isPositiveLink(int, int);
        int getBFIndex(Mnode* node);
        
        std::string multicastProtocol = "RSBF";
        bool isPostiveNode[MAX_NODE];
        std::vector< std::set<PairII> > m_links;
        Mnode* sender; 
        Topolopy* topolopy;
        int distance[MAX_NODE];
        std::vector<std::set<int> > receivers;
        std::set<PairII> postiveLink;
    };
}
#endif