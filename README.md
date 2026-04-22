# NS-3 Composite Energy Model for UAVs and Satellites

## Overview

This project implements a **Composite Energy Model** within the **ns-3** simulation framework, tailored for **Unmanned Aerial Vehicles (UAVs)** and **Satellites**. The model allows UAVs to operate solely on battery power, while Satellites can also harvest energy from solar panels, simulating a **Low Earth Orbit (LEO)** topology.

## Features

- **Battery-Powered UAVs:** Simulates energy consumption during transmission, reception, and idle states.
- **Solar-Harvesting Satellites:** Combines battery power with periodic energy replenishment from solar panels.
- **Modular Design:** Easily extendable to accommodate additional energy models or node types.
- **Comprehensive Examples:** Includes simulation scripts demonstrating usage scenarios.
- **Unit Testing:** For reliability and correctness of energy models through automated tests.

## Installation

### Prerequisites

- **ns-3:** Version 3.36 or later (CMake build). Versions 3.30–3.35 supported via legacy `wscript` (waf).
- **C++ Compiler:** C++17 (as required by the target ns-3 release).
- **Build Tools:** CMake ≥ 3.13 and the `./ns3` wrapper (or `waf` on older releases).
- **Git:** For cloning the repository.

### Steps

1. **Clone the repository:**

   ```bash
   git clone https://github.com/g-ampo/NS3-composite-energy-source-model.git
   ```

2. **Drop the module into your ns-3 tree as a contrib module:**

   ```bash
   cp -r NS3-composite-energy-source-model/contrib/composite-energy <path-to-ns-3-dev>/contrib/
   ```

   The module is self-contained — nothing else needs to be copied, and no
   existing ns-3 file needs to be edited.

3. **Configure and build (ns-3 ≥ 3.36, CMake):**

   ```bash
   cd <path-to-ns-3-dev>
   ./ns3 configure --enable-examples --enable-tests
   ./ns3 build
   ```

   Legacy waf (ns-3 ≤ 3.35):

   ```bash
   ./waf configure --enable-examples --enable-tests
   ./waf build
   ```

4. **Attributes (LEO cycle):**

   The `ns3::CompositeEnergySource` exposes attributes to model a LEO sunlight/umbra cycle:

   - `UseLeoCycle` (bool, default true): enable sunlight/shadow cycling
   - `PanelAreaM2` (double): solar panel area in m^2
   - `PanelEfficiency` (double): panel efficiency [0..1]
   - `SolarConstantWm2` (double): solar constant (default 1361 W/m^2)
   - `HarvestIntervalSeconds` (double): period at which the harvester re-applies its current to the source (s)
   - `SunlightSeconds` (double): sunlight duration per cycle (s)
   - `ShadowSeconds` (double): shadow duration per cycle (s)
   - `MaxEnergyJ` (double): upper bound on remaining energy; harvesting stops at this cap. A value of `0` (default) means use `InitialEnergyJ` as the cap, which is appropriate when `InitialEnergyJ` already represents a fully-charged cell. Set this when you initialise the battery partially discharged and want it to charge back up during sunlight.

   Alternatively, a fixed harvesting window can be set via `AddSolarPanelWindow(powerW, start, end)` with `UseLeoCycle=false`.

5. **Run the example simulation:**

   ```bash
   ./ns3 run composite-energy-model-example
   ```

   Legacy waf:

   ```bash
   ./waf --run composite-energy-model-example
   ```

### Class Diagram

```
ns3::EnergySource
    ^
    |
    +-- ns3::LiIonEnergySource
            ^
            |
            +-- ns3::CompositeEnergySource
                    - Attributes: UseLeoCycle, PanelAreaM2, PanelEfficiency,
                                  SolarConstantWm2, SunlightSeconds, ShadowSeconds,
                                  HarvestIntervalSeconds, MaxEnergyJ
                    - Methods: ConfigureSolarHarvester(), AddSolarPanelWindow(),
                               GetTotalHarvestedEnergy(), IsInSunlight()
                    - Owns: ns3::SolarHarvesterDeviceModel (attached as a
                            DeviceEnergyModel that reports negative current
                            to feed harvested energy through the Li-Ion
                            integrator without touching its private state).

ns3::DeviceEnergyModel
    ^
    |
    +-- ns3::SolarHarvesterDeviceModel
            - Reports -I = -P/V to the owning EnergySource.
```

### Example snippet

```cpp
Ptr<CompositeEnergySource> ces = CreateObject<CompositeEnergySource>();
ces->SetAttribute("UseLeoCycle", BooleanValue(true));
ces->SetAttribute("PanelAreaM2", DoubleValue(2.0));
ces->SetAttribute("PanelEfficiency", DoubleValue(0.28));
ces->SetAttribute("SunlightSeconds", DoubleValue(3900.0));
ces->SetAttribute("ShadowSeconds", DoubleValue(1800.0));
```

## Class Descriptions

This section provides an overview of each class within the project, detailing their inheritance from ns-3's core classes and their functionalities.

