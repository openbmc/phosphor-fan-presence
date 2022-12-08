# Fan Control Configuration File

## Table of Contents

- [Overview](#overview)
- [Data Format](#data-format)
- [Example](#example)
- [System Config Location](#system-config-location)
- [Contents](#contents)
- [Validation](#validation)
- [Firmware Updates](#firmware-updates)
- [Loading and Reloading](#loading-and-reloading)
- [Debug](#debug)

## Overview

The `phosphor-fan-control` application is comprised of as set of configuration
files (config files) that constructs the algorithm in which fans are controlled
within a machine.

## Data Format

The config file is written using the
[JSON (JavaScript Object Notation)](https://www.json.org/) data format and can
be created using a text editor.

## System Config Location

The config file names are:

- `manager.json`
- `profiles.json`
- `fans.json`
- `zones.json`
- `groups.json`
- `events.json`

### Supported Directory

`/usr/share/phosphor-fan-presence/control/`

The supported version of the config files are installed under this read-only
directory location as part of the firmware image install. This is where the
config files will be loaded from when no corresponding override config file
exists.

#### Default Location

Where a single set of config files for 1-or-more system types can be used, the
config files can be located at the base of the supported directory.

i.e.)  
`/usr/share/phosphor-fan-presence/control/manager.json`  
`/usr/share/phosphor-fan-presence/control/profiles.json`  
`/usr/share/phosphor-fan-presence/control/fans.json`  
`/usr/share/phosphor-fan-presence/control/zones.json`  
`/usr/share/phosphor-fan-presence/control/groups.json`  
`/usr/share/phosphor-fan-presence/control/events.json`

#### Compatible System Type Location

The config files location can also be based on a system type. This is necessary
where more than one type of machine is supported in a single BMC firmware image
and those system types can not share any one common config file.

A system type sub-directory can be obtained from the `IBMCompatibleSystem` D-Bus
interface's `Names` property. The `Names` property contains a list of one or
more compatible system types, ordered from most specific to the most general.

Example:

- `ibm,rainier-2u`
- `ibm,rainier`

The `phosphor-fan-control` application then traverses the supported directory,
appending each compatible system type entry as a sub-directory from most
specific to most general on each config file until it is found.

Example:

1. `/usr/share/phosphor-fan-presence/control/ibm,rainier-2u/`
   - (directory/config file does not exist)
2. `/usr/share/phosphor-fan-presence/control/ibm,rainier/events.json`
   - (config file found)

If any required config file is not found and the machine is powered on, an error
is logged and `phosphor-fan-control` application terminates preventing the
machine from successfully powering on.

### Override Directory

`/etc/phosphor-fan-presence/control/`

A different version of each of the config files can be loaded by placing it into
this writable directory location. This avoids the need to build and install a
new firmware image on the BMC when changing any part of the configuration, such
as for testing purposes.

The override directory may not exist on the BMC, therefore to be able to use any
number of overriding config files it must be created using the following
command:

`mkdir -p /etc/phosphor-fan-presence/control`

### Search Order

The `phosphor-fan-control` application will search for each config file at the
directory locations in the following order:

1. Override directory
2. Supported directory
   - Default location
   - Compatible System Type location

## Contents

### Structure

#### fans.json

This file consists of an array of fan objects representing the fan FRUs in the
system.

```
[
    {
        "name": "fan0",
        "zone": "0",
        "sensors": ["fan0_0"],
        "target_interface": "xyz.openbmc_project.Control.FanSpeed"
    }
    ...
]
```

[Syntax](fans.md)

#### zones.json

This file contains parameters specific to the zone.

```
[
    {
        "name": "0",
        "poweron_target": 18000,
        "default_floor": 18000,
        "increase_delay": 5,
        "decrease_interval": 30
    },
    ...
]
```

[Syntax](zones.md)

#### groups.json

This file defines the groups that events.json will use in its actions. Groups
consist of one or more D-Bus object paths and a name.

```
[
   {
     "name": "fan inventory",
     "members": [
       "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan0",
       "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan1",
       "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan2",
       "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan3",
       "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan4",
       "/xyz/openbmc_project/inventory/system/chassis/motherboard/fan5"
     ]
   }
 ...
]
```

[Syntax](groups.md)

#### events.json

This file contains the fan control events, where each event can contain groups,
trigger, and actions.

```
{
  [
    {
      "name": ...
      "groups": [
        {
           ...
        },
        ...
      ],

      "triggers": [
        {
            ...
        },
        ...
      ],

      "actions": [
        {
          ...
        },
        ...
      ],
      ...
    },
    ...
  ]
}
```

[Syntax](events.md)

### Comments

The JSON data format does not support comments. However, the library used by
code to parse the JSON does support '//' as a comment:

```
// This is a comment.
{
   ...
}
```

Alternatively, a comment attribute can be used:

```
{
  "comment": "This is a comment."
  ...
}
```

Fans can be queried and controlled manually using the fanctl utility. Full
documentation can be found at
https://github.com/openbmc/phosphor-fan-presence/blob/master/docs/control/fanctl/README.md

## Validation

TBD

## Firmware Updates

When a new firmware image is installed on the BMC, it will update the config
file in the standard directory.

The override directory will **not** be modified by a firmware update. If a
config file exists in the override directory, it will continue to be used as the
fan presence configuration instead of the config file located under the
appropriate location within the supported directory.

## Loading and Reloading

The config files are loaded when the `phosphor-fan-control` application starts.

To force the application to reload the config files, use the following command
on the BMC:

`systemctl kill -s HUP phosphor-fan-control@0.service`

To confirm which config files were loaded, use the following command on the BMC:

`journalctl -u phosphor-fan-control@0.service | grep Loading`

## Debug

Fan control maintains internal data structures that can be be dumped at runtime.
Details [here](debug.md).
