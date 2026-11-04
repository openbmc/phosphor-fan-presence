# chassis_definitions

## description

A list of configurations for chassis within a multi-chassis system. Defines
attributes for chassis name, available property monitoring, and zones for
monitoring within the chassis. `chassis_numbers` defines which chassis the
configuration applies to, where the `${chassis}` placeholder can be used to
substitute for a given chassis number from within the list. `${node}` can also
be used a placeholder for the node number (chassis number minus 1).

## name

The name of the chassis. This is appended to
`/xyz/openbmc_project/inventory/system/` to generate the full inventory path for
the corresponding chassis DBus object.

## availPropUsed

Whether or not to monitor the `Available` property on the
`xyz.openbmc_project.State.Decorator.Availability` interface when creating zones
for a chassis.

## chassis_numbers

The chassis numbers that a chassis configuration applies to. Substituted in
place of the `${chassis}` placeholder.

## Example

```json
"chassis_definitions": [
    {
        "name": "chassis${chassis}",
        "availPropUsed": false,
        "chassis_numbers": [1, 2, 3, 4, 5, 6, 7, 8],
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
                    ...
                ],
                "fault_handling": {
                    "power_off_config": [
                        ...
                    ]
                }
            }
        ]
    }
]
```
