# monitor_start_delay

## Description

The amount of time(in seconds) to delay from monitoring the fan at the start of
powering on the system. This attribute is optional and defaults to 0, meaning
the fan will immediately be monitored according to its configuration at the
start of power on.

## Attribute Value(s)

integer (default = 0)

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
      <b><i>"monitor_start_delay": 30</i></b>,
      "fan_missing_error_delay": 20,
      "nonfunc_rotor_error_delay": 0,
      "sensors": [
        {
          "name": "fan0_0",
          "has_target": true
        },
        {
          "name": "fan0_1",
          "has_target": false,
          "factor": 1.45,
          "offset": -909
        }
      ]
    }
  ]
}
</code></pre>
