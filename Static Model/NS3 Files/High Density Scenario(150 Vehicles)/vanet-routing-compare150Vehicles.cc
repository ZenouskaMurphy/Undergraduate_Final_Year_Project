 /* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
 /*
  * Copyright (c) 2014 North Carolina State University
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
  * Author: Scott E. Carpenter <scarpen@ncsu.edu>
  *
  */  
/*
  * This example program allows one to run vehicular ad hoc
  * network (VANET) simulation scenarios in ns-3 to assess
  * performance by evaluating different 802.11p MAC/PHY
  * characteristics, propagation loss models (e.g. Friss,
  * Two-Ray Ground, or ITU R-P.1411), and application traffic
  * (e.g. Basic Safety Message) and/or routing traffic (e.g.
  * DSDV, AODV, OLSR, or DSR) by playing back mobility trace files (i.e. ns-2 movement files). 
  * 
    The script draws from several ns-3 examples, including:
  * /examples/routing/manet-routing-compare.cc
  * /src/propagation/model/itu-r-1411-los-propagation-loss-model.cc
  * /src/mobility/examples/ns2-mobility-trace.cc
  * /src/wave/examples/wave-simple-80211p.cc
  *
  *
  * All parameters can be changed from their defaults (see
  * --help) and changing simulation parameters can have dramatic
  * impact on network performance.
  *
  * Several items can be output:
  * - a CSV file of data reception statistics, output once per
  *   second
  * - final statistics, in a CSV file
  * - dump of routing tables at 5 seconds into the simulation
  * - ASCII trace file
  * 
  *
  * Simulation scenarios can be defined and configuration
  * settings can be saved using config-store (raw text)
  * which can they be replayed again.  This is an easy way
  * to define and save the settings for a scenario, and then
  * re-execute the same scenario exactly, or to set up
  * several different simulation scenarios.
  * For example, to set up a scenario and save the configuration
  * as "scenario1.txt":
  *   ./waf --run "vanet-routing-compare --scenario=1 --saveconfig=scenario1.txt"
  * Then, to re-play the scenario using the save configuration
  * settings:
  *   ./waf --run "vanet-routing-compare --loadconfig=scenario1.txt"
  *
  * Class Diagram:
  *   main()
  *     +--uses-- VanetRoutingExperiment
  *                 +--is_a--- WifiApp
  *                 +--uses--- ConfigStoreHelper
  *                 +--has_a-- WaveBsmHelper
  *                 |            +--has_a-- WaveBsmStats
  *                 +--has_a-- RoutingHelper
  *                 |            +--has_a--RoutingStats
  *                 +--has_a-- WifiPhyStats
  *
  */
 
 #include <fstream> //filestream
 #include <iostream>
 #include "ns3/core-module.h"
 #include "ns3/network-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/mobility-module.h"
 #include "ns3/aodv-module.h"
 #include "ns3/olsr-module.h"
 #include "ns3/dsdv-module.h"
 #include "ns3/dsr-module.h"
 #include "ns3/applications-module.h"
 #include "ns3/itu-r-1411-los-propagation-loss-model.h"
 #include "ns3/ocb-wifi-mac.h"
 #include "ns3/wifi-80211p-helper.h"
 #include "ns3/wave-mac-helper.h"
 #include "ns3/config-store-module.h"
 #include "ns3/integer.h"
 #include "ns3/wave-bsm-helper.h"
 #include "ns3/wave-helper.h"
 #include "ns3/yans-wifi-helper.h"
 #include "ns3/netanim-module.h"      
 
 using namespace ns3;
 using namespace dsr;
 
 NS_LOG_COMPONENT_DEFINE ("vanet-routing-compare");
 
 class RoutingStats
 {
 public:
   RoutingStats ();
 
   uint32_t GetRxBytes ();
 
   uint32_t GetCumulativeRxBytes ();
 
   uint32_t GetRxPkts ();
 
   uint32_t GetCumulativeRxPkts ();
 
   void IncRxBytes (uint32_t rxBytes);
 
   void IncRxPkts ();
 
   void SetRxBytes (uint32_t rxBytes);
 
   void SetRxPkts (uint32_t rxPkts);
 
   uint32_t GetTxBytes ();
 
   uint32_t GetCumulativeTxBytes ();
 
   uint32_t GetTxPkts ();
 
   uint32_t GetCumulativeTxPkts ();
 
   void IncTxBytes (uint32_t txBytes);
 
   void IncTxPkts ();
 
   void SetTxBytes (uint32_t txBytes);
 
   void SetTxPkts (uint32_t txPkts);
 
 private:
   uint32_t m_RxBytes; 
   uint32_t m_cumulativeRxBytes; 
   uint32_t m_RxPkts; 
   uint32_t m_cumulativeRxPkts; 
   uint32_t m_TxBytes; 
   uint32_t m_cumulativeTxBytes; 
   uint32_t m_TxPkts; 
   uint32_t m_cumulativeTxPkts; 
 };
 
 RoutingStats::RoutingStats ()
   : m_RxBytes (0),
     m_cumulativeRxBytes (0),
     m_RxPkts (0),
     m_cumulativeRxPkts (0),
     m_TxBytes (0),
     m_cumulativeTxBytes (0),
     m_TxPkts (0),
     m_cumulativeTxPkts (0)
 {
 }
 
 uint32_t
 RoutingStats::GetRxBytes ()
 {
   return m_RxBytes;
 }
 
 uint32_t
 RoutingStats::GetCumulativeRxBytes ()
 {
   return m_cumulativeRxBytes;
 }
 
 uint32_t
 RoutingStats::GetRxPkts ()
 {
   return m_RxPkts;
 }
 
 uint32_t
 RoutingStats::GetCumulativeRxPkts ()
 {
   return m_cumulativeRxPkts;
 }
 
 void
 RoutingStats::IncRxBytes (uint32_t rxBytes)
 {
   m_RxBytes += rxBytes;
   m_cumulativeRxBytes += rxBytes;
 }
 
 void
 RoutingStats::IncRxPkts ()
 {
   m_RxPkts++;
   m_cumulativeRxPkts++;
 }
 
 void
 RoutingStats::SetRxBytes (uint32_t rxBytes)
 {
   m_RxBytes = rxBytes;
 }
 
 void
 RoutingStats::SetRxPkts (uint32_t rxPkts)
 {
   m_RxPkts = rxPkts;
 }
 
 uint32_t
 RoutingStats::GetTxBytes ()
 {
   return m_TxBytes;
 }
 
 uint32_t
 RoutingStats::GetCumulativeTxBytes ()
 {
   return m_cumulativeTxBytes;
 }
 
 uint32_t
 RoutingStats::GetTxPkts ()
 {
   return m_TxPkts;
 }
 
 uint32_t
 RoutingStats::GetCumulativeTxPkts ()
 {
   return m_cumulativeTxPkts;
 }
 
 void
 RoutingStats::IncTxBytes (uint32_t txBytes)
 {
   m_TxBytes += txBytes;
   m_cumulativeTxBytes += txBytes;
 }
 
 void
 RoutingStats::IncTxPkts ()
 {
   m_TxPkts++;
   m_cumulativeTxPkts++;
 }
 
 void
 RoutingStats::SetTxBytes (uint32_t txBytes)
 {
   m_TxBytes = txBytes;
 }
 
 void
 RoutingStats::SetTxPkts (uint32_t txPkts)
 {
   m_TxPkts = txPkts;
 }
 
 class RoutingHelper : public Object
 {
 public:
   static TypeId GetTypeId (void);
 
   RoutingHelper ();
 
   virtual ~RoutingHelper ();
 

  //Parameter NodeContainer =c
  //Parameter NetDeviceContainer = d
  //IPV4 InterfaceContainer = i
  //Parameter TotalTime = total time that nodes should attempt to route data
  //Paramater Protocol = The running protocol (1 = OLSR, 2 = AODV, 3 = DSDV, 4 = DSR)
  //Parameter RoutingTables = dump routing tables at t = 5 seconds (0=no, 1=yes)
   void Install (NodeContainer & c,
                 NetDeviceContainer & d,
                 Ipv4InterfaceContainer & i,
                 double totalTime,
                 int protocol,
                 uint32_t nSinks,
                 int routingTables);
 
   void OnOffTrace (std::string context, Ptr<const Packet> packet);
 
   RoutingStats & GetRoutingStats ();
 
   void SetLogging (int log);
 
 private:
   void SetupRoutingProtocol (NodeContainer & c);
  //Assigns IPV4 addresses to net devices and their interfaces
 
   void AssignIpAddresses (NetDeviceContainer & d,
                           Ipv4InterfaceContainer & adhocTxInterfaces);
  //Sets up routing messages on the nodes and their interfaces
 
   void SetupRoutingMessages (NodeContainer & c,
                              Ipv4InterfaceContainer & adhocTxInterfaces);
 
   Ptr<Socket> SetupRoutingPacketReceive (Ipv4Address addr, Ptr<Node> node);
 
   void ReceiveRoutingPacket (Ptr<Socket> socket);
 
   double m_TotalSimTime;      //This is in seconds  
   uint32_t m_protocol;       
   uint32_t m_port;           
   uint32_t m_nSinks;                 
   int m_routingTables;      
   RoutingStats routingStats; 
   std::string m_protocolName; 
   int m_log; 
 };
 
 NS_OBJECT_ENSURE_REGISTERED (RoutingHelper);
 
 TypeId
 RoutingHelper::GetTypeId (void)
 {
   static TypeId tid = TypeId ("ns3::RoutingHelper")
     .SetParent<Object> ()
     .AddConstructor<RoutingHelper> ();
   return tid;
 }
 
 RoutingHelper::RoutingHelper ()
   : m_TotalSimTime (300.01),
     m_protocol (0),
     m_port (9),
     m_nSinks (0),
     m_routingTables (0),
     m_log (0)
 {
 }
 
 RoutingHelper::~RoutingHelper ()
 {
 }
 
 void
 RoutingHelper::Install (NodeContainer & c,
                         NetDeviceContainer & d,
                         Ipv4InterfaceContainer & i,
                         double totalTime,
                         int protocol,
                         uint32_t nSinks,
                         int routingTables)
 {
   m_TotalSimTime = totalTime;
   m_protocol = protocol;
   m_nSinks = nSinks;
   m_routingTables = routingTables;
 
   SetupRoutingProtocol (c);
   AssignIpAddresses (d, i);
   SetupRoutingMessages (c, i);
 }
 
 Ptr<Socket>
 RoutingHelper::SetupRoutingPacketReceive (Ipv4Address addr, Ptr<Node> node)
 {
   TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
   Ptr<Socket> sink = Socket::CreateSocket (node, tid);
   InetSocketAddress local = InetSocketAddress (addr, m_port);
   sink->Bind (local);
   sink->SetRecvCallback (MakeCallback (&RoutingHelper::ReceiveRoutingPacket, this));
 
   return sink;
 }
 
 void
 RoutingHelper::SetupRoutingProtocol (NodeContainer & c)
 {
   AodvHelper aodv;
   OlsrHelper olsr;
   DsdvHelper dsdv;
   DsrHelper dsr;
   DsrMainHelper dsrMain;
   Ipv4ListRoutingHelper list;
   InternetStackHelper internet;
 
   Time rtt = Time (5.0);
   AsciiTraceHelper ascii;
   Ptr<OutputStreamWrapper> rtw = ascii.CreateFileStream ("routing_table");
 

 //The different cases are for the different routing protocols
   switch (m_protocol)
     {
     case 0:
       m_protocolName = "NONE";
       break;
     case 1:
       if (m_routingTables != 0)
         {
           olsr.PrintRoutingTableAllAt (rtt, rtw);
         }
       list.Add (olsr, 100);
       m_protocolName = "OLSR";
       break;
     case 2:
       if (m_routingTables != 0)
         {
           aodv.PrintRoutingTableAllAt (rtt, rtw);
         }
       list.Add (aodv, 100);
       m_protocolName = "AODV";
       break;
     case 3:
       if (m_routingTables != 0)
         {
           dsdv.PrintRoutingTableAllAt (rtt, rtw);
         }
       list.Add (dsdv, 100);
       m_protocolName = "DSDV";
       break;
     case 4:
       // setup is later
       m_protocolName = "DSR";
       break;
     default:
       NS_FATAL_ERROR ("No such protocol:" << m_protocol);
       break;
     }
 
   if (m_protocol < 4)
     {
       internet.SetRoutingHelper (list);
       internet.Install (c);
     }
   else if (m_protocol == 4)
     {
       internet.Install (c);
       dsrMain.Install (dsr, c);
     }
 
   if (m_log != 0)
     {
       NS_LOG_UNCOND ("Routing Setup for " << m_protocolName);
     }
 }
 
 void
 RoutingHelper::AssignIpAddresses (NetDeviceContainer & d,
                                   Ipv4InterfaceContainer & adhocTxInterfaces)
 {
   NS_LOG_INFO ("Assigning IP addresses");
   Ipv4AddressHelper addressAdhoc;
   // we may have a lot of nodes, and want them all
   // in same subnet, to support broadcast
   addressAdhoc.SetBase ("10.1.0.0", "255.255.0.0");
   adhocTxInterfaces = addressAdhoc.Assign (d);
 }
 
 void
 RoutingHelper::SetupRoutingMessages (NodeContainer & c,
                                      Ipv4InterfaceContainer & adhocTxInterfaces)
 {
   // Setup routing transmissions
   OnOffHelper onoff1 ("ns3::UdpSocketFactory",Address ());
   onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
   onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
 
   Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
   int64_t stream = 2;
   var->SetStream (stream);
   for (uint32_t i = 0; i < m_nSinks; i++)
     {
       // protocol == 0 means no routing data, WAVE BSM only
       // so do not set up sink
       if (m_protocol != 0)
         {
           Ptr<Socket> sink = SetupRoutingPacketReceive (adhocTxInterfaces.GetAddress (i), c.Get (i));
         }
 
       AddressValue remoteAddress (InetSocketAddress (adhocTxInterfaces.GetAddress (i), m_port));
       onoff1.SetAttribute ("Remote", remoteAddress);
 
       ApplicationContainer temp = onoff1.Install (c.Get (i + m_nSinks));
       temp.Start (Seconds (var->GetValue (1.0,2.0)));
       temp.Stop (Seconds (m_TotalSimTime)); 
     }
 }
 
 static inline std::string
 PrintReceivedRoutingPacket (Ptr<Socket> socket, Ptr<Packet> packet, Address srcAddress)
 {
   std::ostringstream oss;
 
   oss << Simulator::Now ().GetSeconds () << " " << socket->GetNode ()->GetId ();
 
   if (InetSocketAddress::IsMatchingType (srcAddress))
     {
       InetSocketAddress addr = InetSocketAddress::ConvertFrom (srcAddress);
       oss << " received one packet from " << addr.GetIpv4 ();
     }
   else
     {
       oss << " received one packet!";
     }
   return oss.str ();
 }
 
 void
 RoutingHelper::ReceiveRoutingPacket (Ptr<Socket> socket)
 {
   Ptr<Packet> packet;
   Address srcAddress;
   while ((packet = socket->RecvFrom (srcAddress)))
     {
       // application data, for goodput
       uint32_t RxRoutingBytes = packet->GetSize ();
       GetRoutingStats ().IncRxBytes (RxRoutingBytes);
       GetRoutingStats ().IncRxPkts ();
       if (m_log != 0)
         {
           NS_LOG_UNCOND (m_protocolName + " " + PrintReceivedRoutingPacket (socket, packet, srcAddress));
         }
     }
 }
 
 void
 RoutingHelper::OnOffTrace (std::string context, Ptr<const Packet> packet)
 {
   uint32_t pktBytes = packet->GetSize ();
   routingStats.IncTxBytes (pktBytes);
 }
 
 RoutingStats &
 RoutingHelper::GetRoutingStats ()
 {
   return routingStats;
 }
 
 void
 RoutingHelper::SetLogging (int log)
 {
   m_log = log;
 }
 
 class WifiPhyStats : public Object //WIFI class
 {
 public:
   static TypeId GetTypeId (void);
 
   WifiPhyStats ();
 
   virtual ~WifiPhyStats ();
 
   uint32_t GetTxBytes ();
 
   void PhyTxTrace (std::string context, Ptr<const Packet> packet, WifiMode mode, WifiPreamble preamble, uint8_t txPower);
 
   void PhyTxDrop (std::string context, Ptr<const Packet> packet);
 
   void PhyRxDrop (std::string context, Ptr<const Packet> packet);
 
 private:
   uint32_t m_phyTxPkts; //Transmit packets
   uint32_t m_phyTxBytes; //Transmit bytes
 };
 
 NS_OBJECT_ENSURE_REGISTERED (WifiPhyStats);
 
 TypeId
 WifiPhyStats::GetTypeId (void)
 {
   static TypeId tid = TypeId ("ns3::WifiPhyStats")
     .SetParent<Object> ()
     .AddConstructor<WifiPhyStats> ();
   return tid;
 }
 
 WifiPhyStats::WifiPhyStats ()
   : m_phyTxPkts (0),
     m_phyTxBytes (0)
 {
 }
 
 WifiPhyStats::~WifiPhyStats ()
 {
 }
 
 void
 WifiPhyStats::PhyTxTrace (std::string context, Ptr<const Packet> packet, WifiMode mode, WifiPreamble preamble, uint8_t txPower)
 {
   NS_LOG_FUNCTION (this << context << packet << "PHYTX mode=" << mode );
   ++m_phyTxPkts;
   uint32_t pktSize = packet->GetSize ();
   m_phyTxBytes += pktSize;
 
   //NS_LOG_UNCOND ("Received PHY size=" << pktSize);
 }
 
 void
 WifiPhyStats::PhyTxDrop (std::string context, Ptr<const Packet> packet)
 {
   NS_LOG_UNCOND ("PHY Tx Drop");
 }
 
 void
 WifiPhyStats::PhyRxDrop (std::string context, Ptr<const Packet> packet)
 {
   NS_LOG_UNCOND ("PHY Rx Drop");
 }
 
 uint32_t
 WifiPhyStats::GetTxBytes ()
 {
   return m_phyTxBytes;
 }
 
 class WifiApp                  //The WifiApp class causes a program flow for the ns3 wifi applications
 {
 public:
   WifiApp ();
 
   virtual ~WifiApp ();
 
   void Simulate (int argc, char **argv);
 
 protected:
   virtual void SetDefaultAttributeValues ();
 
   virtual void ParseCommandLineArguments (int argc, char **argv);
 
   virtual void ConfigureNodes ();
 
   virtual void ConfigureChannels ();
 
   virtual void ConfigureDevices ();
 
   virtual void ConfigureMobility ();
 
   virtual void ConfigureApplications ();
 
   virtual void ConfigureTracing ();
 
   virtual void RunSimulation ();
 
   virtual void ProcessOutputs ();
 };
 
 WifiApp::WifiApp ()
 {
 }
 
 WifiApp::~WifiApp ()
 {
 }
 
 void
 WifiApp::Simulate (int argc, char **argv)
 {
 
   SetDefaultAttributeValues ();
   ParseCommandLineArguments (argc, argv);
   ConfigureNodes ();
   ConfigureChannels ();
   ConfigureDevices ();
   ConfigureMobility ();
   ConfigureApplications ();
   ConfigureTracing ();
   RunSimulation ();
   ProcessOutputs ();
 }
 
 void
 WifiApp::SetDefaultAttributeValues ()
 {
 }
 
 void
 WifiApp::ParseCommandLineArguments (int argc, char **argv)
 {
 }
 
 void
 WifiApp::ConfigureNodes ()
 {
 }
 
 void
 WifiApp::ConfigureChannels ()
 {
 }
 
 void
 WifiApp::ConfigureDevices ()
 {
 }
 
 void
 WifiApp::ConfigureMobility ()
 {
 }
 
 void
 WifiApp::ConfigureApplications ()
 {
 }
 
 void
 WifiApp::ConfigureTracing ()
 {
 }
 
 void
 WifiApp::RunSimulation ()
 {
 }
 
 void
 WifiApp::ProcessOutputs ()
 {
 }
 
 class ConfigStoreHelper                       //The ConFigStoreHelper class simplifies config-store raw text
 {
 public:
   ConfigStoreHelper ();
 
   void LoadConfig (std::string configFilename);
 
   void SaveConfig (std::string configFilename);
 };
 
 ConfigStoreHelper::ConfigStoreHelper ()
 {
 }
 
 void
 ConfigStoreHelper::LoadConfig (std::string configFilename)
 {
   // Input config store from txt format
   Config::SetDefault ("ns3::ConfigStore::Filename", StringValue (configFilename));
   Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
   Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Load"));
   ConfigStore inputConfig;
   inputConfig.ConfigureDefaults ();
   //inputConfig.ConfigureAttributes ();
 }
 
 void
 ConfigStoreHelper::SaveConfig (std::string configFilename)
 {
   // only save if a non-empty filename has been specified
   if (configFilename.compare ("") != 0)
     {
       // Output config store to txt format
       Config::SetDefault ("ns3::ConfigStore::Filename", StringValue (configFilename));
       Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
       Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Save"));
       ConfigStore outputConfig;
       outputConfig.ConfigureDefaults ();
       //outputConfig.ConfigureAttributes ();
     }
 }
 
 class VanetRoutingExperiment : public WifiApp          //The VanetRoutingExperiment implements the WifiApp
 {
 public:
   VanetRoutingExperiment ();
 
 protected:

// These functions are virtual as they have been implemented originally in the WifiApp class
   virtual void SetDefaultAttributeValues ();
 
   virtual void ParseCommandLineArguments (int argc, char **argv);
 
   virtual void ConfigureNodes ();
 
   virtual void ConfigureChannels ();
 
   virtual void ConfigureDevices ();
 
   virtual void ConfigureMobility ();
 
   virtual void ConfigureApplications ();
 
   virtual void ConfigureTracing ();
 
   virtual void RunSimulation ();
 
   virtual void ProcessOutputs ();
 
 private:
   void Run ();
 
   void CommandSetup (int argc, char **argv);
 
   void CheckThroughput ();
 
   void SetupLogFile ();
 
   void SetupLogging ();
 
   void ConfigureDefaults ();
 
   void SetupAdhocMobilityNodes ();
 
   void SetupAdhocDevices ();
 //This is the setup of the IEEE 1609 Wave messages as a basic safety message. This messages are typically
 //200 byte packets broadcasted by all vehicles at a rate of 10Hz
 
   void SetupWaveMessages ();
 
   void SetupRoutingMessages ();
 
   void SetupScenario ();
 
   void WriteCsvHeader ();
 
   void SetConfigFromGlobals ();
 
   void SetGlobalsFromConfig ();
 
   static void
   CourseChange (std::ostream *os, std::string context, Ptr<const MobilityModel> mobility);
 
   uint32_t m_port; 
   std::string m_CSVfileName; 
   std::string m_CSVfileName2; 
   uint32_t m_nSinks; 
   std::string m_protocolName; 
   double m_txp; 
   bool m_traceMobility; 
   uint32_t m_protocol; 
 
   uint32_t m_lossModel; 
   uint32_t m_fading; 
   std::string m_lossModelName; 
 
   std::string m_phyMode; 
   uint32_t m_80211mode; 
 
   std::string m_traceFile; 
   std::string m_logFile; 
   uint32_t m_mobility; 
   uint32_t m_nNodes; 
   double m_TotalSimTime; 
   std::string m_rate; 
   std::string m_phyModeB; 
   std::string m_trName; 
   int m_nodeSpeed; 
   int m_nodePause; 
   uint32_t m_wavePacketSize; 
   double m_waveInterval; 
   int m_verbose; 
   std::ofstream m_os; 
   NetDeviceContainer m_adhocTxDevices; 
   Ipv4InterfaceContainer m_adhocTxInterfaces; 
   uint32_t m_scenario; 
   double m_gpsAccuracyNs; 
   double m_txMaxDelayMs; 
   int m_routingTables; 
   int m_asciiTrace; 
   int m_pcap; 
   std::string m_loadConfigFilename; 
   std::string m_saveConfigFilename; 
 
   WaveBsmHelper m_waveBsmHelper; 
   Ptr<RoutingHelper> m_routingHelper; 
   Ptr<WifiPhyStats> m_wifiPhyStats; 
   int m_log; 
   int64_t m_streamIndex;
   NodeContainer m_adhocTxNodes; 
   
   //Different ranges of safety messages 
   double m_txSafetyRange1;  //50m
   double m_txSafetyRange2;  //100m
   double m_txSafetyRange3;  //150m
   double m_txSafetyRange4;  //200m
   double m_txSafetyRange5;  //250m
   double m_txSafetyRange6;  //300m
   double m_txSafetyRange7;  //400m
   double m_txSafetyRange8;  //450m
   double m_txSafetyRange9;  //500m
   double m_txSafetyRange10; //550m
   std::vector <double> m_txSafetyRanges;       //List of ranges
   std::string m_exp; 
   int m_cumulativeBsmCaptureStart; 
 };
 
 VanetRoutingExperiment::VanetRoutingExperiment ()
   : m_port (9),
     m_CSVfileName ("vanet-routing.output.csv"),
     m_CSVfileName2 ("vanet-routing.output2.csv"),
     m_nSinks (10),
     m_protocolName ("protocol"),
     m_txp (20),
     m_traceMobility (false),
     // AODV
     m_protocol (2),
     // Two-Ray ground
     m_lossModel (3),
     m_fading (0),
     m_lossModelName (""),
     m_phyMode ("OfdmRate6MbpsBW10MHz"),
     // 1=802.11p
     m_80211mode (1),
     m_traceFile (""),
     m_logFile ("low99-ct-unterstrass-1day.filt.7.adj.log"),
     m_mobility (1),
     m_nNodes (156),
     m_TotalSimTime (150.01),
     m_rate ("2048bps"),
     m_phyModeB ("DsssRate11Mbps"),
     m_trName ("vanet-routing-compare"),
     m_nodeSpeed (20),
     m_nodePause (0),
     m_wavePacketSize (200),
     m_waveInterval (1),
     m_verbose (0),
     m_scenario (1),
     m_gpsAccuracyNs (40),
     m_txMaxDelayMs (1000),
     m_routingTables (0),
     m_asciiTrace (0),
     m_pcap (0),
     m_loadConfigFilename ("load-config.txt"),
     m_saveConfigFilename (""),
     m_log (0),
     m_streamIndex (0),
     m_adhocTxNodes (),
     m_txSafetyRange1 (50.0),
     m_txSafetyRange2 (100.0),
     m_txSafetyRange3 (150.0),
     m_txSafetyRange4 (200.0),
     m_txSafetyRange5 (250.0),
     m_txSafetyRange6 (300.0),
     m_txSafetyRange7 (350.0),
     m_txSafetyRange8 (400.0),
     m_txSafetyRange9 (450.0),
     m_txSafetyRange10 (500.0),
     m_txSafetyRanges (),
     m_exp (""),
     m_cumulativeBsmCaptureStart (0)
 {
   m_wifiPhyStats = CreateObject<WifiPhyStats> ();
   m_routingHelper = CreateObject<RoutingHelper> ();
 
   // set to non-zero value to enable
   // simply uncond logging during simulation run
   m_log = 1;
 }
 
 void
 VanetRoutingExperiment::SetDefaultAttributeValues ()
 {
   // handled in constructor
 }
 
 // important configuration items stored in global values
 static ns3::GlobalValue g_port ("VRCport",
                                 "Port",
                                 ns3::UintegerValue (9),
                                 ns3::MakeUintegerChecker<uint32_t> ());
 static ns3::GlobalValue g_nSinks ("VRCnSinks",
                                   "Number of sink nodes for routing non-BSM traffic",
                                   ns3::UintegerValue (10),
                                   ns3::MakeUintegerChecker<uint32_t> ());
 static ns3::GlobalValue g_traceMobility ("VRCtraceMobility",
                                          "Trace mobility 1=yes;0=no",
                                          ns3::UintegerValue (0),
                                          ns3::MakeUintegerChecker<uint32_t> ());
 static ns3::GlobalValue g_protocol ("VRCprotocol",
                                     "Routing protocol",
                                     ns3::UintegerValue (2),
                                     ns3::MakeUintegerChecker<uint32_t> ());
 static ns3::GlobalValue g_lossModel ("VRClossModel",
                                      "Propagation Loss Model",
                                      ns3::UintegerValue (3),
                                      ns3::MakeUintegerChecker<uint32_t> ());
 static ns3::GlobalValue g_fading ("VRCfading",
                                   "Fast Fading Model",
                                   ns3::UintegerValue (0),
                                   ns3::MakeUintegerChecker<uint32_t> ());
 static ns3::GlobalValue g_80211mode ("VRC80211mode",
                                      "802.11 mode (0=802.11a;1=802.11p)",
                                      ns3::UintegerValue (1),
                                      ns3::MakeUintegerChecker<uint32_t> ());
 static ns3::GlobalValue g_mobility ("VRCmobility",
                                     "Mobility mode 0=random waypoint;1=mobility trace file",
                                     ns3::UintegerValue (1),
                                     ns3::MakeUintegerChecker<uint32_t> ());
 static ns3::GlobalValue g_nNodes ("VRCnNodes",
                                   "Number of nodes (vehicles)",
                                   ns3::UintegerValue (149),
                                   ns3::MakeUintegerChecker<uint32_t> ());
 static ns3::GlobalValue g_nodeSpeed ("VRCnodeSpeed",
                                      "Node speed (m/s) for RWP model",
                                      ns3::UintegerValue (20),
                                      ns3::MakeUintegerChecker<uint32_t> ());
 static ns3::GlobalValue g_nodePause ("VRCnodePause",
                                      "Node pause time (s) for RWP model",
                                      ns3::UintegerValue (0),
                                      ns3::MakeUintegerChecker<uint32_t> ());
 static ns3::GlobalValue g_wavePacketSize ("VRCwavePacketSize",
                                           "Size in bytes of WAVE BSM",
                                           ns3::UintegerValue (200),
                                           ns3::MakeUintegerChecker<uint32_t> ());
 static ns3::GlobalValue g_verbose ("VRCverbose",
                                    "Verbose 0=no;1=yes",
                                    ns3::UintegerValue (0),
                                    ns3::MakeUintegerChecker<uint32_t> ());
 static ns3::GlobalValue g_scenario ("VRCscenario",
                                     "Scenario",
                                     ns3::UintegerValue (1),
                                     ns3::MakeUintegerChecker<uint32_t> ());
 static ns3::GlobalValue g_routingTables ("VRCroutingTables",
                                          "Dump routing tables at t=5 seconds 0=no;1=yes",
                                          ns3::UintegerValue (1),
                                          ns3::MakeUintegerChecker<uint32_t> ());
 static ns3::GlobalValue g_asciiTrace ("VRCasciiTrace",
                                       "Dump ASCII trace 0=no;1=yes",
                                       ns3::UintegerValue (1),
                                       ns3::MakeUintegerChecker<uint32_t> ());
 static ns3::GlobalValue g_pcap ("VRCpcap",
                                 "Generate PCAP files 0=no;1=yes",
                                 ns3::UintegerValue (0),
                                 ns3::MakeUintegerChecker<uint32_t> ());
 static ns3::GlobalValue g_cumulativeBsmCaptureStart ("VRCcumulativeBsmCaptureStart",
                                                      "Simulation starte time for capturing cumulative BSM",
                                                      ns3::UintegerValue (0),
                                                      ns3::MakeUintegerChecker<uint32_t> ());
 
 static ns3::GlobalValue g_txSafetyRange1 ("VRCtxSafetyRange1",
                                           "BSM range for PDR inclusion",
                                           ns3::DoubleValue (50.0),
                                           ns3::MakeDoubleChecker<double> ());
 
 static ns3::GlobalValue g_txSafetyRange2 ("VRCtxSafetyRange2",
                                           "BSM range for PDR inclusion",
                                           ns3::DoubleValue (100.0),
                                           ns3::MakeDoubleChecker<double> ());
 
 static ns3::GlobalValue g_txSafetyRange3 ("VRCtxSafetyRange3",
                                           "BSM range for PDR inclusion",
                                           ns3::DoubleValue (150.0),
                                           ns3::MakeDoubleChecker<double> ());
 
 static ns3::GlobalValue g_txSafetyRange4 ("VRCtxSafetyRange4",
                                           "BSM range for PDR inclusion",
                                           ns3::DoubleValue (200.0),
                                           ns3::MakeDoubleChecker<double> ());
 
 static ns3::GlobalValue g_txSafetyRange5 ("VRCtxSafetyRange5",
                                           "BSM range for PDR inclusion",
                                           ns3::DoubleValue (250.0),
                                           ns3::MakeDoubleChecker<double> ());
 static ns3::GlobalValue g_txSafetyRange6 ("VRCtxSafetyRange6",
                                           "BSM range for PDR inclusion",
                                           ns3::DoubleValue (300.0),
                                           ns3::MakeDoubleChecker<double> ());
 static ns3::GlobalValue g_txSafetyRange7 ("VRCtxSafetyRange7",
                                           "BSM range for PDR inclusion",
                                           ns3::DoubleValue (350.0),
                                           ns3::MakeDoubleChecker<double> ());
 static ns3::GlobalValue g_txSafetyRange8 ("VRCtxSafetyRange8",
                                           "BSM range for PDR inclusion",
                                           ns3::DoubleValue (400.0),
                                           ns3::MakeDoubleChecker<double> ());
 static ns3::GlobalValue g_txSafetyRange9 ("VRCtxSafetyRange9",
                                           "BSM range for PDR inclusion",
                                           ns3::DoubleValue (450.0),
                                           ns3::MakeDoubleChecker<double> ());
 static ns3::GlobalValue g_txSafetyRange10 ("VRCtxSafetyRange10",
                                            "BSM range for PDR inclusion",
                                            ns3::DoubleValue (500.0),
                                            ns3::MakeDoubleChecker<double> ());
 static ns3::GlobalValue g_txp ("VRCtxp",
                                "Transmission power dBm",
                                ns3::DoubleValue (15.0),
                                ns3::MakeDoubleChecker<double> ());
 static ns3::GlobalValue g_totalTime ("VRCtotalTime",
                                      "Total simulation time (s)",
                                      ns3::DoubleValue (150.01),
                                      ns3::MakeDoubleChecker<double> ());
 static ns3::GlobalValue g_waveInterval ("VRCwaveInterval",
                                         "Interval (s) between WAVE BSMs",
                                         ns3::DoubleValue (1.0),
                                         ns3::MakeDoubleChecker<double> ());
 static ns3::GlobalValue g_gpsAccuracyNs ("VRCgpsAccuracyNs",
                                          "GPS sync accuracy (ns)",
                                          ns3::DoubleValue (40),
                                          ns3::MakeDoubleChecker<double> ());
 static ns3::GlobalValue g_txMaxDelayMs ("VRCtxMaxDelayMs",
                                         "Tx May Delay (ms)",
                                         ns3::DoubleValue (1000.0),
                                         ns3::MakeDoubleChecker<double> ());
 static ns3::GlobalValue g_CSVfileName ("VRCCSVfileName",
                                        "CSV filename (for time series data)",
                                        ns3::StringValue ("vanet-routing.output.csv"),
                                        ns3::MakeStringChecker ());
 static ns3::GlobalValue g_CSVfileName2 ("VRCCSVfileName2",
                                         "CSV filename 2 (for overall simulation scenario results)",
                                         ns3::StringValue ("vanet-routing.output2.csv"),
                                         ns3::MakeStringChecker ());
 static ns3::GlobalValue g_phyMode ("VRCphyMode",
                                    "PHY mode (802.11p)",
                                    ns3::StringValue ("OfdmRate6MbpsBW10MHz"),
                                    ns3::MakeStringChecker ());
 static ns3::GlobalValue g_traceFile ("VRCtraceFile",
                                      "Mobility trace filename",
                                      ns3::StringValue ("./src/wave/examples/low99-ct-unterstrass-1day.filt.7.adj.mob"),
                                      ns3::MakeStringChecker ());
 static ns3::GlobalValue g_logFile ("VRClogFile",
                                    "Log filename",
                                    ns3::StringValue ("low99-ct-unterstrass-1day.filt.7.adj.log"),
                                    ns3::MakeStringChecker ());
 static ns3::GlobalValue g_rate ("VRCrate",
                                 "Data rate",
                                 ns3::StringValue ("2048bps"),
                                 ns3::MakeStringChecker ());
 static ns3::GlobalValue g_phyModeB ("VRCphyModeB",
                                     "PHY mode (802.11a)",
                                     ns3::StringValue ("DsssRate11Mbps"),
                                     ns3::MakeStringChecker ());
 static ns3::GlobalValue g_trName ("VRCtrName",
                                   "Trace name",
                                   ns3::StringValue ("vanet-routing-compare"),
                                   ns3::MakeStringChecker ());
 
 void
 VanetRoutingExperiment::ParseCommandLineArguments (int argc, char **argv)
 {
   CommandSetup (argc, argv);
   SetupScenario ();
 
   // user may specify up to 10 different tx distances
   // to be used for calculating different values of Packet
   // Delivery Ratio (PDR). Used to see the effects of
   // fading over distance
   m_txSafetyRanges.resize (10, 0);
   m_txSafetyRanges[0] = m_txSafetyRange1;
   m_txSafetyRanges[1] = m_txSafetyRange2;
   m_txSafetyRanges[2] = m_txSafetyRange3;
   m_txSafetyRanges[3] = m_txSafetyRange4;
   m_txSafetyRanges[4] = m_txSafetyRange5;
   m_txSafetyRanges[5] = m_txSafetyRange6;
   m_txSafetyRanges[6] = m_txSafetyRange7;
   m_txSafetyRanges[7] = m_txSafetyRange8;
   m_txSafetyRanges[8] = m_txSafetyRange9;
   m_txSafetyRanges[9] = m_txSafetyRange10;
 
   ConfigureDefaults ();
 
   // we are done with all configuration
   // save config-store, if requested
   SetGlobalsFromConfig ();
   ConfigStoreHelper configStoreHelper;
   configStoreHelper.SaveConfig (m_saveConfigFilename);
 
   m_waveBsmHelper.GetWaveBsmStats ()->SetLogging (m_log);
   m_routingHelper->SetLogging (m_log);
 }
 
 void
 VanetRoutingExperiment::ConfigureNodes ()
 {
   m_adhocTxNodes.Create (m_nNodes);
 }
 
 void
 VanetRoutingExperiment::ConfigureChannels ()
 {
   // set up channel and devices
   SetupAdhocDevices ();
 }
 
 void
 VanetRoutingExperiment::ConfigureDevices ()
 {
   // devices are set up in SetupAdhocDevices(),
   // called by ConfigureChannels()
 
   // every device will have PHY callback for tracing
   // which is used to determine the total amount of
   // data transmitted, and then used to calculate
   // the MAC/PHY overhead beyond the app-data
   Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/Tx", MakeCallback (&WifiPhyStats::PhyTxTrace, m_wifiPhyStats));
   // TxDrop, RxDrop not working yet.  Not sure what I'm doing wrong.
   Config::Connect ("/NodeList/*/DeviceList/*/ns3::WifiNetDevice/Phy/PhyTxDrop", MakeCallback (&WifiPhyStats::PhyTxDrop, m_wifiPhyStats));
   Config::Connect ("/NodeList/*/DeviceList/*/ns3::WifiNetDevice/Phy/PhyRxDrop", MakeCallback (&WifiPhyStats::PhyRxDrop, m_wifiPhyStats));
 }
 
 void
 VanetRoutingExperiment::ConfigureMobility ()
 {
   SetupAdhocMobilityNodes ();
 }
 
 void
 VanetRoutingExperiment::ConfigureApplications ()
 {
   // Traffic mix consists of:
   // 1. routing data
   // 2. Broadcasting of Basic Safety Message (BSM)
   SetupRoutingMessages ();
   SetupWaveMessages ();
 
   // config trace to capture app-data (bytes) for
   // routing data, subtracted and used for
   // routing overhead
   std::ostringstream oss;
   oss.str ("");
   oss << "/NodeList/*/ApplicationList/*/$ns3::OnOffApplication/Tx";
   Config::Connect (oss.str (), MakeCallback (&RoutingHelper::OnOffTrace, m_routingHelper));
 }
 
 void
 VanetRoutingExperiment::ConfigureTracing ()
 {
   WriteCsvHeader ();
   SetupLogFile ();
   SetupLogging ();
 
   AsciiTraceHelper ascii;
   MobilityHelper::EnableAsciiAll (ascii.CreateFileStream (m_trName + ".mob"));
 }
 
 void
 VanetRoutingExperiment::RunSimulation ()
 {
   Run ();
 }
 
 void
 VanetRoutingExperiment::ProcessOutputs ()
 {
   // calculate and output final results
   double bsm_pdr1 = m_waveBsmHelper.GetWaveBsmStats ()->GetCumulativeBsmPdr (1);
   double bsm_pdr2 = m_waveBsmHelper.GetWaveBsmStats ()->GetCumulativeBsmPdr (2);
   double bsm_pdr3 = m_waveBsmHelper.GetWaveBsmStats ()->GetCumulativeBsmPdr (3);
   double bsm_pdr4 = m_waveBsmHelper.GetWaveBsmStats ()->GetCumulativeBsmPdr (4);
   double bsm_pdr5 = m_waveBsmHelper.GetWaveBsmStats ()->GetCumulativeBsmPdr (5);
   double bsm_pdr6 = m_waveBsmHelper.GetWaveBsmStats ()->GetCumulativeBsmPdr (6);
   double bsm_pdr7 = m_waveBsmHelper.GetWaveBsmStats ()->GetCumulativeBsmPdr (7);
   double bsm_pdr8 = m_waveBsmHelper.GetWaveBsmStats ()->GetCumulativeBsmPdr (8);
   double bsm_pdr9 = m_waveBsmHelper.GetWaveBsmStats ()->GetCumulativeBsmPdr (9);
   double bsm_pdr10 = m_waveBsmHelper.GetWaveBsmStats ()->GetCumulativeBsmPdr (10);
 
   double averageRoutingGoodputKbps = 0.0;
   uint32_t totalBytesTotal = m_routingHelper->GetRoutingStats ().GetCumulativeRxBytes ();
   averageRoutingGoodputKbps = (((double) totalBytesTotal * 8.0) / m_TotalSimTime) / 1000.0;
 
   // calculate MAC/PHY overhead (mac-phy-oh)
   // total WAVE BSM bytes sent
   uint32_t cumulativeWaveBsmBytes = m_waveBsmHelper.GetWaveBsmStats ()->GetTxByteCount ();
   uint32_t cumulativeRoutingBytes = m_routingHelper->GetRoutingStats ().GetCumulativeTxBytes ();
   uint32_t totalAppBytes = cumulativeWaveBsmBytes + cumulativeRoutingBytes;
   uint32_t totalPhyBytes = m_wifiPhyStats->GetTxBytes ();
   // mac-phy-oh = (total-phy-bytes - total-app-bytes) / total-phy-bytes
   double mac_phy_oh = 0.0;
   if (totalPhyBytes > 0)
     {
       mac_phy_oh = (double) (totalPhyBytes - totalAppBytes) / (double) totalPhyBytes;
     }
 
   if (m_log != 0)            //BSM_PDR1, BSM_PDR2 = Different safety messages which will be sent at the different ranges of 50m,100m etc
     {
       NS_LOG_UNCOND ("BSM_PDR1=" << bsm_pdr1 << " BSM_PDR2=" << bsm_pdr2 << " BSM_PDR3=" << bsm_pdr3 << " BSM_PDR4=" << bsm_pdr4 << " BSM_PDR5=" << bsm_pdr5 << " BSM_PDR6=" << bsm_pdr6 << " BSM_PDR7=" << bsm_pdr7 << " BSM_PDR8=" << bsm_pdr8 << " BSM_PDR9=" << bsm_pdr9 << " BSM_PDR10=" << bsm_pdr10 << " Goodput=" << averageRoutingGoodputKbps << "Kbps MAC/PHY-oh=" << mac_phy_oh);
 
     }
 
   std::ofstream out (m_CSVfileName2.c_str (), std::ios::app);
 
   out << bsm_pdr1 << ","
       << bsm_pdr2 << ","
       << bsm_pdr3 << ","
       << bsm_pdr4 << ","
       << bsm_pdr5 << ","
       << bsm_pdr6 << ","
       << bsm_pdr7 << ","
       << bsm_pdr8 << ","
       << bsm_pdr9 << ","
       << bsm_pdr10 << ","
       << averageRoutingGoodputKbps << ","
       << mac_phy_oh << ""
       << std::endl;
 
   out.close ();
 
   m_os.close (); // close log file
 }
 
 void
 VanetRoutingExperiment::Run ()
 {
   NS_LOG_INFO ("Run Simulation.");
 
   CheckThroughput ();

 
   Simulator::Stop (Seconds (m_TotalSimTime));
   
   //Uncomment the required line for the routing protocol being tested. 
 
   //AnimationInterface anim ("M50AnimationOLSR.xml");
   //AnimationInterface anim ("M50AnimationAODV.xml");
   //AnimationInterface anim ("M50AnimationDSDV.xml");
   //AnimationInterface anim ("M50AnimationDSR.xml");


   Simulator::Run ();


   Simulator::Destroy ();
 }
 
 // Prints actual position and velocity when a course change event occurs
 void
 VanetRoutingExperiment::
 CourseChange (std::ostream *os, std::string context, Ptr<const MobilityModel> mobility)
 {
   Vector pos = mobility->GetPosition (); // Get position
   Vector vel = mobility->GetVelocity (); // Get velocity
 
   pos.z = 1.5; //Antenna is 1.5 meters from the ground
 
   int nodeId = mobility->GetObject<Node> ()->GetId ();
   double t = (Simulator::Now ()).GetSeconds ();
   if (t >= 1.0)
     {
       WaveBsmHelper::GetNodesMoving ()[nodeId] = 1;
     }
 
   //NS_LOG_UNCOND ("Changing pos for node=" << nodeId << " at " << Simulator::Now () );
 
   // Prints position and velocities
   *os << Simulator::Now () << " POS: x=" << pos.x << ", y=" << pos.y
       << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
       << ", z=" << vel.z << std::endl;
 }
 
 void
 VanetRoutingExperiment::CheckThroughput ()
 {
   uint32_t bytesTotal = m_routingHelper->GetRoutingStats ().GetRxBytes ();
   uint32_t packetsReceived = m_routingHelper->GetRoutingStats ().GetRxPkts ();
   double kbps = (bytesTotal * 8.0) / 1000;
   double wavePDR = 0.0;
   int wavePktsSent = m_waveBsmHelper.GetWaveBsmStats ()->GetTxPktCount ();
   int wavePktsReceived = m_waveBsmHelper.GetWaveBsmStats ()->GetRxPktCount ();
   if (wavePktsSent > 0)
     {
       int wavePktsReceived = m_waveBsmHelper.GetWaveBsmStats ()->GetRxPktCount ();
       wavePDR = (double) wavePktsReceived / (double) wavePktsSent;
     }
 
   int waveExpectedRxPktCount = m_waveBsmHelper.GetWaveBsmStats ()->GetExpectedRxPktCount (1);
   int waveRxPktInRangeCount = m_waveBsmHelper.GetWaveBsmStats ()->GetRxPktInRangeCount (1);
   double wavePDR1_2 = m_waveBsmHelper.GetWaveBsmStats ()->GetBsmPdr (1);
   double wavePDR2_2 = m_waveBsmHelper.GetWaveBsmStats ()->GetBsmPdr (2);
   double wavePDR3_2 = m_waveBsmHelper.GetWaveBsmStats ()->GetBsmPdr (3);
   double wavePDR4_2 = m_waveBsmHelper.GetWaveBsmStats ()->GetBsmPdr (4);
   double wavePDR5_2 = m_waveBsmHelper.GetWaveBsmStats ()->GetBsmPdr (5);
   double wavePDR6_2 = m_waveBsmHelper.GetWaveBsmStats ()->GetBsmPdr (6);
   double wavePDR7_2 = m_waveBsmHelper.GetWaveBsmStats ()->GetBsmPdr (7);
   double wavePDR8_2 = m_waveBsmHelper.GetWaveBsmStats ()->GetBsmPdr (8);
   double wavePDR9_2 = m_waveBsmHelper.GetWaveBsmStats ()->GetBsmPdr (9);
   double wavePDR10_2 = m_waveBsmHelper.GetWaveBsmStats ()->GetBsmPdr (10);
 
   // calculate MAC/PHY overhead (mac-phy-oh)
   // total WAVE BSM bytes sent
   uint32_t cumulativeWaveBsmBytes = m_waveBsmHelper.GetWaveBsmStats ()->GetTxByteCount ();
   uint32_t cumulativeRoutingBytes = m_routingHelper->GetRoutingStats ().GetCumulativeTxBytes ();
   uint32_t totalAppBytes = cumulativeWaveBsmBytes + cumulativeRoutingBytes;
   uint32_t totalPhyBytes = m_wifiPhyStats->GetTxBytes ();
   // mac-phy-oh = (total-phy-bytes - total-app-bytes) / total-phy-bytes
   double mac_phy_oh = 0.0;
   if (totalPhyBytes > 0)
     {
       mac_phy_oh = (double) (totalPhyBytes - totalAppBytes) / (double) totalPhyBytes;
     }
 
   std::ofstream out (m_CSVfileName.c_str (), std::ios::app);
 
   if (m_log != 0 )
     {
       NS_LOG_UNCOND ("At t=" << (Simulator::Now ()).GetSeconds () << "s BSM_PDR1=" << wavePDR1_2 << " BSM_PDR1=" << wavePDR2_2 << " BSM_PDR3=" << wavePDR3_2 << " BSM_PDR4=" << wavePDR4_2 << " BSM_PDR5=" << wavePDR5_2 << " BSM_PDR6=" << wavePDR6_2 << " BSM_PDR7=" << wavePDR7_2 << " BSM_PDR8=" << wavePDR8_2 << " BSM_PDR9=" << wavePDR9_2 << " BSM_PDR10=" << wavePDR10_2 << " Goodput=" << kbps << "Kbps" /*<< " MAC/PHY-OH=" << mac_phy_oh*/);
     }
 
   out << (Simulator::Now ()).GetSeconds () << ","
       << kbps << ","
       << packetsReceived << ","
       << m_nSinks << ","
       << m_protocolName << ","
       << m_txp << ","
       << wavePktsSent << ","
       << wavePktsReceived << ","
       << wavePDR << ","
       << waveExpectedRxPktCount << ","
       << waveRxPktInRangeCount << ","
       << wavePDR1_2 << ","
       << wavePDR2_2 << ","
       << wavePDR3_2 << ","
       << wavePDR4_2 << ","
       << wavePDR5_2 << ","
       << wavePDR6_2 << ","
       << wavePDR7_2 << ","
       << wavePDR8_2 << ","
       << wavePDR9_2 << ","
       << wavePDR10_2 << ","
       << mac_phy_oh << ""
       << std::endl;
 
   out.close ();
 
   m_routingHelper->GetRoutingStats ().SetRxBytes (0);
   m_routingHelper->GetRoutingStats ().SetRxPkts (0);
   m_waveBsmHelper.GetWaveBsmStats ()->SetRxPktCount (0);
   m_waveBsmHelper.GetWaveBsmStats ()->SetTxPktCount (0);
   for (int index = 1; index <= 10; index++)
     {
       m_waveBsmHelper.GetWaveBsmStats ()->SetExpectedRxPktCount (index, 0);
       m_waveBsmHelper.GetWaveBsmStats ()->SetRxPktInRangeCount (index, 0);
     }
 
   double currentTime = (Simulator::Now ()).GetSeconds ();
   if (currentTime <= (double) m_cumulativeBsmCaptureStart)
     {
       for (int index = 1; index <= 10; index++)
         {
           m_waveBsmHelper.GetWaveBsmStats ()->ResetTotalRxPktCounts (index);
         }
     }
 
   Simulator::Schedule (Seconds (1.0), &VanetRoutingExperiment::CheckThroughput, this);
 }
 
 void
 VanetRoutingExperiment::SetConfigFromGlobals ()
 {
   // get settings saved from config-store
   UintegerValue uintegerValue;
   DoubleValue doubleValue;
   StringValue stringValue;
 
   // This may not be the best way to manage program configuration
   // (directing them through global values), but management
   // through the config-store here is copied from
   // src/lte/examples/lena-dual-stripe.cc
 
   GlobalValue::GetValueByName ("VRCport", uintegerValue);
   m_port = uintegerValue.Get ();
   GlobalValue::GetValueByName ("VRCnSinks", uintegerValue);
   m_nSinks = uintegerValue.Get ();
   GlobalValue::GetValueByName ("VRCtraceMobility", uintegerValue);
   m_traceMobility = uintegerValue.Get ();
   GlobalValue::GetValueByName ("VRCprotocol", uintegerValue);
   m_protocol = uintegerValue.Get ();
   GlobalValue::GetValueByName ("VRClossModel", uintegerValue);
   m_lossModel = uintegerValue.Get ();
   GlobalValue::GetValueByName ("VRCfading", uintegerValue);
   m_fading = uintegerValue.Get ();
   GlobalValue::GetValueByName ("VRC80211mode", uintegerValue);
   m_80211mode = uintegerValue.Get ();
   GlobalValue::GetValueByName ("VRCmobility", uintegerValue);
   m_mobility = uintegerValue.Get ();
   GlobalValue::GetValueByName ("VRCnNodes", uintegerValue);
   m_nNodes = uintegerValue.Get ();
   GlobalValue::GetValueByName ("VRCnodeSpeed", uintegerValue);
   m_nodeSpeed = uintegerValue.Get ();
   GlobalValue::GetValueByName ("VRCnodePause", uintegerValue);
   m_nodePause = uintegerValue.Get ();
   GlobalValue::GetValueByName ("VRCwavePacketSize", uintegerValue);
   m_wavePacketSize = uintegerValue.Get ();
   GlobalValue::GetValueByName ("VRCverbose", uintegerValue);
   m_verbose = uintegerValue.Get ();
   GlobalValue::GetValueByName ("VRCscenario", uintegerValue);
   m_scenario = uintegerValue.Get ();
   GlobalValue::GetValueByName ("VRCroutingTables", uintegerValue);
   m_routingTables = uintegerValue.Get ();
   GlobalValue::GetValueByName ("VRCasciiTrace", uintegerValue);
   m_asciiTrace = uintegerValue.Get ();
   GlobalValue::GetValueByName ("VRCpcap", uintegerValue);
   m_pcap = uintegerValue.Get ();
   GlobalValue::GetValueByName ("VRCcumulativeBsmCaptureStart", uintegerValue);
   m_cumulativeBsmCaptureStart = uintegerValue.Get ();
 
   GlobalValue::GetValueByName ("VRCtxSafetyRange1", doubleValue);
   m_txSafetyRange1 = doubleValue.Get ();
   GlobalValue::GetValueByName ("VRCtxSafetyRange2", doubleValue);
   m_txSafetyRange2 = doubleValue.Get ();
   GlobalValue::GetValueByName ("VRCtxSafetyRange3", doubleValue);
   m_txSafetyRange3 = doubleValue.Get ();
   GlobalValue::GetValueByName ("VRCtxSafetyRange4", doubleValue);
   m_txSafetyRange4 = doubleValue.Get ();
   GlobalValue::GetValueByName ("VRCtxSafetyRange5", doubleValue);
   m_txSafetyRange5 = doubleValue.Get ();
   GlobalValue::GetValueByName ("VRCtxSafetyRange6", doubleValue);
   m_txSafetyRange6 = doubleValue.Get ();
   GlobalValue::GetValueByName ("VRCtxSafetyRange7", doubleValue);
   m_txSafetyRange7 = doubleValue.Get ();
   GlobalValue::GetValueByName ("VRCtxSafetyRange8", doubleValue);
   m_txSafetyRange8 = doubleValue.Get ();
   GlobalValue::GetValueByName ("VRCtxSafetyRange9", doubleValue);
   m_txSafetyRange9 = doubleValue.Get ();
   GlobalValue::GetValueByName ("VRCtxSafetyRange10", doubleValue);
   m_txSafetyRange10 = doubleValue.Get ();
   GlobalValue::GetValueByName ("VRCtxp", doubleValue);
   m_txp = doubleValue.Get ();
   GlobalValue::GetValueByName ("VRCtotalTime", doubleValue);
   m_TotalSimTime = doubleValue.Get ();
   GlobalValue::GetValueByName ("VRCwaveInterval", doubleValue);
   m_waveInterval = doubleValue.Get ();
   GlobalValue::GetValueByName ("VRCgpsAccuracyNs", doubleValue);
   m_gpsAccuracyNs = doubleValue.Get ();
   GlobalValue::GetValueByName ("VRCtxMaxDelayMs", doubleValue);
   m_txMaxDelayMs = doubleValue.Get ();
 
   GlobalValue::GetValueByName ("VRCCSVfileName", stringValue);
   m_CSVfileName = stringValue.Get ();
   GlobalValue::GetValueByName ("VRCCSVfileName2", stringValue);
   m_CSVfileName2 = stringValue.Get ();
   GlobalValue::GetValueByName ("VRCphyMode", stringValue);
   m_phyMode = stringValue.Get ();
   GlobalValue::GetValueByName ("VRCtraceFile", stringValue);
   m_traceFile = stringValue.Get ();
   GlobalValue::GetValueByName ("VRClogFile", stringValue);
   m_logFile = stringValue.Get ();
   GlobalValue::GetValueByName ("VRCrate", stringValue);
   m_rate = stringValue.Get ();
   GlobalValue::GetValueByName ("VRCphyModeB", stringValue);
   m_phyModeB = stringValue.Get ();
   GlobalValue::GetValueByName ("VRCtrName", stringValue);
   m_trName = stringValue.Get ();
 }
 
 void
 VanetRoutingExperiment::SetGlobalsFromConfig ()
 {
   // get settings saved from config-store
   UintegerValue uintegerValue;
   DoubleValue doubleValue;
   StringValue stringValue;
 
   g_port.SetValue (UintegerValue (m_port));
   g_nSinks.SetValue (UintegerValue (m_nSinks));
   g_traceMobility.SetValue (UintegerValue (m_traceMobility));
   g_protocol.SetValue (UintegerValue (m_protocol));
   g_lossModel.SetValue (UintegerValue (m_lossModel));
   g_fading.SetValue (UintegerValue (m_fading));
   g_80211mode.SetValue (UintegerValue (m_80211mode));
   g_mobility.SetValue (UintegerValue (m_mobility));
   g_nNodes.SetValue (UintegerValue (m_nNodes));
   g_nodeSpeed.SetValue (UintegerValue (m_nodeSpeed));
   g_nodePause.SetValue (UintegerValue (m_nodePause));
   g_wavePacketSize.SetValue (UintegerValue (m_wavePacketSize));
   g_verbose.SetValue (UintegerValue (m_verbose));
   g_scenario.SetValue (UintegerValue (m_scenario));
   g_routingTables.SetValue (UintegerValue (m_routingTables));
   g_asciiTrace.SetValue (UintegerValue (m_asciiTrace));
   g_pcap.SetValue (UintegerValue (m_pcap));
   g_cumulativeBsmCaptureStart.SetValue (UintegerValue (m_cumulativeBsmCaptureStart));
 
   g_txSafetyRange1.SetValue (DoubleValue (m_txSafetyRange1));
   g_txSafetyRange2.SetValue (DoubleValue (m_txSafetyRange2));
   g_txSafetyRange3.SetValue (DoubleValue (m_txSafetyRange3));
   g_txSafetyRange4.SetValue (DoubleValue (m_txSafetyRange4));
   g_txSafetyRange5.SetValue (DoubleValue (m_txSafetyRange5));
   g_txSafetyRange6.SetValue (DoubleValue (m_txSafetyRange6));
   g_txSafetyRange7.SetValue (DoubleValue (m_txSafetyRange7));
   g_txSafetyRange8.SetValue (DoubleValue (m_txSafetyRange8));
   g_txSafetyRange9.SetValue (DoubleValue (m_txSafetyRange9));
   g_txSafetyRange10.SetValue (DoubleValue (m_txSafetyRange10));
   g_txp.SetValue (DoubleValue (m_txp));
   g_totalTime.SetValue (DoubleValue (m_TotalSimTime));
   g_waveInterval.SetValue (DoubleValue (m_waveInterval));
   g_gpsAccuracyNs.SetValue (DoubleValue (m_gpsAccuracyNs));
   g_txMaxDelayMs.SetValue (DoubleValue (m_txMaxDelayMs));
 
   g_CSVfileName.SetValue (StringValue (m_CSVfileName));
   g_CSVfileName2.SetValue (StringValue (m_CSVfileName2));
   g_phyMode.SetValue (StringValue (m_phyMode));
   g_traceFile.SetValue (StringValue (m_traceFile));
   g_logFile.SetValue (StringValue (m_logFile));
   g_rate.SetValue (StringValue (m_rate));
   g_phyModeB.SetValue (StringValue (m_phyModeB));
   g_trName.SetValue (StringValue (m_trName));
   GlobalValue::GetValueByName ("VRCtrName", stringValue);
   m_trName = stringValue.Get ();
 }
 
 void
 VanetRoutingExperiment::CommandSetup (int argc, char **argv)
 {
   CommandLine cmd;
   double txDist1 = 50.0;
   double txDist2 = 100.0;
   double txDist3 = 150.0;
   double txDist4 = 200.0;
   double txDist5 = 250.0;
   double txDist6 = 300.0;
   double txDist7 = 350.0;
   double txDist8 = 400.0;
   double txDist9 = 450.0;
   double txDist10 = 500.0;
 
   // allow command line overrides
   cmd.AddValue ("CSVfileName", "The name of the CSV output file name", m_CSVfileName);
   cmd.AddValue ("CSVfileName2", "The name of the CSV output file name2", m_CSVfileName2);
   cmd.AddValue ("totaltime", "Simulation end time", m_TotalSimTime);
   cmd.AddValue ("nodes", "Number of nodes (i.e. vehicles)", m_nNodes);
   cmd.AddValue ("sinks", "Number of routing sinks", m_nSinks);
   cmd.AddValue ("txp", "Transmit power (dB), e.g. txp=7.5", m_txp);
   cmd.AddValue ("traceMobility", "Enable mobility tracing", m_traceMobility);
   cmd.AddValue ("protocol", "1=OLSR;2=AODV;3=DSDV;4=DSR", m_protocol);
   cmd.AddValue ("lossModel", "1=Friis;2=ItuR1411Los;3=TwoRayGround;4=LogDistance", m_lossModel);
   cmd.AddValue ("fading", "0=None;1=Nakagami;(buildings=1 overrides)", m_fading);
   cmd.AddValue ("phyMode", "Wifi Phy mode", m_phyMode);
   cmd.AddValue ("80211Mode", "1=802.11p; 2=802.11b; 3=WAVE-PHY", m_80211mode);
   cmd.AddValue ("traceFile", "Ns2 movement trace file", m_traceFile);
   cmd.AddValue ("logFile", "Log file", m_logFile);
   cmd.AddValue ("mobility", "1=trace;2=RWP", m_mobility);
   cmd.AddValue ("rate", "Rate", m_rate);
   cmd.AddValue ("phyModeB", "Phy mode 802.11b", m_phyModeB);
   cmd.AddValue ("speed", "Node speed (m/s)", m_nodeSpeed);
   cmd.AddValue ("pause", "Node pause (s)", m_nodePause);
   cmd.AddValue ("verbose", "0=quiet;1=verbose", m_verbose);
   cmd.AddValue ("bsm", "(WAVE) BSM size (bytes)", m_wavePacketSize);
   cmd.AddValue ("interval", "(WAVE) BSM interval (s)", m_waveInterval);
   cmd.AddValue ("scenario", "1=synthetic, 2=playback-trace", m_scenario);
   // User is allowed to have up to 10 different PDRs (Packet
   // Delivery Ratios) calculate, and so can specify up to
   // 10 different tx distances.
   cmd.AddValue ("txdist1", "Expected BSM tx range, m", txDist1);
   cmd.AddValue ("txdist2", "Expected BSM tx range, m", txDist2);
   cmd.AddValue ("txdist3", "Expected BSM tx range, m", txDist3);
   cmd.AddValue ("txdist4", "Expected BSM tx range, m", txDist4);
   cmd.AddValue ("txdist5", "Expected BSM tx range, m", txDist5);
   cmd.AddValue ("txdist6", "Expected BSM tx range, m", txDist6);
   cmd.AddValue ("txdist7", "Expected BSM tx range, m", txDist7);
   cmd.AddValue ("txdist8", "Expected BSM tx range, m", txDist8);
   cmd.AddValue ("txdist9", "Expected BSM tx range, m", txDist9);
   cmd.AddValue ("txdist10", "Expected BSM tx range, m", txDist10);
   cmd.AddValue ("gpsaccuracy", "GPS time accuracy, in ns", m_gpsAccuracyNs);
   cmd.AddValue ("txmaxdelay", "Tx max delay, in ms", m_txMaxDelayMs);
   cmd.AddValue ("routingTables", "Dump routing tables at t=5 seconds", m_routingTables);
   cmd.AddValue ("asciiTrace", "Dump ASCII Trace data", m_asciiTrace);
   cmd.AddValue ("pcap", "Create PCAP files for all nodes", m_pcap);
   cmd.AddValue ("loadconfig", "Config-store filename to load", m_loadConfigFilename);
   cmd.AddValue ("saveconfig", "Config-store filename to save", m_saveConfigFilename);
   cmd.AddValue ("exp", "Experiment", m_exp);
   cmd.AddValue ("BsmCaptureStart", "Start time to begin capturing pkts for cumulative Bsm", m_cumulativeBsmCaptureStart);
   cmd.Parse (argc, argv);
 
   m_txSafetyRange1 = txDist1;
   m_txSafetyRange2 = txDist2;
   m_txSafetyRange3 = txDist3;
   m_txSafetyRange4 = txDist4;
   m_txSafetyRange5 = txDist5;
   m_txSafetyRange6 = txDist6;
   m_txSafetyRange7 = txDist7;
   m_txSafetyRange8 = txDist8;
   m_txSafetyRange9 = txDist9;
   m_txSafetyRange10 = txDist10;
   // load configuration info from config-store
   ConfigStoreHelper configStoreHelper;
   configStoreHelper.LoadConfig (m_loadConfigFilename);
   // transfer config-store values to config parameters
   SetConfigFromGlobals ();
 
   // parse again so you can override input file default values via command line
   cmd.Parse (argc, argv);
 
   m_txSafetyRange1 = txDist1;
   m_txSafetyRange2 = txDist2;
   m_txSafetyRange3 = txDist3;
   m_txSafetyRange4 = txDist4;
   m_txSafetyRange5 = txDist5;
   m_txSafetyRange6 = txDist6;
   m_txSafetyRange7 = txDist7;
   m_txSafetyRange8 = txDist8;
   m_txSafetyRange9 = txDist9;
   m_txSafetyRange10 = txDist10;
 }
 
 void
 VanetRoutingExperiment::SetupLogFile ()
 {
   // open log file for output
   m_os.open (m_logFile.c_str ());
 }
 
 void VanetRoutingExperiment::SetupLogging ()
 {
 
   // Enable logging from the ns2 helper
   LogComponentEnable ("Ns2MobilityHelper",LOG_LEVEL_DEBUG);
 
   Packet::EnablePrinting ();
 }
 
 void
 VanetRoutingExperiment::ConfigureDefaults ()
 {
   Config::SetDefault ("ns3::OnOffApplication::PacketSize",StringValue ("64"));
   Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue (m_rate));
 
   //Set Non-unicastMode rate to unicast mode
   if (m_80211mode == 2)
     {
       Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (m_phyModeB));
     }
   else
     {
       Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (m_phyMode));
     }
 }
 
 void
 VanetRoutingExperiment::SetupAdhocMobilityNodes ()
 {
   if (m_mobility == 1)
     {
       // Create Ns2MobilityHelper with the specified trace log file as parameter
       Ns2MobilityHelper ns2 = Ns2MobilityHelper (m_traceFile);
       ns2.Install (); // configure movements for each node, while reading trace file
       // initially assume all nodes are not moving
       WaveBsmHelper::GetNodesMoving ().resize (m_nNodes, 0);
     }
   else if (m_mobility == 2)
     {
       MobilityHelper mobilityAdhoc;
 
       ObjectFactory pos;
       pos.SetTypeId ("ns3::RandomBoxPositionAllocator");
       pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"));
       pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=300.0]"));
       // we need antenna height uniform [1.0 .. 2.0] for loss model
       pos.Set ("Z", StringValue ("ns3::UniformRandomVariable[Min=1.0|Max=2.0]"));
 
       Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
       m_streamIndex += taPositionAlloc->AssignStreams (m_streamIndex);
 
       std::stringstream ssSpeed;
       ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << m_nodeSpeed << "]";
       std::stringstream ssPause;
       ssPause << "ns3::ConstantRandomVariable[Constant=" << m_nodePause << "]";
       mobilityAdhoc.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                                       "Speed", StringValue (ssSpeed.str ()),
                                       "Pause", StringValue (ssPause.str ()),
                                       "PositionAllocator", PointerValue (taPositionAlloc));
       mobilityAdhoc.SetPositionAllocator (taPositionAlloc);
       mobilityAdhoc.Install (m_adhocTxNodes);
       m_streamIndex += mobilityAdhoc.AssignStreams (m_adhocTxNodes, m_streamIndex);
 
       // initially assume all nodes are moving
       WaveBsmHelper::GetNodesMoving ().resize (m_nNodes, 1);
     }
 
   // Configure callback for logging
   Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange",
                    MakeBoundCallback (&VanetRoutingExperiment::CourseChange, &m_os));
 }
 
 void
 VanetRoutingExperiment::SetupAdhocDevices ()
 {
   if (m_lossModel == 1)
     {
       m_lossModelName = "ns3::FriisPropagationLossModel";                      //Different loss models
     }
   else if (m_lossModel == 2)
     {
       m_lossModelName = "ns3::ItuR1411LosPropagationLossModel";
     }
   else if (m_lossModel == 3)
     {
       m_lossModelName = "ns3::TwoRayGroundPropagationLossModel";
     }
   else if (m_lossModel == 4)
     {
       m_lossModelName = "ns3::LogDistancePropagationLossModel";
     }
   else
     {
       // Unsupported propagation loss model.
       // Treating as ERROR
       NS_LOG_ERROR ("Invalid propagation loss model specified.  Values must be [1-4], where 1=Friis;2=ItuR1411Los;3=TwoRayGround;4=LogDistance");
     }
 
   // frequency
   double freq = 0.0;
   if ((m_80211mode == 1)
       || (m_80211mode == 3))
     {
       // 802.11p 5.9 GHz
       freq = 5.9e9;                    //5Ghz model
     }
   else
     {
       // 802.11b 2.4 GHz
       freq = 2.4e9;                    //2Ghz model
     }
 
   // Setup propagation models
   YansWifiChannelHelper wifiChannel;
   wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
   if (m_lossModel == 3)
     {
       // two-ray requires antenna height (else defaults to Friss)
       wifiChannel.AddPropagationLoss (m_lossModelName, "Frequency", DoubleValue (freq), "HeightAboveZ", DoubleValue (1.5));
     }
   else
     {
       wifiChannel.AddPropagationLoss (m_lossModelName, "Frequency", DoubleValue (freq));
     }
 
   // Propagation loss models are additive.
   if (m_fading != 0)
     {
       // if no obstacle model, then use Nakagami fading if requested
       wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel");
     }
 
   // the channel
   Ptr<YansWifiChannel> channel = wifiChannel.Create ();
 
   // The below set of helpers will help us to put together the wifi NICs we want
   YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
   wifiPhy.SetChannel (channel);
   // ns-3 supports generate a pcap trace
   wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);
 
   YansWavePhyHelper wavePhy =  YansWavePhyHelper::Default ();
   wavePhy.SetChannel (channel);
   wavePhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);
 
   // Setup WAVE PHY and MAC
   NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default ();
   WaveHelper waveHelper = WaveHelper::Default ();
   Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();
   if (m_verbose)
     {
       wifi80211p.EnableLogComponents ();      // Turn on all Wifi 802.11p logging
       // likewise, turn on WAVE PHY logging
       waveHelper.EnableLogComponents ();
     }
 
   WifiHelper wifi;
 
   // Setup 802.11b stuff
   wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
 
   wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                 "DataMode",StringValue (m_phyModeB),
                                 "ControlMode",StringValue (m_phyModeB));
 
   // Setup 802.11p stuff
   wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                       "DataMode",StringValue (m_phyMode),
                                       "ControlMode",StringValue (m_phyMode));
 
   // Setup WAVE-PHY stuff
   waveHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                       "DataMode",StringValue (m_phyMode),
                                       "ControlMode",StringValue (m_phyMode));
 
   // Set Tx Power
   wifiPhy.Set ("TxPowerStart",DoubleValue (m_txp));
   wifiPhy.Set ("TxPowerEnd", DoubleValue (m_txp));
   wavePhy.Set ("TxPowerStart",DoubleValue (m_txp));
   wavePhy.Set ("TxPowerEnd", DoubleValue (m_txp));
 
   // Add an upper mac and disable rate control
   WifiMacHelper wifiMac;
   wifiMac.SetType ("ns3::AdhocWifiMac");
   QosWaveMacHelper waveMac = QosWaveMacHelper::Default ();
 
   // Setup net devices
 
   if (m_80211mode == 3)
     {
       m_adhocTxDevices = waveHelper.Install (wavePhy, waveMac, m_adhocTxNodes);
     }
   else if (m_80211mode == 1)
     {
       m_adhocTxDevices = wifi80211p.Install (wifiPhy, wifi80211pMac, m_adhocTxNodes);
     }
   else
     {
       m_adhocTxDevices = wifi.Install (wifiPhy, wifiMac, m_adhocTxNodes);
     }
 
   if (m_asciiTrace != 0)
     {
       AsciiTraceHelper ascii;
       Ptr<OutputStreamWrapper> osw = ascii.CreateFileStream ( (m_trName + ".tr").c_str ());
       wifiPhy.EnableAsciiAll (osw);
       wavePhy.EnableAsciiAll (osw);
     }
   if (m_pcap != 0)
     {
       wifiPhy.EnablePcapAll ("vanet-routing-compare-pcap");
       wavePhy.EnablePcapAll ("vanet-routing-compare-pcap");
     }
 }
 
 void
 VanetRoutingExperiment::SetupWaveMessages ()
 {
   // WAVE PHY mode
   // 0=continuous channel; 1=channel-switching
   int chAccessMode = 0;
   if (m_80211mode == 3)
     {
       chAccessMode = 1;
     }
 
   m_waveBsmHelper.Install (m_adhocTxInterfaces,
                            Seconds (m_TotalSimTime),
                            m_wavePacketSize,
                            Seconds (m_waveInterval),
                            // GPS accuracy (i.e, clock drift), in number of ns
                            m_gpsAccuracyNs,
                            m_txSafetyRanges,
                            chAccessMode,
                            // tx max delay before transmit, in ms
                            MilliSeconds (m_txMaxDelayMs));
 
   // fix random number streams
   m_streamIndex += m_waveBsmHelper.AssignStreams (m_adhocTxNodes, m_streamIndex);
 }
 
 void
 VanetRoutingExperiment::SetupRoutingMessages ()
 {
   m_routingHelper->Install (m_adhocTxNodes,
                             m_adhocTxDevices,
                             m_adhocTxInterfaces,
                             m_TotalSimTime,
                             m_protocol,
                             m_nSinks,
                             m_routingTables);
 }
 
 void
 VanetRoutingExperiment::SetupScenario ()
 {
   // member variable parameter use
   // defaults or command line overrides,
   // except where scenario={1,2,3,...}
   // have been specified, in which case
   // specify parameters are overwritten
   // here to setup for specific scenarios
 
   // certain parameters may be further overridden
   // i.e. specify a scenario, override tx power.

     if (m_scenario == 1)
     {
       // 40 nodes in RWP 300 m x 1500 m synthetic highway, 10s
       m_traceFile = "";
       m_logFile = "";
       m_mobility = 2;
       if (m_nNodes == 156)
         {
           m_nNodes = 40;
         }
       if (m_TotalSimTime == 300.01)
         {
           m_TotalSimTime = 10.0;
         }
     }
  else if (m_scenario == 2) 
     {
       m_traceFile = "/home/zen/150Vehiclesmobility.tcl"; 
       m_logFile = "HighDensity.log"; 
       m_mobility = 1;
       m_nNodes = 149;	
       m_TotalSimTime = 150.00;	
       m_nodeSpeed = 20; 
       m_nodePause = 0;
       m_CSVfileName = "HighDensityVanet1.csv"; 
       m_CSVfileName = "HighDensityVanet2.csv"; 
     }
 }
 
 void
 VanetRoutingExperiment::WriteCsvHeader ()
 {
   //blank out the last output file and write the column headers
   std::ofstream out (m_CSVfileName.c_str ());
   out << "SimulationSecond," <<
     "ReceiveRate," <<
     "PacketsReceived," <<
     "NumberOfSinks," <<
     "RoutingProtocol," <<
     "TransmissionPower," <<
     "WavePktsSent," <<
     "WavePtksReceived," <<
     "WavePktsPpr," <<
     "ExpectedWavePktsReceived," <<
     "ExpectedWavePktsInCoverageReceived," <<
     "BSM_PDR1," <<
     "BSM_PDR2," <<
     "BSM_PDR3," <<
     "BSM_PDR4," <<
     "BSM_PDR5," <<
     "BSM_PDR6," <<
     "BSM_PDR7," <<
     "BSM_PDR8," <<
     "BSM_PDR9," <<
     "BSM_PDR10," <<
     "MacPhyOverhead" <<
     std::endl;
   out.close ();
 
   std::ofstream out2 (m_CSVfileName2.c_str ());
   out2 << "BSM_PDR1,"
        << "BSM_PDR2,"
        << "BSM_PDR3,"
        << "BSM_PDR4,"
        << "BSM_PDR5,"
        << "BSM_PDR6,"
        << "BSM_PDR7,"
        << "BSM_PDR8,"
        << "BSM_PDR9,"
        << "BSM_PDR10,"
        << "AverageRoutingGoodputKbps,"
        << "MacPhyOverhead"
        << std::endl;
   out2.close ();
 }
 
 int
 main (int argc, char *argv[])
 {
   VanetRoutingExperiment experiment;
   experiment.Simulate (argc, argv);
 }

