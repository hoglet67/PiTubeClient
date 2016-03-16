#!/bin/bash
for i in 0 1
do
gcc -DPC_SIMULATION -DPROFILING -DTRACE_TO_CONSOLE -DSHOW_INSTRUCTIONS -DTEST_SUITE=${i} -o Pandora${i} Pandora.c -I ../../NS32016/ ../../NS32016/32016.c ../../NS32016/Decode.c ../../NS32016/NSDis.c  ../../NS32016/mem32016.c ../../NS32016/Trap.c ../../NS32016/Profile.c -lm
done

