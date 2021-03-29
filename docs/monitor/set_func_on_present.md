# set_func_on_present

## Description
If the fan FRU and contained rotors should be set to functional immediately on
presence being detected.  Any faults will be re-detected.  This attribute is
optional and defaults to false, meaning a newly inserted fan will need to spin
up before being set back to functional, and if it never spins up, there won't
be additional errors.

## Attribute Value(s)
bool (default = false)

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
      <b><i>"set_func_on_present": true</i></b>,
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
