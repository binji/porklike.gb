CMD_NOTE = 0
CMD_WAIT = 1
CMD_JUMP = 2

; manually assign song_ptr and wait_timer to HRAM
; TODO: better way?
ch0_wait_timer = 0xffa0
ch0_song_ptr   = ch0_wait_timer + 1
ch1_wait_timer = ch0_song_ptr   + 2
ch1_song_ptr   = ch1_wait_timer + 1
ch2_wait_timer = ch1_song_ptr   + 2
ch2_song_ptr   = ch2_wait_timer + 1
ch3_wait_timer = ch2_song_ptr   + 2
ch3_song_ptr   = ch3_wait_timer + 1
snddata_end    = ch3_song_ptr   + 2

.area _CODE

_snd_init::
  ld hl, #ch0_wait_timer
  xor a
  ld (hl+), a
  ld a, #<seq34
  ld (hl+), a
  ld a, #>seq34
  ld (hl+), a

  xor a
  ld (hl+), a
  ld a, #<seq37
  ld (hl+), a
  ld a, #>seq37
  ld (hl+), a

  xor a
  ld (hl+), a
  ld a, #<seq26
  ld (hl+), a
  ld a, #>seq26
  ld (hl+), a

  xor a
  ld (hl+), a
  ld a, #<seq00
  ld (hl+), a
  ld a, #>seq00
  ld (hl+), a

  ; TODO: set instrument in song, not here
  call square_90
  ret

