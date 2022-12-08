# methods

## Description

An array of method objects used to detect the fan's present state. Each
supported method requires its own set of attributes to be provided.

## Attribute Value(s)

Method `type`:

- ["tach"](#tach)
- ["gpio"](#gpio)

### "tach"

Requires an array of `sensors` of each fan tach D-Bus sensor name to be used to
determine fan presence. This is the name of the fan tach sensor located under
the `/xyz/openbmc_project/sensors/fan_tach/` D-Bus path.

```
"type": "tach",
"sensors": [
  "fan0_0"
]
```

### "gpio"

Detects fans with dedicated GPIOs using Linux
[gpio-keys](https://www.kernel.org/doc/Documentation/devicetree/bindings/input/gpio-keys.txt)
device tree bindings, where the event number is provided via the `key`
attribute.

```
"type": "gpio",
"key": 1,
"physpath": "/sys/bus/i2c/devices/1-0001",
"devpath": "/dev/input/by-path/platform-gpio-keys-polled-event"
```

## Example

<pre><code>
[
  {
    "name": "fan0",
    "path": "/system/chassis/motherboard/fan0",
    <b><i>"methods": [
      {
        "type": "tach",
        "sensors": [
          "fan0_0"
        ]
      }
    ]</i></b>,
    "rpolicy": {
      "type": "anyof"
    }"
  }
]
</code></pre>
