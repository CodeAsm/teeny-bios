# Teeny Bios

My attempt at a small bios for Qemu/8088/8086 systems.
for early testing, its based on Pete Batard's blog posts https://pete.akeo.ie/2011/06/crafting-bios-from-scratch.html and where he continues his work into ubrx https://github.com/pbatard/ubrx, i may deviate from there and go my own route.

*License/Copyright will be changed upon enough changes to the actual code.*

A couple of settings would help to know, to test bioses.
Possibly as test targets aswell. A few dumped bioses are known to work, see the following table:

|Bios|Machine type|RAM|CPU|MD5|Note|
|--|-------------|-----|--|--|--|
|jetway_awd_600pgn_i815.BIN|pc|2G|486|7597537ee3e0a86689d83bdfa3cd645d| runs |
|jetway_awd_600pgn_i815.BIN|pc|2G|base|7597537ee3e0a86689d83bdfa3cd645d| stuck at checking hardware |
|jetway_awd_600pgn_i815.BIN|-|2G|-|7597537ee3e0a86689d83bdfa3cd645d| with port80, stuck at 0x12 |
|GX150A07.ROM|pc|2G|base|752488cce0d9e98615fa6e546a0ad0d0| Will not run |
|659_10.bin|pc|2G|486|2dbf49b4b8577d4ebdbf1e28e2d5da03|Runs, slight graphical glitches in the beginning|
|659_10.bin|pc|16m|486|2dbf49b4b8577d4ebdbf1e28e2d5da03|core dumped after 0x12, when using POSTcard|
|bios.rom|pc|2G|486|3c528806a3774d31fb3766d8b184da90|"Hello BIOS world!" gets printed, thus succes|
|bios.rom|pc|2G|pentium|3c528806a3774d31fb3766d8b184da90|"Hello BIOS world!" gets printed, thus succes|
|bios.rom|pc|2G|kvm, host|3c528806a3774d31fb3766d8b184da90|"Hello BIOS world!" gets printed, thus succes|
|acer386/acer386.bin|pc|16m|486|492a7cbc219c64a41562fef20317e2e4|seems to get stuck pretty fast at 0x07|
|ama932j/ami.bin|pc|16m|486|a94ac40a7dec4deb7aebab90e3d8fa4c|runs only until 0x07|
|ami286/amic206.bin|pc|16m|486|23726af3aa2ff759e1caab296937799b|short test, stuck at 0x0c|
|ami386/ami386.bin|pc|16m|486|429709863d4770540a0db5c463a01e7c|various tests, maybe ram, but gets stuck at 0x13 (0x51 when 2G ram)|
|ami386dx/opt495sx.ami|pc|2G|486|2fa55a009a0fc2d23cf088cd9c4b05d6|Gets stuck after 0x13 (regardless of config)|
|ami486/ami486.bin|pc|16m|486|99bb4fafd75e57cfd9f3a8d4ac9303e2|Seems to get stuck at 0x57 after mem tests?|
|amic206.bin|pc|2G|486|23726af3aa2ff759e1caab296937799b|stuck after 80h 0x0c|
|at386/at386.bin|pc|16m|486|d4a6cfdfd6334e0a6a42356424cebd28|stuck eventually at 0x27|
|award286/award.bin|pc|16m|486|44e1dfa456043bed47a1a2907bd3bfee|eventually loops at 0x0d|iwc
|cisco/PIX-515/AM29F400BT.bin|pc|64m|pentium2|1b0c9b81ace84fb7961f6b59d2a33040|Boots a serialcon, with Port80h it segfaults at 0x13|
||pc|16m|486|||
||pc|16m|486|||
||pc|16m|486|||
||pc|16m|486|||
||pc|16m|486|||
||pc|16m|486|||
||pc|16m|486|||
||pc|16m|486|||
||pc|16m|486|||


|430vx/vx29.bin|pc|2G|486|c9ce8598f9e31fbeaa1f76143feb09e0|coredumped after 80h 0x07|
|430vx/ga586atv.bin|pc|2G|486|28baad3404dbf4c001046de7b3ba677d|coredumped after 80h 0x07|
|430vx/55xwuq0e.bin|pc|2G|486|da8ee627a2f9008b7b4be2b30962cd5a|coredumped after 80h 0x07|
||pc|16m|486|||

