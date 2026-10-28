#!/usr/bin/env python3

"""
Usage

Single File: expand.py -t <events | groups> -i /path/to/template -o /path/to/output

OR

Directory: expand.py -t <events | groups> -i /path/to/config_files -o /path/to/output

For Directory mode, input (-i) and output (-o) directories should have the same structure.

Example input (-i) directory structure:

    config_files/
        com.ibm.Hardware.Chassis.Model.<System1>/
            fans.json
            zones.json
            pcie_cards.json
            templates/
                events.json
                groups.json
        com.ibm.Hardware.Chassis.Model.<System2>/
            fans.json
            zones.json
            pcie_cards.json
            templates/
                events.json
                groups.json
        ...

Output (-o) should follow the same internal structure as above, although the name of the top-level
directory (e.g. "config_files" in the above) may differ.
"""

import argparse
import copy
import glob
import json
import os
import re
import sys

EVENTS_TEMPLATE_NAME = "events.json"
GROUPS_TEMPLATE_NAME = "groups.json"


def get_sanitized_json(filename):
    """
    Parse in a JSON file into a Python dictionary list

    filename - Path to the JSON file to parse in
    """
    lines = []
    out = []
    with open(filename, "r") as file:
        lines = file.readlines()

    for line in lines:
        # remove comment lines
        match = re.search(r"\s*\/\/", line)
        if match is None:
            out.append(line)

    return json.loads("".join(out))


def write_json_file(filename, data):
    """
    Write JSON data formatted in compliance with OpenBMC CI formatting rules:
    - 4-space indentation
    - Inlined single-element string arrays (e.g. ["1"])
    - Trailing newline at EOF
    """
    text = json.dumps(data, indent=4)
    # Inline single-element string arrays to match prettier formatting: ["1"]
    text = re.sub(r'\[\s*\n\s*(".*?")\s*\n\s*\]', r"[\1]", text)
    with open(filename, "w") as f:
        f.write(text + "\n")


def expand_groups(group_json, expand_params, event_num):
    """
    Expand a group into one or more groups based on expand variables

    group_json - list of JSON objects representing groups from a particular event
    param_name - Name of the parameter to replace
    param_val - Value of parameter to replace with
    """
    full_group_json = []
    for group in group_json:
        # group must have name, interface, and property
        for req_param in ["name", "interface", "property"]:
            if req_param not in group.keys():
                raise Exception(
                    f"{req_param} is a required parameter for groups"
                )

        # Copy over group and check expand flag. If true, replace variables for required fields, else just keep copied group
        true_group = {
            key: value for key, value in group.items() if key != "expand"
        }
        if group.get("expand", False):
            for param in expand_params:
                true_group["name"] = true_group["name"].replace(
                    "$" + param["name"], param["values"][event_num]
                )
        full_group_json.append(true_group)

    return full_group_json


def expand_triggers(trigger_json, expand_params, event_num):
    """
    Expand the triggers of an event template

    Only the 'parameter' field of a trigger (the name of the parameter that
    causes the trigger to fire) is expanded, and only if the trigger has
    'expand' set to true.

    trigger_json - list of JSON objects representing triggers from a particular event
    expand_params - List of parameters to replace, each with a name and values
    event_num - Index into each parameter's values to replace with
    """
    full_trigger_json = []
    for trigger in trigger_json:
        true_trigger = {
            key: value for key, value in trigger.items() if key != "expand"
        }
        if trigger.get("expand", False) and "parameter" in true_trigger:
            for param in expand_params:
                true_trigger["parameter"] = true_trigger["parameter"].replace(
                    "$" + param["name"], param["values"][event_num]
                )
        full_trigger_json.append(true_trigger)

    return full_trigger_json


