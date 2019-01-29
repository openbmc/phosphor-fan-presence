<%include file="defs.mako"/>\
<%namespace file="defs.mako" import="*"/>\
<%!
def indent(str, depth):
    return ''.join(4*' '*depth+line for line in str.splitlines(True))
%>\
/* This is a generated file. */
#include "manager.hpp"
#include "functor.hpp"
#include "actions.hpp"
#include "handlers.hpp"
#include "preconditions.hpp"
#include "matches.hpp"

using namespace phosphor::fan::control;

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
                        },
                        "${fan['target_interface']}"
                    },
                %endfor
                },
                std::vector<SetSpeedEvent>{
                %for event in zone['events']:
                    %if ('pc' in event) and \
                        (event['pc'] is not None):
                    SetSpeedEvent{
                        Group{
                        %for group in event['pc']['pcgrps']:
                        %for member in group['members']:
                        {
                            "${member['object']}",
                            {"${member['interface']}",
                             "${member['property']}"}
                        },
                        %endfor
                        %endfor
                        },
                        std::vector<Action>{
                        %for i, a in enumerate(event['pc']['pcact']):
                        %if len(a['params']) != 0:
                        make_action(
                            precondition::${a['name']}(
                        %else:
                        make_action(
                            precondition::${a['name']}
                        %endif
                        %for p in a['params']:
                        ${p['type']}${p['open']}
                        %for j, v in enumerate(p['values']):
                        %if (j+1) != len(p['values']):
                            ${v['value']},
                        %else:
                            ${v['value']}
                        %endif
                        %endfor
                        ${p['close']},
                        %endfor
                        %if (i+1) != len(event['pc']['pcact']):
                        %if len(a['params']) != 0:
                        )),
                        %else:
                        ),
                        %endif
                        %endif
                        %endfor
                    std::vector<SetSpeedEvent>{
                    %for pcevt in event['pc']['pcevts']:
                    SetSpeedEvent{\
                    ${indent(genSSE(event=pcevt), 6)}\
                    },
                    %endfor
                    %else:
                    SetSpeedEvent{\
                    ${indent(genSSE(event=event), 6)}
                    %endif
                    %if ('pc' in event) and (event['pc'] is not None):
                    }
                        %if len(event['pc']['pcact'][-1]['params']) != 0:
                        )),
                        %else:
                        ),
                        %endif
                        },
                        TimerConf{
                            ${event['pc']['pctime']['interval']},
                            ${event['pc']['pctime']['type']}
                        },
                        std::vector<Signal>{
                        %for s in event['pc']['pcsigs']:
                            Signal{
                                %if ('match' in s) and \
                                    (s['match'] is not None):
                                match::${s['match']}(
                                %for i, mp in enumerate(s['mparams']):
                                %if (i+1) != len(s['mparams']):
                                "${mp}",
                                %else:
                                "${mp}"
                                %endif
                                %endfor
                                ),
                                %else:
                                "",
                                %endif
                                make_handler(\
                                ${indent(genHandler(sig=s), 9)}\
                                )
                            },
                        %endfor
                        }
                    %endif
                    },
                %endfor
                }
            },
        %endfor
        }
    },
%endfor
};
