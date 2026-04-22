.. include:: replace.txt

Composite Energy
----------------

.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

The |ns3| ``composite-energy`` module extends the stock
``LiIonEnergySource`` with attribute-driven solar harvesting targeted
at LEO satellites and UAV/HAP scenarios. It is a drop-in ``contrib``
module: no in-tree |ns3| file needs to be edited. Any existing
``DeviceEnergyModel`` (``SimpleDeviceEnergyModel``,
``WifiRadioEnergyModel``, ...) attaches directly to the composite
source because it is a ``LiIonEnergySource`` subclass.

Model Description
*****************

The source code lives in ``contrib/composite-energy/model``.

``CompositeEnergySource`` inherits from ``LiIonEnergySource``. The full
Li-Ion discharge/voltage/current model of the parent class is used as
is; harvesting is layered on top by a hidden
``SolarHarvesterDeviceModel`` that the composite source attaches to
itself during ``DoInitialize``. The harvester reports the idiomatic
``DeviceEnergyModel`` current ``I = -P / V`` back to the source, so the
Li-Ion integrator's existing ``sum(I_k) * V * dt`` step adds ``P * dt``
of energy per update period without any access to the parent class's
private remaining-energy state. Supply voltage is used consistently to
derive the injected current, so the book-keeping is exact up to the
usual Shepherd-voltage drift during charging.

Three harvesting modes are supported. They are mutually exclusive and
the precedence order when more than one is configured is:

  1. User-supplied ``IrradianceModel`` (a ``Ptr<SolarIrradianceModel>``
     attribute). When set it is the sole source of irradiance (in
     W/m\ :sup:`2`); the composite source multiplies by
     ``PanelAreaM2`` and ``PanelEfficiency`` to obtain the
     instantaneous harvest power.
  2. LEO cycle (``UseLeoCycle = true``, the default). The built-in
     phase machine alternates ``SunlightSeconds`` of full irradiance
     ``SolarConstantWm2`` with ``ShadowSeconds`` of darkness.
  3. Fixed harvesting window (``UseLeoCycle = false`` plus
     ``AddSolarPanelWindow(P, start, end)``). Constant power ``P`` is
     injected in ``[start, end)`` and nothing outside of it.

Three irradiance-model implementations ship with the module:

  * ``ConstantSolarIrradianceModel`` — time-invariant W/m\ :sup:`2`.
  * ``LeoCycleSolarIrradianceModel`` — periodic sunlight/shadow, with
    a ``PhaseSeconds`` offset so different satellites can start in
    different orbital phases.
  * ``CallbackSolarIrradianceModel`` — delegates to a user callback
    ``double f(Time t)``. Use this to plug in an orbit propagator,
    pointing/attitude model, eclipse geometry, atmospheric
    attenuation, or a CSV trace of measured irradiance.

Two charging-realism knobs are provided:

  * ``MaxChargeVoltageV`` approximates the CC→CV transition in Li-Ion
    chargers: harvesting is clamped to zero once the supply voltage
    reaches the configured ceiling. Disabled when 0 (default).
  * ``ChargeEfficiency`` (in [0,1]) is a lumped loss factor covering
    MPPT / regulator / coulombic inefficiency. It attenuates injected
    power, not the raw irradiance.

Energy is additionally bounded by ``MaxEnergyJ``: the harvester is
driven to zero once the battery's remaining energy reaches this cap.
A value of 0 (default) means the cap equals ``InitialEnergyJ``, so
starting at full charge the battery cannot exceed itself. Set
``MaxEnergyJ`` explicitly when the battery is to be initialised
partially discharged.

Usage
*****

Helpers
=======

No helper class is strictly required. Create a
``CompositeEnergySource`` directly, configure its Li-Ion and harvesting
attributes, and aggregate it to a node via an ``EnergySourceContainer``
exactly as with any other ``LiIonEnergySource``.

Examples
========

