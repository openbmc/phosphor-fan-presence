# fan_type_definitions

## description

A list of pre-defined configurations for different fans within a multi-chassis
system. Each configuration is given a name by the `type` key and has values for
its attributes (e.g. `method`, `deviation`, etc.) set. The type name can then be
referenced by the `type` field in the `fans` list within a zone. On
multi-chassis systems, `sensors` objects have a new key called `path_suffix` in
place of `name` that constructs a sensor name by appending the `path_suffix` to
the `short_name` from a fan object in the `fans` list within a zone based on the
type name in its `type` key.

## type

The type name for a given configuration of fan. Referenced by the `type` field
in the `fans` list within `zones` (see [zones](zones.md))

## path_suffix

Suffix that is appended to a `short_name` for a fan to build a full sensor name
(see [zones](zones.md)).

## Example

```json
    "fan_type_definitions": [
        {
            "type": "systemFan",
            "method": "count",
            "count_interval": 1,
            "deviation": 18,
            "num_sensors_nonfunc_for_fan_nonfunc": 0,
            "monitor_start_delay": 30,
            "fan_missing_error_delay": 20,
            "nonfunc_rotor_error_delay": 0,
            "set_func_on_present": true,
            "sensors": [
                {
                    "path_suffix": "_0",
                    "threshold": 45,
                    "has_target": true
                },
                {
                    "path_suffix": "_1",
                    "threshold": 45,
                    "has_target": false
                }
            ]
        }
    ],
...
```
