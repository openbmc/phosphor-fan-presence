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
#include "test.hpp"

#include "../manager.hpp"
#include "../zone.hpp"
#include "group.hpp"

#include <nlohmann/json.hpp>

#include <iostream>

namespace phosphor::fan::control::json
{

using json = nlohmann::json;

Test::Test(const json& jsonObj, const std::vector<Group>& groups) :
    ActionBase(jsonObj, groups)
{}

void Test::run(Zone& zone)
{
    for (const auto& group : _groups)
    {
        for (const auto& member : group.getMembers())
        {
            std::cout << "Test::member - " << member << " : "
                      << group.getInterface() << " : " << group.getProperty()
                      << std::endl;
            try
            {
                auto value = Manager::getObjValueVariant(
                    member, group.getInterface(), group.getProperty());
                if (auto intPtr = std::get_if<int64_t>(&value))
                {
                    std::cout << "Test::value - " << *intPtr << std::endl;
                }
                else if (auto dblPtr = std::get_if<double>(&value))
                {
                    std::cout << "Test::value - " << *dblPtr << std::endl;
                }
                else if (auto boolPtr = std::get_if<bool>(&value))
                {
                    std::cout << "Test::value - " << std::boolalpha << *boolPtr
                              << std::endl;
                }
                else if (auto strPtr = std::get_if<std::string>(&value))
                {
                    std::cout << "Test::value - " << *strPtr << std::endl;
                }
                else
                {
                    std::cout << "Test::Unknown property value type"
                              << std::endl;
                }
            }
            catch (const std::out_of_range& oore)
            {
                std::cout << "Test::Member not found in cache" << std::endl;
            }
        }
    }
    std::cout << ">>> Test::run >>>" << std::endl;
    std::cout << "Zone::getDefaultFloor() = " << zone.getDefaultFloor()
              << std::endl;
    zone.setTarget(zone.getDefaultFloor());
    std::cout << "<<< Test::run <<<" << std::endl;
}

} // namespace phosphor::fan::control::json
