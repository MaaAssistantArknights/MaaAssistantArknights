#!/usr/bin/env bash
set -e

zipinfo -O utf8 -v $1 \
    | sed -n -e '/^Central directory entry/,+3p' -e '/32-bit CRC value (hex):/p' | grep '^  ' \
    | sed -e 's/^  //g' | sed -z 's/\n32-bit CRC value (hex):\s*/\t/g' \
    | awk '{last = $NF; $NF=""; print last,$0}'
