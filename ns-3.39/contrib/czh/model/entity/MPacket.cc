#include "MPacket.h"
#include <sstream>
#include <iostream>
#include <cmath>
#include <ctime>
#include <random>
namespace ns3
{

std::pair<int, int> getMinXY_by_Enumerate(double S, double p[3]);
std::pair<int, int> getMinXY_by_simulated_annealing2(double S, double p[5]);

// 计算假阳性率的标准公式
double fprr(int n, double m, int k)
{
    double tmp = 1 - 1 / m;
    tmp = pow(tmp, n * k);
    tmp = pow(1 - tmp, k);
    return tmp;
}

class Mes
{
public:
    float fp;
    float tn;
    Mes()
    {
        fp = 0;
        tn = 0;
    }
    float fpr()
    {
        if ((fp + tn) != 0)
        {
            return fp / (fp + tn);
        }
        else
        {
            return 0;
        }
    }
};

uint BloomFilter::RSHash(const char *str, int seed)
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

void BloomFilter::SetKey(const char *str)
{
    for (int i = 0; i < m_k; ++i)
    {
        int pos = static_cast<int>(RSHash(str, arrays[i])) % m_m;
        bit[pos] = 1;
    }
}

int BloomFilter::VaryExist(const char *str)
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

MPacket::MPacket(Session* session, std::string _multicastProtocol){
    multicastProtocol = _multicastProtocol;
    if(multicastProtocol.compare("RSBF") == 0 ){
        initRSBF(session);
    }else{

    }
}

void MPacket::initRSBF(Session* session){
    std::cout<<"RSBF"<<std::endl;
    if(TOPOLOPY_TYPE == 1){ //fattree
        generateLabelFattree(session);
    }else{
        generateLabelLeafspine();
    }
}

void MPacket::generateLabelFattree(Session * session){
    int hops = 5;
    // std::cout<< "session " << session << std::endl;
    Topolopy * topology = session->topolopy;
    // std::cout<<"topolopy " << topology << std::endl; 
    int n = topology->nodes.size(); // 拓扑中节点总数
    std::cout<< "topo size " << n << std::endl;
    std::vector<std::pair<int, int> > positiveLink[hops];
    std::vector<std::pair<int, int> > negativeLink[hops];

    for (int i = 0; i < n; i++)
    {
        Mnode *node = topology->nodes[i];
        int layer = session->distance[node->id] - 1;

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
                positiveLink[layer].push_back(std::make_pair(a, b));
            }
            else
            {
                negativeLink[layer].push_back(std::make_pair(a, b));
            }
        }
    }

    double p[5];
    std::cout<<"number of ports: \n";
    for (int i = 0; i < hops; i++)
    {
        p[i] = positiveLink[i].size();
        std::cout << p[i] << " ";
    }
    std::cout << "\n";

    int sumBit = BF_SIZE;
    int x, y;
    // 计算最优的x,y分配
    std::pair<int, int> minXY = getMinXY_by_Enumerate(sumBit, p);
    std::pair<int, int> minXY2 = getMinXY_by_simulated_annealing2(sumBit, p);
    // std::cout << "true x,y: " << minXY.first << " " << minXY.second << std::endl;
    // std::cout << "my x,y: " << minXY2.first << " " << minXY2.second << std::endl;
    x = minXY2.first, y = minXY2.second;
    int bits[3];
    bits[0] = x;
    bits[1] = y;
    bits[2] = sumBit - bits[0] - bits[1];
    // bits[0] = 1, bits[1] = 1, bits[2] = 999;
    // std::cout<<"avg: "<<(bits[0]*2 + bits[1]*3 + bits[2] * 5 )/ 5<<std::endl;
    int bfindexMap[5] = {0,0,1,2,2};

    for (int i = 0; i < 3; i++){
        this->bfs[i] = BloomFilter(bits[i]);
    }

    for( int i=0;i< hops;i++){
        int bfIndex = bfindexMap[i];
        for (int j = 0; j < positiveLink[i].size(); j++)
        {
            std::string s = std::to_string(positiveLink[i][j].first) + " to " + std::to_string(positiveLink[i][j].second);
            std::cout<<"add s " << s <<  " bf " << bfIndex <<std::endl;
            
            this->bfs[bfIndex].SetKey(s.c_str());
        }
    }
}

