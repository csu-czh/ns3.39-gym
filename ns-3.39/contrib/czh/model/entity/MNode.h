
#ifndef _MNode_H
#define _MNode_H
#include "../tools/GLOBAL.h"

namespace ns3
{
  class Mnode{
  public:
    int id;
    int pod;
    std::string type;
    std::vector< Mnode* > linkedNodes;
  };
}
#endif