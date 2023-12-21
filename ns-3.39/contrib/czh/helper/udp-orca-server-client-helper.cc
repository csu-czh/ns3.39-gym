/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */
#include "udp-orca-server-client-helper.h"

#include "ns3/string.h"
#include "ns3/udp-orca-client.h"
#include "ns3/udp-orca-server.h"
#include "ns3/udp-trace-client.h"
#include "ns3/uinteger.h"

namespace ns3
{

UdpOrcaServerHelper::UdpOrcaServerHelper()
{
    m_factory.SetTypeId(UdpOrcaServer::GetTypeId());
}

UdpOrcaServerHelper::UdpOrcaServerHelper(Address address, uint16_t port)
{
    m_factory.SetTypeId(UdpOrcaServer::GetTypeId());
    SetAttribute("Port", UintegerValue(port));
    SetAttribute("RemoteAddress", AddressValue(address));
    SetAttribute("RemotePort", UintegerValue(port));
}

void
UdpOrcaServerHelper::SetAttribute(std::string name, const AttributeValue& value)
{
    m_factory.Set(name, value);
}

ApplicationContainer
UdpOrcaServerHelper::Install(NodeContainer c)
{
    ApplicationContainer apps;
    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        Ptr<Node> node = *i;
        m_server = m_factory.Create<UdpOrcaServer>();
        node->AddApplication(m_server);
        apps.Add(m_server);
    }
    return apps;
}

Ptr<UdpOrcaServer>
UdpOrcaServerHelper::GetServer()
{
    return m_server;
}

UdpOrcaClientHelper::UdpOrcaClientHelper()
{
    m_factory.SetTypeId(UdpOrcaClient::GetTypeId());
}

UdpOrcaClientHelper::UdpOrcaClientHelper(Address address, uint16_t port)
{
    m_factory.SetTypeId(UdpOrcaClient::GetTypeId());
    SetAttribute("RemoteAddress", AddressValue(address));
    SetAttribute("RemotePort", UintegerValue(port));
}

UdpOrcaClientHelper::UdpOrcaClientHelper(Address address)
{
    m_factory.SetTypeId(UdpOrcaClient::GetTypeId());
    SetAttribute("RemoteAddress", AddressValue(address));
}

void
UdpOrcaClientHelper::SetAttribute(std::string name, const AttributeValue& value)
{
    m_factory.Set(name, value);
}

ApplicationContainer
UdpOrcaClientHelper::Install(NodeContainer c)
{
    ApplicationContainer apps;
    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        Ptr<Node> node = *i;
        Ptr<UdpOrcaClient> client = m_factory.Create<UdpOrcaClient>();
        node->AddApplication(client);
        apps.Add(client);
    }
    return apps;
}

UdpOrcaTraceClientHelper::UdpOrcaTraceClientHelper()
{
    m_factory.SetTypeId(UdpTraceClient::GetTypeId());
}

UdpOrcaTraceClientHelper::UdpOrcaTraceClientHelper(Address address, uint16_t port, std::string filename)
{
    m_factory.SetTypeId(UdpTraceClient::GetTypeId());
    SetAttribute("RemoteAddress", AddressValue(address));
    SetAttribute("RemotePort", UintegerValue(port));
    SetAttribute("TraceFilename", StringValue(filename));
}

UdpOrcaTraceClientHelper::UdpOrcaTraceClientHelper(Address address, std::string filename)
{
    m_factory.SetTypeId(UdpTraceClient::GetTypeId());
    SetAttribute("RemoteAddress", AddressValue(address));
    SetAttribute("TraceFilename", StringValue(filename));
}

void
UdpOrcaTraceClientHelper::SetAttribute(std::string name, const AttributeValue& value)
{
    m_factory.Set(name, value);
}

ApplicationContainer
UdpOrcaTraceClientHelper::Install(NodeContainer c)
{
    ApplicationContainer apps;
    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        Ptr<Node> node = *i;
        Ptr<UdpTraceClient> client = m_factory.Create<UdpTraceClient>();
        node->AddApplication(client);
        apps.Add(client);
    }
    return apps;
}

} // namespace ns3
