# deviation

## Description

The +/- percentage allowed for the sensors of a fan to deviate from any
requested target before being deemed out-of-spec and not functioning as expected
according to the target requested.

## Attribute Value(s)

integer (0 - 100)

Deviation is represented as a percentage, so only 0 to 100 are valid values.

## Example

<pre><code>
{
  "fans": [
    {
      "inventory": "/system/chassis/motherboard/fan0",
      "allowed_out_of_range_time": 30,
      "functional_delay": 5,
      <b><i>"deviation": 15</i></b>,
      "num_sensors_nonfunc_for_fan_nonfunc": 1,
      "monitor_start_delay": 30,
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
