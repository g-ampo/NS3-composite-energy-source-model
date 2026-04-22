#ifndef NS3_COMPOSITE_ENERGY_SOURCE_H
#define NS3_COMPOSITE_ENERGY_SOURCE_H

#include "solar-harvester-device-model.h"
#include "solar-irradiance-model.h"

#include "ns3/event-id.h"
#include "ns3/li-ion-energy-source.h"

namespace ns3
{

/**
 * \ingroup energy
 * \brief Li-Ion battery with optional solar energy harvesting.
 *
 * Design
 *  - Inherits from ns3::LiIonEnergySource so that any existing
 *    DeviceEnergyModel can attach to this source without an adapter and
 *    so that the full Li-Ion discharge / Shepherd-voltage model is
 *    preserved verbatim.
 *  - Harvesting is expressed as a negative current reported by an
 *    internal SolarHarvesterDeviceModel. The Li-Ion integrator already
 *    sums device currents to compute net I*V*dt; a negative contribution
 *    therefore adds energy exactly, with no need to reach inside the
 *    private remaining-energy state of the base class.
 *
 * Harvesting modes
 *  - LEO cycle (UseLeoCycle=true, default): alternating
 *    SunlightSeconds / ShadowSeconds phases. Instantaneous solar input
 *    power P = SolarConstantWm2 * PanelAreaM2 * PanelEfficiency.
 *  - Fixed window (UseLeoCycle=false): constant P in [start, end)
 *    specified via AddSolarPanelWindow(P, start, end).
 *
 * Clamping
 *  - When the remaining energy reaches InitialEnergyJ (full charge) the
 *    harvester current is driven to zero. This avoids unphysical
 *    over-filling of the cell in either harvesting mode.
 */
class CompositeEnergySource : public LiIonEnergySource
{
  public:
    static TypeId GetTypeId();

    CompositeEnergySource();
    ~CompositeEnergySource() override;

    /**
     * \brief Configure a fixed harvesting window.
     *
     * Only effective when UseLeoCycle=false. The source injects energy at
     * constant power \p powerJoulePerSecond (W) during [startTime, endTime).
     */
    void AddSolarPanelWindow(double powerJoulePerSecond, double startTime, double endTime);

    /**
     * \brief Configure the LEO solar harvester from panel geometry.
     *
     * Instantaneous solar input power during sunlight is
     * P = solarConstantWm2 * panelAreaM2 * panelEfficiency.
     */
    void ConfigureSolarHarvester(double panelAreaM2,
                                 double panelEfficiency,
                                 double solarConstantWm2 = 1361.0);

    /** \return Total energy harvested since start of simulation, in Joules. */
    double GetTotalHarvestedEnergy() const;

    /** \return true if the current LEO phase is sunlight. Always true when
     *           using a fixed window (no phase concept). */
    bool IsInSunlight() const;

  protected:
    void DoInitialize() override;
    void DoDispose() override;

  private:
    /** Recompute the harvest current from current mode/phase and apply it
     *  to the internal harvester device model. Reschedules itself every
     *  HarvestIntervalSeconds. */
    void UpdateHarvestCurrent();

    /** Flip the LEO sunlight/shadow phase and reschedule the next toggle. */
    void ToggleSunlight();

    // Harvesting device model driven by this source (reports negative
    // current to the Li-Ion integrator).
    Ptr<SolarHarvesterDeviceModel> m_harvester;

    // Optional user-supplied irradiance model. When non-null, its
    // GetPowerDensityWm2(t) replaces the built-in LEO/window logic as the
    // sole source of harvesting power. PanelAreaM2 and PanelEfficiency
    // continue to be applied as multipliers in both paths.
    Ptr<SolarIrradianceModel> m_irradianceModel;

    // Fixed-window parameters
    double m_windowPowerW;
    double m_windowStart;
    double m_windowEnd;

    // LEO / harvester attributes
    bool m_useLeoCycle;
    double m_panelAreaM2;
    double m_panelEfficiency;
    double m_solarConstantWm2;
    double m_harvestIntervalSeconds;
    double m_sunlightSeconds;
    double m_shadowSeconds;
    double m_maxEnergyJ; // 0 => use GetInitialEnergy() as the cap
    bool m_inSunlight;

    EventId m_harvestEvent;
    EventId m_toggleEvent;
};

} // namespace ns3

#endif // NS3_COMPOSITE_ENERGY_SOURCE_H
