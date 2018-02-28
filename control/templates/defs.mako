<%!
def indent(str, depth):
    return ''.join(4*' '*depth+line for line in str.splitlines(True))
%>\

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
%if ('type' in sig['hparams']) and \
    (sig['hparams']['type'] is not None):
handler::${sig['handler']}<${sig['hparams']['type']}>(
%else:
handler::${sig['handler']}(
%endif
%for i, hpk in enumerate(sig['hparams']['params']):
    %if (i+1) != len(sig['hparams']['params']):
    ${sig['hparams'][hpk]},
    %else:
    ${sig['hparams'][hpk]}
    %endif
%endfor
))
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
Timer{
    ${event['timer']['interval']},
    ${event['timer']['type']}
},
std::vector<Signal>{
%for s in event['signals']:
    Signal{
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
    },
%endfor
}
</%def>\
