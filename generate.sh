#!/bin/bash

HLIST=($($1 gen-fan-detect-defs.py -y $2))
HLISTLEN=${#HLIST[@]}

echo "FAN_DETECT_FILES = \\"
for (( i=0; i<${HLISTLEN}; i++ ))
do
    if [ $((i + 1)) -ge ${HLISTLEN} ]
    then
        echo $'\t'"${HLIST[$i]}"
    else
        echo $'\t'"${HLIST[$i]} \\"
    fi
done
echo
cat << GENERATED
nodist_include_HEADERS = \$(FAN_DETECT_FILES)
.PHONY: distclean-local-generated
distclean-local-generated:
	for i in \$(FAN_DETECT_FILES) ; \\
	do \\
		test -e \$\$i && rm \$\$i ; \\
	done
GENERATED
