//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: George F. Riley<riley@ece.gatech.edu>
//         Gustavo Carneiro <gjc@inescporto.pt>

#define NS_LOG_APPEND_CONTEXT                                                                      \
    if (m_ipv4 && m_ipv4->GetObject<Node>())                                                       \
    {                                                                                              \
        std::clog << Simulator::Now().GetSeconds() << " [node "                                    \
                  << m_ipv4->GetObject<Node>()->GetId() << "] ";                                   \
    }

#include "ipv4-orca-routing.h"

#include "ns3/ipv4-route.h"
#include "ns3/ipv4-routing-table-entry.h"

#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/node.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/udp-header.h"
#include <iomanip>

using std::make_pair;

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4OrcaRouting");

NS_OBJECT_ENSURE_REGISTERED(Ipv4OrcaRouting);

TypeId
Ipv4OrcaRouting::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Ipv4OrcaRouting")
                            .SetParent<Ipv4RoutingProtocol>()
                            .SetGroupName("Internet")
                            .AddConstructor<Ipv4OrcaRouting>();
    return tid;
}

Ipv4OrcaRouting::Ipv4OrcaRouting()
    : m_ipv4(nullptr)
{
    NS_LOG_FUNCTION(this);
}

void
Ipv4OrcaRouting::AddNetworkRouteTo(Ipv4Address network,
                                     Ipv4Mask networkMask,
                                     Ipv4Address nextHop,
                                     uint32_t interface,
                                     uint32_t metric)
{
    NS_LOG_FUNCTION(this << network << " " << networkMask << " " << nextHop << " "
                         << interface << " " << metric);

    Ipv4RoutingTableEntry route =
        Ipv4RoutingTableEntry::CreateNetworkRouteTo(network, networkMask, nextHop, interface);

    if (!LookupRoute(route, metric))
    {
        Ipv4RoutingTableEntry* routePtr = new Ipv4RoutingTableEntry(route);
        m_networkRoutes.emplace_back(routePtr, metric);
    }
}

void
Ipv4OrcaRouting::AddNetworkRouteTo(Ipv4Address network,
                                     Ipv4Mask networkMask,
                                     uint32_t interface,
                                     uint32_t metric)
{
    NS_LOG_FUNCTION(this << network << " " << networkMask << " " << interface << " " << metric);

    Ipv4RoutingTableEntry route =
        Ipv4RoutingTableEntry::CreateNetworkRouteTo(network, networkMask, interface);
    if (!LookupRoute(route, metric))
    {
        Ipv4RoutingTableEntry* routePtr = new Ipv4RoutingTableEntry(route);

        m_networkRoutes.emplace_back(routePtr, metric);
    }
}

void
Ipv4OrcaRouting::AddHostRouteTo(Ipv4Address dest,
                                  Ipv4Address nextHop,
                                  uint32_t interface,
                                  uint32_t metric)
{
    NS_LOG_FUNCTION(this << dest << " " << nextHop << " " << interface << " " << metric);
    AddNetworkRouteTo(dest, Ipv4Mask::GetOnes(), nextHop, interface, metric);
}

void
Ipv4OrcaRouting::AddHostRouteTo(Ipv4Address dest, uint32_t interface, uint32_t metric)
{
    NS_LOG_FUNCTION(this << dest << " " << interface << " " << metric);
    AddNetworkRouteTo(dest, Ipv4Mask::GetOnes(), interface, metric);
}

void
Ipv4OrcaRouting::SetDefaultRoute(Ipv4Address nextHop, uint32_t interface, uint32_t metric)
{
    NS_LOG_FUNCTION(this << nextHop << " " << interface << " " << metric);
    AddNetworkRouteTo(Ipv4Address("0.0.0.0"), Ipv4Mask::GetZero(), nextHop, interface, metric);
}

void
Ipv4OrcaRouting::AddMulticastRoute(Ipv4Address origin,
                                     Ipv4Address group,
                                     uint32_t inputInterface,
                                     std::vector<uint32_t> outputInterfaces)
{
    NS_LOG_FUNCTION(this << origin << " " << group << " " << inputInterface << " "
                         << &outputInterfaces);
    Ipv4MulticastRoutingTableEntry* route = new Ipv4MulticastRoutingTableEntry();
    *route = Ipv4MulticastRoutingTableEntry::CreateMulticastRoute(origin,
                                                                  group,
                                                                  inputInterface,
                                                                  outputInterfaces);
    m_multicastRoutes.push_back(route);
}

