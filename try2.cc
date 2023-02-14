/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include <stdlib.h>    					 	
#include <math.h>							
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/basic-energy-source.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/simple-device-energy-model.h"
#include "ns3/v4ping-helper.h"
#include "ns3/v4ping.h"
#include "ns3/trace-helper.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"


#include "ns3/netanim-module.h"

using namespace ns3;




uint32_t maxPacketCount = 50;
uint32_t stopTime = 100;

int count = 0;

static bool verbose = 1;
char * stringbuilder( char* prefix,  char* sufix){
  char* buf = (char*)malloc(50); 
  snprintf(buf, 50, "%s%s", prefix, sufix);
  return  buf;
}


  Ptr<FlowMonitor> flowMonitor;
FlowMonitorHelper flowHelper;

ApplicationContainer sendMessage(double time, Ptr<Node>source,Ptr<Node>sink, uint32_t packetSize)
  {
        ApplicationContainer serverAppContainer;
    ApplicationContainer apps;
    
    uint16_t port;
    
  //get the address of the receiver
    Ipv4Address  remoteAddress = sink->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
   
   if(count==2)
   {
        port = 3589;
   }
   
   else
   {
        port = 3689; 
   }
  UdpServerHelper Server(port);

//install the sender as the server 
  serverAppContainer.Add(Server.Install(sink));
  

  
  Time interPacketInterval = Seconds (0.2);
  UdpClientHelper client (remoteAddress, port);
  client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client.SetAttribute ("PacketSize", UintegerValue (packetSize));
  client.SetAttribute ("StartTime", TimeValue (Seconds (time)));
  std::cout<<"source-"<<source->GetObject<Ipv4> ()->GetAddress (1,0).GetLocal()<<" sink-"<<sink->GetObject<Ipv4> ()->GetAddress (1,0).GetLocal()<<std::endl;
  
  apps.Add(client.Install (source));
  
    serverAppContainer.Start (Seconds (1.0));
      serverAppContainer.Stop (Seconds(10));
      
  apps.Start (Seconds (time));
  apps.Stop (Seconds (10));
  
  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  
  //montoring the packets for every message exchanged in order to obtain the readings like transmission rate, receiving rate, etc. 
  flowMonitor = flowHelper.InstallAll();
  
    Simulator::Stop (Seconds (5));
   
  
  Simulator::Run ();
  
  //saving the monitored results into the xml file
  flowMonitor->SerializeToXmlFile("try2.xml", true, true);
  
 
          std::cout<<"REACHED HERE "<<std::endl;
  std::cout << "server apps installed till now :"<<serverAppContainer.GetN ()<< std::endl;
  std::cout << "client apps installed till now :"<<apps.GetN ()<< std::endl;
   
  return apps;
 }
 ApplicationContainer getMessage(YansWifiPhyHelper phy, ApplicationContainer appContainer, double time, Ptr<Node> user1,Ptr<Node> user2)
  {
  
  // This function is to generate the messages from a particular sender to a particular receiver

   if(verbose)
   {
        //displaying the ip addresses of all the parties involved in the protocol
        std::cout<<"user1 : "<< user1->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ();
        std::cout<<"user2 : "<< user2->GetObject<Ipv4> ()->GetAddress (1, 0).GetLocal ()<<std::endl;
        
    }	


        count = 0;
  	appContainer = sendMessage(time, user1, user2 , 50);
  	time = time+0.2;
  	    count = count+1;
	appContainer = sendMessage(time, user2, user1, 100); 
	time = time+0.2;
	    count = count+1;

	appContainer = sendMessage(time, user1, user2, 100); 
	time = time+0.2;
		    count = count+1;

	return appContainer;

}

int main (int argc, char *argv[])
{
  uint32_t stopTime = 10;
  bool verbose = 1;
  CommandLine cmd;  
  cmd.AddValue ("t", "simulation stop time (seconds)", stopTime); 
  cmd.AddValue ("v", "Verbose mode.", verbose);
  
  cmd.Parse (argc, argv); 
  
  if(verbose)
  {
        //This is done to display the transmission time and receiving time along with the delay for each message that is sent
        LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
        LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
        LogComponentEnable("Simulator", LOG_LEVEL_INFO);
  }  
  Config::SetDefault ("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue (40.046));  

  NodeContainer userNodes;
  userNodes.Create (3);
  
  NodeContainer statNodes;
  statNodes.Create(1);
  
  //creating the wireless channel for communication
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default() ;
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();

  channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");

  phy.SetChannel (channel.Create ()); 

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));
               
  NetDeviceContainer userDevices;
  userDevices = wifi.Install (phy, mac, userNodes);
  mac.SetType ("ns3::ApWifiMac",
                 "Ssid", SsidValue (ssid));
                 
 
                 
NetDeviceContainer statDevices;
  statDevices = wifi.Install (phy, mac, statNodes);


  //setting the position of the entities within the grid
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (5.0),
                                 "GridWidth", UintegerValue (5),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.Install (userNodes);

  mobility.Install(statNodes); 

  InternetStackHelper stack;
  stack.Install (userNodes);

  
  stack.Install(statNodes);
  
  
  // Assigning the ip addresses to all the entities involved
  Ipv4AddressHelper address;
  address.SetBase ("10.1.3.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (userDevices);
  address.Assign (statDevices);
  
  

  UdpServerHelper Server (3689);
  
  
  
  ApplicationContainer clientAppContainer, serverAppContainer;
  
  
  
  double time = 3.0;
  
  //calling the generateMessage function to load all the parties to send and receive messages
  
  clientAppContainer = getMessage(phy,clientAppContainer, time, userNodes.Get(0),userNodes.Get(1));
  
  
  
  

  
    Simulator::Destroy ();
  return 0;
}
