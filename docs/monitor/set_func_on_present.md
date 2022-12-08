# set_func_on_present

## Description

Immediately set the fan FRU's contained rotors to functional when the fan's
presence has been detected and **only** allow the fan FRU to be set to
functional when presence is detected. Using this configuration option
essentially latches a nonfunctional fan FRU to nonfunctional until it has been
replaced (i.e. removed and replugged). It does still allow the functional state
of the rotors to continue to be updated to reflect their current functional
state(s). After a fan's presence has been detected, any faults will be
re-detected for that fan FRU and its contained rotors.

This attribute is optional and defaults to false, meaning a newly inserted fan
will need to spin up before being set back to functional, and if it never spins
up, there won't be additional errors.

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
