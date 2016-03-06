#!/bin/bash
gcc -DPROFILING -DTRACE_TO_CONSOLE -DSHOW_INSTRUCTIONS -DTEST_SUITE -o Pandora Pandora.c -I ../../NS32016/ ../../NS32016/32016.c ../../NS32016/NSDis.c  ../../NS32016/mem32016.c ../../NS32016/Trap.c ../../NS32016/Profile.c

