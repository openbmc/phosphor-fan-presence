# Fan Monitor

The fan monitor application provides the following functionality:

- Sets the functional state of fan sensors in the inventory based on comparing
  the target speed of the sensor to the actual speed.
- Sets the functional states of fan FRUs in the inventory based on the child
  fan sensor functional states.
- Creates event logs for fan sensors that have been nonfunctional for too long.
- Creates event logs for fans that have been missing for too long.
- Can power off the system based on rules that look at the number of
  nonfunctional fan sensors or missing fans.

This is all configurable in a [JSON file](json.md).  It also supports build
time configuration based on a [YAML file](example/monitor.yaml), though that
doesn't support the full functionality.
