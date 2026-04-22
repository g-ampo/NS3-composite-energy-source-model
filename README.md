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
   - `HarvestIntervalSeconds` (double): integration step for harvesting in seconds
   - `SunlightSeconds` (double): sunlight duration per cycle (s)
   - `ShadowSeconds` (double): shadow duration per cycle (s)

   Alternatively, a fixed harvesting window can be set via `AddSolarPanelWindow(powerJps, start, end)` with `UseLeoCycle=false`.

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
                                  HarvestIntervalSeconds
                    - Methods: ConfigureSolarHarvester(), AddSolarPanelWindow()
                    - Behavior: ChangeRemainingEnergy(+J) during sunlight/window
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
| **LiIonEnergySource**        | `ns3::EnergySource`      | Represents a lithium-ion battery energy source, managing energy storage and consumption for UAVs and Satellites. |
| **SimpleDeviceEnergyModel**  | `ns3::DeviceEnergyModel` | Simulates energy consumption for device activities such as transmission, reception, and idle states.      |
| **CompositeEnergySourceTest**| `ns3::TestCase`          | Contains unit tests to verify the functionality and reliability of the `CompositeEnergySource` class.     |

### Detailed Class Descriptions

#### CompositeEnergySource

- **Header File:** `contrib/composite-energy/model/composite-energy-source.h`
- **Source File:** `contrib/composite-energy/model/composite-energy-source.cc`
- **Inheritance:** Inherits from `ns3::LiIonEnergySource`.
- **Description:** 
  The `CompositeEnergySource` class is a Li-Ion energy source that adds solar harvesting. Satellites can replenish energy via LEO sunlight/shadow cycles or a fixed window. Discharge/voltage/capacity remain handled by the base Li-Ion implementation, while harvesting injects energy using `ChangeRemainingEnergy(+J)`.

#### LiIonEnergySource

- **Header File:** `src/energy/li-ion-energy-source.h` *(Assuming it's part of ns-3's core or appropriately included)*
- **Source File:** `src/energy/li-ion-energy-source.cc`
- **Inheritance:** Inherits from `ns3::EnergySource`.
- **Description:**
  The `LiIonEnergySource` class models a lithium-ion battery, managing its energy storage, voltage levels, and energy consumption based on device activities. It tracks parameters such as initial energy, capacity, cell voltage, internal resistance, and threshold voltage, providing realistic battery behavior within simulations.

#### SimpleDeviceEnergyModel

- **Header File:** `src/energy/simple-device-energy-model.h` *(Assuming it's part of ns-3's core or appropriately included)*
- **Source File:** `src/energy/simple-device-energy-model.cc`
- **Inheritance:** Inherits from `ns3::DeviceEnergyModel`.
- **Description:**
  The `SimpleDeviceEnergyModel` class simulates the energy consumption of device activities, including transmission, reception, and idle states. It allows setting current draws corresponding to different operational modes, thereby affecting the energy source's remaining energy. This model facilitates realistic energy usage patterns for both UAVs and Satellites in the simulation.

#### CompositeEnergySourceTest

- **Source File:** `contrib/composite-energy/test/composite-energy-source-test-suite.cc`
- **Inheritance:** Inherits from `ns3::TestCase`.
- **Description:**
  The `CompositeEnergySourceTest` class contains unit tests designed to verify the correctness and reliability of the `CompositeEnergySource` implementation. It tests functionalities such as energy harvesting, correct energy addition over time, and proper integration with the battery component.


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
