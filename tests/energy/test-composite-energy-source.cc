// tests/energy/test-composite-energy-source.cc

#include "ns3/test.h"
#include "ns3/composite-energy-source.h"
#include "ns3/li-ion-energy-source.h"

using namespace ns3;

/**
 * \brief TestSuite for CompositeEnergySource
 */
class CompositeEnergySourceTestSuite : public TestSuite
{
public:
  CompositeEnergySourceTestSuite ()
    : TestSuite ("composite-energy-source", UNIT)
  {
    AddTestCase (new CompositeEnergySourceTest, TestCase::QUICK);
  }
};

/**
 * \brief TestCase for CompositeEnergySource functionality
 */
class CompositeEnergySourceTest : public TestCase
{
public:
  CompositeEnergySourceTest ()
    : TestCase ("CompositeEnergySource test")
  {
  }

  virtual void DoRun ()
  {
    // Create battery
    Ptr<LiIonEnergySource> battery = CreateObject<LiIonEnergySource> ();
    battery->SetAttribute ("InitialEnergyJ", DoubleValue (2000.0));
    battery->SetAttribute ("CapacityJ", DoubleValue (2000.0));

    // Create CompositeEnergySource
    Ptr<CompositeEnergySource> compositeEnergy = CreateObject<CompositeEnergySource> ();
    compositeEnergy->AddBattery (battery);
    compositeEnergy->AddSolarPanel (500.0, 0.0, 10.0); // Harvesting from 0s to 10s

    // Check initial energy
    NS_TEST_ASSERT_MSG_EQ (compositeEnergy->GetRemainingEnergy (), 2000.0, "Initial energy mismatch");

    // Simulate energy harvesting
    Simulator::Run ();

    // After 10 seconds, harvesting should have stopped
    NS_TEST_ASSERT_MSG_EQ (compositeEnergy->GetRemainingEnergy (), 2000.0 + (500.0 * 10.0), "Energy harvesting incorrect");
  }
};

// Register TestSuite
static CompositeEnergySourceTestSuite compositeEnergySourceTestSuite;