// default multicast routes are stored as a network route
// these routes are _not_ consulted in the forwarding process-- only
// for originating packets
void
Ipv4OrcaRouting::SetDefaultMulticastRoute(uint32_t outputInterface)
{
    NS_LOG_FUNCTION(this << outputInterface);
    Ipv4RoutingTableEntry* route = new Ipv4RoutingTableEntry();
    Ipv4Address network = Ipv4Address("224.0.0.0");
    Ipv4Mask networkMask = Ipv4Mask("240.0.0.0");
    *route = Ipv4RoutingTableEntry::CreateNetworkRouteTo(network, networkMask, outputInterface);
    m_networkRoutes.emplace_back(route, 0);
}

uint32_t
Ipv4OrcaRouting::GetNMulticastRoutes() const
{
    NS_LOG_FUNCTION(this);
    return m_multicastRoutes.size();
}

Ipv4MulticastRoutingTableEntry
Ipv4OrcaRouting::GetMulticastRoute(uint32_t index) const
{
    NS_LOG_FUNCTION(this << index);
    NS_ASSERT_MSG(index < m_multicastRoutes.size(),
                  "Ipv4OrcaRouting::GetMulticastRoute ():  Index out of range");

    if (index < m_multicastRoutes.size())
    {
        uint32_t tmp = 0;
        for (MulticastRoutesCI i = m_multicastRoutes.begin(); i != m_multicastRoutes.end(); i++)
        {
            if (tmp == index)
            {
                return *i;
            }
            tmp++;
        }
    }
    return nullptr;
}

bool
Ipv4OrcaRouting::RemoveMulticastRoute(Ipv4Address origin,
                                        Ipv4Address group,
                                        uint32_t inputInterface)
{
    NS_LOG_FUNCTION(this << origin << " " << group << " " << inputInterface);
    for (MulticastRoutesI i = m_multicastRoutes.begin(); i != m_multicastRoutes.end(); i++)
    {
        Ipv4MulticastRoutingTableEntry* route = *i;
        if (origin == route->GetOrigin() && group == route->GetGroup() &&
            inputInterface == route->GetInputInterface())
        {
            delete *i;
            m_multicastRoutes.erase(i);
            return true;
        }
    }
    return false;
}

void
Ipv4OrcaRouting::RemoveMulticastRoute(uint32_t index)
{
    NS_LOG_FUNCTION(this << index);
    uint32_t tmp = 0;
    for (MulticastRoutesI i = m_multicastRoutes.begin(); i != m_multicastRoutes.end(); i++)
    {
        if (tmp == index)
        {
            delete *i;
            m_multicastRoutes.erase(i);
            return;
        }
        tmp++;
    }
}

bool
Ipv4OrcaRouting::LookupRoute(const Ipv4RoutingTableEntry& route, uint32_t metric)
{
    for (NetworkRoutesI j = m_networkRoutes.begin(); j != m_networkRoutes.end(); j++)
    {
        Ipv4RoutingTableEntry* rtentry = j->first;

        if (rtentry->GetDest() == route.GetDest() &&
            rtentry->GetDestNetworkMask() == route.GetDestNetworkMask() &&
            rtentry->GetGateway() == route.GetGateway() &&
            rtentry->GetInterface() == route.GetInterface() && j->second == metric)
        {
            return true;
        }
    }
    return false;
}

