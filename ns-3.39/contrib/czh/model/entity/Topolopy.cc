#include"Topolopy.h"
namespace ns3
{
void generateFatTree(Topolopy& topolopy){
    int pod = FATTREE_POD;
    int id = 0;
    int coreSwitch[(pod/2)*(pod/2) + 7];
    int aggrSwitch[pod + 7][pod/2 + 7];
    int edgeSwitch[pod + 7][pod/2 + 7];
    int server[(pod/2)*(pod/2)*pod +7];
    int belongPod[MAX_NODE];
    std::vector< Mnode* > nodes;
    std::vector<Mnode*> hosts;
    std::vector<Mnode*> switchs;
    //  Mnode 
    // coreSwitch = (pod/2)*(pod/2); 
    // aggrSwitch = pod/2*pod; 
    // edgeSwitch = pod/2*pod; 
    // server = (pod/2)*(pod/2)*pod;

    for(int i=1;i<=(pod/2)*(pod/2);i++){
        coreSwitch[i] = ++id;
        belongPod[id] = 0;
    }   

    for(int i=1;i<=pod;i++){
        for(int j=1;j<=pod/2;j++){
            aggrSwitch[i][j] = ++id;
            belongPod[id] = i;
        }
    }

    for(int i=1;i<=pod;i++){
        for(int j=1;j<=pod/2;j++){
            edgeSwitch[i][j] = ++id;
            belongPod[id] = i;
        }
    }

    for(int i=1;i<=(pod/2)*(pod/2)*pod;i++){
        server[i] = ++id;
        belongPod[id] = (i-1)/(pod/2*pod/2)+1;
    }

    int n = id;
    Mnode* node = new Mnode;
    nodes.push_back(node);
    for(int i=1;i<=id;i++){ //实际有效的id为1-id
        Mnode* node = new Mnode;
        node->id = i;
        node->pod = belongPod[i];
        nodes.push_back(node);
        
    }
        
    //分组
    for(int i=1;i<=(pod/2)*(pod/2);i++){
        int id = coreSwitch[i];
        nodes[id]->type = "core";
        switchs.push_back(nodes[id]);
    }

    for(int i=1;i<=pod;i++){
        for(int j=1;j<=pod/2;j++){
            int id = aggrSwitch[i][j];
            nodes[id]->type = "aggr";
            switchs.push_back(nodes[id]);
        }
    }

    for(int i=1;i<=pod;i++){
        for(int j=1;j<=pod/2;j++){
            int id = edgeSwitch[i][j];
            nodes[id]->type = "edge";
            switchs.push_back(nodes[id]);
        }
    }

    for(int i=1;i<=(pod/2)*(pod/2)*pod;i++){
        int id = server[i];
        nodes[id]->type = "host";
        hosts.push_back(nodes[id]);

    }

    int sIndex = 0;
    int links = 0;
    for(int i=1;i<=pod;i++){
        int eIndex = 0;
        for(int j=1;j<=pod/2;j++){// aggrSwitch加边
            for(int z=1;z<=pod/2;z++){
                nodes[aggrSwitch[i][j]]->linkedNodes.push_back(nodes[edgeSwitch[i][z]]);
                nodes[edgeSwitch[i][z]]->linkedNodes.push_back(nodes[aggrSwitch[i][j]]);
                links ++;
            }

            for(int z=1;z<=pod/2;z++){
                nodes[aggrSwitch[i][j]]->linkedNodes.push_back(nodes[coreSwitch[++eIndex]]);
                nodes[coreSwitch[eIndex]]->linkedNodes.push_back(nodes[aggrSwitch[i][j]]);
                links ++;
            }
        }

        for(int j=1;j<=pod/2;j++){// edgeSwitch加边
            for(int z=1;z<=pod/2;z++){
                nodes[edgeSwitch[i][j]]->linkedNodes.push_back(nodes[server[++sIndex]]);
                nodes[server[sIndex]]->linkedNodes.push_back(nodes[edgeSwitch[i][j]]);
                links ++;
            }
        }
    }
    // std::cout<<"Links:"<<links<<std::endl;
    topolopy.nodes = nodes;
    topolopy.hosts = hosts;
    topolopy.switchs = switchs;
    topolopy.pod = pod;
}

void generateLeafSpine(Topolopy& topolopy){
    const int leafNum = LEAF_NUM;
    const int spineNum = SPINE_NUM;
    const int hostPerLeaf = HOST_PERLEAF_NUM;
    int spine[spineNum];
    int leaf[leafNum];
    int host[leafNum][hostPerLeaf];
    int id = -1;
    std::vector< Mnode* > nodes;
    std::vector<Mnode*> hosts;
    std::vector<Mnode*> switchs;
    
    for(int i=0;i<spineNum;i++){
        spine[i] = ++id;
        Mnode* node = new Mnode;
        node->id = id;
        node->type="spine";
        nodes.push_back(node);
        switchs.push_back(node);
    }

    for(int i=0;i<leafNum;i++){
        leaf[i] = ++id;
        Mnode* node = new Mnode;
        node->id = id;
        node->type="leaf";
        nodes.push_back(node);
        switchs.push_back(node);
    }

    for(int i=0;i<leafNum;i++){
        for(int j=0;j<hostPerLeaf;j++){
            host[i][j] = ++id;
            Mnode* node = new Mnode;
            node->type="host";
            node->id = id;
            node->pod = i;
            nodes.push_back(node);
            hosts.push_back(node);
        }
    }

    for(int i=0;i<leafNum;i++){
        for(int j=0;j<spineNum;j++){
            nodes[spine[j]]->linkedNodes.push_back(nodes[leaf[i]]);
            nodes[leaf[i]]->linkedNodes.push_back(nodes[spine[j]]);
        }
        for(int j=0;j<hostPerLeaf;j++){
            nodes[leaf[i]]->linkedNodes.push_back(nodes[host[i][j]]);
            nodes[host[i][j]]->linkedNodes.push_back(nodes[leaf[i]]);
        }
    }
    
    // for(int i=0;i<nodes.size();i++){    // 打乱边，让组播树随机点
    //     std::random_shuffle(nodes[i]->linkedNodes.begin(), nodes[i]->linkedNodes.end());
    // }

    topolopy.nodes = nodes;
    topolopy.hosts = hosts;
    topolopy.switchs = switchs;
}
}