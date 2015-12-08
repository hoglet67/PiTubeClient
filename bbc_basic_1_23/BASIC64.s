; This source code in this file is licensed to You by Castle Technology
; Limited ("Castle") and its licensors on contractual terms and conditions
; ("Licence") which entitle you freely to modify and/or to distribute this
; source code subject to Your compliance with the terms of the Licence.
; 
; This source code has been made available to You without any warranties
; whatsoever. Consequently, Your use, modification and distribution of this
; source code is entirely at Your own risk and neither Castle, its licensors
; nor any other person who has contributed to this source code shall be
; liable to You for any loss or damage which You may suffer as a result of
; Your use, modification or distribution of this source code.
; 
; Full details of Your rights and obligations are set out in the Licence.
; You should have received a copy of the Licence with this source code file.
; If You have not received a copy, the text of the Licence is available
; online at www.castle-technology.co.uk/riscosbaselicence.htm
; 
;> BASIC64

STRONGARM                       *       0
FP                              *       0
		

		
RELEASEVER                      *       1                      ;1 for release version: no MANDEL or Roger
OWNERRORS                       *       0                      ;1 for error messages in module
CHECKCRUNCH                     *       1                      ;1 for BASIC$Crunch check on -quit and LIBRARY, INSTALL etc.
LOOKUPHELP                      *       1                      ;1 for lookup command help syntax in Global.Messages file.
DO32BIT                         *       1                      ;1 for 32-bit (and hence not ARM 2/3) compatible

VARS                            *       &8700                  ;allocation of the data pointer itself

		
        AREA    |BASIC$$Code|, CODE, READONLY, PIC
		
        LNK     Basic.s