Ptr<Ipv4Route>
Ipv4OrcaRouting::LookupStatic(Ipv4Address dest, Ptr<NetDevice> oif)
{
    NS_LOG_FUNCTION(this << dest << " " << oif);
    Ptr<Ipv4Route> rtentry = nullptr;
    uint16_t longest_mask = 0;
    uint32_t shortest_metric = 0xffffffff;
    /* when sending on local multicast, there have to be interface specified */
    if (dest.IsLocalMulticast())
    {
        NS_ASSERT_MSG(
            oif,
            "Try to send on link-local multicast address, and no interface index is given!");

        rtentry = Create<Ipv4Route>();
        rtentry->SetDestination(dest);
        rtentry->SetGateway(Ipv4Address::GetZero());
        rtentry->SetOutputDevice(oif);
        rtentry->SetSource(m_ipv4->GetAddress(m_ipv4->GetInterfaceForDevice(oif), 0).GetLocal());
        return rtentry;
    }

    for (NetworkRoutesI i = m_networkRoutes.begin(); i != m_networkRoutes.end(); i++)
    {
        Ipv4RoutingTableEntry* j = i->first;
        uint32_t metric = i->second;
        Ipv4Mask mask = (j)->GetDestNetworkMask();
        uint16_t masklen = mask.GetPrefixLength();
        Ipv4Address entry = (j)->GetDestNetwork();
        NS_LOG_LOGIC("Searching for route to " << dest << ", checking against route to " << entry
                                               << "/" << masklen);
        if (mask.IsMatch(dest, entry))
        {
            NS_LOG_LOGIC("Found global network route " << j << ", mask length " << masklen
                                                       << ", metric " << metric);
            if (oif)
            {
                if (oif != m_ipv4->GetNetDevice(j->GetInterface()))
                {
                    NS_LOG_LOGIC("Not on requested interface, skipping");
                    continue;
                }
            }
            if (masklen < longest_mask) // Not interested if got shorter mask
            {
                NS_LOG_LOGIC("Previous match longer, skipping");
                continue;
            }
            if (masklen > longest_mask) // Reset metric if longer masklen
            {
                shortest_metric = 0xffffffff;
            }
            longest_mask = masklen;
            if (metric > shortest_metric)
            {
                NS_LOG_LOGIC("Equal mask length, but previous metric shorter, skipping");
                continue;
            }
            shortest_metric = metric;
            Ipv4RoutingTableEntry* route = (j);
            uint32_t interfaceIdx = route->GetInterface();
            rtentry = Create<Ipv4Route>();
            rtentry->SetDestination(route->GetDest());
            rtentry->SetSource(m_ipv4->SourceAddressSelection(interfaceIdx, route->GetDest()));
            rtentry->SetGateway(route->GetGateway());
            rtentry->SetOutputDevice(m_ipv4->GetNetDevice(interfaceIdx));
            if (masklen == 32)
            {
                break;
            }
        }
    }
    if (rtentry)
    {
        NS_LOG_LOGIC("Matching route via " << rtentry->GetGateway() << " at the end");
    }
    else
    {
        NS_LOG_LOGIC("No matching route to " << dest << " found");
    }
    return rtentry;
}

Ptr<Ipv4MulticastRoute>
Ipv4OrcaRouting::LookupStatic(Ipv4Address origin, Ipv4Address group, uint32_t interface)
{
    NS_LOG_FUNCTION(this << origin << " " << group << " " << interface);
    Ptr<Ipv4MulticastRoute> mrtentry = nullptr;

    for (MulticastRoutesI i = m_multicastRoutes.begin(); i != m_multicastRoutes.end(); i++)
    {
        Ipv4MulticastRoutingTableEntry* route = *i;
        //
        // We've been passed an origin address, a multicast group address and an
        // interface index.  We have to decide if the current route in the list is
        // a match.
        //
        // The first case is the restrictive case where the origin, group and index
        // matches.
        //
        if (origin == route->GetOrigin() && group == route->GetGroup())
        {
            // Skipping this case (SSM) for now
            NS_LOG_LOGIC("Found multicast source specific route" << *i);
        }
        if (group == route->GetGroup())
        {
            if (interface == Ipv4::IF_ANY || interface == route->GetInputInterface())
            {
                NS_LOG_LOGIC("Found multicast route" << *i);
                mrtentry = Create<Ipv4MulticastRoute>();
                mrtentry->SetGroup(route->GetGroup());
                mrtentry->SetOrigin(route->GetOrigin());
                mrtentry->SetParent(route->GetInputInterface());
                for (uint32_t j = 0; j < route->GetNOutputInterfaces(); j++)
                {
                    if (route->GetOutputInterface(j))
                    {
                        NS_LOG_LOGIC("Setting output interface index "
                                     << route->GetOutputInterface(j));
                        mrtentry->SetOutputTtl(route->GetOutputInterface(j),
                                               Ipv4MulticastRoute::MAX_TTL - 1);
                    }
                }
                return mrtentry;
            }
        }
    }
    return mrtentry;
}

uint32_t
Ipv4OrcaRouting::GetNRoutes() const
{
    NS_LOG_FUNCTION(this);
    return m_networkRoutes.size();
}