void MPacket::generateLabelLeafspine(){

}

bool MPacket::doForwardRSBF(int nodeId, int interfaceId, Topolopy* topology, Session* session){
    Mnode* nodea = topology->nodes[nodeId];
    Mnode* nodeb = nodea->linkedNodes[interfaceId];
    std::string link = std::to_string(nodea->id) + " to " + std::to_string(nodeb->id);
    int bfIndex = 0;
    if(nodea->pod == session->sender->pod){
        bfIndex = 0;
    }else if(nodea->type == "core") {
        bfIndex = 1;
    }else{
        bfIndex = 2;
    }
    bool forward = bfs[bfIndex].VaryExist(link.c_str());
    // if(nodeId == 59){
        // std::cout<< "bfIndex " << bfIndex <<std::endl;
        // std::cout<< "doForward " << interfaceId <<" " <<nodeb->id <<" " << forward<< std::endl;
    // }
    return bfs[bfIndex].VaryExist(link.c_str());
}

// 假设假阳性端口到达的节点为阴性节点。
// 真实情况下，有少部分假阳性端口到达的节点为阳性节点.
// 因此我们计算出来的值偏小
double getRedu(double x, double y, double S, double p[3])
{
    double F[3] = {0, 0, 0};
    double f[3] = {0, 0, 0};

    int k[3] = {SPINE_NUM, LEAF_NUM - 1 , HOST_PERLEAF_NUM}; // 保存每层节点的下游端口数量
    f[0] = fprr(p[0], x, 3);
    f[1] = fprr(p[1], y, 3);
    f[2] = fprr(p[2], S - x - y, 3);

    F[0] = (k[0] - p[0]) * f[0];
    F[1] = ((F[0] + p[0]) * k[1] - p[1]) * f[1];
    F[2] = ((F[1] + p[1]) * k[2] - p[2]) * f[2];
    // std::cout<<"p: "<<p[0]<<" "<<p[1]<<" "<<p[2]<<std::endl;
    // std::cout<<"f: "<<f[0]<<" "<<f[1]<<" "<<f[2]<<std::endl;
    // std::cout<<"F: "<<F[0]<<" "<<F[1]<<" "<<F[2]<<std::endl;
    return (F[0] + F[1] + F[2]);
}

std::pair<int, int> getMinXY_by_Enumerate(double S, double p[3])
{
    std::pair<int, int> res;
    double minn = 1e9 + 7;
    // std::cout<<g[1]<<" "<<g[2]<<" "<<g[3]<<std::endl;
    // int redu = getRedu(11,88,g,S,p);
    // return std::make_pair(11,88);
    for (int i = 1; i < S; i++)
    {
        for (int j = 1; j + i < S; j++)
        {
            double redu = getRedu(i, j, S, p);
            if (redu < minn)
            {
                minn = redu;
                res = std::make_pair(i, j);
            }
        }
    }
    // std::cout << res.first << " " << res.second << std::endl;
    // std::cout << "minn::" << minn << std::endl;
    return res;
}

class SA
{
public:
    double S;
    double p[3];
    int k[3] = {SPINE_NUM, LEAF_NUM-1, HOST_PERLEAF_NUM};

    double expectedForwardingCalc(int x, int y)
    {
        double F[] = {0, 0, 0};
        double f[] = {0, 0, 0};
        
        if (x < 1 || x > S)
        {
            return std::numeric_limits<double>::infinity();
        }
        if (y < 1 || y > S)
        {
            return std::numeric_limits<double>::infinity();
        }
        if (S - x - y < 1 || S - x - y > S)
        {
            return std::numeric_limits<double>::infinity();
        }

        f[0] = fprr(p[0], x, 3);
        f[1] = fprr(p[1], y, 3);
        f[2] = fprr(p[2], S - x - y, 3);

        F[0] = (k[0] - p[0]) * f[0];
        F[1] = ((F[0] + p[0]) * k[1] - p[1]) * f[1];
        F[2] = ((F[1] + p[1]) * k[2] - p[2]) * f[2];
        return F[0] + F[1] + F[2];
    }

