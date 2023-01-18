#pragma once
#include "domain_events.hpp"

namespace sensor::monitor
{
/**
 * @class DomainEventSubscriber
 *
 * This interface handles the DomainEvent send by DomainEventPublisher
 */
class DomainEventSubscriber
{
  public:
    virtual ~DomainEventSubscriber() = default;

    /**
     * @brief The interface for handling a DomainEvent
     */
    virtual void handle(DomainEvent& event) = 0;
};

} // namespace sensor::monitor
