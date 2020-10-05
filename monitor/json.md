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
"inventory": "/xyz/openbmc_project/inventory/system/chassis/fan0"
```

Required.

### functional_delay

This contains the number of seconds a fan rotor needs to be out of spec before
the tach sensor, and possibly the fan FRU depending on the configuration, will
be set to nonfunctional.

```
"functional_delay": 5
```

Required.

### allowed_out_of_range_time

TODO

### deviation

TODO

### num_sensors_nonfunc_for_fan_nonunc

For fan FRUs that have more than 1 rotor and hence tach sensor, this defines
how many tach sensors in that fan FRU must be set to nonfunctional before the
fan FRU inventory object it also set to nonfunctional.

```
"num_sensors_nonfunc_for_fan_nonfunc": 1
```

This is optional and if not present then the code will not touch fan FRU
functional status.

### sensors

This is an array with an entry for each tach sensor contained in the fan FRU.

TODO

## Fault Handling Configuration

TODO
