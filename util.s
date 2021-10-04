_clear_wram::
  ld hl, #l__DATA
  push hl
  ld hl, #0
  push hl
  ld hl, #s__DATA
  push hl
  call _memset
  add sp, #6
  ret

; u16 drag(u16 x)
;
; equivalent to:  x -= (x >> 3), but handles sign properly
_drag::
  ; de = bc = x
  ldhl sp, #2
  ld a, (hl+)
  ld c, a
  ld e, a
  ld b, (hl)
  ld d, b
  ; bc >>= 3  (signed)
  sra b
  rr c
  sra b
  rr c
  sra b
  rr c
  ; de -= bc
  ld a, e
  sub a, c
  ld e, a
  ld a, d
  sbc a, b
  ld d, a
  ; is bc < 0?
  bit 7, b
  jr z, 1$
  ; bc >= 0, complement carry
  ccf
1$:
  ; did subtraction overflow?
  jr nc, 2$
  ; overflow, de = 0
  ld de, #0
2$:
  ret

; u8 is_valid(u8 pos, u8 diff)
;
; pos is a location in a 16x16 grid, where (x, y) is 0xYX.
;
; checks whether pos + diff would overflow the grid in any direction. If so,
; returns 0, otherwise returns pos + diff.
;
; TODO: This unfortunately means that (0, 0) is not considered valid
_is_valid::
  ldhl sp, #3
  ld a, (hl)
  ld e, a
  rlc a
  jr C, 1$
  dec hl
  ld a, (hl)
  add a, e
  ld e, a
  jr 2$
1$:
  ld a, e
  cpl
  inc a
  ld e, a
  dec hl
  ld a, (hl)
  sub a, e
  ld e, a
2$:
  ld a, #0
  daa
  ret Z
  ld e, #0
  ret

; 16-bit xorshift implementation, from John Metcalf. See
; http://www.retroprogramming.com/2017/07/xorshift-pseudorandom-numbers-in-z80.html
; Modified to use sdcc calling convention

.area _DATA

_xrnd_seed:: .ds 2

.area _CODE

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

