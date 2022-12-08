# events.json

This file defines the events that dictate how fan control operates. Each event
can contain groups, triggers, and actions.

Actions are where fan targets are calculated and set, among other things.
Triggers specify when an action should run. Groups specify which D-Bus objects
the triggers and actions should operate on.

Some actions have modifiers, which help calculate a value.

- [Groups](#groups)
- [Triggers](#triggers)
- [Actions](#actions)
- [Modifiers](#modifiers)

## Example

```
[
   {
     "name": "fan(s) missing",
     "groups": [
       {
         "name": "fan inventory",
         "interface": "xyz.openbmc_project.Inventory.Item",
         "property": { "name": "Present" }
       }
     ],
     "triggers": [
       {
         "class": "init",
         "method": "get_properties"
       },
       {
         "class": "signal",
         "signal": "properties_changed"
       }
     ],
     "actions": [
       {
         "name": "count_state_before_target",
         "count": 1,
         "state": false,
         "target": 15000
       }
     ]
   }
]
```

The above event is an example of a method to set the fan target to 15000 when
one or more fans is missing. The basic behavior with this config is:

The trigger `init.get_properties` will, on fan control startup, read the Present
property on the `xyz.openbmc_project.Inventory.Item` interface from every D-Bus
object in the 'fan inventory' group, update those values in the object cache,
and then call the `count_state_before_target` action.

The trigger `signal.properties_changed` will watch for changes on the Present
property on each D-Bus object in the group, update the new value in the object
cache, and call the `count_state_before_target` action when the value changes.

The `count_state_before_target` action will look at the object cache value of
the Present property on each member of the group and set the fan target hold to
15000 when one or more of them is false. Otherwise, it will clear its fan target
hold.

## Groups

```
"groups": [
    {
        "name": "<name>",
        "interface": "<interface>",
        "property": { "name": "<name>" }
    }
    ...
]
```

### name

The name of a group that must be also be defined in [groups.json](groups.md).

### interface

The actions and triggers defined with this group will look at this D-Bus
interface on the members of this group.

### property: name

The actions and triggers defined with this group will look at this D-Bus
property on the members of this group.

## Triggers

There are several classes of triggers, and the JSON configuration is different
for each.

### init

Init triggers run when fan control events are enabled on fan control startup.
After invoking the configured method, any actions configured for this trigger
will run.

```
{
    "class": "init",
    "method": "<method>"
}
```

#### methods

1. `get_properties` - Read the property configured for the group from every
   member of the group, and store it in fan control's object cache.

2. `name_has_owner` - Populates the service owned state from D-Bus for each
   group member in fan control's D-Bus service cache map.

### signal

Signal triggers subscribe to certain D-Bus signals for each member of its
configured group. After handling the signal, any configured actions are run.

```
{
    "class": "signal",
    "signal": "<signal>"
}
```

#### signal types

1. `properties_changed` - Subscribes to the PropertiesChanged signal for the
   D-Bus interface and property specified in the group definition for each group
   member. When the signal occurs, the new property value will be added to or
   updated in the object cache.

2. `interfaces_added` - Subscribes to the InterfacesAdded signal for the D-Bus
   interface specified in the group definition for each group member. When the
   signal occurs, the interface and its properties will be added to the object
   cache.

3. `interfaces_removed` - Subscribes to the InterfacesRemoved signal for the
   D-Bus interface specified in the group definition for each group member. When
   the signal occurs, the interface and properties will be removed from the
   object cache.

4. `name_owner_changed` - Subscribes to the NameOwnerChanged signal for the
   services that host the D-bus interface specified in the group definition for
   each group member. When the signal occurs, the service owned state will be
   updated in the service cache map.

5. `member` - Subscribes to the signal listed on each group member. No caches
   are updated when the signal occurs.

### timer

Timer triggers run actions after the configured type of timer expires.

```
{
    "class": "timer",
    "type": "<type>",
    "interval": "<interval>",
    "preload_groups": "<true/false>"
}
```

#### type

1. `oneshot` - Starts a timer that runs once.

2. `repeating` - Starts a repeating timer.

#### interval

The timer length in microseconds

#### preload_groups

Optional, if set to true, will update the D-Bus properties from the configured
groups in the object cache after the timer expires but before any actions run.

### parameter

Parameter triggers run actions after a parameter changes.

```
{
    "class": "parameter",
    "parameter": "<parameter>"
}
```

#### parameter

The parameter value to watch.

### poweron

PowerOn triggers run when the power turns on. Functionally, they behave like an
init trigger.

```
{
    "class": "poweron",
    "method": "<method>"
}
```

#### method

The methods are the same as with the init trigger.

### poweroff

PowerOff triggers run when the power turns off. Functionally, they behave like
an init trigger.

```
{
    "class": "poweroff",
    "method": "<method>"
}
```

#### method

The methods are the same as with the init trigger.

## Actions

Actions can either operate on the groups listed with the event, or on the groups
listed within the action's JSON config.

Most actions set fan targets or floors. This can be done by requesting a target
value explicitly, or by requesting an increase or decrease delta. Targets and
floors can also be set with a hold, meaning another action can't set a
floor/target below the one that is held.

Some actions set or read a key/value pair called a parameter. These can be
created, updated, and deleted as necessary. For example, one action may
calculate and set a `floor_index` parameter, and another action may then read
that parameter to help choose a fan floor.

The available actions are:

- [set_net_increase_target](#set_net_increase_target)
- [set_net_decrease_target](#set_net_decrease_target)
- [count_state_floor](#count_state_floor)
- [count_state_before_target](#count_state_before_target)
- [default_floor_on_missing_owner](#default_floor_on_missing_owner)
- [mapped_floor](#mapped_floor)
- [set_target_on_missing_owner](#set_target_on_missing_owner)
- [override_fan_target](#override_fan_target)
- [pcie_card_floors](#pcie_card_floors)
- [set_request_target_base_with_max](#set_request_target_base_with_max)
- [set_parameter_from_group_max](#set_parameter_from_group_max)
- [target_from_group_max](#target_from_group_max)
- [call_actions_based_on_timer](#call_actions_based_on_timer)
- [get_managed_objects](#get_managed_objects)

### set_net_increase_target

Calculates the net target increase to be requested based on the value of each
property given within a group. The net target increase is based on the maximum
difference between the `delta` JSON value and all of the properties of the
group. The final result is the increase change that's requested to the current
target of a zone.

The group values can be compared to either a value hardcoded in the JSON, or a
parameter value.

```
{
    "name": "set_net_increase_target",
    "groups": [{
        "name": "pcie temps",
        "interface": "xyz.openbmc_project.Sensor.Value",
        "property": { "name": "Value" }
      }],
    "state": 70.0,
    "delta": 255
}
```

The above config uses a hardcoded state value:

1. For each member of the 'pcie temps' group:

- Read its 'Value' D-Bus property.
- If that property value is greater than the 'state' value of 70.0:
- Subtracts 70.0 from the property value.
- Multiplies that difference by the 'delta' value of 255.

2. Requests an increase of the largest calculated delta value, if there is one.

```
{
    "name": "set_net_increase_target",
    "groups": [{
        "name": "proc0 core temps",
        "interface": "xyz.openbmc_project.Sensor.Value",
        "property": { "name": "Value" }
      }],
    "state_parameter_name": "proc_0_core_dvfs_increase_temp",
    "delta": 300
}
```

The above config uses a parameter as the state value:

1. For each member of the 'proc 0 core temps' group:

- Read its 'Value' D-Bus property.
- If that property value is greater than the value of the parameter listed in
  the 'state_parameter_name' field, in this case
  'proc_0_core_dvfs_increase_temp':
- Subtracts that parameter value from the property value.
- Multiplies that difference by the 'delta' value of 300.

2. Requests an increase of the largest calculated delta value, if there is one.

### set_net_decrease_target

Calculates the net target decrease to be requested based on the value of each
property given within a group. The net target decrease is based on the minimum
difference between the `delta` JSON value and all properties in the group. The
final result is the decrease change that's requested to the current target of a
zone.

The group values can be compared to either a value hardcoded in the JSON, or a
parameter value.

```
{
    "name": "set_net_decrease_target",
    "groups": [{
        "name": "pcie temps",
        "interface": "xyz.openbmc_project.Sensor.Value",
        "property": { "name": "Value" }
      }],
    "state": 65.0,
    "delta": 80
}

```

The above config uses a hardcoded state value:

1. For each member of the 'pcie temps' group:

- Read its 'Value' D-Bus property.
- If that property value is less than the 'state' value of 65.0:
- Subtracts the property value from 65.0.
- Multiplies that difference by the 'delta' value of 80.

2. Requests a decrease of the smallest calculated delta value, if there is one.

```
{
    "name": "set_net_decrease_target",
    "groups": [{
        "name": "proc 0 core temps",
        "interface": "xyz.openbmc_project.Sensor.Value",
        "property": { "name": "Value" }
      }],
    "state_parameter_name": "proc_0_core_dvfs_decrease_temp",
    "delta": 50
}
```

The above config uses a parameter as the state value:

1. For each member of the 'proc 0 core temps' group:

- Read its 'Value' D-Bus property.
- If that property value is less than the value of the parameter listed the
  'state_parameter_name' field, in this case 'proc_0_core_dvfs_decrease_temp':
- Subtracts the property value from the parameter value.
- Multiplies that difference by the 'delta' value of 50.

2. Requests a decrease of the smallest calculated delta value, if there is one.

### count_state_floor

Sets the fans to a configured floor when a number of members within the group
are at a configured state. Once the number of members at the given state falls
below the configured count, the floor hold is released.

```
{
    "name": "count_state_floor",
    "count": 2,
    "state": false,
    "floor": 18000
}
```

The above config reads the configured D-Bus property on each group member
configured for the action. If two or more members have a property value of
false, a floor hold will be requested with a value of 18000. Otherwise, the
floor hold will be released (if it was previously requested).

### count_state_before_target

Sets the fans to a configured target when a number of members within the group
are at a configured state. Once the number of members at the given state falls
below the configured count, active fan target changes are allowed.

```
{
    "name": "count_state_before_target",
    "count": 1,
    "state": false,
    "target": 18000
}
```

The above config reads the configured D-Bus property on each group member
configured for the action. If one or more members have a property value of
false, a target hold will be requested with a value of 18000. Otherwise, the
hold will be released (if it was previously requested).

### default_floor_on_missing_owner

Sets the fan floor to the defined zone's default fan floor when a service
associated to a given group has terminated. Once all services are functional and
providing the sensors, the fan floor is allowed to be set normally again.

There is no additional JSON config for this action.

### mapped_floor

This action can be used to set a floor value based on 2 or more groups having
values within certain ranges, where the key group chooses the set of tables in
which to check the remaining group values.

```
{
  "name": "mapped_floor",
  "key_group": "ambient temp",
  "default_floor": 5555,
  "fan_floors": [
   {
     "key": 25,
     "default_floor": 4444,
     "floor_offset_parameter": "ambient_25_altitude_offset",
     "floors": [
       {
         "parameter": "pcie_floor_index",
         "floors": [
           { "value": 1, "floor": 2000 },
           { "value": 2, "floor": 3000 },
           { "value": 3, "floor": 4000 },
           { "value": 4, "floor": 5000 },
           { "value": 5, "floor": 6000 }
         ]
       },
       {
         "group": "power save",
         "floors": [
            { "value": true, "floor": 1000 }
         ]
       }
     ]
   }
}
```

The above config will use the maximum value of the 'ambient temp' group as the
key into the 'fan_floors' tables. There is one of those tables listed, and it
will be used when the key group has a max value of less than 25.

It will then traverse the contained floors arrays, keeping track of the highest
valid floor value it finds. If it doesn't find any valid floor values, it will
use the `default_floor` value of 4444, though that value is optional.

If no valid tables were found given the key value, the `default_floor` value of
5555 would be used, though that is optional and if not supplied the code would
default to the default floor of the zone.

At the end of the analysis, a floor hold will be set with the final floor value.

This action can also have a condition specified where a group property must
either match or not match a given value to determine if the action should run or
not. This requires the following in the JSON:

- "condition_group": The group name
  - For now, this group must just have a single member.
- "condition_op": Either "equal" or "not_equal"
- "condition_value": The value to check against

For example, the following says the single member of the 'cpu 0' group must have
its Model property be equal to "1234" for the action to run:

```
    "groups": [{
        "name": "cpu 0",
        "interface": "xyz.openbmc_project.Inventory.Decorator.Asset",
        "property": { "name": "Model" }
      }
      ...
    ],
    ...
    "name": "mapped_floor",
    "key_group": "ambient temp",
    "condition_group": "cpu 0",
    "condition_value": "1234",
    "condition_op": "equal",
    ...
```

### set_target_on_missing_owner

Sets the fans to a configured target when any service owner associated to the
group is missing. Once all services are functional and providing all the group
data again, active fan target changes are allowed.

```
{
    "name": "set_target_on_missing_owner",
    "groups": [{
        "name": "fan inventory",
        "interface": "xyz.openbmc_project.Inventory.Item",
        "property": { "name": "Present" }
      }],
    "target": 18000
}
```

The above config will set a target hold of 18000 when the service associated
with the 'fan inventory' group is lost.

### override_fan_target

This action locks fans at configured targets when the configured `count` amount
of fans meet criterion for the particular condition. A locked fan maintains its
override target until unlocked (or locked at a higher target). Upon unlocking,
it will either revert to temperature control or activate the next-highest target
remaining in its list of locks.

```
{
    "name": "override_fan_target",
    "count": 1,
    "state": false,
    "fans": [ "fan0", "fan1", "fan2", "fan3" ],
    "target": 10000
}
```

The above config will lock all fans in the fans array at a target of 10000 when
one or more members in its configured group have a group property value of
false.

This could be used for example, to lock the rotors of a multirotor fan to a high
target when one of its rotors has a functional property equal to false.

### pcie_card_floors

Sets the `pcie_floor_index` parameter based on the current configuration of
plugged and powered on PCIe cards, using data from the `pcie_cards.json` file.

It chooses the highest index from the active PCIe cards to set in the parameter.

It must be configured with the following groups and properties:

- The PCIe slots with the PowerState property
- The PCIe cards with the following properties: Function0DeviceId,
  Function0VendorId, Function0SubsystemId, Function0SubsystemVendorId

```
{
    "name": "pcie_card_floors",
    "use_config_specific_files": true,
    "settle_time": 2
}
```

The `use_config_specific_files` field tells the code to look for the
'pcie_cards.json' files in the same system specific directories as
'events.json'. If missing or false, looks in the base fan control directory.

The `settle_time` field is the amount of time in seconds that needs to pass
without a call to run() from a group property value changing. As the PCIeDevice
attributes are written close together by the host, this allows the action to
wait until the writes are done before selecting the index.

Additional details are in the
[header file](../../control/json/actions/pcie_card_floors.hpp)

### set_request_target_base_with_max

Determines the maximum value from the properties of the group of D-Bus objects
and sets the requested target base to this value. Only positive integer or
floating point types are supported as these are the only valid types for a fan
target to be based off of.

The `requested target base` value is the base value to apply a target delta to.
By default, it's the current zone target unless modified by this action.

```
{
    "name": "set_request_target_base_with_max",
    "groups": [{
        "name": "fan targets",
        "interface": "xyz.openbmc_project.Fan.Target",
        "property": { "name": "Target" }
      }]
}
```

The above config will set the requested target base to the maximum Target
property value of all members of the 'fan targets' group.

### set_parameter_from_group_max

Sets a parameter value based on the maximum group property value. The property
value can be modified before storing it if the JSON specifies a valid modifier
expression.

```
{
   "name": "set_parameter_from_group_max",
   "parameter_name": "proc_0_throttle_temp",
   "modifier": {
     "expression": "minus",
     "value": 4
   }
}
```

The above config will first find the max of its groups property values, subtract
4, and then store the resulting value in the `proc_0_throttle_temp` parameter.

### target_from_group_max

The action sets target of Zone to a value corresponding to the maximum value
from maximum group property value. The mapping is based on a provided table. If
there are more than one event using this action, the maximum speed derived from
the mapping of all groups will be set to the zone's target.

... { "name": "target_from_group_max", "groups": [ { "name": "zone0_ambient",
"interface": "xyz.openbmc_project.Sensor.Value", "property": { "name": "Value" }
} ], "neg_hysteresis": 1, "pos_hysteresis": 0, "map": [ { "value": 10.0,
"target": 38.0 }, ... ] }

The above JSON will cause the action to read the property specified in the group
"zone0_ambient" from all members of the group. The change in the group's members
value will be checked against "neg_hysteresis" and "pos_hysteresis" to decide if
it is worth taking action. "neg_hysteresis" is for the increasing case and
"pos_hysteresis" is for the decreasing case. The maximum property value in the
group will be mapped to the "map" to get the output "target". Each configured
event using this action will be provided with a key in a static map to store its
mapping result. The static map will be shared across the events of this action.
Therefore, the updated "target" value derived from "zone0_ambient" will be
stored in that static map with its own key. Each time it calls this action
running for each event, after the new value is updated to the static map, the
maximum value from it will be used to set to the Zone's target.

### call_actions_based_on_timer

This action starts and stops a timer that runs a list of actions whenever the
timer expires. A timer can be either `oneshot` or `repeating`.

When all groups have a configured value to compare against, that will be
compared against all members within each group to start/stop the timer. When all
group members have a given value and it matches what's in the cache, the timer
is started and if any do not match, the timer is stopped.

When any group does not have a configured value to be compared against, the
groups' service owned state is used to start/stop the timer. When any service
providing a group member is not owned, the timer is started and if all members'
services are owned, the timer is stopped.

Consider the following action config:

```
{
    "name": "call_actions_based_on_timer",
    "timer": {
        "interval": 5000000,
        "type": "oneshot"
    },
    "actions": [{
        "name": "test"
    }]
}

```

If its group configuration has a property value listed, like:

```
{
    "name": "fan inventory",
    "interface": "xyz.openbmc_project.Inventory.Item",
    "property": { "name": "Present", "value": true }
}
```

Then a oneshot timer of 5000000us will be started when every member of the fan
inventory group has a value of true. Otherwise, the timer will be stopped if
it's running.

If the group configuration has no property value listed, like:

```
{
    "name": "fan inventory",
    "interface": "xyz.openbmc_project.Inventory.Item",
    "property": { "name": "Present" }
}
```

Then the timer will be started when any service providing a group member isn't
owned (on D-Bus). Otherwise, it will stop the timer if it's running.

### get_managed_objects

This action adds the members of its groups to the object cache by using the
GetManagedObjects D-Bus method to find and add the results. When that is done,
it then runs any actions listed in the JSON.

This allows an action to run with the latest values in the cache without having
to subscribe to propertiesChanged for them all.

```
{
   "name": "get_managed_objects",
   "groups": [
     {
       "name": "proc temps",
       "interface": "xyz.openbmc_project.Sensor.Value",
       "property": { "name": "Value" }
     }
   ],
   "actions": [
     {
       "name": "set_net_increase_target",
       "state": 30,
       "delta": 100
     }
   ]
}
```

The above config will make the GetManagedObjects call on all services that own
the configured groups and then add all resulting property values to the object
cache. After that, it will call the `set_net_increase_target` action using the
same groups.

## Modifiers

Modifiers are used by some actions to help calculate values.

### minus

Subtract the `value` field from the passed in value.

```
"modifier": {
    "expression": "minus",
    "value": 4
}
```

The above config subtracts 4 from what is passed to it.

### less_than

Returns a value from a data table that is selected when the argument passed in
is less than the `arg_value` entry in the table row. If there is a
`default_value` field supplied, then that will be returned if the argument is
greater than the `arg_value` of the last row.

```
"modifier": {
  "operator": "less_than",
  "default_value": 10000,
  "value": [
      { "arg_value": 500, "parameter_value": 1 },
      { "arg_value": 1000, "parameter_value": 2 },
      { "arg_value": 1500, "parameter_value": 3 }
  ]
}
```

The above config returns 1 if the arg passed is less than 500, 2 if less than
1000, and 3 if less than 1500. Otherwise returns 10000, the default value.
