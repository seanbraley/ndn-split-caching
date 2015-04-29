/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

// test-mobility.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/config-store-module.h"
#include "ns3/mobility-module.h"
#include "ns3/athstats-helper.h"
#include "ns3/internet-module.h"

#include "ns3/ndnSIM-module.h"

#include <iostream>

using namespace std;
namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ndn.TestMobility");

/**
 * This scenario simulates a very simple network topology:
 *
 *
 *      +----------+     1Mbps      +--------+     1Mbps      +----------+
 *      | consumer | <------------> | router | <------------> | producer |
 *      +----------+         10ms   +--------+          10ms  +----------+
 *
 *
 * Consumer requests data from producer with frequency 10 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=test-example
 */


int
main(int argc, char* argv[])
{
  // setting default parameters for Wifi
  // enable rts cts all the time.
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
  // disable fragmentation
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode",
                     StringValue("OfdmRate24Mbps"));

  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("20"));

  std::uint32_t max_routers = 1;
  std::string cSize = "100";
  std::string cSplit = "75";

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.AddValue("cSize", "Cache Size", cSize);
  cmd.AddValue("cSplit", "Cache Split", cSplit);
  cmd.AddValue("routers", "number of routers", max_routers);
  cmd.Parse(argc, argv);


  Packet::EnablePrinting ();

  // Wifi config

  WifiHelper wifi = WifiHelper::Default ();


  // Nodes
  NodeContainer sta_consumers;
  NodeContainer sta_mobile_consumers;
  NodeContainer ap;
  NodeContainer routers;
  NodeContainer producers;

  // ??
  NetDeviceContainer staDevs;
  PacketSocketHelper packetSocket;

  // 5 stationary consumers, 5 mobile, 4 APs and 1 router
  sta_consumers.Create(3*max_routers);
  sta_mobile_consumers.Create(7*max_routers);
  ap.Create (5*max_routers);
  routers.Create(max_routers);
  producers.Create(1);

  // give packet socket powers to nodes.
  packetSocket.Install(sta_mobile_consumers);
  packetSocket.Install(sta_consumers);
  packetSocket.Install (ap);

  // Wifi Config
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiPhy.Set("TxPowerStart", DoubleValue(5));
  wifiPhy.Set("TxPowerEnd", DoubleValue(5));
  Ssid ssid = Ssid ("wifi-default");
  wifi.SetRemoteStationManager ("ns3::ArfWifiManager");
  // setup stas.
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "ActiveProbing", BooleanValue (false));
  // install wifi
  wifi.Install(wifiPhy, wifiMac, sta_mobile_consumers);
  wifi.Install(wifiPhy, wifiMac, sta_consumers);
  // setup ap.
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));
  wifi.Install (wifiPhy, wifiMac, ap);


  // Mobility config -- Change max_routers value to change size of sim
  int number_rows = sqrt(max_routers);
  int x = 0;
  int y = 0;
  int pos_counter = 0;
  Ptr<UniformRandomVariable> randomizerX = CreateObject<UniformRandomVariable>();
  Ptr<UniformRandomVariable> randomizerY = CreateObject<UniformRandomVariable>();

  MobilityHelper stationary_mobility;
  stationary_mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::GaussMarkovMobilityModel", "Bounds", BoxValue(Box (0, number_rows*10, 0, number_rows*10, 0, 0)), 
	  "TimeStep", TimeValue(Seconds(1)));

  // Place router in center of box
  randomizerX->SetAttribute("Min", DoubleValue(5));
  randomizerX->SetAttribute("Max", DoubleValue(5));
  randomizerY->SetAttribute("Min", DoubleValue(5));
  randomizerY->SetAttribute("Max", DoubleValue(5));
  stationary_mobility.SetPositionAllocator("ns3::RandomBoxPositionAllocator", "X", PointerValue(randomizerX),
	  "Y", PointerValue(randomizerY));

  // Install on routers
  stationary_mobility.Install(producers.Get(0));

  // p2p helper
  PointToPointHelper p2p;
  // for each row
  for (int i=0; i < number_rows; i++)
  {
	  x = i * 10 + 5;
	  // for each column
	  for (int j=0; j < number_rows; j++)
	  {
		  y = j * 10 + 5;

		  // Place router in center of box
		  randomizerX->SetAttribute("Min", DoubleValue(x));
		  randomizerX->SetAttribute("Max", DoubleValue(x));
		  randomizerY->SetAttribute("Min", DoubleValue(y));
		  randomizerY->SetAttribute("Max", DoubleValue(y));
		  stationary_mobility.SetPositionAllocator("ns3::RandomBoxPositionAllocator", "X", PointerValue(randomizerX),
			  "Y", PointerValue(randomizerY));

		  // Install on routers
		  stationary_mobility.Install(routers.Get(pos_counter));

		  // Set box (center +/-5)
		  randomizerX->SetAttribute("Min", DoubleValue(x-5));
		  randomizerX->SetAttribute("Max", DoubleValue(x+5));
		  randomizerY->SetAttribute("Min", DoubleValue(y-5));
		  randomizerY->SetAttribute("Max", DoubleValue(y+5));
		  // Connect router to previous if not 1
		  if (pos_counter == 0)
		  {
			  p2p.Install(routers.Get(0), producers.Get(0));
		  }
		  else  // Otherwise connect to router behind you
		  {
			  p2p.Install(routers.Get(pos_counter), routers.Get(pos_counter-1));
		  }
		  // APs
		  for (int k=0; k < 5; k++)
		  {
			  stationary_mobility.SetPositionAllocator("ns3::RandomBoxPositionAllocator", "X", PointerValue(randomizerX),
				  "Y", PointerValue(randomizerY));
			  stationary_mobility.Install(ap.Get(pos_counter * 5 + k));
			  p2p.Install(routers.Get(pos_counter), ap.Get(pos_counter * 5 + k));

		  }

		  // Consumers (stationary)
		  for (int l=0; l < 3; l++)
		  {
			  stationary_mobility.SetPositionAllocator("ns3::RandomBoxPositionAllocator", "X", PointerValue(randomizerX),
				  "Y", PointerValue(randomizerY));
			  stationary_mobility.Install(sta_consumers.Get(pos_counter * 3 + l));
		  }

		  // Consumers (Mobile)
		  for (int m=0; m < 7; m++)
		  {
			  mobility.SetPositionAllocator("ns3::RandomBoxPositionAllocator", "X", PointerValue(randomizerX),
				  "Y", PointerValue(randomizerY));
			  mobility.Install(sta_mobile_consumers.Get(pos_counter * 7 + m));
		  }
	  // Keep track of overall position
	  pos_counter++;
	  }
  }
  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  /*
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Splitcache",
							   "NormalPolicy", "ns3::ndn::cs::Lru", 
							   "SpecialPolicy", "ns3::ndn::cs::Lru", 
							   "TotalCacheSize", "500", 
							   "Configure", "40"); 
  //                       Percentage Special^
  */
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", cSize);

  ndnHelper.Install(ap);
  ndnHelper.Install(routers);
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Nocache");
  ndnHelper.Install(sta_consumers);
  ndnHelper.Install(sta_mobile_consumers);
  ndnHelper.Install(producers);

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::Install(sta_consumers, "/", "/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::Install(sta_mobile_consumers, "/", "/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::Install(ap, "/", "/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::Install(routers, "/", "/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::Install(producers, "/", "/localhost/nfd/strategy/best-route");

  // Installing applications

  // Consumer (basic and special data)
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");
  consumerHelper.SetAttribute("NumberOfContents", StringValue("100")); // 10 different contents
  // Consumer will request /prefix/0, /prefix/1, ...

  // Basic consumers request basic data (and pumpkin spice coffee)
  consumerHelper.SetPrefix("data/");
  consumerHelper.SetAttribute("Frequency", StringValue("10")); // 1 interests a second
  consumerHelper.Install(sta_consumers);   
  consumerHelper.Install(sta_mobile_consumers);

  // Mobile consumers request special data only
  //consumerHelper.SetPrefix("data/special");
  //consumerHelper.SetAttribute("Frequency", StringValue("10")); //  2 interests a second



  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix("/data");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.SetAttribute("Freshness", TimeValue(Seconds(-1.0))); // unlimited freshness
  producerHelper.Install(producers); 

  // Tracers 'n stuff
  //AthstatsHelper athstats;
  //athstats.EnableAthstats("athstats-sta", sta_mobile_consumers);
  //athstats.EnableAthstats("athstats-sta", sta_consumers);
  //athstats.EnableAthstats ("athstats-ap", ap);

  ndn::AppDelayTracer::Install(sta_consumers, "app-delays-trace-stationary-01.txt");
  ndn::AppDelayTracer::Install(sta_mobile_consumers, "app-delays-trace-mobile-01.txt");
  ndn::CsTracer::Install(ap, "cs-trace-ap-01.txt", Seconds(1));
  ndn::CsTracer::Install(routers, "cs-trace-routers-01.txt", Seconds(1));

  // 10 min
  Simulator::Stop(Seconds(600.0));

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3


int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
