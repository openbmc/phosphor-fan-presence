#pragma once


namespace phosphor
{
namespace fan
{
namespace presence
{

// Forward declare FanEnclosure
class FanEnclosure;
/**
 * @class Sensor
 * @brief Base sensor implementation to be extended
 * @details A type of presence detection sensor would extend this to override
 * how presences is determined by the fan enclosure containing that type
 */
class Sensor
{
    public:
        Sensor() = delete;
        Sensor(const Sensor&) = delete;
        Sensor(Sensor&&) = delete;
        Sensor& operator=(const Sensor&) = delete;
        Sensor& operator=(Sensor&&) = default;
        virtual ~Sensor() = default;

        /**
         * @brief Constructs Sensor Object
         *
         * @param[in] id - ID name of this sensor
         * @param[in] fanEnc - Reference to the fan enclosure with this sensor
         */
        Sensor(const std::string& id,
               std::shared_ptr<phosphor::fan::presence::FanEnclosure> fanEnc) :
            id(id),
            fanEnc(std::move(fanEnc))
        {
            //Nothing to do here
        }

        /**
         * @brief Presence function that must be implemented within the derived
         * type of sensor's implementation on how presence is determined
         */
        virtual bool isPresent() = 0;

    protected:
        /** @brief ID name of this sensor */
        const std::string id;
        /** @brief Pointer to the fan enclosure containing this sensor */
        std::shared_ptr<phosphor::fan::presence::FanEnclosure> fanEnc;

};

} // namespace presence
} // namespace fan
} // namespace phosphor
