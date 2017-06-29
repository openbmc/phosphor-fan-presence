#!/usr/bin/env python

"""
This script reads in fan definition and zone definition YAML
files and generates a set of structures for use by the fan control code.
"""

import os
import sys
import yaml
from argparse import ArgumentParser
from mako.template import Template

tmpl = '''/* This is a generated file. */
#include <sdbusplus/bus.hpp>
#include "manager.hpp"
#include "functor.hpp"
#include "actions.hpp"
#include "handlers.hpp"

using namespace phosphor::fan::control;
using namespace sdbusplus::bus::match::rules;

const unsigned int Manager::_powerOnDelay{${mgr_data['power_on_delay']}};

const std::vector<ZoneGroup> Manager::_zoneLayouts
{
%for zone_group in zones:
    ZoneGroup{
        std::vector<Condition>{
        %for condition in zone_group['conditions']:
            Condition{
                "${condition['type']}",
                std::vector<ConditionProperty>{
                %for property in condition['properties']:
                    ConditionProperty{
                        "${property['property']}",
                        "${property['interface']}",
                        "${property['path']}",
                        static_cast<${property['type']}>(${property['value']}),
                    },
                    %endfor
                },
            },
            %endfor
        },
        std::vector<ZoneDefinition>{
        %for zone in zone_group['zones']:
            ZoneDefinition{
                ${zone['num']},
                ${zone['full_speed']},
                ${zone['default_floor']},
                ${zone['increase_delay']},
                ${zone['decrease_interval']},
                std::vector<FanDefinition>{
                %for fan in zone['fans']:
                    FanDefinition{
                        "${fan['name']}",
                        std::vector<std::string>{
                        %for sensor in fan['sensors']:
                            "${sensor}",
                        %endfor
                        }
                    },
                %endfor
                },
                std::vector<SetSpeedEvent>{
                %for event in zone['events']:
                    SetSpeedEvent{
                        Group{
                        %for member in event['group']:
                        {
                            "${member['name']}",
                            {"${member['interface']}",
                             "${member['property']}"}
                        },
                        %endfor
                        },
                        make_action(action::${event['action']['name']}(
                        %for i, p in enumerate(event['action']['parameters']):
                        %if (i+1) != len(event['action']['parameters']):
                            static_cast<${p['type']}>(${p['value']}),
                        %else:
                            static_cast<${p['type']}>(${p['value']})
                        %endif
                        %endfor
                        )),
                        std::vector<PropertyChange>{
                        %for s in event['signal']:
                            PropertyChange{
                                interface("org.freedesktop.DBus.Properties") +
                                member("PropertiesChanged") +
                                type::signal() +
                                path("${s['path']}") +
                                arg0namespace("${s['interface']}"),
                                make_handler(propertySignal<${s['type']}>(
                                    "${s['interface']}",
                                    "${s['property']}",
                                    handler::setProperty<${s['type']}>(
                                        "${s['member']}",
                                        "${s['interface']}",
                                        "${s['property']}"
                                    )
                                ))
                            },
                        %endfor
                        }
                    },
                %endfor
                }
            },
        %endfor
        }
    },
%endfor
};
'''


def convertToMap(listOfDict):
    """
    Converts a list of dictionary entries to a std::map initialization list.
    """
    listOfDict = listOfDict.replace('[', '{')
    listOfDict = listOfDict.replace(']', '}')
    listOfDict = listOfDict.replace(':', ',')
    return listOfDict


def getEventsInZone(zone_num, zone_conditions, events_data):
    """
    Constructs the event entries defined for each zone using the events yaml
    provided.
    """
    events = []

    if 'events' in events_data:
        for e in events_data['events']:

            # Zone numbers are optional in the events yaml but skip if this
            # zone's zone number is not in the event's zone numbers
            if all('zones' in z and z['zones'] is not None and
                   zone_num not in z['zones'] for z in e['zone_conditions']):
                continue

            # Zone conditions are optional in the events yaml but skip if this
            # event's condition is not in this zone's conditions
            if all('name' in z and z['name'] is not None and
                   not any(c['name'] == z['name'] for c in zone_conditions)
                   for z in e['zone_conditions']):
                continue

            event = {}
            # Add set speed event group
            group = []
            groups = next(g for g in events_data['groups']
                          if g['name'] == e['group'])
            for member in groups['members']:
                members = {}
                members['type'] = groups['type']
                members['name'] = ("/xyz/openbmc_project/" +
                                   groups['type'] +
                                   member)
                members['interface'] = e['interface']
                members['property'] = e['property']['name']
                group.append(members)
            event['group'] = group

            # Add set speed action and function parameters
            action = {}
            actions = next(a for a in events_data['actions']
                           if a['name'] == e['action']['name'])
            action['name'] = actions['name']
            params = []
            for p in actions['parameters']:
                param = {}
                if type(e['action'][p]) is not dict:
                    if p == 'property':
                        param['value'] = str(e['action'][p]).lower()
                        param['type'] = str(e['property']['type']).lower()
                    else:
                        # Default type to 'size_t' when not given
                        param['value'] = str(e['action'][p]).lower()
                        param['type'] = 'size_t'
                    params.append(param)
                else:
                    param['type'] = str(e['action'][p]['type']).lower()
                    if p != 'map':
                        param['value'] = str(e['action'][p]['value']).lower()
                    else:
                        emap = convertToMap(str(e['action'][p]['value']))
                        param['value'] = param['type'] + emap
                    params.append(param)
            action['parameters'] = params
            event['action'] = action

            # Add property change signal handler
            signal = []
            for path in group:
                signals = {}
                signals['path'] = path['name']
                signals['interface'] = e['interface']
                signals['property'] = e['property']['name']
                signals['type'] = e['property']['type']
                signals['member'] = path['name']
                signal.append(signals)
            event['signal'] = signal

            events.append(event)

    return events


