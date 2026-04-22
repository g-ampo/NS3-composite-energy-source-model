#ifndef NS3_SOLAR_IRRADIANCE_MODEL_H
#define NS3_SOLAR_IRRADIANCE_MODEL_H

#include "ns3/callback.h"
#include "ns3/nstime.h"
#include "ns3/object.h"

namespace ns3
{

/**
 * \ingroup composite-energy
 * \brief Abstract source of instantaneous solar power density (W/m^2)
 *        used to drive harvesting in CompositeEnergySource.
 *
 * Irradiance is reported as power per unit area at the panel. The
 * consuming CompositeEnergySource multiplies by PanelAreaM2 and
 * PanelEfficiency to obtain the net harvested electrical power in Watts.
 *
 * Three ready-to-use implementations ship with this module:
 *  - \c ConstantSolarIrradianceModel     — time-invariant irradiance.
 *  - \c LeoCycleSolarIrradianceModel     — periodic sunlight/shadow cycle.
 *  - \c CallbackSolarIrradianceModel     — arbitrary user-supplied
 *                                          function of simulation time,
 *                                          suitable for orbital propagation
 *                                          outputs, panel-pointing losses,
 *                                          eclipse geometry, atmospheric
 *                                          attenuation, or trace-driven
 *                                          irradiance profiles.
 *
 * Derived classes implementing their own physics simply override
 * \c GetPowerDensityWm2.
 */
class SolarIrradianceModel : public Object
{
  public:
    static TypeId GetTypeId();
    ~SolarIrradianceModel() override;

    /** \return Instantaneous solar power density at the panel, in W/m^2,
     *          at the given simulation time. */
    virtual double GetPowerDensityWm2(Time t) const = 0;
};

/**
 * \ingroup composite-energy
 * \brief Constant irradiance. Good default and useful for smoke tests.
 */
class ConstantSolarIrradianceModel : public SolarIrradianceModel
{
  public:
    static TypeId GetTypeId();
    ConstantSolarIrradianceModel();
    ~ConstantSolarIrradianceModel() override;

    double GetPowerDensityWm2(Time t) const override;

  private:
    double m_wm2;
};

/**
 * \ingroup composite-energy
 * \brief Periodic sunlight/shadow (LEO) irradiance.
 *
 * The cycle length is \c SunlightSeconds + \c ShadowSeconds. During the
 * first \c SunlightSeconds of each cycle the output is \c PeakWm2; during
 * the remaining \c ShadowSeconds it is zero. \c PhaseSeconds shifts the
 * cycle by a constant amount, letting the user start in a different
 * phase of the orbit.
 */
class LeoCycleSolarIrradianceModel : public SolarIrradianceModel
{
  public:
    static TypeId GetTypeId();
    LeoCycleSolarIrradianceModel();
    ~LeoCycleSolarIrradianceModel() override;

    double GetPowerDensityWm2(Time t) const override;

  private:
    double m_peakWm2;
    double m_sunlightSeconds;
    double m_shadowSeconds;
    double m_phaseSeconds;
};

/**
 * \ingroup composite-energy
 * \brief Delegates to a user-supplied callback. The one-arg callback
 *        receives the current simulation \c Time and returns the
 *        irradiance in W/m^2.
 *
 * Use this to integrate an external orbit propagator, a trace-driven
 * profile (e.g. a CSV of W/m^2 vs. time), a pointing-error model, or
 * any other user code that computes irradiance as a function of time.
 */
class CallbackSolarIrradianceModel : public SolarIrradianceModel
{
  public:
    using IrradianceCallback = Callback<double, Time>;

    static TypeId GetTypeId();
    CallbackSolarIrradianceModel();
    ~CallbackSolarIrradianceModel() override;

    /** Install the irradiance function. Replaces any previous callback. */
    void SetCallback(IrradianceCallback cb);

    double GetPowerDensityWm2(Time t) const override;

  private:
    IrradianceCallback m_cb;
};

} // namespace ns3

#endif // NS3_SOLAR_IRRADIANCE_MODEL_H
