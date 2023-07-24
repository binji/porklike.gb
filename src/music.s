CMD_NOTE = 0
CMD_WAIT = 1
CMD_JUMP = 2
CMD_CALL = 3
CMD_RETN = 4
CMD_NOFX = 5

; manually assign song_ptr and wait_timer to HRAM
; TODO: better way?
ch1_wait_timer = 0xffa0
ch1_song_ptr   = ch1_wait_timer + 1  ; ffa1
ch1_stack      = ch1_song_ptr   + 2  ; ffa3
ch2_wait_timer = ch1_stack      + 2  ; ffa5
ch2_song_ptr   = ch2_wait_timer + 1  ; ffa6
ch2_stack      = ch2_song_ptr   + 2  ; ffa8
ch3_wait_timer = ch2_stack      + 2  ; ffaa
ch3_song_ptr   = ch3_wait_timer + 1  ; ffab
ch3_stack      = ch3_song_ptr   + 2  ; ffad
ch4_wait_timer = ch3_stack      + 2  ; ffaf
ch4_song_ptr   = ch4_wait_timer + 1  ; ffb0
ch4_stack      = ch4_song_ptr   + 2  ; ffb2
snddata_end    = ch4_stack      + 2  ; ffb4

current_sfx_priority = snddata_end + 1

.area _CODE

song_silence::
  .db 0      ; ch1
  .dw seqmute
  .dw 0
  .db 0      ; ch2
  .dw seqmute
  .dw 0
  .db 0      ; ch3
  .dw seqmute
  .dw 0
  .db 0      ; ch4
  .dw seqmute
  .dw 0

song_main::
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
  .dw seqmute
  .dw 0

song_dead::
  .db 0      ; ch1
  .dw ch1_dead
  .dw 0
  .db 0      ; ch2
  .dw ch2_dead
  .dw 0
  .db 0      ; ch3
  .dw ch3_dead
  .dw 0
  .db 0      ; ch4
  .dw seqmute
  .dw 0

song_win::
  .db 0      ; ch1
  .dw ch1_win
  .dw 0
  .db 0      ; ch2
  .dw ch2_win
  .dw 0
  .db 0      ; ch3
  .dw ch3_win
  .dw 0
  .db 0      ; ch4
  .dw seqmute
  .dw 0

sfx::
  .dw sfx_seq0
  .dw sfx_seq1
  .dw sfx_seq2
  .dw sfx_seq3
  .dw sfx_seq4
  .dw sfx_seq5
  .dw sfx_seq6
  .dw sfx_seq7
  .dw sfx_seq8
  .dw sfx_seq9
  .dw sfx_seq10
  .dw sfx_seq11
  .dw sfx_seq12
  .dw sfx_seq13
  .dw sfx_seq14
  .dw sfx_seq15
  .dw sfx_seq16
  .dw sfx_seq17
  .dw sfx_seq18
  .dw sfx_seq19
  .dw sfx_seq20
  .dw sfx_seq21

sfx_priority::
  .db 15 ; SFX_BLIND
  .db  7 ; SFX_TELEPORT
  .db  5 ; SFX_BACK
  .db  4 ; SFX_OK
  .db 17 ; SFX_STAIRS
  .db  3 ; SFX_SELECT
  .db 14 ; SFX_HIT_PLAYER
  .db 13 ; SFX_HIT_MOB
  .db  7 ; SFX_OPEN_OBJECT
  .db  7 ; SFX_OOPS
  .db  8 ; SFX_PICKUP
  .db 10 ; SFX_USE_EQUIP
  .db  1 ; SFX_PLAYER_STEP
  .db  2 ; SFX_MOB_PUSH
  .db 16 ; SFX_BOOM
  .db 12 ; SFX_REAPER
  .db 10 ; SFX_SPIN
  .db 10 ; SFX_SPEAR
  .db 10 ; SFX_DESTROY_WALL
  .db  7 ; SFX_BUMP_TILE
  .db 11 ; SFX_HEART
  .db  9 ; SFX_FAIL

_play_music::
  di
  ld bc, #snddata_end - #ch1_wait_timer
  push bc
  ld c, e
  ld b, d
  ld de, #ch1_wait_timer
  call _memcpy
  ei
  ret

_music_main::
  ld de, #song_main
  call _play_music
  ret

_music_dead::
  ld de, #song_dead
  call _play_music
  ret

_music_win::
  ld de, #song_win
  call _play_music
  ret

