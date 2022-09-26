#!/bin/bash

# execute roc-config
o2-roc-config $@

# collect status
o2-roc-status $1 --onu > /tmp/roc-status-$1
