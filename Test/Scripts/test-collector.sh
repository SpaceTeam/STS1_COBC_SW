#!/usr/bin/env bash

rm test-report.txt &>/dev/null || true
touch test-report.txt
for diff in $(printf '%s' "$@" | tr ';' ' ')
do
    if [ -s "$diff" ]
    then
        echo "%%" >> test-report.txt
        echo "%% $diff" >> test-report.txt
        echo "%%" >> test-report.txt
        cat "$diff" >> test-report.txt
        echo >> test-report.txt
    fi
done
