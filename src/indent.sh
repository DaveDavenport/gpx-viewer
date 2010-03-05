#!/bin/bash

for a in 'gpx-viewer.c' *.vala; do
    bcpp -bnl -i 4 -s -tbnl -yb gpx-viewer.c
done
