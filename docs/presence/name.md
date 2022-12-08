# name

## Description

A unique identifying name for the fan.

## Attribute Value(s)

string

## Example

<pre><code>
[
  {
    <b><i>"name": "fan0"</i></b>,
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
    }"
  }
]
</code></pre>
