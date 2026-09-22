# Fan Control Debug

Fan control's internal data structures can be dumped at runtime using the
`fanctl dump` command, which triggers fan control to write the structures to a
`/tmp/fan_control_dump.json` file. That file is a normal JSON file that can be
viewed, or the `fanctl query_dump` command can be used as a shortcut to just
print portions of the file.

[This page](fanctl/README.md) has additional information about the fanctl
command.

## Flight Recorder

The flight recorder contains interesting events such as each time the zone
target changes.

It can be printed with:

```text
fanctl query_dump -s flight_recorder
```

## Object Cache

The object cache contains the most recent D-Bus properties of all of the groups
from the groups.json files. If any of the D-Bus properties had a value of `NaN`
(not a number), it is deleted from the cache so won't be present.

It can be printed with:

```text
fanctl query_dump -s objects
```

One can restrict the output to specific names with `-n` and properties with
`-p`:

The `-n` option will match on substrings.

Print the full entries of all temp sensors in the cache:

```text
fanctl query_dump -s objects -n sensors/temperature
```

Print just the Value properties of all temperature sensors in the cache:

```text
fanctl query_dump -s objects -n sensors/temperature -p Value
```

This allows one to do something like find all Functional property values that
are false:

```text
fanctl query_dump -s objects -p Functional | grep -B 1 false
```

## Zone Config

The zone configuration contains values like the current target, current floor,
and any outstanding floor or target holds. Each zone also includes a `fans`
sub-object listing every fan currently active in the zone:

| Field      | Description                                         |
| ---------- | --------------------------------------------------- |
| `target`   | RPM/PWM target most recently written by fan control |
| `feedback` | Tach sensor name -> live feedback reading (RPM)     |

A zone whose sled is absent or unavailable will show `"fans": {}`. This
distinguishes it from zones that have never had fans configured.

It can be printed with:

```text
fanctl query_dump -s zones
```

Example output for a zone with two active fans, each with two rotors:

```json
"1": {
    "active": true,
    "target": 13500,
    "floor": 13500,
    "fans": {
        "chassis1_fan0": {
            "feedback": {
                "chassis1_fan0_0": 13450,
                "chassis1_fan0_1": 13460
            },
            "target": 13500
        },
        "chassis1_fan1": {
            "feedback": {
                "chassis1_fan1_0": 13480,
                "chassis1_fan1_1": 13490
            },
            "target": 13500
        }
    }
}
```

## Parameter Values

Parameters are key/value pairs set and used by actions.

They can be printed with:

```text
fanctl query_dump -s parameters
```

## Service Cache

Fan control maintains a map of objects paths of the group members to the
services that own them.

It can be printed with:

```text
fanctl query_dump -s services
```

## Configured Events

Fan control can dump a list of all of its configured event names along with
their group names.

It can be printed with:

```text
fanctl query_dump -s events
```
