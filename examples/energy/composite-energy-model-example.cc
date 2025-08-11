// examples/energy/composite-energy-model-example.cc

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * CompositeEnergySource Example for UAVs and Satellites
 * UAVs: Battery-powered only
 * Satellites: Battery-powered with periodic solar energy harvesting
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/energy-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "CompositeEnergySource.h" // Custom CompositeEnergySource

using namespace ns3;

// Callback to print energy status
static void
PrintEnergyStatus (Ptr<LiIonEnergySource> source)
{
  std::cout << "Time: " << Simulator::Now ().GetSeconds ()
            << "s, Voltage: " << source->GetSupplyVoltage ()
            << "V, Remaining Energy: " << source->GetRemainingEnergy ()
            << " J" << std::endl;

  if (!Simulator::IsFinished ())
    {
      Simulator::Schedule (Seconds (20), &PrintEnergyStatus, source);
    }
}

int
main (int argc, char *argv[])
{
  // Enable logging for debugging (optional)
  // LogComponentEnable("CompositeEnergySource", LOG_LEVEL_INFO);
  // LogComponentEnable("LiIonEnergySource", LOG_LEVEL_INFO);

  // Create UAV and Satellite nodes
  NodeContainer uavs;
  uavs.Create (10); // 10 UAVs

  NodeContainer satellites;
  satellites.Create (2); // 2 Satellites

  // Install mobility models (static for simplicity)
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (uavs);
  mobility.Install (satellites);

  // Setup energy models for UAVs
  for (uint32_t i = 0; i < uavs.GetN (); ++i)
    {
      Ptr<Node> node = uavs.Get (i);

      // Create and configure LiIonEnergySource for UAV
      Ptr<LiIonEnergySource> battery = CreateObject<LiIonEnergySource> ();
      battery->SetAttribute ("InitialEnergyJ", DoubleValue (1500.0)); // 1500 J
      battery->SetAttribute ("CapacityJ", DoubleValue (1500.0));       // 1500 J
      battery->SetAttribute ("InitialCellVoltage", DoubleValue (4.0)); // 4.0 V
      battery->SetAttribute ("NominalCellVoltage", DoubleValue (3.7)); // 3.7 V
      battery->SetAttribute ("ExpCellVoltage", DoubleValue (3.5));     // 3.5 V
      battery->SetAttribute ("InternalResistance", DoubleValue (0.07)); // 0.07 Ω
      battery->SetAttribute ("ThresholdVoltage", DoubleValue (3.2));    // 3.2 V
      battery->SetAttribute ("PeriodicEnergyUpdateInterval", TimeValue (Seconds (1)));

      // Create and configure SimpleDeviceEnergyModel for UAV
      Ptr<SimpleDeviceEnergyModel> deviceEnergyModel = CreateObject<SimpleDeviceEnergyModel> ();
      deviceEnergyModel->SetEnergySource (battery);
      deviceEnergyModel->SetNode (node);

      // Aggregate EnergySourceContainer with the node
      Ptr<EnergySourceContainer> energySourceContainer = CreateObject<EnergySourceContainer> ();
      energySourceContainer->Add (battery);
      node->AggregateObject (energySourceContainer);
    }

  // Setup energy models for Satellites
  for (uint32_t i = 0; i < satellites.GetN (); ++i)
    {
      Ptr<Node> node = satellites.Get (i);

      // Create and configure LiIonEnergySource for Satellite
      Ptr<LiIonEnergySource> battery = CreateObject<LiIonEnergySource> ();
      battery->SetAttribute ("InitialEnergyJ", DoubleValue (2000.0)); // 2000 J
      battery->SetAttribute ("CapacityJ", DoubleValue (2000.0));       // 2000 J
      battery->SetAttribute ("InitialCellVoltage", DoubleValue (4.2)); // 4.2 V
      battery->SetAttribute ("NominalCellVoltage", DoubleValue (3.8)); // 3.8 V
      battery->SetAttribute ("ExpCellVoltage", DoubleValue (3.5));     // 3.5 V
      battery->SetAttribute ("InternalResistance", DoubleValue (0.05)); // 0.05 Ω
      battery->SetAttribute ("ThresholdVoltage", DoubleValue (3.3));    // 3.3 V
      battery->SetAttribute ("PeriodicEnergyUpdateInterval", TimeValue (Seconds (1)));

      // Create and configure CompositeEnergySource for Satellite
      Ptr<CompositeEnergySource> source = CreateObject<CompositeEnergySource> ();
      // Configure LEO cycle harvesting (sunlight/shadow) or fixed window
      source->SetAttribute ("UseLeoCycle", BooleanValue (true));
      source->SetAttribute ("PanelAreaM2", DoubleValue (2.0));
      source->SetAttribute ("PanelEfficiency", DoubleValue (0.28));
      source->SetAttribute ("SolarConstantWm2", DoubleValue (1361.0));
      source->SetAttribute ("SunlightSeconds", DoubleValue (3900.0));
      source->SetAttribute ("ShadowSeconds", DoubleValue (1800.0));
      source->SetAttribute ("HarvestIntervalSeconds", DoubleValue (1.0));
      // Alternatively, to use a fixed harvesting window, uncomment:
      // source->SetAttribute ("UseLeoCycle", BooleanValue (false));
      // source->AddSolarPanelWindow (500.0, 0.0, 1200.0);

      // Attach a SimpleDeviceEnergyModel directly to the CompositeEnergySource (subclass of LiIon)
      Ptr<SimpleDeviceEnergyModel> deviceEnergyModel = CreateObject<SimpleDeviceEnergyModel> ();
      deviceEnergyModel->SetEnergySource (source);
      deviceEnergyModel->SetNode (node);

      // Aggregate EnergySourceContainer with the node
      Ptr<EnergySourceContainer> energySourceContainer = CreateObject<EnergySourceContainer> ();
      energySourceContainer->Add (source);
      node->AggregateObject (energySourceContainer);
    }

  // Schedule energy status printing for Satellites (optional)
  for (uint32_t i = 0; i < satellites.GetN (); ++i)
    {
      Ptr<Node> node = satellites.Get (i);
      Ptr<EnergySourceContainer> energySourceContainer = node->GetObject<EnergySourceContainer> ();
      Ptr<LiIonEnergySource> source = energySourceContainer->Get (0)->GetObject<LiIonEnergySource> ();
      Simulator::Schedule (Seconds (0), &PrintEnergyStatus, source);
    }

  // Example: Schedule some energy-consuming activities
  // For UAVs
  for (uint32_t i = 0; i < uavs.GetN (); ++i)
    {
      Ptr<Node> node = uavs.Get (i);
      Ptr<EnergySourceContainer> energySourceContainer = node->GetObject<EnergySourceContainer> ();
      Ptr<LiIonEnergySource> battery = energySourceContainer->Get (0)->GetObject<LiIonEnergySource> ();
      Ptr<SimpleDeviceEnergyModel> deviceEnergyModel = battery->GetDeviceEnergyModel (0)->GetObject<SimpleDeviceEnergyModel> ();

      // Simulate transmission activity: 2.33 A from 10s to 1701s
      Simulator::Schedule (Seconds (10), &SimpleDeviceEnergyModel::SetCurrentA, deviceEnergyModel, 2.33); // Start transmission
      Simulator::Schedule (Seconds (1701), &SimpleDeviceEnergyModel::SetCurrentA, deviceEnergyModel, 1e-3); // Back to idle
    }

  // For Satellites
  for (uint32_t i = 0; i < satellites.GetN (); ++i)
    {
      Ptr<Node> node = satellites.Get (i);
      Ptr<EnergySourceContainer> energySourceContainer = node->GetObject<EnergySourceContainer> ();
      Ptr<LiIonEnergySource> source = energySourceContainer->Get (0)->GetObject<LiIonEnergySource> ();
      Ptr<SimpleDeviceEnergyModel> deviceEnergyModel = CreateObject<SimpleDeviceEnergyModel> ();
      deviceEnergyModel->SetEnergySource (source);
      deviceEnergyModel->SetNode (node);

      // Simulate high-load transmission: 4.66 A from 10s to 2301s
      Simulator::Schedule (Seconds (10), &SimpleDeviceEnergyModel::SetCurrentA, deviceEnergyModel, 4.66); // Start high-load transmission
      Simulator::Schedule (Seconds (2301), &SimpleDeviceEnergyModel::SetCurrentA, deviceEnergyModel, 1e-3); // Back to idle
    }

  // Run the simulation
  Simulator::Stop (Seconds (2400)); // Total simulation time: 2400 seconds
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
