# rpolicy

## Description

The type of redundancy policy to use against the methods of presence detection.
When there are more than one method of detecting a fan, the policy in which to
use those methods can be configured based on system expectations.

## Attribute Value(s)

Policy `type`:

- ["anyof"](#anyof)
- ["fallback"](#fallback)

### "anyof"

A policy where if any method of fan presence detection denotes the fan is
present, results in the fan being marked present. All methods of detection must
denote the fan as missing to have a fan be not present.

```
"type": "anyof"
```

### "fallback"

A policy to use the first method of fan presence detection and then use the next
method only when the first method denotes the fan is missing.

```
"type": "fallback"
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
    <b><i>"rpolicy": {
      "type": "anyof"
    }"</i></b>
  }
]
</code></pre>
