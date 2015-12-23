// This code was derived from Acorn's 65tube application
//
// The following changes have been made
// - ported from 26 bit ARM2 to 32 bit ARM
// - replaced the x3 trap instructions with appropriate length NOPs
// - implemented BBRn, BBSn, RMBn, SMBn

// Register usage
// r0       = scratch register
// r1       = scratch register
// r2       = scratch register (used to hold CPRS normally)
// r3       = a constant value 0f 0x0000FEF8 >> 3 (for tube register address checking)
// r4       = 6502 flags (only D and I bits maintained here, CVZN use ARM flags)
// r5       = a constant value of 0xFFFFFF00
// r6       = 6502 A
// r7       = 6502 X
// r8       = 6502 Y
// r9       = 6502 SP
// r10 (sl) = 6502 PC
// r11 (fp) = base address of 64KB of flat memory
// r12 (ip) = base address of 64-byte aligned instruction implementations
// r13 (sp) = normal ARM usage
// r14 (lr) = normal ARM usage
// r15 (pc) = normal ARM usage

.text
.global exec_65tube

.equ    N_FLAG,      0x80000000
.equ    Z_FLAG,      0x40000000
.equ    C_FLAG,      0x20000000

// Align start each instruction on a 64-byte boundary
.equ    I_ALIGN,     7

// Macro for fetching the next instruction
.macro FETCH_NEXT
        mov     pc, lr
        // Insert the current literal pool, otherwise constants are to far away and you get a build error
        .ltorg
.endm

// FETCH_NEXT used to be just this, which jumps direct to the next instruction
//        ldrb    r0, [sl], #1
//        add     pc, ip, r0, lsl #I_ALIGN

// Macro for pushing a byte to the stack, whilst correctly handing wrapping
// This relies on the 6502 memory being 64KB aligned
// Optimisations from dp11 on stardot
.macro SPUSH reg
        strb    \reg, [r9], #-1
        orr     r9, #0x0100
        // The original code was just this:
        // strb    \reg, [r9], #-1
.endm

