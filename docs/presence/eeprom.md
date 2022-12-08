# eeprom

## Description

Some fans have EEPROMs on them. In these cases, the `eeprom` JSON stanza can be
used to have the application re-bind the EEPROM driver to the EEPROM instance
after a new fan is detected. This will trigger the EEPROM to be read by the
appropriate code if the platform is configured to do so.

This is optional and only required if the above behavior is desired.

## Keys

- ["bus_address"](#bus_address)
- ["driver_name"](#driver_name)
- ["bind_delay_ms"](#bind_delay_ms)

### "bus_address"

The I2C bus and address string of the form BB-AAAA that is used by the I2C
subsystem in sysfs.

```
"bus_address": "3-0050"
```

### "driver_name"

The name of the eeprom driver in sysfs.

```
"driver_name": "at24"
```

### "bind_delay_ms"

The number of milliseconds to wait after a fan is detected before binding the
driver to the device in case the device takes some time to initialize after
being plugged into power. If no delay is required, a value of zero can be used.

```
"bind_delay_ms": 1000
```

## Example

<pre><code>
[
  {
    "name": "fan0",
    "path": "/system/chassis/motherboard/fan0",
    "methods": [
      {
        "type": "tach",
        "sensors": [
          "fan0_0"
        ]
      }
    ],
    "rpolicy": {
      "type": "anyof"
    },
    <b><i>"eeprom": {
      "bus_address": "30-0050",
      "driver_name": "at24",
      "bind_delay_ms": 1000
    }</i></b>
  }
]
</code></pre>
