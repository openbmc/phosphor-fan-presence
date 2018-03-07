<%!
def indent(str, depth):
    return ''.join(4*' '*depth+line for line in str.splitlines(True))
%>\

<%def name="genParams(par)" buffered="True">
%if ('type' in par['hparams']) and \
    (par['hparams']['type'] is not None):
handler::${par['handler']}<${par['hparams']['type']}>(
%else:
handler::${par['handler']}(
%endif
%for i, hpk in enumerate(par['hparams']['params']):
    %if (i+1) != len(par['hparams']['params']):
    ${par['hparams'][hpk]},
    %else:
    ${par['hparams'][hpk]}
    %endif
%endfor
)
</%def>\

<%def name="genHandler(sig)" buffered="True">
%if ('type' in sig['sparams']) and \
    (sig['sparams']['type'] is not None):
${sig['signal']}<${sig['sparams']['type']}>(
%else:
${sig['signal']}(
%endif
%for spk in sig['sparams']['params']:
${sig['sparams'][spk]},
%endfor
${genParams(par=sig)}\
)
</%def>\

<%def name="genSSE(event)" buffered="True">
Group{
%for group in event['groups']:
%for member in group['members']:
{
    "${member['object']}",
    "${member['interface']}",
    "${member['property']}"
},
%endfor
%endfor
},
std::vector<Action>{
%for a in event['action']:
%if len(a['parameters']) != 0:
make_action(action::${a['name']}(
%else:
make_action(action::${a['name']}
%endif
%for i, p in enumerate(a['parameters']):
%if (i+1) != len(a['parameters']):
    ${p},
%else:
    ${p})
%endif
%endfor
),
%endfor
},
std::vector<Trigger>{
    %if ('timer' in event['triggers']) and \
        (event['triggers']['timer'] is not None):
    make_trigger(trigger::timer(TimerConf{
    ${event['triggers']['timer']['interval']},
    ${event['triggers']['timer']['type']}
    })),
    %endif
    %if ('signal' in event['triggers']) and \
        (event['triggers']['signal'] is not None):
    %for s in event['triggers']['signal']:
    make_trigger(trigger::signal(
        match::${s['match']}(
        %for i, mp in enumerate(s['mparams']):
        %if (i+1) != len(s['mparams']):
        "${mp}",
        %else:
        "${mp}"
        %endif
        %endfor
        ),
        make_handler(\
        ${indent(genHandler(sig=s), 3)}\
        )
    )),
    %endfor
    %endif
    %if ('init' in event['triggers']):
    %for i in event['triggers']['init']:
    make_trigger(trigger::init(
        %if ('handler' in s):
        make_handler(\
        ${indent(genParams(par=i), 3)}\
        )
        %endif
    )),
    %endfor
    %endif
},
</%def>\
