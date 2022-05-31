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
