#include "ns3/boolean.h"
#include "ns3/composite-energy-source.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * Fixed-window harvesting test: UseLeoCycle=false, constant 500 W from
 * t=0 to t=10 s, then zero. Battery starts partially charged (2000 J)
 * with a cap of 10000 J so the clamp never kicks in. We run up to
 * t=11 s (one extra second so the Li-Ion integrator captures the full
 * [9,10) interval via the update at t=10) and check both the total
 * energy added to the battery and the harvester's own accounting.
 */
class CompositeEnergySourceFixedWindowTest : public TestCase
{
  public:
    CompositeEnergySourceFixedWindowTest()
        : TestCase("CompositeEnergySource fixed-window harvesting")
    {
    }

    void DoRun() override
    {
        Ptr<CompositeEnergySource> source = CreateObject<CompositeEnergySource>();
        source->SetAttribute("InitialEnergyJ", DoubleValue(2000.0));
        source->SetAttribute("MaxEnergyJ", DoubleValue(10000.0));
        source->SetAttribute("UseLeoCycle", BooleanValue(false));
        source->AddSolarPanelWindow(500.0, 0.0, 10.0);
        // No node attaches this source; initialise it manually so the
        // harvester and Li-Ion update tick both start running.
        source->Initialize();

        NS_TEST_ASSERT_MSG_EQ_TOL(source->GetRemainingEnergy(),
                                  2000.0,
                                  1e-9,
                                  "initial energy mismatch");

        Simulator::Stop(Seconds(11.0));
        Simulator::Run();
        double remaining = source->GetRemainingEnergy();
        double harvested = source->GetTotalHarvestedEnergy();
        // Break the source<->harvester ref cycle before teardown since we
        // never aggregated the source to a Node.
        source->Dispose();
        Simulator::Destroy();

        // 500 W * 10 s = 5000 J. Allow a small tolerance for fixed-point
        // drift in the Li-Ion voltage integration.
        NS_TEST_ASSERT_MSG_EQ_TOL(remaining,
                                  2000.0 + 500.0 * 10.0,
                                  1.0,
                                  "remaining energy after fixed-window harvest");
        NS_TEST_ASSERT_MSG_EQ_TOL(harvested,
                                  500.0 * 10.0,
                                  1.0,
                                  "total harvested energy after fixed-window harvest");
    }
};

/**
 * LEO-cycle harvesting test: 10 s sunlight, 5 s shadow, starting in
 * sunlight. Over 30 s the cycle visits [0,10) sun, [10,15) shadow,
 * [15,25) sun, [25,30) shadow -> 20 s of harvesting. Only the sunlight
 * intervals should contribute to remaining energy.
 */
class CompositeEnergySourceLeoCycleTest : public TestCase
{
  public:
    CompositeEnergySourceLeoCycleTest()
        : TestCase("CompositeEnergySource LEO cycle harvesting")
    {
    }

    void DoRun() override
    {
        Ptr<CompositeEnergySource> source = CreateObject<CompositeEnergySource>();
        source->SetAttribute("InitialEnergyJ", DoubleValue(1000.0));
        source->SetAttribute("MaxEnergyJ", DoubleValue(100000.0));
        source->SetAttribute("UseLeoCycle", BooleanValue(true));
        source->SetAttribute("PanelAreaM2", DoubleValue(1.0));
        source->SetAttribute("PanelEfficiency", DoubleValue(0.25));
        source->SetAttribute("SolarConstantWm2", DoubleValue(1361.0));
        source->SetAttribute("HarvestIntervalSeconds", DoubleValue(1.0));
        source->SetAttribute("SunlightSeconds", DoubleValue(10.0));
        source->SetAttribute("ShadowSeconds", DoubleValue(5.0));
        source->Initialize();

        Simulator::Stop(Seconds(30.0));
        Simulator::Run();
        double remaining = source->GetRemainingEnergy();
        double harvested = source->GetTotalHarvestedEnergy();
        // Break the source<->harvester ref cycle before teardown since we
        // never aggregated the source to a Node.
        source->Dispose();
        Simulator::Destroy();

        const double p = 1361.0 * 1.0 * 0.25; // W during sunlight
        const double sunlightSeconds = 10.0 + 10.0;
        const double expectedHarvested = p * sunlightSeconds;

        NS_TEST_ASSERT_MSG_EQ_TOL(remaining,
                                  1000.0 + expectedHarvested,
                                  2.0,
                                  "remaining energy after LEO cycle");
        NS_TEST_ASSERT_MSG_EQ_TOL(harvested,
                                  expectedHarvested,
                                  2.0,
                                  "total harvested energy after LEO cycle");
    }
};

class CompositeEnergySourceTestSuite : public TestSuite
{
  public:
    CompositeEnergySourceTestSuite()
        : TestSuite("composite-energy-source", Type::UNIT)
    {
        AddTestCase(new CompositeEnergySourceFixedWindowTest, TestCase::Duration::QUICK);
        AddTestCase(new CompositeEnergySourceLeoCycleTest, TestCase::Duration::QUICK);
    }
};

static CompositeEnergySourceTestSuite g_compositeEnergySourceTestSuite;