Ipv4RoutingTableEntry
Ipv4OrcaRouting::GetDefaultRoute()
{
    NS_LOG_FUNCTION(this);
    // Basically a repeat of LookupStatic, retained for backward compatibility
    Ipv4Address dest("0.0.0.0");
    uint32_t shortest_metric = 0xffffffff;
    Ipv4RoutingTableEntry* result = nullptr;
    for (NetworkRoutesI i = m_networkRoutes.begin(); i != m_networkRoutes.end(); i++)
    {
        Ipv4RoutingTableEntry* j = i->first;
        uint32_t metric = i->second;
        Ipv4Mask mask = (j)->GetDestNetworkMask();
        uint16_t masklen = mask.GetPrefixLength();
        if (masklen != 0)
        {
            continue;
        }
        if (metric > shortest_metric)
        {
            continue;
        }
        shortest_metric = metric;
        result = j;
    }
    if (result)
    {
        return result;
    }
    else
    {
        return Ipv4RoutingTableEntry();
    }
}

Ipv4RoutingTableEntry
Ipv4OrcaRouting::GetRoute(uint32_t index) const
{
    NS_LOG_FUNCTION(this << index);
    uint32_t tmp = 0;
    for (NetworkRoutesCI j = m_networkRoutes.begin(); j != m_networkRoutes.end(); j++)
    {
        if (tmp == index)
        {
            return j->first;
        }
        tmp++;
    }
    NS_ASSERT(false);
    // quiet compiler.
    return nullptr;
}

uint32_t
Ipv4OrcaRouting::GetMetric(uint32_t index) const
{
    NS_LOG_FUNCTION(this << index);
    uint32_t tmp = 0;
    for (NetworkRoutesCI j = m_networkRoutes.begin(); j != m_networkRoutes.end(); j++)
    {
        if (tmp == index)
        {
            return j->second;
        }
        tmp++;
    }
    NS_ASSERT(false);
    // quiet compiler.
    return 0;
}

void
Ipv4OrcaRouting::RemoveRoute(uint32_t index)
{
    NS_LOG_FUNCTION(this << index);
    uint32_t tmp = 0;
    for (NetworkRoutesI j = m_networkRoutes.begin(); j != m_networkRoutes.end(); j++)
    {
        if (tmp == index)
        {
            delete j->first;
            m_networkRoutes.erase(j);
            return;
        }
        tmp++;
    }
    NS_ASSERT(false);
}

Ptr<Ipv4Route>
Ipv4OrcaRouting::RouteOutput(Ptr<Packet> p,
                               const Ipv4Header& header,
                               Ptr<NetDevice> oif,
                               Socket::SocketErrno& sockerr)
{
    UdpHeader udpHeader;
    p->PeekHeader(udpHeader);
    //czh 表示为ack数据包，走global router
    if(p->GetSize()<200)return nullptr;
    NS_LOG_FUNCTION(this << p << header << oif << sockerr);
    Ipv4Address destination = header.GetDestination();
    // 直接发送到对方面节点，因为主机就一个网卡
    Ptr<Ipv4Route> rtentry = Create<Ipv4Route>();
    rtentry->SetDestination(destination);
    rtentry->SetOutputDevice(m_ipv4->GetNetDevice(1));
    // std::cout<<m_ipv4->GetNetDevice(1)->GetAddress()<<std::endl;
    rtentry->SetSource(m_ipv4->GetAddress(1,0).GetLocal() );
    // std::cout<<"Ipv4OrcaRouting::RouteOutput"<<std::endl;
    // std::cout<<m_ipv4->GetAddress(1,0).GetLocal()<<std::endl;
    // std::cout<<m_ipv4->GetNetDevice(1)->GetAddress()<<std::endl;
    // std::cout<<m_ipv4->GetAddress(1,0).GetLocal().Get()<<std::endl;
    rtentry->SetGateway(getPeerAddress(m_ipv4->GetAddress(1,0).GetLocal()));
    return rtentry;
}

