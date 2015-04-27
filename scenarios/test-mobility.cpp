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

static void
SetPosition (Ptr<Node> node, Vector position)
{
  Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
  mobility->SetPosition (position);
}

static Vector
GetPosition (Ptr<Node> node)
{
  Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
  return mobility->GetPosition ();
}

static void
AdvancePosition (Ptr<Node> node)
{
  Vector pos = GetPosition (node);
  pos.x += 5.0;
  pos.y += 5.0;
  if (pos.x >= 210.0)
    {
      return;
    }
  SetPosition (node, pos);

  Simulator::Schedule (Seconds (1.0), &AdvancePosition, node);
}

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

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);


Packet::EnablePrinting ();



  // Mobility config

  MobilityHelper mobility;

  Ptr<UniformRandomVariable> randomizer = CreateObject<UniformRandomVariable>();
  randomizer->SetAttribute("Min", DoubleValue(10));
  randomizer->SetAttribute("Max", DoubleValue(100));

  mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
    "X", StringValue ("ns3::UniformRandomVariable[Min=0|Max=100]"),
    "Y", StringValue ("ns3::UniformRandomVariable[Min=0|Max=100]"));

/*
  mobility.SetPositionAllocator("ns3::RandomBoxPositionAllocator", "X", PointerValue(randomizer),
                                "Y", PointerValue(randomizer), "Z", PointerValue(randomizer));
*/
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

  // Wifi config

WifiHelper wifi = WifiHelper::Default ();


  NodeContainer stas;
  NodeContainer ap;
  NetDeviceContainer staDevs;
  PacketSocketHelper packetSocket;

  stas.Create (3);
  ap.Create (2);

  // give packet socket powers to nodes.
  packetSocket.Install (stas);
  packetSocket.Install (ap);

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
  staDevs = wifi.Install (wifiPhy, wifiMac, stas);
  // setup ap.
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));
  wifi.Install (wifiPhy, wifiMac, ap);


  // Install mobility
  mobility.Install (stas);
  mobility.Install (ap);


  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Splitcache",
							   "NormalPolicy", "ns3::ndn::cs::Lru", 
							   "SpecialPolicy", "ns3::ndn::cs::Lru", 
							   "TotalCacheSize", "500", 
							   "Configure", "40"); 
  //                       Percentage Special^
  ndnHelper.Install(ap);
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Nocache");
  ndnHelper.Install(stas);

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::Install(stas, "/", "/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::Install(ap, "/", "/localhost/nfd/strategy/best-route");

  // Installing applications

  // Consumer (basic and special data)
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");
  consumerHelper.SetAttribute("NumberOfContents", StringValue("10")); // 10 different contents
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetPrefix("data/basic");
  consumerHelper.SetAttribute("Frequency", StringValue("1")); // 1 interests a second
  consumerHelper.Install(stas.Get(0));   

  consumerHelper.SetPrefix("data/special");
  consumerHelper.SetAttribute("Frequency", StringValue("2")); //  2 interests a second
  consumerHelper.Install(stas.Get(2));

  consumerHelper.SetPrefix("data/basic");
  consumerHelper.SetAttribute("Frequency", StringValue("1")); // 1 interests a second
  consumerHelper.Install(stas.Get(1));


  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix("/data");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.SetAttribute("Freshness", TimeValue(Seconds(-1.0))); // unlimited freshness
  producerHelper.Install(ap.Get(0)); // first node
  producerHelper.Install(ap.Get(1)); // second node

  Simulator::Schedule (Seconds (1.0), &AdvancePosition, stas.Get (0));

//  Simulator::Schedule (Seconds (1.0), &AdvancePosition, stas.Get (1));

//  Simulator::Schedule (Seconds (2.0), &AdvancePosition, stas.Get (2));



//  PacketSocketAddress socket;
//  socket.SetSingleDevice (staDevs.Get (0)->GetIfIndex ());
//  socket.SetPhysicalAddress (staDevs.Get (1)->GetAddress ());
//  socket.SetProtocol (1);

//  OnOffHelper onoff ("ns3::PacketSocketFactory", Address (socket));
//  onoff.SetConstantRate (DataRate ("500kb/s"));

//  ApplicationContainer apps = onoff.Install (stas.Get (0));
//  apps.Start (Seconds (0.5));
//  apps.Stop (Seconds (43.0));


//  Config::Connect ("/NodeList/*/DeviceList/*/Mac/MacTx", MakeCallback (&DevTxTrace));
//  Config::Connect ("/NodeList/*/DeviceList/*/Mac/MacRx", MakeCallback (&DevRxTrace));
//  Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/RxOk", MakeCallback (&PhyRxOkTrace));
//  Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/RxError", MakeCallback (&PhyRxErrorTrace));
//  Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/Tx", MakeCallback (&PhyTxTrace));
//  Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/State", MakeCallback (&PhyStateTrace));


  AthstatsHelper athstats;
  athstats.EnableAthstats ("athstats-sta", stas);
  athstats.EnableAthstats ("athstats-ap", ap);

  ndn::AppDelayTracer::InstallAll("app-delays-trace.txt");
  ndn::CsTracer::InstallAll("cs-trace.txt", Seconds(1));

  Simulator::Stop(Seconds(20.0));

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