    std::pair<int, int> simulated_annealing( double x, double y, double T_max, double T_min, double cooling_rate, double step_size, int iteration_per_temp)
    {
        std::srand(std::time(nullptr));
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0, 1);

        int current_x = x, current_y = y;
        double current_energy = expectedForwardingCalc(current_x, current_y);
    
        int best_x = current_x, best_y = current_y;

        double best_energy = current_energy;
        double T = T_max;
        while (T > T_min)
        {
            for (int i = 0; i < iteration_per_temp; ++i)
            {
                int neighbor_x = current_x + step_size * (dis(gen) * 2 - 1);
                int neighbor_y = current_y + step_size * (dis(gen) * 2 - 1);
                // std::cout<< current_x<<" "<<current_y<<std::endl;
                double neighbor_energy = expectedForwardingCalc(neighbor_x, neighbor_y);

                double energy_diff = neighbor_energy - current_energy;

                if (energy_diff < 0)
                {
                    current_x = neighbor_x;
                    current_y = neighbor_y;
                    current_energy = neighbor_energy;

                    if (current_energy < best_energy)
                    {
                        best_x = current_x;
                        best_y = current_y;
                        best_energy = current_energy;
                    }
                }
                else
                {
                    if (dis(gen) < exp(-energy_diff / T))
                    {
                        current_x = neighbor_x;
                        current_y = neighbor_y;
                        current_energy = neighbor_energy;
                    }
                }
            }
            T *= cooling_rate;
        }
        return std::make_pair(best_x, best_y);
    }
};


class SA2
{
public:
    double S;
    double p[5];
    int k[5] = {FATTREE_POD/2, (FATTREE_POD/2)*(FATTREE_POD/2),FATTREE_POD-1, FATTREE_POD/2,FATTREE_POD/2};

    double expectedForwardingCalc(int x, int y)
    {
        double F[] = {0, 0, 0};
        double f[] = {0, 0, 0};
        
        if (x < 1 || x > S)
        {
            return std::numeric_limits<double>::infinity();
        }
        if (y < 1 || y > S)
        {
            return std::numeric_limits<double>::infinity();
        }
        if (S - x - y < 1 || S - x - y > S)
        {
            return std::numeric_limits<double>::infinity();
        }

        f[0] = fprr(p[0]+p[1], x, 3);
        f[1] = fprr(p[2], y, 3);
        f[2] = fprr(p[3]+p[4], S - x - y, 3);

        F[0] = (k[0] - p[0]) * f[0];
        F[1] = ((F[0] + p[0]) * k[1] - p[1]) * f[0];
        F[2] = ((F[1] + p[1]) * k[2] - p[2]) * f[1];
        F[1] = ((F[0] + p[2]) * k[3] - p[3]) * f[2];
        F[2] = ((F[1] + p[3]) * k[4] - p[4]) * f[2];
        return F[0] + F[1] + F[2] + F[3] + F[4];
    }

    std::pair<int, int> simulated_annealing( double x, double y, double T_max, double T_min, double cooling_rate, double step_size, int iteration_per_temp)
    {
        std::srand(std::time(nullptr));
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0, 1);

        int current_x = x, current_y = y;
        double current_energy = expectedForwardingCalc(current_x, current_y);
    
        int best_x = current_x, best_y = current_y;

