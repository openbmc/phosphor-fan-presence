#include "malfunction_monitor.hpp"

#include "logging.hpp"
#include "system.hpp"

#include <gpiod.hpp>

namespace phosphor::fan::monitor
{

static bool isPowerStateOn()
{
    try
    {
        auto state = util::SDBusPlus::getProperty<int32_t>(
            "/org/openbmc/control/power0", "org.openbmc.control.Power",
            "state");
        return static_cast<bool>(state);
    }
    catch (const std::exception& e)
    {
        getLogger().log(std::format("Failed reading power state: {}", e.what()),
                        Logger::Priority::error);
    }
    return true;
}

MalfunctionMonitor::MalfunctionMonitor(System& system, double limit,
                                       std::string_view resetGPIO) :
    system(system), tachLimit(limit), resetGPIO(resetGPIO)
{}

bool MalfunctionMonitor::checkAndAttemptRecovery(const TachSensor& sensor)
{
    if (!malfunctionDetected(sensor))
    {
        return false;
    }

    if (!affectedFans.contains(sensor.getFan().getName()))
    {
        // Don't proceed if a new sensor is caught but the power state is
        // off since the chip can emit false positives right when power is
        // being cut.  If it's on a fan already in affectedFans, then we'll
        // have already done the reset and won't do another one anyway.
        // The PowerState class in System isn't fast enough to catch this
        // since PGood is still up.
        if (!isPowerStateOn())
        {
            return false;
        }

        affectedFans.insert(sensor.getFan().getName());
    }

    if (resetDone)
    {
        return false;
    }

    getLogger().log(
        std::format("FanCtlr malfunction detected. Tach {} value {} "
                    "is over limit.",
                    sensor.name(), sensor.getInput()),
        Logger::Priority::error);

    system.prepForCtlrReset();
    resetFanController();
    logResetError(sensor);
    resetDone = true;

    return true;
}

void MalfunctionMonitor::resetFanController()
{
    getLogger().log("Resetting fan controller to recover",
                    Logger::Priority::error);
    try
    {
        using namespace std::chrono_literals;

        auto line = gpiod::find_line(resetGPIO);

        gpiod::line_request config{__FUNCTION__,
                                   gpiod::line_request::DIRECTION_OUTPUT,
                                   gpiod::line_request::FLAG_OPEN_DRAIN};
        line.request(config, 0);
        line.release();
        std::this_thread::sleep_for(10ms);
        line.request(config, 1);
        line.release();
    }
    catch (const std::exception& e)
    {
        getLogger().log(
            std::format("GPIO error while resetting fan controller: {}",
                        e.what()),
            Logger::Priority::error);
    }
}

void MalfunctionMonitor::logResetError(const TachSensor& sensor) const
{
    using Severity =
        sdbusplus::xyz::openbmc_project::Logging::server::Entry::Level;

    FanError error{"xyz.openbmc_project.Fan.Error.CtlrReset", "", sensor.name(),
                   Severity::Informational};

    auto sensorData = system.captureSensorData();
    error.commit(sensorData);
}

} // namespace phosphor::fan::monitor
