# power_off_config

## Description

A list of power off rules based on the monitoring state of the fans configured.
These power off rules can perform different system power off types to protect a
system that is not able to provide enough cooling due to a number of missing or
nonfunctional fans.

## Attribute Value(s)

- `type` - ["hard", "soft", "epow"]
  - "hard" - Perform a hard shutdown that immediately powers off the system.
  - "soft" - Perform an orderly shutdown requesting that the host OS power off
    the system.
  - "epow" - Sets the thermal alert D-Bus property that a power off is imminent
    after a `service_mode_delay` amount of time passes.
- `cause` - ["missing_fan_frus", "nonfunc_fan_rotors"]
  - "missing_fan_frus" - Power off due to missing fan enclosures
  - "nonfunc_fan_rotors" - Power off due to nonfunctional fan
    rotors([`sensors`](sensors.md))
- `count` - integer
  - Number of the configured `cause` instances to begin the power off `type`

### "hard" & "soft" `type` power offs

- `delay` - integer
  - Time(in seconds) to delay performing the power off after `count` instances
    of the `cause` criteria is met.
- `state` - ["at_pgood", "runtime"] (Optional)
  - `at_pgood` - At system pgood, the power off rule becomes valid. A power off
    rule of `at_pgood` can halt a system power on if the conditions are met.
  - `runtime` - At system runtime, the power off rule becomes valid. A power off
    rule of `runtime` only goes into affect after the system reaches runtime.
    (Default)

### "epow" `type` power offs

- `service_mode_delay` - integer
  - Time(in seconds) given before the thermal alert D-Bus property is set to
    indicate that a power off is imminent if the `count` instances of the
    `cause` criteria still exists. This timer is canceled when the `count`
    instances of the `cause` is not longer met.
- `meltdown_delay` - integer
  - Time(in seconds) before a hard power off occurs after the
    `service_mode_delay` timer expires. This timer can not be canceled once
    started.

## Example

<pre><code>
{
  "fault_handling": [
    {
      "num_nonfunc_rotors_before_error": 1,
      <b><i>"power_off_config": [
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
      ]</i></b>
    }
  ]
}
</code></pre>
