#!/bin/bash

./../bin/client_main -s blue02.mimuw.edu.pl -p 10000 "$@" < /dev/null | \
   aplay -t raw -f cd -B 5000 -v - -D sysdefault
