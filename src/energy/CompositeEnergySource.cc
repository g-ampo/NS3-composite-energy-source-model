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
    .AddConstructor<CompositeEnergySource> ();
  return tid;
}

CompositeEnergySource::CompositeEnergySource ()
  : m_solarPower (0.0),
    m_harvestStart (0.0),
    m_harvestEnd (0.0)
{
}

CompositeEnergySource::~CompositeEnergySource ()
{
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

  // Schedule the start of energy harvesting
  Simulator::Schedule (Seconds (m_harvestStart), &CompositeEnergySource::HarvestEnergy, this);
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
CompositeEnergySource::HarvestEnergy ()
{
  NS_LOG_FUNCTION (this);
  double currentTime = Simulator::Now ().GetSeconds ();
  
  if (currentTime >= m_harvestStart && currentTime < m_harvestEnd)
    {
      // Add harvested energy to the battery
      if (m_battery)
        {
          m_battery->AddEnergy (m_solarPower); // Assuming 1-second interval
          NS_LOG_INFO ("Harvested " << m_solarPower << " J at " << currentTime << "s");
        }
      
      // Schedule next harvesting event after 1 second
      Simulator::Schedule (Seconds (1.0), &CompositeEnergySource::HarvestEnergy, this);
    }
  else
    {
      NS_LOG_INFO ("Energy harvesting ended at " << currentTime << "s");
      // Harvesting period ended; do nothing or handle multiple periods if needed
    }
}

} // namespace ns3
