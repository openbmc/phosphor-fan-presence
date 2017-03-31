#include <sdbusplus/bus.hpp>
//#include <libevdev.h>//FIXME
#include "cooling_type.hpp"

using namespace phosphor::chassis::cooling;

/*
 CoolingType::CoolingType(sdbusplus::bus::bus& bus, std::string gpioPath)
{
    airCooled = false;
    waterCooled = false;
    *gpioDev = NULL;
    gpiofd = open(gpioPath, O_RDONLY);
    if (gpiofd < 0)
    {
        perror("Failed to open GPIO file device %s", gpioPath);
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
*/

CoolingType::~CoolingType()
{
    if (gpiofd > 0)
    {
        close(gpiofd);
    }

    //libevdev_free(gpioDev);//FIXME
}

bool CoolingType::getAirCooled()
{
    /*
       bool airCooled = getProperty(bus, CHASSIS_PATH,
       CHASSIS_BUSNAME,
       "AirCooled");
       */
    return airCooled;
}

void CoolingType::setAirCooled(bool)
{
    // get current state of property from bus, path, name...
    // if does not match what read from device, then
    //     setProperty(bus, ..., "AirCooled");
}

bool CoolingType::getWaterCooled()
{
    /*
       bool waterCooled = getProperty(bus, CHASSIS_PATH,
       CHASSIS_BUSNAME,
       "WaterCooled");
       */
    return waterCooled;
}

void CoolingType::setWaterCooled(bool)
{
    // get current state of property from bus, path, name...
    // if does not match what read from device, then
    //     setProperty(bus, ..., "WaterCooled");
}

int main(int argc, char* argv[])
{
    int rc = -1;
    // Path string to the GPIO "file" device we are going to read.
    const char* gpiopath;

    printf("%s - argc = %d\n", argv[0], argc);
    if (argc < 1)
    {
        fprintf(stderr, "Too few arguments\n");
    }
    else
    {
        gpiopath = argv[1];
        auto bus = sdbusplus::bus::new_default();
        //std::unique_ptr<phosphor::chassis::cooling::CoolingType> coolingType(bus,
        //        gpiopath);
        CoolingType coolingType(bus, gpiopath);

        rc = 0;
    }

    return rc;
}
