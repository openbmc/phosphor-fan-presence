# Sensor Monitor

This application can take action based on sensor thresholds and values.

## Available Monitors

### ShutdownAlarmMonitor

This monitor will watch all instances of the
`xyz.openbmc_project.Sensor.Threshold.HardShutdown` and
`xyz.openbmc_project.Sensor.Threshold.SoftShutdown` D-Bus interfaces for the
alarm properties to trip.  When this happens, a configurable timer is started
at the end of which a power off is executed if the alarms haven't reset by that
time.  Event logs will also be created.

The configuration options `SHUTDOWN_ALARM_HARD_SHUTDOWN_DELAY_MS` and
`SHUTDOWN_ALARM_SOFT_SHUTDOWN_DELAY_MS` can be used to change the delays.

Currently, this monitor is enabled by default.
