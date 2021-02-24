# Repository Details

## Table of Contents
* [Overview](#overview)
* [Building](#building)
* [Contents](#contents)

## Overview
The phosphor-fan-presence repository provides a set of thermal related
applications that control and monitor the cooling of a system using fans. Each
application independently controls or monitors an area related to the thermal
status of a system that uses fans as its primary cooling mechanism. Since each
application independent in its functionality, they can individually be included
in a BMC image at *configure* time to provide only the functionality needed by
a user's machine.


## Building
By default, build time YAML configuration file(s) are used for each
application. The use of YAML configuration file(s) has been deprecated in favor
of using runtime JSON configuration file(s). Support for the use of YAML based
configuration files will be removed once all applications completely support
getting their configuration from JSON file(s) at runtime.

The following applications are built by default:
* [Fan Control](#fan-control)
  * To disable from building, provide the `--disable-control` flag at
  *configure* time:
  ```
  ./configure ${CONFIGURE_FLAGS} --disable-control
  ```
* [Fan Presence Detection](#fan-presence-detection)
  * To disable from building, provide the `--disable-presence` flag at
  *configure* time:
  ```
  ./configure ${CONFIGURE_FLAGS} --disable-presence
  ```
* [Fan Monitoring](#fan-monitoring)
  * To disable from building, provide the `--disable-monitor` flag at
  *configure* time:
  ```
  ./configure ${CONFIGURE_FLAGS} --disable-monitor
  ```
* [Cooling Type](#cooling-type)
  * To disable from building, provide the `--disable-cooling-type` flag at
  *configure* time:
  ```
  ./configure ${CONFIGURE_FLAGS} --disable-cooling-type
  ```

The following applications must be enabled at *configure* time to be built:
* [Sensor Monitoring](#sensor-monitoring)
  * To enable building this, provide the `--enable-sensor-monitor` flag at
  *configure* time:
  ```
  ./configure ${CONFIGURE_FLAGS} --enable-sensor-monitor
  ```

To clean the repository run `./bootstrap.sh clean`.

### YAML (Deprecated)
The location of the YAML configuration file(s) are provided at
*configure* time to each application thru environment variables. The default
YAML configuration file(s) used are the examples found within each
application's example directory. See each application below
([Contents](#contents)) for more information on how to set their specific
*configure* time options, including the location of the YAML configuration
file(s).

To simply build this package to use YAML, without changing the configurations
of each application for a specific system, do the following steps:
```
    1. ./bootstrap.sh
    2. ./configure ${CONFIGURE_FLAGS}
    3. make
```

### JSON
See each application below ([Contents](#contents)) for more information on how
to set their specific *configure* time options and details on how to configure
each using JSON.

To build this package to use JSON based runtime configuration for all
applications, follow these steps and provide the `--enable-json` flag at
*configure* time:
```
    1. ./bootstrap.sh
    2. ./configure ${CONFIGURE_FLAGS} --enable-json
    3. make
```
**Note: Features/Restrictions of applications in this package that are only
supported using the JSON based configuration are listed below:**
#### Features
* [Fan Presence Detection](#fan-presence-detection)
  * Error logging for missing fans
* [Fan Monitoring](#fan-monitoring)
  * Error logging for nonfunctional fans
  * System power off due to missing or nonfunctional fans
* [Sensor Monitoring](#sensor-monitoring) - Only supports JSON

#### Restrictions
* [Fan Control](#fan-control)
  * Currently only supports setting fans to the configured `full_speed`.
  If you require more than just setting fans to the configured `full_speed`, it
  is recommended to continue using YAML based configurations by providing the
  `--disable-json-control` flag at *configure* time.


## Contents

### Fan Control
Controls the fans based on a set of events that are configured using a group of
D-Bus objects and one-or-more triggers that run a configured set of actions.
These events are meant to be configured to handle all aspects of controlling
the fans within a system. Fans are added to zones that then have events
configured against the zone to control the fans based on the state of any sized
group of D-Bus objects.

* *Configure* time environment variables:
  * `CONTROL_BUSNAME` - Application's D-Bus busname to own
    * Default = 'xyz.openbmc_project.Control.Thermal'
  * `CONTROL_OBJPATH` - Application's root D-Bus object path
    * Default = '/xyz/openbmc_project/control/thermal'
  * `CONTROL_PERSIST_ROOT_PATH` - Base location to persist zone property states
  on the BMC
    * Default = '/var/lib/phosphor-fan-presence/control'

#### YAML (Deprecated)
* *Configure* time environment variables:
  * `FAN_DEF_YAML_FILE` - Build time fan configuration file
    * Default = ['control/example/fans.yaml'](control/example/fans.yaml)
  * `FAN_ZONE_YAML_FILE` - Build time zone configuration file
    * Default = ['control/example/zones.yaml'](control/example/zones.yaml)
  * `ZONE_EVENTS_YAML_FILE` - Build time events configuration file
    * Default = ['control/example/events.yaml'](control/example/events.yaml)
  * `ZONE_CONDITIONS_YAML_FILE` Build time zone conditions configuration file
    * Default = ['control/example/zone_conditions.yaml'](control/example/zone_conditions.yaml)

[Example](control/example/)

#### JSON
[README](docs/control/README.md)

---

### Fan Presence Detection
Monitors the presence state of fans using GPIOs, nonzero tach feedbacks, or a
combination of both. This updates a configured location of a fan D-Bus object's
`Present` property according to the state of the methods used to detect the
fan's presence.

* *Configure* time environment variables:
  * `NUM_PRESENCE_LOG_ENTRIES` - Maximum number of entries in the message log
    * Default = 50

#### YAML (Deprecated)
* *Configure* time environment variables:
  * `PRESENCE_CONFIG` - Location of the config file
    * Default = ['presence/example/example.yaml'](presence/example/example.yaml)

Example: [example.yaml](presence/example/example.yaml)

#### JSON
[README](docs/presence/README.md)

---

### Fan Monitoring
Monitors the functional state of fans by comparing the fan feedback speed
against the current target, applying any adjustments to the target due to fan
hardware properties. In addition to updating the configured location of a fan
D-Bus object's `Functional` property, actions can be configured to be taken
based on the state of fans, i.e.) logging errors or powering off the system.

Another feature that can be configured is the ability to cancel the monitoring
of a set of fans that may be necessary to workaround designs of the fan
hardware and/or controller used.

* *Configure* time environment variables:
  * `NUM_MONITOR_LOG_ENTRIES` - Maximum number of entries in the message log
    * Default = 75
  * `THERMAL_ALERT_BUSNAME` - Application's D-Bus busname to own
    * Default = 'xyz.openbmc_project.Thermal.Alert'
  * `THERMAL_ALERT_OBJPATH` - Application's root D-Bus object path
    * Default = '/xyz/openbmc_project/alerts/thermal_fault_alert'

#### YAML (Deprecated)
* *Configure* time environment variables:
  * `FAN_MONITOR_YAML_FILE` - Location of the config file
    * Default = ['monitor/example/monitor.yaml'](monitor/example/monitor.yaml)

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

* *Configure* time environment variables:
  * `SENSOR_MONITOR_PERSIST_ROOT_PATH` - Base location to persist sensor
  monitoring data
    * Default = '/var/lib/phosphor-fan-presence/sensor-monitor'
  * `SHUTDOWN_ALARM_HARD_SHUTDOWN_DELAY_MS` - Milliseconds to delay the alarm
  hard shutdown
    * Default = 23000
  * `SHUTDOWN_ALARM_SOFT_SHUTDOWN_DELAY_MS` - Milliseconds to delay the alarm
  soft shutdown
    * Default = 900000

[README](docs/sensor-monitor/README.md)

---