bool
Ipv4OrcaRouting::RouteInput(Ptr<const Packet> p,
                              const Ipv4Header& ipHeader,
                              Ptr<const NetDevice> idev,
                              const UnicastForwardCallback& ucb,
                              const MulticastForwardCallback& mcb,
                              const LocalDeliverCallback& lcb,
                              const ErrorCallback& ecb)
{
    // Ptr<OutputStreamWrapper> stream = Create<OutputStreamWrapper>(&std::cout);
   
    // p->Print(std::cout);
    // 获取端口号
    UdpHeader udpHeader;
    p->PeekHeader(udpHeader);
    int port = udpHeader.GetDestinationPort();
    // std::cout<<"SourcePort: "<<udpHeader.GetDestinationPort()<<std::endl;
   
    //czh 表示为ack数据包，走global router
    if(p->GetSize()<200)return false;
    NS_LOG_FUNCTION(this << p << ipHeader << ipHeader.GetSource() << ipHeader.GetDestination()
                         << idev << &ucb << &mcb << &lcb << &ecb);

    NS_ASSERT(m_ipv4);
    // Check if input device supports IP
    NS_ASSERT(m_ipv4->GetInterfaceForDevice(idev) >= 0);
    uint32_t iif = m_ipv4->GetInterfaceForDevice(idev);

    // Multicast recognition; handle local delivery here

    if (ipHeader.GetDestination().IsMulticast())
    {
        NS_LOG_LOGIC("Multicast destination");
        Ptr<Ipv4MulticastRoute> mrtentry = LookupStatic(ipHeader.GetSource(),
                                                        ipHeader.GetDestination(),
                                                        m_ipv4->GetInterfaceForDevice(idev));
        if (mrtentry)
        {
            NS_LOG_LOGIC("Multicast route found");
             mcb(mrtentry, p, ipHeader); // multicast forwarding callback
            return true;
        }
        else
        {
            NS_LOG_LOGIC("Multicast route not found");
            return false; // Let other routing protocols try to handle this
        }
    }

    if (m_ipv4->IsDestinationAddress(ipHeader.GetDestination(), iif))
    {
        if (!lcb.IsNull())
        {
            NS_LOG_LOGIC("Local delivery to " << ipHeader.GetDestination());
            lcb(p, ipHeader, iif);
            return true;
        }
        else
        {
            // The local delivery callback is null.  This may be a multicast
            // or broadcast packet, so return false so that another
            // multicast routing protocol can handle it.  It should be possible
            // to extend this to explicitly check whether it is a unicast
            // packet, and invoke the error callback if so
            return false;
        }
    }
   
    // Check if input device supports IP forwarding
    if (!m_ipv4->IsForwarding(iif))
    {
        NS_LOG_LOGIC("Forwarding disabled for this interface");
        ecb(p, ipHeader, Socket::ERROR_NOROUTETOHOST);
        return true;
    }
    // czh 
    // Next, try to find a route
    // 获取到输入端口的ipv4地址
    Ipv4Address inputAddress = m_ipv4->GetAddress(m_ipv4->GetInterfaceForDevice(idev), 0).GetLocal();
    int32_t interface = m_ipv4->GetInterfaceForDevice(idev);
    int nodeId = idev->GetNode()->GetId();
    std::string type = topolopy->nodes[nodeId]->type;
   
    int flowId = port - 1001;
     if (!lcb.IsNull() && sessions[flowId].receivers.find(nodeId) != sessions[flowId].receivers.end())
    {
        // std::cout<<"orca host "<<nodeId<<std::endl;
        NS_LOG_LOGIC("Local delivery to " << ipHeader.GetDestination());
        lcb(p, ipHeader, iif);
        return true;
    }
    // std::cout<<"nodeId "<<nodeId<<std::endl;
    // std::cout<<"_ipv4->GetNInterfaces() "<<m_ipv4->GetNInterfaces()<<std::endl;
    // std::cout<<"type: "<<topolopy->nodes[nodeId]->type <<" "<< nodeId<<std::endl;
    if(type.compare("agent") == 0){
        // std::cout<<"type: "<<topolopy->nodes[nodeId]->type <<" "<< nodeId<<std::endl;
        int32_t outInterface = 1; // 直接发出去
        Ipv4Address outAddress = m_ipv4->GetAddress(outInterface, 0).GetLocal();
        // std::cout<<"inputInterface "<< interface<<std::endl;
        // std::cout<<"inputAddress " << inputAddress <<std::endl;
        // std::cout<<"outInterface "<< outInterface<<std::endl;
        // std::cout<<"outAddress " << outAddress <<std::endl;
        Ptr<Ipv4Route> rtentry = Create<Ipv4Route>();
        rtentry->SetGateway(getPeerAddress(outAddress));
        rtentry->SetOutputDevice(m_ipv4->GetNetDevice(outInterface));
        rtentry->SetDestination(ipHeader.GetDestination() );
        rtentry->SetSource(m_ipv4->GetAddress(interface, 0).GetLocal());
        ucb(rtentry, p, ipHeader); // unicast forwarding callback
    }else if(type.compare("leaf") == 0 && (m_ipv4->GetInterfaceForDevice(idev) == 1 || m_ipv4->GetInterfaceForDevice(idev) == 2)){
        // std::cout<<"agent"<<std::endl;
        int32_t outInterface = 5; // 直接发出去
        Ipv4Address outAddress = m_ipv4->GetAddress(outInterface, 0).GetLocal();
        // std::cout<<"inputInterface "<< interface<<std::endl;
        // std::cout<<"inputAddress " << inputAddress <<std::endl;
        // std::cout<<"outInterface "<< outInterface<<std::endl;
        // std::cout<<"outAddress " << outAddress <<std::endl;
        Ptr<Ipv4Route> rtentry = Create<Ipv4Route>();
        rtentry->SetGateway(getPeerAddress(outAddress));
        rtentry->SetOutputDevice(m_ipv4->GetNetDevice(outInterface));
        rtentry->SetDestination(ipHeader.GetDestination() );
        rtentry->SetSource(m_ipv4->GetAddress(interface, 0).GetLocal());
        ucb(rtentry, p, ipHeader); // unicast forwarding callback
    }else{
        for(int i=1;i<m_ipv4->GetNInterfaces();i++){
            // std::cout<<nodeId<<" "<<i-1<<std::endl;
            if(sessions[flowId].m_links.find(make_pair(nodeId,i-1)) ==  sessions[flowId].m_links.end())
                continue;
            // std::cout<<"send "<<nodeId<<" "<<i<<std::endl;
            int32_t outInterface = i;
            Ipv4Address outAddress = m_ipv4->GetAddress(outInterface, 0).GetLocal();
            // std::cout<<"inputInterface "<< interface<<std::endl;
            // std::cout<<"inputAddress " << inputAddress <<std::endl;
            // std::cout<<"outInterface "<< outInterface<<std::endl;
            // std::cout<<"outAddress " << outAddress <<std::endl;
            Ptr<Ipv4Route> rtentry = Create<Ipv4Route>();
            rtentry->SetGateway(getPeerAddress(outAddress));
            rtentry->SetOutputDevice(m_ipv4->GetNetDevice(outInterface));
            rtentry->SetDestination(ipHeader.GetDestination() );
            rtentry->SetSource(m_ipv4->GetAddress(interface, 0).GetLocal());
            ucb(rtentry, p, ipHeader); // unicast forwarding callback
        }
    // std::cout<<idev->GetNode()->GetId()<<std::endl;
    }
    
    return true;
}

