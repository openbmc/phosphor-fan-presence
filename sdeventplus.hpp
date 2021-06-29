/**
 * Copyright © 2021 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <sdeventplus/event.hpp>

namespace phosphor::fan::util
{

/** @class SDEventPlus
 *  @brief Event access delegate implementation for sdeventplus.
 */
class SDEventPlus
{
  public:
    /**
     * @brief Get the event instance
     */
    static auto& getEvent() __attribute__((pure))
    {
        static auto event = sdeventplus::Event::get_default();
        return event;
    }
};

} // namespace phosphor::fan::util
