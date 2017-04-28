#!/usr/bin/env python

import os
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

using namespace phosphor::fan::monitor;

const std::vector<FanDefinition> fanDefinitions
{
%for fan_data in data:
    FanDefinition{"${fan_data['inventory']}",
                  ${fan_data['startup_allowed_slow_time']},
                  ${fan_data['normal_allowed_slow_time']},
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

    output_file = os.path.join(args.output_dir, "fan_monitor_defs.cpp")
    with open(output_file, 'w') as output:
        output.write(Template(tmpl).render(data=monitor_data))
