// https://blogchinmaya.blogspot.com/2017/04/what-is-fat-tree-and-how-to-construct.html
#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <string>
#include <time.h>
#include <vector>
#define maxn 10000
#define maxm 10000
#define INF 1e9 + 7
#define FATTREE_POD 8
using namespace std;

int k = FATTREE_POD, pod = FATTREE_POD;
int coreSwitch[maxm], aggrSwitch[maxn][maxm], edgeSwitch[maxn][maxm], server[maxm];
int sender;
set<int> receiver;
vector<int> ve[maxn];
string type[maxn];
int podID[maxn];

int main()
{
    int id = -1;
    // coreSwitch = (k/2)*(k/2);
    // aggrSwitch = k/2*pod;
    // edgeSwitch = k/2*pod;
    // server = (k/2)*(k/2)*k;
    for (int i = 1; i <= (k / 2) * (k / 2); i++)
    {
        coreSwitch[i] = ++id;
        type[id] = "core";
        podID[id] = 0;
    }

    for (int i = 1; i <= pod; i++)
    {
        for (int j = 1; j <= k / 2; j++){
            aggrSwitch[i][j] = ++id;
            type[id] = "aggr";
            podID[id] = i;
        }
    }
    for (int i = 1; i <= pod; i++)
    {
        for (int j = 1; j <= k / 2; j++)
        {
            edgeSwitch[i][j] = ++id;
            type[id] = "edge";
            podID[id] = i;
        }
    }

    int serverIndex = 1;
    for(int i = 1;i<=k ;i++ ){

        for (int j = 1; j <= (k / 2) * (k / 2) ; j++)
        {
            server[serverIndex++] = ++id;
            type[id] = "host";
            podID[id] = i;
        }
    }
   

    std::ofstream out_file;
    out_file.open("topology.txt");
    //输出节点数量 和 边的数量
    out_file << id + 1<< " " << k * k * k / 4 * 3 << endl;
    for(int i = 0; i < id;i++){
        out_file << i << " " << podID[i] << " " << type[i] << endl;
    }
    int sIndex = 0;
    for (int i = 1; i <= pod; i++)
    {
        int eIndex = 0;
        for (int j = 1; j <= k / 2; j++)
        { // aggrSwitch加边
            for (int z = 1; z <= k / 2; z++)
            {
                out_file << aggrSwitch[i][j] << " " << edgeSwitch[i][z] << endl;
                ve[aggrSwitch[i][j]].push_back(edgeSwitch[i][z]);
                ve[edgeSwitch[i][z]].push_back(aggrSwitch[i][j]);
            }
            for (int z = 1; z <= k / 2; z++)
            {
                out_file << aggrSwitch[i][j] << " " << coreSwitch[++eIndex] << endl;
                ve[aggrSwitch[i][j]].push_back(coreSwitch[eIndex]);
                ve[coreSwitch[eIndex]].push_back(aggrSwitch[i][j]);
            }
        }

        for (int j = 1; j <= k / 2; j++)
        { // edgeSwitch加边
            for (int z = 1; z <= k / 2; z++)
            {
                out_file << edgeSwitch[i][j] << " " << server[++sIndex] << endl;
                ve[edgeSwitch[i][j]].push_back(server[sIndex]);
                ve[server[sIndex]].push_back(edgeSwitch[i][j]);
            }
        }
    }
    out_file.close();
    return 0;
}