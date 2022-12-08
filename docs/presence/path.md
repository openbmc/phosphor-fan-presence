# path

## Description

The relative path under inventory where the fan's inventory object exists on
D-Bus. Inventory's base path is `/xyz/openbmc_project/inventory` resulting in
the fan's inventory object path being the base path plus what's given here.  
i.e.) `/xyz/openbmc_project/inventory/system/chassis/motherboard/fan0`

## Attribute Value(s)

string

## Example

<pre><code>
[
  {
    "name": "fan0",
    <b><i>"path": "/system/chassis/motherboard/fan0"</i></b>,
    "methods": [
      {
        "type": "tach",
        "sensors": [
          "fan0_0"
        ]
      }
    ],
    "rpolicy": {
      "type": "anyof"
    }"
  }
]
</code></pre>
