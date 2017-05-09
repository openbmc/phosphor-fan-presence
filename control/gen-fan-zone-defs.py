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

#Note: Condition is a TODO (openbmc/openbmc#1500)
tmpl = '''/* This is a generated file. */
#include "manager.hpp"

using namespace phosphor::fan::control;

const unsigned int Manager::_powerOnDelay{${mgr_data['power_on_delay']}};

const std::vector<ZoneGroup> Manager::_zoneLayouts
{
%for zone_group in zones:
    ZoneGroup{std::vector<Condition>{},
              std::vector<ZoneDefinition>{
              %for zone in zone_group['zones']:
                  ZoneDefinition{${zone['num']},
                                 ${zone['full_speed']},
                                 std::vector<FanDefinition>{
                                     %for fan in zone['fans']:
                                     FanDefinition{"${fan['name']}",
                                         std::vector<std::string>{
                                         %for sensor in fan['sensors']:
                                            "${sensor}",
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

        #'cooling_profile' is optional (use 'all' instead)
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


def buildZoneData(zone_data, fan_data):
    """
    Combines the zone definition YAML and fan
    definition YAML to create a data structure defining
    the fan cooling zones.
    """

    zone_groups = []

    for group in zone_data:
        conditions = []
        for c in group['zone_conditions']:
            conditions.append(c['name'])

        zone_group = {}
        zone_group['conditions'] = conditions

        zones = []
        for z in group['zones']:
            zone = {}

            #'zone' is required
            if (not 'zone' in z) or (z['zone'] is None):
                sys.exit("Missing fan zone number in " + zone_yaml)

            zone['num'] = z['zone']

            zone['full_speed'] = z['full_speed']

            #'cooling_profiles' is optional (use 'all' instead)
            if (not 'cooling_profiles' in z) or \
                    (z['cooling_profiles'] is None):
                profiles = ["all"]
            else:
                profiles = z['cooling_profiles']

            fans = getFansInZone(z['zone'], profiles, fan_data)

            if len(fans) == 0:
                sys.exit("Didn't find any fans in zone " + str(zone['num']))

            zone['fans'] = fans
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

    zone_config = buildZoneData(zone_data.get('zone_configuration', {}),
                                fan_data)

    manager_config = zone_data.get('manager_configuration', {})

    if manager_config.get('power_on_delay') is None:
        manager_config['power_on_delay'] = 0

    output_file = os.path.join(args.output_dir, "fan_zone_defs.cpp")
    with open(output_file, 'w') as output:
        output.write(Template(tmpl).render(zones=zone_config,
                     mgr_data=manager_config))