_sfx::
  di
  ld d, a       ; d = a = sfx to play

  ld hl, #sfx_priority
  ld a, l
  add d
  ld l, a
  ld a, h
  adc #0
  ld h, a       ; hl = &sfx_priority[n]
  ld a, (hl)
  ld c, a       ; c = sfx priority

  ; check whether any sfx is playing
  ld a, (#current_sfx_priority)
  ld e, a       ; e = current_sfx_priority
  or a
  jr z, sfxok

  ; skip this sfx if the current one has higher priority
  ld a, c
  cp e
  jr c, sfxskip

sfxok:
  ; write current priority
  ld a, c
  ldh (#current_sfx_priority), a

  ld a, d
  rlca
  ld hl, #sfx
  add l
  ld l, a
  ld a, h
  adc #0
  ld h, a       ; hl = &sfx[n]

  ld a, (hl+)
  ld e, a
  ld a, (hl)
  ld d, a       ; de = sfx[n]

  ld hl, #ch4_wait_timer
  xor a
  ld (hl+), a   ; wait_timer = 0
  ld a, e
  ld (hl+), a   ; song_ptr lo
  ld a, d
  ld (hl), a   ; song_ptr hi

sfxskip:
  ei
  ret


_snd_init::
  xor a
  ldh (#current_sfx_priority), a

  ld de, #song_silence
  call _play_music

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
  cp a, #5
  jr z, nofx

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

  ; are we playing sfx?
  ldh a, (#current_sfx_priority)
  or a
  jr z, noteok
  ; is this channel 2? (used for many sfx)
  ld a, c
  cp a, #ch2_song_ptr + 1
  jr z, notedone

noteok:
  ld de, #notedone  ; TODO: smarter way to save the return address?
  push de
  jp (hl)
notedone:
  pop hl
  jr read_command

nofx:
  xor a
  ldh (#current_sfx_priority), a
  ; jump to silence on this channel (should be channel 4)
  ld hl, #seqmute
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

.macro FADE100 delay    ; 18 + N
  NTWT ch3_vol100, 8
  NTWT ch3_vol50, 6
  NTWT ch3_vol25, 4
  NTWT ch3_mute, delay  ; N
.endm

.macro FADE100_NODELAY  ; 18 + N
  NTWT ch3_vol100, 8
  NTWT ch3_vol50, 6
  NTWT ch3_vol25, 4
  NOTE ch3_mute
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

.macro FADEIN25 delay     ; 18 + N
  NTWT ch3_vol25, 4       ; 4
  NTWT ch3_vol50, 6       ; 6
  NTWT ch3_vol100, 8      ; 8
  WAIT delay              ; N
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

.macro NOFX
  .db CMD_NOFX
.endm

ch1::
  NOTE ch1_mute
  CALL_ seqpause
  CALL_ ch1_seq37
  CALL_ ch1_seq41
  CALL_ ch1_seq48
  CALL_ ch1_seq49
  CALL_ ch1_seq35
  CALL_ ch1_seq39
  CALL_ ch1_seq44
  CALL_ ch1_seq45
  CALL_ ch1_seq30
  CALL_ ch1_seq31
  CALL_ ch1_seq32
  CALL_ ch1_seq33
  CALL_ ch1_seq54
  CALL_ ch1_seq55
  CALL_ ch1_seq56
  CALL_ ch1_seq57
  JUMP ch1

ch2::
  NOTE ch2_mute
  CALL_ seqpause
  CALL_ ch2_seq26
  CALL_ ch2_seq27
  CALL_ ch2_seq28
  CALL_ ch2_seq29
  CALL_ ch2_seq36
  CALL_ ch2_seq40
  CALL_ ch2_seq46
  CALL_ ch2_seq47
  CALL_ ch2_seq37
  CALL_ ch2_seq41
  CALL_ ch2_seq48
  CALL_ ch2_seq49
  CALL_ seqpause
  CALL_ seqpause
  CALL_ seqpause
  CALL_ seqpause
  JUMP ch2

ch3::
  NOTE ch3_mute
  CALL_ ch3_seq34
  CALL_ ch3_seq34
  CALL_ ch3_seq38
  CALL_ ch3_seq42
  CALL_ ch3_seq43
  CALL_ ch3_seq50
  CALL_ ch3_seq51
  CALL_ ch3_seq52
  CALL_ ch3_seq53
  CALL_ ch3_seq34
  CALL_ ch3_seq38
  CALL_ ch3_seq42
  CALL_ ch3_seq43
  CALL_ ch3_seq34
  CALL_ ch3_seq38
  CALL_ ch3_seq42
  CALL_ ch3_seq53
  JUMP ch3

ch1_dead::
  NOTE ch1_mute
  CALL_ ch1_seq63
  JUMP seqmute

ch2_dead::
  NOTE ch2_mute
  CALL_ ch2_seq61
  JUMP seqmute

ch3_dead::
  NOTE ch3_mute
  CALL_ ch3_seq62
  JUMP seqmute


ch1_win::
  NOTE ch1_mute
  CALL_ ch1_seq59
  JUMP seqmute

ch2_win::
  NOTE ch2_mute
  CALL_ ch2_seq58
  JUMP seqmute

ch3_win::
  NOTE ch3_mute
  CALL_ ch3_seq60
  JUMP seqmute


seqpause::
  WAIT 255
  WAIT 255
  WAIT 194
  RETN

seqmute::
  WAIT 255
  JUMP seqmute

;; CHANNEL 1/2 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ENV_1HOLD = 0b00100000
ENV_1FADE = 0b00100100
ENV_1FADEIN = 0b00101100
ENV_2HOLD = 0b01000000
ENV_2FADE = 0b01000100
ENV_2FADEIN = 0b01001100
ENV_3HOLD = 0b01100000
ENV_3FADE = 0b01100100
ENV_3FADEIN = 0b01101100
ENV_4HOLD = 0b10000000
ENV_4FADE = 0b10000100
ENV_4FADEIN = 0b10001100
ENV_5HOLD = 0b10100000
ENV_5FADE = 0b10100100
ENV_6HOLD = 0b11000000
ENV_7FADE = 0b11100100
ENV_7HOLD = 0b11100000

DUTY_125  = 0b00000000
DUTY_25   = 0b01000000
DUTY_50   = 0b10000000
DUTY_75   = 0b11000000

.macro CH12_NOTE name lo hi
name::
  ld (hl+), a
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
  jr ch
.endm

.macro CH2_NOTE_ENV name env ch
name::
  ld hl, #0xff17
  ld a, #env
  jr ch
.endm

.macro CH1_DUTY name duty
name::
  ld a, #duty
  ldh (#11), a
  ret
.endm

.macro CH2_DUTY name duty
name::
  ld a, #duty
  ldh (#16), a
  ret
.endm

ch1_mute::
  xor a
  ldh (#0x12), a
  ret

ch2_mute::
  xor a
  ldh (#0x17), a
  ret

CH1_DUTY ch1_duty125 DUTY_125
CH1_DUTY ch1_duty25 DUTY_25
CH1_DUTY ch1_duty50 DUTY_50
CH1_DUTY ch1_duty75 DUTY_75

CH2_DUTY ch2_duty125 DUTY_125
CH2_DUTY ch2_duty25 DUTY_25
CH2_DUTY ch2_duty50 DUTY_50
CH2_DUTY ch2_duty75 DUTY_75

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

;; CHANNEL 4 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.macro CH4_NOTE name byte
name::
  ld (hl+), a
  ld a, #byte
  ld (hl+), a
  ld a, #0x80
  ld (hl+), a
  ret
.endm

.macro CH4_NOTE_ENV name env ch
name::
  ld hl, #0xff21
  ld a, #env
  jr ch
.endm

ch4_mute::
  ld a, #0b00000000
  ldh (#0x21), a
  ret

;; DATA ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ch3_seq34::
  NOTE ch3_d0
  FADE100, 26
  FADE25, 37
  FADE50, 74
  FADE50, 74
  FADE100, 48
  FADEIN25, 4
  NOTE ch3_cs0
  FADE100, 26
  FADE25, 37
  FADE50, 74
  FADE50, 74
  FADE50, 74
  RETN
ch3_seq38::
  NOTE ch3_d0
  FADE100, 26
  FADE25, 37
  FADE50, 74
  FADE50, 74
  FADE100, 48
  FADEIN25, 4
  NOTE ch3_f0
  FADE100, 26
  FADE25, 37
  FADE50, 74
  FADE50, 74
  FADE50, 74
  RETN
ch3_seq42::
  NOTE ch3_g0
  FADE100, 26
  FADE25, 37
  FADE50, 74
  FADE50, 74
  FADE100, 48
  FADEIN25, 4
  NOTE ch3_ds0
  FADE100, 26
  FADE25, 37
  FADE50, 74
  FADE50, 74
  FADE100, 70
  RETN
ch3_seq43::
  NOTE ch3_a0
  FADE100, 26
  FADE25, 37
  FADE50, 74
  FADE50, 74
  FADE100, 48
  FADEIN25, 4
  NOTE ch3_as0
  FADE100, 26
  FADE25, 37
  FADE50, 74
  FADE50, 74
  NOTE ch3_e0
  FADE50, 30
  NOTE ch3_a0
  FADE50, 30
  RETN
ch3_seq50::
  NOTE ch3_d0
  FADE100, 4
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
  FADEIN25, 4
  NOTE ch3_cs0
  FADE100, 4
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
ch3_seq51::
  NOTE ch3_d0
  FADE100, 4
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
  FADE100, 4
  NOTE ch3_f1
  FADE50, 8
  FADE25, 15
  NOTE ch3_d0
  FADEIN25, 4
  NOTE ch3_f0
  FADE100, 4
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
ch3_seq52::
  NOTE ch3_g0
  FADE100, 4
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
  FADE100, 4
  NOTE ch3_as1
  FADE50, 8
  FADE25, 15
  NOTE ch3_g0
  FADEIN25, 4
  NOTE ch3_ds0
  FADE100, 4
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
ch3_seq53::
  NOTE ch3_a0
  FADE100, 4
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
  FADE100, 4
  NOTE ch3_e1
  FADE50, 8
  NOTE ch3_cs1
  FADE25, 15
  NOTE ch3_a0
  FADEIN25, 4
  NOTE ch3_as0
  FADE100, 4
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
ch3_seq60::
  NOTE ch3_f1
  FADE50, 22
  NOTE ch3_fs1
  FADE50, 22
  NOTE ch3_a1
  FADE50, 22
  NOTE ch3_d1
  FADE100, 18
  NOTE ch3_fs1
  FADE50, 22
  NOTE ch3_a1
  FADE50, 22
  NOTE ch3_g1
  FADE50, 22
  NOTE ch3_as1
  FADE50, 22
  NOTE ch3_d2
  FADE50, 22
  NOTE ch3_g1
  FADE50, 22
  NOTE ch3_as1
  FADE50, 22
  NOTE ch3_e1
  FADE50, 22
  NOTE ch3_d1
  WAIT 18
  FADE100, 18  ; TODO: vibrato
  FADE100, 18  ; TODO: vibrato
  WAIT 90
  RETN
ch3_seq62::
  WAIT 152
  NOTE ch3_d1
  WAIT 19
  NOTE ch3_cs1
  WAIT 19
  NOTE ch3_as0
  WAIT 19
  NOTE ch3_d1
  FADE50, 5
  NOTE ch3_cs1
  FADE50, 24
  NOTE ch3_as0
  WAIT 19
  NOTE ch3_a0
  WAIT 19
  NOTE ch3_d1
  FADE50, 19  ; TODO: vibrato
  WAIT 19
  WAIT 266
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
CH3_NOTE ch3_fs1 0x9d 0x86
CH3_NOTE ch3_g1  0xb1 0x86
CH3_NOTE ch3_gs1 0xc4 0x86
CH3_NOTE ch3_a1  0xd6 0x86
CH3_NOTE ch3_as1 0xe7 0x86
CH3_NOTE ch3_d2  0x21 0x87

ch1_seq37::
  WAIT 44
  NOTE ch1_duty50
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
  NTWT ch1_e1_1_fadein, 22 
  NTWT ch1_c1_1_hold, 22 
  NTWT ch1_cs1_2_hold, 22 
  NTWT ch1_e1_3_hold, 22 
  NTWT ch1_gs1_3_hold, 22 
  NTWT ch1_a1_2_hold, 22 
  NTWT ch1_gs1_1_fade, 22   ; TODO: vibrato
  NTWT ch1_e1_1_fade, 22 
  RETN
ch1_seq41::
  WAIT 44
  NOTE ch1_duty50
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
ch1_seq48::
  WAIT 44
  NOTE ch1_duty50
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
ch1_seq49::
  NOTE ch1_duty50
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
ch1_seq35::
  NOTE ch1_duty50
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
  NTWT ch1_gs2_1_fadein, 22 
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
ch1_seq39::
  NOTE ch1_duty50
  NTWT ch1_a2_1_fadein, 22 
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
  NTWT ch1_gs2_1_fadein, 22 
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
ch1_seq44::
  NOTE ch1_duty50
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
  NTWT ch1_as2_1_fadein, 22 
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
ch1_seq45::
  NOTE ch1_duty50
  NTWT ch1_a2_1_fadein, 22 
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
  NTWT ch1_gs2_1_fadein, 22 
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
ch1_seq30::
  NOTE ch1_duty50
  NTWT ch1_a0_1_fadein, 22 
  NTWT ch1_d1_2_hold, 22 
  NTWT ch1_f1_3_hold, 22 
  NTWT ch1_d1_3_hold, 22 
  NTWT ch1_a0_2_fade, 22 
  NTWT ch1_d1_2_fade, 22 
  NTWT ch1_f1_3_fade, 22 
  NTWT ch1_d0_3_fade, 22 
  NTWT ch1_f0_2_fadein, 22 
  NTWT ch1_d1_2_fade, 22 
  NTWT ch1_f1_3_fade, 22 
  NTWT ch1_d1_3_fade, 22 
  NTWT ch1_a0_4_fade, 22 
  NTWT ch1_d1_4_hold, 22 
  NTWT ch1_f1_4_hold, 22 
  NTWT ch1_d0_4_hold, 22 
  NTWT ch1_gs0_4_hold, 22 
  NTWT ch1_cs1_4_hold, 22 
  NTWT ch1_e1_2_hold, 22 
  NTWT ch1_cs1_2_hold, 22 
  NTWT ch1_gs0_3_fade, 22 
  NTWT ch1_cs1_3_fade, 22 
  NTWT ch1_e1_2_fade, 22 
  NTWT ch1_cs0_2_fade, 22 
  NTWT ch1_e0_2_fadein, 22 
  NTWT ch1_cs1_2_fade, 22 
  NTWT ch1_e1_2_fade, 22 
  NTWT ch1_cs1_2_fade, 22 
  NTWT ch1_gs0_3_fade, 22 
  NTWT ch1_cs1_3_hold, 22 
  NTWT ch1_e1_4_hold, 22 
  NTWT ch1_cs0_4_hold, 22 
  NOTE ch1_mute
  RETN
ch1_seq31::
  NOTE ch1_duty50
  NTWT ch1_a0_2_fadein, 22 
  NTWT ch1_d1_2_hold, 22 
  NTWT ch1_f1_3_hold, 22 
  NTWT ch1_d1_3_hold, 22 
  NTWT ch1_a0_4_fade, 22 
  NTWT ch1_d1_4_fade, 22 
  NTWT ch1_f1_3_fade, 22 
  NTWT ch1_d1_3_fade, 22 
  NTWT ch1_f1_2_fadein, 22 
  NTWT ch1_d1_2_fade, 22 
  NTWT ch1_f1_3_fade, 22 
  NTWT ch1_d1_3_fade, 22 
  NTWT ch1_a0_4_fade, 22 
  NTWT ch1_d1_4_hold, 22 
  NTWT ch1_f1_4_hold, 22 
  NTWT ch1_d1_4_hold, 22 
  NTWT ch1_gs0_4_hold, 22 
  NTWT ch1_cs1_4_hold, 22 
  NTWT ch1_f1_2_hold, 22 
  NTWT ch1_cs1_2_hold, 22 
  NTWT ch1_gs0_3_fade, 22 
  NTWT ch1_cs1_3_fade, 22 
  NTWT ch1_f1_2_fade, 22 
  NTWT ch1_cs1_2_fade, 22 
  NTWT ch1_f1_2_fadein, 22 
  NTWT ch1_cs1_2_fade, 22 
  NTWT ch1_f1_2_fade, 22 
  NTWT ch1_cs1_2_fade, 22 
  NTWT ch1_gs0_3_fade, 22 
  NTWT ch1_cs1_3_hold, 22 
  NTWT ch1_f1_4_hold, 22 
  NTWT ch1_cs1_4_hold, 22 
  NOTE ch1_mute
  RETN
ch1_seq32::
  NOTE ch1_duty50
  NTWT ch1_as0_2_fadein, 22 
  NTWT ch1_d1_2_hold, 22 
  NTWT ch1_g1_3_hold, 22 
  NTWT ch1_d1_3_hold, 22 
  NTWT ch1_as0_4_fade, 22 
  NTWT ch1_d1_4_fade, 22 
  NTWT ch1_g1_3_fade, 22 
  NTWT ch1_d1_3_fade, 22 
  NTWT ch1_g1_2_fadein, 22 
  NTWT ch1_d1_2_fade, 22 
  NTWT ch1_g1_3_fade, 22 
  NTWT ch1_d1_3_fade, 22 
  NTWT ch1_as0_4_fade, 22 
  NTWT ch1_d1_4_hold, 22 
  NTWT ch1_g1_4_hold, 22 
  NTWT ch1_d1_4_hold, 22 
  NTWT ch1_as0_4_hold, 22 
  NTWT ch1_ds1_4_hold, 22 
  NTWT ch1_g1_2_hold, 22 
  NTWT ch1_ds1_2_hold, 22 
  NTWT ch1_as0_3_fade, 22 
  NTWT ch1_ds1_3_fade, 22 
  NTWT ch1_g1_2_fade, 22 
  NTWT ch1_ds1_2_fade, 22 
  NTWT ch1_g1_2_fadein, 22 
  NTWT ch1_ds1_2_fade, 22 
  NTWT ch1_g1_2_fade, 22 
  NTWT ch1_ds1_2_fade, 22 
  NTWT ch1_as0_3_fade, 22 
  NTWT ch1_ds1_3_hold, 22 
  NTWT ch1_g1_4_hold, 22 
  NTWT ch1_ds1_4_hold, 22 
  NOTE ch1_mute
  RETN
ch1_seq33::
  NOTE ch1_duty50
  NTWT ch1_e1_2_fadein, 22 
  NTWT ch1_a0_2_hold, 22 
  NTWT ch1_e1_3_hold, 22 
  NTWT ch1_a0_3_hold, 22 
  NTWT ch1_e1_4_fade, 22 
  NTWT ch1_a0_4_fade, 22 
  NTWT ch1_e1_3_fade, 22 
  NTWT ch1_a0_3_fade, 22 
  NTWT ch1_e1_2_fadein, 22 
  NTWT ch1_a0_2_fade, 22 
  NTWT ch1_e1_3_fade, 22 
  NTWT ch1_a0_3_fade, 22 
  NTWT ch1_e1_4_fade, 22 
  NTWT ch1_a0_4_hold, 22 
  NTWT ch1_e1_4_hold, 22 
  NTWT ch1_a0_4_hold, 22 
  NTWT ch1_f1_4_hold, 22 
  NTWT ch1_d1_4_hold, 22 
  NTWT ch1_f1_2_hold, 22 
  NTWT ch1_d1_2_hold, 22 
  NTWT ch1_f1_3_fade, 22 
  NTWT ch1_d1_3_fade, 22 
  NTWT ch1_f1_2_fade, 22 
  NTWT ch1_d1_2_fade, 22 
  NTWT ch1_f1_2_fadein, 22 
  NTWT ch1_d1_2_fade, 22 
  NTWT ch1_f1_2_fade, 22 
  NTWT ch1_d1_2_fade, 22 
  NTWT ch1_a0_3_fade, 22 
  NTWT ch1_cs1_3_hold, 22 
  NTWT ch1_e1_4_hold, 22 
  NTWT ch1_cs1_4_hold, 22 
  NOTE ch1_mute
  RETN
ch1_seq54::
  NOTE ch1_duty25
  NTWT ch1_d0_1_fade, 22 
  NOTE ch1_duty50
  NTWT ch1_d0_4_fade, 154 
  NOTE ch1_duty50
  NTWT ch1_d1_1_fade, 22 
  NTWT ch1_f1_2_fade, 22 
  NTWT ch1_f1_2_fade, 22 
  NTWT ch1_d1_2_fade, 22 
  NOTE ch1_duty50
  NTWT ch1_d1_3_fade, 22 
  NTWT ch1_f1_3_fade, 22 
  NTWT ch1_f1_4_fade, 22 
  NOTE ch1_duty125
  NTWT ch1_d1_2_fade, 22 
  NOTE ch1_duty25
  NTWT ch1_cs0_1_fade, 22 
  NOTE ch1_duty50
  NTWT ch1_cs0_4_fade, 154 
  NOTE ch1_duty50
  NTWT ch1_e1_1_fade, 22 
  NTWT ch1_gs1_2_fade, 22 
  NTWT ch1_gs1_2_fade, 22 
  NTWT ch1_e1_2_fade, 22 
  NOTE ch1_duty50
  NTWT ch1_e1_3_fade, 22 
  NTWT ch1_a1_3_fade, 22 
  NTWT ch1_a1_4_fade, 22 
  NTWT ch1_gs1_1_fade, 22 
  RETN
ch1_seq55::
  NOTE ch1_duty25
  NTWT ch1_d0_1_fade, 22 
  NOTE ch1_duty50
  NTWT ch1_d0_4_fade, 154 
  NOTE ch1_duty50
  NTWT ch1_d1_1_fade, 22 
  NTWT ch1_f1_2_fade, 22 
  NTWT ch1_f1_2_fade, 22 
  NTWT ch1_d1_2_fade, 22 
  NOTE ch1_duty50
  NTWT ch1_d1_3_fade, 22 
  NTWT ch1_f1_3_fade, 22 
  NTWT ch1_f1_4_fade, 22 
  NOTE ch1_duty125
  NTWT ch1_d1_2_fade, 22 
  NOTE ch1_duty25
  NTWT ch1_f0_1_fade, 22 
  NOTE ch1_duty50
  NTWT ch1_f0_4_fade, 154 
  NOTE ch1_duty50
  NTWT ch1_f1_1_fade, 22 
  NTWT ch1_gs1_2_fade, 22 
  NTWT ch1_gs1_2_fade, 22 
  NTWT ch1_f1_2_fade, 22 
  NOTE ch1_duty50
  NTWT ch1_f1_3_fade, 22 
  NTWT ch1_cs2_3_fade, 22 
  NTWT ch1_cs2_4_fade, 22 
  NTWT ch1_c2_1_fade, 22 
  RETN
ch1_seq56::
  NOTE ch1_duty25
  NTWT ch1_g0_1_fade, 22 
  NOTE ch1_duty50
  NTWT ch1_g0_4_fade, 154 
  NOTE ch1_duty50
  NTWT ch1_g1_1_fade, 22 
  NTWT ch1_as1_2_fade, 22 
  NTWT ch1_as1_2_fade, 22 
  NTWT ch1_g1_2_fade, 22 
  NOTE ch1_duty50
  NTWT ch1_g1_3_fade, 22 
  NTWT ch1_as1_3_fade, 22 
  NTWT ch1_as1_4_fade, 22 
  NOTE ch1_duty125
  NTWT ch1_g1_2_fade, 22 
  NOTE ch1_duty25
  NTWT ch1_ds0_1_fade, 22 
  NOTE ch1_duty50
  NTWT ch1_ds0_4_fade, 154 
  NOTE ch1_duty50
  NTWT ch1_g1_1_fade, 22 
  NTWT ch1_as1_2_fade, 22 
  NTWT ch1_as1_2_fade, 22 
  NTWT ch1_cs1_2_fade, 22 
  NOTE ch1_duty50
  NTWT ch1_ds1_3_fade, 22 
  NTWT ch1_as1_3_fade, 22 
  NTWT ch1_as1_4_fade, 22 
  NTWT ch1_g1_1_fade, 22 
  RETN
ch1_seq57::
  NOTE ch1_duty25
  NTWT ch1_a0_1_fade, 22 
  NOTE ch1_duty50
  NTWT ch1_a0_4_fade, 154 
  NOTE ch1_duty50
  NTWT ch1_cs1_1_fade, 22 
  NTWT ch1_a1_2_fade, 22 
  NTWT ch1_a1_2_fade, 22 
  NTWT ch1_e1_2_fade, 22 
  NOTE ch1_duty50
  NTWT ch1_f1_3_fade, 22 
  NTWT ch1_as1_3_fade, 22 
  NTWT ch1_as1_4_fade, 22 
  NOTE ch1_duty50
  NTWT ch1_a1_2_fade, 22 
  NOTE ch1_duty25
  NTWT ch1_as0_1_fade, 22 
  NOTE ch1_duty50
  NTWT ch1_as0_4_fade, 154 
  NOTE ch1_duty50
  NTWT ch1_d1_1_fade, 22 
  NTWT ch1_e1_2_fade, 22 
  NTWT ch1_f1_2_fade, 22 
  NTWT ch1_d1_2_fade, 22 
  NOTE ch1_duty50
  NTWT ch1_cs1_3_fade, 22 
  NTWT ch1_d1_3_fade, 22 
  NTWT ch1_f1_4_fade, 22 
  NTWT ch1_e1_1_fade, 22 
  RETN
ch1_seq59::
  WAIT 36
  NOTE ch1_duty50
  NTWT ch1_a1_3_fadein, 18 
  NTWT ch1_a1_3_hold, 18 
  NTWT ch1_d2_3_fade, 18   ; TODO: vibrato
  NTWT ch1_d2_3_hold, 18 
  NTWT ch1_e2_4_hold, 18 
  NTWT ch1_e2_4_hold, 18 
  NTWT ch1_e2_4_fade, 18 
  NTWT ch1_d2_4_hold, 18 
  NTWT ch1_a1_4_hold, 18 
  NTWT ch1_a1_4_fade, 18 
  NTWT ch1_as1_3_fade, 18   ; TODO: vibrato
  NTWT ch1_as1_3_hold, 18 
  NTWT ch1_d2_3_hold, 18 
  NTWT ch1_d2_3_fade, 18 
  NTWT ch1_g2_4_hold, 18 
  NTWT ch1_g2_4_fade, 18 
  NTWT ch1_as2_4_fadein, 18 
  NTWT ch1_as2_4_hold, 18 
  NTWT ch1_as2_4_fade, 18   ; TODO: vibrato
  NTWT ch1_as2_4_fade, 18 
  NTWT ch1_a2_3_fadein, 18 
  NTWT ch1_g2_3_hold, 18 
  NTWT ch1_fs2_4_fadein, 18 
  NTWT ch1_fs2_4_hold, 18 
  NTWT ch1_fs2_4_fade, 18   ; TODO: vibrato
  NTWT ch1_fs2_4_fade, 90 
  RETN
ch1_seq63::
  WAIT 152
  NOTE ch1_duty125
  NTWT ch1_as0_4_fade, 57   ; TODO: drop
  NOTE ch1_duty50
  NTWT ch1_as0_6_hold, 19 
  NTWT ch1_a0_6_hold, 19 
  NTWT ch1_mute, 57
  NTWT ch1_d0_7_fade, 19   ; TODO: vibrato
  NTWT ch1_d0_7_fade, 19   ; TODO: vibrato
  NTWT ch1_d0_7_fade, 266   ; TODO: vibrato
  RETN

ch2_seq26::
  WAIT 66
  NOTE ch2_duty50
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
ch2_seq27::
  WAIT 66
  NOTE ch2_duty50
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
ch2_seq28::
  WAIT 66
  NOTE ch2_duty50
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
ch2_seq29::
  WAIT 22
  NOTE ch2_duty50
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
ch2_seq36::
  WAIT 22
  NOTE ch2_duty50
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
  NTWT ch2_gs2_1_fadein, 22 
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
ch2_seq40::
  WAIT 22
  NOTE ch2_duty50
  NTWT ch2_a2_1_fadein, 22 
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
  NTWT ch2_gs2_1_fadein, 22 
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
ch2_seq46::
  WAIT 22
  NOTE ch2_duty50
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
  NTWT ch2_as2_1_fadein, 22 
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
ch2_seq47::
  WAIT 22
  NOTE ch2_duty50
  NTWT ch2_a2_1_fadein, 22 
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
  NTWT ch2_gs2_1_fadein, 22 
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
ch2_seq37::
  WAIT 44
  NOTE ch2_duty50
  NTWT ch2_a1_1_fade, 22 
  NTWT ch2_a1_1_fade, 22 
  NTWT ch2_d2_2_fade, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_f2_1_fade, 22 
  NTWT ch2_f2_1_fade, 22 
  NTWT ch2_as2_2_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_a2_2_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_f2_2_fade, 22 
  NTWT ch2_f2_1_fade, 22 
  NTWT ch2_a1_2_fade, 22 
  NTWT ch2_a1_1_fade, 22 
  NTWT ch2_gs1_2_hold, 22 
  NTWT ch2_gs1_2_fade, 22   ; TODO: vibrato
  NTWT ch2_gs1_2_fade, 22 
  NTWT ch2_gs1_1_fade, 110 
  NTWT ch2_e1_1_fadein, 22 
  NTWT ch2_c1_1_hold, 22 
  NTWT ch2_cs1_2_hold, 22 
  NTWT ch2_e1_3_hold, 22 
  NTWT ch2_gs1_3_hold, 22 
  NTWT ch2_a1_2_hold, 22 
  NTWT ch2_gs1_1_fade, 22   ; TODO: vibrato
  NTWT ch2_e1_1_fade, 22 
  RETN
ch2_seq41::
  WAIT 44
  NOTE ch2_duty50
  NTWT ch2_a1_1_fade, 22 
  NTWT ch2_a1_1_fade, 22 
  NTWT ch2_d2_2_fade, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_f2_2_fade, 22 
  NTWT ch2_f2_1_fade, 22 
  NTWT ch2_as2_2_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_f2_2_fade, 22 
  NTWT ch2_f2_1_fade, 22 
  NTWT ch2_d3_1_fade, 22 
  NTWT ch2_d3_1_fade, 22 
  NTWT ch2_cs3_2_hold, 22 
  NTWT ch2_cs3_1_fade, 22   ; TODO: vibrato
  NTWT ch2_cs3_1_fade, 22 
  NTWT ch2_cs3_1_fade, 132 
  NTWT ch2_cs1_2_hold, 22 
  NTWT ch2_f1_3_hold, 22 
  NTWT ch2_gs1_4_hold, 22 
  NTWT ch2_a1_5_hold, 22 
  NTWT ch2_gs1_4_hold, 22 
  NTWT ch2_cs2_3_hold, 22 
  NTWT ch2_f2_1_hold, 22 
  NOTE ch2_mute
  RETN
ch2_seq48::
  WAIT 44
  NOTE ch2_duty50
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_g2_2_fade, 22 
  NTWT ch2_g2_1_fade, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_as2_2_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_g2_1_fade, 22 
  NTWT ch2_g2_1_fade, 22 
  NTWT ch2_cs2_2_hold, 22 
  NTWT ch2_cs2_2_fade, 22   ; TODO: vibrato
  NTWT ch2_d2_2_fade, 22 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_g2_2_hold, 22 
  NTWT ch2_g2_2_fade, 22   ; TODO: vibrato
  NTWT ch2_g2_1_fade, 22 
  NTWT ch2_g2_1_fade, 132 
  NTWT ch2_ds1_1_hold, 22 
  NTWT ch2_g1_2_hold, 22 
  NTWT ch2_as1_3_hold, 22 
  NTWT ch2_a1_3_hold, 22 
  NTWT ch2_as1_3_fade, 22   ; TODO: vibrato
  NTWT ch2_a1_2_hold, 22 
  NTWT ch2_g1_1_fade, 22 
  RETN
ch2_seq49::
  NOTE ch2_duty50
  NTWT ch2_cs2_2_fade, 22 
  NTWT ch2_cs2_1_fade, 22 
  NTWT ch2_as2_2_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_e2_2_fade, 22 
  NTWT ch2_e2_1_fade, 22 
  NTWT ch2_as2_2_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_a2_2_fade, 22 
  NTWT ch2_a2_1_fade, 22 
  NTWT ch2_e2_2_fade, 22   ; TODO: vibrato
  NTWT ch2_e2_1_fade, 22 
  NTWT ch2_f2_2_fade, 22 
  NTWT ch2_f2_1_fade, 22 
  NTWT ch2_gs2_2_hold, 22 
  NTWT ch2_gs2_2_fade, 22   ; TODO: vibrato
  NTWT ch2_gs2_1_fade, 22 
  NTWT ch2_gs2_1_fade, 44 
  NTWT ch2_d2_1_fade, 22 
  NTWT ch2_f2_1_fade, 22 
  NTWT ch2_as2_1_fade, 22 
  NTWT ch2_d3_1_fade, 22   ; TODO: vibrato
  NTWT ch2_d3_1_fade, 22   ; TODO: vibrato
  NTWT ch2_e3_1_hold, 22 
  NTWT ch2_d3_1_fade, 22   ; TODO: arpreggio
  NTWT ch2_cs3_1_hold, 22 
  NTWT ch2_cs3_1_fade, 22   ; TODO: vibrato
  NTWT ch2_cs3_1_hold, 22 
  NTWT ch2_cs3_1_fade, 22 
  RETN
ch2_seq58::
  NOTE ch2_duty50
  NTWT ch2_fs2_2_hold, 18 
  NTWT ch2_fs2_2_hold, 18 
  NTWT ch2_fs2_3_fade, 18   ; TODO: vibrato
  NTWT ch2_a2_4_hold, 18 
  NTWT ch2_d2_4_hold, 18 
  NTWT ch2_fs2_4_hold, 18 
  NTWT ch2_g2_3_hold, 18 
  NTWT ch2_g2_3_fade, 18   ; TODO: vibrato
  NTWT ch2_g2_3_hold, 18 
  NTWT ch2_g2_3_hold, 18 
  NTWT ch2_fs2_2_hold, 18 
  NTWT ch2_fs2_2_hold, 18 
  NTWT ch2_g2_2_hold, 18 
  NTWT ch2_g2_2_hold, 18 
  NTWT ch2_a2_3_hold, 18 
  NTWT ch2_a2_3_hold, 18 
  NTWT ch2_as2_3_hold, 18 
  NTWT ch2_as2_3_fade, 18   ; TODO: vibrato
  NTWT ch2_f3_2_hold, 18 
  NTWT ch2_f3_2_hold, 18 
  NTWT ch2_f3_2_fade, 18   ; TODO: vibrato
  NTWT ch2_f3_2_fade, 18   ; TODO: vibrato
  NTWT ch2_e3_2_hold, 18 
  NTWT ch2_e3_2_hold, 18 
  NTWT ch2_d3_2_hold, 18 
  NTWT ch2_d3_2_hold, 18 
  NTWT ch2_d3_2_fade, 18   ; TODO: vibrato
  NTWT ch2_d3_2_fade, 90   ; TODO: vibrato
  RETN
ch2_seq61::
  NOTE ch2_duty25
  NTWT ch2_cs1_2_hold, 19 
  NTWT ch2_c1_2_hold, 19 
  NTWT ch2_b0_2_hold, 19 
  NTWT ch2_mute, 95
  NOTE ch2_duty50
  NTWT ch2_f3_2_hold, 19 
  NTWT ch2_e3_3_hold, 19 
  NTWT ch2_d3_2_fade, 19   ; TODO: arpreggio
  NTWT ch2_d3_4_fade, 19 
  NTWT ch2_e3_4_hold, 19 
  NTWT ch2_a2_3_hold, 19 
  NTWT ch2_f3_2_hold, 19 
  NTWT ch2_a2_3_fade, 19   ; TODO: vibrato
  NTWT ch2_d3_3_hold, 19 
  NTWT ch2_d3_3_fade, 19   ; TODO: vibrato
  NTWT ch2_d3_3_fade, 266 
  RETN

sfx_seq0::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch2_cs4_6_hold, 2 
  NTWT ch2_cs4_6_hold, 2 
  NTWT ch2_mute, 4
  NTWT ch2_b4_6_hold, 2 
  NTWT ch2_b4_6_hold, 2 
  NTWT ch2_mute, 4
  NTWT ch2_cs4_6_hold, 2 
  NTWT ch2_cs4_6_hold, 2 
  NTWT ch2_mute, 4
  NTWT ch2_a4_6_hold, 2 
  NTWT ch2_a4_6_hold, 2 
  NTWT ch2_mute, 4
  NTWT ch2_cs4_6_hold, 2 
  NTWT ch2_cs4_6_hold, 2 
  NTWT ch2_cs4_6_hold, 2 
  NTWT ch2_cs4_6_hold, 2 
  NTWT ch2_cs4_5_hold, 2 
  NTWT ch2_cs4_5_hold, 2 
  NTWT ch2_mute, 20
  NOFX
sfx_seq1::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch2_cs4_4_hold, 1 
  NTWT ch2_cs4_4_hold, 1 
  NTWT ch2_c4_4_hold, 1 
  NTWT ch2_b3_4_hold, 1 
  NTWT ch2_a3_5_hold, 1 
  NTWT ch2_gs3_5_hold, 1 
  NTWT ch2_fs3_5_hold, 1 
  NTWT ch2_e3_6_hold, 1 
  NTWT ch2_ds3_6_hold, 1 
  NTWT ch2_cs3_6_hold, 1 
  NTWT ch2_b2_7_hold, 1 
  NTWT ch2_a2_7_hold, 1 
  NTWT ch2_fs2_7_hold, 1 
  NTWT ch2_f2_7_hold, 1 
  NTWT ch2_ds2_7_hold, 1 
  NTWT ch2_c2_7_hold, 1 
  NTWT ch2_as1_7_hold, 1 
  NTWT ch2_gs1_7_hold, 1 
  NTWT ch2_fs1_7_hold, 1 
  NTWT ch2_fs1_6_hold, 1 
  NTWT ch2_f1_6_hold, 1 
  NTWT ch2_f1_4_hold, 1 
  NTWT ch2_f1_4_hold, 1 
  NTWT ch2_d1_4_hold, 1 
  NTWT ch2_b0_4_hold, 1 
  NTWT ch2_g0_4_hold, 1 
  NTWT ch2_mute, 6
  NOFX
sfx_seq2::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch2_c3_4_hold, 1 
  NTWT ch2_as3_5_hold, 1 
  NTWT ch2_g3_5_hold, 1 
  NTWT ch2_d3_5_hold, 1 
  NTWT ch2_a2_4_hold, 1 
  NTWT ch2_a2_4_hold, 1 
  NTWT ch2_d2_4_hold, 1 
  NTWT ch2_mute, 25
  NOFX
sfx_seq3::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch2_c3_4_hold, 1 
  NOTE ch2_mute
  NTWT ch2_ds4_5_hold, 1 
  NTWT ch2_ds4_5_hold, 1 
  NTWT ch2_ds4_5_hold, 1 
  NTWT ch2_ds3_4_hold, 1 
  NTWT ch2_as4_4_hold, 1 
  NTWT ch2_as4_4_hold, 1 
  NTWT ch2_mute, 25
  NOFX
sfx_seq4::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch4_a0_5_hold, 3 
  NTWT ch4_f1_6_hold, 3 
  NTWT ch4_f0_5_hold, 3 
  NTWT ch4_f0_4_fade, 24 
  NTWT ch4_cs0_5_hold, 3 
  NTWT ch4_c0_5_hold, 3 
  NTWT ch4_as0_4_hold, 3 
  NTWT ch4_mute, 27
  NTWT ch4_d0_4_hold, 3 
  NTWT ch4_d0_4_hold, 3 
  NTWT ch4_d0_4_fade, 21 
  NOFX
sfx_seq5::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch2_gs1_5_hold, 1 
  NTWT ch2_d2_5_hold, 1 
  NTWT ch2_mute, 30
  NOFX
sfx_seq6::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch2_a2_4_hold, 1 
  NTWT ch2_a2_7_hold, 1 
  NTWT ch2_a1_7_hold, 1 
  NTWT ch2_ds3_6_hold, 1 
  NOTE ch2_mute
  NTWT ch4_ds1_6_hold, 1 
  NTWT ch4_ds1_4_hold, 1 
  NTWT ch4_e2_4_hold, 1 
  NTWT ch4_mute, 1
  NTWT ch4_b1_4_hold, 1 
  NTWT ch4_mute, 1
  NTWT ch4_a1_4_hold, 1 
  NTWT ch4_mute, 1
  NTWT ch4_c1_4_hold, 1 
  NTWT ch4_mute, 19
  NOFX
sfx_seq7::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch4_ds2_4_hold, 1 
  NOTE ch4_mute
  NTWT ch2_fs0_7_hold, 1 
  NTWT ch2_fs0_7_hold, 1 
  NOTE ch2_mute
  NTWT ch4_cs2_6_hold, 1 
  NTWT ch4_c2_6_hold, 1 
  NTWT ch4_f1_4_hold, 1 
  NTWT ch4_d1_4_hold, 1 
  NTWT ch4_c1_4_hold, 1 
  NTWT ch4_a0_4_hold, 1 
  NTWT ch4_gs0_4_hold, 1 
  NTWT ch4_mute, 22
  NOFX
sfx_seq8::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch2_g2_6_hold, 1 
  NTWT ch2_g3_6_hold, 1 
  NTWT ch2_as3_6_hold, 1 
  NTWT ch2_as3_6_hold, 1 
  NTWT ch2_mute, 3
  NTWT ch2_ds3_4_hold, 1 
  NTWT ch2_ds3_4_hold, 1 
  NTWT ch2_e3_4_hold, 1 
  NTWT ch2_fs3_4_hold, 1 
  NTWT ch2_mute, 3
  NTWT ch2_ds3_4_hold, 1 
  NTWT ch2_f3_4_hold, 1 
  NTWT ch2_f3_4_hold, 1 
  NTWT ch2_mute, 4
  NTWT ch2_c3_4_hold, 1 
  NTWT ch2_c3_4_hold, 1 
  NTWT ch2_c3_4_hold, 1 
  NTWT ch2_ds3_4_hold, 1 
  NTWT ch2_f3_4_hold, 1 
  NTWT ch2_mute, 6
  NOFX
sfx_seq9::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch2_g1_5_hold, 1 
  NTWT ch2_g1_5_hold, 1 
  NTWT ch2_g1_7_hold, 1 
  NTWT ch2_g1_7_hold, 1 
  NTWT ch2_f1_6_hold, 1 
  NTWT ch2_fs1_6_hold, 1 
  NTWT ch2_e1_4_hold, 1 
  NTWT ch2_ds1_4_hold, 1 
  NTWT ch2_cs1_4_hold, 1 
  NTWT ch2_cs1_4_hold, 1 
  NTWT ch2_as0_4_hold, 1 
  NTWT ch2_as0_4_hold, 1 
  NTWT ch2_a0_4_hold, 1 
  NTWT ch2_a0_4_hold, 1 
  NTWT ch2_as0_4_hold, 1 
  NTWT ch2_as0_4_hold, 1 
  NTWT ch2_as0_4_hold, 1 
  NTWT ch2_as0_4_hold, 1 
  NTWT ch2_as0_4_hold, 1 
  NTWT ch2_as0_4_hold, 1 
  NTWT ch2_as0_4_hold, 1 
  NTWT ch2_as0_4_hold, 1 
  NTWT ch2_as0_4_hold, 1 
  NTWT ch2_as0_4_hold, 1 
  NTWT ch2_as0_4_hold, 1 
  NTWT ch2_as0_4_hold, 1 
  NTWT ch2_as0_4_hold, 1 
  NTWT ch2_as0_4_hold, 1 
  NTWT ch2_as0_4_hold, 1 
  NTWT ch2_as0_4_hold, 1 
  NTWT ch2_mute, 2
  NOFX
sfx_seq10::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch2_e1_6_hold, 1 
  NTWT ch2_e1_6_hold, 1 
  NTWT ch2_e2_6_hold, 1 
  NTWT ch2_as3_6_hold, 1 
  NTWT ch2_b3_4_hold, 1 
  NTWT ch2_b3_4_hold, 1 
  NTWT ch2_c4_4_hold, 1 
  NTWT ch2_d4_7_hold, 1 
  NTWT ch2_ds4_7_hold, 1 
  NTWT ch2_f4_7_hold, 1 
  NTWT ch2_c5_7_hold, 1 
  NTWT ch2_c5_7_hold, 1 
  NTWT ch2_mute, 20
  NOFX
sfx_seq11::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch2_f4_7_hold, 1 
  NTWT ch2_b3_7_hold, 1 
  NTWT ch2_gs3_7_hold, 1 
  NTWT ch2_fs3_7_hold, 1 
  NTWT ch2_e3_7_hold, 1 
  NTWT ch2_d3_6_hold, 1 
  NTWT ch2_cs3_6_hold, 1 
  NTWT ch2_b2_5_hold, 1 
  NTWT ch2_as2_5_hold, 1 
  NTWT ch2_gs2_5_hold, 1 
  NTWT ch2_fs2_4_hold, 1 
  NTWT ch2_f2_4_hold, 1 
  NTWT ch2_e2_4_hold, 1 
  NTWT ch2_ds2_4_hold, 1 
  NTWT ch2_ds2_4_hold, 1 
  NTWT ch2_d2_4_hold, 1 
  NTWT ch2_d2_4_hold, 1 
  NTWT ch2_cs2_4_hold, 1 
  NTWT ch2_c2_4_hold, 1 
  NTWT ch2_mute, 13
  NOFX
sfx_seq12::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch2_cs1_5_hold, 1 
  NTWT ch2_g1_5_hold, 1 
  NTWT ch2_cs1_4_hold, 1 
  NTWT ch2_mute, 29
  NOFX
sfx_seq13::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch2_a2_6_hold, 1 
  NTWT ch2_ds3_6_hold, 1 
  NTWT ch2_cs3_7_hold, 1 
  NTWT ch2_b2_6_hold, 1 
  NTWT ch2_d2_6_hold, 1 
  NTWT ch2_cs1_4_hold, 1 
  NTWT ch2_mute, 26
  NOFX
sfx_seq14::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch4_b2_7_hold, 3 
  NTWT ch4_gs3_7_hold, 3 
  NTWT ch4_gs2_7_hold, 3 
  NTWT ch4_c5_7_hold, 3 
  NTWT ch4_fs4_7_hold, 3 
  NTWT ch4_e2_7_hold, 3 
  NTWT ch4_cs3_7_hold, 3 
  NTWT ch4_ds2_7_hold, 3 
  NTWT ch4_gs1_7_hold, 3 
  NTWT ch4_d2_7_hold, 3 
  NTWT ch4_ds1_6_hold, 3 
  NTWT ch4_cs1_6_hold, 3 
  NTWT ch4_a1_5_hold, 3 
  NTWT ch4_gs0_4_hold, 3 
  NTWT ch4_fs0_4_hold, 3 
  NTWT ch4_f0_4_hold, 3 
  NTWT ch4_ds0_4_hold, 3 
  NTWT ch4_d0_4_hold, 3 
  NTWT ch4_mute, 42
  NOFX
sfx_seq15::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch2_a1_5_hold, 16 
  NTWT ch2_d2_5_hold, 16 
  NTWT ch2_f2_6_hold, 16 
  NTWT ch2_as2_7_hold, 16 
  NTWT ch2_a2_7_hold, 16 
  NTWT ch2_f2_7_hold, 16 
  NTWT ch2_a1_6_hold, 16 
  NTWT ch2_gs1_6_hold, 16 
  NTWT ch2_gs1_5_hold, 16 
  NTWT ch2_mute, 368
  NOFX
sfx_seq16::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch4_gs2_6_hold, 2 
  NTWT ch4_f2_6_hold, 2 
  NTWT ch4_f2_5_hold, 2 
  NTWT ch4_mute, 6
  NTWT ch4_ds2_6_hold, 2 
  NTWT ch4_ds2_6_hold, 2 
  NTWT ch4_ds2_5_hold, 2 
  NTWT ch4_mute, 6
  NTWT ch4_e2_5_hold, 2 
  NTWT ch4_ds2_5_hold, 2 
  NTWT ch4_ds2_5_hold, 2 
  NTWT ch4_mute, 6
  NTWT ch4_f2_4_hold, 2 
  NTWT ch4_ds2_4_hold, 2 
  NTWT ch4_cs2_4_hold, 2 
  NTWT ch4_mute, 22
  NOFX
sfx_seq17::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch4_ds4_7_hold, 2 
  NTWT ch4_gs4_6_hold, 2 
  NTWT ch4_c5_5_hold, 2 
  NTWT ch4_d5_4_hold, 2 
  NTWT ch4_d5_4_hold, 2 
  NTWT ch4_d5_4_fade, 2 
  NTWT ch4_d5_4_fade, 52 
  NOFX
sfx_seq18::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch2_as2_4_hold, 2 
  NTWT ch2_d3_4_hold, 2 
  NOTE ch2_mute
  NTWT ch4_e2_7_hold, 2 
  NTWT ch4_mute, 2
  NTWT ch4_d2_5_hold, 2 
  NTWT ch4_mute, 6
  NTWT ch4_d1_4_hold, 2 
  NTWT ch4_mute, 46
  NOFX
sfx_seq19::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch4_ds0_6_hold, 2 
  NTWT ch4_mute, 2
  NTWT ch4_fs0_5_hold, 2 
  NOTE ch4_mute
  NTWT ch2_c3_4_hold, 2 
  NTWT ch2_mute, 2
  NTWT ch2_g3_4_hold, 2 
  NTWT ch2_c5_4_hold, 2 
  NTWT ch2_mute, 2
  NTWT ch4_fs0_5_hold, 2 
  NTWT ch4_mute, 2
  NTWT ch4_d0_5_hold, 2 
  NTWT ch4_d0_4_hold, 2 
  NTWT ch4_mute, 40
  NOFX
sfx_seq20::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch2_e2_6_hold, 2 
  NTWT ch2_e2_6_hold, 2 
  NTWT ch2_mute, 4
  NTWT ch2_cs3_6_hold, 2 
  NTWT ch2_cs3_6_hold, 2 
  NTWT ch2_mute, 4
  NTWT ch2_cs4_6_hold, 2 
  NTWT ch2_cs4_6_hold, 2 
  NTWT ch2_mute, 4
  NTWT ch2_a4_6_hold, 2 
  NTWT ch2_a4_6_hold, 2 
  NTWT ch2_mute, 4
  NTWT ch2_a4_6_hold, 2 
  NTWT ch2_a4_6_hold, 2 
  NTWT ch2_a4_6_hold, 2 
  NTWT ch2_a4_6_hold, 2 
  NTWT ch2_a4_5_hold, 2 
  NTWT ch2_a4_5_hold, 2 
  NTWT ch2_mute, 6
  NTWT ch2_a4_5_hold, 2 
  NTWT ch2_a4_5_hold, 2 
  NTWT ch2_a4_5_hold, 2 
  NTWT ch2_a4_4_hold, 2 
  NTWT ch2_a4_4_hold, 2 
  NTWT ch2_a4_4_hold, 2 
  NTWT ch2_mute, 2
  NOFX
sfx_seq21::
  NOTE ch2_mute
  NOTE ch4_mute
  NTWT ch2_c3_4_hold, 1 
  NOTE ch2_mute
  NTWT ch2_ds4_5_hold, 1 
  NTWT ch2_ds4_5_hold, 1 
  NTWT ch2_ds4_5_hold, 1 
  NTWT ch2_ds3_4_hold, 1 
  NTWT ch2_as4_4_hold, 1 
  NTWT ch2_as4_4_hold, 1 
  NTWT ch2_mute, 1
  NTWT ch2_b1_7_hold, 1 
  NTWT ch2_g1_7_hold, 1 
  NTWT ch2_f1_6_hold, 1 
  NTWT ch2_d1_4_hold, 1 
  NTWT ch2_c1_4_hold, 1 
  NTWT ch2_as0_4_hold, 1 
  NTWT ch2_a0_4_hold, 1 
  NTWT ch2_a0_4_hold, 1 
  NTWT ch2_a0_4_hold, 1 
  NTWT ch2_a0_4_hold, 1 
  NTWT ch2_a0_4_hold, 1 
  NTWT ch2_a0_4_hold, 1 
  NTWT ch2_a0_4_hold, 1 
  NTWT ch2_a0_4_hold, 1 
  NTWT ch2_a0_4_hold, 1 
  NTWT ch2_a0_4_hold, 1 
  NTWT ch2_a0_4_hold, 1 
  NTWT ch2_a0_4_hold, 1 
  NTWT ch2_a0_4_hold, 1 
  NTWT ch2_fs0_4_hold, 1 
  NTWT ch2_ds0_4_hold, 1 
  NTWT ch2_cs0_4_hold, 1 
  NTWT ch2_mute, 2
  NOFX

CH12_NOTE    ch12_cs0 0x94 0x80
CH1_NOTE_ENV ch1_cs0_1_fade ENV_1FADE ch12_cs0
CH1_NOTE_ENV ch1_cs0_2_fade ENV_2FADE ch12_cs0
CH1_NOTE_ENV ch1_cs0_4_hold ENV_4HOLD ch12_cs0
CH2_NOTE_ENV ch2_cs0_4_hold ENV_4HOLD ch12_cs0
CH1_NOTE_ENV ch1_cs0_4_fade ENV_4FADE ch12_cs0
CH12_NOTE    ch12_d0  0x05 0x81
CH1_NOTE_ENV ch1_d0_1_fade  ENV_1FADE ch12_d0
CH1_NOTE_ENV ch1_d0_3_fade  ENV_3FADE ch12_d0
CH1_NOTE_ENV ch1_d0_4_hold  ENV_4HOLD ch12_d0
CH1_NOTE_ENV ch1_d0_4_fade  ENV_4FADE ch12_d0
CH1_NOTE_ENV ch1_d0_7_fade  ENV_7FADE ch12_d0
CH12_NOTE    ch12_ds0 0x6a 0x81
CH1_NOTE_ENV ch1_ds0_1_fade ENV_1FADE ch12_ds0
CH2_NOTE_ENV ch2_ds0_4_hold ENV_4HOLD ch12_ds0
CH1_NOTE_ENV ch1_ds0_4_fade ENV_4FADE ch12_ds0
CH12_NOTE    ch12_e0  0xc3 0x81
CH1_NOTE_ENV ch1_e0_2_fadein ENV_2FADEIN ch12_e0
CH12_NOTE    ch12_f0  0x20 0x82
CH1_NOTE_ENV ch1_f0_1_fade  ENV_1FADE ch12_f0
CH1_NOTE_ENV ch1_f0_2_fadein ENV_2FADEIN ch12_f0
CH1_NOTE_ENV ch1_f0_4_fade  ENV_4FADE ch12_f0
CH12_NOTE    ch12_fs0 0x72 0x82
CH2_NOTE_ENV ch2_fs0_4_hold ENV_4HOLD ch12_fs0
CH2_NOTE_ENV ch2_fs0_7_hold ENV_7HOLD ch12_fs0
CH12_NOTE    ch12_g0  0xc5 0x82
CH1_NOTE_ENV ch1_g0_1_fade  ENV_1FADE ch12_g0
CH2_NOTE_ENV ch2_g0_4_hold  ENV_4HOLD ch12_g0
CH1_NOTE_ENV ch1_g0_4_fade  ENV_4FADE ch12_g0
CH12_NOTE    ch12_gs0 0x0f 0x83
CH1_NOTE_ENV ch1_gs0_3_fade ENV_3FADE ch12_gs0
CH1_NOTE_ENV ch1_gs0_4_hold ENV_4HOLD ch12_gs0
CH12_NOTE    ch12_a0  0x55 0x83
CH1_NOTE_ENV ch1_a0_1_fadein ENV_1FADEIN ch12_a0
CH1_NOTE_ENV ch1_a0_1_fade  ENV_1FADE ch12_a0
CH1_NOTE_ENV ch1_a0_2_hold  ENV_2HOLD ch12_a0
CH1_NOTE_ENV ch1_a0_2_fadein ENV_2FADEIN ch12_a0
CH1_NOTE_ENV ch1_a0_2_fade  ENV_2FADE ch12_a0
CH1_NOTE_ENV ch1_a0_3_hold  ENV_3HOLD ch12_a0
CH1_NOTE_ENV ch1_a0_3_fade  ENV_3FADE ch12_a0
CH1_NOTE_ENV ch1_a0_4_hold  ENV_4HOLD ch12_a0
CH2_NOTE_ENV ch2_a0_4_hold  ENV_4HOLD ch12_a0
CH1_NOTE_ENV ch1_a0_4_fade  ENV_4FADE ch12_a0
CH1_NOTE_ENV ch1_a0_6_hold  ENV_6HOLD ch12_a0
CH12_NOTE    ch12_as0 0x9a 0x83
CH1_NOTE_ENV ch1_as0_1_fade ENV_1FADE ch12_as0
CH1_NOTE_ENV ch1_as0_2_fadein ENV_2FADEIN ch12_as0
CH1_NOTE_ENV ch1_as0_3_fade ENV_3FADE ch12_as0
CH1_NOTE_ENV ch1_as0_4_hold ENV_4HOLD ch12_as0
CH2_NOTE_ENV ch2_as0_4_hold ENV_4HOLD ch12_as0
CH1_NOTE_ENV ch1_as0_4_fade ENV_4FADE ch12_as0
CH1_NOTE_ENV ch1_as0_6_hold ENV_6HOLD ch12_as0
CH12_NOTE    ch12_b0  0xd5 0x83
CH2_NOTE_ENV ch2_b0_2_hold  ENV_2HOLD ch12_b0
CH2_NOTE_ENV ch2_b0_4_hold  ENV_4HOLD ch12_b0
CH12_NOTE    ch12_c1  0x14 0x84
CH1_NOTE_ENV ch1_c1_1_hold  ENV_1HOLD ch12_c1
CH2_NOTE_ENV ch2_c1_1_hold  ENV_1HOLD ch12_c1
CH2_NOTE_ENV ch2_c1_2_hold  ENV_2HOLD ch12_c1
CH2_NOTE_ENV ch2_c1_4_hold  ENV_4HOLD ch12_c1
CH12_NOTE    ch12_cs1 0x4c 0x84
CH2_NOTE_ENV ch2_cs1_1_hold ENV_1HOLD ch12_cs1
CH1_NOTE_ENV ch1_cs1_1_fade ENV_1FADE ch12_cs1
CH2_NOTE_ENV ch2_cs1_1_fade ENV_1FADE ch12_cs1
CH1_NOTE_ENV ch1_cs1_2_hold ENV_2HOLD ch12_cs1
CH2_NOTE_ENV ch2_cs1_2_hold ENV_2HOLD ch12_cs1
CH1_NOTE_ENV ch1_cs1_2_fade ENV_2FADE ch12_cs1
CH1_NOTE_ENV ch1_cs1_3_hold ENV_3HOLD ch12_cs1
CH1_NOTE_ENV ch1_cs1_3_fade ENV_3FADE ch12_cs1
CH1_NOTE_ENV ch1_cs1_4_hold ENV_4HOLD ch12_cs1
CH2_NOTE_ENV ch2_cs1_4_hold ENV_4HOLD ch12_cs1
CH2_NOTE_ENV ch2_cs1_5_hold ENV_5HOLD ch12_cs1
CH12_NOTE    ch12_d1  0x82 0x84
CH1_NOTE_ENV ch1_d1_1_fade  ENV_1FADE ch12_d1
CH2_NOTE_ENV ch2_d1_1_fade  ENV_1FADE ch12_d1
CH1_NOTE_ENV ch1_d1_2_hold  ENV_2HOLD ch12_d1
CH1_NOTE_ENV ch1_d1_2_fade  ENV_2FADE ch12_d1
CH1_NOTE_ENV ch1_d1_3_hold  ENV_3HOLD ch12_d1
CH1_NOTE_ENV ch1_d1_3_fade  ENV_3FADE ch12_d1
CH1_NOTE_ENV ch1_d1_4_hold  ENV_4HOLD ch12_d1
CH2_NOTE_ENV ch2_d1_4_hold  ENV_4HOLD ch12_d1
CH1_NOTE_ENV ch1_d1_4_fade  ENV_4FADE ch12_d1
CH12_NOTE    ch12_ds1 0xb5 0x84
CH1_NOTE_ENV ch1_ds1_1_hold ENV_1HOLD ch12_ds1
CH2_NOTE_ENV ch2_ds1_1_hold ENV_1HOLD ch12_ds1
CH2_NOTE_ENV ch2_ds1_1_fade ENV_1FADE ch12_ds1
CH1_NOTE_ENV ch1_ds1_2_hold ENV_2HOLD ch12_ds1
CH1_NOTE_ENV ch1_ds1_2_fade ENV_2FADE ch12_ds1
CH1_NOTE_ENV ch1_ds1_3_hold ENV_3HOLD ch12_ds1
CH1_NOTE_ENV ch1_ds1_3_fade ENV_3FADE ch12_ds1
CH1_NOTE_ENV ch1_ds1_4_hold ENV_4HOLD ch12_ds1
CH2_NOTE_ENV ch2_ds1_4_hold ENV_4HOLD ch12_ds1
CH12_NOTE    ch12_e1  0xe3 0x84
CH2_NOTE_ENV ch2_e1_1_hold  ENV_1HOLD ch12_e1
CH1_NOTE_ENV ch1_e1_1_fadein ENV_1FADEIN ch12_e1
CH2_NOTE_ENV ch2_e1_1_fadein ENV_1FADEIN ch12_e1
CH1_NOTE_ENV ch1_e1_1_fade  ENV_1FADE ch12_e1
CH2_NOTE_ENV ch2_e1_1_fade  ENV_1FADE ch12_e1
CH1_NOTE_ENV ch1_e1_2_hold  ENV_2HOLD ch12_e1
CH1_NOTE_ENV ch1_e1_2_fadein ENV_2FADEIN ch12_e1
CH1_NOTE_ENV ch1_e1_2_fade  ENV_2FADE ch12_e1
CH1_NOTE_ENV ch1_e1_3_hold  ENV_3HOLD ch12_e1
CH2_NOTE_ENV ch2_e1_3_hold  ENV_3HOLD ch12_e1
CH1_NOTE_ENV ch1_e1_3_fade  ENV_3FADE ch12_e1
CH1_NOTE_ENV ch1_e1_4_hold  ENV_4HOLD ch12_e1
CH2_NOTE_ENV ch2_e1_4_hold  ENV_4HOLD ch12_e1
CH1_NOTE_ENV ch1_e1_4_fade  ENV_4FADE ch12_e1
CH2_NOTE_ENV ch2_e1_6_hold  ENV_6HOLD ch12_e1
CH12_NOTE    ch12_f1  0x10 0x85
CH2_NOTE_ENV ch2_f1_1_hold  ENV_1HOLD ch12_f1
CH1_NOTE_ENV ch1_f1_1_fade  ENV_1FADE ch12_f1
CH2_NOTE_ENV ch2_f1_1_fade  ENV_1FADE ch12_f1
CH1_NOTE_ENV ch1_f1_2_hold  ENV_2HOLD ch12_f1
CH1_NOTE_ENV ch1_f1_2_fadein ENV_2FADEIN ch12_f1
CH1_NOTE_ENV ch1_f1_2_fade  ENV_2FADE ch12_f1
CH1_NOTE_ENV ch1_f1_3_hold  ENV_3HOLD ch12_f1
CH2_NOTE_ENV ch2_f1_3_hold  ENV_3HOLD ch12_f1
CH1_NOTE_ENV ch1_f1_3_fade  ENV_3FADE ch12_f1
CH1_NOTE_ENV ch1_f1_4_hold  ENV_4HOLD ch12_f1
CH2_NOTE_ENV ch2_f1_4_hold  ENV_4HOLD ch12_f1
CH1_NOTE_ENV ch1_f1_4_fade  ENV_4FADE ch12_f1
CH2_NOTE_ENV ch2_f1_6_hold  ENV_6HOLD ch12_f1
CH12_NOTE    ch12_fs1 0x3a 0x85
CH2_NOTE_ENV ch2_fs1_6_hold ENV_6HOLD ch12_fs1
CH2_NOTE_ENV ch2_fs1_7_hold ENV_7HOLD ch12_fs1
CH12_NOTE    ch12_g1  0x63 0x85
CH2_NOTE_ENV ch2_g1_1_hold  ENV_1HOLD ch12_g1
CH1_NOTE_ENV ch1_g1_1_fade  ENV_1FADE ch12_g1
CH2_NOTE_ENV ch2_g1_1_fade  ENV_1FADE ch12_g1
CH1_NOTE_ENV ch1_g1_2_hold  ENV_2HOLD ch12_g1
CH2_NOTE_ENV ch2_g1_2_hold  ENV_2HOLD ch12_g1
CH1_NOTE_ENV ch1_g1_2_fadein ENV_2FADEIN ch12_g1
CH1_NOTE_ENV ch1_g1_2_fade  ENV_2FADE ch12_g1
CH1_NOTE_ENV ch1_g1_3_hold  ENV_3HOLD ch12_g1
CH1_NOTE_ENV ch1_g1_3_fade  ENV_3FADE ch12_g1
CH1_NOTE_ENV ch1_g1_4_hold  ENV_4HOLD ch12_g1
CH2_NOTE_ENV ch2_g1_5_hold  ENV_5HOLD ch12_g1
CH2_NOTE_ENV ch2_g1_7_hold  ENV_7HOLD ch12_g1
CH12_NOTE    ch12_gs1 0x89 0x85
CH2_NOTE_ENV ch2_gs1_1_hold ENV_1HOLD ch12_gs1
CH1_NOTE_ENV ch1_gs1_1_fade ENV_1FADE ch12_gs1
CH2_NOTE_ENV ch2_gs1_1_fade ENV_1FADE ch12_gs1
CH1_NOTE_ENV ch1_gs1_2_hold ENV_2HOLD ch12_gs1
CH2_NOTE_ENV ch2_gs1_2_hold ENV_2HOLD ch12_gs1
CH1_NOTE_ENV ch1_gs1_2_fade ENV_2FADE ch12_gs1
CH2_NOTE_ENV ch2_gs1_2_fade ENV_2FADE ch12_gs1
CH1_NOTE_ENV ch1_gs1_3_hold ENV_3HOLD ch12_gs1
CH2_NOTE_ENV ch2_gs1_3_hold ENV_3HOLD ch12_gs1
CH1_NOTE_ENV ch1_gs1_4_hold ENV_4HOLD ch12_gs1
CH2_NOTE_ENV ch2_gs1_4_hold ENV_4HOLD ch12_gs1
CH2_NOTE_ENV ch2_gs1_5_hold ENV_5HOLD ch12_gs1
CH2_NOTE_ENV ch2_gs1_6_hold ENV_6HOLD ch12_gs1
CH2_NOTE_ENV ch2_gs1_7_hold ENV_7HOLD ch12_gs1
CH12_NOTE    ch12_a1  0xab 0x85
CH2_NOTE_ENV ch2_a1_1_hold  ENV_1HOLD ch12_a1
CH1_NOTE_ENV ch1_a1_1_fade  ENV_1FADE ch12_a1
CH2_NOTE_ENV ch2_a1_1_fade  ENV_1FADE ch12_a1
CH1_NOTE_ENV ch1_a1_2_hold  ENV_2HOLD ch12_a1
CH2_NOTE_ENV ch2_a1_2_hold  ENV_2HOLD ch12_a1
CH1_NOTE_ENV ch1_a1_2_fade  ENV_2FADE ch12_a1
CH2_NOTE_ENV ch2_a1_2_fade  ENV_2FADE ch12_a1
CH1_NOTE_ENV ch1_a1_3_hold  ENV_3HOLD ch12_a1
CH2_NOTE_ENV ch2_a1_3_hold  ENV_3HOLD ch12_a1
CH1_NOTE_ENV ch1_a1_3_fadein ENV_3FADEIN ch12_a1
CH1_NOTE_ENV ch1_a1_3_fade  ENV_3FADE ch12_a1
CH1_NOTE_ENV ch1_a1_4_hold  ENV_4HOLD ch12_a1
CH1_NOTE_ENV ch1_a1_4_fade  ENV_4FADE ch12_a1
CH1_NOTE_ENV ch1_a1_5_hold  ENV_5HOLD ch12_a1
CH2_NOTE_ENV ch2_a1_5_hold  ENV_5HOLD ch12_a1
CH2_NOTE_ENV ch2_a1_6_hold  ENV_6HOLD ch12_a1
CH2_NOTE_ENV ch2_a1_7_hold  ENV_7HOLD ch12_a1
CH12_NOTE    ch12_as1 0xcd 0x85
CH2_NOTE_ENV ch2_as1_1_hold ENV_1HOLD ch12_as1
CH1_NOTE_ENV ch1_as1_1_fade ENV_1FADE ch12_as1
CH2_NOTE_ENV ch2_as1_1_fade ENV_1FADE ch12_as1
CH1_NOTE_ENV ch1_as1_2_fade ENV_2FADE ch12_as1
CH1_NOTE_ENV ch1_as1_3_hold ENV_3HOLD ch12_as1
CH2_NOTE_ENV ch2_as1_3_hold ENV_3HOLD ch12_as1
CH1_NOTE_ENV ch1_as1_3_fade ENV_3FADE ch12_as1
CH2_NOTE_ENV ch2_as1_3_fade ENV_3FADE ch12_as1
CH1_NOTE_ENV ch1_as1_4_fade ENV_4FADE ch12_as1
CH2_NOTE_ENV ch2_as1_7_hold ENV_7HOLD ch12_as1
CH12_NOTE    ch12_b1  0xeb 0x85
CH2_NOTE_ENV ch2_b1_7_hold  ENV_7HOLD ch12_b1
CH12_NOTE    ch12_c2  0x0b 0x86
CH1_NOTE_ENV ch1_c2_1_fade  ENV_1FADE ch12_c2
CH2_NOTE_ENV ch2_c2_1_fade  ENV_1FADE ch12_c2
CH1_NOTE_ENV ch1_c2_2_fade  ENV_2FADE ch12_c2
CH2_NOTE_ENV ch2_c2_4_hold  ENV_4HOLD ch12_c2
CH2_NOTE_ENV ch2_c2_7_hold  ENV_7HOLD ch12_c2
CH12_NOTE    ch12_cs2 0x27 0x86
CH2_NOTE_ENV ch2_cs2_1_hold ENV_1HOLD ch12_cs2
CH1_NOTE_ENV ch1_cs2_1_fade ENV_1FADE ch12_cs2
CH2_NOTE_ENV ch2_cs2_1_fade ENV_1FADE ch12_cs2
CH1_NOTE_ENV ch1_cs2_2_hold ENV_2HOLD ch12_cs2
CH2_NOTE_ENV ch2_cs2_2_hold ENV_2HOLD ch12_cs2
CH1_NOTE_ENV ch1_cs2_2_fade ENV_2FADE ch12_cs2
CH2_NOTE_ENV ch2_cs2_2_fade ENV_2FADE ch12_cs2
CH1_NOTE_ENV ch1_cs2_3_hold ENV_3HOLD ch12_cs2
CH2_NOTE_ENV ch2_cs2_3_hold ENV_3HOLD ch12_cs2
CH1_NOTE_ENV ch1_cs2_3_fade ENV_3FADE ch12_cs2
CH2_NOTE_ENV ch2_cs2_4_hold ENV_4HOLD ch12_cs2
CH1_NOTE_ENV ch1_cs2_4_fade ENV_4FADE ch12_cs2
CH12_NOTE    ch12_d2  0x41 0x86
CH1_NOTE_ENV ch1_d2_1_fade  ENV_1FADE ch12_d2
CH2_NOTE_ENV ch2_d2_1_fade  ENV_1FADE ch12_d2
CH1_NOTE_ENV ch1_d2_2_fade  ENV_2FADE ch12_d2
CH2_NOTE_ENV ch2_d2_2_fade  ENV_2FADE ch12_d2
CH1_NOTE_ENV ch1_d2_3_hold  ENV_3HOLD ch12_d2
CH1_NOTE_ENV ch1_d2_3_fade  ENV_3FADE ch12_d2
CH1_NOTE_ENV ch1_d2_4_hold  ENV_4HOLD ch12_d2
CH2_NOTE_ENV ch2_d2_4_hold  ENV_4HOLD ch12_d2
CH2_NOTE_ENV ch2_d2_5_hold  ENV_5HOLD ch12_d2
CH2_NOTE_ENV ch2_d2_6_hold  ENV_6HOLD ch12_d2
CH12_NOTE    ch12_ds2 0x5a 0x86
CH1_NOTE_ENV ch1_ds2_1_fade ENV_1FADE ch12_ds2
CH2_NOTE_ENV ch2_ds2_1_fade ENV_1FADE ch12_ds2
CH1_NOTE_ENV ch1_ds2_2_fade ENV_2FADE ch12_ds2
CH2_NOTE_ENV ch2_ds2_4_hold ENV_4HOLD ch12_ds2
CH2_NOTE_ENV ch2_ds2_7_hold ENV_7HOLD ch12_ds2
CH12_NOTE    ch12_e2  0x72 0x86
CH1_NOTE_ENV ch1_e2_1_hold  ENV_1HOLD ch12_e2
CH2_NOTE_ENV ch2_e2_1_hold  ENV_1HOLD ch12_e2
CH1_NOTE_ENV ch1_e2_1_fade  ENV_1FADE ch12_e2
CH2_NOTE_ENV ch2_e2_1_fade  ENV_1FADE ch12_e2
CH1_NOTE_ENV ch1_e2_2_hold  ENV_2HOLD ch12_e2
CH1_NOTE_ENV ch1_e2_2_fade  ENV_2FADE ch12_e2
CH2_NOTE_ENV ch2_e2_2_fade  ENV_2FADE ch12_e2
CH1_NOTE_ENV ch1_e2_3_fade  ENV_3FADE ch12_e2
CH1_NOTE_ENV ch1_e2_4_hold  ENV_4HOLD ch12_e2
CH2_NOTE_ENV ch2_e2_4_hold  ENV_4HOLD ch12_e2
CH1_NOTE_ENV ch1_e2_4_fade  ENV_4FADE ch12_e2
CH2_NOTE_ENV ch2_e2_6_hold  ENV_6HOLD ch12_e2
CH12_NOTE    ch12_f2  0x88 0x86
CH1_NOTE_ENV ch1_f2_1_hold  ENV_1HOLD ch12_f2
CH2_NOTE_ENV ch2_f2_1_hold  ENV_1HOLD ch12_f2
CH1_NOTE_ENV ch1_f2_1_fade  ENV_1FADE ch12_f2
CH2_NOTE_ENV ch2_f2_1_fade  ENV_1FADE ch12_f2
CH1_NOTE_ENV ch1_f2_2_hold  ENV_2HOLD ch12_f2
CH1_NOTE_ENV ch1_f2_2_fade  ENV_2FADE ch12_f2
CH2_NOTE_ENV ch2_f2_2_fade  ENV_2FADE ch12_f2
CH2_NOTE_ENV ch2_f2_4_hold  ENV_4HOLD ch12_f2
CH2_NOTE_ENV ch2_f2_6_hold  ENV_6HOLD ch12_f2
CH2_NOTE_ENV ch2_f2_7_hold  ENV_7HOLD ch12_f2
CH12_NOTE    ch12_fs2 0x9e 0x86
CH2_NOTE_ENV ch2_fs2_2_hold ENV_2HOLD ch12_fs2
CH2_NOTE_ENV ch2_fs2_3_fade ENV_3FADE ch12_fs2
CH1_NOTE_ENV ch1_fs2_4_hold ENV_4HOLD ch12_fs2
CH2_NOTE_ENV ch2_fs2_4_hold ENV_4HOLD ch12_fs2
CH1_NOTE_ENV ch1_fs2_4_fadein ENV_4FADEIN ch12_fs2
CH1_NOTE_ENV ch1_fs2_4_fade ENV_4FADE ch12_fs2
CH2_NOTE_ENV ch2_fs2_7_hold ENV_7HOLD ch12_fs2
CH12_NOTE    ch12_g2  0xb2 0x86
CH1_NOTE_ENV ch1_g2_1_hold  ENV_1HOLD ch12_g2
CH2_NOTE_ENV ch2_g2_1_hold  ENV_1HOLD ch12_g2
CH1_NOTE_ENV ch1_g2_1_fade  ENV_1FADE ch12_g2
CH2_NOTE_ENV ch2_g2_1_fade  ENV_1FADE ch12_g2
CH1_NOTE_ENV ch1_g2_2_hold  ENV_2HOLD ch12_g2
CH2_NOTE_ENV ch2_g2_2_hold  ENV_2HOLD ch12_g2
CH1_NOTE_ENV ch1_g2_2_fade  ENV_2FADE ch12_g2
CH2_NOTE_ENV ch2_g2_2_fade  ENV_2FADE ch12_g2
CH1_NOTE_ENV ch1_g2_3_hold  ENV_3HOLD ch12_g2
CH2_NOTE_ENV ch2_g2_3_hold  ENV_3HOLD ch12_g2
CH2_NOTE_ENV ch2_g2_3_fade  ENV_3FADE ch12_g2
CH1_NOTE_ENV ch1_g2_4_hold  ENV_4HOLD ch12_g2
CH1_NOTE_ENV ch1_g2_4_fade  ENV_4FADE ch12_g2
CH2_NOTE_ENV ch2_g2_6_hold  ENV_6HOLD ch12_g2
CH12_NOTE    ch12_gs2 0xc4 0x86
CH2_NOTE_ENV ch2_gs2_1_hold ENV_1HOLD ch12_gs2
CH1_NOTE_ENV ch1_gs2_1_fadein ENV_1FADEIN ch12_gs2
CH2_NOTE_ENV ch2_gs2_1_fadein ENV_1FADEIN ch12_gs2
CH1_NOTE_ENV ch1_gs2_1_fade ENV_1FADE ch12_gs2
CH2_NOTE_ENV ch2_gs2_1_fade ENV_1FADE ch12_gs2
CH1_NOTE_ENV ch1_gs2_2_hold ENV_2HOLD ch12_gs2
CH2_NOTE_ENV ch2_gs2_2_hold ENV_2HOLD ch12_gs2
CH1_NOTE_ENV ch1_gs2_2_fade ENV_2FADE ch12_gs2
CH2_NOTE_ENV ch2_gs2_2_fade ENV_2FADE ch12_gs2
CH1_NOTE_ENV ch1_gs2_3_fade ENV_3FADE ch12_gs2
CH2_NOTE_ENV ch2_gs2_5_hold ENV_5HOLD ch12_gs2
CH12_NOTE    ch12_a2  0xd6 0x86
CH1_NOTE_ENV ch1_a2_1_hold  ENV_1HOLD ch12_a2
CH2_NOTE_ENV ch2_a2_1_hold  ENV_1HOLD ch12_a2
CH1_NOTE_ENV ch1_a2_1_fadein ENV_1FADEIN ch12_a2
CH2_NOTE_ENV ch2_a2_1_fadein ENV_1FADEIN ch12_a2
CH1_NOTE_ENV ch1_a2_1_fade  ENV_1FADE ch12_a2
CH2_NOTE_ENV ch2_a2_1_fade  ENV_1FADE ch12_a2
CH1_NOTE_ENV ch1_a2_2_hold  ENV_2HOLD ch12_a2
CH1_NOTE_ENV ch1_a2_2_fade  ENV_2FADE ch12_a2
CH2_NOTE_ENV ch2_a2_2_fade  ENV_2FADE ch12_a2
CH2_NOTE_ENV ch2_a2_3_hold  ENV_3HOLD ch12_a2
CH1_NOTE_ENV ch1_a2_3_fadein ENV_3FADEIN ch12_a2
CH1_NOTE_ENV ch1_a2_3_fade  ENV_3FADE ch12_a2
CH2_NOTE_ENV ch2_a2_3_fade  ENV_3FADE ch12_a2
CH2_NOTE_ENV ch2_a2_4_hold  ENV_4HOLD ch12_a2
CH2_NOTE_ENV ch2_a2_6_hold  ENV_6HOLD ch12_a2
CH2_NOTE_ENV ch2_a2_7_hold  ENV_7HOLD ch12_a2
CH12_NOTE    ch12_as2 0xe7 0x86
CH1_NOTE_ENV ch1_as2_1_hold ENV_1HOLD ch12_as2
CH2_NOTE_ENV ch2_as2_1_hold ENV_1HOLD ch12_as2
CH1_NOTE_ENV ch1_as2_1_fadein ENV_1FADEIN ch12_as2
CH2_NOTE_ENV ch2_as2_1_fadein ENV_1FADEIN ch12_as2
CH1_NOTE_ENV ch1_as2_1_fade ENV_1FADE ch12_as2
CH2_NOTE_ENV ch2_as2_1_fade ENV_1FADE ch12_as2
CH1_NOTE_ENV ch1_as2_2_hold ENV_2HOLD ch12_as2
CH1_NOTE_ENV ch1_as2_2_fade ENV_2FADE ch12_as2
CH2_NOTE_ENV ch2_as2_2_fade ENV_2FADE ch12_as2
CH2_NOTE_ENV ch2_as2_3_hold ENV_3HOLD ch12_as2
CH2_NOTE_ENV ch2_as2_3_fade ENV_3FADE ch12_as2
CH1_NOTE_ENV ch1_as2_4_hold ENV_4HOLD ch12_as2
CH2_NOTE_ENV ch2_as2_4_hold ENV_4HOLD ch12_as2
CH1_NOTE_ENV ch1_as2_4_fadein ENV_4FADEIN ch12_as2
CH1_NOTE_ENV ch1_as2_4_fade ENV_4FADE ch12_as2
CH2_NOTE_ENV ch2_as2_5_hold ENV_5HOLD ch12_as2
CH2_NOTE_ENV ch2_as2_7_hold ENV_7HOLD ch12_as2
CH12_NOTE    ch12_b2  0xf6 0x86
CH2_NOTE_ENV ch2_b2_5_hold  ENV_5HOLD ch12_b2
CH2_NOTE_ENV ch2_b2_6_hold  ENV_6HOLD ch12_b2
CH2_NOTE_ENV ch2_b2_7_hold  ENV_7HOLD ch12_b2
CH12_NOTE    ch12_c3  0x05 0x87
CH2_NOTE_ENV ch2_c3_4_hold  ENV_4HOLD ch12_c3
CH12_NOTE    ch12_cs3 0x13 0x87
CH1_NOTE_ENV ch1_cs3_1_hold ENV_1HOLD ch12_cs3
CH2_NOTE_ENV ch2_cs3_1_hold ENV_1HOLD ch12_cs3
CH1_NOTE_ENV ch1_cs3_1_fade ENV_1FADE ch12_cs3
CH2_NOTE_ENV ch2_cs3_1_fade ENV_1FADE ch12_cs3
CH1_NOTE_ENV ch1_cs3_2_hold ENV_2HOLD ch12_cs3
CH2_NOTE_ENV ch2_cs3_2_hold ENV_2HOLD ch12_cs3
CH2_NOTE_ENV ch2_cs3_6_hold ENV_6HOLD ch12_cs3
CH2_NOTE_ENV ch2_cs3_7_hold ENV_7HOLD ch12_cs3
CH12_NOTE    ch12_d3  0x21 0x87
CH2_NOTE_ENV ch2_d3_1_hold  ENV_1HOLD ch12_d3
CH1_NOTE_ENV ch1_d3_1_fade  ENV_1FADE ch12_d3
CH2_NOTE_ENV ch2_d3_1_fade  ENV_1FADE ch12_d3
CH2_NOTE_ENV ch2_d3_2_hold  ENV_2HOLD ch12_d3
CH2_NOTE_ENV ch2_d3_2_fade  ENV_2FADE ch12_d3
CH2_NOTE_ENV ch2_d3_3_hold  ENV_3HOLD ch12_d3
CH2_NOTE_ENV ch2_d3_3_fade  ENV_3FADE ch12_d3
CH2_NOTE_ENV ch2_d3_4_hold  ENV_4HOLD ch12_d3
CH2_NOTE_ENV ch2_d3_4_fade  ENV_4FADE ch12_d3
CH2_NOTE_ENV ch2_d3_5_hold  ENV_5HOLD ch12_d3
CH2_NOTE_ENV ch2_d3_6_hold  ENV_6HOLD ch12_d3
CH12_NOTE    ch12_ds3 0x2d 0x87
CH2_NOTE_ENV ch2_ds3_4_hold ENV_4HOLD ch12_ds3
CH2_NOTE_ENV ch2_ds3_6_hold ENV_6HOLD ch12_ds3
CH12_NOTE    ch12_e3  0x39 0x87
CH1_NOTE_ENV ch1_e3_1_hold  ENV_1HOLD ch12_e3
CH2_NOTE_ENV ch2_e3_1_hold  ENV_1HOLD ch12_e3
CH2_NOTE_ENV ch2_e3_1_fade  ENV_1FADE ch12_e3
CH2_NOTE_ENV ch2_e3_2_hold  ENV_2HOLD ch12_e3
CH2_NOTE_ENV ch2_e3_3_hold  ENV_3HOLD ch12_e3
CH2_NOTE_ENV ch2_e3_4_hold  ENV_4HOLD ch12_e3
CH2_NOTE_ENV ch2_e3_6_hold  ENV_6HOLD ch12_e3
CH2_NOTE_ENV ch2_e3_7_hold  ENV_7HOLD ch12_e3
CH12_NOTE    ch12_f3  0x44 0x87
CH2_NOTE_ENV ch2_f3_1_hold  ENV_1HOLD ch12_f3
CH2_NOTE_ENV ch2_f3_1_fade  ENV_1FADE ch12_f3
CH2_NOTE_ENV ch2_f3_2_hold  ENV_2HOLD ch12_f3
CH2_NOTE_ENV ch2_f3_2_fade  ENV_2FADE ch12_f3
CH2_NOTE_ENV ch2_f3_4_hold  ENV_4HOLD ch12_f3
CH12_NOTE    ch12_fs3 0x4f 0x87
CH2_NOTE_ENV ch2_fs3_4_hold ENV_4HOLD ch12_fs3
CH2_NOTE_ENV ch2_fs3_5_hold ENV_5HOLD ch12_fs3
CH2_NOTE_ENV ch2_fs3_7_hold ENV_7HOLD ch12_fs3
CH12_NOTE    ch12_g3  0x59 0x87
CH2_NOTE_ENV ch2_g3_4_hold  ENV_4HOLD ch12_g3
CH2_NOTE_ENV ch2_g3_5_hold  ENV_5HOLD ch12_g3
CH2_NOTE_ENV ch2_g3_6_hold  ENV_6HOLD ch12_g3
CH12_NOTE    ch12_gs3 0x62 0x87
CH2_NOTE_ENV ch2_gs3_5_hold ENV_5HOLD ch12_gs3
CH2_NOTE_ENV ch2_gs3_7_hold ENV_7HOLD ch12_gs3
CH12_NOTE    ch12_a3  0x6b 0x87
CH2_NOTE_ENV ch2_a3_5_hold  ENV_5HOLD ch12_a3
CH12_NOTE    ch12_as3 0x73 0x87
CH2_NOTE_ENV ch2_as3_5_hold ENV_5HOLD ch12_as3
CH2_NOTE_ENV ch2_as3_6_hold ENV_6HOLD ch12_as3
CH12_NOTE    ch12_b3  0x7b 0x87
CH2_NOTE_ENV ch2_b3_4_hold  ENV_4HOLD ch12_b3
CH2_NOTE_ENV ch2_b3_7_hold  ENV_7HOLD ch12_b3
CH12_NOTE    ch12_c4  0x83 0x87
CH2_NOTE_ENV ch2_c4_4_hold  ENV_4HOLD ch12_c4
CH12_NOTE    ch12_cs4 0x8a 0x87
CH2_NOTE_ENV ch2_cs4_4_hold ENV_4HOLD ch12_cs4
CH2_NOTE_ENV ch2_cs4_5_hold ENV_5HOLD ch12_cs4
CH2_NOTE_ENV ch2_cs4_6_hold ENV_6HOLD ch12_cs4
CH12_NOTE    ch12_d4  0x90 0x87
CH2_NOTE_ENV ch2_d4_7_hold  ENV_7HOLD ch12_d4
CH12_NOTE    ch12_ds4 0x97 0x87
CH2_NOTE_ENV ch2_ds4_5_hold ENV_5HOLD ch12_ds4
CH2_NOTE_ENV ch2_ds4_7_hold ENV_7HOLD ch12_ds4
CH12_NOTE    ch12_f4  0xa2 0x87
CH2_NOTE_ENV ch2_f4_7_hold  ENV_7HOLD ch12_f4
CH12_NOTE    ch12_a4  0xb6 0x87
CH2_NOTE_ENV ch2_a4_4_hold  ENV_4HOLD ch12_a4
CH2_NOTE_ENV ch2_a4_5_hold  ENV_5HOLD ch12_a4
CH2_NOTE_ENV ch2_a4_6_hold  ENV_6HOLD ch12_a4
CH12_NOTE    ch12_as4 0xba 0x87
CH2_NOTE_ENV ch2_as4_4_hold ENV_4HOLD ch12_as4
CH12_NOTE    ch12_b4  0xbd 0x87
CH2_NOTE_ENV ch2_b4_6_hold  ENV_6HOLD ch12_b4
CH12_NOTE    ch12_c5  0xc1 0x87
CH2_NOTE_ENV ch2_c5_4_hold  ENV_4HOLD ch12_c5
CH2_NOTE_ENV ch2_c5_7_hold  ENV_7HOLD ch12_c5
CH4_NOTE     ch4_c0   0x84  ; freq=65.27
CH4_NOTE_ENV ch4_c0_5_hold  ENV_5HOLD ch4_c0
CH4_NOTE     ch4_cs0  0x77  ; freq=68.97
CH4_NOTE_ENV ch4_cs0_5_hold ENV_5HOLD ch4_cs0
CH4_NOTE     ch4_d0   0x77  ; freq=73.35
CH4_NOTE_ENV ch4_d0_4_hold  ENV_4HOLD ch4_d0
CH4_NOTE_ENV ch4_d0_4_fade  ENV_4FADE ch4_d0
CH4_NOTE_ENV ch4_d0_5_hold  ENV_5HOLD ch4_d0
CH4_NOTE     ch4_ds0  0x77  ; freq=77.72
CH4_NOTE_ENV ch4_ds0_4_hold ENV_4HOLD ch4_ds0
CH4_NOTE_ENV ch4_ds0_6_hold ENV_6HOLD ch4_ds0
CH4_NOTE     ch4_f0   0x76  ; freq=87.14
CH4_NOTE_ENV ch4_f0_4_hold  ENV_4HOLD ch4_f0
CH4_NOTE_ENV ch4_f0_4_fade  ENV_4FADE ch4_f0
CH4_NOTE_ENV ch4_f0_5_hold  ENV_5HOLD ch4_f0
CH4_NOTE     ch4_fs0  0x76  ; freq=92.19
CH4_NOTE_ENV ch4_fs0_4_hold ENV_4HOLD ch4_fs0
CH4_NOTE_ENV ch4_fs0_5_hold ENV_5HOLD ch4_fs0
CH4_NOTE     ch4_gs0  0x75  ; freq=103.63
CH4_NOTE_ENV ch4_gs0_4_hold ENV_4HOLD ch4_gs0
CH4_NOTE     ch4_a0   0x75  ; freq=109.68
CH4_NOTE_ENV ch4_a0_4_hold  ENV_4HOLD ch4_a0
CH4_NOTE_ENV ch4_a0_5_hold  ENV_5HOLD ch4_a0
CH4_NOTE     ch4_as0  0x74  ; freq=116.42
CH4_NOTE_ENV ch4_as0_4_hold ENV_4HOLD ch4_as0
CH4_NOTE     ch4_c1   0x74  ; freq=130.55
CH4_NOTE_ENV ch4_c1_4_hold  ENV_4HOLD ch4_c1
CH4_NOTE     ch4_cs1  0x67  ; freq=138.28
CH4_NOTE_ENV ch4_cs1_6_hold ENV_6HOLD ch4_cs1
CH4_NOTE     ch4_d1   0x67  ; freq=146.69
CH4_NOTE_ENV ch4_d1_4_hold  ENV_4HOLD ch4_d1
CH4_NOTE     ch4_ds1  0x67  ; freq=155.45
CH4_NOTE_ENV ch4_ds1_4_hold ENV_4HOLD ch4_ds1
CH4_NOTE_ENV ch4_ds1_6_hold ENV_6HOLD ch4_ds1
CH4_NOTE     ch4_f1   0x66  ; freq=174.29
CH4_NOTE_ENV ch4_f1_4_hold  ENV_4HOLD ch4_f1
CH4_NOTE_ENV ch4_f1_6_hold  ENV_6HOLD ch4_f1
CH4_NOTE     ch4_gs1  0x65  ; freq=207.59
CH4_NOTE_ENV ch4_gs1_7_hold ENV_7HOLD ch4_gs1
CH4_NOTE     ch4_a1   0x65  ; freq=219.71
CH4_NOTE_ENV ch4_a1_4_hold  ENV_4HOLD ch4_a1
CH4_NOTE_ENV ch4_a1_5_hold  ENV_5HOLD ch4_a1
CH4_NOTE     ch4_b1   0x64  ; freq=245.95
CH4_NOTE_ENV ch4_b1_4_hold  ENV_4HOLD ch4_b1
CH4_NOTE     ch4_c2   0x64  ; freq=261.43
CH4_NOTE_ENV ch4_c2_6_hold  ENV_6HOLD ch4_c2
CH4_NOTE     ch4_cs2  0x57  ; freq=276.9
CH4_NOTE_ENV ch4_cs2_4_hold ENV_4HOLD ch4_cs2
CH4_NOTE_ENV ch4_cs2_6_hold ENV_6HOLD ch4_cs2
CH4_NOTE     ch4_d2   0x57  ; freq=293.38
CH4_NOTE_ENV ch4_d2_5_hold  ENV_5HOLD ch4_d2
CH4_NOTE_ENV ch4_d2_7_hold  ENV_7HOLD ch4_d2
CH4_NOTE     ch4_ds2  0x57  ; freq=310.89
CH4_NOTE_ENV ch4_ds2_4_hold ENV_4HOLD ch4_ds2
CH4_NOTE_ENV ch4_ds2_5_hold ENV_5HOLD ch4_ds2
CH4_NOTE_ENV ch4_ds2_6_hold ENV_6HOLD ch4_ds2
CH4_NOTE_ENV ch4_ds2_7_hold ENV_7HOLD ch4_ds2
CH4_NOTE     ch4_e2   0x56  ; freq=329.39
CH4_NOTE_ENV ch4_e2_4_hold  ENV_4HOLD ch4_e2
CH4_NOTE_ENV ch4_e2_5_hold  ENV_5HOLD ch4_e2
CH4_NOTE_ENV ch4_e2_7_hold  ENV_7HOLD ch4_e2
CH4_NOTE     ch4_f2   0x56  ; freq=348.91
CH4_NOTE_ENV ch4_f2_4_hold  ENV_4HOLD ch4_f2
CH4_NOTE_ENV ch4_f2_5_hold  ENV_5HOLD ch4_f2
CH4_NOTE_ENV ch4_f2_6_hold  ENV_6HOLD ch4_f2
CH4_NOTE     ch4_gs2  0x55  ; freq=415.18
CH4_NOTE_ENV ch4_gs2_6_hold ENV_6HOLD ch4_gs2
CH4_NOTE_ENV ch4_gs2_7_hold ENV_7HOLD ch4_gs2
CH4_NOTE     ch4_b2   0x54  ; freq=491.91
CH4_NOTE_ENV ch4_b2_7_hold  ENV_7HOLD ch4_b2
CH4_NOTE     ch4_cs3  0x47  ; freq=553.81
CH4_NOTE_ENV ch4_cs3_7_hold ENV_7HOLD ch4_cs3
CH4_NOTE     ch4_gs3  0x47  ; freq=830.72
CH4_NOTE_ENV ch4_gs3_7_hold ENV_7HOLD ch4_gs3
CH4_NOTE     ch4_ds4  0x47  ; freq=1243.56
CH4_NOTE_ENV ch4_ds4_7_hold ENV_7HOLD ch4_ds4
CH4_NOTE     ch4_fs4  0x47  ; freq=1479.74
CH4_NOTE_ENV ch4_fs4_7_hold ENV_7HOLD ch4_fs4
CH4_NOTE     ch4_gs4  0x47  ; freq=1661.43
CH4_NOTE_ENV ch4_gs4_6_hold ENV_6HOLD ch4_gs4
CH4_NOTE     ch4_c5   0x47  ; freq=2091.41
CH4_NOTE_ENV ch4_c5_5_hold  ENV_5HOLD ch4_c5
CH4_NOTE_ENV ch4_c5_7_hold  ENV_7HOLD ch4_c5
CH4_NOTE     ch4_d5   0x47  ; freq=2347.12
CH4_NOTE_ENV ch4_d5_4_hold  ENV_4HOLD ch4_d5
CH4_NOTE_ENV ch4_d5_4_fade  ENV_4FADE ch4_d5
