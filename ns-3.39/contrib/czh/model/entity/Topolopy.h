#ifndef _TOPOLOPY_H
#define _TOPOLOPY_H
#include "../tools/GLOBAL.h"
#include "../entity/MNode.h"
namespace ns3
{
  class Topolopy{
  public:
    std::vector<Mnode*> nodes;
    std::vector<Mnode*> hosts;
    std::vector<Mnode*> switchs;
    int pod; //fattree 拓扑
  };

  void generateLeafSpine(Topolopy& topolopy);
  void generateFatTree(Topolopy& topolopy);
}
#endif