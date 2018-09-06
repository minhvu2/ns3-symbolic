#!/bin/bash

FILENAME=$1

cat $FILENAME/messages.txt | grep -oP 'Range of State : \d+\s*range: [a-z0-9]*, \K[a-z0-9]*' > $FILENAME".txt"

./findmax $FILENAME".txt"
