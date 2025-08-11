// CompositeEnergySource.h

#ifndef COMPOSITE_ENERGY_SOURCE_H
#define COMPOSITE_ENERGY_SOURCE_H

#include "ns3/energy-source.h"
#include "ns3/energy-source-container.h"
#include "ns3/li-ion-energy-source.h"
#include "ns3/event-id.h"
#include "ns3/simulator.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/nstime.h"

namespace ns3 {

/**
 * \brief CompositeEnergySource combines a LiIonEnergySource with energy harvesting capabilities.
 *
 * This class allows nodes (e.g., Satellites) to have a battery and periodically harvest energy
 * from renewable sources like solar panels.
 */
class CompositeEnergySource : public EnergySource
{
public:
  static TypeId GetTypeId (void);
  
  CompositeEnergySource ();
  virtual ~CompositeEnergySource ();

  /**
   * \brief Add a battery to the composite energy source.
   * \param battery Ptr to the LiIonEnergySource.
   */
  void AddBattery (Ptr<LiIonEnergySource> battery);

  /**
   * \brief Add a solar panel energy harvesting mechanism.
   * \param powerJoulePerSecond Energy harvested per second (J/s).
   * \param startTime Start time for energy harvesting (seconds).
   * \param endTime End time for energy harvesting (seconds).
   */
  void AddSolarPanel (double powerJoulePerSecond, double startTime, double endTime);

  /**
   * \brief Configure a solar harvester using panel area and efficiency.
   * Solar input power (J/s) = solarConstantWm2 * panelAreaM2 * panelEfficiency.
   */
  void ConfigureSolarHarvester (double panelAreaM2, double panelEfficiency, double solarConstantWm2 = 1361.0);
  
  // Override methods to provide energy information
  virtual double GetRemainingEnergy () const override;
  virtual double GetTotalEnergy () const override;
  virtual double GetSupplyVoltage () const override;

  /**
   * \brief Get the battery component.
   * \return Ptr to the LiIonEnergySource.
   */
  Ptr<LiIonEnergySource> GetBattery () const;

private:
  // ns-3 lifecycle
  void DoInitialize () override;

  // Harvesting loop and cycle management
  void HarvestEnergy ();
  void StartHarvestCycle ();
  void ToggleSunlight (); // switch sunlight/shadow in LEO cycle

  // Helper to add energy to the underlying battery regardless of API naming
  template <typename T>
  static auto TryAddEnergyImpl (T* batt, double j, int) -> decltype(batt->AddEnergy(j), void())
  {
    batt->AddEnergy (j);
  }
  template <typename T>
  static auto TryAddEnergyImpl (T* batt, double j, long) -> decltype(batt->ChangeRemainingEnergy(j), void())
  {
    batt->ChangeRemainingEnergy (j);
  }
  static void TryAddEnergy (Ptr<LiIonEnergySource> batt, double j)
  {
    if (!batt) return;
    TryAddEnergyImpl (PeekPointer(batt), j, 0);
  }

  Ptr<LiIonEnergySource> m_battery;    ///< Battery component
  double m_solarPower;                  ///< Explicit harvested power (J/s)
  EventId m_harvestEvent;               ///< Event ID for harvesting
  double m_harvestStart;                ///< Harvest start time (s) for explicit window
  double m_harvestEnd;                  ///< Harvest end time (s) for explicit window

  // Attribute-driven LEO cycle and harvester parameters
  bool   m_useLeoCycle;                 ///< Use repeating sunlight/shadow cycle
  double m_panelAreaM2;                 ///< Solar panel area (m^2)
  double m_panelEfficiency;             ///< Panel efficiency (0..1)
  double m_solarConstantWm2;            ///< Solar constant (W/m^2)
  double m_harvestIntervalSeconds;      ///< Integration step (s)
  double m_sunlightSeconds;             ///< Sunlight duration per orbit segment (s)
  double m_shadowSeconds;               ///< Shadow (umbra) duration (s)
  bool   m_inSunlight;                  ///< Current cycle state
  EventId m_toggleEvent;                ///< Event to toggle sunlight/shadow
};

} // namespace ns3

#endif // COMPOSITE_ENERGY_SOURCE_H
