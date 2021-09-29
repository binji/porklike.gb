CMD_NOTE = 0
CMD_WAIT = 1
CMD_JUMP = 2
CMD_CALL = 3
CMD_RETN = 4

; manually assign song_ptr and wait_timer to HRAM
; TODO: better way?
ch1_wait_timer = 0xffa0
ch1_song_ptr   = ch1_wait_timer + 1
ch1_stack      = ch1_song_ptr   + 2
ch2_wait_timer = ch1_stack      + 2
ch2_song_ptr   = ch2_wait_timer + 1
ch2_stack      = ch2_song_ptr   + 2
ch3_wait_timer = ch2_stack      + 2
ch3_song_ptr   = ch3_wait_timer + 1
ch3_stack      = ch3_song_ptr   + 2
ch4_wait_timer = ch3_stack      + 2
ch4_song_ptr   = ch4_wait_timer + 1
ch4_stack      = ch4_song_ptr   + 2
snddata_end    = ch4_stack      + 2

.area _CODE

snd_data_init::
  .db 0      ; ch1
  .dw ch1
  .dw 0
  .db 0      ; ch2
  .dw ch2
  .dw 0
  .db 0      ; ch3
  .dw ch3
  .dw 0
  .db 0      ; ch4
  .dw ch4
  .dw 0


_snd_init::
  ld hl, #snddata_end - #ch1_wait_timer
  push hl
  ld hl, #snd_data_init
  push hl
  ld hl, #ch1_wait_timer
  push hl
  call _memcpy
  add sp, #6

  ; TODO: set instrument in song, not here
  call square_90
  ret

