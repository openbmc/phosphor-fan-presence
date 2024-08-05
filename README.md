# Repository Details

## Table of Contents

- [Overview](#overview)
- [Building](#building)
- [Contents](#contents)

## Overview

The phosphor-fan-presence repository provides a set of thermal related
applications that control and monitor the cooling of a system using fans. Each
application independently controls or monitors an area related to the thermal
status of a system that uses fans as its primary cooling mechanism. Since each
application independent in its functionality, they can individually be included
in a BMC image at _configure_ time to provide only the functionality needed by a
user's machine.

## Building

By default, runtime JSON configuration file(s) are used for each application.
The use of YAML configuration file(s) has been deprecated in favor of the JSON
method. Support for the use of YAML based configuration files may be removed
once all applications completely support getting their configuration from JSON
file(s) at runtime.

The following applications are built by default:

- [Fan Control](#fan-control)
  - To disable from building, use the `-Dcontrol-service=disabled` meson option:
  ```
  meson build -Dcontrol-service=disabled
  ```
- [Fan Presence Detection](#fan-presence-detection)
  - To disable from building, use the `-Dpresence-service=disabled` meson
    option:
  ```
  meson build -Dpresence-service=disabled
  ```
- [Fan Monitoring](#fan-monitoring)
  - To disable from building, use the `-Dmonitor-service=disabled` meson option:
  ```
  meson build -Dmonitor-service=disabled
  ```
- [Sensor Monitoring](#sensor-monitoring)
  - To disable from building, use the `-Dsensor-monitor-service=disabled` meson
    option:
  ```
  meson build -Dsensor-monitor-service=disabled
  ```

The following applications must be enabled at _configure_ time to be built:

- [Cooling Type](#cooling-type)
  - To enable building this, set the `-Dcooling-type-service=enable` meson
    option:
  ```
  meson build -Dcooling-type-service=enabled
  ```

### YAML (Deprecated)

The location of the YAML configuration file(s) are provided at _configure_ time
to each application thru environment variables. The default YAML configuration
file(s) used are the examples found within each application's example directory.
See each application below ([Contents](#contents)) for more information on how
to set their specific _configure_ time options, including the location of the
YAML configuration file(s).

Meson defaults to JSON based runtime configuration, so to select the YAML
configuration use the '-Djson-config=disabled' option when building:

```
    1. meson build -Djson-config=disabled
    2. ninja -C build
```

### JSON

See each application below ([Contents](#contents)) for more information on how
to set their specific _configure_ time options and details on how to configure
each using JSON.

As JSON based runtime configuration is the default option, no extra options are
required to build:

```
    1. meson build
    2. ninja -C build
```

**Note: Features/Restrictions of applications in this package that are only
supported using the JSON based configuration are listed below:**

#### Features

- [Fan Presence Detection](#fan-presence-detection)
  - Error logging for missing fans
- [Fan Monitoring](#fan-monitoring)
  - Error logging for nonfunctional fans
  - System power off due to missing or nonfunctional fans
- [Sensor Monitoring](#sensor-monitoring) - Only supports JSON

## Contents

### Fan Control

Controls the fans based on a set of events that are configured using a group of
D-Bus objects and one-or-more triggers that run a configured set of actions.
These events are meant to be configured to handle all aspects of controlling the
fans within a system. Fans are added to zones that then have events configured
against the zone to control the fans based on the state of any sized group of
D-Bus objects.

- Available meson options:
  - `control-persist-root-path` - Base location to persist zone property states
    on the BMC
    - Default = '/var/lib/phosphor-fan-presence/control'

#### YAML (Deprecated)

- Available meson options:
  - `fan-def-yaml-file` - Build time fan configuration file
    - Default = ['control/example/fans.yaml'](control/example/fans.yaml)
  - `fan-zone-yaml-file` - Build time zone configuration file
    - Default = ['control/example/zones.yaml'](control/example/zones.yaml)
  - `fan-events-yaml-file` - Build time events configuration file
    - Default = ['control/example/events.yaml'](control/example/events.yaml)
  - `zone-conditions-yaml-file` Build time zone conditions configuration file
    - Default =
      ['control/example/zone_conditions.yaml'](control/example/zone_conditions.yaml)

[Example](control/example/)

#### JSON

[README](docs/control/README.md)

---

### Fan Presence Detection

Monitors the presence state of fans using GPIOs, nonzero tach feedbacks, or a
combination of both. This updates a configured location of a fan D-Bus object's
`Present` property according to the state of the methods used to detect the
fan's presence.

- Available meson options:
  - `num-presence-log-entries` - Maximum number of entries in the message log
    - Default = 50

#### YAML (Deprecated)

- Available meson options:
  - `presence-config` - Location of the config file
    - Default = ['presence/example/example.yaml'](presence/example/example.yaml)

Example: [example.yaml](presence/example/example.yaml)

#### JSON

[README](docs/presence/README.md)

---

### Fan Monitoring

Monitors the functional state of fans by comparing the fan feedback speed
against the current target, applying any configured adjustments to the target
due to fan hardware properties. In addition to updating the configured location
of a fan D-Bus object's `Functional` property in inventory, actions can be
configured\* to be taken based on the state of fans, i.e.) creating event logs
or powering off the system.

Another feature that can be configured is the ability to cancel the monitoring
of a set of fans that may be necessary to workaround designs of the fan hardware
and/or controller used.

\*_Actions to be taken based on the state of fans is only available using a JSON
based configuration_

- Available meson options:
  - `num-monitor-log-entries` - Maximum number of entries in the message log
    - Default = 75

#### YAML (Deprecated)

- Available meson options:
  - `fan-monitor-yaml-file` - Location of the config file
    - Default = ['monitor/example/monitor.yaml'](monitor/example/monitor.yaml)

Example: [monitor.yaml](monitor/example/monitor.yaml)

#### JSON

[README](docs/monitor/README.md)

---

### Cooling Type

Sets the `AirCooled` and `WaterCooled` property on the
`xyz.openbmc_project.Inventory.Decorator.CoolingType` interface in inventory
based on a given GPIO. No configuration files are used with this application as
it is command line driven.

---

### Sensor Monitoring

Takes actions, such as powering off the system, based on sensor thresholds and
values.

- Available meson options:
  - `sensor-monitor-persist-root-path` - Base location to persist sensor
    monitoring data
    - Default = '/var/lib/phosphor-fan-presence/sensor-monitor'
  - `sensor-monitor-hard-shutdown-delay` - Milliseconds to delay the alarm hard
    shutdown
    - Default = 23000
  - `sensor-monitor-soft-shutdown-delay` - Milliseconds to delay the alarm soft
    shutdown
    - Default = 900000
  - `use-host-power-state` - Use the host state for the power state as opposed
    to the PGOOD state.

[README](docs/sensor-monitor/README.md)

---
