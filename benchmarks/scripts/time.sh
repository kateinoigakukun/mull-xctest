#!/bin/bash

output=$1
shift
(time -p "$@" 2>&1) 2> $output