        double best_energy = current_energy;
        double T = T_max;
        while (T > T_min)
        {
            for (int i = 0; i < iteration_per_temp; ++i)
            {
                int neighbor_x = current_x + step_size * (dis(gen) * 2 - 1);
                int neighbor_y = current_y + step_size * (dis(gen) * 2 - 1);
                // std::cout<< current_x<<" "<<current_y<<std::endl;
                double neighbor_energy = expectedForwardingCalc(neighbor_x, neighbor_y);

                double energy_diff = neighbor_energy - current_energy;

                if (energy_diff < 0)
                {
                    current_x = neighbor_x;
                    current_y = neighbor_y;
                    current_energy = neighbor_energy;

                    if (current_energy < best_energy)
                    {
                        best_x = current_x;
                        best_y = current_y;
                        best_energy = current_energy;
                    }
                }
                else
                {
                    if (dis(gen) < exp(-energy_diff / T))
                    {
                        current_x = neighbor_x;
                        current_y = neighbor_y;
                        current_energy = neighbor_energy;
                    }
                }
            }
            T *= cooling_rate;
        }
        return std::make_pair(best_x, best_y);
    }
};

std::pair<int, int> getMinXY_by_simulated_annealing(double S, double p[3])
{
    SA sa;
    sa.S = S;
    sa.p[0] = p[0];
    sa.p[1] = p[1];
    sa.p[2] = p[2];

    int initial_x = 10.0, initial_y = 10.0;
    double T_max = 100.0;
    double T_min = 0.001;
    double cooling_rate = 0.95;
    double step_size = 5.0;
    int iteration_per_temp = 200;
    auto best_state = sa.simulated_annealing( initial_x, initial_y, T_max, T_min, cooling_rate, step_size, iteration_per_temp);
    double best_energy = sa.expectedForwardingCalc(best_state.first, best_state.second);
    // std::cout << f(29,94) <<std::endl;
    // std::cout << "Optimal solution: (" << best_state.first << ", " << best_state.second << ")\n";
    // std::cout << "Minimum value: " << best_energy << std::endl;
    return std::make_pair(best_state.first, best_state.second);
}

std::pair<int, int> getMinXY_by_simulated_annealing2(double S, double p[5])
{
    SA2 sa;
    sa.S = S;
    for(int i=0;i<5;i++){
        sa.p[i] = p[i];
    }
    int initial_x = 10.0, initial_y = 10.0;
    double T_max = 100.0;
    double T_min = 0.001;
    double cooling_rate = 0.95;
    double step_size = 5.0;
    int iteration_per_temp = 200;
    auto best_state = sa.simulated_annealing( initial_x, initial_y, T_max, T_min, cooling_rate, step_size, iteration_per_temp);
    double best_energy = sa.expectedForwardingCalc(best_state.first, best_state.second);
    // std::cout << f(29,94) <<std::endl;
    // std::cout << "Optimal solution: (" << best_state.first << ", " << best_state.second << ")\n";
    // std::cout << "Minimum value: " << best_energy << std::endl;
    return std::make_pair(best_state.first, best_state.second);
}

