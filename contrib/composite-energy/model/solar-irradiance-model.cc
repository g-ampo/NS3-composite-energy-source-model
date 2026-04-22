#include "solar-irradiance-model.h"

#include "ns3/double.h"
#include "ns3/log.h"

#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SolarIrradianceModel");

// -------------------------------------------------------------------------
// SolarIrradianceModel (abstract base)
// -------------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED(SolarIrradianceModel);

TypeId
SolarIrradianceModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SolarIrradianceModel").SetParent<Object>().SetGroupName("Energy");
    return tid;
}

SolarIrradianceModel::~SolarIrradianceModel() = default;

// -------------------------------------------------------------------------
// ConstantSolarIrradianceModel
// -------------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED(ConstantSolarIrradianceModel);

TypeId
ConstantSolarIrradianceModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ConstantSolarIrradianceModel")
                            .SetParent<SolarIrradianceModel>()
                            .SetGroupName("Energy")
                            .AddConstructor<ConstantSolarIrradianceModel>()
                            .AddAttribute("PowerDensityWm2",
                                          "Constant solar power density (W/m^2).",
                                          DoubleValue(1361.0),
                                          MakeDoubleAccessor(&ConstantSolarIrradianceModel::m_wm2),
                                          MakeDoubleChecker<double>(0.0));
    return tid;
}

ConstantSolarIrradianceModel::ConstantSolarIrradianceModel()
    : m_wm2(1361.0)
{
}

ConstantSolarIrradianceModel::~ConstantSolarIrradianceModel() = default;

double
ConstantSolarIrradianceModel::GetPowerDensityWm2(Time /*t*/) const
{
    return m_wm2;
}

// -------------------------------------------------------------------------
// LeoCycleSolarIrradianceModel
// -------------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED(LeoCycleSolarIrradianceModel);

TypeId
LeoCycleSolarIrradianceModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::LeoCycleSolarIrradianceModel")
            .SetParent<SolarIrradianceModel>()
            .SetGroupName("Energy")
            .AddConstructor<LeoCycleSolarIrradianceModel>()
            .AddAttribute("PeakWm2",
                          "Peak solar power density during sunlight (W/m^2).",
                          DoubleValue(1361.0),
                          MakeDoubleAccessor(&LeoCycleSolarIrradianceModel::m_peakWm2),
                          MakeDoubleChecker<double>(0.0))
            .AddAttribute("SunlightSeconds",
                          "Sunlight duration per cycle (s).",
                          DoubleValue(3900.0),
                          MakeDoubleAccessor(&LeoCycleSolarIrradianceModel::m_sunlightSeconds),
                          MakeDoubleChecker<double>(0.0))
            .AddAttribute("ShadowSeconds",
                          "Shadow (umbra) duration per cycle (s).",
                          DoubleValue(1800.0),
                          MakeDoubleAccessor(&LeoCycleSolarIrradianceModel::m_shadowSeconds),
                          MakeDoubleChecker<double>(0.0))
            .AddAttribute("PhaseSeconds",
                          "Phase offset into the cycle at simulation start (s). 0 means "
                          "start at the beginning of the sunlight phase.",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&LeoCycleSolarIrradianceModel::m_phaseSeconds),
                          MakeDoubleChecker<double>(0.0));
    return tid;
}

LeoCycleSolarIrradianceModel::LeoCycleSolarIrradianceModel()
    : m_peakWm2(1361.0),
      m_sunlightSeconds(3900.0),
      m_shadowSeconds(1800.0),
      m_phaseSeconds(0.0)
{
}

LeoCycleSolarIrradianceModel::~LeoCycleSolarIrradianceModel() = default;

double
LeoCycleSolarIrradianceModel::GetPowerDensityWm2(Time t) const
{
    double period = m_sunlightSeconds + m_shadowSeconds;
    if (period <= 0.0)
    {
        return m_sunlightSeconds > 0.0 ? m_peakWm2 : 0.0;
    }
    double phase = std::fmod(t.GetSeconds() + m_phaseSeconds, period);
    if (phase < 0.0)
    {
        phase += period;
    }
    return (phase < m_sunlightSeconds) ? m_peakWm2 : 0.0;
}

// -------------------------------------------------------------------------
// CallbackSolarIrradianceModel
// -------------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED(CallbackSolarIrradianceModel);

TypeId
CallbackSolarIrradianceModel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::CallbackSolarIrradianceModel")
                            .SetParent<SolarIrradianceModel>()
                            .SetGroupName("Energy")
                            .AddConstructor<CallbackSolarIrradianceModel>();
    return tid;
}

CallbackSolarIrradianceModel::CallbackSolarIrradianceModel() = default;
CallbackSolarIrradianceModel::~CallbackSolarIrradianceModel() = default;

void
CallbackSolarIrradianceModel::SetCallback(IrradianceCallback cb)
{
    m_cb = cb;
}

double
CallbackSolarIrradianceModel::GetPowerDensityWm2(Time t) const
{
    if (m_cb.IsNull())
    {
        return 0.0;
    }
    return m_cb(t);
}

} // namespace ns3
