# num_nonfunc_rotors_before_error

## Description

The number of fan rotors (also known as [`sensors`](sensors.md)) that must be
deemed nonfunctional for an error event log to be created. When the number of
nonfunctional fan rotors are below this value, informational event logs are
created against those nonfunctional fan rotors. This attribute is optional and
defaults to 1, meaning an error event log should be created when any one of the
fan rotors([`sensors`](sensors.md)) being monitored is nonfunctional.

## Attribute Value(s)

integer (default = 1)

## Example

<pre><code>
{
  "fault_handling": [
    {
      <b><i>"num_nonfunc_rotors_before_error": 1</i></b>,
      "power_off_config": [
        {
          "type": "hard",
          "cause": "missing_fan_frus",
          "count": 1,
          "delay": 25,
          "state": "at_pgood"
        },
        {
          "type": "soft",
          "cause": "nonfunc_fan_rotors",
          "count": 2,
          "delay": 30,
          "state": "runtime"
        },
        {
          "type": "epow",
          "cause": "nonfunc_fan_rotors",
          "count": 3,
          "service_mode_delay": 300,
          "meltdown_delay": 300
        }
      ]
    }
  ]
}
</code></pre>
