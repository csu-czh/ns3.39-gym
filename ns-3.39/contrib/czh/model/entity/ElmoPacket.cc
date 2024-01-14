#include "ElmoPacket.h"

#include "BloomFilter.h"

#include <cmath>
#include <ctime>
#include <iostream>
#include <random>
#include <sstream>
using namespace std;

namespace ns3
{

int dist(int a, int b);
vector<Rule> clustering(vector<pair<int, unsigned long long>> nodes);
pair<int, unsigned long long> getMinNode(vector<pair<int, unsigned long long>> nodes,
                                         int min_k_bitmap);
Rule approx_min_k_union(vector<pair<int, unsigned long long>> nodes, int k);

vector<pair<int, unsigned long long>> remove(vector<pair<int, unsigned long long>> nodes,
                                             vector<pair<int, unsigned long long>> nodes2);
vector<pair<int, unsigned long long>> remove(vector<pair<int, unsigned long long>> nodes,
                                             pair<int, unsigned long long> node);
void printVe(vector<pair<int, unsigned long long>> nodes);

ElmoPacket::ElmoPacket(Session* session)
{
    init(session);
}

void
ElmoPacket::init(Session* session)
{
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
ElmoPacket::generateLabelFattree(Session* session)
{
    Topolopy* topolopy = session->topolopy;
    sumBits = 0;
    int corr = 0;
    Mnode* sender = session->sender;
    int numberOfNode = topolopy->nodes.size();

    // nodeid,bitmap maxlen is the number of leaf node
    vector<pair<int, unsigned long long>> edgeBitmap;
    vector<pair<int, unsigned long long>> aggrBitmap; // pod,bitmap maxlen is the number of pods
    std::vector<Mnode*> edgeSwitchs;
    std::vector<Mnode*> aggrSwitchs;
    for (int i = 0; i < numberOfNode; i++)
    {
        if (topolopy->nodes[i]->type.compare("aggr") == 0)
        {
            aggrSwitchs.push_back(topolopy->nodes[i]);
        }
        else if (topolopy->nodes[i]->type.compare("edge") == 0)
        {
            edgeSwitchs.push_back(topolopy->nodes[i]);
        }
    }
    std::cout<< topolopy->nodes[25]->type <<" " << topolopy->nodes[25]->pod<< std::endl;
    std::cout<<"edgeSwitchs " << edgeSwitchs.size() << " " << aggrSwitchs.size() <<" "  << topolopy->switchs.size() << std::endl;
    // cout << "topolopy.host.size(): " << topolopy.hosts.size() << endl;
    // cout << "topolopy.receivers.size(): " << session.receivers.size() << endl;

    for (int i = 0; i < edgeSwitchs.size(); i++)
    {
        Mnode* edgeSwitch = edgeSwitchs[i];
        unsigned long long bitmap = 0;
        for (int j = 0; j < edgeSwitch->linkedNodes.size(); j++)
        {
            Mnode* host = edgeSwitch->linkedNodes[j];
            //  cout<<leafNode->type<<" "<< host->type<<endl;
            // cout << leafNode->id << " l,h " << host->id << endl;
            if (session->isPostiveNode[host->id] && host->type.compare("host") == 0)
            {
                corr++;
                bitmap += (1ULL << (j )); // 产生 keafdown port数 长的端口位图
                // cout << j - 16 << " ^ " << leafNode->linkedNodes.size() << endl;
            }
        }
        edgeBitmap.push_back(make_pair(edgeSwitch->id, bitmap));
    }
    // cout<<"leafbitmap size: "<<leafBitmap.size()<<endl;
    edgePRules = clustering(edgeBitmap);

    for (int i = 0; i < aggrSwitchs.size(); i++)
    {
        if (i % FATTREE_POD / 2 != 0)
            continue;
        Mnode* aggrSwitch = aggrSwitchs[i];
        unsigned long long bitmap = 0;
        for (int j = 0; j < aggrSwitch->linkedNodes.size(); j++)
        {
            Mnode* edgeSwitch = aggrSwitch->linkedNodes[j];
            //  cout<<leafNode->type<<" "<< host->type<<endl;
            // cout << leafNode->id << " l,h " << host->id << endl;
            if (session->isPostiveNode[edgeSwitch->id] && edgeSwitch->type.compare("edge") == 0)
            {
                corr++;
                bitmap += (1ULL << (j)); // 产生 keafdown port数 长的端口位图
                // cout << j - 16 << " ^ " << leafNode->linkedNodes.size() << endl;
            }
        }
        // cout <<" aggre bitmap: " <<aggrSwitch->id << " " << bitmap << endl;
        aggrBitmap.push_back(make_pair(aggrSwitch->id, bitmap));
    }
    // cout<<"leafbitmap size: "<<leafBitmap.size()<<endl;
    // cout<<"leafbitmap size: "<<spineBitmap.size()<<endl;
    cout << "corr: " << corr << endl;
    aggrPRules = clustering(aggrBitmap);
}

void
ElmoPacket::generateLabelLeafspine()
{

}

bool
ElmoPacket::doForward(int nodeId, int interfaceId, Topolopy* topology, Session* session)
{
    int forwardPacket1 = 0;
    int totalForward = 0;
    int corretForward = 0;
    sumBits = 32 + 1 + 16 + LEAF_NUM + 1;
    sumBits += LEAF_NUM; // default bitmap
    // packet.leafPRules;
    // 这里应该是每个pod用一个
    Mnode* nodea = topology->nodes[nodeId];
    Mnode* nodeb = nodea->linkedNodes[interfaceId];
    // return session->isPositiveLink(nodea->id, nodeb->id);
    if(session->distance[nodea->id] <= 3){
        if(session->isPositiveLink(nodea->id, nodeb->id) )
            return true;
        else{
            return false;
        }
    }
    
    if (nodea->type.compare("aggr") == 0 && nodeb->type.compare("edge") == 0)
    {
        for (int i = 0; i < aggrPRules.size(); i++)
        {
            Rule rule = aggrPRules[i];
            for (int j = 0; j < rule.nodes.size(); j++)
            {
                if (nodea->id == rule.nodes[j].first)
                {
                    unsigned long long bitmap = rule.bitmap;
                    if (bitmap & (1LL << (interfaceId)))
                    { // 需要转发
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }
        }
        return false;
    }
    else if (nodea->type.compare("edge") == 0 && nodeb->type.compare("host") == 0)
    {
        for (int i = 0; i < edgePRules.size(); i++)
        {
            Rule rule = edgePRules[i];
            for (int j = 0; j < rule.nodes.size(); j++)
            {
                if (nodea->id == rule.nodes[j].first)
                {
                    unsigned long long bitmap = rule.bitmap;
                    if (bitmap & (1LL << (interfaceId)))
                    { // 需要转发
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }
        }
        return false;
    }
    else
    {
        return false;
    }
}

Rule
approx_min_k_union(vector<pair<int, unsigned long long>> nodes, int k)
{
    for (int i = 0; i < nodes.size(); i++) // 自己优化的部分
    {
        vector<pair<int, unsigned long long>> min_k_nodes;
        for (int j = 0; j < nodes.size(); j++)
        {
            if (nodes[i].second == nodes[j].second && min_k_nodes.size() < k)
            {
                min_k_nodes.push_back(nodes[j]);
            };
        }
        if (min_k_nodes.size() == k)
        {
            Rule rule;
            rule.nodes = min_k_nodes;
            rule.bitmap = nodes[i].second;
            return rule;
        }
    }

    int min_k_bitmap = 0;
    vector<pair<int, unsigned long long>> min_k_nodes;
    for (int i = 0; i < k; i++)
    {
        pair<int, unsigned long long> node = getMinNode(nodes, min_k_bitmap);
        nodes = remove(nodes, node);
        min_k_bitmap = min_k_bitmap | node.second;
        min_k_nodes.push_back(node);
    }

    Rule rule;
    rule.nodes = min_k_nodes;
    rule.bitmap = min_k_bitmap;
    return rule;
}

// 计算Hamming Distances
int
dist(int a, int b)
{
    int res = 0;
    for (int i = 0; i < 32; i++)
    { // 最多24个端口
        int x = 1 << i;
        if ((x & a) != (x & b))
            res++;
    }
    return res;
}

vector<pair<int, unsigned long long>>
remove(vector<pair<int, unsigned long long>> nodes, vector<pair<int, unsigned long long>> nodes2)
{
    vector<pair<int, unsigned long long>> res;
    for (int i = 0; i < nodes.size(); i++)
    {
        int find = 1;
        for (int j = 0; j < nodes2.size(); j++)
        {
            if (nodes2[j].first == nodes[i].first && nodes2[j].second == nodes[i].second)
            {
                find = 0;
                break;
            }
        }
        if (find)
            res.push_back(nodes[i]);
    }
    return res;
}

vector<pair<int, unsigned long long>>
remove(vector<pair<int, unsigned long long>> nodes, pair<int, unsigned long long> node)
{
    vector<pair<int, unsigned long long>> res;
    int fla = 1;
    for (int i = 0; i < nodes.size(); i++)
    {
        if (node.first == nodes[i].first && fla)
        {
            fla = 0;
            continue;
        }
        res.push_back(nodes[i]);
    }
    return res;
}

pair<int, unsigned long long>
getMinNode(vector<pair<int, unsigned long long>> nodes, int min_k_bitmap)
{
    // nodes_map[l]['bitmap'] | min_k_bitmap
    unsigned long long minn = nodes[0].second;
    int index = 0;
    for (int i = 1; i < nodes.size(); i++)
    {
        int tmp = (nodes[i].second | min_k_bitmap);
        if (tmp < minn)
        {
            minn = tmp;
            index = i;
        }
    }
    return nodes[index];
}

void
printVe(vector<pair<int, unsigned long long>> nodes)
{
    // cout<<"printVe"<<endl;
    // for (int i = 0; i < nodes.size(); i++)
    // {
    //     cout << nodes[i].second << " ";
    // }
    // cout << endl;
}

vector<Rule>
clustering(vector<pair<int, unsigned long long>> nodes)
{
    // printVe(nodes);
    vector<Rule> p_rules;
    Rule default_p_rule;
    vector<pair<int, unsigned long long>> unassigned = nodes;
    int Hmax = 300;
    int K = 300;

    while (unassigned.size() && p_rules.size() < Hmax)
    {
        if (unassigned.size() < K)
        {
            K--;
            continue;
        }
        Rule min_k_rule = approx_min_k_union(unassigned, K);

        int output_bm = min_k_rule.bitmap;

        bool condition = true;
        for (int i = 0; i < min_k_rule.nodes.size(); i++)
        {
            if (dist(output_bm, min_k_rule.nodes[i].second) > ELMO_R)
            {
                condition = false;
                break;
            }
        }

        if (condition)
        {
            p_rules.push_back(min_k_rule);
            unassigned = remove(unassigned, min_k_rule.nodes);
        }
        else
        {
            K--;
        }
    }

    for (int i = 0; i < unassigned.size(); i++)
    {
        default_p_rule.bitmap = default_p_rule.bitmap | unassigned[i].second;
    }

    p_rules.push_back(default_p_rule); // default_p_rule 里面的nodes 为0
    // cout<<"p_rules[i].nodes.size() :"<<endl;
    // for(int i=0;i<p_rules.size();i++){
    //     cout<<p_rules[i].nodes.size()<<endl;
    // }
    return p_rules;
}

} // namespace ns3