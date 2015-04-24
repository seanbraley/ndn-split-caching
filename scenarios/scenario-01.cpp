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

// ndn-simple.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

namespace ns3 {

/**
 * This scenario simulates a very simple network topology:
 *
 *         Nocache                    Cache                      Nocache
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
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-simple
 */

int
main(int argc, char* argv[])
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("20"));

  std::uint32_t nTime = 40;
  std::uint32_t nSplit = 40;
  std::uint32_t nSize = 200;
  std::string pNormal = "ns3::ndn::cs::Lru";
  std::string pSpecial = "ns3::ndn::cs::Lru";

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.AddValue("nTime", "Time to run simulation", nTime);
  cmd.AddValue("nSplit", "Percent max Special Cache", nSplit);
  cmd.AddValue("nSize", "Percent max Cache Size", nSize);
  cmd.AddValue("pNormal", "Normal Cache Policy", pNormal);
  cmd.AddValue("pSpecial", "Special Cache Policy", pSpecial);
  cmd.Parse(argc, argv);


  // Creating nodes
  NodeContainer nodes;
  nodes.Create(3);

  // Connecting nodes using two links
  PointToPointHelper p2p;
  p2p.Install(nodes.Get(0), nodes.Get(1));
  p2p.Install(nodes.Get(1), nodes.Get(2));

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Splitcache",
	  "NormalPolicy", pNormal,
	  "SpecialPolicy", pSpecial,
	  "TotalCacheSize", std::to_string(nSize),
	  "Configure", std::to_string(nSplit));
  ndnHelper.Install(nodes.Get(1));
  
  // No cache on consumer/producer
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Nocache");
  ndnHelper.Install(nodes.Get(0));
  ndnHelper.Install(nodes.Get(2));

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/broadcast");

  // Installing applications

  // Consumer
  // cout << "Starting Consumer";
  //ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  // Consumer will request /prefix/0, /prefix/1, ...
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");

  consumerHelper.SetPrefix("data/basic");
  consumerHelper.SetAttribute("Frequency", StringValue("5")); // 1 interests a second
  consumerHelper.Install(nodes.Get(0));                        // first node
  consumerHelper.SetPrefix("data/special");
  consumerHelper.SetAttribute("Frequency", StringValue("15")); //  2 interests a second
  consumerHelper.Install(nodes.Get(0));                        // first node

  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix("/data");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(nodes.Get(2)); // last node

  Simulator::Stop(Seconds(nTime));

  // CS Trace (METRIC 1)
  ndn::CsTracer::InstallAll("cs-trace.txt", Seconds(1));

  // (METRIC 2)
  ndn::AppDelayTracer::InstallAll("app-delays-trace.txt");


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
