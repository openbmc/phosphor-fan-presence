# zones

## description

A list of configurations that defines fan monitoring zones for a chassis within
a multi-chassis system. Each configuration is given a name, a list of fan
configurations that apply attributes from the `fan_type_definitions` list, and a
`fault_handling` object that describes power off rules.

## name

The name of the zone. Can use the `${chassis}` to substitute in a chassis
number. `${node}` can also be used a placeholder for the node number (chassis
number minus 1).

## fans

### description

A list of fan objects that provide conifgurations for fans and their
corresponding presence GPIOs within a zone.

### type

The type of fan. Applies attributes from `fan_type_definitions` for the
corresponding fan type.

### inventory

The inventory path of the fan (appended to the base
`/xyz/openbmc_project/inventory`). Can use `${chassis}` as a placeholder for the
chassis number or `${node}` as a placeholder for the node number (chassis number
minus 1).

### short_name

The short name of the fan. The `path_suffix` from the `sensors` list in
`fan_type_definitions` is appended to this to generate the full sensor names for
the fan's sensors. Can use `${chassis}` as a placeholder for the chassis number
or `${node}` as a placeholder for the node number (chassis number minus 1).

### pretty_name

The value posted to the `PrettyName` property under the
`xyz.openbmc_project.Inventory.Item` interface. Can use `${chassis}` as a
placeholder for the chassis number or `${node}` as a placeholder for the node
number (chassis number minus 1).

### presence_gpio

The name of the presence GPIO for the fan. Can use `${chassis}` as a placeholder
for the chassis number or `${node}` as a placeholder for the node number
(chassis number minus 1).

### presence_gpio_polarity

The polarity of the presence GPIO for the fan. Can be either `Low` or `High`.

### presence_tach_redundancy

If true, fan monitor will use both non-zero tach readings and presence GPIO
readings to determine the presence state of the fans. If false, only presence
GPIO readings will be used.

## fault_handling

An object with a list of power off rules (`power_off_config`) for the zone.
Fields are same as single-chassis (see [power_off_config](power_off_config.md)).

## Example

```json
"zones": [
    {
        "name": "chassis_${chassis}_zone",
        "fans": [
            {
                "type": "systemFan",
                "inventory": "/system/chassis${chassis}/motherboard/fan0",
                "short_name": "chassis${chassis}_fan0",
                "pretty_name": "Node ${node} Fan 0",
                "presence_gpio": "presence-fan0-chassis${chassis}",
                "presence_gpio_polarity": "Low",
                "presence_tach_redundancy": true
            },
            {
                "type": "systemFan",
                "inventory": "/system/chassis${chassis}/motherboard/fan1",
                "short_name": "chassis${chassis}_fan1",
                "pretty_name": "Node ${node} Fan 1",
                "presence_gpio": "presence-fan1-chassis${chassis}",
                "presence_gpio_polarity": "Low",
                "presence_tach_redundancy": true
            },
            {
                "type": "systemFan",
                "inventory": "/system/chassis${chassis}/motherboard/fan2",
                "short_name": "chassis${chassis}_fan2",
                "pretty_name": "Node ${node} Fan 2",
                "presence_gpio": "presence-fan2-chassis${chassis}",
                "presence_gpio_polarity": "Low",
                "presence_tach_redundancy": true
            },
            {
                "type": "systemFan",
                "inventory": "/system/chassis${chassis}/motherboard/fan3",
                "short_name": "chassis${chassis}_fan3",
                "pretty_name": "Node ${node} Fan 3",
                "presence_gpio": "presence-fan3-chassis${chassis}",
                "presence_gpio_polarity": "Low",
                "presence_tach_redundancy": true
            }
        ],
        "fault_handling": {
            "power_off_config": [
                {
                    "type": "epow",
                    "cause": "nonfunc_fan_rotors",
                    "count": 2,
                    "service_mode_delay": 150,
                    "meltdown_delay": 150
                }
            ]
        }
    }
]
```
