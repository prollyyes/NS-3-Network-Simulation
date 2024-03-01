#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/rectangle.h"
#include "ns3/netanim-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/udp-echo-helper.h"
#include "ns3/udp-echo-server.h"
#include "ns3/udp-echo-client.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-routing-protocol.h"
#include "iostream"
#include "ns3/log.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Task_2077561");


int main(int argc, char* argv[]) {

    CommandLine cmd(__FILE__);
    bool enableRtsCts = false;
    bool tracing = false;
    std::string studentId;
    cmd.AddValue("enableRtsCts", "Enable/disable RTS/CTS", enableRtsCts);
    cmd.AddValue("studentId", "Student ID", studentId);
    cmd.AddValue("tracing", "Enable pcap tracing", tracing);
    cmd.Parse(argc, argv);

    if (enableRtsCts) {
        Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("0"));
    } else {
        Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("999999"));
    }

    if (studentId.empty()) {
        NS_LOG_UNCOND("Student ID is mandatory. Please provide the correct student ID to start the simulation.");
        return 0;
    }

    //first subnet for network C

    NodeContainer networkC, routerC;
    networkC.Create(2);
    routerC.Create(1);

    /*
    //assign names
    std::map<uint32_t, std::string> nodeNamesNetC;
    nodeNamesNetC[networkC.Get(0)->GetId()] = "networkC_Node0";
    nodeNamesNetC[networkC.Get(1)->GetId()] = "networkC_Node1";
    nodeNamesNetC[routerC.Get(0)->GetId()] = "routerC";

    //create a map to get the pointer to the node from its name
    std::map<std::string, Ptr<Node>> nodeNameToPtrNetC;
    nodeNameToPtrNetC["networkC_Node0"] = networkC.Get(0);
    nodeNameToPtrNetC["networkC_Node1"] = networkC.Get(1);
    nodeNameToPtrNetC["routerC"] = routerC.Get(0);
    */ 
            //abort, may be confusing to use this way of identifying nodes. I will use
            //the default IDs assigned by ns3 and keep in mind the order of creation.

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("200ms"));

    NetDeviceContainer devices1, devices2;
    devices1.Add(pointToPoint.Install(networkC.Get(0), routerC.Get(0))); 
    devices2.Add(pointToPoint.Install(networkC.Get(1), routerC.Get(0)));

    //install the Internet stack with routing enabled on the router

    InternetStackHelper stackC;
    stackC.Install(networkC);
    stackC.Install(routerC);

    //initially, I tried using the same IP subnet for both links, but it didn't work, it generated
    //an ambiguity in the routing table. So I used two different subnets, one for each link.
    //now, there's slighlty more overhead, but it works.

    //upon later examination, my first approach was inherently wrong since a PTP link requires two
    //two addresses. I was trying to assign the same address to two different PTP links, which is
    //wrong. The second approach is the correct one.

    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.252"); //first ptp link
    Ipv4InterfaceContainer interfacesC1 = address.Assign(devices1);

    address.SetBase("192.168.1.4", "255.255.255.252"); //second ptp link
    Ipv4InterfaceContainer interfacesC2 = address.Assign(devices2);


   
    //second subnet for network D
    
    NodeContainer networkD, routerD, serverD;
    serverD.Create(1);
    routerD.Create(1);
    networkD.Add(serverD);
    networkD.Add(routerD);

    NetDeviceContainer devicesD;
    CsmaHelper csmaD;
    csmaD.SetChannelAttribute("DataRate", StringValue("10Mbps"));
    csmaD.SetChannelAttribute("Delay", TimeValue(MilliSeconds(200)));
    devicesD = csmaD.Install(networkD);

    //ip addresses

    Ipv4AddressHelper addressD;
    InternetStackHelper stackD;
    stackD.Install(networkD);
    stackD.Install(routerD);
    addressD.SetBase("192.168.1.16", "255.255.255.252");
    Ipv4InterfaceContainer interfacesD = addressD.Assign(devicesD);

    //second subnet for network B

    NodeContainer networkB; //nodes except the router
    NodeContainer routerB; //router

    routerB.Create(1);
    networkB.Create(3);
    NodeContainer allNodesB = NodeContainer(routerB, networkB);

    NetDeviceContainer devicesB;
    CsmaHelper csmaB;
    csmaB.SetChannelAttribute("DataRate", StringValue("5Mbps"));
    csmaB.SetChannelAttribute("Delay", TimeValue(MilliSeconds(20)));
    devicesB = csmaB.Install(allNodesB);
    //ip addresses and routing

    Ipv4AddressHelper addressB;
    InternetStackHelper stackB;
    stackB.Install(allNodesB);
    stackB.Install(routerB);
    addressB.SetBase("192.168.1.8", "255.255.255.248");
    Ipv4InterfaceContainer interfacesB = addressB.Assign(devicesB);

    //fourth subnet for network A

    NodeContainer networkA, wifiNodes, apNode;

    apNode.Create(1);
    networkA.Add(apNode);
    wifiNodes.Create(7);
    networkA.Add(wifiNodes);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211g);
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    WifiMacHelper mac;
    Ssid ssid = Ssid("2077561");

    //routing
    InternetStackHelper stackA;
    stackA.Install(apNode);
    stackA.Install(wifiNodes);

    NetDeviceContainer staDevices;
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    staDevices = wifi.Install(phy, mac, wifiNodes);

    Ipv4AddressHelper addressA;
    addressA.SetBase("192.168.1.32", "255.255.255.240");

    NetDeviceContainer apDevices;
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    apDevices = wifi.Install(phy, mac, apNode);


    Ipv4InterfaceContainer apInterfaces = addressA.Assign(apDevices);
    Ipv4InterfaceContainer staInterfaces = addressA.Assign(staDevices);

    //mobility for stationary nodes
    MobilityHelper mobilityAP;
    mobilityAP.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityAP.Install(apNode);
    mobilityAP.Install(networkC);
    mobilityAP.Install(routerC);
    mobilityAP.Install(networkB);
    mobilityAP.Install(routerB);
    mobilityAP.Install(networkD);

    //mobility for stations (random walk)
    MobilityHelper mobilityStations;
    mobilityStations.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                          "X", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=30.0]"),
                                          "Y", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=30.0]"));

    mobilityStations.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                                      "Bounds", RectangleValue(Rectangle(0,30,0,30)));
    mobilityStations.Install(wifiNodes);


    //link all existing nodes via a shared router.
    //L1
    NodeContainer L1;
    L1.Add(routerC.Get(0));
    L1.Add(routerD.Get(0));
    mobilityAP.Install(L1);

    PointToPointHelper pointToPointL1;
    pointToPointL1.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPointL1.SetChannelAttribute("Delay", StringValue("20ms"));

    NetDeviceContainer devicesL1 = pointToPointL1.Install(L1);
    Ipv4AddressHelper addressL1;
    InternetStackHelper stackL1;
    stackL1.Install(L1);
    addressL1.SetBase("192.168.1.48", "255.255.255.252");
    Ipv4InterfaceContainer interfacesL1 = addressL1.Assign(devicesL1);

    //L2
    NodeContainer L2;
    L2.Add(apNode.Get(0));
    L2.Add(routerD.Get(0));
    mobilityAP.Install(L2);

    PointToPointHelper pointToPointL2;
    pointToPointL2.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPointL2.SetChannelAttribute("Delay", StringValue("20ms"));

    NetDeviceContainer devicesL2 = pointToPointL2.Install(L2);
    Ipv4AddressHelper addressL2;
    InternetStackHelper stackL2;
    stackL2.Install(L2);
    addressL2.SetBase("192.168.1.52", "255.255.255.252");
    Ipv4InterfaceContainer interfacesL2 = addressL2.Assign(devicesL2);

    //L3
    NodeContainer L3;
    L3.Add(routerD.Get(0));
    L3.Add(routerB.Get(0));
    mobilityAP.Install(L3);

    PointToPointHelper pointToPointL3;
    pointToPointL3.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPointL3.SetChannelAttribute("Delay", StringValue("20ms"));

    InternetStackHelper stackL3;
    stackL3.Install(L3);
    NetDeviceContainer devicesL3 = pointToPointL3.Install(L3);
    Ipv4AddressHelper addressL3;
    addressL3.SetBase("192.168.1.56", "255.255.255.252");
    Ipv4InterfaceContainer interfacesL3 = addressL3.Assign(devicesL3);


    //NetAnim
    AnimationInterface anim("task_2077561.xml");
    anim.EnableIpv4RouteTracking("routingtable-wireless.xml",
                                 Seconds(0),
                                 Seconds(15));

    
    //applications
    //TCP 0.72s
    //sender
    uint16_t port = 1025;
    BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(interfacesC1.GetAddress(0), port));
    source.SetAttribute("MaxBytes", UintegerValue(1221 * 1024 * 1024));
    ApplicationContainer sourceApps = source.Install(networkB.Get(0));
    sourceApps.Start(Seconds(0.72));
    //receiver
    PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(networkC.Get(0));
    sinkApps.Start(Seconds(0.71));

    //TCP 3.93s
    //sender
    uint16_t port1 = 1026;
    BulkSendHelper source1("ns3::TcpSocketFactory", InetSocketAddress(interfacesC2.GetAddress(0), port1));
    source1.SetAttribute("MaxBytes", UintegerValue(1264 * 1024 * 1024));
    ApplicationContainer sourceApps1 = source1.Install(wifiNodes.Get(5));
    sourceApps1.Start(Seconds(3.93));
    //receiver
    PacketSinkHelper sink1("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port1));
    ApplicationContainer sinkApps1 = sink1.Install(networkC.Get(1));
    sinkApps1.Start(Seconds(0.0));

    //TCP 3.87s
    //sender
    uint16_t port2 = 1027;
    BulkSendHelper source2("ns3::TcpSocketFactory", InetSocketAddress(interfacesC1.GetAddress(0), port2));
    source2.SetAttribute("MaxBytes", UintegerValue(1754 * 1024 * 1024));
    ApplicationContainer sourceApps2 = source2.Install(wifiNodes.Get(6));
    sourceApps2.Start(Seconds(3.87));

    //receiver
    PacketSinkHelper sink2("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port2));
    ApplicationContainer sinkApps2 = sink2.Install(networkC.Get(0));
    sinkApps2.Start(Seconds(3.86));

    //UDP Echo Application
    //server
    uint16_t port3 = 1028;
    UdpEchoServerHelper server(port3);
    ApplicationContainer serverApps = server.Install(networkD.Get(0));
    serverApps.Start(Seconds(0.0));

    //client
    UdpEchoClientHelper client(interfacesD.GetAddress(0), port3);
    client.SetAttribute("MaxPackets", UintegerValue(250));
    client.SetAttribute("Interval", TimeValue(Seconds(0.02)));
    client.SetAttribute("PacketSize", UintegerValue(1964));
    ApplicationContainer clientApps = client.Install(wifiNodes.Get(2));
    clientApps.Start(Seconds(0.0));

    //set the data to send in the packets
    Ptr<UdpEchoClient> echoClient = DynamicCast<UdpEchoClient>(clientApps.Get(0));
    echoClient->SetFill("Edoardo, Simonetti, Emmanuele, Piola, Antonio, Ponti, Tania, Silveri");
    
    //generate routing tables log file
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    Ipv4GlobalRoutingHelper g;
    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>("routing-table.log", std::ios::out);
    g.PrintRoutingTableAllAt(Seconds(0), routingStream);
    //end of routing tables log file





    //what's inside intefacesC1?
    std::cout << "InterfacesC\n"<< std::endl;
    for (uint32_t i = 0; i < interfacesC1.GetN(); i++) {
        Ptr<Ipv4> ipv4 = interfacesC1.Get(i).first;
        uint32_t interfaceIndex = interfacesC1.Get(i).second;
        Ipv4Address ipAddress = ipv4->GetAddress(interfaceIndex, 0).GetLocal();
        std::cout << "Node: " << ipv4->GetObject<Node>()->GetId() << ", Interface Index: " << interfaceIndex << ", IP Address: " << ipAddress << std::endl;
    }

    //what's inside networkC?
    std::cout << "\nNetworkC\n" << std::endl;
    for (uint32_t i = 0; i < networkC.GetN(); i++) {
        Ptr<Node> node = networkC.Get(i);
        std::cout << "Node ID: " << node->GetId() << std::endl;
    }

    //what's inside networkB?
    std::cout << "\nNetworkB\n" << std::endl;
    for (uint32_t i = 0; i < networkB.GetN(); i++) {
        Ptr<Node> node = networkB.Get(i);
        std::cout << "Node ID: " << node->GetId() << std::endl;
    }

    //what's inside networkD?
    std::cout << "\nNetworkD\n" << std::endl;
    for (uint32_t i = 0; i < networkD.GetN(); i++) {
        Ptr<Node> node = networkD.Get(i);
        std::cout << "Node ID: " << node->GetId() << std::endl;
    }

    //What's inside interfacesD?
    std::cout << "\nInterfacesD\n" << std::endl;
    for (uint32_t i = 0; i < interfacesD.GetN(); i++) {
        Ptr<Ipv4> ipv4 = interfacesD.Get(i).first;
        uint32_t interfaceIndex = interfacesD.Get(i).second;
        Ipv4Address ipAddress = ipv4->GetAddress(interfaceIndex, 0).GetLocal();
        std::cout << "Node: " << ipv4->GetObject<Node>()->GetId() << ", Interface Index: " << interfaceIndex << ", IP Address: " << ipAddress << std::endl;
    }

    //generate pcap files for all nodes
    if (tracing) {

        pointToPoint.EnablePcapAll("task");
        pointToPointL1.EnablePcapAll("task");
        pointToPointL2.EnablePcapAll("task");
        pointToPointL3.EnablePcapAll("task");

        csmaD.EnablePcap("task", devicesD.Get(0)); //3
        csmaD.EnablePcap("task", devicesD.Get(1)); //4
        csmaB.EnablePcap("task", devicesB.Get(0)); //5
        csmaB.EnablePcap("task", devicesB.Get(1)); //6
        csmaB.EnablePcap("task", devicesB.Get(0)); //7
        csmaB.EnablePcap("task", devicesB.Get(2)); //8

        phy.EnablePcap("task", apDevices.Get(0)); //9
        phy.EnablePcap("task", staDevices.Get(3)); //10
        phy.EnablePcap("task", staDevices.Get(1)); //11
        phy.EnablePcap("task", staDevices.Get(4)); //12
        phy.EnablePcap("task", staDevices.Get(6)); //13
        phy.EnablePcap("task", staDevices.Get(0)); //14
        phy.EnablePcap("task", staDevices.Get(2)); //15
        phy.EnablePcap("task", staDevices.Get(5)); //16
        
    }

    //why does the ap move? START DEBUGGING
    /*
    Ptr<MobilityModel> mobilityModel = apNode.Get(0)->GetObject<MobilityModel>();
    if (mobilityModel) {
        std::cout << "Mobility model for AP: " << mobilityModel->GetInstanceTypeId().GetName() << std::endl;
    } else {
        std::cout << "No mobility model found for AP" << std::endl;
    }
    */
    //END DEBUGGING

    Simulator::Stop(Seconds(15.0));


    Simulator::Run();
    Simulator::Destroy();

}