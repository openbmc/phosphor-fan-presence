# Fan Monitor Configuration File

## Table of Contents
* [Overview](#overview)
* [Data Format](#data-format)
* [Example](#example)
* [System Config Location](#system-config-location)
* [Contents](#contents)
* [Validation](#validation)
* [Firmware Updates](#firmware-updates)
* [Loading and Reloading](#loading-and-reloading)


## Overview

The `phosphor-fan-monitor` application is controlled by a configuration
file (config file) to manage the `Functional` property's state on the
`xyz.openbmc_project.State.Decorator.OperationalStatus` interface for fan
inventory objects. The config file defines the method to use in determining the
functional state and the logging of errors for each fan within a machine. It
also provides the ability to adjust the determination of a fan being
non-functional due to fan hardware limitations.


## Data Format

The config file is written using the [JSON (JavaScript Object
Notation)](https://www.json.org/) data format and can be created using a text
editor.


## Example

See [config.json](../../monitor/example/config.json).


## System Config Location

The config file name is `config.json`.

### Supported Directory

`/usr/share/phosphor-fan-presence/monitor/`

The supported version of the config file is installed under this read-only
directory location as part of the firmware image install. This is where the
config file will be loaded from when no overriding config file exists.

#### Default Location

Where a single config file for 1-or-more system types can be used,
the config file can be located at the base of the supported directory.

i.e.) `/usr/share/phosphor-fan-presence/monitor/config.json`

#### Compatible System Type Location

The config file location can also be based on a system type. This is necessary
where more than one type of machine is supported in a single BMC firmware image
and those system types can not share a common config file.

A system type sub-directory can be obtained from the `IBMCompatibleSystem`
D-Bus interface's `Names` property. The `Names` property contains a list of one
or more compatible system types, ordered from most specific to the most general.

Example:
* `ibm,rainier-2u`
* `ibm,rainier`

The `phosphor-fan-monitor` application then traverses the supported
directory, appending each compatible system type entry as a sub-directory from
most specific to most general until the config file is found.

Example:
1. `/usr/share/phosphor-fan-presence/monitor/ibm,rainier-2u/`
   * (directory/config file does not exist)
2. `/usr/share/phosphor-fan-presence/monitor/ibm,rainier/config.json`
   * (config file found)

If a config file is not found and the machine is powered on, an error is logged
and `phosphor-fan-monitor` application terminates preventing the machine
from successfully powering on.

### Override Directory

`/etc/phosphor-fan-presence/monitor/`

A different version of the config file can be loaded by placing it into this
writable directory location. This avoids the need to build and install a new
firmware image on the BMC when changing the configuration, such as for testing
purposes.

The override directory may not exist on the BMC, therefore to be able to use
an overriding config file it must be created using the following command:

`mkdir -p /etc/phosphor-fan-presence/monitor`

### Search Order

The `phosphor-fan-monitor` application will search for the config file at
the directory locations in the following order:
1. Override directory
2. Supported directory
   * Default location
   * Compatible System Type location


## Contents

### Structure

TBD

### Syntax

TBD

### Comments

TBD


## Validation

TBD


## Firmware Updates

When a new firmware image is installed on the BMC, it will update the config
file in the standard directory.

The override directory will **not** be modified by a firmware update. If a
config file exists in the override directory, it will continue to be used as
the fan presence configuration instead of the config file located under the
appropriate location within the supported directory.


## Loading and Reloading

The config file is loaded when the `phosphor-fan-monitor` application
starts.

To force the application to reload the config file, use the following command
on the BMC:

`systemctl kill -s HUP phosphor-fan-monitor@0.service`

To confirm which config file was loaded, use the following command on the BMC:

`journalctl -u phosphor-fan-monitor@0.service | grep Loading`