``examples/composite-energy-model-example.cc`` wires up a mixed
topology of UAV nodes (battery only) and satellite nodes
(CompositeEnergySource with LEO cycle) and prints remaining plus
harvested energy periodically:

.. sourcecode:: bash

  $ ./ns3 run composite-energy-model-example

Minimal code sketch:

.. sourcecode:: cpp

  Ptr<CompositeEnergySource> src = CreateObject<CompositeEnergySource>();
  src->SetAttribute("InitialEnergyJ",   DoubleValue(2000.0));
  src->SetAttribute("MaxEnergyJ",       DoubleValue(4000.0));
  src->SetAttribute("PanelAreaM2",      DoubleValue(2.0));
  src->SetAttribute("PanelEfficiency",  DoubleValue(0.28));
  src->SetAttribute("SunlightSeconds",  DoubleValue(3900.0));
  src->SetAttribute("ShadowSeconds",    DoubleValue(1800.0));
  src->SetAttribute("MaxChargeVoltageV",DoubleValue(4.2));
  src->SetAttribute("ChargeEfficiency", DoubleValue(0.9));

Pluggable irradiance model:

.. sourcecode:: cpp

  Ptr<CallbackSolarIrradianceModel> model =
      CreateObject<CallbackSolarIrradianceModel>();
  model->SetCallback(MakeCallback(&MyOrbitPropagator::GetIrradiance));
  src->SetAttribute("IrradianceModel", PointerValue(model));

Attributes
==========

``CompositeEnergySource`` inherits all of ``LiIonEnergySource``'s
attributes (``InitialEnergyJ``, ``InitialCellVoltage``,
``NominalCellVoltage``, ``ExpCellVoltage``, ``RatedCapacity``,
``NomCapacity``, ``ExpCapacity``, ``InternalResistance``,
``ThresholdVoltage``, ``PeriodicEnergyUpdateInterval``) and adds:

* ``UseLeoCycle`` (bool, default ``true``)
* ``PanelAreaM2`` (double, m\ :sup:`2`)
* ``PanelEfficiency`` (double, in [0,1])
* ``SolarConstantWm2`` (double, W/m\ :sup:`2`, default 1361)
* ``HarvestIntervalSeconds`` (double, control-loop period)
* ``SunlightSeconds`` / ``ShadowSeconds`` (double)
* ``MaxEnergyJ`` (double; cap, 0 means ``InitialEnergyJ``)
* ``IrradianceModel`` (``Ptr<SolarIrradianceModel>``, optional override)
* ``MaxChargeVoltageV`` (double; CC-CV cap, 0 disables)
* ``ChargeEfficiency`` (double, in [0,1], default 1.0)

Tracing
=======

``CompositeEnergySource`` exposes one trace source in addition to
those inherited from ``LiIonEnergySource``:

* ``HarvestedPower`` — ``TracedValue<double>``: instantaneous
  injected power in W, after efficiency and CC-CV clamps. Fires on
  every change.

Validation
**********

The module ships with a ``composite-energy-source`` test suite
covering:

* fixed harvesting window energy accounting;
* LEO sunlight/shadow alternation;
* ``IrradianceModel`` callback override of built-in modes;
* ``ChargeEfficiency`` scaling;
* ``MaxChargeVoltageV`` hard clamp.

Run with:

.. sourcecode:: bash

  $ ./ns3 run "test-runner --suite=composite-energy-source"

References
**********

* Rakhmatov, D., and Vrudhula, S. "An analytical high-level battery
  model for use in energy management of portable electronic systems."
  *ICCAD 2001*.
* Tremblay, O., Dessaint, L.-A. "Experimental validation of a battery
  dynamic model for EV applications." *World Electric Vehicle Journal*,
  2009 (Shepherd equation used by |ns3| ``LiIonEnergySource``).
* Knap, V. et al. "A review of battery sizing and degradation models
  for LEO satellites." *Applied Energy*, 2020.