// Macro for poping a byte from the stack, whilst correctly handing wrapping
// This relies on the 6502 memory being 64KB aligned
// Optimisations from dp11 on stardot
.macro SPOP reg
        add     r9, r9, #1
        bic     r9, #0xFF00
        ldrb    \reg, [r9,#0x0100]!
        // The original code was just this:
        // ldrb    \reg, [r9, #1]!
.endm

// Macro for pushing a half word to the stack, whilst correctly handing wrapping
// This relies on the 6502 memory being 64KB aligned
// Note: Doesn't deal with the case where SP is 0x100 - locations 0xFF is incorrectly written instead of 0x1FF
.macro SPUSHH reg
        sub     r9, r9, #1        
        strh    \reg, [r9], #-1
        orr     r9, #0x0100
.endm

// Macro for poping a half word from the stack, whilst correctly handing wrapping
// This relies on the 6502 memory being 64KB aligned
// Note: Doesn't deal with the case where SP is 0x1FF - locations 0x200 and 0x201 are read
.macro SPOPH reg
        add     r9, r9, #1
        ldrh    \reg, [r9], #1
        bic     r9, #0xFF00
        orr     r9, #0x0100
.endm
        
.macro TRACE_65TUBE
        push    {r0-r12, lr}
        mrs     r3, CPSR
        push    {r3,lr}
        mov     r0, sl
        ldrb    r1, [sl]
        bl      copro_65tube_trace
        pop     {r3,lr}
        msr     CPSR_flg, r3
        pop     {r0-r12, lr}
.endm

// Update a histogram of opcodes in memory
.macro HISTOGRAM_65TUBE
        ldr     r1, =histogram_memory
        add     r1, r0, lsl #2
        ldr     r2, [r1]
        add     r2, #1
        str     r2, [r1]
.endm

// Macros for calculating effective address in r0

.macro EA_ABSOLUTE
        ldrh    r0, [sl], #2
.endm
        
.macro EA_ABSOLUTE_INDEXED reg
        ldrh    r0, [sl], #2
        add     r0, r0, \reg, lsr #24
        bic     r0, r0, #0x10000
.endm

.macro EA_INDIRECT
        ldrb    r0, [sl], #1
        ldrh    r0, [r0, fp]
.endm
        
.macro EA_INDIRECT_X
        ldrb    r0, [sl], #1
        add     r0, r0, r7, lsr #24
        and     r0, r0, #0xff
        ldrh    r0, [r0, fp]
.endm
        
.macro EA_INDIRECT_Y        
        ldrb    r0, [sl], #1
        ldrh    r0, [r0, fp]
        add     r0, r0, r8, lsr #24
        bic     r0, r0, #0x10000
.endm
        
// Macros for reading/writing memory
.macro LOAD_INDIRECT
        ldrb    r1, [fp, r0]
.endm

.macro STORE_INDIRECT
        strb    r1, [fp, r0]
.endm

.macro LOAD_ABSOLUTE               // r0 holds the absolute address
        mrs     r2, CPSR           // preserve the 6502 flags
        cmp     r3, r0, lsr #3     // r3 holds the constant 0xFEF8 >> 3
        pusheq  {lr}               // after this, the "eq" condition means 0xFEF8 <= r0 <= 0xFEFF
        bleq    tube_load_handler  // tube memory access - must preserve current flags
        popeq   {lr}               // tube memory access
        ldrneb  r1, [fp, r0]       // normal memory read
        msr     CPSR_flg, r2       // restore 6502 flags
.endm

.macro STORE_ABSOLUTE              // r0 holds the absolute address
        mrs     r2, CPSR           // preserve the 6502 flags
        cmp     r3, r0, lsr #3     // r3 holds the constant 0xFEF8 >> 3
        pusheq  {lr}               // after this, the "eq" condition means 0xFEF8 <= r0 <= 0xFEFF
        bleq    tube_store_handler // tube memory access - must preserve current flags
        popeq   {lr}               // tube memory access
        strneb  r1, [fp, r0]       // normal memory write
        msr     CPSR_flg, r2       // restore 6502 flags
.endm

// Macros for additional Rockwell 65C02 and WDC 65C02 instructions
.macro BBR bitnum
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        ldrb    r0, [sl], #1
        mrs     r2, CPSR
        tst     r1, #(1 << \bitnum)
        lsleq   r0, r0, #24
        addeq   sl, sl, r0, asr #24
        msr     CPSR_flg, r2
        FETCH_NEXT
.endm

.macro BBS bitnum
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        ldrb    r0, [sl], #1
        mrs     r2, CPSR
        tst     r1, #(1 << \bitnum)
        lslne   r0, r0, #24
        addne   sl, sl, r0, asr #24
        msr     CPSR_flg, r2
        FETCH_NEXT
.endm

.macro RMB bitnum
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        bic     r1, #(1 << \bitnum)
        strb    r1, [fp, r0]
        FETCH_NEXT
.endm

.macro SMB bitnum
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        orr     r1, #(1 << \bitnum)
        strb    r1, [fp, r0]
        FETCH_NEXT
.endm

// **********************************************
// Start of opcode implementations
// **********************************************

// Opcode 00 - BRK
.align I_ALIGN
opcode_00:
        b       handle_brk

// Opcode 01 - ORA ($00,X)
.align I_ALIGN
opcode_01:
        EA_INDIRECT_X
        LOAD_INDIRECT
        lsl     r1, r1, #24
        orrs    r6, r6, r1
        FETCH_NEXT

// Opcode 02 - NOP #$00
.align I_ALIGN
opcode_02:
        add     sl, sl, #1
        FETCH_NEXT

// Opcode 03 - NOP
.align I_ALIGN
opcode_03:
        FETCH_NEXT

// Opcode 04 - TSB $00
.align I_ALIGN
opcode_04:
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        mrs     r2, CPSR
        tst     r1, r6, lsr #24
        bic     r2, r2, #Z_FLAG
        orreq   r2, r2, #Z_FLAG
        msr     CPSR_flg, r2
        orr     r1, r1, r6, lsr #24
        strb    r1, [fp, r0]
        FETCH_NEXT

// Opcode 05 - ORA $00
.align I_ALIGN
opcode_05:
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        lsl     r1, r1, #24
        orrs    r6, r6, r1
        FETCH_NEXT

// Opcode 06 - ASL $00
.align I_ALIGN
opcode_06:
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        lsl     r1, r1, #1
        strb    r1, [fp, r0]
        tst     r5, r1, lsl #24
        FETCH_NEXT

// Opcode 07 - RMB0 $00
.align I_ALIGN
opcode_07:
        RMB 0

// Opcode 08 - PHP
.align I_ALIGN
opcode_08:
        movpl   r0, #0x30
        movmi   r0, #0xB0
        orrcs   r0, r0, #1
        orreq   r0, r0, #2
        orr     r0, r0, r4
        SPUSH   r0
        FETCH_NEXT

// Opcode 09 - ORA #$00
.align I_ALIGN
opcode_09:
        ldrb    r1, [sl], #1
        lsl     r1, r1, #24
        orrs    r6, r6, r1
        FETCH_NEXT

// Opcode 0A - ASL A
.align I_ALIGN
opcode_0A:
        lsls    r6, r6, #1
        FETCH_NEXT

// Opcode 0B - NOP
.align I_ALIGN
opcode_0B:
        FETCH_NEXT

// Opcode 0C - TSB $0000
.align I_ALIGN
opcode_0C:
        EA_ABSOLUTE
        LOAD_ABSOLUTE
        mrs     r2, CPSR
        tst     r1, r6, lsr #24
        bic     r2, r2, #Z_FLAG
        orreq   r2, r2, #Z_FLAG
        msr     CPSR_flg, r2
        orr     r1, r1, r6, lsr #24
        STORE_ABSOLUTE
        FETCH_NEXT

// Opcode 0D - ORA $0000
.align I_ALIGN
opcode_0D:
        EA_ABSOLUTE
        LOAD_ABSOLUTE
        lsl     r1, r1, #24
        orrs    r6, r6, r1
        FETCH_NEXT

// Opcode 0E - ASL $0000
.align I_ALIGN
opcode_0E:
        EA_ABSOLUTE
        LOAD_ABSOLUTE
        lsl     r1, r1, #1
        STORE_ABSOLUTE
        tst     r5, r1, lsl #24
        FETCH_NEXT

// Opcode 0F - BBR0
.align I_ALIGN
opcode_0F:
        BBR 0

// Opcode 10 - BPL
.align I_ALIGN
opcode_10:
        ldrsb   r0, [sl], #1
        addpl   sl, sl, r0
        FETCH_NEXT

// Opcode 11 - ORA ($00),Y
.align I_ALIGN
opcode_11:
        EA_INDIRECT_Y
        LOAD_INDIRECT
        lsl     r1, r1, #24
        orrs    r6, r6, r1
        FETCH_NEXT

// Opcode 12 - ORA ($00)
.align I_ALIGN
opcode_12:
        EA_INDIRECT
        LOAD_INDIRECT
        lsl     r1, r1, #24
        orrs    r6, r6, r1
        FETCH_NEXT

// Opcode 13 - NOP
.align I_ALIGN
opcode_13:
        FETCH_NEXT

// Opcode 14 - TRB $00
.align I_ALIGN
opcode_14:
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        mrs     r2, CPSR
        tst     r1, r6, lsr #24
        bic     r2, r2, #Z_FLAG
        orreq   r2, r2, #Z_FLAG
        msr     CPSR_flg, r2
        bic     r1, r1, r6, lsr #24
        strb    r1, [fp, r0]
        FETCH_NEXT

// Opcode 15 - ORA $00,X
.align I_ALIGN
opcode_15:
        ldrb    r0, [sl], #1
        add     r0, r7, r0, lsl #24
        ldrb    r1, [fp, r0, lsr #24]
        lsl     r1, r1, #24
        orrs    r6, r6, r1
        FETCH_NEXT

// Opcode 16 - ASL $00, X
.align I_ALIGN
opcode_16:
        ldrb    r0, [sl], #1
        add     r0, r7, r0, lsl #24
        ldrb    r1, [fp, r0, lsr #24]
        lsl     r1, r1, #1
        strb    r1, [fp, r0, lsr #24]
        tst     r5, r1, lsl #24
        FETCH_NEXT

// Opcode 17 - RMB1 $00
.align I_ALIGN
opcode_17:
        RMB 1

// Opcode 18 - CLC
.align I_ALIGN
opcode_18:
        mrs     r0, CPSR
        bic     r0, r0, #C_FLAG
        msr     CPSR_flg, r0
        FETCH_NEXT

// Opcode 19 - ORA $0000,Y
.align I_ALIGN
opcode_19:
        EA_ABSOLUTE_INDEXED r8
        LOAD_ABSOLUTE
        lsl     r1, r1, #24
        orrs    r6, r6, r1
        FETCH_NEXT

// Opcode 1A - INC A
.align I_ALIGN
opcode_1A:
        add     r6, r6, #0x1000000
        teq     r6, #0
        FETCH_NEXT

// Opcode 1B - NOP
.align I_ALIGN
opcode_1B:
        FETCH_NEXT

// Opcode 1C - TRB $0000
.align I_ALIGN
opcode_1C:
        EA_ABSOLUTE
        LOAD_ABSOLUTE
        mrs     r2, CPSR
        tst     r1, r6, lsr #24
        bic     r2, r2, #Z_FLAG
        orreq   r2, r2, #Z_FLAG
        msr     CPSR_flg, r2
        bic     r1, r1, r6, lsr #24
        STORE_ABSOLUTE
        FETCH_NEXT

// Opcode 1D - ORA $0000,X
.align I_ALIGN
opcode_1D:
        EA_ABSOLUTE_INDEXED r7
        LOAD_ABSOLUTE
        lsl     r1, r1, #24
        orrs    r6, r6, r1
        FETCH_NEXT

// Opcode 1E - ASL $0000,X
.align I_ALIGN
opcode_1E:
        EA_ABSOLUTE_INDEXED r7
        LOAD_ABSOLUTE
        lsl     r1, r1, #1
        STORE_ABSOLUTE
        tst     r5, r1, lsl #24
        FETCH_NEXT

// Opcode 1F - BBR1
.align I_ALIGN
opcode_1F:
        BBR 1

// Opcode 20 - JSR $0000
.align I_ALIGN
opcode_20:
        ldrh    r0, [sl], #1
        SPUSHH  sl
        add     sl, fp, r0
        FETCH_NEXT

// Opcode 21 - AND ($00,X)
.align I_ALIGN
opcode_21:
        EA_INDIRECT_X
        LOAD_INDIRECT
        lsl     r1, r1, #24
        ands    r6, r6, r1
        FETCH_NEXT

// Opcode 22 - NOP #$00
.align I_ALIGN
opcode_22:
        add     sl, sl, #1
        FETCH_NEXT

// Opcode 23 - NOP
.align I_ALIGN
opcode_23:
        FETCH_NEXT

// Opcode 24 - BIT $00
.align I_ALIGN
opcode_24:
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        lsl     r0, r1, #24
        tst     r0, r6
        and     r1, r1, #0x40
        bic     r4, r4, #0x40
        orr     r4, r4, r1
        and     r0, r0, #N_FLAG
        mrs     r1, CPSR
        bic     r1, r1, #N_FLAG
        orr     r1, r1, r0
        msr     CPSR_flg, r1
        FETCH_NEXT

// Opcode 25 - AND     $00
.align I_ALIGN
opcode_25:
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        lsl     r1, r1, #24
        ands    r6, r6, r1
        FETCH_NEXT

// Opcode 26 - ROL $00
.align I_ALIGN
opcode_26:
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        adc     r1, r1, r1
        strb    r1, [fp, r0]
        lsls    r0, r1, #24
        FETCH_NEXT

// Opcode 27 - RMB2 $00
.align I_ALIGN
opcode_27:
        RMB 2

// Opcode 28 - PLP
.align I_ALIGN
opcode_28:
        SPOP    r0
        and     r4, r0, #0x4C
        mrs     r2, CPSR
        bic     r2, #(N_FLAG + Z_FLAG + C_FLAG)
        and     r1, r0, #0x80
        orr     r2, r2, r1, lsl #24
        and     r1, r0, #0x03
        orr     r2, r2, r1, lsl #29
        msr     CPSR_flg, r2
        FETCH_NEXT

// Opcode 29 - AND     #$00
.align I_ALIGN
opcode_29:
        ldrb    r1, [sl], #1
        lsl     r1, r1, #24
        ands    r6, r6, r1
        FETCH_NEXT

// Opcode 2A - ROL A
.align I_ALIGN
opcode_2A:
        orrcs   r6, r6, #0x800000
        lsls    r6, r6, #1
        FETCH_NEXT

// Opcode 2B - NOP
.align I_ALIGN
opcode_2B:
        FETCH_NEXT

// Opcode 2C - BIT $0000
.align I_ALIGN
opcode_2C:
        EA_ABSOLUTE
        LOAD_ABSOLUTE
        lsl     r0, r1, #24
        tst     r0, r6
        and     r1, r1, #0x40
        bic     r4, r4, #0x40
        orr     r4, r4, r1
        and     r0, r0, #N_FLAG
        mrs     r1, CPSR
        bic     r1, r1, #N_FLAG
        orr     r1, r1, r0
        msr     CPSR_flg, r1
        FETCH_NEXT

// Opcode 2D - AND     $0000
.align I_ALIGN
opcode_2D:
        EA_ABSOLUTE
        LOAD_ABSOLUTE
        lsl     r1, r1, #24
        ands    r6, r6, r1
        FETCH_NEXT

// Opcode 2E - ROL $0000
.align I_ALIGN
opcode_2E:
        EA_ABSOLUTE
        LOAD_ABSOLUTE
        adc     r1, r1, r1
        STORE_ABSOLUTE
        lsls    r0, r1, #24
        FETCH_NEXT

// Opcode 2F - BBR2
.align I_ALIGN
opcode_2F:
        BBR 2

// Opcode 30 - BMI
.align I_ALIGN
opcode_30:
        ldrsb   r0, [sl], #1
        addmi   sl, sl, r0
        FETCH_NEXT

// Opcode 31 - AND ($00),Y
.align I_ALIGN
opcode_31:
        EA_INDIRECT_Y
        LOAD_INDIRECT
        lsl     r1, r1, #24
        ands    r6, r6, r1
        FETCH_NEXT

// Opcode 32 - AND ($00)
.align I_ALIGN
opcode_32:
        EA_INDIRECT
        LOAD_INDIRECT
        lsl     r1, r1, #24
        ands    r6, r6, r1
        FETCH_NEXT

// Opcode 33 - NOP
.align I_ALIGN
opcode_33:
        FETCH_NEXT

// Opcode 34 - BIT $00,X
.align I_ALIGN
opcode_34:
        ldrb    r0, [sl], #1
        add     r0, r7, r0, lsl #24
        ldrb    r1, [fp, r0, lsr #24]
        lsl     r0, r1, #24
        tst     r0, r6
        and     r1, r1, #0x40
        bic     r4, r4, #0x40
        orr     r4, r4, r1
        and     r0, r0, #N_FLAG
        mrs     r1, CPSR
        bic     r1, r1, #N_FLAG
        orr     r1, r1, r0
        msr     CPSR_flg, r1
        FETCH_NEXT

// Opcode 35 - AND $00,X
.align I_ALIGN
opcode_35:
        ldrb    r0, [sl], #1
        add     r0, r7, r0, lsl #24
        ldrb    r1, [fp, r0, lsr #24]
        lsl     r1, r1, #24
        ands    r6, r6, r1
        FETCH_NEXT

// Opcode 36 - ROL $00,X
.align I_ALIGN
opcode_36:
        ldrb    r0, [sl], #1
        add     r0, r7, r0, lsl #24
        ldrb    r1, [fp, r0, lsr #24]
        adc     r1, r1, r1
        strb    r1, [fp, r0, lsr #24]
        lsls    r0, r1, #24
        FETCH_NEXT

// Opcode 37 - RMB3 $00
.align I_ALIGN
opcode_37:
        RMB 3

// Opcode 38 - SEC
.align I_ALIGN
opcode_38:
        mrs     r0, CPSR
        orr     r0, r0, #C_FLAG
        msr     CPSR_flg, r0
        FETCH_NEXT

// Opcode 39 - AND $0000,Y
.align I_ALIGN
opcode_39:
        EA_ABSOLUTE_INDEXED r8
        LOAD_ABSOLUTE
        lsl     r1, r1, #24
        ands    r6, r6, r1
        FETCH_NEXT

// Opcode 3A - DEC A
.align I_ALIGN
opcode_3A:
        sub     r6, r6, #0x1000000
        teq     r6, #0
        FETCH_NEXT

// Opcode 3B - NOP
.align I_ALIGN
opcode_3B:
        FETCH_NEXT

// Opcode 3C - BIT $0000,X
.align I_ALIGN
opcode_3C:
        EA_ABSOLUTE_INDEXED r7
        LOAD_ABSOLUTE
        lsl     r0, r1, #24
        tst     r0, r6
        and     r1, r1, #0x40
        bic     r4, r4, #0x40
        orr     r4, r4, r1
        and     r0, r0, #N_FLAG
        mrs     r1, CPSR
        bic     r1, r1, #N_FLAG
        orr     r1, r1, r0
        msr     CPSR_flg, r1
        FETCH_NEXT

// Opcode 3D - AND $0000,X
.align I_ALIGN
opcode_3D:
        EA_ABSOLUTE_INDEXED r7
        LOAD_ABSOLUTE
        lsl     r1, r1, #24
        ands    r6, r6, r1
        FETCH_NEXT

// Opcode 3E - ROL $0000,X
.align I_ALIGN
opcode_3E:
        EA_ABSOLUTE_INDEXED r7
        LOAD_ABSOLUTE
        adc     r1, r1, r1
        STORE_ABSOLUTE
        lsls    r0, r1, #24
        FETCH_NEXT

// Opcode 3F - BBR3
.align I_ALIGN
opcode_3F:
        BBR 3

// Opcode 40 - RTI
.align I_ALIGN
opcode_40:
        b       handle_rti

// Opcode 41 - EOR ($00,X)
.align I_ALIGN
opcode_41:
        EA_INDIRECT_X
        LOAD_INDIRECT
        lsl     r1, r1, #24
        eors    r6, r6, r1
        FETCH_NEXT

// Opcode 42 - NOP #$00
.align I_ALIGN
opcode_42:
        add     sl, sl, #1
        FETCH_NEXT

// Opcode 43 - NOP
.align I_ALIGN
opcode_43:
        FETCH_NEXT

// Opcode 44 - NOP $00
.align I_ALIGN
opcode_44:
        add     sl, sl, #1
        FETCH_NEXT

// Opcode 45 - EOR $00
.align I_ALIGN
opcode_45:
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        lsl     r1, r1, #24
        eors    r6, r6, r1
        FETCH_NEXT

// Opcode 46 - LSR $00
.align I_ALIGN
opcode_46:
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        lsrs    r1, r1, #1
        strb    r1, [fp, r0]
        lsl     r0, r1, #24
        teq     r0, #0
        FETCH_NEXT

// Opcode 47 - RMB4 $00
.align I_ALIGN
opcode_47:
        RMB 4

// Opcode 48 - PHA
.align I_ALIGN
opcode_48:
        lsr     r0, r6, #24
        SPUSH   r0
        FETCH_NEXT

// Opcode 49 - EOR #$00
.align I_ALIGN
opcode_49:
        ldrb    r1, [sl], #1
        lsl     r1, r1, #24
        eors    r6, r6, r1
        FETCH_NEXT

// Opcode 4A - LSR A
.align I_ALIGN
opcode_4A:
        lsrs    r0, r6, #25
        lsl     r6, r0, #24
        teq     r6, #0
        FETCH_NEXT

// Opcode 4B - NOP
.align I_ALIGN
opcode_4B:
        FETCH_NEXT

// Opcode 4C - JMP $0000
.align I_ALIGN
opcode_4C:
        EA_ABSOLUTE
        add     sl, r0, fp
        FETCH_NEXT

// Opcode 4D - EOR $0000
.align I_ALIGN
opcode_4D:
        EA_ABSOLUTE
        LOAD_ABSOLUTE
        lsl     r1, r1, #24
        eors    r6, r6, r1
        FETCH_NEXT

// Opcode 4E - LSR $0000
.align I_ALIGN
opcode_4E:
        EA_ABSOLUTE
        LOAD_ABSOLUTE
        lsrs    r1, r1, #1
        STORE_ABSOLUTE
        lsl     r0, r1, #24
        teq     r0, #0
        FETCH_NEXT

// Opcode 4F - BBR4
.align I_ALIGN
opcode_4F:
        BBR 4


// Opcode 50 - BVC
.align I_ALIGN
opcode_50:
        ldrsb   r0, [sl], #1
        mrs     r2, CPSR
        tst     r4, #0x40
        addeq   sl, sl, r0
        msr     CPSR_flg, r2
        FETCH_NEXT

// Opcode 51 - EOR ($00),Y
.align I_ALIGN
opcode_51:
        EA_INDIRECT_Y
        LOAD_INDIRECT
        lsl     r1, r1, #24
        eors    r6, r6, r1
        FETCH_NEXT

// Opcode 52 - EOR ($00)
.align I_ALIGN
opcode_52:
        EA_INDIRECT
        LOAD_INDIRECT
        lsl     r1, r1, #24
        eors    r6, r6, r1
        FETCH_NEXT

// Opcode 53 - NOP
.align I_ALIGN
opcode_53:
        FETCH_NEXT

// Opcode 54 - NOP $00,X
.align I_ALIGN
opcode_54:
        add     sl, sl, #1
        FETCH_NEXT

// Opcode 55 - EOR $00,X
.align I_ALIGN
opcode_55:
        ldrb    r0, [sl], #1
        add     r0, r7, r0, lsl #24
        ldrb    r1, [fp, r0, lsr #24]
        lsl     r1, r1, #24
        eors    r6, r6, r1
        FETCH_NEXT

// Opcode 56 - LSR $00,X
.align I_ALIGN
opcode_56:
        ldrb    r0, [sl], #1
        add     r0, r7, r0, lsl #24
        ldrb    r1, [fp, r0, lsr #24]
        lsrs    r1, r1, #1
        strb    r1, [fp, r0, lsr #24]
        lsl     r0, r1, #24
        teq     r0, #0
        FETCH_NEXT

// Opcode 57 - RMB5 $00
.align I_ALIGN
opcode_57:
        RMB 5

// Opcode 58 - CLI
.align I_ALIGN
opcode_58:
        bic     r4, r4, #4
        FETCH_NEXT

// Opcode 59 - EOR $0000,Y
.align I_ALIGN
opcode_59:
        EA_ABSOLUTE_INDEXED r8
        LOAD_ABSOLUTE
        lsl     r1, r1, #24
        eors    r6, r6, r1
        FETCH_NEXT

// Opcode 5A - PHY
.align I_ALIGN
opcode_5A:
        lsr     r0, r8, #24
        SPUSH   r0
        FETCH_NEXT

// Opcode 5B - NOP
.align I_ALIGN
opcode_5B:
        FETCH_NEXT

// Opcode 5C - NOP $0000
.align I_ALIGN
opcode_5C:
        add     sl, sl, #2
        FETCH_NEXT

// Opcode 5D - EOR $0000,X
.align I_ALIGN
opcode_5D:
        EA_ABSOLUTE_INDEXED r7
        LOAD_ABSOLUTE
        lsl     r1, r1, #24
        eors    r6, r6, r1
        FETCH_NEXT

// Opcode 5E - LSR $0000,X
.align I_ALIGN
opcode_5E:
        EA_ABSOLUTE_INDEXED r7
        LOAD_ABSOLUTE
        lsrs    r1, r1, #1
        STORE_ABSOLUTE
        lsl     r0, r1, #24
        teq     r0, #0
        FETCH_NEXT

// Opcode 5F - BBR5
.align I_ALIGN
opcode_5F:
        BBR 5

// Opcode 60 - RTS
.align I_ALIGN
opcode_60:
        SPOPH   r0
        add     sl, r0, #1
        add     sl, sl, fp
        FETCH_NEXT

// Opcode 61 - ADC ($00,X)
.align I_ALIGN
opcode_61:
        EA_INDIRECT_X
        LOAD_INDIRECT
        tst     r4, #8
        bne     decimal_mode_adc
        orrcs   r1, r1, r5
        adcs    r6, r6, r1, ror #8
        bicvc   r4, #0x40
        orrvs   r4, #0x40
        FETCH_NEXT

// Opcode 62 - NOP #$00
.align I_ALIGN
opcode_62:
        add     sl, sl, #1
        FETCH_NEXT

// Opcode 63 - NOP
.align I_ALIGN
opcode_63:
        FETCH_NEXT

// Opcode 64 - STZ $00
.align I_ALIGN
opcode_64:
        ldrb    r0, [sl], #1
        strb    r5, [fp, r0]
        FETCH_NEXT

// Opcode 65 - ADC $00
.align I_ALIGN
opcode_65:
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        tst     r4, #8
        bne     decimal_mode_adc
        orrcs   r1, r1, r5
        adcs    r6, r6, r1, ror #8
        bicvc   r4, #0x40
        orrvs   r4, #0x40
        FETCH_NEXT

// Opcode 66 - ROR $00
.align I_ALIGN
opcode_66:
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        orrcs   r1, r1, #0x100
        lsrs    r1, r1, #1
        strb    r1, [fp, r0]
        lsl     r0, r1, #24
        teq     r0, #0
        FETCH_NEXT

// Opcode 67 - RMB6 $00
.align I_ALIGN
opcode_67:
        RMB 6

// Opcode 68 - PLA
.align I_ALIGN
opcode_68:
        SPOP    r6
        lsl     r6, r6, #24
        teq     r6, #0
        FETCH_NEXT

// Opcode 69 - ADC #$00
.align I_ALIGN
opcode_69:
        ldrb    r1, [sl], #1
        tst     r4, #8
        bne     decimal_mode_adc
        orrcs   r1, r1, r5
        adcs    r6, r6, r1, ror #8
        bicvc   r4, #0x40
        orrvs   r4, #0x40
        FETCH_NEXT

// Opcode 6A - ROR A
.align I_ALIGN
opcode_6A:
        orrcs   r6, r6, #1
        rors    r0, r6, #25
        lsl     r6, r0, #24
        teq     r6, #0
        FETCH_NEXT

// Opcode 6B - NOP
.align I_ALIGN
opcode_6B:
        FETCH_NEXT

// Opcode 6C - JMP ($0000)
.align I_ALIGN
opcode_6C:
        ldrb    r0, [sl], #1
        ldrb    r1, [sl], #0
        add     r0, r0, r1, lsl #8
        ldrb    r1, [r0, fp]!
        ldrb    r0, [r0, #1]
        add     sl, r1, r0, lsl #8
        add     sl, sl, fp
        FETCH_NEXT

// Opcode 6D - ADC $0000
.align I_ALIGN
opcode_6D:
        EA_ABSOLUTE
        LOAD_ABSOLUTE
        tst     r4, #8
        bne     decimal_mode_adc
        orrcs   r1, r1, r5
        adcs    r6, r6, r1, ror #8
        bicvc   r4, #0x40
        orrvs   r4, #0x40
        FETCH_NEXT

// Opcode 6E - ROR $0000
.align I_ALIGN
opcode_6E:
        EA_ABSOLUTE
        LOAD_ABSOLUTE
        orrcs   r1, r1, #0x100
        lsrs    r1, r1, #1
        STORE_ABSOLUTE
        lsl     r0, r1, #24
        teq     r0, #0
        FETCH_NEXT

// Opcode 6F - BBR6
.align I_ALIGN
opcode_6F:
        BBR 6

// Opcode 70 - BVS
.align I_ALIGN
opcode_70:
        ldrsb   r0, [sl], #1
        mrs     r2, CPSR
        tst     r4, #0x40
        addne   sl, sl, r0
        msr     CPSR_flg, r2
        FETCH_NEXT

// Opcode 71 - ADC ($00),Y
.align I_ALIGN
opcode_71:
        EA_INDIRECT_Y
        LOAD_INDIRECT
        tst     r4, #8
        bne     decimal_mode_adc
        orrcs   r1, r1, r5
        adcs    r6, r6, r1, ror #8
        bicvc   r4, #0x40
        orrvs   r4, #0x40
        FETCH_NEXT

// Opcode 72 - ADC ($00)
.align I_ALIGN
opcode_72:
        EA_INDIRECT
        LOAD_INDIRECT
        tst     r4, #8
        bne     decimal_mode_adc
        orrcs   r1, r1, r5
        adcs    r6, r6, r1, ror #8
        bicvc   r4, #0x40
        orrvs   r4, #0x40
        FETCH_NEXT

// Opcode 73 - NOP
.align I_ALIGN
opcode_73:
        FETCH_NEXT

// Opcode 74 - STZ $00,X
.align I_ALIGN
opcode_74:
        ldrb    r0, [sl], #1
        add     r0, r7, r0, lsl #24
        strb    r5, [fp, r0, lsr #24]
        FETCH_NEXT

// Opcode 75 - ADC $00,X
.align I_ALIGN
opcode_75:
        ldrb    r0, [sl], #1
        add     r0, r7, r0, lsl #24
        ldrb    r1, [fp, r0, lsr #24]
        tst     r4, #8
        bne     decimal_mode_adc
        orrcs   r1, r1, r5
        adcs    r6, r6, r1, ror #8
        bicvc   r4, #0x40
        orrvs   r4, #0x40
        FETCH_NEXT

// Opcode 76 - ROR $00,X
.align I_ALIGN
opcode_76:
        ldrb    r0, [sl], #1
        add     r0, r7, r0, lsl #24
        ldrb    r1, [fp, r0, lsr #24]
        orrcs   r1, r1, #0x100
        lsrs    r1, r1, #1
        strb    r1, [fp, r0, lsr #24]
        lsl     r0, r1, #24
        teq     r0, #0
        FETCH_NEXT

// Opcode 77 - RMB7 $00
.align I_ALIGN
opcode_77:
        RMB 7

// Opcode 78 - SEI
.align I_ALIGN
opcode_78:
        orr     r4, r4, #4
        FETCH_NEXT

// Opcode 79 - ADC $0000,Y
.align I_ALIGN
opcode_79:
        EA_ABSOLUTE_INDEXED r8
        LOAD_ABSOLUTE
        tst     r4, #8
        bne     decimal_mode_adc
        orrcs   r1, r1, r5
        adcs    r6, r6, r1, ror #8
        bicvc   r4, #0x40
        orrvs   r4, #0x40
        FETCH_NEXT

// Opcode 7A - PLY
.align I_ALIGN
opcode_7A:
        SPOP    r8
        lsl     r8, r8, #24
        teq     r8, #0
        FETCH_NEXT

// Opcode 7B - NOP
.align I_ALIGN
opcode_7B:
        FETCH_NEXT

// Opcode 7C - JMP ($0000,X)
.align I_ALIGN
opcode_7C:
        EA_ABSOLUTE_INDEXED r7
        ldrb    r1, [r0, fp]!
        ldrb    r0, [r0, #1]
        add     sl, r1, r0, lsl #8
        add     sl, sl, fp
        FETCH_NEXT

// Opcode 7D - ADC $0000,X
.align I_ALIGN
opcode_7D:
        EA_ABSOLUTE_INDEXED r7
        LOAD_ABSOLUTE
        tst     r4, #8
        bne     decimal_mode_adc
        orrcs   r1, r1, r5
        adcs    r6, r6, r1, ror #8
        bicvc   r4, #0x40
        orrvs   r4, #0x40
        FETCH_NEXT

// Opcode 7E - ROR $0000,X
.align I_ALIGN
opcode_7E:
        EA_ABSOLUTE_INDEXED r7
        LOAD_ABSOLUTE
        orrcs   r1, r1, #0x100
        lsrs    r1, r1, #1
        STORE_ABSOLUTE
        lsl     r0, r1, #24
        teq     r0, #0
        FETCH_NEXT

// Opcode 7F - BBR7
.align I_ALIGN
opcode_7F:
        BBR 7

// Opcode 80 - BRA
.align I_ALIGN
opcode_80:
        ldrsb   r0, [sl], #1
        add     sl, sl, r0
        FETCH_NEXT

// Opcode 81 - STA ($00,X)
.align I_ALIGN
opcode_81:
        EA_INDIRECT_X
        lsr     r1, r6, #24
        STORE_INDIRECT
        FETCH_NEXT

// Opcode 82 - NOP #$00
.align I_ALIGN
opcode_82:
        add     sl, sl, #1
        FETCH_NEXT

// Opcode 83 - NOP
.align I_ALIGN
opcode_83:
        FETCH_NEXT

// Opcode 84 - STY $00
.align I_ALIGN
opcode_84:
        ldrb    r0, [sl], #1
        lsr     r1, r8, #24
        strb    r1, [fp, r0]
        FETCH_NEXT

// Opcode 85 - STA $00
.align I_ALIGN
opcode_85:
        ldrb    r0, [sl], #1
        lsr     r1, r6, #24
        strb    r1, [fp, r0]
        FETCH_NEXT

// Opcode 86 - STX $00
.align I_ALIGN
opcode_86:
        ldrb    r0, [sl], #1
        lsr     r1, r7, #24
        strb    r1, [fp, r0]
        FETCH_NEXT

// Opcode 87 - SMB0 $00
.align I_ALIGN
opcode_87:
        SMB 0

// Opcode 88 - DEY
.align I_ALIGN
opcode_88:
        sub     r8, r8, #0x1000000
        teq     r8, #0
        FETCH_NEXT

// Opcode 89 - BIT #$00
.align I_ALIGN
opcode_89:
        ldrb    r1, [sl], #1
        lsl     r0, r1, #24
        mrs     r2, CPSR
        and     r2, r2, #N_FLAG
        tst     r0, r6
        mrs     r1, CPSR
        bic     r1, r1, #N_FLAG
        orr     r1, r1, r2
        msr     CPSR_flg, r1
        FETCH_NEXT

// Opcode 8A - TXA
.align I_ALIGN
opcode_8A:
        movs    r6, r7
        FETCH_NEXT

// Opcode 8B - NOP
.align I_ALIGN
opcode_8B:
        FETCH_NEXT

// Opcode 8C - STY $0000
.align I_ALIGN
opcode_8C:
        EA_ABSOLUTE
        lsr     r1, r8, #24
        STORE_ABSOLUTE
        FETCH_NEXT

// Opcode 8D - STA $0000
.align I_ALIGN
opcode_8D:
        EA_ABSOLUTE
        lsr     r1, r6, #24
        STORE_ABSOLUTE
        FETCH_NEXT

// Opcode 8E - STX $0000
.align I_ALIGN
opcode_8E:
        EA_ABSOLUTE
        lsr     r1, r7, #24
        STORE_ABSOLUTE
        FETCH_NEXT

// Opcode 8F - BBS0
.align I_ALIGN
opcode_8F:
        BBS 0

// Opcode 90 - BCC
.align I_ALIGN
opcode_90:
        ldrsb   r0, [sl], #1
        addcc   sl, sl, r0
        FETCH_NEXT

// Opcode 91 - STA ($00),Y
.align I_ALIGN
opcode_91:
        EA_INDIRECT_Y
        lsr     r1, r6, #24
        STORE_INDIRECT
        FETCH_NEXT

// Opcode 92 - STA ($00)
.align I_ALIGN
opcode_92:
        EA_INDIRECT
        lsr     r1, r6, #24
        STORE_INDIRECT
        FETCH_NEXT

// Opcode 93 - NOP
.align I_ALIGN
opcode_93:
        FETCH_NEXT

// Opcode 94 - STY $00,X
.align I_ALIGN
opcode_94:
        ldrb    r0, [sl], #1
        add     r0, r7, r0, lsl #24
        lsr     r1, r8, #24
        strb    r1, [fp, r0, lsr #24]
        FETCH_NEXT

// Opcode 95 - STA $00,X
.align I_ALIGN
opcode_95:
        ldrb    r0, [sl], #1
        add     r0, r7, r0, lsl #24
        lsr     r1, r6, #24
        strb    r1, [fp, r0, lsr #24]
        FETCH_NEXT

// Opcode 96 - STX $00,Y
.align I_ALIGN
opcode_96:
        ldrb    r0, [sl], #1
        add     r0, r8, r0, lsl #24
        lsr     r1, r7, #24
        strb    r1, [fp, r0, lsr #24]
        FETCH_NEXT

// Opcode 97 - SMB1 $00
.align I_ALIGN
opcode_97:
        SMB 1

// Opcode 98 - TYA
.align I_ALIGN
opcode_98:
        movs    r6, r8
        FETCH_NEXT

// Opcode 99 - STA $0000,Y
.align I_ALIGN
opcode_99:
        EA_ABSOLUTE_INDEXED r8
        lsr     r1, r6, #24
        STORE_ABSOLUTE
        FETCH_NEXT

// Opcode 9A - TXS
.align I_ALIGN
opcode_9A:
        add     r9, fp, #0x100
        add     r9, r9, r7, lsr #24
        FETCH_NEXT

// Opcode 9B - NOP
.align I_ALIGN
opcode_9B:
        FETCH_NEXT

// Opcode 9C - STZ $0000
.align I_ALIGN
opcode_9C:
        EA_ABSOLUTE
        strb    r5, [fp, r0]
        FETCH_NEXT

// Opcode 9D - STA $0000,X
.align I_ALIGN
opcode_9D:
        EA_ABSOLUTE_INDEXED r7
        lsr     r1, r6, #24
        STORE_ABSOLUTE
        FETCH_NEXT

// Opcode 9E - STZ $0000,X
.align I_ALIGN
opcode_9E:
        EA_ABSOLUTE_INDEXED r7
        strb    r5, [fp, r0]
        FETCH_NEXT

// Opcode 9F - BBS1
.align I_ALIGN
opcode_9F:
        BBS 1

// Opcode A0 - LDY #$00
.align I_ALIGN
opcode_A0:
        ldrb    r8, [sl], #1
        lsl     r8, r8, #24
        teq     r8, #0
        FETCH_NEXT

// Opcode A1 - LDA ($00,X)
.align I_ALIGN
opcode_A1:
        EA_INDIRECT_X
        LOAD_INDIRECT
        lsl     r6, r1, #24
        teq     r6, #0
        FETCH_NEXT

// Opcode A2 - LDX #$00
.align I_ALIGN
opcode_A2:
        ldrb    r7, [sl], #1
        lsl     r7, r7, #24
        teq     r7, #0
        FETCH_NEXT

// Opcode A3 - NOP
.align I_ALIGN
opcode_A3:
        FETCH_NEXT

// Opcode A4 - LDY $00
.align I_ALIGN
opcode_A4:
        ldrb    r0, [sl], #1
        ldrb    r8, [fp, r0]
        lsl     r8, r8, #24
        teq     r8, #0
        FETCH_NEXT

// Opcode A5 - LDA $00
.align I_ALIGN
opcode_A5:
        ldrb    r0, [sl], #1
        ldrb    r6, [fp, r0]
        lsl     r6, r6, #24
        teq     r6, #0
        FETCH_NEXT

// Opcode A6 - LDX $00
.align I_ALIGN
opcode_A6:
        ldrb    r0, [sl], #1
        ldrb    r7, [fp, r0]
        lsl     r7, r7, #24
        teq     r7, #0
        FETCH_NEXT

// Opcode A7 - SMB2 $00
.align I_ALIGN
opcode_A7:
        SMB 2

// Opcode A8 - TAY
.align I_ALIGN
opcode_A8:
        movs    r8, r6
        FETCH_NEXT

// Opcode A9 - LDA #$00
.align I_ALIGN
opcode_A9:
        ldrb    r6, [sl], #1
        lsl     r6, r6, #24
        teq     r6, #0
        FETCH_NEXT

// Opcode AA - TAX
.align I_ALIGN
opcode_AA:
        movs    r7, r6
        FETCH_NEXT

// Opcode AB - NOP
.align I_ALIGN
opcode_AB:
        FETCH_NEXT

// Opcode AC - LDY $0000
.align I_ALIGN
opcode_AC:
        EA_ABSOLUTE
        LOAD_ABSOLUTE
        lsl     r8, r1, #24
        teq     r8, #0
        FETCH_NEXT

// Opcode AD - LDA $0000
.align I_ALIGN
opcode_AD:
        EA_ABSOLUTE
        LOAD_ABSOLUTE
        lsl     r6, r1, #24
        teq     r6, #0
        FETCH_NEXT

// Opcode AE - LDX $0000
.align I_ALIGN
opcode_AE:
        EA_ABSOLUTE
        LOAD_ABSOLUTE
        lsl     r7, r1, #24
        teq     r7, #0
        FETCH_NEXT

// Opcode AF - BBS2
.align I_ALIGN
opcode_AF:
        BBS 2

// Opcode B0 - BCS
.align I_ALIGN
opcode_B0:
        ldrsb   r0, [sl], #1
        addcs   sl, sl, r0
        FETCH_NEXT

// Opcode B1 - LDA ($00),Y
.align I_ALIGN
opcode_B1:
        EA_INDIRECT_Y
        LOAD_INDIRECT
        lsl     r6, r1, #24
        teq     r6, #0
        FETCH_NEXT

// Opcode B2 - LDA ($00)
.align I_ALIGN
opcode_B2:
        EA_INDIRECT
        LOAD_INDIRECT
        lsl     r6, r1, #24
        teq     r6, #0
        FETCH_NEXT

// Opcode B3 - NOP
.align I_ALIGN
opcode_B3:
        FETCH_NEXT

// Opcode B4 - LDY $00,X
.align I_ALIGN
opcode_B4:
        ldrb    r0, [sl], #1
        add     r0, r7, r0, lsl #24
        ldrb    r8, [fp, r0, lsr #24]
        lsl     r8, r8, #24
        teq     r8, #0
        FETCH_NEXT

// Opcode B5 - LDA $00,X
.align I_ALIGN
opcode_B5:
        ldrb    r0, [sl], #1
        add     r0, r7, r0, lsl #24
        ldrb    r6, [fp, r0, lsr #24]
        lsl     r6, r6, #24
        teq     r6, #0
        FETCH_NEXT

// Opcode B6 - LDX $00,Y
.align I_ALIGN
opcode_B6:
        ldrb    r0, [sl], #1
        add     r0, r8, r0, lsl #24
        ldrb    r7, [fp, r0, lsr #24]
        lsl     r7, r7, #24
        teq     r7, #0
        FETCH_NEXT

// Opcode B7 - SMB3 $00
.align I_ALIGN
opcode_B7:
        SMB 3

// Opcode B8 - CLV
.align I_ALIGN
opcode_B8:
        bic     r4, #0x40
        FETCH_NEXT

// Opcode B9 - LDA $0000,Y
.align I_ALIGN
opcode_B9:
        EA_ABSOLUTE_INDEXED r8
        LOAD_ABSOLUTE
        lsl     r6, r1, #24
        teq     r6, #0
        FETCH_NEXT

// Opcode BA - TSX
.align I_ALIGN
opcode_BA:
        lsl     r7, r9, #24
        teq     r7, #0
        FETCH_NEXT

// Opcode BB - NOP
.align I_ALIGN
opcode_BB:
        FETCH_NEXT

// Opcode BC - LDY $0000,X
.align I_ALIGN
opcode_BC:
        EA_ABSOLUTE_INDEXED r7
        LOAD_ABSOLUTE
        lsl     r8, r1, #24
        teq     r8, #0
        FETCH_NEXT

// Opcode BD - LDA $0000,X
.align I_ALIGN
opcode_BD:
        EA_ABSOLUTE_INDEXED r7
        LOAD_ABSOLUTE
        lsl     r6, r1, #24
        teq     r6, #0
        FETCH_NEXT

// Opcode BE - LDX $0000,Y
.align I_ALIGN
opcode_BE:
        EA_ABSOLUTE_INDEXED r8
        LOAD_ABSOLUTE
        lsl     r7, r1, #24
        teq     r7, #0
        FETCH_NEXT

// Opcode BF - BBS3
.align I_ALIGN
opcode_BF:
        BBS 3

// Opcode C0 - CPY #$00
.align I_ALIGN
opcode_C0:
        ldrb    r0, [sl], #1
        cmp     r8, r0, lsl #24
        FETCH_NEXT

// Opcode C1 - CMP ($00,X)
.align I_ALIGN
opcode_C1:
        EA_INDIRECT_X
        LOAD_INDIRECT
        cmp     r6, r1, lsl #24
        FETCH_NEXT

// Opcode C2 - NOP #$00
.align I_ALIGN
opcode_C2:
        add     sl, sl, #1
        FETCH_NEXT

// Opcode C3 - NOP
.align I_ALIGN
opcode_C3:
        FETCH_NEXT

// Opcode C4 - CPY $00
.align I_ALIGN
opcode_C4:
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        cmp     r8, r1, lsl #24
        FETCH_NEXT

// Opcode C5 - CMP $00
.align I_ALIGN
opcode_C5:
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        cmp     r6, r1, lsl #24
        FETCH_NEXT

// Opcode C6 - DEC $00
.align I_ALIGN
opcode_C6:
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        sub     r1, r1, #1
        strb    r1, [fp, r0]
        lsl     r0, r1, #24
        teq     r0, #0
        FETCH_NEXT

// Opcode C7 - SMB4 $00
.align I_ALIGN
opcode_C7:
        SMB 4

// Opcode C8 - INY
.align I_ALIGN
opcode_C8:
        add     r8, r8, #0x1000000
        teq     r8, #0
        FETCH_NEXT

// Opcode C9 - CMP #$00
.align I_ALIGN
opcode_C9:
        ldrb    r0, [sl], #1
        cmp     r6, r0, lsl #24
        FETCH_NEXT

// Opcode CA - DEX
.align I_ALIGN
opcode_CA:
        sub     r7, r7, #0x1000000
        teq     r7, #0
        FETCH_NEXT

// Opcode CB - WAI
.align I_ALIGN
opcode_CB:
        // TODO - Implement this
        FETCH_NEXT

// Opcode CC - CPY $0000
.align I_ALIGN
opcode_CC:
        EA_ABSOLUTE
        LOAD_ABSOLUTE
        cmp     r8, r1, lsl #24
        FETCH_NEXT

// Opcode CD - CMP $0000
.align I_ALIGN
opcode_CD:
        EA_ABSOLUTE
        LOAD_ABSOLUTE
        cmp     r6, r1, lsl #24
        FETCH_NEXT

// Opcode CE - DEC $0000
.align I_ALIGN
opcode_CE:
        EA_ABSOLUTE
        LOAD_ABSOLUTE
        sub     r1, r1, #1
        STORE_ABSOLUTE
        lsl     r0, r1, #24
        teq     r0, #0
        FETCH_NEXT

// Opcode CF - BBS4
.align I_ALIGN
opcode_CF:
        BBS 4

// Opcode D0 - BNE
.align I_ALIGN
opcode_D0:
        ldrsb   r0, [sl], #1
        addne   sl, sl, r0
        FETCH_NEXT

// Opcode D1 - CMP ($00),Y
.align I_ALIGN
opcode_D1:
        EA_INDIRECT_Y
        LOAD_INDIRECT
        cmp     r6, r1, lsl #24
        FETCH_NEXT

// Opcode D2 - CMP ($00)
.align I_ALIGN
opcode_D2:
        EA_INDIRECT
        LOAD_INDIRECT
        cmp     r6, r1, lsl #24
        FETCH_NEXT

// Opcode D3 - NOP
.align I_ALIGN
opcode_D3:
        FETCH_NEXT

// Opcode D4 - NOP $00,X
.align I_ALIGN
opcode_D4:
        add     sl, sl, #1
        FETCH_NEXT

// Opcode D5 - CMP $00,X
.align I_ALIGN
opcode_D5:
        ldrb    r0, [sl], #1
        add     r0, r7, r0, lsl #24
        ldrb    r0, [fp, r0, lsr #24]
        cmp     r6, r0, lsl #24
        FETCH_NEXT

// Opcode D6 - DEC $00,X
.align I_ALIGN
opcode_D6:
        ldrb    r0, [sl], #1
        add     r0, r7, r0, lsl #24
        ldrb    r1, [fp, r0, lsr #24]
        sub     r1, r1, #1
        strb    r1, [fp, r0, lsr #24]
        lsl     r0, r1, #24
        teq     r0, #0
        FETCH_NEXT

// Opcode D7 - SMB5 $00
.align I_ALIGN
opcode_D7:
        SMB 5

// Opcode D8 - CLD
.align I_ALIGN
opcode_D8:
        bic     r4, r4, #8
        FETCH_NEXT

// Opcode D9 - CMP $0000,Y
.align I_ALIGN
opcode_D9:
        EA_ABSOLUTE_INDEXED r8
        LOAD_ABSOLUTE
        cmp     r6, r1, lsl #24
        FETCH_NEXT

// Opcode DA - PHX
.align I_ALIGN
opcode_DA:
        lsr     r0, r7, #24
        SPUSH   r0
        FETCH_NEXT

// Opcode DB - STP
.align I_ALIGN
opcode_DB:
        // TODO - Implement this
        FETCH_NEXT

// Opcode DC - NOP $0000
.align I_ALIGN
opcode_DC:
        add     sl, sl, #2
        FETCH_NEXT

// Opcode DD - CMP $0000,X
.align I_ALIGN
opcode_DD:
        EA_ABSOLUTE_INDEXED r7
        LOAD_ABSOLUTE
        cmp     r6, r1, lsl #24
        FETCH_NEXT

// Opcode DE - DEX $0000,X
.align I_ALIGN
opcode_DE:
        EA_ABSOLUTE_INDEXED r7
        LOAD_ABSOLUTE
        sub     r1, r1, #1
        STORE_ABSOLUTE
        lsl     r0, r1, #24
        teq     r0, #0
        FETCH_NEXT

// Opcode DF - BBS5
.align I_ALIGN
opcode_DF:
        BBS 5

// Opcode E0 - CPX #$00
.align I_ALIGN
opcode_E0:
        ldrb    r1, [sl], #1
        cmp     r7, r1, lsl #24
        FETCH_NEXT

// Opcode E1 - SBC ($00,X)
.align I_ALIGN
opcode_E1:
        EA_INDIRECT_X
        LOAD_INDIRECT
        tst     r4, #8
        bne     decimal_mode_sbc
        orrcc   r1, r1, r5
        sbcs    r6, r6, r1, ror #8
        bicvc   r4, #0x40
        orrvs   r4, #0x40
        FETCH_NEXT

// Opcode E2 - NOP #$00
.align I_ALIGN
opcode_E2:
        add     sl, sl, #1
        FETCH_NEXT

// Opcode E3 - NOP
.align I_ALIGN
opcode_E3:
        FETCH_NEXT

// Opcode E4 - CPX $00
.align I_ALIGN
opcode_E4:
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        cmp     r7, r1, lsl #24
        FETCH_NEXT

// Opcode E5 - SBC $00
.align I_ALIGN
opcode_E5:
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        tst     r4, #8
        bne     decimal_mode_sbc
        orrcc   r1, r1, r5
        sbcs    r6, r6, r1, ror #8
        bicvc   r4, #0x40
        orrvs   r4, #0x40
        FETCH_NEXT

// Opcode E6 - INC $00
.align I_ALIGN
opcode_E6:
        ldrb    r0, [sl], #1
        ldrb    r1, [fp, r0]
        add     r1, r1, #1
        strb    r1, [fp, r0]
        lsl     r0, r1, #24
        teq     r0, #0
        FETCH_NEXT

// Opcode E7 - SMB6 $00
.align I_ALIGN
opcode_E7:
        SMB 6

// Opcode E8 - INX
.align I_ALIGN
opcode_E8:
        add     r7, r7, #0x1000000
        teq     r7, #0
        FETCH_NEXT

// Opcode E9 - SBC #$00
.align I_ALIGN
opcode_E9:
        ldrb    r1, [sl], #1
        tst     r4, #8
        bne     decimal_mode_sbc
        orrcc   r1, r1, r5
        sbcs    r6, r6, r1, ror #8
        bicvc   r4, #0x40
        orrvs   r4, #0x40
        FETCH_NEXT

// Opcode EA - NOP
.align I_ALIGN
opcode_EA:
        FETCH_NEXT

// Opcode EB - NOP
.align I_ALIGN
opcode_EB:
        FETCH_NEXT

// Opcode EC - CPX $0000
.align I_ALIGN
opcode_EC:
        EA_ABSOLUTE
        LOAD_ABSOLUTE
        cmp     r7, r1, lsl #24
        FETCH_NEXT

// Opcode ED - SBC $0000
.align I_ALIGN
opcode_ED:
        EA_ABSOLUTE
        LOAD_ABSOLUTE
        tst     r4, #8
        bne     decimal_mode_sbc
        orrcc   r1, r1, r5
        sbcs    r6, r6, r1, ror #8
        bicvc   r4, #0x40
        orrvs   r4, #0x40
        FETCH_NEXT

// Opcode EE - INC $0000
.align I_ALIGN
opcode_EE:
        EA_ABSOLUTE
        LOAD_ABSOLUTE
        add     r1, r1, #1
        STORE_ABSOLUTE
        lsl     r0, r1, #24
        teq     r0, #0
        FETCH_NEXT

// Opcode EF - BBS6
.align I_ALIGN
opcode_EF:
        BBS 6

// Opcode F0 - BEQ
.align I_ALIGN
opcode_F0:
        ldrsb   r0, [sl], #1
        addeq   sl, sl, r0
        FETCH_NEXT

// Opcode F1 - SBC ($00),Y
.align I_ALIGN
opcode_F1:
        EA_INDIRECT_Y
        LOAD_INDIRECT
        tst     r4, #8
        bne     decimal_mode_sbc
        orrcc   r1, r1, r5
        sbcs    r6, r6, r1, ror #8
        bicvc   r4, #0x40
        orrvs   r4, #0x40
        FETCH_NEXT

// Opcode F2 - SBC ($00)
.align I_ALIGN
opcode_F2:
        EA_INDIRECT
        LOAD_INDIRECT
        tst     r4, #8
        bne     decimal_mode_sbc
        orrcc   r1, r1, r5
        sbcs    r6, r6, r1, ror #8
        bicvc   r4, #0x40
        orrvs   r4, #0x40
        FETCH_NEXT

// Opcode F3 - NOP
.align I_ALIGN
opcode_F3:
        FETCH_NEXT

// Opcode F4 - NOP $00,X
.align I_ALIGN
opcode_F4:
        add     sl, sl, #1
        FETCH_NEXT

// Opcode F5 - SBC $00,X
.align I_ALIGN
opcode_F5:
        ldrb    r0, [sl], #1
        add     r0, r7, r0, lsl #24
        ldrb    r1, [fp, r0, lsr #24]
        tst     r4, #8
        bne     decimal_mode_sbc
        orrcc   r1, r1, r5
        sbcs    r6, r6, r1, ror #8
        bicvc   r4, #0x40
        orrvs   r4, #0x40
        FETCH_NEXT

// Opcode F6 - INC $00,X
.align I_ALIGN
opcode_F6:
        ldrb    r0, [sl], #1
        add     r0, r7, r0, lsl #24
        ldrb    r1, [fp, r0, lsr #24]
        add     r1, r1, #1
        strb    r1, [fp, r0, lsr #24]
        lsl     r0, r1, #24
        teq     r0, #0
        FETCH_NEXT

// Opcode F7 - SMB7 $00
.align I_ALIGN
opcode_F7:
        SMB 7

// Opcode F8 - SED
.align I_ALIGN
opcode_F8:
        orr     r4, r4, #8
        FETCH_NEXT

// Opcode F9 - SBC $0000,Y
.align I_ALIGN
opcode_F9:
        EA_ABSOLUTE_INDEXED r8
        LOAD_ABSOLUTE
        tst     r4, #8
        bne     decimal_mode_sbc
        orrcc   r1, r1, r5
        sbcs    r6, r6, r1, ror #8
        bicvc   r4, #0x40
        orrvs   r4, #0x40
        FETCH_NEXT

// Opcode FA - PLX
.align I_ALIGN
opcode_FA:
        SPOP    r7
        lsl     r7, r7, #24
        teq     r7, #0
        FETCH_NEXT

// Opcode FB - NOP
.align I_ALIGN
opcode_FB:
        FETCH_NEXT

// Opcode FC - NOP $0000
.align I_ALIGN
opcode_FC:
        add     sl, sl, #2
        FETCH_NEXT

// Opcode FD - SBC $0000,X
.align I_ALIGN
opcode_FD:
        EA_ABSOLUTE_INDEXED r7
        LOAD_ABSOLUTE
        tst     r4, #8
        bne     decimal_mode_sbc
        orrcc   r1, r1, r5
        sbcs    r6, r6, r1, ror #8
        bicvc   r4, #0x40
        orrvs   r4, #0x40
        FETCH_NEXT

// Opcode FE - INC $0000,X
.align I_ALIGN
opcode_FE:
        EA_ABSOLUTE_INDEXED r7
        LOAD_ABSOLUTE
        add     r1, r1, #1
        STORE_ABSOLUTE
        lsl     r0, r1, #24
        teq     r0, #0
        FETCH_NEXT

// Opcode FF - BBS7
.align I_ALIGN
opcode_FF:
        BBS 7

.align I_ALIGN

// **********************************************
// Start of opcode implementations
// **********************************************

handle_brk:
        sub     r1, sl, fp
        add     r1, r1, #1
        lsr     r0, r1, #8
        SPUSH   r0
        SPUSH   r1
        movpl   r0, #0x30
        movmi   r0, #0xB0
        orrcs   r0, r0, #1
        orreq   r0, r0, #2
        orr     r0, r0, r4
        SPUSH   r0
        add     r0, fp, #0x10000
        ldr     r0, [r0, #-4]
        add     sl, fp, r0, lsr #16
        FETCH_NEXT

handle_rti:
        SPOP    r0
        and     r4, r0, #0x4C
        mrs     r2, CPSR
        bic     r2, #(N_FLAG + Z_FLAG + C_FLAG)
        and     r1, r0, #0x80
        orr     r2, r2, r1, lsl #24
        and     r1, r0, #0x03
        orr     r2, r2, r1, lsl #29
        msr     CPSR_flg, r2
        SPOP    r0
        SPOP    r1
        add     sl, r0, r1, lsl #8
        add     sl, sl, fp
        FETCH_NEXT

decimal_mode_adc:
        and     r2, r1, #15
        and     r0, r6, #0xf000000
        adc     r2, r2, r0, lsr #24
        cmp     r2, #10
        subcs   r2, r2, #10
        lsl     r2, r2, #24
        lsr     r1, r1, #4
        adc     r6, r1, r6, lsr #28
        cmp     r6, #10
        subcs   r6, r6, #10
        orr     r6, r2, r6, lsl #28
        teq     r6, #0
        bicvc   r4, #0x40
        orrvs   r4, #0x40
        FETCH_NEXT

decimal_mode_sbc:
        and     r2, r1, #15
        and     r0, r6, #0xf000000
        rscs    r2, r2, r0, lsr #24
        addcc   r2, r2, #10
        lsr     r1, r1, #4
        rscs    r6, r1, r6, lsr #28
        addcc   r6, r6, #10
        orr     r6, r2, r6, lsl #4
        lsl     r6, r6, #24
        teq     r6, #0
        bicvc   r4, #0x40
        orrvs   r4, #0x40
        FETCH_NEXT

tube_load_handler:
        push    {r0-r3,r12,lr}
        mrs     r3, CPSR
        push    {r3}
        bl      copro_65tube_tube_read
        pop     {r3}
        msr     CPSR_flg, r3
        str     r0, [sp, #4]              // save result to r1 on the stack
        pop     {r0-r3,r12,pc}

tube_store_handler:
        push    {r0-r3,r12,lr}
        mrs     r3, CPSR
        push    {r3}
        bl      copro_65tube_tube_write
        pop     {r3}
        msr     CPSR_flg, r3
        pop     {r0-r3,r12,pc}

handle_rst:
        orr     r4, r4, #4       // Set the I bit
        bic     r4, r4, #8       // Clear the D bit
        add     r0, fp, #0x10000
        ldrh    r0, [r0, #-4]    // Fetch the address in FFFC,FFFD
        add     sl, fp, r0       // Point to this instruction in memory
        FETCH_NEXT

exec_65tube:

        push     {r0-r12,lr}
        mov      fp, r0                   // set fp (r11) to the 64K 6502 memory array
        ldr      ip, =opcode_00           // set ip (r12) to the base address of the 6502 instruction handler
        ldr      r3, =(0x0000fef8 >> 3)   // initialize the constant in r3
        ldr      r5, =0xffffff00          // initialize the constant in r5
        bl       handle_rst               // start by resetting the 6502

// execute N instructions

exec_65tube_loop:

.rept 16
        // TRACE_65TUBE
        ldrb    r0, [sl], #1              // Fetch the next instruction
        // HISTOGRAM_65TUBE
        add     r0, ip, r0, lsl #I_ALIGN  // Compute the address of the handler
        blx     r0                        // Call the handler
.endr

// Test RST_n, IRQ_n, and NMI_n and update interrupt state accordingly

exec_65tube_test:
        mrs     r2, CPSR                  // preserve the 6502 flags

        ldr     r0, =0x20200034           // TODO: This address is different on a RPI2
        ldr     r0, [r0]                  // Read GPLEV0 GPIO register
        ldr     r1, last_gplev0           // Read the previous value (so we can do edge detection)
        str     r0, last_gplev0           // remember gplev0 for next time

        tst     r0, #(1 << 7)             // Test for RST (GPIO7)
        beq     exec_65tube_exit          // exit immediatly if low

        eor     r1, r1, #(1 << 18)        // Test for NMI (GPIO18) (edge sensitive)
        orr     r1, r1, r0                // r1 (bit 18) = !lastNMIn OR NMIn
        tst     r1, #(1 << 18)            // r1 (bit 18) = 0 then we have seen an edge
        beq     exec_65tube_handle_nmi    // Take the NMI interrupt

        tst     r4, #4                    // Test whether interrupts are enabled (bit 2 = 0)
        tsteq   r0, #(1 << 17)            // Test for IRQ (GPIO17) (level sensitive, active low)
        beq     exec_65tube_handle_irq    // Take the IRQ interrupt

        msr     CPSR_flg, r2              // restore the 6502 flags
        b       exec_65tube_loop          // execute the next block of N instructions

exec_65tube_exit:
        pop      {r0-r12,lr}
        mov      pc, lr

exec_65tube_handle_irq:
        msr     CPSR_flg, r2              // Restore the 6502 flags
        sub     r1, sl, fp
        lsr     r0, r1, #8
        SPUSH   r0
        SPUSH   r1
        movpl   r0, #0
        movmi   r0, #0x80
        orrcs   r0, r0, #1
        orreq   r0, r0, #2
        orr     r0, r0, r4
        bic     r0, r0, #0x10             // Make sure B is clear
        SPUSH   r0
        orr     r4, r4, #4                // Set the I bit
        bic     r4, r4, #8                // Clear the D bit
        add     r0, fp, #0x10000
        ldrh    r0, [r0, #-2]             // Fetch the address in FFFE,FFFF
        add     sl, fp, r0                // Point to this instruction in memory
        b       exec_65tube_loop          // Execute N more instructions

exec_65tube_handle_nmi:
        msr     CPSR_flg, r2              // Restore the 6502 flags
        sub     r1, sl, fp
        lsr     r0, r1, #8
        SPUSH   r0
        SPUSH   r1
        movpl   r0, #0x20
        movmi   r0, #0xA0
        orrcs   r0, r0, #1
        orreq   r0, r0, #2
        orr     r0, r0, r4
        bic     r0, r0, #0x10             // Make sure B is clear
        SPUSH   r0
        orr     r4, r4, #4                // Set the I bit
        bic     r4, r4, #8                // Clear the D bit
        add     r0, fp, #0x10000
        ldrh    r0, [r0, #-6]             // Fetch the address in FFFA,FFFB
        add     sl, fp, r0                // Point to this instruction in memory
        b       exec_65tube_loop          // Execute N more instructions

last_gplev0:
        .word 0

