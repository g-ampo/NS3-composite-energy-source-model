#include "composite-energy-source.h"

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("CompositeEnergySource");
NS_OBJECT_ENSURE_REGISTERED(CompositeEnergySource);

TypeId
CompositeEnergySource::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CompositeEnergySource")
            .SetParent<LiIonEnergySource>()
            .SetGroupName("Energy")
            .AddConstructor<CompositeEnergySource>()
            .AddAttribute("UseLeoCycle",
                          "Enable the repeating sunlight/shadow LEO cycle. When false, "
                          "harvesting is driven by AddSolarPanelWindow() instead.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&CompositeEnergySource::m_useLeoCycle),
                          MakeBooleanChecker())
            .AddAttribute("PanelAreaM2",
                          "Solar panel area (m^2).",
                          DoubleValue(2.0),
                          MakeDoubleAccessor(&CompositeEnergySource::m_panelAreaM2),
                          MakeDoubleChecker<double>(0.0))
            .AddAttribute("PanelEfficiency",
                          "Panel conversion efficiency, in [0,1].",
                          DoubleValue(0.28),
                          MakeDoubleAccessor(&CompositeEnergySource::m_panelEfficiency),
                          MakeDoubleChecker<double>(0.0, 1.0))
            .AddAttribute("SolarConstantWm2",
                          "Solar constant (W/m^2).",
                          DoubleValue(1361.0),
                          MakeDoubleAccessor(&CompositeEnergySource::m_solarConstantWm2),
                          MakeDoubleChecker<double>(0.0))
            .AddAttribute("HarvestIntervalSeconds",
                          "Period at which the harvest current is recomputed and re-applied "
                          "to the internal harvester (s). Smaller values track state changes "
                          "(sunlight toggle, full-charge clamp) more tightly at some cost.",
                          DoubleValue(1.0),
                          MakeDoubleAccessor(&CompositeEnergySource::m_harvestIntervalSeconds),
                          MakeDoubleChecker<double>(1e-6))
            .AddAttribute("SunlightSeconds",
                          "Duration of sunlight per LEO cycle (s).",
                          DoubleValue(3900.0),
                          MakeDoubleAccessor(&CompositeEnergySource::m_sunlightSeconds),
                          MakeDoubleChecker<double>(0.0))
            .AddAttribute("ShadowSeconds",
                          "Duration of umbra per LEO cycle (s).",
                          DoubleValue(1800.0),
                          MakeDoubleAccessor(&CompositeEnergySource::m_shadowSeconds),
                          MakeDoubleChecker<double>(0.0))
            .AddAttribute("MaxEnergyJ",
                          "Upper bound (J) on the battery's remaining energy. Harvest "
                          "current is driven to zero when the remaining energy reaches this "
                          "value. Use this to permit charging above InitialEnergyJ when the "
                          "battery starts partially discharged. A value of 0 (default) means "
                          "use GetInitialEnergy() as the cap (i.e. InitialEnergyJ represents "
                          "a full cell).",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&CompositeEnergySource::m_maxEnergyJ),
                          MakeDoubleChecker<double>(0.0));
    return tid;
}

CompositeEnergySource::CompositeEnergySource()
    : m_harvester(CreateObject<SolarHarvesterDeviceModel>()),
      m_windowPowerW(0.0),
      m_windowStart(0.0),
      m_windowEnd(0.0),
      m_useLeoCycle(true),
      m_panelAreaM2(2.0),
      m_panelEfficiency(0.28),
      m_solarConstantWm2(1361.0),
      m_harvestIntervalSeconds(1.0),
      m_sunlightSeconds(3900.0),
      m_shadowSeconds(1800.0),
      m_maxEnergyJ(0.0),
      m_inSunlight(true)
{
    NS_LOG_FUNCTION(this);
}

CompositeEnergySource::~CompositeEnergySource()
{
    NS_LOG_FUNCTION(this);
}

void
CompositeEnergySource::AddSolarPanelWindow(double powerJoulePerSecond,
                                           double startTime,
                                           double endTime)
{
    NS_LOG_FUNCTION(this << powerJoulePerSecond << startTime << endTime);
    m_windowPowerW = powerJoulePerSecond;
    m_windowStart = startTime;
    m_windowEnd = endTime;
    // The periodic UpdateHarvestCurrent() tick picks up the window
    // boundaries automatically; no separate scheduling required.
}

