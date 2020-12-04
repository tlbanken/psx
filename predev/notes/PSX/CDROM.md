# CD-ROM
Source: [nocash](http://problemkaputt.de/psx-spx.htm#cdromdrive)

## CDROM Controller I/O Ports (Registers)
### Index/Status Register (0x1f80_1800) (Bit0-1 R/W) (Bit2-7 R)
```
0-1     Index       Port 0x1f80_180[1-3] index (0..3 = Index0..Index3) (R/W)
2       ADPBUSY     XA-ADPCM fifo empty     (0=Empty) ;set when playing XA-ADPCM sound
3       PRMEMPT     parameter fifo empty    (1=Empty) ;triggered before writing 1st byte
4       PRMWRDY     parameter fifo full     (0=Full)  ;triggered after writing 16 bytes
5       RSLRRDY     response fifo empty     (0=Empty) ;triggered after reading LAST byte
6       DRQSTS      data fifo empty         (0=Empty) ;triggered after reading LAST byte
7       BUSYSTS     command/parameter transmission busy (1=busy)

Bit3,4,5 are bound to 5bit counters; ie. the bits becom true at specifies amount
of reads/writes, and thereafter once on every further 32 reads/writes.
```

### Command Register (0x1f80_1801.Index0) (W)
- writing to this address sends the command byte to the CDROM controller, which will then read-out any Parameter byte(s) which have been previously stored in the Parameter Fifo
- once completed, interrupt `INT3` is generated (or `INT5` in case of invalid cmd/param values)
    - response code (or error code) can be then read from the Response fifo
- some commands have a second response, which is sent with another interrupt

```
0-7     Command Byte
```

### Parameter Fifo (0x1f80_1802.Index0) (W)
- before sending a command, write any parameter byte(s) to this address
```
0-7     Parameter Byte(s) to beused for next command
```

### Request Register (0x1f80_1803.Index0) (W)
```
0-4             Not used (should be zero)
5       SMEN    Want command start interrupt on next command (0=No change, 1=Yes)
6       BFWR    ...
7       BFRD    Want Data (0=No/Reset data fifo, 1=yes/load data fifo)
```

### Data Fifo - 8bit/16bit (0x1f80_1802.Index0..3) (R)
- to read datablocks:
    1. ReadS/ReadN commands generate INT1
    2. Software sets the Want Data bit (`0x1f80_1803.Index0.bit7`)
    3. wait until Data Fifo becomes not empty (`0x1f80_1800.Bit6`)
- The datablock (disk sector) can be read from this register

```
0-7     Data 8bit (one byte), or alternately...
0-15    Data 16bit (LSB=first byte, MSB=second byte)
```

- the PSX hw allows reads of 2048-byte or 2340-byte sectors
    - indexed as [0x000..0x7ff] or [0x000..0x923]
    - when trying to read further bytes, the PSX will repeat the byte at index [0x800-8] or [0x924-4] as padding value
- port `0x1f80_1802` can be accessd with 8bit or 16bit reads

### Response Fifo (0x1f80_1801.Index1)/(Index[0,2,3] are mirrors) (R)
```
0-7     Response byte(s) received after sending a command
```
- the response fifo is a 16-byte buffer
    - most or all responses are less than 16 bytes
- after reading the last used byte (or before reading anything when the response is 0-byte long), Bit5 of the Index/Status register becomes zero to indicate that the last byte was received
- when reading further bytes
    - buffer is padded with 0x00's to the end of the 16bytes
    - restart at the first response byte

### Interrupt Enable Register (0x1f80_1802.Index1)(W)
### Interrupt Enable Register (0x1f80_1803.Index0)(R)
### Interrupt Enable Register (0x1f80_1803.Index2)(R) (Mirror)
```
0-4     Interrupt enable bits (usually all set, ie. 0x1f=Enable all IRQs)
5-7     Unknown/unused (write: should be zero) (read: usually all bits set)
```

### Interrupt Flag Register (0x1f80_1803.Index1)(R/W)
### Interrupt Flag Register (0x1f80_1803.Index3)(R) (Mirror)
```
0-2     Read: Response received     Write: 7=Acknowledge       ;INT1..INT7
3       Read: Unknown (usually 0)   Write: 1=Acknowledge       ;INT8   ;XXX CLRBFEMPT
4       Read: Command Start         Write: 1=Acknowledge       ;INT10h ;XXX CLRBFWRDY
5       Read: Always 1  ;XXX "_"    Write: 1=Unknown                   ;XXX SMADPCLR
6       Read: Always 1  ;XXX "_"    Write: 1=Reset Parameter Fifo      ;XXX CLRPRM
7       Read: Always 1  ;XXX "_"    Write: 1=Unknown                   ;XXX CHPRST
```
- writing "1" bits to bit0-4 resets the corresponding IRQ flags
> The lower 3bit indicates teh type of response received
```
INT0    No response received (no interrupt request)
INT1    Received SECOND  (or further) response to ReadS/ReadN (and Play+Report)
INT2    Received SECOND response (to various commands)
INT3    Received FIRST response (to any command)
INT4    DataEnd (when Play/Forward reaches end of disk) (maybe also for Read?)
INT5    Received error-code (in FIRST or SECOND response)
        INT5 also occurs on SECOND GetID response, on unlicensed disks
        INT5 also occurs when opening the drive door (even if no command was sent,
        ie. even if no read-command or other command is active)
INT6    N/A
INT7    N/A
```
> The other 2bit indicates something else
```
INT8    Unknown
INT10h  Command Start (when INT10h requested via 0x1f80_1803.Index0.bit5)
```
- the response interrupts are queued

### Audio Volume for Left-CD-Out to Left-SPU-Input (0x1f80_1802.Index2)(W)
### Audio Volume for Left-CD-Out to Right-SPU-Input (0x1f80_1803.Index2)(W)
### Audio Volume for Right-CD-Out to Right-SPU-Input (0x1f80_1801.Index3)(W)
### Audio Volume for Right-CD-Out to Left-SPU-Input (0x1f80_1802.Index3)(W)
- Allows to configure the CD for mono/stereo output 
- when using bigger values, the hardware does have some incomplete staturation support
    - the saturation works up to double volume
    - saturation does NOT work properly when exceeding double volume
```
0-7     Volume Level (0x00..0xff) (0x00=off, 0xff=Max/Double, 0x80=Default/Normal)
```
- after changing these registers, write 0x20 to `0x1f80_1803.Index3`


### Audio Volume Apply Changes (by writing bit5=1) (0x1f80_1803.Index3)
```
0       ADPMUTE     Mute ADPCM (0=Normal, 1=Mute)
1-4     -           Unused (should be zero)
5       CHNGATV     Apply audio volume changes (0=No change, 1=Apply)
6-7     -           Unused (should be zero)
```

### Sound Map Data Out (0x1f80_1801.Index1)(W)
- restricted to 8bit bus
- unknown how/if the PSX DMA writes to it (maybe only 16bit data for CDROM)
```
0-7     Data
```

### Sound Map Coding Info (W)
```
0       Mono/Stereo         (0=Mono, 1=Stereo)
1       Reserved            (0)
2       Sample Rate         (0=37800Hz, 1=18900Hz)
3       Reserved            (0)
4       Bits per Sample     (0=4bit, 1=8bit)
5       Reserved            (0)
6       Emphasis            (0=Off, 1=Emphasis)
7       Reserved            (0)
```

### Command Execution
- command/parameter transmission is indicated by bit7 of `0x1f80_1800`
- when that bit gets zero, the response can be read (or wait for IRQ)
- if there are any pending cdrom interrupts, these MUST be acknowledged before sending the command (otherwise bit7 of `0x1f80_1800` will stay set forever)

### Command Busy Flag (0x1f80_1800.Bit7)
- indicates ready-to-send-new-command
```
0=Ready to send a new command
1=Busy sending a command/parameters
```
- trying to send a new command in the Busy-phase causes malfunction
- when busy-flag goes off, a new command cab be sent immediately
    - however, the new command stays in the Busy-phase until IRQ from the previous command is acknowledged, at that point the actual transmission of the new command starts, and the busy flag goes off


## Controller Command Summary
### Command Summary
| Command | Parameters | Response(s) |
| ------- | ---------- | ----------- |
| 0x00 - | - | INT5 (0x11,0x40) (??) |
| 0x01 Getstat | - | INT3 (stat) |
| 0x02 Setloc | (E) amm,ass,asect | INT3 (stat) |
| 0x03 Play | (E) track | IN3(stat), optional INT1 (report bytes) |
| 0x04 Forward | (E) - | INT3 (stat), optional INT1 (report bytes) |
| 0x05 Backward | (E) - | INT3 (stat), optional INT1 (report bytes) |
| 0x06 ReadN | (E) - | INT3 (stat), INT1 (stat), datablock |
| 0x07 MororOn | (E) - | INT3 (stat), INT2 (stat) |
| 0x08 Stop | (E) - | INT3 (stat), INT2 (stat) |
| 0x09 Pause | (E) - | INT3 (stat), INT2 (stat) |
| 0x0a Init | - | INT3 (late-stat), INT2 (stat) |
| 0x0b Mute | (E) - | INT3 (stat) |
| 0x0c Demute | (E) - | INT3 (stat) |
| 0x0d Setfilter | (E) file, channel mode | INT3 (stat) |
| 0x0e Setmode | mode | INT3 (stat) |
| 0x0f Getparam | - | INT3 (stat,mode,null,file,channel) |
| 0x10 GetlocL | (E) - | INT3 (amm,ass,asect,mode,file,channel,sm,ci) |
| 0x11 GetlocP | (E) - | INT3 (track,index,mm,ss,sect,amm,ass,asect) |
| 0x12 SetSession | (E) session | INT3 (stat), INT2 (stat) |
| 0x13 GetTN | (E) - | INT3 (stat,first,last) ;BCD |
| 0x14 GetTD | (E) track (BCD) | INT3 (stat,mm,ss) ;BCD |
| 0x15 SeekL | (E) - | INT3 (stat), INT2 (stat) ;use prior Setloc |
| 0x16 SeekP | (E) - | INT3 (stat), INT2 (stat) |
| 0x17 - | - | INT5 (0x11,0x40) ;"SetClock??" |
| 0x18 - | - | INT5 (0x11,0x40) ;"GetClock??" |
| 0x19 Test | sub_function | depends on sub_function (see below) |
| 0x1a GetID | (E) - | INT3 (stat), INT2/5 (stat,flg,typ,atip,"SCEx") |
| 0x1b ReadS | (E?) - | INT3 (stat), INT1 (stat), datablock |
| 0x1c Reset | - | INT3 (stat), Delay |
| 0x1d GetQ | (E) adr,point | INT3 (stat), INT2 (10bytesSubQ,peak_lo) |
| 0x1e ReadTOC | - | INT3 (late-stat), INT2 (stat) |
| 0x1f VideoCD | sub,a,b,c,d,e | INT3 (stat,a,b,c,d,e) ;SCPH-5903 only |
| 0x1f..0x4f | - | INT5 (0x11,0x40) ;unused/invalid |
| 0x50 Secret 1 | - | INT5 (0x11,0x40) |
| 0x51 Secret 2 | "Licensed by" | INT5 (0x11, 0x40) |
| 0x52 Secret 3 | "Sony" | INT5 (0x11,0x40) |
| 0x53 Secret 4 | "Computer" | INT5 (0x11,0x40) |
| 0x54 Secret 5 | "Entertainment" | INT5 (0x11,0x40) |
| 0x55 Secret 6 | "\<region\>" | INT5 (0x11,0x40) |
| 0x56 Secret 7 | - | INT5 (0x11,0x40) |
| 0x57 SecretLock | - | INT5 (0x11,0x40) |
| 0x58..0x5f Crash | - | Crashes the HC05 (jumps into a data area) |
| 0x6f..0xff | - | INT5 (0x11, 0x40) ;unused/invalid |

> E = Error 0x80 appears on some commands when the disk is missing, or when the drive unit is disconnected from the mainboard.

### Sub_Function Numbers (Command 0x19)
| Sub | Params | Response | Effect |
| --- | ------ | -------- | ------ |
| 0x00 | - | INT3 (stat) | Force motor on, clockwise, even if door open |
| 0x01 | - | INT3 (stat) | Force motor on, anti-clockwise, super-fast |
| 0x02 | - | INT3 (stat) | Force motor on, anti-clockwise, super-fast |
| 0x03 | - | INT3 (stat) | Force motor off (ignored during spin-up) |
| 0x04 | - | INT3 (stat) | Start SCEx reading and reset counters |
| 0x05 | - | INT3 (total,success) | Stop SCEx reading and get counters |
| 0x06 (1) | n | INT3 (old) | adjust balance in RAM, send CX(30+n XOR 7) |
| 0x07 (1) | n | INT3 (old) | adjust gain in RAM, send CX(38+n XOR 7) |
| 0x08 (1) | n | INT3 (old) | adjust balance in RAM only |
| 0x06..0x0f | - | INT5 (0x11,0x10) | N/A |
| 0x10 | - | INT3 (stat) | CX(..) force motor on, anti-clockwise, super-fast |
| 0x11 | - | INT3 (stat) | CX(03) move lens up (leave parking position) |
| 0x12 | - | INT3 (stat) | CX(02) move lens down (enter parking position) |
| 0x13 | - | INT3 (stat) | CX(28) move lens Outwards |
| 0x14 | - | INT3 (stat) | CX(2c) move lens inwards |
| 0x15 | - | INT3 (stat) | CX(22) if motor on: move outwards, inwards, motor off |
| 0x16 | - | INT3 (stat) | CX(23) no effect? |
| 0x17 | - | INT3 (stat) | CX(e8) force motor on, clockwise, super-fast |
| 0x18 | - | INT3 (stat) | CX(ea) force motor on, anti-clockwise, super-fast |
| 0x19 | - | INT3 (stat) | CX(25) no effect? |
| 0x1a | - | INT3 (stat) | CX(21) no effect? |
| 0x1b..1f | - | INT5(0x11,0x10) | N/A (0x11,0x20 when NONZERO number of params) |
| 0x20 | - | INT3 (yy,mm,dd,ver) | Get cdrom BIOS date/version (yy,mm,dd,ver) |
| 0x21 | - | INT3 (n) | Get drive switches (bit0=POS0, bit1=DOOR) |
| 0x22 (3) | - | INT3("for ...") | Get region ID string |
| 0x23 (3) | - | INT3("CXD...") | Get chip ID string for servo amplifier |
| 0x24 (3) | - | INT3 ("CXD...") | Get chip ID string for signal processor |
| 0x25 (3) | - | INT3 ("CXD...") | Get chip ID string for decoder/fifo |
| 0x26..0x2f | - | INT5 (0x11, 0x10) | N/A |
| 0x30 (1) | i,x,y | INT3 (stat) | prototype/debug stuff |
| 0x31 (1) | x,y | INT3 (stat) | prototype/debug stuff |
| 0x4X | i | INT3 (x,y) | prototype/debug stuff |
| 0x30..0x4f | .. | INT5 (0x11,0x10) | N/A |
| 0x50 | a[,b[,c]] | INT3 (stat) | servo/signal send CX(a:b:c) |
| 0x51 (2) | 0x39,xx | INT3 (stat,hi,lo) | Servo/Signal send CX(39xx) with response |
| 0x51..0x5f | - | INT5 (0x11,0x10) | N/A |
| 0x60 | lo,hi | INT3 (databyte) | HC05 SUB-CPU read RAM and I/O ports |
| 0x61..0x70 | - | INT5 (0x11,0x10) | N/A |
| 0x71 (3) | adr | INT3 (databyte) | Decoder Read one register |
| 0x72 (3) | adr,dat | INT3 (stat) | Decoder Write one register |
| 0x73 (3) | adr,len | INT3 (databytes..) | decoder Read multiple registers, bugged |
| 0x74 (3) | adr,len,.. | INT3 (stat) | decoder write multiple registers, bugged |
| 0x75 (3) | - | INT3 (lo,hi,lo,hi) | Decoder get host Xfer info remain/addr |
| 0x76 (3) | a,b,c,d | INT3 (stat) | Decoder prepare transfer to/from SRAM |
| 0x77..0xff | - | INT5(0x11, 0x10) | N/A |

#### KEY
```
(1)     Sub_functions are supported only in vC0 and vC1
(2)     Sub_functions supported only in BIOS version vC2 and up
(3)     Sub_functions supported only in BIOS version vC1 and up
```

## Control Commands
### Sync - Command 0x00 -> INTx(stat+1,0x40)(?)
- reportedly ...
    - command does not succeed until all other commands complete
    - this can be used for synchronization
- actually just returns error code 0x40 = invalid command?

### Setfilter - Command 0x0d, file, channel -> INT3(stat)
- auto ADPCM filter ignores sectors except those which have the same channel and file numbers in their subheader
    - this is mechananism used to select which of multiple songs in a single .XA file to play
- Setfilter does not affect actual reading (sector reads still occur for all sectors)

### Setmode - Command 0x0e, mode -> INT3(stat)
```
7   Speed           (0=Normal speed, 1=Double speed)
6   XA-ADPCM        (0=Off, 1=Send XA-ADPCM sectors to SPU audio input)
5   Sector Size     (0=0x800=DataOnly, 1=0x924=WholeSectorExceptSyncBytes)
4   Ignore Bit      (0=Normal, 1=Ignore Sector Size and Setloc position)
3   XA-Filter       (0=Off, 1=Process only XA-ADPCM sectors that match Setfilter)
2   Report          (0=Off, 1=Enable Report-Interrupts for Audio Play)
1   AutoPause       (0=Off, 1=Auto Pause upon End of Track) ;for audio play
0   CDDA            (0=Off, 1=Allow to read CD-DA Sectors; ignore missing EDC)
```
- the "ignore bit" seems to cause the controller to ignore the sector size in bit5
    - instead size is kept from most recent Setmode command which didn't have Bit4 set
- bit4 also seems to cause the controller to ignore the exact Setloc position
    - data is randomly returned from the "Setloc position minus 0..3 sectors"
- Unsure what the practical purpose of bit4 is

### Init - Command 0x0a -> INT3(stat) -> INT2(stat)
- multiple effects at once
- sets mode=0x00
- activates drive motor, Standby, abort all commands

### Reset - Command 0x1c,(...) -> INT3(stat) -> Delay(1/8 seconds)
- resets the drive controller (same as opening and closing the drive door)
- executes no matter if/how many parameters used
- INT3 indicates that the command was started
- No INT indicating when the command is finished
    - wait 1/8 seconds in software for it to finish

### MotorOn - Command 0x07 -> INT3(stat) -> INT2(stat)
- activates the drive motor
    - works ONLY if the motor was off
    - otherwise fails with INT5(stat,0x20)
- Commands like Read, Seek, and Play auto start the motor when needed
    - pretty rare command

### Stop - Command 0x08 -> INT3(stat) -> INT2(stat)
- stops motor with magnetic brakes and moves the drive head to the beginning of the first track
- Official way to restart is command `0x0a`, but almost any command will restart it
- the first response returns the current status (with bit5 cleared)
- the second response returns the new status (with bit1 cleared)

### Pause - Command 0x09 -> INT3(stat) -> INT2(stat)
- aborts reading and playing
- the motor is kept spinning
- drive head maintains the current location within reasonable error
- the first response returns the current status (still with bit5 set if a Read command was active)
- second response returns the new status (with bit5 cleared)

## Seek Commands
### Setloc - Command 0x02,amm,ass,asect -> INT3(stat)
- sets the seek target, but without starting the seek operation
- the amm,ass,asect params refer to the entire disk (not to the current track)
    - to seek to a specific location within track, use GetTD to get start address of the track, and add the desired time offset to it
 
### SeekL - Command 0x15 -> INT3(stat) -> INT2(stat)
- seek to Setloc's location in data mode
    - uses data sector header position data, which works/exists only on Data tracks, not on CD-DA audio tracks
- after seek, the disk stays on the seeked location forever

### SeekP - Command 0x16 -> INT3(stat) -> INT2(stat)
- seek to Setloc's location in audio mode
    - uses Subchannel Q position data, which works on both audio or Data disks
- after seek, the disk stays on the seeked location forever
- after the seek, status is stat.bit7 = 0 (audio playback off), until sending a new Play command to start playback

### SetSession - Command 0x12,session -> INT3(stat) -> INT2(stat)
- seeks to session
    - moves the drive head to the session, with stat bit6 set during the seek phase
- when issued during active-play, the command returns error code 0x80
- when issued during play-spin-up, play is aborted
```
___Errors___
session = 00h causes error code 10h.     ;INT5(03h,10h), no 2nd/3rd response

___On a non-multisession-disk___
session = 01h passes okay.               ;INT3(stat), and once INT2(stat)
session = 02h or higher cause seek error ;INT3(stat), and twice INT5(06h,40h)

___On a multisession-disk with N sessions___
session = 01h..N+1 passes okay   ;where N+1 moves to the END of LAST session
session = N+2 or higher cause seek error  ;2nd response = INT5(06h,20h)
```

## Read Commands
### ReadN - Command 0x06 -> INT3(stat) -> INT1(stat) -> datablock
- read with retry
- responds once with INT3(stat) and the it's repeatedly sending INT1(stat) -> datablock
    - continued even after a successful read has occured
    - use Pause command to terminate the repeated INT1 response
- ReadN and ReadS supposed to cause errors if your're trying to read an unlicensed CD or CD-R

### ReadS - Command 0x1b -> INT3(stat) -> INT1(stat) -> datablock
- Read without automatic retry
- unsure on purpose

### ReadN/ReadS
- both ReadN/ReadS are reading data sequentially, starting at the sector specified with Setloc, and the automatically reading the following sectors


### CDROM Incoming Data / Buffer Overrun Timings
- Read commands are continously receiving 75 sectors per second (or 150 at double speed)
- the software must be fast enough to process that amount of incoming data
- hardware includes a buffer that can hold up to a handul (unknown exact) of sectors
    - so occasional delays of more that 1/75 seconds won't cause lost sectors
- hardware does not support a buffer overrun flag

### ReadTOC - Command 0x1e -> INT3(stat) -> INT2(stat)
> Supported only in BIOS version vC1 and up
- reread the Table of Contents of current session without reset
- only returns status information
    - Use GetTD and GetTN commands for actual TOC info

### Setloc, Read, Pause
- a normal CDROM access (like reading) consists of three commands
    1. Setloc
    2. Read
    3. Pause
- if read is issued with an unprocessed Setloc, drive auto seeks the Setloc location and marks Setloc as processed
- If Read issued without an unprocessed Setloc, the following happens...
    - if reading is already in process, it continues reading
    - if reading was paused, then reading resumes at most recently received sector

## Status Commands
### Status Code (stat)
- 8bit status code returned by Getstat command (and others)
```
7   Play            Playing CD-DA
6   Seek            Seeking
5   Read            Reading data sectors
4   ShellOpen       Once shell open (0=Closed, 1=Is/Was open)
3   IdError         (0=Okay, 1=GetID denied) (also set when Setmode.bit4=1)
2   SeekError       (0=Okay, 1=Seek error) (followed by Error Byte)
1   Spindle Motor   (0=Motor off, or in spin-up phase, 1=Motor on)
0   Error           Invalid command/params (followed by Error byte)

Notes:
Only 1 of bits 5,6,7 can be set at a time
```
- if shell is closed, bit4 is auto reset to 0
- if stat bit0 or bit2 is set, then normal response and interrupt are not sent
- instead INT5 occurs and error-byte is sent as second response byte
```
ERROR CODES

First Response:
0x10    Invalid Sub_function (for command 0x19), or invalid param value
0x20    Wrong number of parameters
0x40    Invalid command
0x80    Cannot respond yet

Second Response:
0x04    Seek failed (when trying to use SeekL on Audio CDs)

Errors when no Command Sent (stat.bit2 set):
0x08    Drive door became open
```
- 0x80 appears on some commands when the disk is missing or when the drive unit is disconnected from the mainboard

### GetStat - Command 0x01 -> INT3(stat)
- returns stat and reset the shell open flag
    - other commands which return stat do not reset the shell open flag

### Getparam - Command 0x0f -> INT3(stat,mode,null,file,channel)
- returns stat, mode, null byte (0x00), and file/channel filter values

### GetlocL - Command 0x10 -> INT3(amm,ass,asect,mode,file,channel,sm,ci)
- retrieves 4-byte sector header, plus 4-byte subheader of the current sector
- can be sent during active Read commands
- hardware can buffer a handful of sectors
    - INT1 handler reveives the *oldest* buffered sector
    - the GetlocL command returns the header and subheader of the *newest* buffered sector
- fails (error code 0x80) when playing audio CDs
    - Audio sectors don't have any header/subheader
    - should use GetlocP
- also fails (error code 0x80) when in seek phase

### GetlocP - Command 0x11 - INT3(track,index,mm,ss,sect,amm,ass,asect)
- retrieves 8 bytes of position information from Subchannel Q with ADR=1
- mainly intended for displaying the current audio position during Play
- all results are in BCD
```
track   track number (0xaa=Lead-out area) (0xff=unknown,toc,none?)
index   index number (usually 0x01)
mm      minute number within track (0x00 and up)
ss      second number within track (0x00 to 0x74)
sect    sector number within track (0x00 to 0x74)
amm     minute number on entire disk (0x00 and up)
ass     second number on entire disk (0x00 to 0x59)
asect   sector number on entire disk (0x00 to 0x74)
```

### GetTN - Command 0x13 -> INT3(stat,first,last) ;BCD
- get first and last track number in the TOC of the current Session
- number of tracks in session can be calculated as (last-first+1)

### GetTD - Command 0x14,track -> INT3(stat,mm,ss) ;BCD
- for a disk with NN tracks...
    - parameter values 0x01..0xNN return start of the specified track
    - parameter value 0x00 returns the end of the last track
    - parameter values bigger than 0xNN return error code 0x10
- GetTD values are relative to index=1

### GetQ - Command 0x1d,adr,point -> INT3(stat) -> INT2(10bytesSubQ,peak_lo)
> Supported only in BIOS version vC1 and up
- allows to read 10 bytes from Subchannel Q in Lead-In
- unlike GetTD, the command allows to receive the exact MM:SS:FF address of the pointed track
- with ADR=1 point can be any point number for ADR=1 in Lead-In
    - the returned 10 bytes are raw SubQ data
- ADR=5 can be used on multisession disks

### GetID - Command 0x1a -> INT3(stat) -> INT2/5(stat,flags,type,atip,"SCEx")
| Drive Status | 1st Response | 2nd Response |
| ------------ | ------------ | ------------ |
| Door Open | INT5(0x11,0x80) | N/A |
| Spin-up | INT5(0x01,0x80) | N/A |
| Detect busy | INT5(0x03,0x80) | N/A |
| No Disk | INT3(stat) | INT5(0x08,0x40, 0x00,0x00, 0x00,0x00,0x00,0x00) |
| Audio Disk | INT3(stat) | INT5(0x0a,0x90, 0x00,0x00, 0x00,0x00,0x00,0x00) |
| Unlicensed:Mode1 | INT3(stat) | INT5(0x0a,0x80, 0x00,0x00, 0x00,0x00,0x00,0x00) |
| Unlicensed:Mode2 | INT3(stat) | INT5(0x0a,0x80, 0x20,0x00, 0x00,0x00,0x00,0x00) |
| Unlicensed:Mode2+Audio | INT3(stat) | INT5(0x0a,0x90, 0x20,0x00, 0x00,0x00,0x00,0x00) |
| Debug/Yaroze:Mode2 | INT3(stat) | INT2(0x02,0x00, 0x20,0x00, 0x20,0x20,0x20,0x20) |
| Licensed:Mode2 | INT3(stat) | INT2(0x02,0x00, 0x20,0x00, 0x53,0x43,0x45,0x4X) |
| Modchip:Audio/Model | INT3(stat) | INT2(0x02,0x00, 0x00,0x00, 0x53,0x43,0x45,0x4X) |


## CD Audio Commands
### Mute - Command 0x0b -> INT3(stat)
- turn off audio streaming to SPU (affects both CD-DA and XA-ADPCM)
- even when muted, the controller is still internally processing audio sectors
- Muting forces the CD output volume to zero

### Demute - Command 0x0c -> INT3(stat)
- turn on audio streaming to SPU
- only needed if used the Mute command

### Play - Command 0x03(track) -> INT3(stat) -> optional INT1(report bytes)
- starts CD audio playback
- parameter is optional
    - if no param is given, play starts at either Setloc position or current location
- params 1..N are starting the selected track (given N tracks)
- parameters N+1..0x99 are restarting the breginning of the current track
- motor is switched off when Play reaches the end of the disk and INT4(stat)is generated

### Forward - Command 0x04 -> INT3(stat) -> optional INT1(report bytes)
### Backward - Command 0x05 -> INT3(stat) -> optional INT1(report bytes)
- after sending the command, the drive is in fast forward/backward mode
    - skips every some sectors
- the skipping rate is fixed
- it increases when sending the command again
- to terminate, send a new Play command (with no params)
- Backward auto switches to Play when reaching the beginning of Track 1
- Forward auto stops the drive motor with INT4(stat) when reaching the end of the last track
- only works if the drive was in Play state, and only if Play had already started
    - if drive not in Play stat, then INT5(stat+1, 0x80) occurs

### Report -> INT1(stat,track,index,mm/amm,ss+0x80/ass,sect/asect,peaklo,peaki)
- when report enabled via Setmode, the Play, Forward and Backward commands repetedly generate INT1 interrupts with 8 bytes response length
- (peaklo,peakhi) contain the Peak values from  the CXD2510Q Signal Processor
    - unsigned absolute peak level in lower 15bit
    - L/R flag in upper bit
- Report mode only affects CD Audio (not Data, nor XA-ADPCM sectors)

### AutoPause -> INT4(stat)
- autopause can be enabled/disabled via Setmode.bit1
- when on, it will pause at the end of a track, when off it will stop at th end of Disk

### Playing XA-ADPCM Sectors
- organized in Files (not tracks)
- played with the Read command (not Play command)
- need to init SPU for CD Audio input, enabled ADPCM via Setmode, then select the sector via Setloc

## Sections Excluded
- Test Commands
- Secret Commands
- Video Commands

