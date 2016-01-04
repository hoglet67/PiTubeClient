#!/bin/bash

# This will contain the version of basic build here
new_disk=14

# This will contain the original version of
orig_disk=15

# Changeme!!
mmb=/media/dmb/5059-F38A/BEEB.MMB

version=135

# Keep all the files in a tmp directory
tmp=tmp

# Create tmp directory if it doesn't exist
mkdir -p ${tmp}

# Build Basic V aka BASIC105
name=BAS${version}
path=${tmp}/${name}
../not_in_git/asasm/src/asasm -From BASIC105.s -o ${path}.elf
arm-none-eabi-objcopy -O binary ${path}.elf ${path}

# Create the .inf file
echo -e "\$."${name}"\tF000\tF104" > ${path}.inf

# Build Basic VI aka BASIC64 aka 64 bit floating point basic
name64=FBAS${version}
path64=${tmp}/${name64}
../not_in_git/asasm/src/asasm -From BASIC64.s -o ${path64}.elf
arm-none-eabi-objcopy -O binary ${path64}.elf ${path64}

# Create the .inf file
echo -e "\$."${name64}"\tF000\tF108" > ${path64}.inf


ssd=${tmp}/basic${version}_new.ssd

rm -f $ssd
../not_in_git/mmb_utils/blank_ssd.pl $ssd
../not_in_git/mmb_utils/putfile.pl $ssd $path $path64 samples/*
../not_in_git/mmb_utils/info.pl $ssd
../not_in_git/mmb_utils/dkill.pl -f $mmb -y $new_disk
../not_in_git/mmb_utils/dput_ssd.pl -f $mmb $new_disk $ssd


# Rebuild disk image with original

ssd=${tmp}/basic${version}_orig.ssd

rm -f $ssd
../not_in_git/mmb_utils/blank_ssd.pl $ssd
../not_in_git/mmb_utils/putfile.pl $ssd original/BAS135 samples/*
../not_in_git/mmb_utils/info.pl $ssd
../not_in_git/mmb_utils/dkill.pl -f $mmb -y $orig_disk
../not_in_git/mmb_utils/dput_ssd.pl -f $mmb $orig_disk $ssd
