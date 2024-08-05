# name

## Description

A unique identifying name for the fan.

## Attribute Value(s)

string

## Example

```json
[
  {
    "name": "fan0",
    "path": "/system/chassis/motherboard/fan0",
    "methods": [
      {
        "type": "tach",
        "sensors": ["fan0_0"]
      }
    ],
    "rpolicy": {
      "type": "anyof"
    }
  }
]
```
