#!/bin/sh
if [ -d $1 ]; then
    cd $1
    echo "nobase_dist_configs_DATA = \\"
    configs=`find * -type f`
    for i in ${configs};
    do
        echo "	${i} \\"
    done
fi
