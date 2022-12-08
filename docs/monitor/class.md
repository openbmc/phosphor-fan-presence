# class

## Description

A trust group determination class that implements a way to check a group of fan
sensors trust state. Each of these 'classes' override a required function that
determines if the group is trusted by checking each fan sensors listed within
the trust group against an algorithm that returns whether they can be trusted or
not.

## Attribute Value(s)

- `NonzeroSpeed`
  - Determines if the group is trusted by checking if any sensors in the group
    has a nonzero speed. If all sensors report zero, then no sensors in the
    group are trusted and therefore monitoring of all the sensors in the group
    is canceled.

## Example

<pre><code>
{
  "sensor_trust_groups": [
    {
      <b><i>"class": "NonzeroSpeed"</i></b>,
      "group": [
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
      ]
    }
  ]
}
</code></pre>
