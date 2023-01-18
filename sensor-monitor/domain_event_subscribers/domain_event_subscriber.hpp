#pragma once
#include "domain_events.hpp"

namespace sensor::monitor
{

class DomainEventSubscriber
{
  public:
    virtual ~DomainEventSubscriber() = default;
    virtual void handle(DomainEvent& event) = 0;
};

} // namespace sensor::monitor
