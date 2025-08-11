// CompositeEnergySource.h

#ifndef COMPOSITE_ENERGY_SOURCE_H
#define COMPOSITE_ENERGY_SOURCE_H

#include "ns3/li-ion-energy-source.h"
#include "ns3/event-id.h"
#include "ns3/simulator.h"
#include "ns3/double.h"
#include "ns3/boolean.h"
#include "ns3/nstime.h"

namespace ns3 {

/**
 * \brief CompositeEnergySource: Li-Ion battery with solar harvesting staged for LEO.
 *
 * Design intent
 *  - Inherit from ns3::LiIonEnergySource so that existing ns-3 DeviceEnergyModel
 *    classes can attach directly to this source without any adapter.
 *  - Add attribute-driven solar harvesting that operates either via a repeating
 *    sunlight/shadow (LEO) cycle or via a fixed-time harvesting window.
 *  - Leverage the Li-Ion model for discharge dynamics, voltage, capacity, and
 *    current/energy accounting, while this subclass only injects harvested energy.
 *
 * Usage
 *  - Create a CompositeEnergySource, set Li-Ion attributes (InitialEnergyJ, CapacityJ,
 *    voltages, internal resistance, etc.), then set harvesting attributes.
 *  - Attach a DeviceEnergyModel (e.g., SimpleDeviceEnergyModel) directly to this source
 *    with SetEnergySource(this) and install it on the node.
 *
 * Notes
 *  - Harvesting adds energy using ChangeRemainingEnergy(+J), while consumption is
 *    handled by the base Li-Ion class (invoked by DeviceEnergyModel execution).
 *  - If UseLeoCycle is true, the fixed AddSolarPanelWindow() is ignored.
 */
class CompositeEnergySource : public LiIonEnergySource
{
public:
  static TypeId GetTypeId (void);
  
  CompositeEnergySource ();
  virtual ~CompositeEnergySource ();

  /**
   * \brief Configure a fixed harvesting window (ignored when UseLeoCycle=true).
   *        The source will add energy at a constant power (J/s) in [start,end).
   */
  void AddSolarPanelWindow (double powerJoulePerSecond, double startTime, double endTime);

  /**
   * \brief Configure a solar harvester using panel area and efficiency.
   *        Instantaneous solar input power (J/s) = SolarConstantWm2 * PanelAreaM2 * PanelEfficiency.
   */
  void ConfigureSolarHarvester (double panelAreaM2, double panelEfficiency, double solarConstantWm2 = 1361.0);
  
  // Li-Ion energy accounting and voltage are handled by the base class.

private:
  // ns-3 lifecycle
  void DoInitialize () override;

  // Harvesting loop and cycle management
  void HarvestEnergy ();
  void StartHarvestCycle ();
  void ToggleSunlight (); // switch sunlight/shadow in LEO cycle

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
