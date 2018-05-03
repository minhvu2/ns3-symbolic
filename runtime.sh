#!/bin/bash

for ((i=0; i<5; i++))
{
./waf --run scratch/tcp-bulk-send
}

exit 0
