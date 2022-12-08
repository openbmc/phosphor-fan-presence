# sensors

## Description

An array of sensor objects that make up the fan enclosure object. The sensors
are what's monitored to determine the functional state of the fan.

A sensor's functional range is determined by the following equation(where
_[target]_ is current requested target of the fan):

- Minimum = ([target] _ (100 - [`deviation`](deviation.md)) / 100) _ `factor` +
  `offset`
- Maximum = ([target] _ (100 + [`deviation`](deviation.md)) / 100) _ `factor` +
  `offset`

Therefore, a fan sensor must be above the minimum and less than the maximum to
be deemed functional.

## Attribute Value(s)

- `name` - string
  - The name of the fan tach sensors located under the
    `/xyz/openbmc_project/sensors/fan_tach` D-Bus path.
- `has_target` - boolean
  - Whether this sensor D-Bus object contains the `Target` property or not.
- `target_interface` - string (Optional)
  - The D-Bus interface containing the `Target` property. This defaults to
    `xyz.openbmc_project.Control.FanSpeed` for RPM controlled fans or can be set
    to `xyz.openbmc_project.Control.FanPwm` for PWM controlled fans.
- `target_path` - string (Optional)
  - The D-Bus full object path containing the `Target` property. This defaults
    to `/xyz/openbmc_project/sensors/fan_tach`+`name`.
- `factor` - double (Optional)
  - A value to multiply the current target by to adjust the monitoring of this
    sensor due to how the hardware works. This sensor attribute is optional and
    defaults to 1.0.
- `offset` - integer (Optional)
  - A value to shift the current target by to adjust the monitoring of this
    sensor due to how the hardware works. This sensors attribute is optional and
    defaults to 0.

## Example

<pre><code>
{
  "fans": [
    {
      "inventory": "/system/chassis/motherboard/fan0",
      "allowed_out_of_range_time": 30,
      "functional_delay": 5,
      "deviation": 15,
      "num_sensors_nonfunc_for_fan_nonfunc": 1,
      "monitor_start_delay": 30,
      "fan_missing_error_delay": 20,
      "nonfunc_rotor_error_delay": 0,
      <b><i>"sensors": [
        {
          "name": "fan0_0",
          "has_target": true,
          "target_interface": "xyz.openbmc_project.Control.FanPwm",
          "target_path": "/xyz/openbmc_project/control/fanpwm/PWM0"
        },
        {
          "name": "fan0_1",
          "has_target": false,
          "factor": 1.45,
          "offset": -909
        }
      ]</i></b>
    }
  ]
}
</code></pre>
