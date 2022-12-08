# group

## Description

A list of sensor object names to include in determining trust of their values
based on the configured [`class`](class.md). These sensors are what's included
in the group that then has all of their current values trusted or not for
monitoring based on algorithm of the [`class`](class.md).

## Attribute Value(s)

- `name` - string
  - The name of the fan tach sensor located under the
    `/xyz/openbmc_project/sensros/fan_tach` D-Bus path.
- `in_trust` - boolean (Optional)
  - By default, the sensor is included in determining trust of the group.
    However this can be used to cancel/restart monitoring of a fan tach sensor
    but not have it included in the determination of trust.

## Example

<pre><code>
{
  "sensor_trust_groups": [
    {
      "class": "NonzeroSpeed",
      <b><i>"group": [
        {
          "name": "fan0_0"
        },
        {
          "name": "fan1_0"
        },
        {
          "name": "fan2_0",
          "in_trust": false
        }
      ]</i></b>
    }
  ]
}
</code></pre>