_snd_update::
  ; switch to bank 2
  ld a, #2
  ld (#0x2000), a

  ld c, #<ch0_wait_timer
loop:
  ldh a, (c)
  or a
  jr z, nowait
  ; decrement wait timer
  dec a
  ldh (c), a
  jr z, nowait

  ; waiting on this channel, go to the next
  inc c
  inc c
  jr nextchannel

nowait:
  inc c
  ldh a, (c)
  ld l, a
  inc c
  ldh a, (c)
  ld h, a
  ld a, (hl+)   ; hl = song_ptr, a = *song_ptr, c = &song_ptr hi

  cp a, #1
  jr z, wait
  jr c, note

jump:
  ; read new song_ptr into hl
  ld a, (hl+)
  ld e, a
  ld a, (hl+)
  ld h, a
  ld l, e
  ; go back to &song_ptr lo
  dec c
  jr write_song_ptr

note:
  ld a, (hl+)
  ld e, a
  ld a, (hl+)
  push hl
  ld h, a
  ld l, e
  ld de, #retaddr  ; TODO: smarter way to save the return address?
  push de
  jp (hl)
retaddr:
  pop hl
  dec c
  jr write_song_ptr

wait:
  ; go back and write wait_timer
  dec c
  dec c
  ld a, (hl+)
  ldh (c), a
  ; go forward to song_ptr
  inc c

write_song_ptr:
  ; write back new song_ptr
  ld a, l
  ldh (c), a
  inc c
  ld a, h
  ldh (c), a

nextchannel:
  inc c
  ; finished?
  ld a, c
  cp a, #snddata_end
  jr nz, loop

  ; switch to previous bank
  ldh a, (__current_bank)
  ld (#0x2000), a
  ret


square_90:
  ld hl, #0xff1a
  xor a
  ld (hl), a     ; disable ch3

  ld l, #0x30
  ld a, #0x99
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  xor a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a
  ld (hl+), a

  ld l, #0x1a
  ld a, #0x80
  ld (hl), a     ; enable ch3
  ret


.area _CODE_2

.macro NOTE addr
  .db CMD_NOTE
  .dw addr
.endm

.macro NTWT addr delay
  NOTE addr
  WAIT delay
.endm

.macro FADE100 delay    ; 21 + N
  NTWT ch3_vol100, 9    ; 10
  NTWT ch3_vol50, 5     ; 6
  NTWT ch3_vol25, 3     ; 4
  NTWT ch3_mute, delay  ; 1 + N
.endm

.macro FADE100_NODELAY  ; 21
  NTWT ch3_vol100, 9    ; 10
  NTWT ch3_vol50, 5     ; 6
  NTWT ch3_vol25, 3     ; 4
  NOTE ch3_mute         ; 1
.endm

.macro FADE50 delay     ; 15 + N
  NTWT ch3_vol50, 8     ; 9
  NTWT ch3_vol25, 4     ; 5
  NTWT ch3_mute, delay  ; 1 + N
.endm

.macro FADE25 delay     ; 8 + N
  NTWT ch3_vol25, 6     ; 7
  NTWT ch3_mute, delay  ; 1 + N
.endm

.macro WAIT delay
  .db CMD_WAIT
  .db delay
.endm

.macro JUMP addr
  .db CMD_JUMP
  .dw addr
.endm

seq00::
  WAIT 255
  WAIT 255
  WAIT 194
  JUMP seq00

;; CHANNEL 3 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.macro CH3_NOTE name lo hi
name::
  ld hl, #0xff1d
  ld a, #lo
  ld (hl+), a
  ld a, #hi
  ld (hl+), a
  ret
.endm

; TODO set ch3 instrument
seq34::
  NOTE ch3_d0
  FADE100, 22
  FADE25, 36
  FADE50, 73
  FADE50, 73
  FADE100, 45
  FADE25, 14  ; TODO: fadein
  NOTE ch3_cs0
  FADE100, 22
  FADE25, 36
  FADE50, 73
  FADE50, 73
  FADE50, 73
seq38::
  NOTE ch3_d0
  FADE100, 22
  FADE25, 36
  FADE50, 73
  FADE50, 73
  FADE100, 45
  FADE25, 14  ; TODO: fadein
  NOTE ch3_f0
  FADE100, 22
  FADE25, 36
  FADE50, 73
  FADE50, 73
  FADE50, 73
seq42::
  NOTE ch3_g0
  FADE100, 22
  FADE25, 36
  FADE50, 73
  FADE50, 73
  FADE100, 45
  FADE25, 14  ; TODO: fadein
  NOTE ch3_ds0
  FADE100, 22
  FADE25, 36
  FADE50, 73
  FADE50, 73
  FADE100, 67
seq43::
  NOTE ch3_a0
  FADE100, 22
  FADE25, 36
  FADE50, 73
  FADE50, 73
  FADE100, 45
  FADE25, 14  ; TODO: fadein
  NOTE ch3_as0
  FADE100, 22
  FADE25, 36
  FADE50, 73
  FADE50, 73
  NOTE ch3_e0
  FADE50, 28
  NOTE ch3_a0
  FADE50, 28
seq50::
  NOTE ch3_d0
  FADE100_NODELAY
  NOTE ch3_a0
  FADE50, 6
  NOTE ch3_f1
  FADE25, 13
  NOTE ch3_d0
  FADE50, 6
  FADE50, 7
  NOTE ch3_a0
  FADE50, 6
  NOTE ch3_f1
  FADE25, 35
  NOTE ch3_d0
  FADE50, 72
  FADE50, 7
  NOTE ch3_f1
  FADE50, 6
  FADE25, 14
  NOTE ch3_d0
  FADE25, 13  ; TODO: fadein
  NOTE ch3_cs0
  FADE100_NODELAY
  NOTE ch3_gs0
  FADE50, 6
  NOTE ch3_e1
  FADE25, 13
  NOTE ch3_cs0
  FADE50, 6
  FADE50, 7
  NOTE ch3_gs0
  FADE50, 6
  NOTE ch3_e1
  FADE25, 35
  NOTE ch3_cs0
  FADE50, 72
  FADE50, 7
  NOTE ch3_e1
  FADE50, 6
  FADE25, 36
seq51::
  NOTE ch3_d0
  FADE100_NODELAY
  NOTE ch3_a0
  FADE50, 6
  NOTE ch3_f1
  FADE25, 13
  NOTE ch3_d0
  FADE50, 6
  FADE50, 7
  NOTE ch3_a0
  FADE50, 6
  NOTE ch3_f1
  FADE25, 35
  NOTE ch3_d0
  FADE50, 72
  FADE100, 1
  NOTE ch3_f1
  FADE50, 6
  FADE25, 14
  NOTE ch3_d0
  FADE25, 13  ; TODO: fadein
  NOTE ch3_f0
  FADE100_NODELAY
  NOTE ch3_c1
  FADE50, 6
  NOTE ch3_gs1
  FADE25, 13
  NOTE ch3_f0
  FADE50, 6
  FADE50, 7
  NOTE ch3_c1
  FADE50, 6
  NOTE ch3_gs1
  FADE25, 35
  NOTE ch3_f0
  FADE50, 72
  FADE50, 7
  NOTE ch3_gs1
  FADE50, 6
  FADE25, 36
seq52::
  NOTE ch3_g0
  FADE100_NODELAY
  NOTE ch3_d1
  FADE50, 6
  NOTE ch3_as1
  FADE25, 13
  NOTE ch3_g0
  FADE50, 6
  FADE50, 7
  NOTE ch3_d1
  FADE50, 6
  NOTE ch3_as1
  FADE25, 35
  NOTE ch3_g0
  FADE50, 72
  FADE100, 1
  NOTE ch3_as1
  FADE50, 6
  FADE25, 14
  NOTE ch3_g0
  FADE25, 13  ; TODO: fadein
  NOTE ch3_ds0
  FADE100_NODELAY
  NOTE ch3_as1
  FADE50, 6
  NOTE ch3_g1
  FADE25, 13
  NOTE ch3_ds0
  FADE50, 6
  FADE50, 7
  NOTE ch3_as1
  FADE50, 6
  NOTE ch3_g1
  FADE25, 35
  NOTE ch3_ds0
  FADE50, 72
  FADE50, 7
  NOTE ch3_as1
  FADE50, 6
  FADE25, 36
seq53::
  NOTE ch3_a0
  FADE100_NODELAY
  NOTE ch3_e1
  FADE50, 6
  NOTE ch3_f1
  FADE25, 13
  NOTE ch3_a0
  FADE50, 6
  FADE50, 7
  NOTE ch3_e1
  FADE50, 6
  NOTE ch3_f1
  FADE25, 35
  NOTE ch3_a0
  FADE50, 72
  FADE100, 1
  NOTE ch3_e1
  FADE50, 6
  NOTE ch3_cs1
  FADE25, 13
  NOTE ch3_a0
  FADE25, 13  ; TODO: fadein
  NOTE ch3_as0
  FADE100_NODELAY
  NOTE ch3_e1
  FADE50, 6
  NOTE ch3_f1
  FADE25, 13
  NOTE ch3_as0
  FADE50, 6
  FADE50, 7
  NOTE ch3_e1
  FADE50, 6
  NOTE ch3_f1
  FADE25, 35
  NOTE ch3_as0
  FADE50, 72
  NOTE ch3_a0
  FADE50, 6
  NOTE ch3_a1
  FADE50, 6
  NOTE ch3_cs1
  FADE25, 13
  NOTE ch3_a0
  WAIT 21

  JUMP seq34

CH3_NOTE ch3_cs0 0x4a 0x84
CH3_NOTE ch3_d0  0x83 0x84
CH3_NOTE ch3_ds0 0xb5 0x84
CH3_NOTE ch3_e0  0xe2 0x84
CH3_NOTE ch3_f0  0x10 0x85
CH3_NOTE ch3_g0  0x63 0x85
CH3_NOTE ch3_gs0 0x88 0x85
CH3_NOTE ch3_a0  0xaa 0x85
CH3_NOTE ch3_as0 0xcd 0x85
CH3_NOTE ch3_c1  0x0a 0x86
CH3_NOTE ch3_cs1 0x26 0x86
CH3_NOTE ch3_d1  0x41 0x86
CH3_NOTE ch3_e1  0x72 0x86
CH3_NOTE ch3_f1  0x88 0x86
CH3_NOTE ch3_g1  0xb1 0x86
CH3_NOTE ch3_gs1 0xc4 0x86
CH3_NOTE ch3_a1  0xd6 0x86
CH3_NOTE ch3_as1 0xe7 0x86

ch3_vol100::
  ld a, #0b01000000
  ldh (#0x1c), a
  ret

ch3_vol50::
  ld a, #0b01000000
  ldh (#0x1c), a
  ret

ch3_vol25::
  ld a, #0b01100000
  ldh (#0x1c), a
  ret

ch3_mute::
  ld a, #0b00000000
  ldh (#0x1c), a
  ret

;; CHANNEL 1/2 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ENV_1HOLD = 0b00100000
ENV_1FADE = 0b00100011
ENV_2HOLD = 0b01000000
ENV_2FADE = 0b01000011
ENV_3HOLD = 0b01100000
ENV_3FADE = 0b01100011
ENV_4HOLD = 0b10000000
ENV_5HOLD = 0b10100000

.macro CH12_NOTE name lo hi
name::
  ld a, #lo
  ld (hl+), a
  ld a, #hi
  ld (hl+), a
  ret
.endm

.macro CH1_NOTE_ENV name env ch
name::
  ld hl, #0xff12
  ld a, #env
  ld (hl+), a
  jr ch
.endm

.macro CH2_NOTE_ENV name env ch
name::
  ld hl, #0xff17
  ld a, #env
  ld (hl+), a
  jr ch
.endm

ch1_mute::
  xor a
  ldh (#0x12), a
  ret

ch2_mute::
  xor a
  ldh (#0x17), a
  ret

seq37::
  WAIT 44
  NTWT ch1_a1_1_fade, 21 
  NTWT ch1_a1_1_fade, 21 
  NTWT ch1_d2_2_fade, 21 
  NTWT ch1_d2_1_fade, 21 
  NTWT ch1_f2_1_fade, 21 
  NTWT ch1_f2_1_fade, 21 
  NTWT ch1_as2_2_fade, 21 
  NTWT ch1_as2_1_fade, 21 
  NTWT ch1_a2_2_fade, 21 
  NTWT ch1_a2_1_fade, 21 
  NTWT ch1_f2_2_fade, 21 
  NTWT ch1_f2_1_fade, 21 
  NTWT ch1_a1_2_fade, 21 
  NTWT ch1_a1_1_fade, 21 
  NTWT ch1_gs1_2_hold, 21 
  NTWT ch1_gs1_2_fade, 21   ; TODO: vibrato
  NTWT ch1_gs1_2_fade, 21 
  NTWT ch1_gs1_1_fade, 109 
  NTWT ch1_e1_1_fade, 21   ; TODO: fadein
  NTWT ch1_c1_1_hold, 21 
  NTWT ch1_cs1_2_hold, 21 
  NTWT ch1_e1_3_hold, 21 
  NTWT ch1_gs1_3_hold, 21 
  NTWT ch1_a1_2_hold, 21 
  NTWT ch1_gs1_1_fade, 21   ; TODO: vibrato
  NTWT ch1_e1_1_fade, 21 
seq41::
  WAIT 44
  NTWT ch1_a1_1_fade, 21 
  NTWT ch1_a1_1_fade, 21 
  NTWT ch1_d2_2_fade, 21 
  NTWT ch1_d2_1_fade, 21 
  NTWT ch1_f2_2_fade, 21 
  NTWT ch1_f2_1_fade, 21 
  NTWT ch1_as2_2_fade, 21 
  NTWT ch1_as2_1_fade, 21 
  NTWT ch1_a2_1_fade, 21 
  NTWT ch1_a2_1_fade, 21 
  NTWT ch1_f2_2_fade, 21 
  NTWT ch1_f2_1_fade, 21 
  NTWT ch1_d3_1_fade, 21 
  NTWT ch1_d3_1_fade, 21 
  NTWT ch1_cs3_2_hold, 21 
  NTWT ch1_cs3_1_fade, 21   ; TODO: vibrato
  NTWT ch1_cs3_1_fade, 21 
  NTWT ch1_cs3_1_fade, 131 
  NTWT ch1_cs1_2_hold, 21 
  NTWT ch1_f1_3_hold, 21 
  NTWT ch1_gs1_4_hold, 21 
  NTWT ch1_a1_5_hold, 21 
  NTWT ch1_gs1_4_hold, 21 
  NTWT ch1_cs2_3_hold, 21 
  NTWT ch1_f2_1_hold, 20 
  NOTE ch1_mute
seq48::
  WAIT 44
  NTWT ch1_as2_1_fade, 21 
  NTWT ch1_as2_1_fade, 21 
  NTWT ch1_g2_2_fade, 21 
  NTWT ch1_g2_1_fade, 21 
  NTWT ch1_d2_1_fade, 21 
  NTWT ch1_d2_1_fade, 21 
  NTWT ch1_as2_2_fade, 21 
  NTWT ch1_as2_1_fade, 21 
  NTWT ch1_g2_1_fade, 21 
  NTWT ch1_g2_1_fade, 21 
  NTWT ch1_cs2_2_hold, 21 
  NTWT ch1_cs2_2_fade, 21   ; TODO: vibrato
  NTWT ch1_d2_2_fade, 21 
  NTWT ch1_d2_1_fade, 21 
  NTWT ch1_g2_2_hold, 21 
  NTWT ch1_g2_2_fade, 21   ; TODO: vibrato
  NTWT ch1_g2_1_fade, 21 
  NTWT ch1_g2_1_fade, 131 
  NTWT ch1_ds1_1_hold, 21 
  NTWT ch1_g1_2_hold, 21 
  NTWT ch1_as1_3_hold, 21 
  NTWT ch1_a1_3_hold, 21 
  NTWT ch1_as1_3_fade, 21   ; TODO: vibrato
  NTWT ch1_a1_2_hold, 21 
  NTWT ch1_g1_1_fade, 21 
seq49::
  NTWT ch1_cs2_2_fade, 21 
  NTWT ch1_cs2_1_fade, 21 
  NTWT ch1_as2_2_fade, 21 
  NTWT ch1_as2_1_fade, 21 
  NTWT ch1_a2_1_fade, 21 
  NTWT ch1_a2_1_fade, 21 
  NTWT ch1_e2_2_fade, 21 
  NTWT ch1_e2_1_fade, 21 
  NTWT ch1_as2_2_fade, 21 
  NTWT ch1_as2_1_fade, 21 
  NTWT ch1_a2_2_fade, 21 
  NTWT ch1_a2_1_fade, 21 
  NTWT ch1_e2_2_fade, 21   ; TODO: vibrato
  NTWT ch1_e2_1_fade, 21 
  NTWT ch1_f2_2_fade, 21 
  NTWT ch1_f2_1_fade, 21 
  NTWT ch1_gs2_2_hold, 21 
  NTWT ch1_gs2_2_fade, 21   ; TODO: vibrato
  NTWT ch1_gs2_1_fade, 21 
  NTWT ch1_gs2_1_fade, 43 
  NTWT ch1_d2_1_fade, 21 
  NTWT ch1_f2_1_fade, 21 
  NTWT ch1_as2_1_fade, 21 
  NTWT ch1_d3_1_fade, 21   ; TODO: vibrato
  NTWT ch1_d3_1_fade, 21   ; TODO: vibrato
  NTWT ch1_e3_1_hold, 21 
  NTWT ch1_d3_1_fade, 21   ; TODO: arpreggio
  NTWT ch1_cs3_1_hold, 21 
  NTWT ch1_cs3_1_fade, 21   ; TODO: vibrato
  NTWT ch1_cs3_1_hold, 21 
  NTWT ch1_cs3_1_fade, 21 
seq35::
  NTWT ch1_a2_1_hold, 21 
  NTWT ch1_f2_1_hold, 21 
  NTWT ch1_cs2_2_fade, 21 
  NTWT ch1_d2_3_fade, 21 
  NTWT ch1_a2_3_fade, 21 
  NTWT ch1_f2_2_hold, 21 
  NTWT ch1_cs2_1_fade, 21 
  NTWT ch1_d2_1_fade, 21 
  NTWT ch1_a2_1_fade, 21   ; TODO: vibrato
  NTWT ch1_a2_1_fade, 21 
  NTWT ch1_a2_2_fade, 21 
  NTWT ch1_as2_2_fade, 21 
  NTWT ch1_a2_2_fade, 21 
  NTWT ch1_a2_1_fade, 21 
  NTWT ch1_d1_1_fade, 21 
  NTWT ch1_f1_1_fade, 21 
  NTWT ch1_gs2_1_fade, 21   ; TODO: fadein
  NTWT ch1_e2_1_hold, 21 
  NTWT ch1_cs2_2_fade, 21 
  NTWT ch1_e2_3_fade, 21 
  NTWT ch1_gs2_3_fade, 21 
  NTWT ch1_e2_2_hold, 21 
  NTWT ch1_cs2_1_fade, 21 
  NTWT ch1_e2_1_fade, 21 
  NTWT ch1_gs2_1_fade, 21   ; TODO: vibrato
  NTWT ch1_gs2_1_fade, 21 
  NTWT ch1_gs2_2_fade, 21 
  NTWT ch1_a2_2_fade, 21 
  NTWT ch1_gs2_2_fade, 21 
  NTWT ch1_gs2_1_fade, 21 
  NTWT ch1_cs1_1_fade, 21 
  NTWT ch1_e1_1_fade, 21 
seq39::
  NTWT ch1_a2_1_fade, 21   ; TODO: fadein
  NTWT ch1_f2_1_hold, 21 
  NTWT ch1_cs2_2_fade, 21 
  NTWT ch1_d2_2_fade, 21 
  NTWT ch1_a2_2_fade, 21 
  NTWT ch1_f2_2_hold, 21 
  NTWT ch1_cs2_1_fade, 21 
  NTWT ch1_d2_1_fade, 21 
  NTWT ch1_a2_1_fade, 21   ; TODO: vibrato
  NTWT ch1_a2_1_fade, 21 
  NTWT ch1_a2_2_hold, 21 
  NTWT ch1_as2_2_hold, 21 
  NTWT ch1_a2_2_fade, 21 
  NTWT ch1_a2_1_fade, 21 
  NTWT ch1_d1_2_fade, 21 
  NTWT ch1_f1_1_fade, 21 
  NTWT ch1_gs2_1_fade, 21   ; TODO: fadein
  NTWT ch1_f2_1_hold, 21 
  NTWT ch1_c2_2_fade, 21 
  NTWT ch1_cs2_2_fade, 21 
  NTWT ch1_gs2_2_fade, 21 
  NTWT ch1_f2_2_hold, 21 
  NTWT ch1_c2_1_fade, 21 
  NTWT ch1_cs2_1_fade, 21 
  NTWT ch1_gs2_1_fade, 21   ; TODO: vibrato
  NTWT ch1_gs2_1_fade, 21 
  NTWT ch1_e2_2_hold, 21 
  NTWT ch1_f2_2_hold, 21 
  NTWT ch1_gs2_2_fade, 21 
  NTWT ch1_gs2_1_fade, 21 
  NTWT ch1_e1_2_fade, 21 
  NTWT ch1_f1_1_fade, 21 
seq44::
  NTWT ch1_as2_1_fade, 21   ; TODO: vibrato
  NTWT ch1_g2_2_hold, 21 
  NTWT ch1_d2_2_fade, 21 
  NTWT ch1_g2_1_fade, 21 
  NTWT ch1_as2_2_fade, 21 
  NTWT ch1_g2_2_hold, 21 
  NTWT ch1_d2_1_fade, 21 
  NTWT ch1_g2_1_fade, 21 
  NTWT ch1_a2_1_fade, 21   ; TODO: vibrato
  NTWT ch1_a2_1_fade, 21   ; TODO: vibrato
  NTWT ch1_as2_2_fade, 21 
  NTWT ch1_a2_2_fade, 21 
  NTWT ch1_g2_2_fade, 21 
  NTWT ch1_g2_1_fade, 21 
  NTWT ch1_d1_2_fade, 21 
  NTWT ch1_g1_1_fade, 21 
  NTWT ch1_as2_1_fade, 21   ; TODO: fadein
  NTWT ch1_g2_1_hold, 21 
  NTWT ch1_ds2_2_fade, 21 
  NTWT ch1_g2_2_fade, 21 
  NTWT ch1_as2_2_fade, 21 
  NTWT ch1_g2_2_hold, 21 
  NTWT ch1_ds2_1_fade, 21 
  NTWT ch1_g2_1_fade, 21 
  NTWT ch1_a2_1_fade, 21   ; TODO: vibrato
  NTWT ch1_a2_1_fade, 21   ; TODO: vibrato
  NTWT ch1_as2_2_fade, 21 
  NTWT ch1_a2_2_fade, 21 
  NTWT ch1_g2_2_fade, 21 
  NTWT ch1_g2_1_fade, 21 
  NTWT ch1_ds1_2_fade, 21 
  NTWT ch1_g1_1_fade, 21 
seq45::
  NTWT ch1_a2_1_fade, 21   ; TODO: fadein
  NTWT ch1_e2_1_hold, 21 
  NTWT ch1_cs2_2_fade, 21 
  NTWT ch1_f2_1_fade, 21 
  NTWT ch1_a2_2_fade, 21 
  NTWT ch1_e2_2_hold, 21 
  NTWT ch1_cs2_1_fade, 21 
  NTWT ch1_f2_1_fade, 21 
  NTWT ch1_a2_1_fade, 21   ; TODO: vibrato
  NTWT ch1_as2_1_hold, 21 
  NTWT ch1_a2_2_hold, 21 
  NTWT ch1_g2_1_hold, 21 
  NTWT ch1_a2_1_fade, 21   ; TODO: vibrato
  NTWT ch1_a2_1_fade, 21 
  NTWT ch1_cs1_2_fade, 21 
  NTWT ch1_e1_1_fade, 21 
  NTWT ch1_gs2_1_fade, 21   ; TODO: fadein
  NTWT ch1_f2_1_hold, 21 
  NTWT ch1_d2_2_fade, 21 
  NTWT ch1_as1_1_fade, 21 
  NTWT ch1_gs2_1_fade, 21 
  NTWT ch1_f2_2_hold, 21 
  NTWT ch1_d2_1_fade, 21 
  NTWT ch1_as1_1_fade, 21 
  NTWT ch1_gs2_2_fade, 21   ; TODO: vibrato
  NTWT ch1_gs2_1_fade, 21 
  NTWT ch1_f2_1_fade, 21 
  NTWT ch1_gs2_2_fade, 21 
  NTWT ch1_g2_2_fade, 21 
  NTWT ch1_f2_1_fade, 21 
  NTWT ch1_e2_2_fade, 21 
  NTWT ch1_cs2_1_fade, 21 
  JUMP seq37

seq26::
  WAIT 66
  NTWT ch2_a1_1_fade, 21 
  NTWT ch2_a1_1_fade, 21 
  NTWT ch2_d2_1_fade, 21 
  NTWT ch2_d2_1_fade, 21 
  NTWT ch2_f2_1_fade, 21 
  NTWT ch2_f2_1_fade, 21 
  NTWT ch2_as2_1_fade, 21 
  NTWT ch2_as2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_f2_1_fade, 21 
  NTWT ch2_f2_1_fade, 21 
  NTWT ch2_a1_1_fade, 21 
  NTWT ch2_a1_1_fade, 21 
  NTWT ch2_gs1_1_hold, 21 
  NTWT ch2_gs1_1_fade, 21   ; TODO: vibrato
  NTWT ch2_gs1_1_fade, 21 
  NTWT ch2_gs1_1_fade, 109 
  NTWT ch2_e1_1_hold, 21 
  NTWT ch2_c1_1_hold, 21 
  NTWT ch2_cs1_1_hold, 21 
  NTWT ch2_e1_1_hold, 21 
  NTWT ch2_gs1_1_hold, 21 
  NTWT ch2_a1_1_hold, 21 
  NTWT ch2_gs1_1_hold, 20 
  NOTE ch2_mute
seq27::
  WAIT 66
  NTWT ch2_a1_1_fade, 21 
  NTWT ch2_a1_1_fade, 21 
  NTWT ch2_d2_1_fade, 21 
  NTWT ch2_d2_1_fade, 21 
  NTWT ch2_f2_1_fade, 21 
  NTWT ch2_f2_1_fade, 21 
  NTWT ch2_as2_1_fade, 21 
  NTWT ch2_as2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_f2_1_fade, 21 
  NTWT ch2_d2_1_fade, 21 
  NTWT ch2_d3_1_fade, 21 
  NTWT ch2_d3_1_fade, 21 
  NTWT ch2_cs3_1_hold, 21 
  NTWT ch2_cs2_1_fade, 21 
  NTWT ch2_cs2_1_fade, 153 
  NTWT ch2_cs1_1_hold, 21 
  NTWT ch2_f1_1_hold, 21 
  NTWT ch2_gs1_1_hold, 21 
  NTWT ch2_a1_1_hold, 21 
  NTWT ch2_gs1_1_hold, 21 
  NTWT ch2_cs2_1_hold, 20 
  NOTE ch2_mute
seq28::
  WAIT 66
  NTWT ch2_as2_1_fade, 21 
  NTWT ch2_as2_1_fade, 21 
  NTWT ch2_g2_1_fade, 21 
  NTWT ch2_g2_1_fade, 21 
  NTWT ch2_d2_1_fade, 21 
  NTWT ch2_d2_1_fade, 21 
  NTWT ch2_as2_1_fade, 21 
  NTWT ch2_as2_1_fade, 21 
  NTWT ch2_g2_1_fade, 21 
  NTWT ch2_g2_1_fade, 21 
  NTWT ch2_cs2_1_hold, 21 
  NTWT ch2_cs2_1_fade, 21   ; TODO: vibrato
  NTWT ch2_d2_1_fade, 21 
  NTWT ch2_d2_1_fade, 21 
  NTWT ch2_g2_1_hold, 21 
  NTWT ch2_g2_1_fade, 21   ; TODO: vibrato
  NTWT ch2_g1_1_fade, 153 
  NTWT ch2_ds1_1_hold, 21 
  NTWT ch2_g1_1_hold, 21 
  NTWT ch2_as1_1_hold, 21 
  NTWT ch2_a1_1_hold, 21 
  NTWT ch2_as1_1_fade, 21   ; TODO: vibrato
  NTWT ch2_a1_1_hold, 20 
  NOTE ch2_mute
seq29::
  WAIT 22
  NTWT ch2_cs2_1_fade, 21 
  NTWT ch2_cs2_1_fade, 21 
  NTWT ch2_as2_1_fade, 21 
  NTWT ch2_as2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_e2_1_fade, 21 
  NTWT ch2_e2_1_fade, 21 
  NTWT ch2_as2_1_fade, 21 
  NTWT ch2_as2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_e2_1_fade, 21   ; TODO: vibrato
  NTWT ch2_e2_1_fade, 21 
  NTWT ch2_f2_1_fade, 21 
  NTWT ch2_f2_1_fade, 21 
  NTWT ch2_gs2_1_hold, 21 
  NTWT ch2_gs2_1_fade, 21   ; TODO: vibrato
  NTWT ch2_gs2_1_fade, 65 
  NTWT ch2_d2_1_fade, 21 
  NTWT ch2_as2_1_fade, 21 
  NTWT ch2_f3_1_hold, 21 
  NTWT ch2_f3_1_fade, 21   ; TODO: vibrato
  NTWT ch2_d3_1_hold, 21 
  NTWT ch2_e3_1_fade, 21   ; TODO: arpreggio
  NTWT ch2_e3_1_hold, 21 
  NTWT ch2_e3_1_fade, 21   ; TODO: vibrato
  NTWT ch2_e3_1_fade, 43 
seq36::
  WAIT 22
  NTWT ch2_a2_1_hold, 21 
  NTWT ch2_f2_1_hold, 21 
  NTWT ch2_cs2_1_fade, 21 
  NTWT ch2_d2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_f2_1_hold, 21 
  NTWT ch2_cs2_1_fade, 21 
  NTWT ch2_d2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21   ; TODO: vibrato
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_as2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_d1_1_fade, 21 
  NTWT ch2_f1_1_fade, 21 
  NTWT ch2_gs2_1_fade, 21   ; TODO: fadein
  NTWT ch2_e2_1_hold, 21 
  NTWT ch2_cs2_1_fade, 21 
  NTWT ch2_e2_1_fade, 21 
  NTWT ch2_gs2_1_fade, 21 
  NTWT ch2_e2_1_hold, 21 
  NTWT ch2_cs2_1_fade, 21 
  NTWT ch2_e2_1_fade, 21 
  NTWT ch2_gs2_1_fade, 21   ; TODO: vibrato
  NTWT ch2_gs2_1_fade, 21 
  NTWT ch2_gs2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_gs2_1_fade, 21 
  NTWT ch2_gs2_1_fade, 21 
  NTWT ch2_cs1_1_fade, 21 
seq40::
  WAIT 22
  NTWT ch2_a2_1_fade, 21   ; TODO: fadein
  NTWT ch2_f2_1_hold, 21 
  NTWT ch2_cs2_1_fade, 21 
  NTWT ch2_d2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_f2_1_hold, 21 
  NTWT ch2_cs2_1_fade, 21 
  NTWT ch2_d2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21   ; TODO: vibrato
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_a2_1_hold, 21 
  NTWT ch2_as2_1_hold, 21 
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_d1_1_fade, 21 
  NTWT ch2_f1_1_fade, 21 
  NTWT ch2_gs2_1_fade, 21   ; TODO: fadein
  NTWT ch2_f2_1_hold, 21 
  NTWT ch2_c2_1_fade, 21 
  NTWT ch2_cs2_1_fade, 21 
  NTWT ch2_gs2_1_fade, 21 
  NTWT ch2_f2_1_hold, 21 
  NTWT ch2_c2_1_fade, 21 
  NTWT ch2_cs2_1_fade, 21 
  NTWT ch2_gs2_1_fade, 21   ; TODO: vibrato
  NTWT ch2_gs2_1_fade, 21 
  NTWT ch2_e2_1_hold, 21 
  NTWT ch2_f2_1_hold, 21 
  NTWT ch2_gs2_1_fade, 21 
  NTWT ch2_gs2_1_fade, 21 
  NTWT ch2_e1_1_fade, 21 
seq46::
  WAIT 22
  NTWT ch2_as2_1_fade, 21   ; TODO: vibrato
  NTWT ch2_g2_1_hold, 21 
  NTWT ch2_d2_1_fade, 21 
  NTWT ch2_g2_1_fade, 21 
  NTWT ch2_as2_1_fade, 21 
  NTWT ch2_g2_1_hold, 21 
  NTWT ch2_d2_1_fade, 21 
  NTWT ch2_g2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21   ; TODO: vibrato
  NTWT ch2_a2_1_fade, 21   ; TODO: vibrato
  NTWT ch2_as2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_g2_1_fade, 21 
  NTWT ch2_g2_1_fade, 21 
  NTWT ch2_d1_1_fade, 21 
  NTWT ch2_g1_1_fade, 21 
  NTWT ch2_as2_1_fade, 21   ; TODO: fadein
  NTWT ch2_g2_1_hold, 21 
  NTWT ch2_ds2_1_fade, 21 
  NTWT ch2_g2_1_fade, 21 
  NTWT ch2_as2_1_fade, 21 
  NTWT ch2_g2_1_hold, 21 
  NTWT ch2_ds2_1_fade, 21 
  NTWT ch2_g2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21   ; TODO: vibrato
  NTWT ch2_a2_1_fade, 21   ; TODO: vibrato
  NTWT ch2_as2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_g2_1_fade, 21 
  NTWT ch2_g2_1_fade, 21 
  NTWT ch2_ds1_1_fade, 21 
seq47::
  WAIT 22
  NTWT ch2_a2_1_fade, 21   ; TODO: fadein
  NTWT ch2_e2_1_hold, 21 
  NTWT ch2_cs2_1_fade, 21 
  NTWT ch2_f2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_e2_1_hold, 21 
  NTWT ch2_cs2_1_fade, 21 
  NTWT ch2_f2_1_fade, 21 
  NTWT ch2_a2_1_fade, 21   ; TODO: vibrato
  NTWT ch2_as2_1_hold, 21 
  NTWT ch2_a2_1_hold, 21 
  NTWT ch2_g2_1_hold, 21 
  NTWT ch2_a2_1_hold, 21 
  NTWT ch2_a2_1_fade, 21 
  NTWT ch2_cs1_1_fade, 21 
  NTWT ch2_e1_1_fade, 21 
  NTWT ch2_gs2_1_fade, 21   ; TODO: fadein
  NTWT ch2_f2_1_hold, 21 
  NTWT ch2_d2_1_fade, 21 
  NTWT ch2_as1_1_fade, 21 
  NTWT ch2_gs2_1_fade, 21 
  NTWT ch2_f2_1_hold, 21 
  NTWT ch2_d2_1_fade, 21 
  NTWT ch2_gs2_1_fade, 21 
  NTWT ch2_gs2_1_hold, 21 
  NTWT ch2_gs2_1_fade, 21 
  NTWT ch2_f2_1_fade, 21 
  NTWT ch2_gs2_1_fade, 21 
  NTWT ch2_g2_1_fade, 21 
  NTWT ch2_f2_1_fade, 21 
  NTWT ch2_e2_1_fade, 21 
  JUMP seq26

CH12_NOTE    ch12_c1  0x14 0x84
CH1_NOTE_ENV ch1_c1_1_hold  ENV_1HOLD ch12_c1
CH2_NOTE_ENV ch2_c1_1_hold  ENV_1HOLD ch12_c1
CH12_NOTE    ch12_cs1 0x4c 0x84
CH2_NOTE_ENV ch2_cs1_1_hold ENV_1HOLD ch12_cs1
CH1_NOTE_ENV ch1_cs1_1_fade ENV_1FADE ch12_cs1
CH2_NOTE_ENV ch2_cs1_1_fade ENV_1FADE ch12_cs1
CH1_NOTE_ENV ch1_cs1_2_hold ENV_2HOLD ch12_cs1
CH1_NOTE_ENV ch1_cs1_2_fade ENV_2FADE ch12_cs1
CH12_NOTE    ch12_d1  0x82 0x84
CH1_NOTE_ENV ch1_d1_1_fade  ENV_1FADE ch12_d1
CH2_NOTE_ENV ch2_d1_1_fade  ENV_1FADE ch12_d1
CH1_NOTE_ENV ch1_d1_2_fade  ENV_2FADE ch12_d1
CH12_NOTE    ch12_ds1 0xb5 0x84
CH1_NOTE_ENV ch1_ds1_1_hold ENV_1HOLD ch12_ds1
CH2_NOTE_ENV ch2_ds1_1_hold ENV_1HOLD ch12_ds1
CH2_NOTE_ENV ch2_ds1_1_fade ENV_1FADE ch12_ds1
CH1_NOTE_ENV ch1_ds1_2_fade ENV_2FADE ch12_ds1
CH12_NOTE    ch12_e1  0xe3 0x84
CH2_NOTE_ENV ch2_e1_1_hold  ENV_1HOLD ch12_e1
CH1_NOTE_ENV ch1_e1_1_fade  ENV_1FADE ch12_e1
CH2_NOTE_ENV ch2_e1_1_fade  ENV_1FADE ch12_e1
CH1_NOTE_ENV ch1_e1_2_fade  ENV_2FADE ch12_e1
CH1_NOTE_ENV ch1_e1_3_hold  ENV_3HOLD ch12_e1
CH12_NOTE    ch12_f1  0x10 0x85
CH2_NOTE_ENV ch2_f1_1_hold  ENV_1HOLD ch12_f1
CH1_NOTE_ENV ch1_f1_1_fade  ENV_1FADE ch12_f1
CH2_NOTE_ENV ch2_f1_1_fade  ENV_1FADE ch12_f1
CH1_NOTE_ENV ch1_f1_3_hold  ENV_3HOLD ch12_f1
CH12_NOTE    ch12_g1  0x63 0x85
CH2_NOTE_ENV ch2_g1_1_hold  ENV_1HOLD ch12_g1
CH1_NOTE_ENV ch1_g1_1_fade  ENV_1FADE ch12_g1
CH2_NOTE_ENV ch2_g1_1_fade  ENV_1FADE ch12_g1
CH1_NOTE_ENV ch1_g1_2_hold  ENV_2HOLD ch12_g1
CH12_NOTE    ch12_gs1 0x89 0x85
CH2_NOTE_ENV ch2_gs1_1_hold ENV_1HOLD ch12_gs1
CH1_NOTE_ENV ch1_gs1_1_fade ENV_1FADE ch12_gs1
CH2_NOTE_ENV ch2_gs1_1_fade ENV_1FADE ch12_gs1
CH1_NOTE_ENV ch1_gs1_2_hold ENV_2HOLD ch12_gs1
CH1_NOTE_ENV ch1_gs1_2_fade ENV_2FADE ch12_gs1
CH1_NOTE_ENV ch1_gs1_3_hold ENV_3HOLD ch12_gs1
CH1_NOTE_ENV ch1_gs1_4_hold ENV_4HOLD ch12_gs1
CH12_NOTE    ch12_a1  0xab 0x85
CH2_NOTE_ENV ch2_a1_1_hold  ENV_1HOLD ch12_a1
CH1_NOTE_ENV ch1_a1_1_fade  ENV_1FADE ch12_a1
CH2_NOTE_ENV ch2_a1_1_fade  ENV_1FADE ch12_a1
CH1_NOTE_ENV ch1_a1_2_hold  ENV_2HOLD ch12_a1
CH1_NOTE_ENV ch1_a1_2_fade  ENV_2FADE ch12_a1
CH1_NOTE_ENV ch1_a1_3_hold  ENV_3HOLD ch12_a1
CH1_NOTE_ENV ch1_a1_5_hold  ENV_5HOLD ch12_a1
CH12_NOTE    ch12_as1 0xcd 0x85
CH2_NOTE_ENV ch2_as1_1_hold ENV_1HOLD ch12_as1
CH1_NOTE_ENV ch1_as1_1_fade ENV_1FADE ch12_as1
CH2_NOTE_ENV ch2_as1_1_fade ENV_1FADE ch12_as1
CH1_NOTE_ENV ch1_as1_3_hold ENV_3HOLD ch12_as1
CH1_NOTE_ENV ch1_as1_3_fade ENV_3FADE ch12_as1
CH12_NOTE    ch12_c2  0x0b 0x86
CH1_NOTE_ENV ch1_c2_1_fade  ENV_1FADE ch12_c2
CH2_NOTE_ENV ch2_c2_1_fade  ENV_1FADE ch12_c2
CH1_NOTE_ENV ch1_c2_2_fade  ENV_2FADE ch12_c2
CH12_NOTE    ch12_cs2 0x27 0x86
CH2_NOTE_ENV ch2_cs2_1_hold ENV_1HOLD ch12_cs2
CH1_NOTE_ENV ch1_cs2_1_fade ENV_1FADE ch12_cs2
CH2_NOTE_ENV ch2_cs2_1_fade ENV_1FADE ch12_cs2
CH1_NOTE_ENV ch1_cs2_2_hold ENV_2HOLD ch12_cs2
CH1_NOTE_ENV ch1_cs2_2_fade ENV_2FADE ch12_cs2
CH1_NOTE_ENV ch1_cs2_3_hold ENV_3HOLD ch12_cs2
CH12_NOTE    ch12_d2  0x41 0x86
CH1_NOTE_ENV ch1_d2_1_fade  ENV_1FADE ch12_d2
CH2_NOTE_ENV ch2_d2_1_fade  ENV_1FADE ch12_d2
CH1_NOTE_ENV ch1_d2_2_fade  ENV_2FADE ch12_d2
CH1_NOTE_ENV ch1_d2_3_fade  ENV_3FADE ch12_d2
CH12_NOTE    ch12_ds2 0x5a 0x86
CH1_NOTE_ENV ch1_ds2_1_fade ENV_1FADE ch12_ds2
CH2_NOTE_ENV ch2_ds2_1_fade ENV_1FADE ch12_ds2
CH1_NOTE_ENV ch1_ds2_2_fade ENV_2FADE ch12_ds2
CH12_NOTE    ch12_e2  0x72 0x86
CH1_NOTE_ENV ch1_e2_1_hold  ENV_1HOLD ch12_e2
CH2_NOTE_ENV ch2_e2_1_hold  ENV_1HOLD ch12_e2
CH1_NOTE_ENV ch1_e2_1_fade  ENV_1FADE ch12_e2
CH2_NOTE_ENV ch2_e2_1_fade  ENV_1FADE ch12_e2
CH1_NOTE_ENV ch1_e2_2_hold  ENV_2HOLD ch12_e2
CH1_NOTE_ENV ch1_e2_2_fade  ENV_2FADE ch12_e2
CH1_NOTE_ENV ch1_e2_3_fade  ENV_3FADE ch12_e2
CH12_NOTE    ch12_f2  0x88 0x86
CH1_NOTE_ENV ch1_f2_1_hold  ENV_1HOLD ch12_f2
CH2_NOTE_ENV ch2_f2_1_hold  ENV_1HOLD ch12_f2
CH1_NOTE_ENV ch1_f2_1_fade  ENV_1FADE ch12_f2
CH2_NOTE_ENV ch2_f2_1_fade  ENV_1FADE ch12_f2
CH1_NOTE_ENV ch1_f2_2_hold  ENV_2HOLD ch12_f2
CH1_NOTE_ENV ch1_f2_2_fade  ENV_2FADE ch12_f2
CH12_NOTE    ch12_g2  0xb2 0x86
CH1_NOTE_ENV ch1_g2_1_hold  ENV_1HOLD ch12_g2
CH2_NOTE_ENV ch2_g2_1_hold  ENV_1HOLD ch12_g2
CH1_NOTE_ENV ch1_g2_1_fade  ENV_1FADE ch12_g2
CH2_NOTE_ENV ch2_g2_1_fade  ENV_1FADE ch12_g2
CH1_NOTE_ENV ch1_g2_2_hold  ENV_2HOLD ch12_g2
CH1_NOTE_ENV ch1_g2_2_fade  ENV_2FADE ch12_g2
CH12_NOTE    ch12_gs2 0xc4 0x86
CH2_NOTE_ENV ch2_gs2_1_hold ENV_1HOLD ch12_gs2
CH1_NOTE_ENV ch1_gs2_1_fade ENV_1FADE ch12_gs2
CH2_NOTE_ENV ch2_gs2_1_fade ENV_1FADE ch12_gs2
CH1_NOTE_ENV ch1_gs2_2_hold ENV_2HOLD ch12_gs2
CH1_NOTE_ENV ch1_gs2_2_fade ENV_2FADE ch12_gs2
CH1_NOTE_ENV ch1_gs2_3_fade ENV_3FADE ch12_gs2
CH12_NOTE    ch12_a2  0xd6 0x86
CH1_NOTE_ENV ch1_a2_1_hold  ENV_1HOLD ch12_a2
CH2_NOTE_ENV ch2_a2_1_hold  ENV_1HOLD ch12_a2
CH1_NOTE_ENV ch1_a2_1_fade  ENV_1FADE ch12_a2
CH2_NOTE_ENV ch2_a2_1_fade  ENV_1FADE ch12_a2
CH1_NOTE_ENV ch1_a2_2_hold  ENV_2HOLD ch12_a2
CH1_NOTE_ENV ch1_a2_2_fade  ENV_2FADE ch12_a2
CH1_NOTE_ENV ch1_a2_3_fade  ENV_3FADE ch12_a2
CH12_NOTE    ch12_as2 0xe7 0x86
CH1_NOTE_ENV ch1_as2_1_hold ENV_1HOLD ch12_as2
CH2_NOTE_ENV ch2_as2_1_hold ENV_1HOLD ch12_as2
CH1_NOTE_ENV ch1_as2_1_fade ENV_1FADE ch12_as2
CH2_NOTE_ENV ch2_as2_1_fade ENV_1FADE ch12_as2
CH1_NOTE_ENV ch1_as2_2_hold ENV_2HOLD ch12_as2
CH1_NOTE_ENV ch1_as2_2_fade ENV_2FADE ch12_as2
CH12_NOTE    ch12_cs3 0x13 0x87
CH1_NOTE_ENV ch1_cs3_1_hold ENV_1HOLD ch12_cs3
CH2_NOTE_ENV ch2_cs3_1_hold ENV_1HOLD ch12_cs3
CH1_NOTE_ENV ch1_cs3_1_fade ENV_1FADE ch12_cs3
CH1_NOTE_ENV ch1_cs3_2_hold ENV_2HOLD ch12_cs3
CH12_NOTE    ch12_d3  0x21 0x87
CH2_NOTE_ENV ch2_d3_1_hold  ENV_1HOLD ch12_d3
CH1_NOTE_ENV ch1_d3_1_fade  ENV_1FADE ch12_d3
CH2_NOTE_ENV ch2_d3_1_fade  ENV_1FADE ch12_d3
CH12_NOTE    ch12_e3  0x39 0x87
CH1_NOTE_ENV ch1_e3_1_hold  ENV_1HOLD ch12_e3
CH2_NOTE_ENV ch2_e3_1_hold  ENV_1HOLD ch12_e3
CH2_NOTE_ENV ch2_e3_1_fade  ENV_1FADE ch12_e3
CH12_NOTE    ch12_f3  0x44 0x87
CH2_NOTE_ENV ch2_f3_1_hold  ENV_1HOLD ch12_f3
CH2_NOTE_ENV ch2_f3_1_fade  ENV_1FADE ch12_f3
