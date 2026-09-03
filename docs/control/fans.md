# fans.json

## JSON Example

```json
[
    {
        "name": "fan0",
        "zone": "0",
        "sensors": ["fan0_0"],
        "target_interface": "xyz.openbmc_project.Control.FanSpeed",
        "target_path": "/xyz/openbmc_project/control/fanpwm/"
    },
    {
        "name": "fan1",
        "zone": "1",
        "sensors": ["fan1_0"],
        "target_interface": "xyz.openbmc_project.Control.FanSpeed",
        "chassis_path": "/xyz/openbmc_project/inventory/system/chassis1",
        "check_chassis_availability": true
    }
    ...
]
```

## Attributes

### name

The D-Bus name of the fan FRU. Required.

### zone

The zone the fan is in. Required.

### sensors

The D-bus sensor names associated with that fan. Required.

### target_interface

The D-Bus interface to use for setting the fan target speed/PWM. Either
`xyz.openbmc_project.Control.FanSpeed` for RPM controlled fans or
`xyz.openbmc_project.Control.FanPWM` for PWM controlled fans. Required.

### target_path

The D-Bus object path used for setting the fan target speed/PWM via
"target_interface". If not configured, it defaults to
`/xyz/openbmc_project/sensors/fan_tach/`. Optional.

### chassis_path

The D-Bus inventory path of the chassis sled this fan belongs to, e.g.
`/xyz/openbmc_project/inventory/system/chassis1`. Optional.

When present, fan control defers binding this fan's sensors until the chassis
reports `Present = true` on the `xyz.openbmc_project.Inventory.Item` interface.
If the chassis is not yet present when fan control starts, the fan is held out
of its zone until a `PropertiesChanged` or `InterfacesAdded` signal indicates it
has become present. Omit this key on systems where all fans are always available
(single-chassis or non-hotplug systems).

### check_chassis_availability

When `true`, fan control additionally gates this fan on the `Available` property
of the `xyz.openbmc_project.State.Decorator.Availability` interface on the
chassis at `chassis_path`. The fan is only added to its zone when both `Present`
and `Available` are `true`. Requires `chassis_path` to be set. Optional;
defaults to `false`.
