#!/bin/bash

sox -q "sample.mp3" -r 44100 -b 16 -e signed-integer -c 2 -t raw - | \
   ./../bin/client_main -s 127.0.0.1 "$@" | \
   aplay -t raw -f cd -B 5000 -v -D sysdefault -
