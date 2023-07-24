.include "global.s"

; void clear_wram(void)
_clear_wram::
  ld hl, #l__DALIGNED
  push hl
  ld hl, #0
  push hl
  ld hl, #s__DALIGNED
  push hl
  call _memset
  add sp, #6
  ret

; void clear_map(u8* map)
;   de = map
_clear_map::
  ld l, e
  ld h, d
  xor a
  ld e, #32
1$:
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  dec e
  jr nz, 1$
  ret

; u16 drag(u16 x)
;   de = x
;   bc = result
;
; equivalent to:  x -= (x >> 3), but handles sign properly
_drag::
  ; de = bc = x
  ld c, e
  ld b, d
  ; de >>= 3  (signed)
  sra d
  rr e
  sra d
  rr e
  sra d
  rr e
  ; bc -= de
  ld a, c
  sub a, e
  ld c, a
  ld a, b
  sbc a, d
  ld b, a
  ; is de < 0?
  bit 7, d
  jr z, 1$
  ; de >= 0, complement carry
  ccf
1$:
  ; did subtraction overflow?
  jr nc, 2$
  ; overflow, bc = 0
  ld bc, #0
2$:
  ret

; u8 is_new_pos_valid(u8 pos, u8 diff)
;   a = pos
;   e = diff
;   a = result
;
; pos is a location in a 16x16 grid, where (x, y) is 0xYX.
; diff is a signed vector, from -8 to 7 in each dimension as 0xYX.
;
; checks whether pos + diff would overflow the grid in any direction. If so,
; returns 0. Otherwise, returns 1 and sets pos_result to the sum.
.area _DATA
_pos_result:: .ds 1
.area _CODE
_is_new_pos_valid::
  ld h, a       ; h=pos
  ld a, e       ; a=e=diff (as 0xYX)
  cpl
  add #0x10
  swap a
  add #0x10
  swap a
  ld d, a       ; d=-diff (as independent 0xYX)
  ld a, h       ; a=pos
  xor #0x88
  ld l, a       ; l=pos'=pos^0x88, treats position as unsigned
  sub d         ; deliberately sub instead of add to set N flag
  ld d, a       ; e=sum
  ld a, #0      ; a=0 (don't touch H or N flags)
  daa           ; if half-carry => lo nibble=0xa
  rla           ; rotate half-carry into low bit of hi nibble
  and #0x10     ; mask off hi bit (either a=0 or a=0x10)
  add d
  ld h, a       ; h=sum' (corrected in case of overflow)
  xor e
  ld d, a       ; d=sum'^diff
  ld a, l
  xor h         ; a=pos'^sum'
  and d         ; a=(pos'^sum')&(sum'^diff)
  and #0x88     ; a=overflow in hi or lo nibble
  ld a, #0
  ret nz        ; return a=0 if there was overflow
  ld a, h
  xor #0x88
  ld hl, #_pos_result
  ld (hl), a
  ld a, #1
  ret           ; otherwise return a=1 and write result to pos_result

; 16-bit xorshift implementation, from John Metcalf. See
; http://www.retroprogramming.com/2017/07/xorshift-pseudorandom-numbers-in-z80.html
; Modified to use sdcc calling convention

.area _DATA

_xrnd_seed:: .ds 2

.area _CODE

; u8 xrnd(void)
;   a = result
_xrnd::
  ;; read seed
  ld hl, #_xrnd_seed
  ld a, (hl+)
  ld d, (hl)
  ld e, a
  ld a, d

  ;; xorshift
  rra
  ld a,e
  rra
  xor d
  ld d,a
  ld a,e
  rra
  ld a,d
  rra
  xor e
  ld e,a
  xor d
  ld d,a

  ; write back seed
  ld (hl-), a
  ld (hl), e
  ret

; void xrnd_init(u16 seed)
;   de = seed
_xrnd_init::
  ld a, e
  ld (#_xrnd_seed), a
  ld a, d
  ld (#_xrnd_seed + 1), a
  ret

; void xrnd_mix(u8 entropy)
;  a = entropy
;
; mix in 8 bits of entropy by xoring with the low byte of the seed, then
; calling xrnd to distribute it across the 16 bit seed.
_xrnd_mix::
  ld d, a
  ld a, (#_xrnd_seed)
  xor d
  ld (#_xrnd_seed), a
  jp _xrnd


; void counter_zero(Counter* c)
;   de = c
_counter_zero::
  ld l, e
  ld h, d

  ld a, #4
  ld (hl+), a  ; start=4
  ld a, #0xe5
  ld (hl+), a  ; 0
  ld (hl+), a  ; 0
  ld (hl+), a  ; 0
  ld (hl+), a  ; 0
  ld (hl), a   ; 0
  ret

; void counter_thirty(Counter* c)
;   de = c
_counter_thirty::
  ld l, e
  ld h, d

  ld a, #3
  ld (hl+), a  ; start=3
  ld a, #0xe5
  ld (hl+), a  ; 0
  ld (hl+), a  ; 0
  ld (hl+), a  ; 0
  inc hl
  ld (hl-), a  ; 0
  ld a, #0xe8
  ld (hl), a   ; 3
  ret

; void counter_inc(Counter* c)
;   de = c
_counter_inc::
  ld l, e
  ld h, d

  inc hl
  inc hl
  inc hl
  inc hl
  inc hl

  ld c, #5
0$:
  inc (hl)      ; increment digit
  ld a, (hl)
  sub a, #0xef  ; exit loop if > 9?
  jr nz, 1$
  ld a, #0xe5
  ld (hl-), a   ; set to digit 0
  dec c
  jr nz, 0$
  jr 3$
1$:

  ;; move hl back to de
  ld h, d
  ld l, e
3$:
  ; move forward to the most significant digit
  inc hl

  ; search hl forward counting zeroes
  ld c, #255
2$:
  inc c
  ld a, (hl+)
  sub a, #0xe5
  jr z, 2$

  ; write new start value
  ld a, c
  ld (de), a
  ret

; void counter_dec(Counter* c)
;   de = c
_counter_dec::
  ld l, e
  ld h, d

  inc hl
  inc hl
  inc hl
  inc hl
  inc hl

  ld c, #5
0$:
  dec (hl)        ; decrement digit
  ld a, (hl)
  sub a, #0xe5
  jr nc, 1$       ; exit loop if >= 0
  ld a, #0xee
  ld (hl-), a     ; set to digit 0
  dec c
  jr nz, 0$
  jr 3$
1$:

  ;; move hl back to de
  ld h, d
  ld l, e
3$:
  ; move forward to the most significant digit
  inc hl

  ; search hl forward counting zeroes
  ld c, #255
2$:
  inc c
  ld a, (hl+)
  sub a, #0xe5
  jr z, 2$

  ; write new start value
  ld a, c
  ld (de), a
  ret

; void counter_out(Counter* c, u16 vram)
;   de = c
;   bc = vram
_counter_out::
  ld l, c      ; save low dest byte to l
  ld a, (de)   ; read start value
  inc de
  ld h, a      ; copy it to h
  ld a, #5
  sub a, h
  ld c, a      ; c = len

  ld a, h      ; move forward to first non-zero digit
  add a, e
  ld e, a
  ld a, d
  adc a, #0
  ld d, a      ; de = source
  ld h, b      ; hl = dest (high byte was loaded at start)

1$:
  ldh a, (.STAT) ; wait until VRAM is unlocked
  and a, #STATF_BUSY
  jr nz, 1$

  ld a, (de)    ; copy tile
  inc de
  ld (hl+), a

  dec c
  jr nz, 1$
  ret

; void vram_copy(u16 dst, void* src, u8 len);
;   dst = de
;   src = bc
;   len = stack
_vram_copy::
  ldhl sp, #2
  ld a, (hl)  ; a = len
  ld l, e
  ld h, d     ; hl = dst
  ld e, c
  ld d, b     ; de = src
  ld c, a     ; c = len

1$:
  ldh a, (.STAT) ; wait until VRAM is unlocked
  and a, #STATF_BUSY
  jr nz, 1$

  ld a, (de)    ; copy tile
  inc de
  ld (hl+), a

  dec c
  jr nz, 1$
  pop hl
  add sp, #-1
  pop de
  jp (hl)

; void get_bkg_palettes(u8* dest)
;   de = dest
_get_bkg_palettes::
  ld l, e
  ld h, d  ; hl = dst

  ld b, #0  ; byte index

1$:
  ldh a, (.STAT) ; wait until VRAM is unlocked
  and a, #STATF_BUSY
  jr nz, 1$

  ld a, b
  ldh (0x68), a   ; write index
  ldh a, (0x69)   ; read value
  ld (hl+), a

  inc b
  ld a, b
  cp #0x40
  jr nz, 1$
  ret

; void get_obp_palettes(u8* dest)
;   de = dest
_get_obp_palettes::
  ld l, e
  ld h, d  ; hl = dst

  ld b, #0  ; byte index

1$:
  ldh a, (.STAT) ; wait until VRAM is unlocked
  and a, #STATF_BUSY
  jr nz, 1$

  ld a, b
  ldh (0x6a), a   ; write index
  ldh a, (0x6b)   ; read value
  ld (hl+), a

  inc b
  ld a, b
  cp #0x40
  jr nz, 1$
  ret

; zero page vars
DIST=0x93
MAXDIST=0x94
HEAD=0x95
OLDTAIL=0x96
NEWTAIL=0x97

.macro CHECKDIR DIRBIT DIRADD, ?exit, ?visited, ?newcand
  bit DIRBIT, b
  jr z, exit
  ld a, c
  add #DIRADD
  ld e, a     ; e => newpos
  ld d, #>_distmap
  ld a, (de)
  or a
  jr nz, exit
  ; mark as pending
  ld a, #255
  ld d, #>_distmap
  ld (de), a
;main.c:1707: !IS_MOB_AI(tmap[newpos], newpos)) {
  ; get the flag bits
  ld d, #>_flagmap
  ld a, (de)
  and #3
  jr nz, visited
  ; now get mobmap[newpos] and check if it is non-zero
  ld d, #>_mobmap
  ld a, (de)
  or a
  jr z, newcand
  ; there's a mob here, but maybe it's active
  dec a
  add #<_mob_active
  ld l, a
  ld a, #0
  adc #>_mob_active
  ld h, a
  ld a, (hl)
  or a
  jr z, visited
newcand:
;main.c:1708: cands[newtail++] = newpos;
  ldh a, (NEWTAIL)
  ld l, a
  ld h, #>_cands
  ld (hl), e
  inc a
  ldh (NEWTAIL), a
  jr exit
visited:
  ; failed to add this pos, mark it as visited
  ld a, #254
  ld d, #>_distmap
  ld (de), a
exit:
.endm

; void calcdist_ai(u8 from, u8 to);
;   [sp+2] = from
;   [sp+3] = to
_calcdist_ai::
;main.c:1691: cands[head = 0] = to;
  xor a
  ldh (HEAD), a
  ldhl sp,  #3
  ld a, (hl)
  ld hl, #_cands
  ld (hl), a
;main.c:1692: newtail = 1;
  ld a, #1
  ldh (NEWTAIL), a
;main.c:1694: memset(distmap, 0, sizeof(distmap));
  ld hl, #_distmap
  ld d, #32
  xor a
016$:
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  dec d
  jr nz, 016$
;main.c:1696: maxdist = 255;
  ld a, #255
  ldh (MAXDIST), a
;main.c:1698: dist = 1;
  ld a, #1
  ldh (DIST), a
00133$:
;main.c:1699: oldtail = newtail;
  ldh a, (NEWTAIL)
  ldh (OLDTAIL), a
00129$:
;main.c:1701: pos = cands[head];
  ldh a, (HEAD)
  ld e, a
  ld d, #>_cands
  ld a, (de)
  ld e, a           ; e => pos
;main.c:1703: if (!(distmap[pos] == 0 || distmap[pos] == 255)) {
  ld d, #>_distmap
  ld a, (de)
  or a
  jr z, 00101$
  inc a
  jr z, 00101$
  jp 00131$
00101$:
;main.c:1702: if (pos == from) { maxdist = dist + 2; }
  ldhl sp, #2
  ld b, (hl)
  ld a, e
  sub b
  jr nz, 00102$
  ldh a, (DIST)
  add #2
  ldh (MAXDIST), a
00102$:
;main.c:1704: distmap[pos] = dist;
  ldh a, (DIST)
  ld (de), a
;main.c:1705: valid = validmap[pos];
  ld d, #>_validmap
  ld a, (de)
  ld b, a     ; b => valid
  ld c, e     ; c => pos

  CHECKDIR 5 #0xf0  ; up
  CHECKDIR 4 #0x10  ; down
  CHECKDIR 7 #0xff  ; left
  CHECKDIR 6 #0x01  ; right

00131$:
;main.c:1723: } while (++head != oldtail);
  ld hl, #0xff00 + HEAD
  inc (hl)
  ld a, (hl+)
  sub (hl)      ; OLDTAIL
  jp  nz, 00129$

;main.c:1724: } while (++dist != maxdist && oldtail != newtail);
  ld hl, #0xff00 + DIST
  inc (hl)
  ld a, (hl+)
  sub (hl)      ; MAXDIST
  jr z, 00136$

  ld hl, #0xff00 + OLDTAIL
  ld a, (hl+)
  sub (hl)      ; NEWTAIL
  jp nz, 00133$
00136$:
;main.c:1725: }
  ret
