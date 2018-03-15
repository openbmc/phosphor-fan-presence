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
from mako.lookup import TemplateLookup


def convertToMap(listOfDict):
    """
    Converts a list of dictionary entries to a std::map initialization list.
    """
    listOfDict = listOfDict.replace('\'', '\"')
    listOfDict = listOfDict.replace('[', '{')
    listOfDict = listOfDict.replace(']', '}')
    listOfDict = listOfDict.replace(':', ',')
    return listOfDict


def getGroups(zNum, zCond, edata, events):
    """
    Extract and construct the groups for the given event.
    """
    groups = []
    for eGroups in edata['groups']:
        if ('zone_conditions' in eGroups) and \
           (eGroups['zone_conditions'] is not None):
            # Zone conditions are optional in the events yaml but skip
            # if this event's condition is not in this zone's conditions
            if all('name' in z and z['name'] is not None and
                   not any(c['name'] == z['name'] for c in zCond)
                   for z in eGroups['zone_conditions']):
                continue

            # Zone numbers are optional in the events yaml but skip if this
            # zone's zone number is not in the event's zone numbers
            if all('zones' in z and z['zones'] is not None and
                   zNum not in z['zones']
                   for z in eGroups['zone_conditions']):
                continue

        eGroup = next(g for g in events['groups']
                      if g['name'] == eGroups['name'])

        group = {}
        members = []
        group['name'] = eGroup['name']
        for m in eGroup['members']:
            member = {}
            member['path'] = eGroup['type']
            member['object'] = (eGroup['type'] + m)
            member['interface'] = eGroups['interface']
            member['property'] = eGroups['property']['name']
            member['type'] = eGroups['property']['type']
            # Add expected group member's property value if given
            if ('value' in eGroups['property']) and \
               (eGroups['property']['value'] is not None):
                    if isinstance(eGroups['property']['value'], str) or \
                            "string" in str(member['type']).lower():
                        member['value'] = (
                            "\"" + eGroups['property']['value'] + "\"")
                    else:
                        member['value'] = eGroups['property']['value']
            members.append(member)
        group['members'] = members
        groups.append(group)
    return groups


def getHandler(member, group, eHandler, events):
    """
    Extracts and constructs an event handler's parameters
    """
    hparams = {}
    if ('parameters' in eHandler) and \
       (eHandler['parameters'] is not None):
        hplist = []
        for p in eHandler['parameters']:
            hp = str(p)
            if (hp != 'type'):
                hplist.append(hp)
                if (hp != 'group'):
                    hparams[hp] = "\"" + member[hp] + "\""
                else:
                    hparams[hp] = "Group{\n"
                    for m in group['members']:
                        hparams[hp] += (
                            "{\n" +
                            "\"" + str(m['object']) + "\",\n" +
                            "\"" + str(m['interface']) + "\"," +
                            "\"" + str(m['property']) + "\"\n" +
                            "},\n")
                    hparams[hp] += "}"
            else:
                hparams[hp] = member[hp]
        hparams['params'] = hplist
    return hparams


def getSignal(eGrps, eTrig, events):
    """
    Extracts and constructs for each group member a signal
    subscription of each match listed in the trigger.
    """
    signals = []
    for group in eGrps:
        for member in group['members']:
            for eMatches in eTrig['matches']:
                signal = {}
                eMatch = next(m for m in events['matches']
                              if m['name'] == eMatches['name'])
                signal['match'] = eMatch['name']
                params = []
                if ('parameters' in eMatch) and \
                   (eMatch['parameters'] is not None):
                    for p in eMatch['parameters']:
                        params.append(member[str(p)])
                signal['mparams'] = params
                # Add signal handler parameters
                if ('parameters' in eMatch['signal']) and \
                   (eMatch['signal']['parameters'] is not None):
                    eSignal = eMatch['signal']
                else:
                    eSignal = next(s for s in events['signals']
                                   if s['name'] == eMatch['signal'])
                signal['signal'] = eSignal['name']
                signal['hparams'] = getHandler(member, group, eSignal, events)

                signals.append(signal)
    return signals


def getTimer(eTrig):
    """
    Extracts and constructs the required parameters for an
    event timer.
    """
    timer = {}
    timer['interval'] = (
        "static_cast<std::chrono::microseconds>" +
        "(" + str(eTrig['interval']) + ")")
    timer['type'] = "util::Timer::TimerType::" + str(eTrig['type'])
    return timer


def getActions(edata, actions, events):
    """
    Extracts and constructs the make_action function call for
    all the actions within the given event.
    """
    action = []
    for eActions in actions['actions']:
        actions = {}
        eAction = next(a for a in events['actions']
                       if a['name'] == eActions['name'])
        actions['name'] = eAction['name']
        params = []
        if ('parameters' in eAction) and \
           (eAction['parameters'] is not None):
            for p in eAction['parameters']:
                param = "static_cast<"
                if type(eActions[p]) is not dict:
                    if p == 'actions':
                        param = "std::vector<Action>{"
                        pActs = getActions(edata, eActions, events)
                        for a in pActs:
                            if (len(a['parameters']) != 0):
                                param += (
                                    "make_action(action::" +
                                    a['name'] +
                                    "(\n")
                                for i, ap in enumerate(a['parameters']):
                                    if (i+1) != len(a['parameters']):
                                        param += (ap + ",")
                                    else:
                                        param += (ap + ")")
                            else:
                                param += ("make_action(action::" + a['name'])
                            param += "),"
                        param += "}"
                    elif p == 'property':
                        if isinstance(eActions[p], str) or \
                           "string" in str(eActions[p]['type']).lower():
                            param += (
                                str(eActions[p]['type']).lower() +
                                ">(\"" + str(eActions[p]) + "\")")
                        else:
                            param += (
                                str(eActions[p]['type']).lower() +
                                ">(" + str(eActions[p]['value']).lower() + ")")
                    else:
                        # Default type to 'size_t' when not given
                        param += ("size_t>(" + str(eActions[p]).lower() + ")")
                else:
                    if p == 'timer':
                        t = getTimer(eActions[p])
                        param = (
                            "Timer{" + t['interval'] + "," + t['type'] + "}")
                    else:
                        param += (str(eActions[p]['type']).lower() + ">(")
                        if p != 'map':
                            if isinstance(eActions[p]['value'], str) or \
                               "string" in str(eActions[p]['type']).lower():
                                param += \
                                    "\"" + str(eActions[p]['value']) + "\")"
                            else:
                                param += \
                                    str(eActions[p]['value']).lower() + ")"
                        else:
                            param += (
                                str(eActions[p]['type']).lower() +
                                convertToMap(str(eActions[p]['value'])) + ")")
                params.append(param)
        actions['parameters'] = params
        action.append(actions)
    return action


