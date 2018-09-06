#!/bin/bash

FILENAME=$1
NUMSYM=$2

cat $FILENAME/messages.txt | grep -ozP 'State : \d+\s*range: [a-z0-9]*, [a-z0-9]*' | grep -oP '0x[a-z0-9]*, 0x[a-z0-9]*' > tmp.txt

sed 's/,//g' tmp.txt > tmp1.txt

./coverage tmp1.txt $NUMSYM
