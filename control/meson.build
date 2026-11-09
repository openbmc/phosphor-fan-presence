
include_dirs = ['.', '..']

deps = [
    phosphor_dbus_interfaces_dep,
    phosphor_logging_dep,
    sdbusplus_dep,
    sdeventplus_dep,
    cereal_dep,
]

sources = ['main.cpp']

if conf.has('CONTROL_USE_JSON')
    deps += nlohmann_json_dep
    include_dirs += ['./json', './json/actions', './json/triggers']
    sources += [
        'json/chassis_manager.cpp',
        'json/dbus_zone.cpp',
        'json/event.cpp',
        'json/fan.cpp',
        'json/group.cpp',
        'json/manager.cpp',
        'json/profile.cpp',
        'json/zone.cpp',
        'json/actions/count_state_floor.cpp',
        'json/actions/count_state_target.cpp',
        'json/actions/default_floor.cpp',
        'json/actions/get_managed_objects.cpp',
        'json/actions/mapped_floor.cpp',
        'json/actions/missing_owner_target.cpp',
        'json/actions/net_target_decrease.cpp',
        'json/actions/net_target_increase.cpp',
        'json/actions/override_fan_target.cpp',
        'json/actions/pcie_card_floors.cpp',
        'json/actions/request_target_base.cpp',
        'json/actions/set_parameter_from_group_max.cpp',
        'json/actions/target_from_group_max.cpp',
        'json/actions/timer_based_actions.cpp',
        'json/utils/flight_recorder.cpp',
        'json/utils/modifier.cpp',
        'json/utils/pcie_card_metadata.cpp',
        'json/triggers/init.cpp',
        'json/triggers/parameter.cpp',
        'json/triggers/signal.cpp',
        'json/triggers/timer.cpp',
    ]

    # Expand the events/groups templates (if the machine has any) so that the
    # expanded JSON files are generated at build time instead of only by
    # an external recipe at install time.
    #
    # Meson can't glob, so find the templates at configure time.  This prints
    # the path of each 'config_files/<machine>/<model>/templates/*.json'
    # relative to the control source directory.
    fs = import('fs')
    machine_config_dir = 'config_files' / get_option('machine-name')
    if get_option('machine-name') != '' and fs.is_dir(machine_config_dir)
        find_templates = run_command(
            python_prog,
            '-c',
            '''
import glob, os, sys
base = sys.argv[1]
for name in ("events.json", "groups.json"):
    for p in sorted(glob.glob(os.path.join(base, "*", "templates", name))):
        print(os.path.relpath(p, os.path.dirname(os.path.dirname(base))))
''',
            meson.current_source_dir() / machine_config_dir,
            check: true,
        )

        template_files = []
        expanded_models = []
        foreach template : find_templates.stdout().strip().split('\n')
            if template != ''
                template_files += files(template)
                # config_files/<machine>/<model>/templates/<name>.json -> <model>
                model = fs.name(fs.parent(fs.parent(template)))
                if model not in expanded_models
                    expanded_models += model
                endif
            endif
        endforeach

        if expanded_models.length() > 0
            # One output directory per model, each containing the expanded
            # events.json and/or groups.json.  Each directory is installed
            # under the same place install_subdir() puts the rest of that
            # model's config files.
            expanded_install_dirs = []
            foreach model : expanded_models
                expanded_install_dirs += usr_share_dir / 'control'
            endforeach

            custom_target(
                'expand_fan_control_json',
                input: template_files,
                depend_files: files('json/tools/expand.py'),
                command: [
                    python_prog,
                    files('json/tools/expand.py'),
                    '-t',
                    'all',
                    '-i',
                    meson.current_source_dir() / machine_config_dir,
                    '-o',
                    '@OUTDIR@',
                ],
                output: expanded_models,
                build_by_default: true,
                install: true,
                install_dir: expanded_install_dirs,
            )
        endif
    endif
else
    script = files('gen-fan-zone-defs.py')
    fan_zone_defs_cpp_dep = custom_target(
        'fan_zone_defs.cpp',
        input: files(
            conf.get_unquoted('FAN_DEF_YAML_FILE'),
            conf.get_unquoted('FAN_ZONE_YAML_FILE'),
            conf.get_unquoted('ZONE_EVENTS_YAML_FILE'),
            conf.get_unquoted('ZONE_CONDITIONS_YAML_FILE'),
        ),
        command: [
            python_prog,
            script,
            '-f',
            '@INPUT0@',
            '-z',
            '@INPUT1@',
            '-e',
            '@INPUT2@',
            '-c',
            '@INPUT3@',
            '-o',
            'control',
        ],
        output: 'fan_zone_defs.cpp',
    )

    sources += [
        'actions.cpp',
        'fan.cpp',
        fan_zone_defs_cpp_dep,
        'manager.cpp',
        'preconditions.cpp',
        'triggers.cpp',
        'utility.cpp',
        'zone.cpp',
    ]

endif

phosphor_fan_control_include_directories = include_directories(include_dirs)
phosphor_fan_control = executable(
    'phosphor-fan-control',
    sources,
    dependencies: deps,
    implicit_include_directories: false,
    include_directories: phosphor_fan_control_include_directories,
    install: true,
)

fanctl = executable(
    'fanctl',
    'fanctl.cpp',
    dependencies: [
        CLI11_dep,
        nlohmann_json_dep,
        phosphor_logging_dep,
        sdbusplus_dep,
    ],
    include_directories: phosphor_fan_control_include_directories,
    install: true,
)
