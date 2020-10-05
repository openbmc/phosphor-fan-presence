# JSON Configuration

The JSON config for the fan monitor application is broken up into 2 sections -
[fan configuration](#fan-configuration) and [fault handling
configuration](#fault-handling-configuration).

## Fan Configuration

Fans are individually configured in an array:

```
"fans": [
    {
        // Configuration entries
    },

    ...
]
```

The available configuration entries are:

### inventory

This contains the inventory D-Bus path of the fan.

```
"inventory": "/system/chassis/fan0"
```

Required.

### monitor_start_delay

This is the number of seconds to wait after system PGOOD before the tach
feedback sensors will start being compared to their targets to look for faults.
This gives the fans a chance to ramp up after a cold start.

```
"monitor_start_delay": 30
```

This is optional and defaults to 30s.

### functional_delay

This contains the number of seconds a fan rotor needs to be within spec before
it is set back to functional from nonfunctional.

```
"functional_delay": 5
```

This is optional and defaults to zero.

### allowed_out_of_range_time

This contains the number of seconds a fan rotor needs to be out of spec before
the tach sensor, and possibly the fan FRU depending on the configuration, will
be set to nonfunctional.

### deviation

TODO

### num_sensors_nonfunc_for_fan_nonfunc

For fan FRUs that have more than 1 rotor and hence tach sensor, this defines
how many tach sensors in that fan FRU must be set to nonfunctional before the
fan FRU inventory object it also set to nonfunctional.

```
"num_sensors_nonfunc_for_fan_nonfunc": 1
```

This is optional and if not present then the code will not touch fan FRU
functional status.

### nonfunc_rotor_error_delay

This defines how many seconds a fan rotor must be nonfunctional before an error
will be created.

```
"nonfunc_rotor_error_delay": 1
```

Optional.  If not present but there is a [fault handling
configuration](#fault-handling-configuration) section, then it defaults to 0.

### sensors

This is an array with an entry for each tach sensor contained in the fan FRU.

TODO

## Fault Handling Configuration

The fault handling configuration section is optional and defines how fan faults
are handled.  If not present, then event logs will not be created when fans
become nonfunctional after the
'[nonfunc_rotor_error_delay](#nonfunc_rotor_error_delay)' time, otherwise they
will be.

```
"fault_handling":
{


}
```

It contains the following properties.

### num_nonfunc_rotors_before_error

This specifies how many nonfunctional fan rotors there must be at the same time
before an event log with an error severity is created for the rotor.  When
there are fewer than this many nonfunctional rotors, then event logs with an
informational severity will be created.

For example, if this number is 2, when the first fan rotor becomes
nonfunctional an informational event log will be created.  Then when the second
fan rotor becomes nonfunctional while the original one is still nonfunctional
an event log with an error severity will be created.

```
"num_nonfunc_rotors_before_error": 2
```

This is optional and defaults to 1 if there is a 'fault_handling' section,
meaning the first nonfunctional rotor will result in an error severity event
log being created.

### power_off_config

This is an array of entries where each entry has a cause and type of power off
along with related settings.  Entries become active when they are triggered by
their cause, which starts one or more delays, followed by the shutdown type
specified.  More than one entry can be active at the same time, where the first
entry that finishes the delay will be the one to shut the system down.

This entry is optional.  If not specified, then no power offs will be done.

```
"power_off_config":
[
    {
        // power off configuration
        "type": "hard", "soft", or "epow"
        ...
    }
    ...
]
```

There are 2 formats of power off entries - [hard/soft](#hard-or-soft-power-off)
and [epow](#epow-power-off), which stands for early power off warning.  The
format is selected by the 'type' property, which is either 'hard', 'soft', or
'epow'.

#### Hard Or Soft Power Off

The hard/soft entry has the required properties 'cause', 'count', 'delay', and
'type'.  It has one optional property - 'state'.

##### cause

When the cause and the count conditions are satisfied, the timer to invoke the
shutdown is started.  This property can have the following values:

* missing_fan_frus - There must greater than or equal to the configured
    [count](#count) number of missing fan FRUs.
* nonfunc_fan_rotors - There must be greater than or equal to the configured
    [count](#count) number of nonfunctional fan rotors.  Though there is no
    present/missing property for a rotor, if the rotor is indeed missing it is
    considered nonfunctional.

##### count

This specifies the number of FRUs/rotors with the status specified by the cause
field for the power off timer to start.  For example, if cause is
'nonfunc_fan_rotors' and this field is 3, then the shutdown timer starts when
there are 3 or more nonfunctional fan rotors.

##### delay

This is the delay in seconds between the cause condition triggering and the
shutdown being requested.  If the cause becomes false while the delay is in
progress, such as if a fan is replaced, then the power off is canceled.

##### type

This is the type of power off to do, either 'hard' or 'soft'.  A hard power off
is an immediate power off.  A soft power off is where the BMC asks the hosts to
handle the power off process, with some built in timeout where if the host
hasn't shut down by then then the BMC will do it.

##### state

This says when the entry is valid, and is either 'at_pgood', or 'runtime'.  The
'at_pgood' value means the entry is only valid at the point when the power
state changes from off to on.  The 'runtime' value means the entry is valid at
any time that the power state is on, which includes when the 'at_pgood' rules
are checked.

The fan code always starts with all fans functional at power on, so it is
really only useful to have the cause be 'missing_fan_frus' when 'at_pgood' is
used.

```
[
    {
        "type": "hard"
        "cause": "missing_fan_frus",
        "count": 2,
        "delay": 0,
        "state": "at_pgood"
    },
    {
        "type": "soft"
        "cause": "nonfunc_fan_rotors",
        "count": 3,
        "delay": 60,
    },
    {
        "type": "hard"
        "cause": "nonfunc_fan_rotors",
        "count": 3,
        "delay": 120,
    }
]
```

The first entry states that when the power state changes to on, immediately do
a hard power off if there are 2 or more missing fan FRUs.

The second entry above states that as soon as there are three nonfunctional fan
rotors, it will start a 60 second timer. When that expires, do a soft power
off.  If one of those nonfunctional fan rotors becomes functional before the 60
seconds has passed, cancel the timer and don't request the power off.

The third entry also looks for 3 nonfunctional fan rotors, and will do a hard
power off after 120 seconds.  This acts like a failsafe, shutting the system
off if the host didn't respond to the soft power off request in time.

#### EPOW Power Off

The EPOW format of the power off event has the same 'cause' and 'count' fields
as the hard/soft format, but introduces two different delay fields -
'service_mode_delay' and 'meltdown_delay':

##### service_mode_delay

A timer is started with this delay upon the 'cause' and 'count' conditions
being satisfied. This is the window for service to be done to the machine
without any actions being taken, so if the condition is resolved within
this time the power off will be canceled.

##### meltdown_delay

A timer with this delay is started as soon as the service mode timer expires.
When this timer expires, an immediate power off will be done.

In addition to starting this timer, a D-Bus property will be set (TBD)
indicating there is an impending shutdown due to fans.  On systems that support
it, such as IBM systems that run the PowerVM hypervisor, this would trigger a
notification via the hypervisor to the OS that there is a thermal fault and
that an orderly power off should be done.  This does not include sending the
regular [soft power off](#type) request.

Resolving fan errors during this time will not stop the immediate power off.
The only way it won't occur is if the previous request to the host to power off
the system happens first.

```
[
    {
        "type": "epow"
        "cause": "nonfunc_fan_rotors",
        "count": 3,
        "service_mode_delay": 300,
        "meltdown_delay": 500
    }
]
```

The entry above states that as soon as there are 3 nonfunctional fan rotors, a
300 second service mode timer will be started to allow time for the fans to be
replaced.  At the end of this time, a 500 second timer will be started, at the
end of which the system will be immediately powered off.
