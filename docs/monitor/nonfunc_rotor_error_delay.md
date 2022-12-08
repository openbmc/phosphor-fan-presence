# nonfunc_rotor_error_delay

## Description

The amount of time(in seconds) to delay logging an error against the fan when
any of its sensors are deemed nonfunctional. This attribute is optional and
defaults to **no error** being logged when the attribute is not given and a
sensor is nonfunctional.

**Note: The default of this changes to 0(immediately log an error) if the
`fault_handling` section of the configuration is given.**

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
      "fan_missing_error_delay": 20,
      <b><i>"nonfunc_rotor_error_delay": 0</i></b>,
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
