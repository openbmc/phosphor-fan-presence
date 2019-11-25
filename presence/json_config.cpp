/**
 * Copyright © 2019 IBM Corporation
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
#include <string>

#include "json_config.hpp"

namespace phosphor
{
namespace fan
{
namespace presence
{

policies JsonConfig::_policies;

JsonConfig::JsonConfig(const std::string& jsonFile)
{

}

const policies& JsonConfig::get()
{
    return _policies;
}

} // namespace presence
} // namespace fan
} // namespace phosphor
