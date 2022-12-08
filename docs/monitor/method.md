# method

## Description

The method to use in monitoring a fan's functional state. One method can be
configured per fan and each supported method requires its own set of attributes
to be provided.

## Attribute Value(s)

Methods:

- ["timebased"](#timebased) - Default
- ["count"](#count)

### "timebased"

Uses timers for determining when a fan's sensor should be marked nonfunctional
or functional after its been in that state for that continuous amount of time.
Separate timers are used to transition a fan from functional to
nonfunctional(`allowed_out_of_range_time`) and nonfunctional to
functional(`functional_delay`).

- `allowed_out_of_range_time` - Time(in seconds) that each fan sensor is allowed
  to be calculated out of range of a current target before being marked
  nonfunctional.
- `functional_delay` - Optional, default = 0
  - Time(in seconds) that each fan sensor must be calculated within range of a
    current target before being marked functional.

```
"method": "timebased",
"allowed_out_of_range_time": 30,
"functional_delay": 5
```

**Note: Since this method is the default, simply providing
`allowed_out_of_range_time` and `functional_delay` attributes will result in
this method being used.**

- This is equivalent to above:
  ```
  "allowed_out_of_range_time": 30,
  "functional_delay": 5
  ```

### "count"

An up/down counter for determining when a fan's sensor should be marked
nonfunctional based on a `threshold` or functional when the counter = 0. Each
fan sensor update where its feedback speed is calculated out of range of the
current target increments the counter by 1. Once the counter reaches the
`threshold`, the fan is marked nonfunctional. However, at any point the sensor's
feedback speed is within range, the counter is decremented by 1. Therefore the
fan's sensor(s) do not have to continuously be in a faulted state to be marked
nonfunctional and instead is deemed nonfunctional once it accumulates the
`threshold` number of times deemed out of range. The same is true for a
nonfunctional fan sensor to become functional, where the counter must accumulate
enough times deemed within range to decrement the counter to 0. This checking
occurs at an interval dictated by the `count_interval` field.

- `threshold` - Number of total times a fan sensor must be calculated out of
  range before being marked nonfunctional.

- `count_interval` - The interval, in seconds, to check the feedback speed and
  increment/decrement the counter. Defaults to 1s if not present.

```
"method": "count",
"count_interval": 5,
"sensors": [
  {
    "threshold": 30
  }
]
```

## Example

<pre><code>
{
  "fans": [
    {
      "inventory": "/system/chassis/motherboard/fan0",
      <b><i>"allowed_out_of_range_time": 30,
      "functional_delay": 5</i></b>,
      "deviation": 15,
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
    },
    {
      "inventory": "/system/chassis/motherboard/fan1",
      <b><i>"method": "count",
      "count_interval": 1</i></b>,
      "deviation": 15,
      "num_sensors_nonfunc_for_fan_nonfunc": 1,
      "monitor_start_delay": 30,
      "fan_missing_error_delay": 20,
      "nonfunc_rotor_error_delay": 0,
      "sensors": [
        {
          "name": "fan0_0",
          "has_target": true,
          <b><i>"threshold": 30</i></b>
        },
        {
          "name": "fan0_1",
          "has_target": false,
          "factor": 1.45,
          "offset": -909,
          <b><i>"threshold": 30</i></b>
        }
      ]
    }
  ]
}
</code></pre>
