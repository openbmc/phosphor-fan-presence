#pragma once
#include "domain_event_subscriber.hpp"
#include "domain_events.hpp"
#include "recovery_subscriber.hpp"

#include <map>
#include <vector>

namespace sensor::monitor
{

/**
 * @class DomainEventPublisher
 *
 * This class publishes DomainEvents triggered by sensor-monitor,
 * notify the Subscribers to handle the DomainEvent. It can be
 * used to decouple the dependency between AlarmHandlers if there
 * are some conditions need to be fulfilled to trigger specific handling.
 */
class DomainEventPublisher
{
  public:
    /**
     * @brief Get the instance of the Object
     */
    static DomainEventPublisher& instance()
    {
        static DomainEventPublisher theInstance;
        return theInstance;
    }

    /**
     * @brief Register a DomainEventSubscriber into this class.
     *
     * @param[in] subscriber - The DomainEventSubscriber who want to register
     */
    void subscribe(const std::shared_ptr<DomainEventSubscriber>& subscriber)
    {
        subscribers.emplace_back(subscriber);
    }

    /**
     * @brief Publish a DomainEvent
     *
     * @param[in] event = The published DomainEvent
     */
    void publish(DomainEvent& event)
    {
        for (auto& subscriber : subscribers)
        {
            subscriber->handle(event);
        }
    }

  private:
    /**
     * @brief The list of registered DomainEventSubscribers
     */
    std::vector<std::shared_ptr<DomainEventSubscriber>> subscribers;
};
} // namespace sensor::monitor
