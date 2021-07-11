#!/bin/bash
SOURCES=$(find Sources -name '*.swift' | awk '{ print "--files-to-mutate " $1 }' | tr '\n' ' ')
muter run --skip-coverage $SOURCES $@
