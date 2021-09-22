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
  jr z, $0
  ; bc >= 0, complement carry
  ccf
$0:
  ; did subtraction overflow?
  jr nc, $1
  ; overflow, de = 0
  ld de, #0
$1:
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