_xrnd_init::
  ldhl sp, #2
  ld a, (hl+)
  ld (#_xrnd_seed), a
  ld a, (hl)
  ld (#_xrnd_seed + 1), a
  ret


_counter_zero::
  ldhl sp, #2
  ld a, (hl+)
  ld h, (hl)
  ld l, a

  ld a, #4
  ld (hl+), a  ; start=4
  ld a, #0xe5
  ld (hl+), a  ; 0
  ld (hl+), a  ; 0
  ld (hl+), a  ; 0
  ld (hl+), a  ; 0
  ld (hl), a   ; 0
  ret

_counter_thirty::
  ldhl sp, #2
  ld a, (hl+)
  ld h, (hl)
  ld l, a

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

_counter_inc::
  ldhl sp, #2
  ld a, (hl+)
  ld d, (hl)
  ld e, a
  ld h, d
  ld l, e   ; hl = de = [sp+2]

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

_counter_dec::
  ldhl sp, #2
  ld a, (hl+)
  ld d, (hl)
  ld e, a
  ld h, d
  ld l, e   ; hl = de = [sp+2]

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

_counter_out::
  ldhl sp, #2
  ld a, (hl+)
  ld h, (hl)
  ld l, a

  ld a, (hl+)  ; read start value
  ld b, a      ; copy it to b
  ld a, #5
  sub a, b
  ld c, a      ; c is len

  ld a, b      ; move forward to first non-zero digit
  add a, l
  ld l, a
  ld a, h
  adc a, #0
  ld h, a

  ld e, l      ; de = source
  ld d, h

  ldhl sp, #4
  ld a, (hl+)
  ld h, (hl)
  ld l, a      ; hl = dest

1$:
  ldh a, (#0x41) ; wait until VRAM is unlocked
  and a, #2
  jr nz, 1$

  ld a, (de)    ; copy tile
  inc de
  ld (hl+), a

  dec c
  jr nz, 1$
  ret

; void vram_copy(u16 dst, void* src, u8 len);
_vram_copy::
  ldhl sp, #6
  ld c, (hl)

  ldhl sp, #4
  ld a, (hl+)
  ld d, (hl)
  ld e, a  ; de = src

  ldhl sp, #2
  ld a, (hl+)
  ld h, (hl)
  ld l, a  ; hl = dst

1$:
  ldh a, (#0x41) ; wait until VRAM is unlocked
  and a, #2
  jr nz, 1$

  ld a, (de)    ; copy tile
  inc de
  ld (hl+), a

  dec c
  jr nz, 1$
  ret

; zero page vars
HEAD=0x93
TAIL=0x94
NEWTAIL=0x95
OLDTAIL=0x96
MAXDIST=0x97
DIST=0x98

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
;main.c:1703: if (!distmap[pos]) {
  ld d, #>_distmap
  ld a, (de)
  or a
  jp nz, 00130$
;main.c:1704: distmap[pos] = dist;
  ldh a, (DIST)
  ld (de), a
;main.c:1705: valid = validmap[pos];
  ld d, #>_validmap
  ld a, (de)
  ld b, a     ; b => valid
  ld c, e     ; c => pos

;main.c:1706: if ((valid & VALID_U) && !distmap[newpos = POS_U(pos)] &&
  bit 5, b
  jr z, 00104$
  ld a, c
  add #0xf0
  ld e, a     ; e => newpos
  ld d, #>_distmap
  ld a, (de)
  or a
  jr nz, 00104$
;main.c:1707: !IS_MOB_AI(tmap[newpos], newpos)) {
  ; first get the tile (tmap is aligned)
  ld d, #>_tmap
  ld a, (de)
  ; now read the flags for this tile (flags_bin is not aligned)
  add #<_flags_bin
  ld l, a
  ld a, #0
  adc #>_flags_bin
  ld h, a
  ld a, (hl)
  and #3
  jr nz, 00104$
  ; now get mobmap[newpos] and check if it is non-zero
  ld d, #>_mobmap
  ld a, (de)
  or a
  jr z, 00103$
  ; there's a mob here, but maybe it's active
  dec a
  add #<_mob_active
  ld l, a
  ld a, #0
  adc #>_mob_active
  ld h, a
  ld a, (hl)
  or a
  jr z, 00104$
00103$:
;main.c:1708: cands[newtail++] = newpos;
  ld hl, #0xff00 + NEWTAIL
  ld a, (hl)
  inc (hl)
  ld l, a
  ld h, #>_cands
  ld (hl), e
00104$:

;main.c:1710: if ((valid & VALID_D) && !distmap[newpos = POS_D(pos)] &&
  bit 4, b
  jr z, 00110$
  ld a, c
  add #0x10
  ld e, a     ; e => newpos
  ld d, #>_distmap
  ld a, (de)
  or a
  jr nz, 00110$
;main.c:1707: !IS_MOB_AI(tmap[newpos], newpos)) {
  ; first get the tile (tmap is aligned)
  ld d, #>_tmap
  ld a, (de)
  ; now read the flags for this tile (flags_bin is not aligned)
  add #<_flags_bin
  ld l, a
  ld a, #0
  adc #>_flags_bin
  ld h, a
  ld a, (hl)
  and #3
  jr nz, 00110$
  ; now get mobmap[newpos] and check if it is non-zero
  ld d, #>_mobmap
  ld a, (de)
  or a
  jr z, 00109$
  ; there's a mob here, but maybe it's active
  dec a
  add #<_mob_active
  ld l, a
  ld a, #0
  adc #>_mob_active
  ld h, a
  ld a, (hl)
  or a
  jr z, 00110$
00109$:
;main.c:1708: cands[newtail++] = newpos;
  ld hl, #0xff00 + NEWTAIL
  ld a, (hl)
  inc (hl)
  ld l, a
  ld h, #>_cands
  ld (hl), e
00110$:

;main.c:1714: if ((valid & VALID_L) && !distmap[newpos = POS_L(pos)] &&
  bit 7, b
  jr z, 00116$
  ld a, c
  dec a
  ld e, a     ; e => newpos
  ld d, #>_distmap
  ld a, (de)
  or a
  jr nz, 00116$
;main.c:1707: !IS_MOB_AI(tmap[newpos], newpos)) {
  ; first get the tile (tmap is aligned)
  ld d, #>_tmap
  ld a, (de)
  ; now read the flags for this tile (flags_bin is not aligned)
  add #<_flags_bin
  ld l, a
  ld a, #0
  adc #>_flags_bin
  ld h, a
  ld a, (hl)
  and #3
  jr nz, 00116$
  ; now get mobmap[newpos] and check if it is non-zero
  ld d, #>_mobmap
  ld a, (de)
  or a
  jr z, 00115$
  ; there's a mob here, but maybe it's active
  dec a
  add #<_mob_active
  ld l, a
  ld a, #0
  adc #>_mob_active
  ld h, a
  ld a, (hl)
  or a
  jr z, 00116$
00115$:
;main.c:1708: cands[newtail++] = newpos;
  ld hl, #0xff00 + NEWTAIL
  ld a, (hl)
  inc (hl)
  ld l, a
  ld h, #>_cands
  ld (hl), e
00116$:

;main.c:1718: if ((valid & VALID_R) && !distmap[newpos = POS_R(pos)] &&
  bit 6, b
  jr z, 00130$
  ld a, c
  inc a
  ld e, a     ; e => newpos
  ld d, #>_distmap
  ld a, (de)
  or a
  jr nz, 00130$
;main.c:1707: !IS_MOB_AI(tmap[newpos], newpos)) {
  ; first get the tile (tmap is aligned)
  ld d, #>_tmap
  ld a, (de)
  ; now read the flags for this tile (flags_bin is not aligned)
  add #<_flags_bin
  ld l, a
  ld a, #0
  adc #>_flags_bin
  ld h, a
  ld a, (hl)
  and #3
  jr nz, 00130$
  ; now get mobmap[newpos] and check if it is non-zero
  ld d, #>_mobmap
  ld a, (de)
  or a
  jr z, 00121$
  ; there's a mob here, but maybe it's active
  dec a
  add #<_mob_active
  ld l, a
  ld a, #0
  adc #>_mob_active
  ld h, a
  ld a, (hl)
  or a
  jr z, 00130$
00121$:
;main.c:1708: cands[newtail++] = newpos;
  ld hl, #0xff00 + NEWTAIL
  ld a, (hl)
  inc (hl)
  ld l, a
  ld h, #>_cands
  ld (hl), e
00130$:

;main.c:1723: } while (++head != oldtail);
  ld hl, #0xff00 + HEAD
  inc (hl)
  ld e, (hl)
  ldh a, (OLDTAIL)
  sub e
  jp  nz, 00129$

;main.c:1724: } while (++dist != maxdist && oldtail != newtail);
  ld hl, #0xff00 + DIST
  inc (hl)
  ld e, (hl)
  ldh a, (MAXDIST)
  sub e
  jr z, 00136$

  ldh a, (OLDTAIL)
  ld e, a
  ldh a, (NEWTAIL)
  sub e
  jp nz, 00133$
00136$:
;main.c:1725: }
  ret
