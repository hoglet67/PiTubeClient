# Raspberry Pi Tube Client (PiTubeClient)

A project to turn a Raspberry Pi into a SPI Connected Second Processor
for the Acorn MMC Micro Model B or Master 128.

See:http://stardot.org.uk/forums/viewtopic.php?f=3&t=10421
 
# Current Memory Map

| From     | To       | Size  | Description                 |
| -------- | -------- | ----- | --------------------------- |
| 00000000 | 0000003F | 64B   | ARM vectors                 |
| 00008000 | 00008EFF | ~4KB  | Basic workspace             |
| 00008F00 | 001FFFFF | ~2MB  | Basic program storage       |
| 00200000 | 01F2FFFF | ~29MB | Unused                      |
| 01F30000 | 01F3FFFF | 64KB  | Undefined Instruction Stack |
| 01F40000 | 01F4FFFF | 64KB  | Abort Stack                 |
| 01F50000 | 01F5FFFF | 64KB  | User Stack                  |
| 01F60000 | 01F6FFFF | 64KB  | IRQ Stack                   |
| 01F70000 | 01F7FFFF | 64KB  | Supervisor Stack            |
| 01F80000 | 01FFFFFF | 512KB | Tube Client OS              |
| 02000000 | 0FFFFFFF | 224MB | Unused                      |

Notes:
* PAGE = 00008F00
* HIMEM = 00200000

# Memory Layout Changes to Consider

## Relocate BASIC above HIMEM.

Currently the BASIC 135 executable loads to 0xF000, which is in the
middle of the program area, and will cause problems for large
programs.

The only reason for this is that MMFS seems to ignore the higher
order bits of the address, and we are currently just using *RUN.

In fact, in DFS there are only two higher order bits, so the max load
address is not significantly larger anyway.

Eventually BASIC should be a relocateable module. Until then, it migh
be work writing a small boot loader.

Note: *LOAD BAS135 00200000 does work.

## Move the Tube Client OS and stacks further up.

Putting them at just below 32MB was completely arbitrary.

## Move HIMEM further up.

Again, placing HIMEM at 2MB was completely arbitrary.

# TODO List

* Implement the SWI X flag to have errors returned in a Error Block
* SWI OS_Exit should accept params in R0,R1 and R2
* SWI's OS_IntOn and OS_IntOff currently broken
* Run time debug level settings
* Commands to enable/disable the Caches
* Port JGH's Module Handling: http://mdfs.net/User/JGH/Develop/ARMCoPro/Modules/
* Address possinle IRQ Stack Memory Leak
* Provide an option for soft vs hard reset. Possibly this could be configured on the "boot command line"
* Vectors - currently we don't implement any vectors. Then implement OS_Claim and OS_Release.
* Implement more SWIs, especially
  * OS_SynchroniseCodeAreas (SWI &6E) (called by BASIC)
  * OS_Mouse
  * OS_Byte 82-84 (read memory limits)
  * OS_ReadEscapeState
  * OS_ExitAndDie
  * OS_ReadMemMapInfo
  * OS_ValidateAddress
  * OS_LeaveOS
  * OS_ReadLine32
  * The set that Sprow implements is a good target: http://www.sprow.co.uk/bbc/hardware/armcopro/armtubeoscalls.pdf