def expand_fan_floors(fan_floors, expand_params, event_num):
    """
    Expand the parameter names inside a mapped_floor action's fan_floors

    The 'parameter' field of each entry in fan_floors[].floors[] is expanded.

    fan_floors - The 'fan_floors' JSON from a mapped_floor action
    expand_params - List of parameters to replace, each with a name and values
    event_num - Index into each parameter's values to replace with
    """
    # Deep copy so that the template, which is shared by every expansion of
    # the event, isn't modified.
    expanded = copy.deepcopy(fan_floors)
    for fan_floor in expanded:
        for group in fan_floor.get("floors", []):
            if "parameter" in group:
                for param in expand_params:
                    group["parameter"] = group["parameter"].replace(
                        "$" + param["name"], param["values"][event_num]
                    )

    return expanded


def expand_actions(action_json, expand_params, event_num):
    """
    Expand an action template into one or more actions based on expand variables

    action_json - list of JSON objects representing actios from a particular event
    param_name - Name of the parameter to replace
    param_val - Value of parameter to replace with
    """
    full_action_json = []
    for action in action_json:
        # action must have name
        if "name" not in action.keys():
            raise Exception("'name' is a required parameter for actions")

        # Copy over group and check expand flag.  If true, replace variables
        true_action = {
            key: value for key, value in action.items() if key != "expand"
        }
        if action.get("expand", False):
            for param in expand_params:
                for key, value in true_action.items():
                    if key in [
                        "name",
                        "parameter_name",
                        "state_parameter_name",
                    ]:
                        true_action[key] = value.replace(
                            "$" + param["name"], param["values"][event_num]
                        )
                    if key == "zones":
                        true_action[key] = [
                            zone.replace(
                                "$" + param["name"], param["values"][event_num]
                            )
                            for zone in value
                        ]
                    if key == "groups":
                        true_action[key] = expand_groups(
                            value, expand_params, event_num
                        )
                    if key == "fan_floors":
                        true_action[key] = expand_fan_floors(
                            value, [param], event_num
                        )
        full_action_json.append(true_action)

    return full_action_json


def expand_full_events_json(filename):
    """
    Expand a full events JSON file

    filename - name of JSON file template to expand
    """
    json_template = get_sanitized_json(filename)
    full_json = []
    for event in json_template:
        # event must have name, groups, triggers, and actions
        for req_param in ["name", "groups", "triggers"]:
            if req_param not in event.keys():
                raise Exception(
                    f"'{req_param}' is a required parameter for events"
                )

        # Check that there is an expand list. If not, or expand list is empty, copy over event
        expand_params = event.get("expand", [])
        if len(expand_params) == 0:
            full_json.append(
                {key: value for key, value in event.items() if key != "expand"}
            )
            continue

        # Check that all parameters in the expand list have the same length
        num_vals = len(expand_params[0].get("values", []))
        if num_vals == 0:
            raise Exception("Parameters must have at least one value")
        for param in expand_params:
            if len(param.get("values", [])) != num_vals:
                raise Exception(
                    "All parameters must have the same number of values."
                )

        # Copy template for however many events are needed then replace fields with parameter values
        expanded_event = [
            {key: value for key, value in event.items() if key != "expand"}
            for _ in range(num_vals)
        ]
        for event_num in range(num_vals):
            for param in expand_params:
                expanded_event[event_num]["name"] = expanded_event[event_num][
                    "name"
                ].replace("$" + param["name"], param["values"][event_num])
            expanded_event[event_num]["groups"] = expand_groups(
                expanded_event[event_num]["groups"], expand_params, event_num
            )
            expanded_event[event_num]["triggers"] = expand_triggers(
                expanded_event[event_num]["triggers"], expand_params, event_num
            )
            if "actions" in expanded_event[event_num].keys():
                expanded_event[event_num]["actions"] = expand_actions(
                    expanded_event[event_num]["actions"],
                    expand_params,
                    event_num,
                )

        # Concatenate expanded events to full JSON list
        full_json += expanded_event

    return full_json


