;
;   This file contains assembly language macros for PPT external modules.
;
;   PPT is (C) Janne Jalkanen 1995
;
;   $Id: externals.i,v 1.1 1995/10/02 21:52:11 jj Exp $
;

;
; EFFECTHEADER <tagarray>
;
;  This must be in the first bytes of every effect

EFFECTHEADER    MACRO
        moveq.l #-1,D0          ; exit gracefully, if someone runs us by accident
        rts
        dc.b "PPTF"             ; Identifier
        dc.l \1                 ; Tagarray
        ENDM

;
; LOADERHEADER <tagarray>
;
;  This must be in the first bytes of every filter

LOADERHEADER    MACRO
        moveq.l #-1,D0          ; exit gracefully, if someone runs us by accident
        rts
        dc.b "PPTL"             ; Identifier
        dc.l \1                 ; Tagarray
        ENDM