def getFansInZone(zone_num, profiles, fan_data):
    """
    Parses the fan definition YAML files to find the fans
    that match both the zone passed in and one of the
    cooling profiles.
    """

    fans = []

    for f in fan_data['fans']:

        if zone_num != f['cooling_zone']:
            continue

        # 'cooling_profile' is optional (use 'all' instead)
        if f.get('cooling_profile') is None:
            profile = "all"
        else:
            profile = f['cooling_profile']

        if profile not in profiles:
            continue

        fan = {}
        fan['name'] = f['inventory']
        fan['sensors'] = f['sensors']
        fans.append(fan)

    return fans


def getConditionInZoneConditions(zone_condition, zone_conditions_data):
    """
    Parses the zone conditions definition YAML files to find the condition
    that match both the zone condition passed in.
    """

    condition = {}

    for c in zone_conditions_data['conditions']:

        if zone_condition != c['name']:
            continue
        condition['type'] = c['type']
        properties = []
        for p in c['properties']:
            property = {}
            property['property'] = p['property']
            property['interface'] = p['interface']
            property['path'] = p['path']
            property['type'] = p['type'].lower()
            property['value'] = str(p['value']).lower()
            properties.append(property)
        condition['properties'] = properties

        return condition


def buildZoneData(zone_data, fan_data, events_data, zone_conditions_data):
    """
    Combines the zone definition YAML and fan
    definition YAML to create a data structure defining
    the fan cooling zones.
    """

    zone_groups = []

    for group in zone_data:
        conditions = []
        # zone conditions are optional
        if 'zone_conditions' in group and group['zone_conditions'] is not None:
            for c in group['zone_conditions']:

                if not zone_conditions_data:
                    sys.exit("No zone_conditions YAML file but " +
                             "zone_conditions used in zone YAML")

                condition = getConditionInZoneConditions(c['name'],
                                                         zone_conditions_data)

                if not condition:
                    sys.exit("Missing zone condition " + c['name'])

                conditions.append(condition)

        zone_group = {}
        zone_group['conditions'] = conditions

        zones = []
        for z in group['zones']:
            zone = {}

            # 'zone' is required
            if ('zone' not in z) or (z['zone'] is None):
                sys.exit("Missing fan zone number in " + zone_yaml)

            zone['num'] = z['zone']

            zone['full_speed'] = z['full_speed']

            zone['default_floor'] = z['default_floor']

            # 'increase_delay' is optional (use 0 by default)
            key = 'increase_delay'
            zone[key] = z.setdefault(key, 0)

            # 'decrease_interval' is optional (use 0 by default)
            key = 'decrease_interval'
            zone[key] = z.setdefault(key, 0)

            # 'cooling_profiles' is optional (use 'all' instead)
            if ('cooling_profiles' not in z) or \
                    (z['cooling_profiles'] is None):
                profiles = ["all"]
            else:
                profiles = z['cooling_profiles']

            fans = getFansInZone(z['zone'], profiles, fan_data)
            events = getEventsInZone(z['zone'], group['zone_conditions'],
                                     events_data)

            if len(fans) == 0:
                sys.exit("Didn't find any fans in zone " + str(zone['num']))

            zone['fans'] = fans
            zone['events'] = events
            zones.append(zone)

        zone_group['zones'] = zones
        zone_groups.append(zone_group)

    return zone_groups


if __name__ == '__main__':
    parser = ArgumentParser(
        description="Phosphor fan zone definition parser")

    parser.add_argument('-z', '--zone_yaml', dest='zone_yaml',
                        default="example/zones.yaml",
                        help='fan zone definitional yaml')
    parser.add_argument('-f', '--fan_yaml', dest='fan_yaml',
                        default="example/fans.yaml",
                        help='fan definitional yaml')
    parser.add_argument('-e', '--events_yaml', dest='events_yaml',
                        help='events to set speeds yaml')
    parser.add_argument('-c', '--zone_conditions_yaml',
                        dest='zone_conditions_yaml',
                        help='conditions to determine zone yaml')
    parser.add_argument('-o', '--output_dir', dest='output_dir',
                        default=".",
                        help='output directory')
    args = parser.parse_args()

    if not args.zone_yaml or not args.fan_yaml:
        parser.print_usage()
        sys.exit(-1)

    with open(args.zone_yaml, 'r') as zone_input:
        zone_data = yaml.safe_load(zone_input) or {}

    with open(args.fan_yaml, 'r') as fan_input:
        fan_data = yaml.safe_load(fan_input) or {}

    events_data = {}
    if args.events_yaml:
        with open(args.events_yaml, 'r') as events_input:
            events_data = yaml.safe_load(events_input) or {}

    zone_conditions_data = {}
    if args.zone_conditions_yaml:
        with open(args.zone_conditions_yaml, 'r') as zone_conditions_input:
            zone_conditions_data = yaml.safe_load(zone_conditions_input) or {}

    zone_config = buildZoneData(zone_data.get('zone_configuration', {}),
                                fan_data, events_data, zone_conditions_data)

    manager_config = zone_data.get('manager_configuration', {})

    if manager_config.get('power_on_delay') is None:
        manager_config['power_on_delay'] = 0

    output_file = os.path.join(args.output_dir, "fan_zone_defs.cpp")
    with open(output_file, 'w') as output:
        output.write(Template(tmpl).render(zones=zone_config,
                     mgr_data=manager_config))
