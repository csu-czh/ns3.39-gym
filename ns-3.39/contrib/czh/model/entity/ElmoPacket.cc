#include "ElmoPacket.h"
#include <cmath>
#include <ctime>
#include <iostream>
#include <random>
#include <sstream>
#include "BloomFilter.h"
using namespace std;
namespace ns3
{

int dist(int a, int b);
vector<Rule> clustering(vector<pair<int, unsigned long long >> nodes);
pair<int, unsigned long long > getMinNode(vector<pair<int, unsigned long long >> nodes, int min_k_bitmap);
Rule approx_min_k_union(vector<pair<int, unsigned long long >> nodes, int k);

vector<pair<int, unsigned long long >> remove(vector<pair<int, unsigned long long >> nodes, vector<pair<int, unsigned long long >> nodes2);
vector<pair<int, unsigned long long >> remove(vector<pair<int, unsigned long long >> nodes, pair<int, unsigned long long > node);
void printVe(vector<pair<int, unsigned long long >> nodes);

ElmoPacket::ElmoPacket(Session* session)
{
   init(session);
}

void
ElmoPacket::init(Session* session)
{
    std::cout << "Elmo" << std::endl;
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

}

void
ElmoPacket::generateLabelLeafspine()
{

}

bool
ElmoPacket::doForward(int nodeId, int interfaceId, Topolopy* topology, Session* session)
{
    return 1;
}


Rule approx_min_k_union(vector<pair<int, unsigned long long >> nodes, int k)
{
    for (int i = 0; i < nodes.size(); i++) // 自己优化的部分
    {
        vector<pair<int, unsigned long long >> min_k_nodes;
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
    vector<pair<int, unsigned long long >> min_k_nodes;
    for (int i = 0; i < k; i++)
    {
        pair<int, unsigned long long > node = getMinNode(nodes, min_k_bitmap);
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
int dist(int a, int b)
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

vector<pair<int, unsigned long long >> remove(vector<pair<int, unsigned long long >> nodes, vector<pair<int, unsigned long long >> nodes2)
{
    vector<pair<int, unsigned long long >> res;
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

vector<pair<int, unsigned long long >> remove(vector<pair<int, unsigned long long >> nodes, pair<int, unsigned long long > node)
{
    vector<pair<int, unsigned long long >> res;
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

pair<int, unsigned long long > getMinNode(vector<pair<int, unsigned long long >> nodes, int min_k_bitmap)
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

void printVe(vector<pair<int,unsigned long long >> nodes)
{
    // cout<<"printVe"<<endl;
    // for (int i = 0; i < nodes.size(); i++)
    // {
    //     cout << nodes[i].second << " ";
    // }
    // cout << endl;
}
vector<Rule> clustering(vector<pair<int, unsigned long long >> nodes)
{
    // printVe(nodes);
    vector<Rule> p_rules;
    Rule default_p_rule;
    vector<pair<int, unsigned long long >> unassigned = nodes;
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

    p_rules.push_back(default_p_rule); //default_p_rule 里面的nodes 为0
    // cout<<"p_rules[i].nodes.size() :"<<endl;
    // for(int i=0;i<p_rules.size();i++){
    //     cout<<p_rules[i].nodes.size()<<endl; 
    // }
    return p_rules;
}

} // namespace ns3