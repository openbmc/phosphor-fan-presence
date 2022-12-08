# Fan Control tool

A tool that enables a user to view the status of a system in regard to fan
control including the ability to manually set the fans to a desired RPM (or PWM
if supported by the system). This tool has been tested against systems utilizing
the phosphor-fan-presence repository set of fan applications (i.e. romulus,
witherspoon, etc...) and is not warranted to work on systems using some other
set of BMC fan applications.

## Intention

The intention of this tool is to temporarily stop the automatic fan control
algorithm and allow the user to manually set the fans within the system chassis
to a given target. Once a user no longer has a need to manually control the
fans, the resume operation re-enables and restarts the phosphor-fan-control
service. The status command provides a simple way to get the status of the fans
along with the the main system states and fan control systemd service, while the
reload command is available to commit JSON changes made to config files
(YAML-based configurations are not reloadable).

Note: In the case where a system does not have an active fan control algorithm
enabled yet, an intended safe fan target should be set prior to resuming.

## Usage

```
NAME
  fanctl - Manually control, get fan tachs, view status, reload config,
  and resume automatic control of all fans within a chassis.

SYNOPSIS
  fanctl [OPTION]

OPTIONS
set <TARGET> [TARGET SENSOR LIST]
    <TARGET>
        - RPM/PWM target to set the fans
    [TARGET SENSOR LIST]
        - space-delimited list of target sensors to set
get
    - Get the current fan target and feedback speeds for all rotors
status
    - Get the full system status in regard to fans
reload
    - Reload phosphor-fan configuration JSON files (YAML configuration not
      supported)
resume
    - Resume automatic fan control
    * Note: In the case where a system does not have an active fan control
      algorithm enabled yet, an intended safe fan target should be set
      prior to resuming
dump
    - Tell fan control to dump its caches and flight recorder.
query_dump
    - Provides arguments to search the dump file.
help
    - Display this help and exit
```

## Examples:

- Set all fans to a target value (The tool determines whether the machine is
  using RPM or PWM fan speeds, and sets them to the value provided):

  > fanctl set 10500

- Set only fan_0, fan1_0, fan2_0 to target 8500:

  > fanctl set 8500 fan0_0 fan1_0 fan2_0

- Resume automatic fan control:

  > fanctl resume

- Get the current fan target and feedback speeds for all rotors:

  > fanctl get

  ```
  > fanctl get
  TARGET SENSOR    TARGET(RPM)   FEEDBACK SENSOR    FEEDBACK(RPM)
  ===============================================================
  fan0_0             10000            fan0_0            7020
                                      fan0_1           10000
  fan1_0              2300            fan1_0            2192
                                      fan1_1            2300
  fan2_0              2300            fan2_0            2192
                                      fan2_1            2300
  fan3_0              3333            fan3_0            2839
                                      fan3_1            3333
  fan4_0              3333            fan4_0            2839
                                      fan4_1            3333
  fan5_0             10000            fan5_0            7020
                                      fan5_1           10000
  ```

- Get the full system status in regard to fans:

  > fanctl status

  ```
  Fan Control Service State   : loaded, inactive(dead)

  CurrentBMCState     : xyz.openbmc_project.State.BMC.BMCState.Ready
  CurrentPowerState   : xyz.openbmc_project.State.Chassis.PowerState.Off
  CurrentHostState    : xyz.openbmc_project.State.Host.HostState.Off

   FAN        TARGET(RPM)   FEEDBACK(RPM)  PRESENT     FUNCTIONAL
  ===============================================================
   fan0         10000        7020/10000      true         true
   fan1         10000        7020/10000      true         true
   fan2         10000        7020/10000      true         true
   fan3         10000        7020/10000      true         true
   fan4         10000        7020/10000      true         true
   fan5         10000        7020/10000      true         true

  ```

- Reload all json config files in the order each is found: override location,
  given `Compatible` interface location, default location.

  > fanctl reload

- Tell the fan control daemon to dump debug data to /tmp/fan_control_dump.json

  > fanctl dump

- Print all temperatures in the fan control cache after running 'fanctl dump':

  > fanctl query_dump -s objects -n sensors/temperature -p Value

- Print every interface and property in the Ambient temp sensor's cache entry:

  > fanctl query_dump -s objects -n Ambient

- Print the flight recorder after running 'fanctl dump':
  > fanctl query_dump -s flight_recorder