Ipv4OrcaRouting::~Ipv4OrcaRouting()
{
    NS_LOG_FUNCTION(this);
}

void
Ipv4OrcaRouting::DoDispose()
{
    NS_LOG_FUNCTION(this);
    for (NetworkRoutesI j = m_networkRoutes.begin(); j != m_networkRoutes.end();
         j = m_networkRoutes.erase(j))
    {
        delete (j->first);
    }
    for (MulticastRoutesI i = m_multicastRoutes.begin(); i != m_multicastRoutes.end();
         i = m_multicastRoutes.erase(i))
    {
        delete (*i);
    }
    m_ipv4 = nullptr;
    Ipv4RoutingProtocol::DoDispose();
}

void
Ipv4OrcaRouting::NotifyInterfaceUp(uint32_t i)
{
    NS_LOG_FUNCTION(this << i);
    // If interface address and network mask have been set, add a route
    // to the network of the interface (like e.g. ifconfig does on a
    // Linux box)
    for (uint32_t j = 0; j < m_ipv4->GetNAddresses(i); j++)
    {
        if (m_ipv4->GetAddress(i, j).GetLocal() != Ipv4Address() &&
            m_ipv4->GetAddress(i, j).GetMask() != Ipv4Mask() &&
            m_ipv4->GetAddress(i, j).GetMask() != Ipv4Mask::GetOnes())
        {
            AddNetworkRouteTo(
                m_ipv4->GetAddress(i, j).GetLocal().CombineMask(m_ipv4->GetAddress(i, j).GetMask()),
                m_ipv4->GetAddress(i, j).GetMask(),
                i);
        }
    }
}

