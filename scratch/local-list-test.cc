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

// Network topology
//
//       n0    n1    n2    n3
//       |     |     |     |
//       ===================
//         5ms   5ms   15ms
//
// - UDP flows from n0 to n1 and n3 to n2

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include <fstream>

#include "s2e.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("test");

int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::MS);
  //LogComponentEnable ("test", LOG_LEVEL_INFO);
  
  uint32_t numpackets = 1;
  uint64_t interval = 1024;
  uint64_t firstSymPacket = 1;
  
  CommandLine cmd;
  cmd.AddValue ("interval", "Range of the interval", interval);
  cmd.AddValue ("num", "Total number of symbolic packets", numpackets);
  cmd.AddValue ("first", "First packet to be symbolic", firstSymPacket);
  cmd.Parse (argc, argv);

  Simulator::SetInterval (interval);
  Simulator::SetNumberSymPackets (numpackets);
  Simulator::SetFirstSymPacket (firstSymPacket);

  NodeContainer nodes1,nodes2,nodes3;
  nodes1.Create (2);
  nodes2.Add(nodes1.Get(1));
  nodes2.Create (1);
  nodes3.Add(nodes2.Get(1));
  nodes3.Create (1);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("5ms"));
  NetDeviceContainer devices1;
  devices1 = pointToPoint.Install (nodes1);
  NetDeviceContainer devices2;
  devices2 = pointToPoint.Install (nodes2);
  
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("15ms"));
  NetDeviceContainer devices3;
  devices3 = pointToPoint.Install (nodes3);

  InternetStackHelper stack;
  stack.Install (nodes1.Get(0));
  stack.Install (nodes2.Get(0));
  stack.Install (nodes3);

  
  Ipv4AddressHelper address1;
  address1.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4AddressHelper address2;
  address2.SetBase ("10.0.1.0", "255.255.255.0");
  Ipv4AddressHelper address3;
  address3.SetBase ("10.0.2.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces1 = address1.Assign (devices1);
  Ipv4InterfaceContainer interfaces2 = address2.Assign (devices2);
  Ipv4InterfaceContainer interfaces3 = address3.Assign (devices3);

  Address serverAddress;
  uint32_t MaxPacketSize = 1024;
  Time interPacketInterval = Seconds (0.5);
  uint32_t maxPacketCount = 1;
  uint16_t port = 4000;

//-----------------------------
// Create one udpServer applications on node one.
//  
  UdpServerHelper server (port);
  ApplicationContainer apps = server.Install (nodes1.Get (1));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (10.0));

//
// Create one UdpClient application to send UDP datagrams from node zero to
// node one.
//
  serverAddress = Address (interfaces1.GetAddress (1));
  UdpClientHelper client (serverAddress, port);
  client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
  apps = client.Install (nodes1.Get (0));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (10.0)); 

//----------------------------
// Create one udpServer applications on node two.
//
  UdpServerHelper server1 (port);
  ApplicationContainer apps1 = server1.Install (nodes3.Get (0));
  apps1.Start (Seconds (1.0));
  apps1.Stop (Seconds (10.0));

//
// Create one UdpClient application to send UDP datagrams from node three to
// node two.
//
  serverAddress = Address (interfaces3.GetAddress (0));
  UdpClientHelper client1 (serverAddress, port);
  client1.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client1.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client1.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
  apps1 = client1.Install (nodes3.Get (1));
  apps1.Start (Seconds (1.0));
  apps1.Stop (Seconds (10.0)); 

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  //AsciiTraceHelper ascii;
  //pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("local-list-test.tr"));
  //pointToPoint.EnablePcapAll ("local-list-test");

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop(Seconds(15));
  Simulator::Run ();

  Simulator::Destroy ();
  
  s2e_kill_state (0, "End of Simulation");
  return 0;
}