def getEvent(zone_num, zone_conditions, e, events_data):
    """
    Parses the sections of an event and populates the properties
    that construct an event within the generated source.
    """
    event = {}

    # Add set speed event groups
    grps = getGroups(zone_num, zone_conditions, e, events_data)
    if not grps:
        return
    event['groups'] = grps

    # Add optional set speed actions and function parameters
    event['action'] = []
    if ('actions' in e) and \
       (e['actions'] is not None):
        event['action'] = getActions(e, e, events_data)

    # Add event triggers
    event['triggers'] = {}
    for trig in e['triggers']:
        triggers = []
        if (trig['name'] == "timer"):
            event['triggers']['timer'] = getTimer(trig)
        elif (trig['name'] == "signal"):
            triggers = getSignal(event['groups'], trig, events_data)
            event['triggers']['signal'] = triggers

    return event


def addPrecondition(zNum, zCond, event, events_data):
    """
    Parses the precondition section of an event and populates the necessary
    structures to generate a precondition for a set speed event.
    """
    precond = {}
    # Add set speed event precondition group
    grps = getGroups(zNum, zCond, event['precondition'], events_data)
    if not grps:
        return
    precond['pcgrps'] = grps

    # Add set speed event precondition actions
    pc = []
    pcs = {}
    pcs['name'] = event['precondition']['name']
    epc = next(p for p in events_data['preconditions']
               if p['name'] == event['precondition']['name'])
    params = []
    for p in epc['parameters']:
        param = {}
        if p == 'groups':
            param['type'] = "std::vector<PrecondGroup>"
            param['open'] = "{"
            param['close'] = "}"
            values = []
            for group in precond['pcgrps']:
                for pcgrp in group['members']:
                    value = {}
                    value['value'] = (
                        "PrecondGroup{\"" +
                        str(pcgrp['object']) + "\",\"" +
                        str(pcgrp['interface']) + "\",\"" +
                        str(pcgrp['property']) + "\"," +
                        "static_cast<" +
                        str(pcgrp['type']).lower() + ">")
                    if isinstance(pcgrp['value'], str) or \
                       "string" in str(pcgrp['type']).lower():
                        value['value'] += ("(" + str(pcgrp['value']) + ")}")
                    else:
                        value['value'] += \
                            ("(" + str(pcgrp['value']).lower() + ")}")
                    values.append(value)
            param['values'] = values
        params.append(param)
    pcs['params'] = params
    pc.append(pcs)
    precond['pcact'] = pc

    pcevents = []
    for pce in event['precondition']['events']:
        pcevent = getEvent(zNum, zCond, pce, events_data)
        if not pcevent:
            continue
        pcevents.append(pcevent)
    precond['pcevts'] = pcevents

    # Add precondition event triggers
    precond['triggers'] = {}
    for trig in event['precondition']['triggers']:
        triggers = []
        if (trig['name'] == "timer"):
            precond['triggers']['pctime'] = getTimer(trig)
        elif (trig['name'] == "signal"):
            triggers = getSignal(precond['pcgrps'], trig, events_data)
            precond['triggers']['pcsigs'] = triggers

    return precond


def getEventsInZone(zone_num, zone_conditions, events_data):
    """
    Constructs the event entries defined for each zone using the events yaml
    provided.
    """
    events = []

    if 'events' in events_data:
        for e in events_data['events']:
            event = {}
            # Add precondition if given
            if ('precondition' in e) and \
               (e['precondition'] is not None):
                event['pc'] = addPrecondition(zone_num,
                                              zone_conditions,
                                              e,
                                              events_data)
            else:
                event = getEvent(zone_num, zone_conditions, e, events_data)
                if not event:
                    continue
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
        fan['target_interface'] = f.get(
            'target_interface',
            'xyz.openbmc_project.Control.FanSpeed')
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

    tmpls_dir = os.path.join(
        os.path.dirname(os.path.realpath(__file__)),
        "templates")
    output_file = os.path.join(args.output_dir, "fan_zone_defs.cpp")
    if sys.version_info < (3, 0):
        lkup = TemplateLookup(
            directories=tmpls_dir.split(),
            disable_unicode=True)
    else:
        lkup = TemplateLookup(
            directories=tmpls_dir.split())
    tmpl = lkup.get_template('fan_zone_defs.mako.cpp')
    with open(output_file, 'w') as output:
        output.write(tmpl.render(zones=zone_config,
                                 mgr_data=manager_config))
