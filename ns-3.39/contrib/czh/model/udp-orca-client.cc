/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
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
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 */
#include "udp-orca-client.h"

#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/seq-ts-header.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"
#include "ns3/ipv4-header.h"
#include <cstdio>
#include <cstdlib>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UdpOrcaClient");

NS_OBJECT_ENSURE_REGISTERED(UdpOrcaClient);

TypeId
UdpOrcaClient::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::UdpOrcaClient")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddConstructor<UdpOrcaClient>()
            .AddAttribute(
                "MaxPackets",
                "The maximum number of packets the application will send (zero means infinite)",
                UintegerValue(100),
                MakeUintegerAccessor(&UdpOrcaClient::m_count),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute("Interval",
                          "The time to wait between packets",
                          TimeValue(Seconds(1.0)),
                          MakeTimeAccessor(&UdpOrcaClient::m_interval),
                          MakeTimeChecker())
            .AddAttribute("RemoteAddress",
                          "The destination Address of the outbound packets",
                          AddressValue(),
                          MakeAddressAccessor(&UdpOrcaClient::m_peerAddress),
                          MakeAddressChecker())
            .AddAttribute("RemotePort",
                          "The destination port of the outbound packets",
                          UintegerValue(100),
                          MakeUintegerAccessor(&UdpOrcaClient::m_peerPort),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("PacketSize",
                          "Size of packets generated. The minimum packet size is 12 bytes which is "
                          "the size of the header carrying the sequence number and the time stamp.",
                          UintegerValue(1024),
                          MakeUintegerAccessor(&UdpOrcaClient::m_size),
                          MakeUintegerChecker<uint32_t>(12, 65507))
            .AddAttribute("WindowSize",
                          "允许的最大飞行包数量",
                          UintegerValue(100),
                          MakeUintegerAccessor(&UdpOrcaClient::m_window_size),
                          MakeUintegerChecker<uint32_t>())
            .AddTraceSource("Tx",
                            "A new packet is created and sent",
                            MakeTraceSourceAccessor(&UdpOrcaClient::m_txTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("TxWithAddresses",
                            "A new packet is created and sent",
                            MakeTraceSourceAccessor(&UdpOrcaClient::m_txTraceWithAddresses),
                            "ns3::Packet::TwoAddressTracedCallback");
    return tid;
}

UdpOrcaClient::UdpOrcaClient()
{
    NS_LOG_FUNCTION(this);
    m_sent = 0;
    m_totalTx = 0;
    m_socket = nullptr;
    m_sendEvent = EventId();
    m_ack_num = 0;
    m_flight_size = 0;
    receiver_num = 0;
    finished = false;
}

UdpOrcaClient::~UdpOrcaClient()
{
    NS_LOG_FUNCTION(this);
}

void
UdpOrcaClient::SetRemote(Address ip, uint16_t port)
{
    NS_LOG_FUNCTION(this << ip << port);
    m_peerAddress = ip;
    m_peerPort = port;
}

void
UdpOrcaClient::SetRemote(Address addr)
{
    NS_LOG_FUNCTION(this << addr);
    m_peerAddress = addr;
}

void
UdpOrcaClient::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Application::DoDispose();
}

void
UdpOrcaClient::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (!m_socket)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);

        if (Ipv4Address::IsMatchingType(m_peerAddress))
        {
            if (m_socket->Bind() == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
            m_socket->Connect(
                InetSocketAddress(Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
        else if (Ipv6Address::IsMatchingType(m_peerAddress))
        {
            if (m_socket->Bind6() == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
            m_socket->Connect(
                Inet6SocketAddress(Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
        else if (InetSocketAddress::IsMatchingType(m_peerAddress))
        {
            if (m_socket->Bind() == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
            m_socket->Connect(m_peerAddress);
        }
        else if (Inet6SocketAddress::IsMatchingType(m_peerAddress))
        {
            if (m_socket->Bind6() == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
            m_socket->Connect(m_peerAddress);
        }
        else
        {
            NS_ASSERT_MSG(false, "Incompatible address type: " << m_peerAddress);
        }
    }

#ifdef NS3_LOG_ENABLE
    std::stringstream peerAddressStringStream;
    if (Ipv4Address::IsMatchingType(m_peerAddress))
    {
        peerAddressStringStream << Ipv4Address::ConvertFrom(m_peerAddress);
    }
    else if (Ipv6Address::IsMatchingType(m_peerAddress))
    {
        peerAddressStringStream << Ipv6Address::ConvertFrom(m_peerAddress);
    }
    else if (InetSocketAddress::IsMatchingType(m_peerAddress))
    {
        peerAddressStringStream << InetSocketAddress::ConvertFrom(m_peerAddress).GetIpv4();
    }
    else if (Inet6SocketAddress::IsMatchingType(m_peerAddress))
    {
        peerAddressStringStream << Inet6SocketAddress::ConvertFrom(m_peerAddress).GetIpv6();
    }
    m_peerAddressString = peerAddressStringStream.str();
#endif // NS3_LOG_ENABLE

    // m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    m_socket->SetRecvCallback(MakeCallback(&UdpOrcaClient::HandleRead, this));
    m_socket->SetAllowBroadcast(true);
    m_sendEvent = Simulator::Schedule(Seconds(0.0), &UdpOrcaClient::Send, this);
}

void
UdpOrcaClient::StopApplication()
{
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(m_sendEvent);
}

void
UdpOrcaClient::Send()
{
    // std::cout << "send " << std::endl;
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_sendEvent.IsExpired());
    // std::cout<<"Send"<<m_flight_size<<" "<<m_window_size<<std::endl;
    if (m_flight_size < m_window_size)
    {
        if (m_sent == 0)
        {
            int64_t currentTimeNanoSeconds = ns3::Simulator::Now().GetNanoSeconds();
            std::cout << "czh start flow id = " << m_peerPort - 1000 << " "
                      << currentTimeNanoSeconds << std::endl;
        }
        // std::cout<<"Send"<<m_flight_size<<" "<<m_window_size<<std::endl;
        Address from;
        Address to;
        m_socket->GetSockName(from);
        m_socket->GetPeerName(to);
        SeqTsHeader seqTs;
        seqTs.SetSeq(m_sent);
        NS_ABORT_IF(m_size < seqTs.GetSerializedSize());
        Ptr<Packet> p = Create<Packet>(m_size - seqTs.GetSerializedSize());

        // Trace before adding header, for consistency with PacketSink
        m_txTrace(p);
        m_txTraceWithAddresses(p, from, to);

        p->AddHeader(seqTs);

        if ((m_socket->Send(p)) >= 0)
        {
            m_flight_size++;
            ++m_sent;
            m_totalTx += p->GetSize();
            // std::cout<< p->GetSize() << std::endl;
        }
    }

    //    std::cout <<m_flight_size <<" "<< m_window_size<<std::endl;
    if (m_sent < m_count || m_count == 0)
    {
        m_sendEvent = Simulator::Schedule(m_interval, &UdpOrcaClient::Send, this);
    }
}

uint64_t
UdpOrcaClient::GetTotalTx() const
{
    return m_totalTx;
}

void
UdpOrcaClient::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    Ptr<Packet> packet;
    Address from;
    Address localAddress;
    while ((packet = socket->RecvFrom(from)))
    {
        if (InetSocketAddress::IsMatchingType(from))
        {
            // packet->ToString();
            // Ipv4Header ipv4Header;
            // packet->PeekHeader(ipv4Header);
            // SeqTsHeader seqTs;
            // packet->PeekHeader(seqTs);

            // std::cout<<"receiver seqTs : "<< packet->ToString()<<std::endl;
            // std::cout<<m_ack_num<<std::endl;

            Ipv4Address ipv4Addr = InetSocketAddress::ConvertFrom(from).GetIpv4();
            // std::cout<<ipv4Addr<<std::endl;
            if (receiver_packet_num[ipv4Addr] == 0)
            {
                receivers.push_back(ipv4Addr);
            }
            receiver_packet_num[ipv4Addr]++;
            //  std::cout<<"judge\n";
            //         for(auto it =
            //         receiver_packet_num.begin();it!=receiver_packet_num.end();it++){
            //             std::cout<< (*it).second << " ";
            //         }
            //         std::cout<<std::endl;
            if (receiver_num >= receivers.size())
            {
                int minn = m_count;

                for (auto it = receiver_packet_num.begin(); it != receiver_packet_num.end(); it++)
                {
                    minn = std::min(minn, (*it).second);
                }
                m_flight_size -= minn - m_ack_num;
                m_ack_num += minn - m_ack_num;
                if (m_ack_num == m_count && finished == false)
                {
                    finished = true;
                    // std::cout<<"judge\n";
                    // for(auto it =
                    // receiver_packet_num.begin();it!=receiver_packet_num.end();it++){
                    //     std::cout<< (*it).second << " ";
                    // }
                    std::cout << std::endl;
                    int64_t currentTimeNanoSeconds = ns3::Simulator::Now().GetNanoSeconds();
                    int64_t fct = currentTimeNanoSeconds - 1000000000;
                    std::cout << "\n\n";
                    std::cout << "Finsh flowid : " << m_peerPort - 1000 << ", "
                              << "Fct = " << currentTimeNanoSeconds - 1000000000 << " ns"
                              << ", "
                              << "Rate: " << 1.0 * m_count * m_size * 8 / fct << " Gbps"
                              << std::endl;
                }
            }

            // std::cout<<"czh receive a ack"<<" "<<
            // InetSocketAddress::ConvertFrom(from).GetIpv4()<<std::endl; std::cout<<"czh receive a
            // ack"<<" "<< m_ack_num <<" "<< m_count<<std::endl;

            NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " client received "
                                   << packet->GetSize() << " bytes from "
                                   << InetSocketAddress::ConvertFrom(from).GetIpv4() << " port "
                                   << InetSocketAddress::ConvertFrom(from).GetPort());
        }
        else if (Inet6SocketAddress::IsMatchingType(from))
        {
            NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " client received "
                                   << packet->GetSize() << " bytes from "
                                   << Inet6SocketAddress::ConvertFrom(from).GetIpv6() << " port "
                                   << Inet6SocketAddress::ConvertFrom(from).GetPort());
        }
    }
}
} // Namespace ns3