def expand_full_groups_json(filename):
    """
    Expand a full groups JSON file

    filename - name of JSON file template to expand
    """
    json_template = get_sanitized_json(filename)
    full_json = []
    for group in json_template:
        # Groups must have name and members parameters filled out
        if "name" not in group.keys() or "members" not in group.keys():
            raise Exception("'name' and 'members' are required fields")

        # check if there is an expand list, if not or expand list is empty, copy over group
        expand_params = group.get("expand", [])
        if len(expand_params) == 0:
            full_json.append(
                {key: value for key, value in group.items() if key != "expand"}
            )
            continue

        # check that all parameters in the expand list have the same length for the values field
        num_vals = len(expand_params[0].get("values", []))
        if num_vals == 0:
            raise Exception("Parameters must have at least one value")
        for param in expand_params:
            if len(param.get("values", [])) != num_vals:
                raise Exception(
                    "All parameters must have the same number of values."
                )

        # copy template for however many groups are needed then replace the name and members field with parameters
        expanded_group = [
            {key: value for key, value in group.items() if key != "expand"}
            for _ in range(num_vals)
        ]
        for group_num in range(num_vals):
            for param in expand_params:
                expanded_group[group_num]["name"] = expanded_group[group_num][
                    "name"
                ].replace("$" + param["name"], param["values"][group_num])
                expanded_group[group_num]["members"] = [
                    member.replace(
                        "$" + param["name"], param["values"][group_num]
                    )
                    for member in expanded_group[group_num]["members"]
                ]

        # concatenate the expanded group items to the full JSON list
        full_json += expanded_group
    return full_json


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Expand an events.json or groups.json file into a larger file based on a given template, or search a config files directory for those."
    )
    parser.add_argument(
        "-t",
        "--type",
        dest="file_type",
        help="The type of file to expand (events or groups)",
        choices=["events", "groups"],
    )
    parser.add_argument(
        "-i",
        "--input",
        dest="input_file",
        help="file to expand or directory to search",
    )
    parser.add_argument(
        "-o",
        "--output",
        dest="output_file",
        help="Expanded output file or output directory",
    )
    args = parser.parse_args()
    if not args.file_type or not args.input_file or not args.output_file:
        sys.exit(
            "Missing file type (events or groups)"
            if not args.file_type
            else (
                "Missing input file"
                if not args.input_file
                else "Missing output file"
            )
        )
    if not os.path.exists(args.input_file) or not os.path.exists(
        args.output_file
    ):
        sys.exit(
            f"{args.input_file} does not exist"
            if not os.path.exists(args.input_file)
            else f"{args.output_file} does not exist"
        )

    if os.path.isdir(args.input_file) and os.path.isdir(args.output_file):
        template_name = (
            EVENTS_TEMPLATE_NAME
            if args.file_type == "events"
            else GROUPS_TEMPLATE_NAME
        )
        template_pattern = os.path.join(
            args.input_file, "*/templates/", template_name
        )

        for file in glob.glob(template_pattern, recursive=True):
            full_json = (
                expand_full_events_json(file)
                if args.file_type == "events"
                else expand_full_groups_json(file)
            )
            # Generated configs are output to the same subdirectory in
            # the output as in the input (e.g. com.ibm.Hardware.Chassis.Model...)
            config_dir = os.path.basename(
                os.path.dirname(os.path.dirname(file))
            )
            expanded_filename = os.path.join(
                args.output_file, config_dir, template_name
            )
            try:
                write_json_file(expanded_filename, full_json)
            except Exception as e:
                print(f"An error has occurred: {e}")

    elif os.path.isfile(args.input_file) and os.path.isfile(args.output_file):
        full_json = (
            expand_full_events_json(args.input_file)
            if args.file_type == "events"
            else expand_full_groups_json(args.input_file)
        )
        try:
            write_json_file(args.output_file, full_json)
        except Exception as e:
            print(f"An error has occurred: {e}")
    else:
        # Input and Output must both be either individual files or a folder containing subdirectories with config files.
        # This is to enforce the subdirectory structure for the config_files
        sys.exit("File mismatch, one file and one directory given.")
