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

#Note: Condition is a TODO
tmpl = '''/* This is a generated file. */
#include "manager.hpp"

using namespace phosphor::fan::control;

const std::vector<ZoneGroup> Manager::_zoneLayouts
{
%for zone_group in zones:
    std::make_tuple(
            std::vector<Condition>{},
            std::vector<ZoneDefinition>{
            %for zone in zone_group['zones']:
                std::make_tuple(${zone['num']},
                                ${zone['initial_speed']},
                                std::vector<FanDefinition>{
                                    %for fan in zone['fans']:
                                    std::make_tuple("${fan['name']}",
                                        std::vector<std::string>{
                                        %for sensor in fan['sensors']:
                                           "${sensor}",
                                        %endfor
                                        }
                                    ),
                                    %endfor
                                }
                ),
            %endfor
            }
    ),
%endfor
};
'''

if __name__ == '__main__':
    parser = ArgumentParser(
        description="Phosphor fan zone definition parser")

    parser.add_argument('-z', '--zone_yaml', dest='zone_yaml',
                        default="example/zones.yaml",
                        help='fan zone definitional yaml')
    parser.add_argument('-f', '--fan_yaml', dest='fan_yaml',
                        default="example/fans.yaml",
                        help='fan definitional yaml')
    args = parser.parse_args()

    if not args.zone_yaml or not args.fan_yaml:
        parser.print_usage()
        sys.exit(-1)

    with open(args.zone_yaml, 'r') as zone_input:
        zone_data = yaml.safe_load(zone_input) or {}

    with open(args.fan_yaml, 'r') as fan_input:
        fan_data = yaml.safe_load(fan_input) or {}

    #Fill in with using zone_data and fan_data with next commit
    zone_data = []

    pwd = os.path.dirname(os.path.abspath(__file__))
    output_file = os.path.join(pwd, "fan_zone_defs.cpp")

    with open(output_file, 'w') as output:
        output.write(Template(tmpl).render(zones=zone_data))
