# events.json

This file defines the events that dictate how fan control operates.  Each event
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

The trigger `init.get_properties` will, on fan control startup, read the
Present property on the `xyz.openbmc_project.Inventory.Item` interface from
every D-Bus object in the 'fan inventory' group, update those values in the
object cache, and then call the `count_state_before_target` action.

The trigger `signal.properties_changed` will watch for changes on the Present
property on each D-Bus object in the group, update the new value in the object
cache, and call the `count_state_before_target` action when the value changes.

The `count_state_before_target` action will look at the object cache value of
the Present property on each member of the group and set the fan target hold to
15000 when one or more of them is false.  Otherwise, it will clear its fan
target hold.

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
The name of a group that must be also be defined in
[groups.json](groups.md).

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
configured group.  After handling the signal, any configured actions are run.

```
{
    "class": "signal",
    "signal": "<signal>"
}
```

#### signal types
1. `properties_changed` - Subscribes to the PropertiesChanged signal for the
   D-Bus interface and property specified in the group definition for each
   group member.  When the signal occurs, the new property value will be added
   to or updated in the object cache.

2. `interfaces_added` - Subscribes to the InterfacesAdded signal for the D-Bus
   interface specified in the group definition for each group member.  When the
   signal occurs, the interface and its properties will be added to the object
   cache.

3. `interfaces_removed` - Subscribes to the InterfacesRemoved signal for the
   D-Bus interface specified in the group definition for each group member.
   When the signal occurs, the interface and properties will be removed from
   the object cache.

4. `name_owner_changed` - Subscribes to the NameOwnerChanged signal for the
   services that host the D-bus interface specified in the group definition for
   each group member.  When the signal occurs, the service owned state will be
   updated in the service cache map.

5. `member` - Subscribes to the signal listed on each group member.  No caches
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
PowerOn triggers run when the power turns on.  Functionally, they behave like
an init trigger.

```
{
    "class": "poweron",
    "method": "<method>"
}
```

#### method
The methods are the same as with the init trigger.

### poweroff

PowerOff triggers run when the power turns off.  Functionally, they behave like
an init trigger.

```
{
    "class": "poweroff",
    "method": "<method>"
}
```

#### method
The methods are the same as with the init trigger.