void
Ipv4OrcaRouting::NotifyInterfaceDown(uint32_t i)
{
    NS_LOG_FUNCTION(this << i);
    // Remove all static routes that are going through this interface
    for (NetworkRoutesI it = m_networkRoutes.begin(); it != m_networkRoutes.end();)
    {
        if (it->first->GetInterface() == i)
        {
            delete it->first;
            it = m_networkRoutes.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void
Ipv4OrcaRouting::NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address)
{
    NS_LOG_FUNCTION(this << interface << " " << address.GetLocal());
    if (!m_ipv4->IsUp(interface))
    {
        return;
    }

    Ipv4Address networkAddress = address.GetLocal().CombineMask(address.GetMask());
    Ipv4Mask networkMask = address.GetMask();
    if (address.GetLocal() != Ipv4Address() && address.GetMask() != Ipv4Mask())
    {
        AddNetworkRouteTo(networkAddress, networkMask, interface);
    }
}

void
Ipv4OrcaRouting::NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address)
{
    NS_LOG_FUNCTION(this << interface << " " << address.GetLocal());
    if (!m_ipv4->IsUp(interface))
    {
        return;
    }
    Ipv4Address networkAddress = address.GetLocal().CombineMask(address.GetMask());
    Ipv4Mask networkMask = address.GetMask();
    // Remove all static routes that are going through this interface
    // which reference this network
    for (NetworkRoutesI it = m_networkRoutes.begin(); it != m_networkRoutes.end();)
    {
        if (it->first->GetInterface() == interface && it->first->IsNetwork() &&
            it->first->GetDestNetwork() == networkAddress &&
            it->first->GetDestNetworkMask() == networkMask)
        {
            delete it->first;
            it = m_networkRoutes.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void
Ipv4OrcaRouting::SetIpv4(Ptr<Ipv4> ipv4)
{
    NS_LOG_FUNCTION(this << ipv4);
    NS_ASSERT(!m_ipv4 && ipv4);
    m_ipv4 = ipv4;
    for (uint32_t i = 0; i < m_ipv4->GetNInterfaces(); i++)
    {
        if (m_ipv4->IsUp(i))
        {
            NotifyInterfaceUp(i);
        }
        else
        {
            NotifyInterfaceDown(i);
        }
    }
}

// Formatted like output of "route -n" command
void
Ipv4OrcaRouting::PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
    NS_LOG_FUNCTION(this << stream);
    std::ostream* os = stream->GetStream();
    // Copy the current ostream state
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);

    *os << "Node: " << m_ipv4->GetObject<Node>()->GetId() << ", Time: " << Now().As(unit)
        << ", Local time: " << m_ipv4->GetObject<Node>()->GetLocalTime().As(unit)
        << ", Ipv4OrcaRouting table" << std::endl;

    if (GetNRoutes() > 0)
    {
        *os << "Destination     Gateway         Genmask         Flags Metric Ref    Use Iface"
            << std::endl;
        for (uint32_t j = 0; j < GetNRoutes(); j++)
        {
            std::ostringstream dest;
            std::ostringstream gw;
            std::ostringstream mask;
            std::ostringstream flags;
            Ipv4RoutingTableEntry route = GetRoute(j);
            dest << route.GetDest();
            *os << std::setw(16) << dest.str();
            gw << route.GetGateway();
            *os << std::setw(16) << gw.str();
            mask << route.GetDestNetworkMask();
            *os << std::setw(16) << mask.str();
            flags << "U";
            if (route.IsHost())
            {
                flags << "HS";
            }
            else if (route.IsGateway())
            {
                flags << "GS";
            }
            *os << std::setw(6) << flags.str();
            *os << std::setw(7) << GetMetric(j);
            // Ref ct not implemented
            *os << "-"
                << "      ";
            // Use not implemented
            *os << "-"
                << "   ";
            if (!Names::FindName(m_ipv4->GetNetDevice(route.GetInterface())).empty())
            {
                *os << Names::FindName(m_ipv4->GetNetDevice(route.GetInterface()));
            }
            else
            {
                *os << route.GetInterface();
            }
            *os << std::endl;
        }
    }
    *os << std::endl;
    // Restore the previous ostream state
    (*os).copyfmt(oldState);
}

Ipv4Address
Ipv4OrcaRouting::getPeerAddress(Ipv4Address address){
    if(address.Get() % 2 ){
        return Ipv4Address(address.Get() + 1);
        // rtentry->SetGateway( );
        // std::cout<<Ipv4Address(m_ipv4->GetAddress(1,0).GetLocal().Get() + 1)  <<std::endl;
    }else{
        return Ipv4Address(address.Get() - 1);
    }
};

} // namespace ns3
