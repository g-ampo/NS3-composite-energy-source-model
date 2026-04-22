#ifndef NS3_SOLAR_HARVESTER_DEVICE_MODEL_H
#define NS3_SOLAR_HARVESTER_DEVICE_MODEL_H

#include "ns3/device-energy-model.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"

namespace ns3
{

class EnergySource;

/**
 * \ingroup energy
 * \brief DeviceEnergyModel that injects solar-harvested energy into an
 *        ns-3 EnergySource by reporting a *negative* current draw.
 *
 * ns-3 energy sources (LiIonEnergySource in particular) sum the currents
 * reported by all attached DeviceEnergyModel instances to compute the net
 * current leaving the source each update period. Reporting a negative
 * current is therefore the idiomatic way for a subclass-independent
 * harvester to feed energy into the source without touching the private
 * remaining-energy state of the battery class. Energy accounting is exact:
 * the source integrates I * V * dt with the same supply voltage used to
 * derive I = P / V, so the injected energy equals P * dt irrespective of
 * any voltage drift inside the battery model.
 *
 * The public API is intentionally minimal: one setter, SetHarvestCurrentA,
 * which accepts the (non-negative) magnitude of the charge current the
 * caller wants to inject. The model reports -magnitude to the source.
 */
class SolarHarvesterDeviceModel : public DeviceEnergyModel
{
  public:
    static TypeId GetTypeId();

    SolarHarvesterDeviceModel();
    ~SolarHarvesterDeviceModel() override;

    // DeviceEnergyModel API
    void SetEnergySource(Ptr<EnergySource> source) override;
    Ptr<EnergySource> GetEnergySource() const;
    double GetTotalEnergyConsumption() const override;
    void ChangeState(int newState) override;
    void HandleEnergyDepletion() override;
    void HandleEnergyRecharged() override;
    void HandleEnergyChanged() override;

    /**
     * \param a Non-negative magnitude of the solar charge current in A.
     *          The model will report -a to the attached EnergySource.
     */
    void SetHarvestCurrentA(double a);

    /** \return Non-negative magnitude of the current harvest current. */
    double GetHarvestCurrentA() const;

    /** \return Total harvested energy in Joules since construction. */
    double GetTotalHarvestedEnergy() const;

  protected:
    void DoDispose() override;
    double DoGetCurrentA() const override;

  private:
    /**
     * Integrate P*dt since the last update into m_totalHarvestedJ using the
     * current supply voltage and the previously-set harvest current. Called
     * before any transition that changes the reported current so that the
     * bookkeeping total reflects the full rectangular integral.
     */
    void AccrueSinceLastUpdate();

    Ptr<EnergySource> m_source;
    double m_harvestCurrentA;
    double m_totalHarvestedJ;
    Time m_lastUpdate;
};

} // namespace ns3

#endif // NS3_SOLAR_HARVESTER_DEVICE_MODEL_H
