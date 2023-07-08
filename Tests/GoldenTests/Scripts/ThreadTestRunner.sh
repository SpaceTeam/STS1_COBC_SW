#!/usr/bin/env bash
set -e
# Kill test executable (with SIGKILL) after 8 seconds.  Necessary
# because sometimes tests might deadlock.
if timeout -s 9 8 $1 > "$1.output"
then
    true
else
    echo ":: KILLED BY TIMEOUT" >> "$1.output"
fi

# Remove header / intro from output and expected output
linecount=`wc -l < "$1"`
pattern="--------------- Application running ------------"
grep -h -B 0 -A $linecount -e "$pattern" $1.output > $1.output.trimmed || true
