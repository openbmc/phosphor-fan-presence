# groups.json

This file contains an array of group definitions. In this usage, a group refers
to a group of D-Bus objects that all of the same D-Bus path and interface, and
are acted upon by actions and triggers in events.json. Only the D-Bus objects
paths are specified in this file, the interface and properties are specified at
the time of usage in events.json.

## JSON example

```
[
  {
     "name": "occ objects",
     "service": "org.open_power.OCC.Control",
     "members": [
       "/org/open_power/control/occ0",
       "/org/open_power/control/occ1"
     ]
   }
]
```

## Attributes

### members

An array of the D-Bus object paths that are in the group. Required.

### name

The group name. This is how the group is referenced in events.json. Required.

### service

If known, the D-Bus service name that provides these D-bus objects. All members
of the group must be hosted by the same service for this to be used. This
provides performance improvements in certain cases. Optional.
