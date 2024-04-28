# TeenyBios

My attempt at a small bios for Qemu/8088/8086 systems.

A couple of settings would help to know, to test bioses.
Possibly as test targets aswell. A few dumped bioses are known to work, see the following table:

|Bios|Machine type|RAM|CPU|MD5|Note|
|--|-------------|-----|--|--|--|
|jetway_awd_600pgn_i815.BIN|pc|2G|486|7597537ee3e0a86689d83bdfa3cd645d| runs |
|jetway_awd_600pgn_i815.BIN|pc|2G|base|7597537ee3e0a86689d83bdfa3cd645d| stuck at checking hardware |
|GX150A07.ROM|pc|2G|base|752488cce0d9e98615fa6e546a0ad0d0| Will not run |
|659_10.bin|pc|2G|486|2dbf49b4b8577d4ebdbf1e28e2d5da03|Runs, slight graphical glitches in the beginning|



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
