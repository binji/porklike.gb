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
  ld a, (hl)
  ld b, a
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

xrnd_seed: .ds 2

.area _CODE

_xrnd::
  ;; read seed
  ld hl, #xrnd_seed
  ld a, (hl+)
  ld e, a
  ld a, (hl)
  ld d, a

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
  ld a, e
  ld (hl), a
  ret

_xrnd_init::
  ldhl sp, #2
  ld a, (hl+)
  ld (#xrnd_seed), a
  ld a, (hl)
  ld (#xrnd_seed + 1), a
  ret


_counter_zero::
  ldhl sp, #2
  ld a, (hl+)
  ld e, a
  ld a, (hl)
  ld h, a
  ld l, e

  ld a, #4
  ld (hl+), a  ; start=4
  ld a, #0xe5
  ld (hl+), a  ; 0
  ld (hl+), a  ; 0
  ld (hl+), a  ; 0
  ld (hl+), a  ; 0
  ld (hl), a   ; 0
  ret

_counter_inc::
  ldhl sp, #2
  ld a, (hl+)
  ld e, a
  ld a, (hl)
  ld h, a
  ld l, e

  ld d, #4
  inc hl
  inc hl
  inc hl
  inc hl
  inc hl

  ;; index 4
  inc (hl)    ; increment digit
  ld a, (hl)
  sub a, #0xef    ; is > 9?
  ret nz
  dec d
  ld a, #0xe5
  ld (hl-), a ; set to digit 0

  ;; index 3
  inc (hl)    ; increment digit
  ld a, (hl)
  sub a, #0xef    ; is > 9?
  jr nz, 1$
  dec d
  ld a, #0xe5
  ld (hl-), a ; set to digit 0

  ;; index 2
  inc (hl)    ; increment digit
  ld a, (hl)
  sub a, #0xef    ; is > 9?
  jr nz, 2$
  dec d
  ld a, #0xe5
  ld (hl-), a ; set to digit 0

  ;; index 1
  inc (hl)    ; increment digit
  ld a, (hl)
  sub a, #0xef    ; is > 9?
  jr nz, 3$
  dec d
  ld a, #0xe5
  ld (hl-), a ; set to digit 0

  ;; index 0
  inc (hl)    ; increment digit
  ld a, (hl)
  sub a, #0xef    ; is > 9?
  jr nz, 4$
  ld a, #0xe5
  ld (hl), a  ; set to digit 0
  jr 5$

1$: dec hl
2$: dec hl
3$: dec hl
4$: dec hl
5$:

  ; update start, if necessary
  ld a, (hl)
  sub a, d
  ret c
  ; write new start value
  ld a, d
  ld (hl), a

  ret


_counter_out::
  ldhl sp, #2
  ld a, (hl+)
  ld e, a
  ld a, (hl)
  ld h, a
  ld l, e

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
  ld b, a
  ld a, (hl)
  ld h, a
  ld l, b      ; hl = dest

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
