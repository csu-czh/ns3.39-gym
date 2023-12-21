#ifndef _SESSION_H
#define _SESSION_H
#include "../tools/GLOBAL.h"
#include "Topolopy.h"
#include "MNode.h"
namespace ns3
{
    class Session{
    public:
        std::set<std::pair<int,int> > m_links[1000];
        Mnode* sender; 
        Topolopy* topolopy;
        
        int distance[MAX_NODE];
        std::set<int>receivers[1000];
        std::set< std::pair<int,int> > postiveLink;
        void addNewSession(int port,int src,std::vector<int> dst);
        Session();
        bool isPositiveLink(int,int);
        int getBFIndex(Mnode* node);
    };
}
#endif