_snd_update::
  ; switch to bank 2
  ld a, #2
  ld (#0x2000), a

  ld c, #<ch1_wait_timer
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
  ld h, a       ; hl = song_ptr

read_command:
  ld a, (hl+)   ; a = *song_ptr, c = &song_ptr hi
  cp a, #1
  jr z, wait
  jr c, note
  cp a, #3
  jr z, call
  jr c, jump

retn:
  ; read song ptr from "stack"
  inc c
  ldh a, (c)
  ld l, a
  inc c
  ldh a, (c)
  ld h, a
  ; skip over "CALL" instruction
  inc hl
  inc hl
  ; move c back to &song_ptr hi
  dec c
  dec c
  jr read_command

call:
  ; save current song ptr to "stack"
  inc c
  ld a, l
  ldh (c), a
  inc c
  ld a, h
  ldh (c), a
  ; move c back to &song_ptr hi
  dec c
  dec c
  ; fallthrough to jump

jump:
  ; read new song_ptr into hl
  ld a, (hl+)
  ld e, a
  ld a, (hl+)
  ld h, a
  ld l, e
  jr read_command

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
  jr read_command

wait:
  ; go back and write wait_timer
  dec c
  dec c
  ld a, (hl+)
  ldh (c), a
  ; go forward and write song_ptr
  inc c
  ld a, l
  ldh (c), a
  inc c
  ld a, h
  ldh (c), a

nextchannel:
  inc c
  inc c
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

.macro FADE100 delay    ; 20 + N
  NTWT ch3_vol100, 10
  NTWT ch3_vol50, 6
  NTWT ch3_vol25, 4
  NTWT ch3_mute, delay  ; N
.endm

.macro FADE50 delay     ; 14 + N
  NTWT ch3_vol50, 9
  NTWT ch3_vol25, 5
  NTWT ch3_mute, delay  ; N
.endm

.macro FADE25 delay     ; 7 + N
  NTWT ch3_vol25, 7     ; 7
  NTWT ch3_mute, delay  ; N
.endm

.macro WAIT delay
  .db CMD_WAIT
  .db delay
.endm

.macro JUMP addr
  .db CMD_JUMP
  .dw addr
.endm

.macro CALL_ addr
  .db CMD_CALL
  .dw addr
.endm

.macro RETN
  .db CMD_RETN
.endm

ch1::
  CALL_ seq37
  CALL_ seq41
  CALL_ seq48
  CALL_ seq49
  CALL_ seq35
  CALL_ seq39
  CALL_ seq44
  CALL_ seq45
  CALL_ seq37
  CALL_ seq41
  CALL_ seq48
  CALL_ seq49
  CALL_ seq00
  CALL_ seq00
  CALL_ seq00
  CALL_ seq00
  JUMP ch1

ch2::
  CALL_ seq26
  CALL_ seq27
  CALL_ seq28
  CALL_ seq29
  CALL_ seq36
  CALL_ seq40
  CALL_ seq46
  CALL_ seq47
  CALL_ seq30
  CALL_ seq31
  CALL_ seq32
  CALL_ seq33
  CALL_ seq54
  CALL_ seq55
  CALL_ seq56
  CALL_ seq57
  JUMP ch2

ch3::
  CALL_ seq34
  CALL_ seq38
  CALL_ seq42
  CALL_ seq43
  CALL_ seq50
  CALL_ seq51
  CALL_ seq52
  CALL_ seq53
  CALL_ seq34
  CALL_ seq38
  CALL_ seq42
  CALL_ seq43
  CALL_ seq34
  CALL_ seq38
  CALL_ seq42
  CALL_ seq53
  JUMP ch3

ch4::
  CALL_ seq00
  JUMP ch4


seq00::
  WAIT 255
  WAIT 255
  WAIT 194
  RETN

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
ENV_4FADE = 0b10000011
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

;; DATA ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

seq34::
  NOTE ch3_d0
  FADE100, 24
  FADE25, 37
  FADE50, 74
  FADE50, 74
  FADE100, 46
  FADE25, 15  ; TODO: fadein
  NOTE ch3_cs0
  FADE100, 24
  FADE25, 37
  FADE50, 74
  FADE50, 74
  FADE50, 74
  RETN
seq38::
  NOTE ch3_d0
  FADE100, 24
  FADE25, 37
  FADE50, 74
  FADE50, 74
  FADE100, 46
  FADE25, 15  ; TODO: fadein
  NOTE ch3_f0
  FADE100, 24
  FADE25, 37
  FADE50, 74
  FADE50, 74
  FADE50, 74
  RETN
seq42::
  NOTE ch3_g0
  FADE100, 24
  FADE25, 37
  FADE50, 74
  FADE50, 74
  FADE100, 46
  FADE25, 15  ; TODO: fadein
  NOTE ch3_ds0
  FADE100, 24
  FADE25, 37
  FADE50, 74
  FADE50, 74
  FADE100, 68
  RETN
seq43::
  NOTE ch3_a0
  FADE100, 24
  FADE25, 37
  FADE50, 74
  FADE50, 74
  FADE100, 46
  FADE25, 15  ; TODO: fadein
  NOTE ch3_as0
  FADE100, 24
  FADE25, 37
  FADE50, 74
  FADE50, 74
  NOTE ch3_e0
  FADE50, 30
  NOTE ch3_a0
  FADE50, 30
  RETN
seq50::
  NOTE ch3_d0
  FADE100, 2
  NOTE ch3_a0
  FADE50, 8
  NOTE ch3_f1
  FADE25, 15
  NOTE ch3_d0
  FADE50, 8
  FADE50, 8
  NOTE ch3_a0
  FADE50, 8
  NOTE ch3_f1
  FADE25, 37
  NOTE ch3_d0
  FADE50, 74
  FADE50, 8
  NOTE ch3_f1
  FADE50, 8
  FADE25, 15
  NOTE ch3_d0
  FADE25, 15  ; TODO: fadein
  NOTE ch3_cs0
  FADE100, 2
  NOTE ch3_gs0
  FADE50, 8
  NOTE ch3_e1
  FADE25, 15
  NOTE ch3_cs0
  FADE50, 8
  FADE50, 8
  NOTE ch3_gs0
  FADE50, 8
  NOTE ch3_e1
  FADE25, 37
  NOTE ch3_cs0
  FADE50, 74
  FADE50, 8
  NOTE ch3_e1
  FADE50, 8
  FADE25, 37
  RETN
seq51::
  NOTE ch3_d0
  FADE100, 2
  NOTE ch3_a0
  FADE50, 8
  NOTE ch3_f1
  FADE25, 15
  NOTE ch3_d0
  FADE50, 8
  FADE50, 8
  NOTE ch3_a0
  FADE50, 8
  NOTE ch3_f1
  FADE25, 37
  NOTE ch3_d0
  FADE50, 74
  FADE100, 2
  NOTE ch3_f1
  FADE50, 8
  FADE25, 15
  NOTE ch3_d0
  FADE25, 15  ; TODO: fadein
  NOTE ch3_f0
  FADE100, 2
  NOTE ch3_c1
  FADE50, 8
  NOTE ch3_gs1
  FADE25, 15
  NOTE ch3_f0
  FADE50, 8
  FADE50, 8
  NOTE ch3_c1
  FADE50, 8
  NOTE ch3_gs1
  FADE25, 37
  NOTE ch3_f0
  FADE50, 74
  FADE50, 8
  NOTE ch3_gs1
  FADE50, 8
  FADE25, 37
  RETN
seq52::
  NOTE ch3_g0
  FADE100, 2
  NOTE ch3_d1
  FADE50, 8
  NOTE ch3_as1
  FADE25, 15
  NOTE ch3_g0
  FADE50, 8
  FADE50, 8
  NOTE ch3_d1
  FADE50, 8
  NOTE ch3_as1
  FADE25, 37
  NOTE ch3_g0
  FADE50, 74
  FADE100, 2
  NOTE ch3_as1
  FADE50, 8
  FADE25, 15
  NOTE ch3_g0
  FADE25, 15  ; TODO: fadein
  NOTE ch3_ds0
  FADE100, 2
  NOTE ch3_as1
  FADE50, 8
  NOTE ch3_g1
  FADE25, 15
  NOTE ch3_ds0
  FADE50, 8
  FADE50, 8
  NOTE ch3_as1
  FADE50, 8
  NOTE ch3_g1
  FADE25, 37
  NOTE ch3_ds0
  FADE50, 74
  FADE50, 8
  NOTE ch3_as1
  FADE50, 8
  FADE25, 37
  RETN
seq53::
  NOTE ch3_a0
  FADE100, 2
  NOTE ch3_e1
  FADE50, 8
  NOTE ch3_f1
  FADE25, 15
  NOTE ch3_a0
  FADE50, 8
  FADE50, 8
  NOTE ch3_e1
  FADE50, 8
  NOTE ch3_f1
  FADE25, 37
  NOTE ch3_a0
  FADE50, 74
  FADE100, 2
  NOTE ch3_e1
  FADE50, 8
  NOTE ch3_cs1
  FADE25, 15
  NOTE ch3_a0
  FADE25, 15  ; TODO: fadein
  NOTE ch3_as0
  FADE100, 2
  NOTE ch3_e1
  FADE50, 8
  NOTE ch3_f1
  FADE25, 15
  NOTE ch3_as0
  FADE50, 8
  FADE50, 8
  NOTE ch3_e1
  FADE50, 8
  NOTE ch3_f1
  FADE25, 37
  NOTE ch3_as0
  FADE50, 74
  NOTE ch3_a0
  FADE50, 8
  NOTE ch3_a1
  FADE50, 8
  NOTE ch3_cs1
  FADE25, 15
  NOTE ch3_a0
  WAIT 22
  RETN

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

seq37::
  WAIT 44
  NTWT ch1_a1_1_fade, 22 
  NTWT ch1_a1_1_fade, 22 
  NTWT ch1_d2_2_fade, 22 
  NTWT ch1_d2_1_fade, 22 
  NTWT ch1_f2_1_fade, 22 
  NTWT ch1_f2_1_fade, 22 
  NTWT ch1_as2_2_fade, 22 
  NTWT ch1_as2_1_fade, 22 
  NTWT ch1_a2_2_fade, 22 
  NTWT ch1_a2_1_fade, 22 
  NTWT ch1_f2_2_fade, 22 
  NTWT ch1_f2_1_fade, 22 
  NTWT ch1_a1_2_fade, 22 
  NTWT ch1_a1_1_fade, 22 
  NTWT ch1_gs1_2_hold, 22 
  NTWT ch1_gs1_2_fade, 22   ; TODO: vibrato
  NTWT ch1_gs1_2_fade, 22 
  NTWT ch1_gs1_1_fade, 110 
  NTWT ch1_e1_1_fade, 22   ; TODO: fadein
  NTWT ch1_c1_1_hold, 22 
  NTWT ch1_cs1_2_hold, 22 
  NTWT ch1_e1_3_hold, 22 
  NTWT ch1_gs1_3_hold, 22 
  NTWT ch1_a1_2_hold, 22 
  NTWT ch1_gs1_1_fade, 22   ; TODO: vibrato
  NTWT ch1_e1_1_fade, 22 
  RETN
seq41::
  WAIT 44
  NTWT ch1_a1_1_fade, 22 
  NTWT ch1_a1_1_fade, 22 
  NTWT ch1_d2_2_fade, 22 
  NTWT ch1_d2_1_fade, 22 
  NTWT ch1_f2_2_fade, 22 
  NTWT ch1_f2_1_fade, 22 
  NTWT ch1_as2_2_fade, 22 
  NTWT ch1_as2_1_fade, 22 
  NTWT ch1_a2_1_fade, 22 
  NTWT ch1_a2_1_fade, 22 
  NTWT ch1_f2_2_fade, 22 
  NTWT ch1_f2_1_fade, 22 
  NTWT ch1_d3_1_fade, 22 
  NTWT ch1_d3_1_fade, 22 
  NTWT ch1_cs3_2_hold, 22 
  NTWT ch1_cs3_1_fade, 22   ; TODO: vibrato
  NTWT ch1_cs3_1_fade, 22 
  NTWT ch1_cs3_1_fade, 132 
  NTWT ch1_cs1_2_hold, 22 
  NTWT ch1_f1_3_hold, 22 
  NTWT ch1_gs1_4_hold, 22 
  NTWT ch1_a1_5_hold, 22 
  NTWT ch1_gs1_4_hold, 22 
  NTWT ch1_cs2_3_hold, 22 
  NTWT ch1_f2_1_hold, 22 
  NOTE ch1_mute
  RETN
seq48::
  WAIT 44
  NTWT ch1_as2_1_fade, 22 
  NTWT ch1_as2_1_fade, 22 
  NTWT ch1_g2_2_fade, 22 
  NTWT ch1_g2_1_fade, 22 
  NTWT ch1_d2_1_fade, 22 
  NTWT ch1_d2_1_fade, 22 
  NTWT ch1_as2_2_fade, 22 
  NTWT ch1_as2_1_fade, 22 
  NTWT ch1_g2_1_fade, 22 
  NTWT ch1_g2_1_fade, 22 
  NTWT ch1_cs2_2_hold, 22 
  NTWT ch1_cs2_2_fade, 22   ; TODO: vibrato
  NTWT ch1_d2_2_fade, 22 
  NTWT ch1_d2_1_fade, 22 
  NTWT ch1_g2_2_hold, 22 
  NTWT ch1_g2_2_fade, 22   ; TODO: vibrato
  NTWT ch1_g2_1_fade, 22 
  NTWT ch1_g2_1_fade, 132 
  NTWT ch1_ds1_1_hold, 22 
  NTWT ch1_g1_2_hold, 22 
  NTWT ch1_as1_3_hold, 22 
  NTWT ch1_a1_3_hold, 22 
  NTWT ch1_as1_3_fade, 22   ; TODO: vibrato
  NTWT ch1_a1_2_hold, 22 
  NTWT ch1_g1_1_fade, 22 
  RETN
seq49::
  NTWT ch1_cs2_2_fade, 22 
  NTWT ch1_cs2_1_fade, 22 
  NTWT ch1_as2_2_fade, 22 
  NTWT ch1_as2_1_fade, 22 
  NTWT ch1_a2_1_fade, 22 
  NTWT ch1_a2_1_fade, 22 
  NTWT ch1_e2_2_fade, 22 
  NTWT ch1_e2_1_fade, 22 
  NTWT ch1_as2_2_fade, 22 
  NTWT ch1_as2_1_fade, 22 
  NTWT ch1_a2_2_fade, 22 
  NTWT ch1_a2_1_fade, 22 
  NTWT ch1_e2_2_fade, 22   ; TODO: vibrato
  NTWT ch1_e2_1_fade, 22 
  NTWT ch1_f2_2_fade, 22 
  NTWT ch1_f2_1_fade, 22 
  NTWT ch1_gs2_2_hold, 22 
  NTWT ch1_gs2_2_fade, 22   ; TODO: vibrato
  NTWT ch1_gs2_1_fade, 22 
  NTWT ch1_gs2_1_fade, 44 
  NTWT ch1_d2_1_fade, 22 
  NTWT ch1_f2_1_fade, 22 
  NTWT ch1_as2_1_fade, 22 
  NTWT ch1_d3_1_fade, 22   ; TODO: vibrato
  NTWT ch1_d3_1_fade, 22   ; TODO: vibrato
  NTWT ch1_e3_1_hold, 22 
  NTWT ch1_d3_1_fade, 22   ; TODO: arpreggio
  NTWT ch1_cs3_1_hold, 22 
  NTWT ch1_cs3_1_fade, 22   ; TODO: vibrato
  NTWT ch1_cs3_1_hold, 22 
  NTWT ch1_cs3_1_fade, 22 
  RETN
seq35::
  NTWT ch1_a2_1_hold, 22 
  NTWT ch1_f2_1_hold, 22 
  NTWT ch1_cs2_2_fade, 22 
  NTWT ch1_d2_3_fade, 22 
  NTWT ch1_a2_3_fade, 22 
  NTWT ch1_f2_2_hold, 22 
  NTWT ch1_cs2_1_fade, 22 
  NTWT ch1_d2_1_fade, 22 
  NTWT ch1_a2_1_fade, 22   ; TODO: vibrato
  NTWT ch1_a2_1_fade, 22 
  NTWT ch1_a2_2_fade, 22 
  NTWT ch1_as2_2_fade, 22 
  NTWT ch1_a2_2_fade, 22 
  NTWT ch1_a2_1_fade, 22 
  NTWT ch1_d1_1_fade, 22 
  NTWT ch1_f1_1_fade, 22 
  NTWT ch1_gs2_1_fade, 22   ; TODO: fadein
  NTWT ch1_e2_1_hold, 22 
  NTWT ch1_cs2_2_fade, 22 
  NTWT ch1_e2_3_fade, 22 
  NTWT ch1_gs2_3_fade, 22 
  NTWT ch1_e2_2_hold, 22 
  NTWT ch1_cs2_1_fade, 22 
  NTWT ch1_e2_1_fade, 22 
  NTWT ch1_gs2_1_fade, 22   ; TODO: vibrato
  NTWT ch1_gs2_1_fade, 22 
  NTWT ch1_gs2_2_fade, 22 
  NTWT ch1_a2_2_fade, 22 
  NTWT ch1_gs2_2_fade, 22 
  NTWT ch1_gs2_1_fade, 22 
  NTWT ch1_cs1_1_fade, 22 
  NTWT ch1_e1_1_fade, 22 
  RETN
seq39::
  NTWT ch1_a2_1_fade, 22   ; TODO: fadein
  NTWT ch1_f2_1_hold, 22 
  NTWT ch1_cs2_2_fade, 22 
  NTWT ch1_d2_2_fade, 22 
  NTWT ch1_a2_2_fade, 22 
  NTWT ch1_f2_2_hold, 22 
  NTWT ch1_cs2_1_fade, 22 
  NTWT ch1_d2_1_fade, 22 
  NTWT ch1_a2_1_fade, 22   ; TODO: vibrato
  NTWT ch1_a2_1_fade, 22 
  NTWT ch1_a2_2_hold, 22 
  NTWT ch1_as2_2_hold, 22 
  NTWT ch1_a2_2_fade, 22 
  NTWT ch1_a2_1_fade, 22 
  NTWT ch1_d1_2_fade, 22 
  NTWT ch1_f1_1_fade, 22 
  NTWT ch1_gs2_1_fade, 22   ; TODO: fadein
  NTWT ch1_f2_1_hold, 22 
  NTWT ch1_c2_2_fade, 22 
  NTWT ch1_cs2_2_fade, 22 
  NTWT ch1_gs2_2_fade, 22 
  NTWT ch1_f2_2_hold, 22 
  NTWT ch1_c2_1_fade, 22 
  NTWT ch1_cs2_1_fade, 22 
  NTWT ch1_gs2_1_fade, 22   ; TODO: vibrato
  NTWT ch1_gs2_1_fade, 22 
  NTWT ch1_e2_2_hold, 22 
  NTWT ch1_f2_2_hold, 22 
  NTWT ch1_gs2_2_fade, 22 
  NTWT ch1_gs2_1_fade, 22 
  NTWT ch1_e1_2_fade, 22 
  NTWT ch1_f1_1_fade, 22 
  RETN
seq44::
  NTWT ch1_as2_1_fade, 22   ; TODO: vibrato
  NTWT ch1_g2_2_hold, 22 
  NTWT ch1_d2_2_fade, 22 
  NTWT ch1_g2_1_fade, 22 
  NTWT ch1_as2_2_fade, 22 
  NTWT ch1_g2_2_hold, 22 
  NTWT ch1_d2_1_fade, 22 
  NTWT ch1_g2_1_fade, 22 
  NTWT ch1_a2_1_fade, 22   ; TODO: vibrato
  NTWT ch1_a2_1_fade, 22   ; TODO: vibrato
  NTWT ch1_as2_2_fade, 22 
  NTWT ch1_a2_2_fade, 22 
  NTWT ch1_g2_2_fade, 22 
  NTWT ch1_g2_1_fade, 22 
  NTWT ch1_d1_2_fade, 22 
  NTWT ch1_g1_1_fade, 22 
  NTWT ch1_as2_1_fade, 22   ; TODO: fadein
  NTWT ch1_g2_1_hold, 22 
  NTWT ch1_ds2_2_fade, 22 
  NTWT ch1_g2_2_fade, 22 
  NTWT ch1_as2_2_fade, 22 
  NTWT ch1_g2_2_hold, 22 
  NTWT ch1_ds2_1_fade, 22 
  NTWT ch1_g2_1_fade, 22 
  NTWT ch1_a2_1_fade, 22   ; TODO: vibrato
  NTWT ch1_a2_1_fade, 22   ; TODO: vibrato
  NTWT ch1_as2_2_fade, 22 
  NTWT ch1_a2_2_fade, 22 
  NTWT ch1_g2_2_fade, 22 
  NTWT ch1_g2_1_fade, 22 
  NTWT ch1_ds1_2_fade, 22 
  NTWT ch1_g1_1_fade, 22 
  RETN
seq45::
  NTWT ch1_a2_1_fade, 22   ; TODO: fadein
  NTWT ch1_e2_1_hold, 22 
  NTWT ch1_cs2_2_fade, 22 
  NTWT ch1_f2_1_fade, 22 
  NTWT ch1_a2_2_fade, 22 
  NTWT ch1_e2_2_hold, 22 
  NTWT ch1_cs2_1_fade, 22 
  NTWT ch1_f2_1_fade, 22 
  NTWT ch1_a2_1_fade, 22   ; TODO: vibrato
  NTWT ch1_as2_1_hold, 22 
  NTWT ch1_a2_2_hold, 22 
  NTWT ch1_g2_1_hold, 22 
  NTWT ch1_a2_1_fade, 22   ; TODO: vibrato
  NTWT ch1_a2_1_fade, 22 
  NTWT ch1_cs1_2_fade, 22 
  NTWT ch1_e1_1_fade, 22 
  NTWT ch1_gs2_1_fade, 22   ; TODO: fadein
  NTWT ch1_f2_1_hold, 22 
  NTWT ch1_d2_2_fade, 22 
  NTWT ch1_as1_1_fade, 22 
  NTWT ch1_gs2_1_fade, 22 
  NTWT ch1_f2_2_hold, 22 
  NTWT ch1_d2_1_fade, 22 
  NTWT ch1_as1_1_fade, 22 
  NTWT ch1_gs2_2_fade, 22   ; TODO: vibrato
  NTWT ch1_gs2_1_fade, 22 
  NTWT ch1_f2_1_fade, 22 
  NTWT ch1_gs2_2_fade, 22 
  NTWT ch1_g2_2_fade, 22 
  NTWT ch1_f2_1_fade, 22 
  NTWT ch1_e2_2_fade, 22 
  NTWT ch1_cs2_1_fade, 22 
  RETN

seq26::
  WAIT 66
  NTWT ch2_a1_1_fade, 22 
  NTWT ch2_a1_1_fade, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_f2_1_fade, 22 
  NTWT ch2_f2_1_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_f2_1_fade, 22 
  NTWT ch2_f2_1_fade, 22 
  NTWT ch2_a1_1_fade, 22 
  NTWT ch2_a1_1_fade, 22 
  NTWT ch2_gs1_1_hold, 22 
  NTWT ch2_gs1_1_fade, 22   ; TODO: vibrato
  NTWT ch2_gs1_1_fade, 22 
  NTWT ch2_gs1_1_fade, 110 
  NTWT ch2_e1_1_hold, 22 
  NTWT ch2_c1_1_hold, 22 
  NTWT ch2_cs1_1_hold, 22 
  NTWT ch2_e1_1_hold, 22 
  NTWT ch2_gs1_1_hold, 22 
  NTWT ch2_a1_1_hold, 22 
  NTWT ch2_gs1_1_hold, 22 
  NOTE ch2_mute
  RETN
seq27::
  WAIT 66
  NTWT ch2_a1_1_fade, 22 
  NTWT ch2_a1_1_fade, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_f2_1_fade, 22 
  NTWT ch2_f2_1_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_f2_1_fade, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_d3_1_fade, 22 
  NTWT ch2_d3_1_fade, 22 
  NTWT ch2_cs3_1_hold, 22 
  NTWT ch2_cs2_1_fade, 22 
  NTWT ch2_cs2_1_fade, 154 
  NTWT ch2_cs1_1_hold, 22 
  NTWT ch2_f1_1_hold, 22 
  NTWT ch2_gs1_1_hold, 22 
  NTWT ch2_a1_1_hold, 22 
  NTWT ch2_gs1_1_hold, 22 
  NTWT ch2_cs2_1_hold, 22 
  NOTE ch2_mute
  RETN
seq28::
  WAIT 66
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_g2_1_fade, 22 
  NTWT ch2_g2_1_fade, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_g2_1_fade, 22 
  NTWT ch2_g2_1_fade, 22 
  NTWT ch2_cs2_1_hold, 22 
  NTWT ch2_cs2_1_fade, 22   ; TODO: vibrato
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_g2_1_hold, 22 
  NTWT ch2_g2_1_fade, 22   ; TODO: vibrato
  NTWT ch2_g1_1_fade, 154 
  NTWT ch2_ds1_1_hold, 22 
  NTWT ch2_g1_1_hold, 22 
  NTWT ch2_as1_1_hold, 22 
  NTWT ch2_a1_1_hold, 22 
  NTWT ch2_as1_1_fade, 22   ; TODO: vibrato
  NTWT ch2_a1_1_hold, 22 
  NOTE ch2_mute
  RETN
seq29::
  WAIT 22
  NTWT ch2_cs2_1_fade, 22 
  NTWT ch2_cs2_1_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_e2_1_fade, 22 
  NTWT ch2_e2_1_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_e2_1_fade, 22   ; TODO: vibrato
  NTWT ch2_e2_1_fade, 22 
  NTWT ch2_f2_1_fade, 22 
  NTWT ch2_f2_1_fade, 22 
  NTWT ch2_gs2_1_hold, 22 
  NTWT ch2_gs2_1_fade, 22   ; TODO: vibrato
  NTWT ch2_gs2_1_fade, 66 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_f3_1_hold, 22 
  NTWT ch2_f3_1_fade, 22   ; TODO: vibrato
  NTWT ch2_d3_1_hold, 22 
  NTWT ch2_e3_1_fade, 22   ; TODO: arpreggio
  NTWT ch2_e3_1_hold, 22 
  NTWT ch2_e3_1_fade, 22   ; TODO: vibrato
  NTWT ch2_e3_1_fade, 44 
  RETN
seq36::
  WAIT 22
  NTWT ch2_a2_1_hold, 22 
  NTWT ch2_f2_1_hold, 22 
  NTWT ch2_cs2_1_fade, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_f2_1_hold, 22 
  NTWT ch2_cs2_1_fade, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22   ; TODO: vibrato
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_d1_1_fade, 22 
  NTWT ch2_f1_1_fade, 22 
  NTWT ch2_gs2_1_fade, 22   ; TODO: fadein
  NTWT ch2_e2_1_hold, 22 
  NTWT ch2_cs2_1_fade, 22 
  NTWT ch2_e2_1_fade, 22 
  NTWT ch2_gs2_1_fade, 22 
  NTWT ch2_e2_1_hold, 22 
  NTWT ch2_cs2_1_fade, 22 
  NTWT ch2_e2_1_fade, 22 
  NTWT ch2_gs2_1_fade, 22   ; TODO: vibrato
  NTWT ch2_gs2_1_fade, 22 
  NTWT ch2_gs2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_gs2_1_fade, 22 
  NTWT ch2_gs2_1_fade, 22 
  NTWT ch2_cs1_1_fade, 22 
  RETN
seq40::
  WAIT 22
  NTWT ch2_a2_1_fade, 22   ; TODO: fadein
  NTWT ch2_f2_1_hold, 22 
  NTWT ch2_cs2_1_fade, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_f2_1_hold, 22 
  NTWT ch2_cs2_1_fade, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22   ; TODO: vibrato
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_a2_1_hold, 22 
  NTWT ch2_as2_1_hold, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_d1_1_fade, 22 
  NTWT ch2_f1_1_fade, 22 
  NTWT ch2_gs2_1_fade, 22   ; TODO: fadein
  NTWT ch2_f2_1_hold, 22 
  NTWT ch2_c2_1_fade, 22 
  NTWT ch2_cs2_1_fade, 22 
  NTWT ch2_gs2_1_fade, 22 
  NTWT ch2_f2_1_hold, 22 
  NTWT ch2_c2_1_fade, 22 
  NTWT ch2_cs2_1_fade, 22 
  NTWT ch2_gs2_1_fade, 22   ; TODO: vibrato
  NTWT ch2_gs2_1_fade, 22 
  NTWT ch2_e2_1_hold, 22 
  NTWT ch2_f2_1_hold, 22 
  NTWT ch2_gs2_1_fade, 22 
  NTWT ch2_gs2_1_fade, 22 
  NTWT ch2_e1_1_fade, 22 
  RETN
seq46::
  WAIT 22
  NTWT ch2_as2_1_fade, 22   ; TODO: vibrato
  NTWT ch2_g2_1_hold, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_g2_1_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_g2_1_hold, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_g2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22   ; TODO: vibrato
  NTWT ch2_a2_1_fade, 22   ; TODO: vibrato
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_g2_1_fade, 22 
  NTWT ch2_g2_1_fade, 22 
  NTWT ch2_d1_1_fade, 22 
  NTWT ch2_g1_1_fade, 22 
  NTWT ch2_as2_1_fade, 22   ; TODO: fadein
  NTWT ch2_g2_1_hold, 22 
  NTWT ch2_ds2_1_fade, 22 
  NTWT ch2_g2_1_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_g2_1_hold, 22 
  NTWT ch2_ds2_1_fade, 22 
  NTWT ch2_g2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22   ; TODO: vibrato
  NTWT ch2_a2_1_fade, 22   ; TODO: vibrato
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_g2_1_fade, 22 
  NTWT ch2_g2_1_fade, 22 
  NTWT ch2_ds1_1_fade, 22 
  RETN
seq47::
  WAIT 22
  NTWT ch2_a2_1_fade, 22   ; TODO: fadein
  NTWT ch2_e2_1_hold, 22 
  NTWT ch2_cs2_1_fade, 22 
  NTWT ch2_f2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_e2_1_hold, 22 
  NTWT ch2_cs2_1_fade, 22 
  NTWT ch2_f2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22   ; TODO: vibrato
  NTWT ch2_as2_1_hold, 22 
  NTWT ch2_a2_1_hold, 22 
  NTWT ch2_g2_1_hold, 22 
  NTWT ch2_a2_1_hold, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_cs1_1_fade, 22 
  NTWT ch2_e1_1_fade, 22 
  NTWT ch2_gs2_1_fade, 22   ; TODO: fadein
  NTWT ch2_f2_1_hold, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_as1_1_fade, 22 
  NTWT ch2_gs2_1_fade, 22 
  NTWT ch2_f2_1_hold, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_gs2_1_fade, 22 
  NTWT ch2_gs2_1_hold, 22 
  NTWT ch2_gs2_1_fade, 22 
  NTWT ch2_f2_1_fade, 22 
  NTWT ch2_gs2_1_fade, 22 
  NTWT ch2_g2_1_fade, 22 
  NTWT ch2_f2_1_fade, 22 
  NTWT ch2_e2_1_fade, 22 
  RETN
seq30::
  NTWT ch2_a0_1_fade, 22   ; TODO: fadein
  NTWT ch2_d1_2_hold, 22 
  NTWT ch2_f1_3_hold, 22 
  NTWT ch2_d1_3_hold, 22 
  NTWT ch2_a0_2_fade, 22 
  NTWT ch2_d1_2_fade, 22 
  NTWT ch2_f1_3_fade, 22 
  NTWT ch2_d0_3_fade, 22 
  NTWT ch2_f0_2_fade, 22   ; TODO: fadein
  NTWT ch2_d1_2_fade, 22 
  NTWT ch2_f1_3_fade, 22 
  NTWT ch2_d1_3_fade, 22 
  NTWT ch2_a0_4_fade, 22 
  NTWT ch2_d1_4_hold, 22 
  NTWT ch2_f1_4_hold, 22 
  NTWT ch2_d0_4_hold, 22 
  NTWT ch2_gs0_4_hold, 22 
  NTWT ch2_cs1_4_hold, 22 
  NTWT ch2_e1_2_hold, 22 
  NTWT ch2_cs1_2_hold, 22 
  NTWT ch2_gs0_3_fade, 22 
  NTWT ch2_cs1_3_fade, 22 
  NTWT ch2_e1_2_fade, 22 
  NTWT ch2_cs0_2_fade, 22 
  NTWT ch2_e0_2_fade, 22   ; TODO: fadein
  NTWT ch2_cs1_2_fade, 22 
  NTWT ch2_e1_2_fade, 22 
  NTWT ch2_cs1_2_fade, 22 
  NTWT ch2_gs0_3_fade, 22 
  NTWT ch2_cs1_3_hold, 22 
  NTWT ch2_e1_4_hold, 22 
  NTWT ch2_cs0_4_hold, 22 
  NOTE ch2_mute
  RETN
seq31::
  NTWT ch2_a0_2_fade, 22   ; TODO: fadein
  NTWT ch2_d1_2_hold, 22 
  NTWT ch2_f1_3_hold, 22 
  NTWT ch2_d1_3_hold, 22 
  NTWT ch2_a0_4_fade, 22 
  NTWT ch2_d1_4_fade, 22 
  NTWT ch2_f1_3_fade, 22 
  NTWT ch2_d1_3_fade, 22 
  NTWT ch2_f1_2_fade, 22   ; TODO: fadein
  NTWT ch2_d1_2_fade, 22 
  NTWT ch2_f1_3_fade, 22 
  NTWT ch2_d1_3_fade, 22 
  NTWT ch2_a0_4_fade, 22 
  NTWT ch2_d1_4_hold, 22 
  NTWT ch2_f1_4_hold, 22 
  NTWT ch2_d1_4_hold, 22 
  NTWT ch2_gs0_4_hold, 22 
  NTWT ch2_cs1_4_hold, 22 
  NTWT ch2_f1_2_hold, 22 
  NTWT ch2_cs1_2_hold, 22 
  NTWT ch2_gs0_3_fade, 22 
  NTWT ch2_cs1_3_fade, 22 
  NTWT ch2_f1_2_fade, 22 
  NTWT ch2_cs1_2_fade, 22 
  NTWT ch2_f1_2_fade, 22   ; TODO: fadein
  NTWT ch2_cs1_2_fade, 22 
  NTWT ch2_f1_2_fade, 22 
  NTWT ch2_cs1_2_fade, 22 
  NTWT ch2_gs0_3_fade, 22 
  NTWT ch2_cs1_3_hold, 22 
  NTWT ch2_f1_4_hold, 22 
  NTWT ch2_cs1_4_hold, 22 
  NOTE ch2_mute
  RETN
seq32::
  NTWT ch2_as0_2_fade, 22   ; TODO: fadein
  NTWT ch2_d1_2_hold, 22 
  NTWT ch2_g1_3_hold, 22 
  NTWT ch2_d1_3_hold, 22 
  NTWT ch2_as0_4_fade, 22 
  NTWT ch2_d1_4_fade, 22 
  NTWT ch2_g1_3_fade, 22 
  NTWT ch2_d1_3_fade, 22 
  NTWT ch2_g1_2_fade, 22   ; TODO: fadein
  NTWT ch2_d1_2_fade, 22 
  NTWT ch2_g1_3_fade, 22 
  NTWT ch2_d1_3_fade, 22 
  NTWT ch2_as0_4_fade, 22 
  NTWT ch2_d1_4_hold, 22 
  NTWT ch2_g1_4_hold, 22 
  NTWT ch2_d1_4_hold, 22 
  NTWT ch2_as0_4_hold, 22 
  NTWT ch2_ds1_4_hold, 22 
  NTWT ch2_g1_2_hold, 22 
  NTWT ch2_ds1_2_hold, 22 
  NTWT ch2_as0_3_fade, 22 
  NTWT ch2_ds1_3_fade, 22 
  NTWT ch2_g1_2_fade, 22 
  NTWT ch2_ds1_2_fade, 22 
  NTWT ch2_g1_2_fade, 22   ; TODO: fadein
  NTWT ch2_ds1_2_fade, 22 
  NTWT ch2_g1_2_fade, 22 
  NTWT ch2_ds1_2_fade, 22 
  NTWT ch2_as0_3_fade, 22 
  NTWT ch2_ds1_3_hold, 22 
  NTWT ch2_g1_4_hold, 22 
  NTWT ch2_ds1_4_hold, 22 
  NOTE ch2_mute
  RETN
seq33::
  NTWT ch2_e1_2_fade, 22   ; TODO: fadein
  NTWT ch2_a0_2_hold, 22 
  NTWT ch2_e1_3_hold, 22 
  NTWT ch2_a0_3_hold, 22 
  NTWT ch2_e1_4_fade, 22 
  NTWT ch2_a0_4_fade, 22 
  NTWT ch2_e1_3_fade, 22 
  NTWT ch2_a0_3_fade, 22 
  NTWT ch2_e1_2_fade, 22   ; TODO: fadein
  NTWT ch2_a0_2_fade, 22 
  NTWT ch2_e1_3_fade, 22 
  NTWT ch2_a0_3_fade, 22 
  NTWT ch2_e1_4_fade, 22 
  NTWT ch2_a0_4_hold, 22 
  NTWT ch2_e1_4_hold, 22 
  NTWT ch2_a0_4_hold, 22 
  NTWT ch2_f1_4_hold, 22 
  NTWT ch2_d1_4_hold, 22 
  NTWT ch2_f1_2_hold, 22 
  NTWT ch2_d1_2_hold, 22 
  NTWT ch2_f1_3_fade, 22 
  NTWT ch2_d1_3_fade, 22 
  NTWT ch2_f1_2_fade, 22 
  NTWT ch2_d1_2_fade, 22 
  NTWT ch2_f1_2_fade, 22   ; TODO: fadein
  NTWT ch2_d1_2_fade, 22 
  NTWT ch2_f1_2_fade, 22 
  NTWT ch2_d1_2_fade, 22 
  NTWT ch2_a0_3_fade, 22 
  NTWT ch2_cs1_3_hold, 22 
  NTWT ch2_e1_4_hold, 22 
  NTWT ch2_cs1_4_hold, 22 
  NOTE ch2_mute
  RETN
seq54::
  NTWT ch2_d0_1_fade, 22 
  NTWT ch2_d0_4_fade, 154 
  NTWT ch2_d1_1_fade, 22 
  NTWT ch2_f1_2_fade, 22 
  NTWT ch2_f1_2_fade, 22 
  NTWT ch2_d1_2_fade, 22 
  NTWT ch2_d1_3_fade, 22 
  NTWT ch2_f1_3_fade, 22 
  NTWT ch2_f1_4_fade, 22 
  NTWT ch2_d1_2_fade, 22 
  NTWT ch2_cs0_1_fade, 22 
  NTWT ch2_cs0_4_fade, 154 
  NTWT ch2_e1_1_fade, 22 
  NTWT ch2_gs1_2_fade, 22 
  NTWT ch2_gs1_2_fade, 22 
  NTWT ch2_e1_2_fade, 22 
  NTWT ch2_e1_3_fade, 22 
  NTWT ch2_a1_3_fade, 22 
  NTWT ch2_a1_4_fade, 22 
  NTWT ch2_gs1_1_fade, 22 
  RETN
seq55::
  NTWT ch2_d0_1_fade, 22 
  NTWT ch2_d0_4_fade, 154 
  NTWT ch2_d1_1_fade, 22 
  NTWT ch2_f1_2_fade, 22 
  NTWT ch2_f1_2_fade, 22 
  NTWT ch2_d1_2_fade, 22 
  NTWT ch2_d1_3_fade, 22 
  NTWT ch2_f1_3_fade, 22 
  NTWT ch2_f1_4_fade, 22 
  NTWT ch2_d1_2_fade, 22 
  NTWT ch2_f0_1_fade, 22 
  NTWT ch2_f0_4_fade, 154 
  NTWT ch2_f1_1_fade, 22 
  NTWT ch2_gs1_2_fade, 22 
  NTWT ch2_gs1_2_fade, 22 
  NTWT ch2_f1_2_fade, 22 
  NTWT ch2_f1_3_fade, 22 
  NTWT ch2_cs2_3_fade, 22 
  NTWT ch2_cs2_4_fade, 22 
  NTWT ch2_c2_1_fade, 22 
  RETN
seq56::
  NTWT ch2_g0_1_fade, 22 
  NTWT ch2_g0_4_fade, 154 
  NTWT ch2_g1_1_fade, 22 
  NTWT ch2_as1_2_fade, 22 
  NTWT ch2_as1_2_fade, 22 
  NTWT ch2_g1_2_fade, 22 
  NTWT ch2_g1_3_fade, 22 
  NTWT ch2_as1_3_fade, 22 
  NTWT ch2_as1_4_fade, 22 
  NTWT ch2_g1_2_fade, 22 
  NTWT ch2_ds0_1_fade, 22 
  NTWT ch2_ds0_4_fade, 154 
  NTWT ch2_g1_1_fade, 22 
  NTWT ch2_as1_2_fade, 22 
  NTWT ch2_as1_2_fade, 22 
  NTWT ch2_cs1_2_fade, 22 
  NTWT ch2_ds1_3_fade, 22 
  NTWT ch2_as1_3_fade, 22 
  NTWT ch2_as1_4_fade, 22 
  NTWT ch2_g1_1_fade, 22 
  RETN
seq57::
  NTWT ch2_a0_1_fade, 22 
  NTWT ch2_a0_4_fade, 154 
  NTWT ch2_cs1_1_fade, 22 
  NTWT ch2_a1_2_fade, 22 
  NTWT ch2_a1_2_fade, 22 
  NTWT ch2_e1_2_fade, 22 
  NTWT ch2_f1_3_fade, 22 
  NTWT ch2_as1_3_fade, 22 
  NTWT ch2_as1_4_fade, 22 
  NTWT ch2_a1_2_fade, 22 
  NTWT ch2_as0_1_fade, 22 
  NTWT ch2_as0_4_fade, 154 
  NTWT ch2_d1_1_fade, 22 
  NTWT ch2_e1_2_fade, 22 
  NTWT ch2_f1_2_fade, 22 
  NTWT ch2_d1_2_fade, 22 
  NTWT ch2_cs1_3_fade, 22 
  NTWT ch2_d1_3_fade, 22 
  NTWT ch2_f1_4_fade, 22 
  NTWT ch2_e1_1_fade, 22 
  RETN

CH12_NOTE    ch12_cs0 0x94 0x80
CH2_NOTE_ENV ch2_cs0_1_fade ENV_1FADE ch12_cs0
CH2_NOTE_ENV ch2_cs0_2_fade ENV_2FADE ch12_cs0
CH2_NOTE_ENV ch2_cs0_4_hold ENV_4HOLD ch12_cs0
CH2_NOTE_ENV ch2_cs0_4_fade ENV_4FADE ch12_cs0
CH12_NOTE    ch12_d0  0x05 0x81
CH2_NOTE_ENV ch2_d0_1_fade  ENV_1FADE ch12_d0
CH2_NOTE_ENV ch2_d0_3_fade  ENV_3FADE ch12_d0
CH2_NOTE_ENV ch2_d0_4_hold  ENV_4HOLD ch12_d0
CH2_NOTE_ENV ch2_d0_4_fade  ENV_4FADE ch12_d0
CH12_NOTE    ch12_ds0 0x6a 0x81
CH2_NOTE_ENV ch2_ds0_1_fade ENV_1FADE ch12_ds0
CH2_NOTE_ENV ch2_ds0_4_fade ENV_4FADE ch12_ds0
CH12_NOTE    ch12_e0  0xc3 0x81
CH2_NOTE_ENV ch2_e0_2_fade  ENV_2FADE ch12_e0
CH12_NOTE    ch12_f0  0x20 0x82
CH2_NOTE_ENV ch2_f0_1_fade  ENV_1FADE ch12_f0
CH2_NOTE_ENV ch2_f0_2_fade  ENV_2FADE ch12_f0
CH2_NOTE_ENV ch2_f0_4_fade  ENV_4FADE ch12_f0
CH12_NOTE    ch12_g0  0xc5 0x82
CH2_NOTE_ENV ch2_g0_1_fade  ENV_1FADE ch12_g0
CH2_NOTE_ENV ch2_g0_4_fade  ENV_4FADE ch12_g0
CH12_NOTE    ch12_gs0 0x0f 0x83
CH2_NOTE_ENV ch2_gs0_3_fade ENV_3FADE ch12_gs0
CH2_NOTE_ENV ch2_gs0_4_hold ENV_4HOLD ch12_gs0
CH12_NOTE    ch12_a0  0x55 0x83
CH2_NOTE_ENV ch2_a0_1_fade  ENV_1FADE ch12_a0
CH2_NOTE_ENV ch2_a0_2_hold  ENV_2HOLD ch12_a0
CH2_NOTE_ENV ch2_a0_2_fade  ENV_2FADE ch12_a0
CH2_NOTE_ENV ch2_a0_3_hold  ENV_3HOLD ch12_a0
CH2_NOTE_ENV ch2_a0_3_fade  ENV_3FADE ch12_a0
CH2_NOTE_ENV ch2_a0_4_hold  ENV_4HOLD ch12_a0
CH2_NOTE_ENV ch2_a0_4_fade  ENV_4FADE ch12_a0
CH12_NOTE    ch12_as0 0x9a 0x83
CH2_NOTE_ENV ch2_as0_1_fade ENV_1FADE ch12_as0
CH2_NOTE_ENV ch2_as0_2_fade ENV_2FADE ch12_as0
CH2_NOTE_ENV ch2_as0_3_fade ENV_3FADE ch12_as0
CH2_NOTE_ENV ch2_as0_4_hold ENV_4HOLD ch12_as0
CH2_NOTE_ENV ch2_as0_4_fade ENV_4FADE ch12_as0
CH12_NOTE    ch12_c1  0x14 0x84
CH1_NOTE_ENV ch1_c1_1_hold  ENV_1HOLD ch12_c1
CH2_NOTE_ENV ch2_c1_1_hold  ENV_1HOLD ch12_c1
CH12_NOTE    ch12_cs1 0x4c 0x84
CH2_NOTE_ENV ch2_cs1_1_hold ENV_1HOLD ch12_cs1
CH1_NOTE_ENV ch1_cs1_1_fade ENV_1FADE ch12_cs1
CH2_NOTE_ENV ch2_cs1_1_fade ENV_1FADE ch12_cs1
CH1_NOTE_ENV ch1_cs1_2_hold ENV_2HOLD ch12_cs1
CH2_NOTE_ENV ch2_cs1_2_hold ENV_2HOLD ch12_cs1
CH1_NOTE_ENV ch1_cs1_2_fade ENV_2FADE ch12_cs1
CH2_NOTE_ENV ch2_cs1_2_fade ENV_2FADE ch12_cs1
CH2_NOTE_ENV ch2_cs1_3_hold ENV_3HOLD ch12_cs1
CH2_NOTE_ENV ch2_cs1_3_fade ENV_3FADE ch12_cs1
CH2_NOTE_ENV ch2_cs1_4_hold ENV_4HOLD ch12_cs1
CH12_NOTE    ch12_d1  0x82 0x84
CH1_NOTE_ENV ch1_d1_1_fade  ENV_1FADE ch12_d1
CH2_NOTE_ENV ch2_d1_1_fade  ENV_1FADE ch12_d1
CH2_NOTE_ENV ch2_d1_2_hold  ENV_2HOLD ch12_d1
CH1_NOTE_ENV ch1_d1_2_fade  ENV_2FADE ch12_d1
CH2_NOTE_ENV ch2_d1_2_fade  ENV_2FADE ch12_d1
CH2_NOTE_ENV ch2_d1_3_hold  ENV_3HOLD ch12_d1
CH2_NOTE_ENV ch2_d1_3_fade  ENV_3FADE ch12_d1
CH2_NOTE_ENV ch2_d1_4_hold  ENV_4HOLD ch12_d1
CH2_NOTE_ENV ch2_d1_4_fade  ENV_4FADE ch12_d1
CH12_NOTE    ch12_ds1 0xb5 0x84
CH1_NOTE_ENV ch1_ds1_1_hold ENV_1HOLD ch12_ds1
CH2_NOTE_ENV ch2_ds1_1_hold ENV_1HOLD ch12_ds1
CH2_NOTE_ENV ch2_ds1_1_fade ENV_1FADE ch12_ds1
CH2_NOTE_ENV ch2_ds1_2_hold ENV_2HOLD ch12_ds1
CH1_NOTE_ENV ch1_ds1_2_fade ENV_2FADE ch12_ds1
CH2_NOTE_ENV ch2_ds1_2_fade ENV_2FADE ch12_ds1
CH2_NOTE_ENV ch2_ds1_3_hold ENV_3HOLD ch12_ds1
CH2_NOTE_ENV ch2_ds1_3_fade ENV_3FADE ch12_ds1
CH2_NOTE_ENV ch2_ds1_4_hold ENV_4HOLD ch12_ds1
CH12_NOTE    ch12_e1  0xe3 0x84
CH2_NOTE_ENV ch2_e1_1_hold  ENV_1HOLD ch12_e1
CH1_NOTE_ENV ch1_e1_1_fade  ENV_1FADE ch12_e1
CH2_NOTE_ENV ch2_e1_1_fade  ENV_1FADE ch12_e1
CH2_NOTE_ENV ch2_e1_2_hold  ENV_2HOLD ch12_e1
CH1_NOTE_ENV ch1_e1_2_fade  ENV_2FADE ch12_e1
CH2_NOTE_ENV ch2_e1_2_fade  ENV_2FADE ch12_e1
CH1_NOTE_ENV ch1_e1_3_hold  ENV_3HOLD ch12_e1
CH2_NOTE_ENV ch2_e1_3_hold  ENV_3HOLD ch12_e1
CH2_NOTE_ENV ch2_e1_3_fade  ENV_3FADE ch12_e1
CH2_NOTE_ENV ch2_e1_4_hold  ENV_4HOLD ch12_e1
CH2_NOTE_ENV ch2_e1_4_fade  ENV_4FADE ch12_e1
CH12_NOTE    ch12_f1  0x10 0x85
CH2_NOTE_ENV ch2_f1_1_hold  ENV_1HOLD ch12_f1
CH1_NOTE_ENV ch1_f1_1_fade  ENV_1FADE ch12_f1
CH2_NOTE_ENV ch2_f1_1_fade  ENV_1FADE ch12_f1
CH2_NOTE_ENV ch2_f1_2_hold  ENV_2HOLD ch12_f1
CH2_NOTE_ENV ch2_f1_2_fade  ENV_2FADE ch12_f1
CH1_NOTE_ENV ch1_f1_3_hold  ENV_3HOLD ch12_f1
CH2_NOTE_ENV ch2_f1_3_hold  ENV_3HOLD ch12_f1
CH2_NOTE_ENV ch2_f1_3_fade  ENV_3FADE ch12_f1
CH2_NOTE_ENV ch2_f1_4_hold  ENV_4HOLD ch12_f1
CH2_NOTE_ENV ch2_f1_4_fade  ENV_4FADE ch12_f1
CH12_NOTE    ch12_g1  0x63 0x85
CH2_NOTE_ENV ch2_g1_1_hold  ENV_1HOLD ch12_g1
CH1_NOTE_ENV ch1_g1_1_fade  ENV_1FADE ch12_g1
CH2_NOTE_ENV ch2_g1_1_fade  ENV_1FADE ch12_g1
CH1_NOTE_ENV ch1_g1_2_hold  ENV_2HOLD ch12_g1
CH2_NOTE_ENV ch2_g1_2_hold  ENV_2HOLD ch12_g1
CH2_NOTE_ENV ch2_g1_2_fade  ENV_2FADE ch12_g1
CH2_NOTE_ENV ch2_g1_3_hold  ENV_3HOLD ch12_g1
CH2_NOTE_ENV ch2_g1_3_fade  ENV_3FADE ch12_g1
CH2_NOTE_ENV ch2_g1_4_hold  ENV_4HOLD ch12_g1
CH12_NOTE    ch12_gs1 0x89 0x85
CH2_NOTE_ENV ch2_gs1_1_hold ENV_1HOLD ch12_gs1
CH1_NOTE_ENV ch1_gs1_1_fade ENV_1FADE ch12_gs1
CH2_NOTE_ENV ch2_gs1_1_fade ENV_1FADE ch12_gs1
CH1_NOTE_ENV ch1_gs1_2_hold ENV_2HOLD ch12_gs1
CH1_NOTE_ENV ch1_gs1_2_fade ENV_2FADE ch12_gs1
CH2_NOTE_ENV ch2_gs1_2_fade ENV_2FADE ch12_gs1
CH1_NOTE_ENV ch1_gs1_3_hold ENV_3HOLD ch12_gs1
CH1_NOTE_ENV ch1_gs1_4_hold ENV_4HOLD ch12_gs1
CH12_NOTE    ch12_a1  0xab 0x85
CH2_NOTE_ENV ch2_a1_1_hold  ENV_1HOLD ch12_a1
CH1_NOTE_ENV ch1_a1_1_fade  ENV_1FADE ch12_a1
CH2_NOTE_ENV ch2_a1_1_fade  ENV_1FADE ch12_a1
CH1_NOTE_ENV ch1_a1_2_hold  ENV_2HOLD ch12_a1
CH1_NOTE_ENV ch1_a1_2_fade  ENV_2FADE ch12_a1
CH2_NOTE_ENV ch2_a1_2_fade  ENV_2FADE ch12_a1
CH1_NOTE_ENV ch1_a1_3_hold  ENV_3HOLD ch12_a1
CH2_NOTE_ENV ch2_a1_3_fade  ENV_3FADE ch12_a1
CH2_NOTE_ENV ch2_a1_4_fade  ENV_4FADE ch12_a1
CH1_NOTE_ENV ch1_a1_5_hold  ENV_5HOLD ch12_a1
CH12_NOTE    ch12_as1 0xcd 0x85
CH2_NOTE_ENV ch2_as1_1_hold ENV_1HOLD ch12_as1
CH1_NOTE_ENV ch1_as1_1_fade ENV_1FADE ch12_as1
CH2_NOTE_ENV ch2_as1_1_fade ENV_1FADE ch12_as1
CH2_NOTE_ENV ch2_as1_2_fade ENV_2FADE ch12_as1
CH1_NOTE_ENV ch1_as1_3_hold ENV_3HOLD ch12_as1
CH1_NOTE_ENV ch1_as1_3_fade ENV_3FADE ch12_as1
CH2_NOTE_ENV ch2_as1_3_fade ENV_3FADE ch12_as1
CH2_NOTE_ENV ch2_as1_4_fade ENV_4FADE ch12_as1
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
CH2_NOTE_ENV ch2_cs2_3_fade ENV_3FADE ch12_cs2
CH2_NOTE_ENV ch2_cs2_4_fade ENV_4FADE ch12_cs2
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
