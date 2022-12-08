# fan_missing_error_delay

## Description

The amount of time(in seconds) to delay logging an error against the fan when
its not present. This attribute is optional and defaults to **no error** being
logged when the fan is not present when the attribute is not given.

## Attribute Value(s)

integer (default = do not create an error log)

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
      <b><i>"fan_missing_error_delay": 20</i></b>,
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
