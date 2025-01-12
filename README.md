# ns-3 Composite Energy Model for UAVs and Satellites

## Overview

This project implements a **Composite Energy Model** within the **ns-3** simulation framework, tailored for **Unmanned Aerial Vehicles (UAVs)** and **Satellites**. The model allows UAVs to operate solely on battery power, while Satellites can also harvest energy from solar panels, simulating a **Low Earth Orbit (LEO)** topology.

## Features

- **Battery-Powered UAVs:** Simulates energy consumption during transmission, reception, and idle states.
- **Solar-Harvesting Satellites:** Combines battery power with periodic energy replenishment from solar panels.
- **Modular Design:** Easily extendable to accommodate additional energy models or node types.
- **Comprehensive Examples:** Includes simulation scripts demonstrating usage scenarios.

## Installation

### Prerequisites

- **ns-3:** Version 3.30 or later.
- **C++ Compiler:** Compatible with ns-3 requirements.
- **Build Tools:** `waf`, `g++`, etc.

This project is licensed under the MIT License.
