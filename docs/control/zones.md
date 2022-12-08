# zones.json

Zones are groups of fans that are set to the same values and have the same
properties like `default_floor` and `poweron_target`. The events in
[events.json](events.md) are then configured to operate on specific or all
zones.

## JSON Example

```
{
  "name": "0",
  "poweron_target": 18000,
  "default_floor": 18000,
  "increase_delay": 5,
  "decrease_interval": 30
}
```

## Attributes

### name

The zone name. Required.

### poweron_target

The fan target value set immediately after the power state changes to on. This
is also the ceiling unless `default_ceiling` is specified. Required.

### default_floor

The default fan floor value to use for the zone, if necessary. Possibly an
action will be configured to be in charge of the floor so this isn't necessary.
Optional.

### increase_delay

This throttles fan increases to the specified delay.

The delay, in seconds, between fan target increases, when an action such as
NetTargetIncrease requests an increase. All increases requested inside of this
interval are analyzed and if the highest requested target is greater than the
current target, this new target is set when the delay expires.

Some actions may set the fan target directly, such as one that increases the
target when a fan is removed, and this does not apply then.

Optional with a default of zero, meaning increases are immediately requested.

### decrease_interval

This throttles fan decreases to the specified delay.

The delay, in seconds, between fan target decreases, when an action such as
NetTargetDecrease requests a decrease. All increases and decreases requested
inside of this interval are analyzed and if the highest requested target is
lower than the current target, this new target is set when the delay expires.

Optional with a default of zero, meaning decreases are immediately requested.

### default_ceiling

The ceiling of the zone, i.e. the highest target that can be set.

This is optional and if not specified it defaults to the `poweron_target` value.
