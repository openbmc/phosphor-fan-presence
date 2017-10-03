#!/usr/bin/env python

import os
import sys
import yaml
from argparse import ArgumentParser
from mako.template import Template

"""
This script generates the data structures for the
phosphor-fan-monitor application.

A future improvement is to get the fan inventory names
from a separate file, so just that file could be generated
from the MRW.
"""


tmpl = '''/* This is a generated file. */
#include "fan_defs.hpp"
#include "types.hpp"
#include "groups.hpp"

using namespace phosphor::fan::monitor;
using namespace phosphor::fan::trust;

const std::vector<FanDefinition> fanDefinitions
{
%for fan_data in data.get('fans', {}):
    FanDefinition{"${fan_data['inventory']}",
                  ${fan_data['allowed_out_of_range_time']},
                  ${fan_data['deviation']},
                  ${fan_data['num_sensors_nonfunc_for_fan_nonfunc']},
                  std::vector<SensorDefinition>{
                  %for sensor in fan_data['sensors']:
                  <%
                      #has_target is a bool, and we need a true instead of True
                      has_target = str(sensor['has_target']).lower()
                  %> \
                      SensorDefinition{"${sensor['name']}", ${has_target}},
                  %endfor
                  },
    },
%endfor
};

##Function to generate the group creation lambda.
##If a group were to ever need a different constructor,
##it could be handled here.
<%def name="get_lambda_contents(group)">
            std::vector<std::string> names{
            %for sensor in group['sensors']:
                "${sensor['name']}",
            %endfor
            };
            return std::make_unique<${group['class']}>(names);
</%def>
const std::vector<CreateGroupFunction> trustGroups
{
%for group in data.get('sensor_trust_groups', {}):
    {
        []()
        {\
${get_lambda_contents(group)}\
        }
    },
%endfor
};
'''


if __name__ == '__main__':
    parser = ArgumentParser(
        description="Phosphor fan monitor definition parser")

    parser.add_argument('-m', '--monitor_yaml', dest='monitor_yaml',
                        default="example/monitor.yaml",
                        help='fan monitor definitional yaml')
    parser.add_argument('-o', '--output_dir', dest='output_dir',
                        default=".",
                        help='output directory')
    args = parser.parse_args()

    if not args.monitor_yaml:
        parser.print_usage()
        sys.exit(-1)

    with open(args.monitor_yaml, 'r') as monitor_input:
        monitor_data = yaml.safe_load(monitor_input) or {}

    #Do some minor input validation
    for fan in monitor_data.get('fans', {}):
        if ((fan['deviation'] < 0) or (fan['deviation'] > 100)):
            sys.exit("Invalid deviation value " + str(fan['deviation']))

    output_file = os.path.join(args.output_dir, "fan_monitor_defs.cpp")
    with open(output_file, 'w') as output:
        output.write(Template(tmpl).render(data=monitor_data))
