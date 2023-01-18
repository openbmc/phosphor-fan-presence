#pragma once
#include "domain_event_subscriber.hpp"
#include "domain_events.hpp"
#include "recovery_subscriber.hpp"

#include <map>
#include <vector>

namespace sensor::monitor
{
class DomainEventPublisher
{
  public:
    static DomainEventPublisher& instance()
    {
        static DomainEventPublisher theInstance;
        return theInstance;
    }

    void subscribe(const std::shared_ptr<DomainEventSubscriber>& subscriber)
    {
        subscribers.emplace_back(subscriber);
    }

    void publish(DomainEvent& event)
    {
        for (auto& subscriber : subscribers)
        {
            subscriber->handle(event);
        }
    }

  private:
    std::vector<std::shared_ptr<DomainEventSubscriber>> subscribers;
};
} // namespace sensor::monitor
