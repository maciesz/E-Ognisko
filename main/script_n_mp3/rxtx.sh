#!/bin/bash

sox -q "sample.mp3" -r 44100 -b 16 -e signed-integer -c 2 -t raw - | \
   ./../bin/client_main -s 10.1.1.172 -p 10501 "$@" | \
   aplay -t raw -f cd -B 5000 -v -D sysdefault -
