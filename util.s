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

.globl _xrnd_seed
_xrnd_seed: .ds 2

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
