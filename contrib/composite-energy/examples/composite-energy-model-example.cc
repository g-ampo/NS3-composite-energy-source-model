/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * CompositeEnergySource example: UAVs and Satellites in a toy topology.
 *
 * UAVs        : Li-Ion battery only, no harvesting.
 * Satellites  : Li-Ion battery + solar harvesting via CompositeEnergySource.
 *               The LEO cycle alternates SunlightSeconds / ShadowSeconds.
 */

#include "ns3/composite-energy-module.h"
#include "ns3/core-module.h"
#include "ns3/energy-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

#include <iostream>

using namespace ns3;

static void
PrintEnergyStatus(Ptr<LiIonEnergySource> source)
{
    std::cout << "t=" << Simulator::Now().GetSeconds() << "s"
              << " V=" << source->GetSupplyVoltage()
              << "V E_remaining=" << source->GetRemainingEnergy() << "J";
    Ptr<CompositeEnergySource> ces = DynamicCast<CompositeEnergySource>(source);
    if (ces)
    {
        std::cout << " E_harvested=" << ces->GetTotalHarvestedEnergy() << "J"
                  << " sunlight=" << (ces->IsInSunlight() ? 1 : 0);
    }
    std::cout << std::endl;

    if (!Simulator::IsFinished())
    {
        Simulator::Schedule(Seconds(60), &PrintEnergyStatus, source);
    }
}

int
main(int argc, char* argv[])
{
    // LogComponentEnable("CompositeEnergySource", LOG_LEVEL_INFO);
    // LogComponentEnable("SolarHarvesterDeviceModel", LOG_LEVEL_INFO);

    NodeContainer uavs;
    uavs.Create(10);

    NodeContainer satellites;
    satellites.Create(2);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(uavs);
    mobility.Install(satellites);

    // --- UAVs: Li-Ion battery only -----------------------------------------
    for (uint32_t i = 0; i < uavs.GetN(); ++i)
    {
        Ptr<Node> node = uavs.Get(i);

        Ptr<LiIonEnergySource> battery = CreateObject<LiIonEnergySource>();
        battery->SetAttribute("InitialEnergyJ", DoubleValue(1500.0));
        battery->SetAttribute("InitialCellVoltage", DoubleValue(4.0));
        battery->SetAttribute("NominalCellVoltage", DoubleValue(3.7));
        battery->SetAttribute("ExpCellVoltage", DoubleValue(3.5));
        battery->SetAttribute("InternalResistance", DoubleValue(0.07));
        battery->SetAttribute("ThresholdVoltage", DoubleValue(3.2));
        battery->SetAttribute("PeriodicEnergyUpdateInterval", TimeValue(Seconds(1)));

        Ptr<SimpleDeviceEnergyModel> dem = CreateObject<SimpleDeviceEnergyModel>();
        dem->SetEnergySource(battery);
        dem->SetNode(node);

        Ptr<EnergySourceContainer> container = CreateObject<EnergySourceContainer>();
        container->Add(battery);
        node->AggregateObject(container);
    }

    // --- Satellites: Li-Ion battery with solar harvesting ------------------
    for (uint32_t i = 0; i < satellites.GetN(); ++i)
    {
        Ptr<Node> node = satellites.Get(i);

        Ptr<CompositeEnergySource> source = CreateObject<CompositeEnergySource>();
        // Start partially discharged, room to harvest up to 4000 J.
        source->SetAttribute("InitialEnergyJ", DoubleValue(2000.0));
        source->SetAttribute("MaxEnergyJ", DoubleValue(4000.0));
        source->SetAttribute("InitialCellVoltage", DoubleValue(4.2));
        source->SetAttribute("NominalCellVoltage", DoubleValue(3.8));
        source->SetAttribute("ExpCellVoltage", DoubleValue(3.5));
        source->SetAttribute("InternalResistance", DoubleValue(0.05));
        source->SetAttribute("ThresholdVoltage", DoubleValue(3.3));
        source->SetAttribute("PeriodicEnergyUpdateInterval", TimeValue(Seconds(1)));
        // LEO harvesting configuration.
        source->SetAttribute("UseLeoCycle", BooleanValue(true));
        source->SetAttribute("PanelAreaM2", DoubleValue(2.0));
        source->SetAttribute("PanelEfficiency", DoubleValue(0.28));
        source->SetAttribute("SolarConstantWm2", DoubleValue(1361.0));
        source->SetAttribute("SunlightSeconds", DoubleValue(3900.0));
        source->SetAttribute("ShadowSeconds", DoubleValue(1800.0));
        source->SetAttribute("HarvestIntervalSeconds", DoubleValue(1.0));
        // Alternatively, a fixed window:
        //   source->SetAttribute("UseLeoCycle", BooleanValue(false));
        //   source->AddSolarPanelWindow(500.0, 0.0, 1200.0);

        Ptr<SimpleDeviceEnergyModel> dem = CreateObject<SimpleDeviceEnergyModel>();
        dem->SetEnergySource(source);
        dem->SetNode(node);

        Ptr<EnergySourceContainer> container = CreateObject<EnergySourceContainer>();
        container->Add(source);
        node->AggregateObject(container);
    }

    // --- Periodic energy-status printout (satellites) ----------------------
    for (uint32_t i = 0; i < satellites.GetN(); ++i)
    {
        Ptr<Node> node = satellites.Get(i);
        Ptr<EnergySourceContainer> container = node->GetObject<EnergySourceContainer>();
        Ptr<LiIonEnergySource> source = container->Get(0)->GetObject<LiIonEnergySource>();
        Simulator::Schedule(Seconds(0), &PrintEnergyStatus, source);
    }

    // --- UAV traffic: constant moderate draw from t=10 to t=1701 -----------
    for (uint32_t i = 0; i < uavs.GetN(); ++i)
    {
        Ptr<Node> node = uavs.Get(i);
        Ptr<EnergySourceContainer> container = node->GetObject<EnergySourceContainer>();
        Ptr<LiIonEnergySource> battery = container->Get(0)->GetObject<LiIonEnergySource>();
        Ptr<SimpleDeviceEnergyModel> dem =
            battery->GetDeviceEnergyModel(0)->GetObject<SimpleDeviceEnergyModel>();

        Simulator::Schedule(Seconds(10), &SimpleDeviceEnergyModel::SetCurrentA, dem, 2.33);
        Simulator::Schedule(Seconds(1701), &SimpleDeviceEnergyModel::SetCurrentA, dem, 1e-3);
    }

    // --- Satellite traffic: heavy draw that outpaces harvesting briefly ----
    // On a CompositeEnergySource, the SimpleDeviceEnergyModel attached during
    // setup is at index 0; the internal SolarHarvesterDeviceModel is at
    // index 1 and is managed by the source itself.
    for (uint32_t i = 0; i < satellites.GetN(); ++i)
    {
        Ptr<Node> node = satellites.Get(i);
        Ptr<EnergySourceContainer> container = node->GetObject<EnergySourceContainer>();
        Ptr<LiIonEnergySource> source = container->Get(0)->GetObject<LiIonEnergySource>();
        Ptr<SimpleDeviceEnergyModel> dem =
            source->GetDeviceEnergyModel(0)->GetObject<SimpleDeviceEnergyModel>();

        Simulator::Schedule(Seconds(10), &SimpleDeviceEnergyModel::SetCurrentA, dem, 4.66);
        Simulator::Schedule(Seconds(2301), &SimpleDeviceEnergyModel::SetCurrentA, dem, 1e-3);
    }

    Simulator::Stop(Seconds(2400));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
