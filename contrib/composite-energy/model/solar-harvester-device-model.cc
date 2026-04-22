#include "solar-harvester-device-model.h"

#include "ns3/double.h"
#include "ns3/energy-source.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SolarHarvesterDeviceModel");
NS_OBJECT_ENSURE_REGISTERED(SolarHarvesterDeviceModel);

TypeId
SolarHarvesterDeviceModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SolarHarvesterDeviceModel")
                            .SetParent<DeviceEnergyModel>()
                            .SetGroupName("Energy")
                            .AddConstructor<SolarHarvesterDeviceModel>();
    return tid;
}

SolarHarvesterDeviceModel::SolarHarvesterDeviceModel()
    : m_source(nullptr),
      m_harvestCurrentA(0.0),
      m_totalHarvestedJ(0.0),
      m_lastUpdate(Seconds(0))
{
    NS_LOG_FUNCTION(this);
}

SolarHarvesterDeviceModel::~SolarHarvesterDeviceModel()
{
    NS_LOG_FUNCTION(this);
}

void
SolarHarvesterDeviceModel::SetEnergySource(Ptr<EnergySource> source)
{
    NS_LOG_FUNCTION(this << source);
    m_source = source;
    m_lastUpdate = Simulator::Now();
}

Ptr<EnergySource>
SolarHarvesterDeviceModel::GetEnergySource() const
{
    return m_source;
}

double
SolarHarvesterDeviceModel::GetTotalEnergyConsumption() const
{
    // Harvester consumes no energy; expose -totalHarvested so that the
    // standard ns-3 accounting (consumption is non-negative) remains
    // consistent: a negative number signals net energy injection.
    return -m_totalHarvestedJ;
}

void
SolarHarvesterDeviceModel::ChangeState(int /*newState*/)
{
    // Harvester is stateless from the energy-model perspective.
}

void
SolarHarvesterDeviceModel::HandleEnergyDepletion()
{
    NS_LOG_FUNCTION(this);
    // Battery flat: stop reporting charge current so the source does not
    // immediately re-hydrate itself from a depleted state via us.
    SetHarvestCurrentA(0.0);
}

void
SolarHarvesterDeviceModel::HandleEnergyRecharged()
{
    NS_LOG_FUNCTION(this);
}

void
SolarHarvesterDeviceModel::HandleEnergyChanged()
{
    // No-op: the harvester does not react to supply-voltage changes; it
    // lets the owning CompositeEnergySource drive current updates.
}

void
SolarHarvesterDeviceModel::SetHarvestCurrentA(double a)
{
    NS_LOG_FUNCTION(this << a);
    if (a < 0.0)
    {
        a = 0.0;
    }
    AccrueSinceLastUpdate();
    m_harvestCurrentA = a;
}

double
SolarHarvesterDeviceModel::GetHarvestCurrentA() const
{
    return m_harvestCurrentA;
}

double
SolarHarvesterDeviceModel::GetTotalHarvestedEnergy() const
{
    return m_totalHarvestedJ;
}

void
SolarHarvesterDeviceModel::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_source = nullptr;
    DeviceEnergyModel::DoDispose();
}

double
SolarHarvesterDeviceModel::DoGetCurrentA() const
{
    // Negative sign turns the reported current into an injection from the
    // source's point of view (its integrator subtracts I*V*dt).
    return -m_harvestCurrentA;
}

void
SolarHarvesterDeviceModel::AccrueSinceLastUpdate()
{
    Time now = Simulator::Now();
    double dt = (now - m_lastUpdate).GetSeconds();
    if (dt > 0.0 && m_harvestCurrentA > 0.0 && m_source)
    {
        double v = m_source->GetSupplyVoltage();
        m_totalHarvestedJ += m_harvestCurrentA * v * dt;
    }
    m_lastUpdate = now;
}

} // namespace ns3
