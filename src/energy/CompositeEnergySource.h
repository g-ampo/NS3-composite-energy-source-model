// CompositeEnergySource.h

#ifndef COMPOSITE_ENERGY_SOURCE_H
#define COMPOSITE_ENERGY_SOURCE_H

#include "ns3/energy-source.h"
#include "ns3/energy-source-container.h"
#include "ns3/li-ion-energy-source.h"
#include "ns3/event-id.h"
#include "ns3/simulator.h"

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
  // Function to handle energy harvesting
  void HarvestEnergy ();

  Ptr<LiIonEnergySource> m_battery;    ///< Battery component
  double m_solarPower;                  ///< Energy harvested per second (J/s)
  EventId m_harvestEvent;               ///< Event ID for harvesting
  double m_harvestStart;                ///< Harvesting start time (seconds)
  double m_harvestEnd;                  ///< Harvesting end time (seconds)
};

} // namespace ns3

#endif // COMPOSITE_ENERGY_SOURCE_H
