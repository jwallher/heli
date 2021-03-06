#!/bin/bash

KITHOME="/home/bstewar2/devkitadv/"

#ensure that an input file was passed
if [ $# -ne 1 ]
then
    echo "Usage: $(basename $0) input.c"
    exit
fi

# strip the .c extension
base=$(basename $1 .c)

#compile and pack the file into a GBA game
$KITHOME/bin/arm-agb-elf-gcc -std=c99 -O3 -nostartfiles $KITHOME/arm-agb-elf/lib/crt0.o -o $base.elf $base.c -lm
$KITHOME/bin/arm-agb-elf-objcopy -O binary $base.elf $base.gba
