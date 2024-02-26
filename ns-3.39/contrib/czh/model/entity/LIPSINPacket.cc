#include "LIPSINPacket.h"

#include <cmath>
#include <ctime>
#include <iostream>
#include <random>
#include <sstream>

namespace ns3
{


LIPSINPacket::LIPSINPacket(Session* session)
{
    init(session);
}

void
LIPSINPacket::init(Session* session)
{
    std::cout << "LIPSIN init" << std::endl;
    if (TOPOLOPY_TYPE == 1)
    { // fattree
        generateLabelFattree(session);
    }
    else
    {
        generateLabelLeafspine();
    }
}

void
LIPSINPacket::generateLabelFattree(Session* session)
{
    Topolopy* topology = session->topolopy;
    int n = topology->nodes.size(); // 拓扑中节点总数
    std::vector<std::pair<int, int>> positiveLink;
    std::vector<std::pair<int, int>> negativeLink;

    for (int i = 0; i < n; i++)
    {
        Mnode* node = topology->nodes[i];
        if (!session->isPostiveNode[i])
            continue;

        if (node->type.compare("host") == 0)
            continue; // 只考虑交换机

        // std::cout<<"pk is positive "<< i << std::endl;
        for (int j = 0; j < node->linkedNodes.size(); j++)
        {
            int to = node->linkedNodes[j]->id;
            int isPositive = session->isPositiveLink(node->id, to);
            int a = node->id;
            int b = to;
            // std::cout<< session->distance[to] <<" " <<session->distance[node->id]<<std::endl;
            if (session->distance[to] != session->distance[node->id] + 1) // 不能访问同端的点
                continue;
            // std::cout<< a <<" " <<to<<std::endl;
            if (isPositive)
            {
                positiveLink.push_back(std::make_pair(a, b));
            }
            else
            {
                negativeLink.push_back(std::make_pair(a, b));
            }
        }
    }

    this->bf = BloomFilter(BF_SIZE);

    for (int j = 0; j < positiveLink.size(); j++)
    {
        int a = positiveLink[j].first;
        int b = positiveLink[j].second;

        std::string s = std::to_string(positiveLink[j].first) + " to " +
                        std::to_string(positiveLink[j].second);
        std::cout << "add string  " << s << std::endl;

        this->bf.SetKey(s.c_str());
    }
}

void
LIPSINPacket::generateLabelLeafspine()
{
}

bool
LIPSINPacket::doForward(int nodeId, int interfaceId, Topolopy* topology, Session* session)
{
    Mnode* nodea = topology->nodes[nodeId];
    Mnode* nodeb = nodea->linkedNodes[interfaceId];
    if (session->distance[nodea->id] + 1 != session->distance[nodeb->id])
        return false;
    std::string link = std::to_string(nodea->id) + " to " + std::to_string(nodeb->id);
    bool forward = bf.VaryExist(link.c_str());

    if(forward == true && session->isPositiveLink(nodea->id,nodeb->id) == false){
        if(erroredge.find(std::make_pair(nodea->id,nodeb->id)) == erroredge.end()){
            erroredge.insert(std::make_pair(nodea->id,nodeb->id));
            std::cout<<session->distance[nodea->id] <<" " << nodea->id << " " << nodeb->id<< std::endl;

        }
        
    }
    return forward;
}

} // namespace ns3