# fans.json

## JSON Example

```
[
    {
        "name": "fan0",
        "zone": "0",
        "sensors": ["fan0_0"],
        "target_interface": "xyz.openbmc_project.Control.FanSpeed",
        "target_path": "/xyz/openbmc_project/control/fanpwm/"
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
