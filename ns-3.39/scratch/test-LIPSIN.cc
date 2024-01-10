#include "ns3/Session.h"
#include "ns3/Topolopy.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-czh-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/udp-orca-server-client-helper.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <map>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("test-czh");

std::map<std::string, std::string> read_config(std::string config_path) {
    std::ifstream config_file(config_path); // 打开文件
    std::map<std::string, std::string> config_values; // 存储配置项的字典
    std::string key;
    std::string value;

    std::cout<< "config_path" << "\t\t\t" << config_path << std::endl;
    if (config_file.is_open()) {
        while (config_file >> key >> value) { // 读取每行的键和值
            config_values[key] = value;
            std::cout<< key << "\t\t\t" << value<< std::endl;
        }
        config_file.close(); // 关闭文件
    } else {
        std::cout << "Unable to open file" << std::endl;
    }
    return config_values;
}

int main(int argc, char* argv[])
{
    std::ifstream topof, flowf;
    NodeContainer nodes;
    InternetStackHelper internet;
    Ipv4CzhRoutingHelper ipv4CzhRoutingHelper;
    CommandLine cmd(__FILE__);
    std::map<std::string, std::string> config_values;
    Ipv4GlobalRoutingHelper globalRouting;
    Ipv4ListRoutingHelper listRouting;
    PointToPointHelper p2p;
    Ipv4AddressHelper ipv4;
    Topolopy* topolopy = new Topolopy();
    Session* session = new Session();

    int nodeNum;
    int linkNum;
    int flowNum;
    
    cmd.Parse(argc, argv);
    config_values = read_config("./scratch/config/setting-czh.txt");
    topof.open(config_values["TOPOLOGY_FILE"]);
    topof >> nodeNum >> linkNum;
    flowf.open(config_values["FLOW_FILE"]);
    flowf >> flowNum;
    nodes.Create(nodeNum);
    
    listRouting.Add(ipv4CzhRoutingHelper, 1);
    listRouting.Add(globalRouting, 0);
    internet.SetRoutingHelper(listRouting);
    internet.Install(nodes);
    
    session->topolopy = topolopy;
    session->multicastProtocol = "LIPSIN";
    for (int i = 0; i < nodeNum; i++){
        Ptr<Ipv4> ipv4 = nodes.Get(i)->GetObject<Ipv4>();
        ns3::Ptr<ns3::Ipv4CzhRouting> routing = ipv4CzhRoutingHelper.GetCzhRouting(ipv4);
        routing->topolopy = topolopy;
        routing->session = session;
    }
    
    for (int i = 0; i < nodeNum; i++){
        Mnode* node = new Mnode();
        node->id = i;
        topolopy->nodes.push_back(node);
    }

    for (int i = 0; i < linkNum; i++){
        int a, b;
        p2p.SetDeviceAttribute("DataRate", StringValue(config_values["DATA_RATE"]));
        p2p.SetChannelAttribute("Delay", StringValue(config_values["DELAY"]));
        // 设置队列属性
        p2p.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(config_values["MAX_QUEUE_LEN"]));
        topof >> a >> b;
        topolopy->nodes[a - 1]->linkedNodes.push_back(topolopy->nodes[b - 1]);
        topolopy->nodes[b - 1]->linkedNodes.push_back(topolopy->nodes[a - 1]);
        NetDeviceContainer devices = p2p.Install(nodes.Get(a - 1), nodes.Get(b - 1));
        char ipstring[25];
        sprintf(ipstring, "10.%d.%d.0", i / 254 + 1, i % 254 + 1);
        ipv4.SetBase(ipstring, "255.255.255.0");
        ipv4.Assign(devices);
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    for (int i = 0; i < flowNum; i++){
        std::vector<int> dsts;
        int src, dstNum, maxPacketCount;
        double start_time, stop_time;
        uint16_t port = 1001 + i;
        flowf >> src >> dstNum >> start_time >> stop_time >> maxPacketCount;

        // ipv4 代表ipv4网络协议栈
        Ptr<Ipv4> ipv4 = nodes.Get(src - 1)->GetObject<Ipv4>();
        ns3::Ptr<ns3::Ipv4CzhRouting> routing = ipv4CzhRoutingHelper.GetCzhRouting(ipv4);
        Ipv4Address clientAddress = ipv4->GetAddress(1, 0).GetLocal(); // GetAddress(0,0) is the loopback 127.0.0.1
        ApplicationContainer apps;
        Ipv4Address serverAddress;
        UdpOrcaClientHelper client(Ipv4Address("0.0.0.0"), port); // multicast address
        client.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
        client.SetAttribute("Interval", TimeValue(NanoSeconds(100)));
        client.SetAttribute("PacketSize", UintegerValue(1224));
        apps = client.Install(nodes.Get(src - 1));
        apps.Start(Seconds(start_time));
        apps.Stop(Seconds(stop_time));

        for (int j = 0; j < dstNum; j++){
            int dst;
            flowf >> dst;
            dsts.push_back(dst - 1);
            Ptr<Ipv4> ipv4 = nodes.Get(dst - 1)->GetObject<Ipv4>();
            serverAddress = ipv4->GetAddress(1, 0).GetLocal(); // GetAddress(0,0) is the loopback 127.0.0.1
            UdpOrcaServerHelper server(clientAddress, port);
            apps = server.Install(nodes.Get(dst - 1));
            apps.Start(Seconds(start_time));
            apps.Stop(Seconds(stop_time));
        }
        session->addNewSession(port, src - 1, dsts);
    }

    AsciiTraceHelper ascii;
    p2p.EnableAsciiAll(ascii.CreateFileStream("./output/test-czh.tr"));
    p2p.EnablePcapAll("./output/test-czh");

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}