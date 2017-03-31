#pragma once
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
//#include <libevdev.h>//FIXME

namespace phosphor
{
namespace chassis
{
namespace cooling
{
constexpr auto CHASSIS_PATH = "/xyz/openbmc_project/inventory/system/chassis";
constexpr auto CHASSIS_BUSNAME = "xyz.openbmc_project.Inventory.System.Chassis";

class CoolingType
{
    public:
        CoolingType() = delete;
        //~CoolingType() = default;
        ~CoolingType();

        /**
         * @brief Constructs Cooling Type Object
         *
         * @param[in] bus - Dbus bus object
         * @param[in] gpioKey - GPIO to read ...
         */
        CoolingType(sdbusplus::bus::bus& bus,
                    const char* gpioPath)
        {
            airCooled = false;
            waterCooled = false;
            //gpioDev = nullptr;//FIXME
            gpiofd = open(gpioPath, O_RDONLY);
            if (gpiofd < 0)
            {
                perror("Failed to open GPIO file device");
                //perror("Failed to open GPIO file device %s", gpioPath);
            }
            else
            {
                int rc = 0;//libevdev_new_from_fd(gpiofd, &gpioDev);//FIXME
                if (rc < 0)
                {
                    fprintf(stderr, "Failed to get file descriptor for %s (%s)\n", gpioPath,
                            strerror(-rc));
                }

                //TODO - more to go here?
            }
        }

        bool getAirCooled();
        void setAirCooled(bool);
        bool getWaterCooled();
        void setWaterCooled(bool);

    private:
        // File descriptor for the GPIO file we are going to read.
        int gpiofd;
        //struct libevdev* gpioDev;
        bool airCooled = false;
        bool waterCooled = false;
};

}
}
}
