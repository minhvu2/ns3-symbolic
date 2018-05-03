#!/bin/bash

S2E="./build/qemu-release/i386-s2e-softmmu/qemu-system-i386"
IMAGE="ubuntu14-32bit-ns3-5techs.raw.s2e"
CONFIG="simple.lua"
OPTION="-s2e-verbose -curses"
OUTPUT="s2e-last/messages.txt"

SCRIPT="tcp"
TECHS="all"
PACKETS="1p"
INTERVALS="256 512 1024 2048 4096 16384"

for TECH in $TECHS 
do
  for PACKET in $PACKETS 
  do
    for INTERVAL in $INTERVALS 
    do
      SNAPSHOT=$SCRIPT"-"$TECH"-"$PACKET"-"$INTERVAL
      time $S2E -net none $IMAGE -loadvm $SNAPSHOT -s2e-config-file $CONFIG $OPTION
      cat $OUTPUT | grep "End of Simulation" | wc -l
    done
  done
done

exit 0