| **Class Name**               | **Inherits From**        | **Description**                                                                                          |
|------------------------------|--------------------------|----------------------------------------------------------------------------------------------------------|
| **CompositeEnergySource**    | `ns3::LiIonEnergySource` | Li-Ion battery with solar harvesting (LEO cycle or fixed window). DeviceEnergyModel attaches directly. |
| **SolarHarvesterDeviceModel**| `ns3::DeviceEnergyModel` | Internal helper that reports a negative current to the source so harvested energy flows through the standard Li-Ion integrator. |
| **LiIonEnergySource**        | `ns3::EnergySource`      | Represents a lithium-ion battery energy source, managing energy storage and consumption for UAVs and Satellites. |
| **SimpleDeviceEnergyModel**  | `ns3::DeviceEnergyModel` | Simulates energy consumption for device activities such as transmission, reception, and idle states.      |
| **CompositeEnergySourceTestSuite** | `ns3::TestSuite`   | Unit tests verifying fixed-window and LEO-cycle harvesting behaviour of `CompositeEnergySource`. |

### Detailed Class Descriptions

#### CompositeEnergySource

- **Header File:** `contrib/composite-energy/model/composite-energy-source.h`
- **Source File:** `contrib/composite-energy/model/composite-energy-source.cc`
- **Inheritance:** Inherits from `ns3::LiIonEnergySource`.
- **Description:** 
  The `CompositeEnergySource` class is a Li-Ion energy source that adds solar harvesting. Satellites can replenish energy via LEO sunlight/shadow cycles or a fixed harvesting window. Discharge, voltage, and capacity remain handled entirely by the base Li-Ion implementation. Harvesting is realised by an internal `SolarHarvesterDeviceModel` that reports a negative current `-P/V` to the source: the Li-Ion integrator then sums it with the real device currents, so `remaining_energy` grows by exactly `P * dt` per update period, with no access to the base class's private state.

#### SolarHarvesterDeviceModel

- **Header File:** `contrib/composite-energy/model/solar-harvester-device-model.h`
- **Source File:** `contrib/composite-energy/model/solar-harvester-device-model.cc`
- **Inheritance:** Inherits from `ns3::DeviceEnergyModel`.
- **Description:**
  A minimal `DeviceEnergyModel` used internally by `CompositeEnergySource` to feed harvested energy through the standard ns-3 current-summation path. When a harvest current `I_h` is set via `SetHarvestCurrentA(I_h)`, the model reports `-I_h` to the source, turning what the source sees as a "consumer" into a net energy injector. Tracks total harvested energy in Joules.

#### LiIonEnergySource

- **Header File:** `src/energy/model/li-ion-energy-source.h` (shipped with ns-3)
- **Source File:** `src/energy/model/li-ion-energy-source.cc` (shipped with ns-3)
- **Inheritance:** Inherits from `ns3::EnergySource`.
- **Description:**
  The `LiIonEnergySource` class models a lithium-ion battery, managing its energy storage, voltage levels, and energy consumption based on device activities. It tracks parameters such as initial energy, capacity, cell voltage, internal resistance, and threshold voltage, providing realistic battery behavior within simulations.

#### SimpleDeviceEnergyModel

- **Header File:** `src/energy/model/simple-device-energy-model.h` (shipped with ns-3)
- **Source File:** `src/energy/model/simple-device-energy-model.cc` (shipped with ns-3)
- **Inheritance:** Inherits from `ns3::DeviceEnergyModel`.
- **Description:**
  The `SimpleDeviceEnergyModel` class simulates the energy consumption of device activities, including transmission, reception, and idle states. It allows setting current draws corresponding to different operational modes, thereby affecting the energy source's remaining energy. This model facilitates realistic energy usage patterns for both UAVs and Satellites in the simulation.

#### CompositeEnergySourceTestSuite

- **Source File:** `contrib/composite-energy/test/composite-energy-source-test-suite.cc`
- **Inheritance:** `ns3::TestSuite` with two `ns3::TestCase` entries: `CompositeEnergySourceTest` (fixed window) and `CompositeEnergySourceLeoCycleTest` (sunlight/shadow cycle).
- **Description:**
  Verifies that the source correctly injects the expected amount of energy during a fixed harvesting window, and that during a LEO cycle only sunlight phases contribute to harvested energy (shadow phases must not).


---

## Additional Information

### Running Tests

To execute the unit tests and ensure that all components are functioning correctly:

```bash
./ns3 run "test-runner --suite=composite-energy-source"
```

Legacy waf:

```bash
./waf --run "test-runner --suite=composite-energy-source"
```
---

## Contact

For questions, suggestions, or support, please open an issue in the this repo or contact me diretly.

## Cite as

IEEE:

```
G. Amponis, “NS-3 Composite Energy Source Model,” 2025, GitHub. Available: https://github.com/g-ampo/NS3-composite-energy-source-model
```

BibTeX:

```bibtex
@misc{Amponis2025NS3CompositeEnergy,
  title        = {NS-3 Composite Energy Source Model},
  author       = {Amponis, George},
  year         = {2025},
  howpublished = {Software},
  note         = {Implements a Li-Ion energy source with solar harvesting (LEO cycle and fixed window) for ns-3},
  url          = {https://github.com/g-ampo/NS3-composite-energy-source-model}
}
```
