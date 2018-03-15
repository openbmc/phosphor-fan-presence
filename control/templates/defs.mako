<%!
def indent(str, depth):
    return ''.join(4*' '*depth+line for line in str.splitlines(True))
%>\

<%def name="genHandler(sig)" buffered="True">
%if ('type' in sig['hparams']) and \
    (sig['hparams']['type'] is not None):
${sig['signal']}<${sig['hparams']['type']}>(
%else:
${sig['signal']}(
%endif
%for i, hpk in enumerate(sig['hparams']['params']):
%if (i+1) != len(sig['hparams']['params']):
${sig['hparams'][hpk]},
%else:
${sig['hparams'][hpk]}
%endif
%endfor
)
</%def>\

<%def name="genSSE(event)" buffered="True">
Group{
%for group in event['groups']:
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
    make_trigger(trigger::timer(Timer{
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
},
</%def>\
