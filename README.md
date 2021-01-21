# phosphor-fan-presence
Phosphor fan provides a set of fan monitoring and control applications:

## Fan Control 

Sets fan speeds based on configuration data.

## Fan Presence Detection

Monitors for fan presence using either GPIOs or nonzero tach readings or both.

## Fan Monitor

Compares actual fan speeds against expected ones and takes actions.
Additional documentation is [here](monitor/README.md).

## Cooling Type

Sets the cooling type property in the inventory based on a GPIO.

This is not built by default, use --enable-cooling type to enable.
  
## Sensor Monitor

Takes actions based on sensor thresholds and the like.
Additional documentation is [here](sensor-monitor/README.md).

This is not built by default, use --enable-sensor-monitor to enable.

## To Build
By default, YAML configuration file(s) are used at build time for each fan
application. The location of the YAML configuration file(s) are provided at
configure time to each application.

To build this package using YAML, do the following steps:
```
    1. ./bootstrap.sh
    2. ./configure ${CONFIGURE_FLAGS}
    3. make
```

To enable the use of JSON configuration file(s) at runtime, provide the
`--enable-json` flag at configure time.
```
    1. ./bootstrap.sh
    2. ./configure ${CONFIGURE_FLAGS} --enable-json
    3. make
```
*Note: The following fan applications support the use of a JSON configuration
file.*
* Fan presence detection(presence)
  * The error logging feature is only available via the JSON configuration.
* [Fan monitoring(monitor)](monitor/README.md)
  * [JSON Documentation](monitor/json)
* Fan control(control) - limited functionality
    * Currently only supports setting fans to the configured `full_speed`  
      (If you require more than just setting fans to the configured
       `full_speed`, it is recommended to continue using YAML based
       configurations until the final work can be completed to enable
       full fan control JSON support.)

To clean the repository run `./bootstrap.sh clean`.
