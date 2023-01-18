#pragma once
#include "types.hpp"

#include <string>

namespace sensor::monitor
{

class DomainEvent
{
  public:
    virtual ~DomainEvent() = default;
};

class SystemProtectionTriggered : public DomainEvent
{
  public:
    explicit SystemProtectionTriggered(const std::string& sensorPath) :
        sensorPath(sensorPath){};
    std::string getSensorPath()
    {
        return sensorPath;
    }

  private:
    const std::string sensorPath;
};

class SystemRecoveryTriggered : public DomainEvent
{
  public:
    explicit SystemRecoveryTriggered(const std::string& sensorPath,
                                     AlarmType type) :
        sensorPath(sensorPath),
        type(type){};
    std::string getSensorPath()
    {
        return sensorPath;
    }

    AlarmType getAlarmType()
    {
        return type;
    }

  private:
    const std::string sensorPath;
    const AlarmType type;
};

} // namespace sensor::monitor
