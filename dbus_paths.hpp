
#pragma once

// Global constants from Phosphor Fan Presence's D-Bus names and paths

// Control Application's D-Bus busname to own
static constexpr char CONTROL_BUSNAME[] = "xyz.openbmc_project.Control.Thermal";

// Control Application's root D-Bus object path
static constexpr char CONTROL_OBJPATH[] =
    "/xyz/openbmc_project/control/thermal";

// Thermal Application's D-Bus busname to own
static constexpr char THERMAL_ALERT_BUSNAME[] =
    "xyz.openbmc_project.Thermal.Alert";

// Thermal Application's root D-Bus object path
static constexpr char THERMAL_ALERT_OBJPATH[] =
    "/xyz/openbmc_project/alerts/thermal_fault_alert";
