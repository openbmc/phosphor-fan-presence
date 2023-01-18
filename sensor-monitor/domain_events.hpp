#pragma once
#include "types.hpp"

#include <string>

namespace sensor::monitor
{

class DomainEvent
{
  public:
    virtual ~DomainEvent() = default;
    virtual std::string getName()
    {
        return "Base DomainEvent";
    }
};

class SensorProtectionTriggered : public DomainEvent
{
  public:
    explicit SensorProtectionTriggered(const std::string& sensorPath) :
        _sensorPath(sensorPath){};
    std::string getSensorPath()
    {
        return _sensorPath;
    }

  private:
    const std::string _sensorPath;
};

class SensorRecoveryTriggered : public DomainEvent
{
  public:
    SensorRecoveryTriggered(const std::string& sensorPath, AlarmType type) :
        _sensorPath(sensorPath), _type(type){};
    std::string getSensorPath()
    {
        return _sensorPath;
    }

    AlarmType getAlarmType()
    {
        return _type;
    }

  private:
    const std::string _sensorPath;
    const AlarmType _type;
};

} // namespace sensor::monitor