// 这里产生包头的三个BF
void generateLeafSpinePacket(Topolopy &topolopy, Session &session, MPacket &packet)
{
    int n = topolopy.nodes.size(); // 拓扑中节点总数
    std::vector<std::pair<int, int> > positiveLink[3];
    std::vector<std::pair<int, int> > negativeLink[3];
    for (int i = 0; i < 3; i++)
    {
        positiveLink[i].clear();
        negativeLink[i].clear();
    }

    for (int i = 0; i < n; i++)
    {
        Mnode *node = topolopy.nodes[i];
        if (!session.isPostiveNode[i])
            continue;
        if (node->type.compare("host") == 0)
            continue; // 只考虑交换机
        for (int j = 0; j < node->linkedNodes.size(); j++)
        {
            int to = node->linkedNodes[j]->id;
            int isPositive = session.isPositiveLink(node->id, to);
            int BFIndex = session.getBFIndex(node);
            int a = node->id;
            int b = to;
            if (session.distance[to] != session.distance[node->id] + 1) // 不能访问同端的点
                continue;
            if (isPositive)
            {
                positiveLink[BFIndex].push_back(std::make_pair(a, b));
            }
            else
            {
                negativeLink[BFIndex].push_back(std::make_pair(a, b));
            }
        }
    }

    double p[3];
    // std::cout<<"number of ports: ";
    for (int i = 0; i < 3; i++)
    {
        p[i] = positiveLink[i].size();
        // std::cout << p[i] << " ";
    }
    // std::cout << "\n";

    int sumBit = BF_SIZE;
    int x, y;
    // 计算最优的x,y分配
    std::pair<int, int> minXY = getMinXY_by_Enumerate(sumBit, p);
    std::pair<int, int> minXY2 = getMinXY_by_simulated_annealing(sumBit, p);
    // std::cout << "true x,y: " << minXY.first << " " << minXY.second << std::endl;
    // std::cout << "my x,y: " << minXY2.first << " " << minXY2.second << std::endl;
    x = minXY2.first, y = minXY2.second;
    int bits[3];
    bits[0] = x;
    bits[1] = y;
    bits[2] = sumBit - bits[0] - bits[1];
    // std::cout<<"avg: "<<(bits[0] + bits[1]*2+ bits[2]*3 )/ 3<<std::endl;

    for (int i = 0; i < 3; i++)
    {
        int useBit = bits[i];
        BloomFilter bf = BloomFilter(useBit);
        // std::cout<<positiveLink[i].size()<<std::endl;
        for (int j = 0; j < positiveLink[i].size(); j++)
        {
            std::string s = std::to_string(positiveLink[i][j].first) + " to " + std::to_string(positiveLink[i][j].second);
            bf.SetKey(s.c_str());
            // std::cout<<s<<std::endl;
        }
        packet.bfs[i] = bf;
    }
}

// This file contains the implementation of functions related to BloomFilter and packet forwarding.
// The BloomFilter class provides functions to set keys and check if a key exists in the filter.
// The generatePacket function generates three BloomFilters based on the positive and negative links in the topology.
// The forwardPacket function forwards the packet to the next node and checks if the link is positive or negative using the BloomFilters. It also calculates the false positive rate for each BloomFilter.
void forwardPacket(Topolopy &topolopy, Session &session, MPacket &packet, int &forwardPacket1, double fpr[3])
{
    int correct_forward[3] = {0,0,0};
    int error_forward[3] = {0,0,0};
    int forwardTotal[3] = {0,0,0};
    Mes mes[3];
    forwardPacket1 = 0;
    std::queue<Mnode*> que;
    que.push(session.sender);
    while (que.size())
    {
        Mnode *node = que.front();
        int BFIndex = session.getBFIndex(node);
        que.pop();

        if (node->type.compare("host") == 0 && node->id != session.sender->id)
            continue; // 只考虑交换机

        for (int i = 0; i < node->linkedNodes.size(); i++)
        {
            Mnode *nextNode = node->linkedNodes[i];
            int a = node->id;
            int b = nextNode->id;
            std::string link = std::to_string(a) + " to " + std::to_string(b);
            bool isPosi = session.isPositiveLink(a, b);

            if (session.distance[node->id] + 1 != session.distance[nextNode->id])
                continue;
            
            if (isPosi && node->id == session.sender->id)
            {
                // forwardPacket1++;
                que.push(nextNode); // 主机不需要bf来编码
                continue;
            }

            if(BFIndex==0&&nextNode->type.compare("host") == 0){ //防止第一跳的leaf交换机的数据包发数据包到主机
                continue;
            }


            if (packet.bfs[BFIndex].VaryExist(link.c_str()))
            {
                forwardTotal[BFIndex]++;
                if (isPosi == 0)
                {
                    std::cout<<"erroredge "<<a<<" "<<b<<std::endl;
                    error_forward[BFIndex]++;
                    forwardPacket1++;

                    mes[0].fp++;
                }else {

                    correct_forward[BFIndex] ++;
                }
                que.push(nextNode);
            }
            else
            {
                mes[0].tn++;
            }
        }
    }

    //  std::cout<<error_forward[0]<<" "<<error_forward[1]<<" "<<error_forward[2]<<std::endl;
}

}