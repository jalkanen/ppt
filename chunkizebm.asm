;-------------------------------------------------------------------------
;
;  Functions to convert from bitplane format to chunky format
;
;  $Id: chunkizebm.asm,v 1.1 1995/10/02 21:52:34 jj Exp $
;


                SECTION chunkize,CODE


;-------------------------------------------------------------------------
;
;  Chunkizer (UBYTE **planes, UBYTE *output, int width, int depth)
;             a0              a1             d0         d1
;
;  Generic bitplane-to-chunky converter.  There must be 8 elements
;  in the planes array, and they will be modified by this function.
;  Output is where to store chunky data, width is the number of
;  pixels to convert, and depth is the number of bitplanes.
;
;  Assumes starting at leftmost edge of plane pointers.


                xdef    _Chunkizer
_Chunkizer:
                movem.l d2/d4-d6/a2-a4,-(sp)

                moveq   #7,d6                   ; Init Bit offset.
                bra.s   7$

                ; Column loop.
1$              moveq   #0,d2                   ; Init color to zero.
                moveq   #1,d4                   ; Init color mask.

                move.l  a0,a3                   ; Point to planes.
                move.w  d1,d5                   ; Init plane counter.
                bra.s   4$

                ; Plane loop.
2$              move.l  (a3)+,a4                ; Get plane pointer.
                btst.b  d6,(a4)                 ; Color plane set?
                beq.s   3$                      ; Nope - skip it.
                add.w   d4,d2                   ; Yes - add in color mask.
3$              add.w   d4,d4                   ; Increment color mask.
4$              dbf     d5,2$

                ; Bump to next column.
                subq.w  #1,d6                   ; Next bit over.
                bpl.s   5$
                moveq   #7,d6                   ; Byte change...
                move.l  a0,a3
                move.w  d1,d5
                bra.s   42$
41$             addq.l  #1,(a3)+
42$             dbf     d5,41$

5$              move.b  d2,(a1)+

7$              dbf     d0,1$

                movem.l (sp)+,d2/d4-d6/a2-a4
                rts




                include 'exec/types.i'
                include 'graphics/gfx.i'


;-------------------------------------------------------------------------
;
;  ChunkizeBMRow (struct BitMap *bm, UBYTE *output, int width, int row)
;                 a0                 a1             d0         d1
;
;  Convert a scanline from a BitMap to chunky 8-bit per pixel format.
;  Calls Chunkize() above.


                xdef    _ChunkizeBMRow
_ChunkizeBMRow:
                movem.l d2/a2-a4,-(sp)

                move.l  a0,a2

                clr.l   -(sp)
                clr.l   -(sp)
                clr.l   -(sp)
                clr.l   -(sp)
                clr.l   -(sp)
                clr.l   -(sp)
                clr.l   -(sp)
                clr.l   -(sp)
                move.l  sp,a0

                mulu    bm_BytesPerRow(a2),d1

                lea     bm_Planes(a2),a3
                move.l  a0,a4
                moveq   #0,d2
                move.b  bm_Depth(a2),d2
                bra.s   2$
1$              move.l  (a3)+,(a4)
                add.l   d1,(a4)+
2$              dbf     d2,1$

                moveq   #0,d1
                move.b  bm_Depth(a2),d1

                bsr.s   _Chunkizer

                lea     32(sp),sp

                movem.l (sp)+,d2/a2-a4
                rts


                end
