# inventory

## Description

The relative path under inventory where the fan's inventory object exists on
D-Bus. Inventory's base path is `/xyz/openbmc_project/inventory` resulting in
the fan's inventory object path being the base path plus what's given here.  
i.e.) `/xyz/openbmc_project/inventory/system/chassis/motherboard/fan0`

## Attribute Value(s)

string

## Example

<pre><code>
{
  "fans": [
    {
      <b><i>"inventory": "/system/chassis/motherboard/fan0"</i></b>,
      "allowed_out_of_range_time": 30,
      "functional_delay": 5,
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
    }
  ]
}
</code></pre>
