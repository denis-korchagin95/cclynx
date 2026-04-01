#!/bin/bash
# Wrapper to pipe file content via stdin to cclynx.
# jcunit passes: <filepath> <args...>
# We pipe the file via stdin and pass remaining args to cclynx.
filepath="$1"
shift
cat "$filepath" | ./bin/cclynx "$@" /dev/stdin