One observation is with my added port 80 device, the jetway bios gets stuck. i wonder if my implementation is incomplete, or it actually dislikes this device, anti modding?

## Combine roms
For example the dells200 bioses are split in 2, 32k roms, making a 64k total rom. to combine them, and able to run:

```sh
xxd -p -c 1 rom0.bin
xxd -p -c 1 rom1.bin
paste -d '' rom0.txt rom1.txt | xxd -r -p > fullrom.bin
```

this now should able to be ran.


## Port 80, POSTcard

Long story short, I used hardware POSTcards before and i wanted one in qemu aswell. PCem probably emulates many systems fine, but I wanna understand and work with qemu. also, why do some bioses work without my POSTcard mod for qemu? (will be a public repo soon)

Lets look at a example:
```assembly
; Set AL to 0x06 and write to port 0x80
B0 06      ; MOV AL, 0x06
E6 80      ; OUT 0x80, AL

; Set AL to 0x07 and write to port 0x80
B0 07      ; MOV AL, 0x07
E6 80      ; OUT 0x80, AL
```

When we search our bioses for these, we might find these. and this might help pinpoint where our bios gets stuck, or is looping (forever?)

There are different ways to write to port 80h tho, and i might explore them later.

### the "other" methods in code

Sorry for the way these are formatted, its due to the source of these examples.

```assembly
; Write a byte to port 0x80
MOV AL, 0x06 ; B0 06 
OUT 0x80, AL ; E6 80

; Write a word to port 0x80
MOV AX, 0x1234 ; B8 34 12
OUT 0x80, AX   ; E7 80

; Write a byte from AL to port 0x80 using OUTSB
MOV DX, 0x80 ; BA 80 00
MOV AL, 0x07 ; B0 07
OUTSB        ; 6E

; Write a word to port 0x80 using a 16-bit register
MOV DX, 0x80
MOV AX, 0x5678
OUT DX, AX

```

## Plans, ideas

As the title sais, what will I do with this?
 - write to serial port on actual hardware i own
 - write to video buffer thingy (if possible, very generic way)
 - being able to be debugged using gdb or something
 
I will not attempt to replace coreboot, nor will i add UEFI support. possibly will never attempt to boot anything, many more better bioses excist.


## Resources

* Qemu bioses

https://github.com/Hunterrules0-0/Award-bioses-for-qemu
Way more bioses
https://www.vogons.org/viewtopic.php?t=74628

some require  -cpu 486 
440fx: The 440fx is the oldest but works the best with qemu and bochs
440bx: this dosent work with qemu I think but It sometimes works with bochs as they have limited 440bx support
i815: i815 Is werid I dont know how it works with qemu and bochs but it dose. alot of borads dont work but some do weridly
 https://forums.mydigitallife.net/threads/award-bios-for-qemu-and-bochs.84899/
 
* Vogons https://www.vogons.org/viewtopic.php?t=86937
https://www.vogons.org/viewtopic.php?t=84642
this links to https://web.archive.org/web/20070701012145/http://www.rebelshavenforum.com/

https://stackoverflow.com/questions/49405417/how-to-launch-openbios-from-qemu#51436979
 
https://theretroweb.com/motherboards/s/aaeon-technology-sbc-659
Various tools:
http://www.racelinecentral.com/AwardBios.html



* Bios debugger
https://github.com/Theldus/bread


https://www.bios-mods.com/forum/showthread.php?tid=12083

* Scratch bios?
https://pete.akeo.ie/2011/06/crafting-bios-from-scratch.html
https://pete.akeo.ie/2011/07/introducing-ubrx-universal-bios.html
https://github.com/pbatard/ubrx


https://pete.akeo.ie/2011/11/bin2coff.html


http://bioshacking.blogspot.nl/
https://sites.google.com/site/pinczakko

* GLaBIOS
https://glabios.org/
https://github.com/640-KB/GLaBIOS/tree/main/src

* 8088 Bios
https://github.com/skiselev/8088_bios

*  S80186 has also a custom bios
https://www.jamieiles.com/80186/
hmm
https://github.com/tkchia/gcc-ia16/issues/113
