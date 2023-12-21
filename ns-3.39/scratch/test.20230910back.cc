#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-orca-routing-helper.h"
#include "ns3/udp-orca-server-client-helper.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("test");

int main(int argc, char* argv[]) {
    std::ifstream topof, flowf;
    NodeContainer nodes;
    InternetStackHelper internet;
    Ipv4OrcaRoutingHelper ipv4OrcaRoutingHelper;
    CommandLine cmd(__FILE__);

    int nodeNum;
    int linkNum;
    int flowNum;

    cmd.Parse(argc, argv);
	topof.open("/home/chenzihao/workspace/ns3/ns-allinone-3.39/ns-3.39/scratch/config/topology.txt");
    topof>>nodeNum>>linkNum;
    flowf.open("/home/chenzihao/workspace/ns3/ns-allinone-3.39/ns-3.39/scratch/config/flow.txt");
    flowf>>flowNum;
    nodes.Create (nodeNum);
    Ipv4GlobalRoutingHelper globalRouting;
    Ipv4ListRoutingHelper listRouting;
    listRouting.Add(ipv4OrcaRoutingHelper, 1);
    listRouting.Add(globalRouting, 0);
    internet.SetRoutingHelper(listRouting);
    internet.Install(nodes);
    PointToPointHelper p2p;
    Ipv4AddressHelper ipv4;
    for (int i = 0; i < linkNum; i++){
        int a,b;
        p2p.SetDeviceAttribute("DataRate", StringValue("100GBps"));
        p2p.SetChannelAttribute("Delay", StringValue("1us"));
        // 设置队列属性
        p2p.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue("300p"));
        
        topof>>a>>b;
        NetDeviceContainer devices = p2p.Install(nodes.Get(a-1), nodes.Get(b-1));
        char ipstring[25];
		sprintf(ipstring, "10.%d.%d.0", i / 254 + 1, i % 254 + 1);
		ipv4.SetBase(ipstring, "255.255.255.0");
		ipv4.Assign(devices);
    }
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    std::cout<<"start"<<std::endl;
    for(int i = 0;i <flowNum; i++){
        // 21 36 1 10 10000
        int src,dst,maxPacketCount;
        double start_time, stop_time;
        uint16_t port = 1001 + i;
        flowf>>src>>dst >> start_time >> stop_time >> maxPacketCount;
        Ptr<Ipv4> ipv4 = nodes.Get(dst-1)->GetObject<Ipv4>();
		Ipv4Address serverAddress = ipv4->GetAddress(1, 0).GetLocal(); //GetAddress(0,0) is the loopback 127.0.0.1
        ipv4 = nodes.Get(src-1)->GetObject<Ipv4>();
        Ipv4Address clientAddress = ipv4->GetAddress(1, 0).GetLocal(); //GetAddress(0,0) is the loopback 127.0.0.1
        UdpOrcaClientHelper client(serverAddress, port);
        client.SetAttribute ("MaxPackets", UintegerValue (1));
        client.SetAttribute ("Interval", TimeValue (Seconds (0)));
        client.SetAttribute ("PacketSize", UintegerValue (1024));

        ApplicationContainer apps = client.Install (nodes.Get(src - 1));
        apps.Start(Seconds(start_time));
        apps.Stop(Seconds(stop_time));

        UdpOrcaServerHelper server(clientAddress,port);
        apps = server.Install(nodes.Get(dst - 1));
        apps.Start(Seconds(start_time));
        apps.Stop(Seconds(stop_time));
        apps = server.Install(nodes.Get(dst - 2));
        apps.Start(Seconds(start_time));
        apps.Stop(Seconds(stop_time));
    }
    AsciiTraceHelper ascii;
    p2p.EnableAsciiAll(ascii.CreateFileStream("test.tr"));
    p2p.EnablePcapAll("test");

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
