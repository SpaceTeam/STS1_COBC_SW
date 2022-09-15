#!/usr/bin/env bash

echo "::: Running $1"
set -e
# Kill test executable (with SIGKILL) after 8 seconds.  Necessary
# because sometimes tests might deadlock.
if timeout -s 9 8 $1 > "$1.output"
then
    true
else
    echo ":: KILLED BY TIMEOUT" >> "$1.output"
fi
