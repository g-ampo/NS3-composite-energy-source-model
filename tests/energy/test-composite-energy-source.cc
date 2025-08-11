// tests/energy/test-composite-energy-source.cc

#include "ns3/test.h"
#include "CompositeEnergySource.h"
#include "ns3/simulator.h"

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
    AddTestCase (new CompositeEnergySourceLeoCycleTest, TestCase::QUICK);
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
    // Create CompositeEnergySource (subclass of LiIonEnergySource)
    Ptr<CompositeEnergySource> source = CreateObject<CompositeEnergySource> ();
    source->SetAttribute ("InitialEnergyJ", DoubleValue (2000.0));
    source->SetAttribute ("CapacityJ", DoubleValue (2000.0));
    source->SetAttribute ("UseLeoCycle", BooleanValue (false));
    source->AddSolarPanelWindow (500.0, 0.0, 10.0); // 500 J/s for 10 s

    NS_TEST_ASSERT_MSG_EQ_TOL (source->GetRemainingEnergy (), 2000.0, 1e-9, "Initial energy mismatch");

    Simulator::Stop (Seconds (10.0));
    Simulator::Run ();
    Simulator::Destroy ();

    NS_TEST_ASSERT_MSG_EQ_TOL (source->GetRemainingEnergy (), 2000.0 + (500.0 * 10.0), 1e-6, "Energy harvesting incorrect");
  }
};

// Test LEO cycle: harvest during sunlight segments; no harvest in shadow
class CompositeEnergySourceLeoCycleTest : public TestCase
{
public:
  CompositeEnergySourceLeoCycleTest ()
    : TestCase ("CompositeEnergySource LEO cycle test")
  {
  }

  virtual void DoRun ()
  {
    Ptr<CompositeEnergySource> source = CreateObject<CompositeEnergySource> ();
    source->SetAttribute ("InitialEnergyJ", DoubleValue (1000.0));
    source->SetAttribute ("CapacityJ", DoubleValue (10000.0));
    source->SetAttribute ("UseLeoCycle", BooleanValue (true));
    source->SetAttribute ("PanelAreaM2", DoubleValue (1.0));
    source->SetAttribute ("PanelEfficiency", DoubleValue (0.25));
    source->SetAttribute ("SolarConstantWm2", DoubleValue (1361.0));
    source->SetAttribute ("HarvestIntervalSeconds", DoubleValue (1.0));
    source->SetAttribute ("SunlightSeconds", DoubleValue (10.0));
    source->SetAttribute ("ShadowSeconds", DoubleValue (5.0));

    // Run 30 seconds: sunlight [0,10), shadow [10,15), sunlight [15,25), shadow [25,30)
    Simulator::Stop (Seconds (30.0));
    Simulator::Run ();
    Simulator::Destroy ();

    // Expected harvested energy is sunlight_time * solar_power
    const double p = 1361.0 * 1.0 * 0.25; // W == J/s
    const double sunlightTotal = 10.0 + 10.0; // 20 seconds sunlight
    const double expected = 1000.0 + p * sunlightTotal;
    NS_TEST_ASSERT_MSG_EQ_TOL (source->GetRemainingEnergy (), expected, 1e-6, "LEO cycle harvesting incorrect");
  }
};

// Register TestSuite
static CompositeEnergySourceTestSuite compositeEnergySourceTestSuite;
