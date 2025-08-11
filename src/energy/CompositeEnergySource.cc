// CompositeEnergySource.cc

#include "CompositeEnergySource.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CompositeEnergySource");
NS_OBJECT_ENSURE_REGISTERED (CompositeEnergySource);

TypeId
CompositeEnergySource::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CompositeEnergySource")
    .SetParent<EnergySource> ()
    .SetGroupName("Energy")
    .AddConstructor<CompositeEnergySource> ()
    .AddAttribute ("UseLeoCycle",
                   "Enable repeating sunlight/shadow LEO cycle.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&CompositeEnergySource::m_useLeoCycle),
                   MakeBooleanChecker ())
    .AddAttribute ("PanelAreaM2",
                   "Solar panel area (m^2).",
                   DoubleValue (2.0),
                   MakeDoubleAccessor (&CompositeEnergySource::m_panelAreaM2),
                   MakeDoubleChecker<double> (0.0))
    .AddAttribute ("PanelEfficiency",
                   "Panel efficiency (0..1).",
                   DoubleValue (0.28),
                   MakeDoubleAccessor (&CompositeEnergySource::m_panelEfficiency),
                   MakeDoubleChecker<double> (0.0, 1.0))
    .AddAttribute ("SolarConstantWm2",
                   "Solar constant (W/m^2).",
                   DoubleValue (1361.0),
                   MakeDoubleAccessor (&CompositeEnergySource::m_solarConstantWm2),
                   MakeDoubleChecker<double> (0.0))
    .AddAttribute ("HarvestIntervalSeconds",
                   "Numerical integration step for harvesting (s).",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&CompositeEnergySource::m_harvestIntervalSeconds),
                   MakeDoubleChecker<double> (1e-6))
    .AddAttribute ("SunlightSeconds",
                   "Duration of sunlight per cycle (s).",
                   DoubleValue (3900.0),
                   MakeDoubleAccessor (&CompositeEnergySource::m_sunlightSeconds),
                   MakeDoubleChecker<double> (0.0))
    .AddAttribute ("ShadowSeconds",
                   "Duration of umbra per cycle (s).",
                   DoubleValue (1800.0),
                   MakeDoubleAccessor (&CompositeEnergySource::m_shadowSeconds),
                   MakeDoubleChecker<double> (0.0));
  return tid;
}

CompositeEnergySource::CompositeEnergySource ()
  : m_solarPower (0.0),
    m_harvestStart (0.0),
    m_harvestEnd (0.0),
    m_useLeoCycle (true),
    m_panelAreaM2 (2.0),
    m_panelEfficiency (0.28),
    m_solarConstantWm2 (1361.0),
    m_harvestIntervalSeconds (1.0),
    m_sunlightSeconds (3900.0),
    m_shadowSeconds (1800.0),
    m_inSunlight (true)
{
}

CompositeEnergySource::~CompositeEnergySource ()
{
  if (m_harvestEvent.IsRunning ())
    {
      Simulator::Cancel (m_harvestEvent);
    }
  if (m_toggleEvent.IsRunning ())
    {
      Simulator::Cancel (m_toggleEvent);
    }
}

void
CompositeEnergySource::AddBattery (Ptr<LiIonEnergySource> battery)
{
  NS_LOG_FUNCTION (this << battery);
  m_battery = battery;
}

void
CompositeEnergySource::AddSolarPanel (double powerJoulePerSecond, double startTime, double endTime)
{
  NS_LOG_FUNCTION (this << powerJoulePerSecond << startTime << endTime);
  m_solarPower = powerJoulePerSecond;
  m_harvestStart = startTime;
  m_harvestEnd = endTime;

  // If LEO cycle is disabled, schedule explicit window harvesting; otherwise, LEO cycle will drive harvesting
  if (!m_useLeoCycle)
    {
      Simulator::Schedule (Seconds (m_harvestStart), &CompositeEnergySource::HarvestEnergy, this);
    }
}

void
CompositeEnergySource::ConfigureSolarHarvester (double panelAreaM2, double panelEfficiency, double solarConstantWm2)
{
  NS_LOG_FUNCTION (this << panelAreaM2 << panelEfficiency << solarConstantWm2);
  m_panelAreaM2 = panelAreaM2;
  m_panelEfficiency = panelEfficiency;
  m_solarConstantWm2 = solarConstantWm2;
}

double
CompositeEnergySource::GetRemainingEnergy () const
{
  NS_LOG_FUNCTION (this);
  if (m_battery)
    return m_battery->GetRemainingEnergy ();
  else
    return 0.0;
}

double
CompositeEnergySource::GetTotalEnergy () const
{
  NS_LOG_FUNCTION (this);
  if (m_battery)
    return m_battery->GetTotalEnergy ();
  else
    return 0.0;
}

double
CompositeEnergySource::GetSupplyVoltage () const
{
  NS_LOG_FUNCTION (this);
  if (m_battery)
    return m_battery->GetSupplyVoltage ();
  else
    return 0.0;
}

Ptr<LiIonEnergySource>
CompositeEnergySource::GetBattery () const
{
  return m_battery;
}

void
CompositeEnergySource::DoInitialize ()
{
  // If using LEO cycle, start it; otherwise rely on explicit AddSolarPanel window.
  if (m_useLeoCycle)
    {
      StartHarvestCycle ();
    }
  EnergySource::DoInitialize ();
}

void
CompositeEnergySource::StartHarvestCycle ()
{
  // Start in sunlight; schedule harvesting and toggle to shadow later
  m_inSunlight = true;
  // kick off harvesting loop immediately
  if (!m_harvestEvent.IsRunning ())
    {
      m_harvestEvent = Simulator::ScheduleNow (&CompositeEnergySource::HarvestEnergy, this);
    }
  // schedule toggle to shadow
  m_toggleEvent = Simulator::Schedule (Seconds (m_sunlightSeconds), &CompositeEnergySource::ToggleSunlight, this);
}

void
CompositeEnergySource::ToggleSunlight ()
{
  m_inSunlight = !m_inSunlight;
  double next = m_inSunlight ? m_sunlightSeconds : m_shadowSeconds;
  m_toggleEvent = Simulator::Schedule (Seconds (next), &CompositeEnergySource::ToggleSunlight, this);
}

void
CompositeEnergySource::HarvestEnergy ()
{
  NS_LOG_FUNCTION (this);
  double currentTime = Simulator::Now ().GetSeconds ();

  double dt = m_harvestIntervalSeconds;
  bool windowActive = (currentTime >= m_harvestStart && currentTime < m_harvestEnd);

  double harvestedJ = 0.0;
  if (m_useLeoCycle)
    {
      if (m_inSunlight)
        {
          // Compute instantaneous solar power from panel and efficiency
          double p = m_solarConstantWm2 * m_panelAreaM2 * m_panelEfficiency; // W == J/s
          harvestedJ = p * dt;
        }
    }
  else if (windowActive && m_solarPower > 0.0)
    {
      harvestedJ = m_solarPower * dt;
    }

  if (harvestedJ > 0.0 && m_battery)
    {
      TryAddEnergy (m_battery, harvestedJ);
      NS_LOG_INFO ("Harvested " << harvestedJ << " J at t=" << currentTime << "s");
    }

  // Continue loop if either LEO cycle is on or explicit window still active
  if (m_useLeoCycle || (windowActive && m_solarPower > 0.0))
    {
      m_harvestEvent = Simulator::Schedule (Seconds (dt), &CompositeEnergySource::HarvestEnergy, this);
    }
}

} // namespace ns3