void
CompositeEnergySource::ConfigureSolarHarvester(double panelAreaM2,
                                               double panelEfficiency,
                                               double solarConstantWm2)
{
    NS_LOG_FUNCTION(this << panelAreaM2 << panelEfficiency << solarConstantWm2);
    m_panelAreaM2 = panelAreaM2;
    m_panelEfficiency = panelEfficiency;
    m_solarConstantWm2 = solarConstantWm2;
}

double
CompositeEnergySource::GetTotalHarvestedEnergy() const
{
    return m_harvester ? m_harvester->GetTotalHarvestedEnergy() : 0.0;
}

bool
CompositeEnergySource::IsInSunlight() const
{
    return m_useLeoCycle ? m_inSunlight : true;
}

void
CompositeEnergySource::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    // Attach the internal harvester as a DeviceEnergyModel on this source so
    // the Li-Ion integrator will include its (negative) current contribution.
    m_harvester->SetEnergySource(this);
    AppendDeviceEnergyModel(m_harvester);

    // Base-class initialization schedules the periodic Li-Ion update.
    LiIonEnergySource::DoInitialize();

    // Kick off the harvest-control loop. First tick at t=0 sets the initial
    // current; subsequent ticks track full-charge clamping and LEO/window
    // transitions.
    m_harvestEvent = Simulator::ScheduleNow(&CompositeEnergySource::UpdateHarvestCurrent, this);

    if (m_useLeoCycle)
    {
        m_inSunlight = true;
        m_toggleEvent = Simulator::Schedule(Seconds(m_sunlightSeconds),
                                            &CompositeEnergySource::ToggleSunlight,
                                            this);
    }
}

void
CompositeEnergySource::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(m_harvestEvent);
    Simulator::Cancel(m_toggleEvent);
    if (m_harvester)
    {
        m_harvester->Dispose();
        m_harvester = nullptr;
    }
    LiIonEnergySource::DoDispose();
}

void
CompositeEnergySource::ToggleSunlight()
{
    NS_LOG_FUNCTION(this);
    m_inSunlight = !m_inSunlight;
    // We deliberately do NOT call UpdateHarvestCurrent() inline here: the
    // periodic harvest tick (UID strictly greater than ours for same simtime)
    // will pick up the new phase on its next firing. Calling it inline would
    // reschedule m_harvestEvent in addition to the already-queued self-
    // reschedule from the previous tick, producing duplicate events that
    // compound each cycle.
    double next = m_inSunlight ? m_sunlightSeconds : m_shadowSeconds;
    m_toggleEvent =
        Simulator::Schedule(Seconds(next), &CompositeEnergySource::ToggleSunlight, this);
}

void
CompositeEnergySource::UpdateHarvestCurrent()
{
    NS_LOG_FUNCTION(this);

    // Clamp: when at (or above) the configured cap, stop injecting. This
    // keeps the Li-Ion integrator from over-filling the cell in sustained
    // sunlight. MaxEnergyJ=0 (default) means the cap equals GetInitialEnergy().
    double cap = (m_maxEnergyJ > 0.0) ? m_maxEnergyJ : GetInitialEnergy();
    bool full = GetRemainingEnergy() >= cap;

    double harvestPowerW = 0.0;
    if (!full)
    {
        if (m_useLeoCycle)
        {
            if (m_inSunlight)
            {
                harvestPowerW = m_solarConstantWm2 * m_panelAreaM2 * m_panelEfficiency;
            }
        }
        else
        {
            double t = Simulator::Now().GetSeconds();
            if (t >= m_windowStart && t < m_windowEnd)
            {
                harvestPowerW = m_windowPowerW;
            }
        }
    }

    double v = GetSupplyVoltage();
    double harvestCurrentA = (harvestPowerW > 0.0 && v > 0.0) ? (harvestPowerW / v) : 0.0;
    m_harvester->SetHarvestCurrentA(harvestCurrentA);

    NS_LOG_DEBUG("t=" << Simulator::Now().GetSeconds()
                      << "s sunlight=" << (m_inSunlight ? 1 : 0) << " P=" << harvestPowerW
                      << "W V=" << v << "V I=" << harvestCurrentA << "A full=" << full);

    // Reschedule. Even when harvestPowerW==0 we keep ticking so that a
    // transition back into sunlight, or discharge below full, is picked up.
    m_harvestEvent = Simulator::Schedule(Seconds(m_harvestIntervalSeconds),
                                         &CompositeEnergySource::UpdateHarvestCurrent,
                                         this);
}

} // namespace ns3
