#!/usr/bin/env python

"""
This script parses the given fan presence definition yaml file and generates
a header file based on the defined methods for determining when a fan is
present.
"""

import os
import sys
import yaml
from argparse import ArgumentParser
from mako.template import Template

tmpl = '''/* This is a generated file. */
#include "fan_detect_defs.hpp"

const std::map<std::string, std::set<phosphor::fan::Properties>>
fanDetectMap = {
%for methods in presence:
    %for method in methods:
    <% m = method.lower() %> \
    {"${m}", {
        %for fan in methods[method]:
            std::make_tuple("${fan['Inventory']}",
                            "${fan['PrettyName']}",
                            std::vector<std::string>{
            %for s in fan['Sensors']:
                                                    "${s}",
            %endfor
                                                    }),
        %endfor
    %endfor
    }},
%endfor
};
'''


def get_filename():
    """
    Constructs and returns the fully qualified header filename.
    """
    script_dir = os.path.dirname(os.path.abspath(__file__))
    header_file = os.path.join(script_dir, "fan_detect_defs.cpp")

    return header_file


if __name__ == '__main__':
    parser = ArgumentParser()
    # Input yaml containing how each fan's presence detection should be done
    parser.add_argument("-y", "--yaml", dest="pres_yaml",
                        default=
                        "example/fan-detect.yaml",
                        help=
                        "Input fan presences definition yaml file to parse")
    args = parser.parse_args(sys.argv[1:])

    # Verify given yaml file exists
    yaml_file = os.path.abspath(args.pres_yaml)
    if not os.path.isfile(yaml_file):
        print "Unable to find input yaml file " + yaml_file
        exit(1)

    with open(yaml_file, 'r') as yaml_input:
        presence_data = yaml.safe_load(yaml_input) or {}

    output_file = get_filename()

    with open(output_file, 'w') as out:
        out.write(Template(tmpl).render(presence=presence_data))
