# Memory Cards
## Data Format
### Data Size
- split into 16 blocks
- each block split into 64 sectors (frames)
- first block is used as Directory
    - the remaining blocks are containing Files
    - each file can occupy one or more blocks

```
Total Mem 128KB = 131072 bytes = 0x2_0000 bytes
1 Block 8KB = 8192 bytes = 0x2000 bytes
1 Frame 128 bytes = 0x80 bytes
```

### Header Frame (Block 0, Frame 0)
```
0x00-0x01   Memory Card ID (ASCII "MC")
0x02-0x7e   Unused (zero)
0x7f        Checksum (all above bytes XORed with each other) (usually 0x0e)
```

### Directory Frames (Block 0, Frame 1..15)
```
0x00-0x03   Block Allocation State
                0x51 - In use ;first-or-only block of a file
                0x52 - In use ;middle block of a file (if 3 or more blocks)
                0x53 - In use ;last block of a file (if 2 or more blocks)
                0xa0 - Free ;freshly formatted
                0xa1 - Free ;deleted (first-or-only block of file)
                0xa2 - Free ;deleted (middle block of file)
                0xa3 - Free ;deleted (last block of file)
0x04-0x07   Filesize in bytes (0x2000..0x1e00; in multiples of 8KB)
0x08-0x09   Pointer to the NEXT block number (minus 1) used by the file
            (ie. 0..14 for Block NUmber 1..15) (or 0xffff if last-or-only block)
0x0a-0x1e   Filename in ASCII, terminated by 0x00 (max 20 chars, plus ending 0x00)
0x1f        Zero (unused)
0x20-0x7e   Garbage (usually 0x00-filled)
0x7f        Checksum (all above bytes XORed with each other)
```

### Broken Sector List (Block 0, Frame 16..35)
- if Block0/Frame(16+N) indicates that a given sector is broken, then the data for that sector is stored in Block0/Frame(36+N)

```
0x00-0x03   Broken Sector NUmber (Bock*64 + Frame) (0xffff_ffff=None)
0x04-0x7e   Garbage (usually 0x00-filled) (some cards have [0x08..0x09]=0xffff)
0x7f        Checksum (all above bytes XORed with each other)
```

### Broken Sector Replacement Data (Block 0, Frame 36..55)
```
0x00-0x7f   Data (usually 0xff-filled, if there's no broken sector)
```

### Unused Frames (Block 0, Frame 56..62)
```
0x00-0x7f   Unused (usually 0xff-filled)
```

### Write Test Frame (Block 0, Frame 63)
- reportedly "write test"
- usually same as Block 0 ("MC", 253 zero-bytes, plus checksum 0x0e)

### Title Frame (Block 1..15, Frame 0)(in first block of file only)
```
0x00-0x01   ID (ASCII "SC")
0x02        Icon Display Flag
                0x11...Icon has 1 frame (static)
                0x12...Icon has 2 frames (animated)
                0x13...Icon has 3 frames (animated)
            Values other than 0x11..0x13 are treated as corrupted file
0x03        Block Number (1-15) "icon block count" ??
0x04-0x43   Title in Shift-JIS format (64 bytes = max 32 characters)
0x44-0x4f   Reserved (0x00)
0x50-0x5f   Reserved (0x00) (used for pocketstation)
0x60-0x7f   Icon 16 Color Palette Data (each entry is 16bit CLUT)
```

### Icon Frame(s) (Block 1..15, Frame 1..3) (in first block of file only)
- icons are shown in the BIOS bootmenu (when starting the PlayStation without a CDROM insterted)
```
0x00-0x7f   Icon Bitmap (16x16 pixels, 4bit color depth)
```

### Data Frame(s) (Block 1..15, Frame N..63; N=excluding any Title/Icon Frames)
```
0x00-0x7f   Data
```

### Shift-JIS Character Set (16bit) (used in Title Frames)
- can contain japanese or english text
```
ENGLISH Encoding:
0x81,0x40           SPC
0x81,0x43..0x97     Punctuation marks
0x82,0x4f..0x58     "0..9"
0x82,0x60..0x79     "A..Z"
0x82,0x81..0x9a     "a..z"
```

## Read/Write Commands
### Reading Data From Memory Card
| Send | Reply | Comment |
| ---- | ----- | ------- |
| 0x81 | N/A | Memory Card Access |
| 0x52 | FLAG | Send Read Command (ASCII "R"), Receive FLAG Byte |
| 0x00 | 0x5a | Receive Memory Card ID1 |
| 0x00 | 0x5d | Receive Memory Card ID2 |
| MSB | 0x00 | Send Address MSB (sector number 0..0x3ff)
| LSB | 0x00 | Send Address LSB |
| 0x00 | 0x5c | Receive Command Acknownledge 1 |
| 0x00 | 0x5d | Receive Command Acknownledge 2 |
| 0x00 | MSB | Receive Confirmed address MSB |
| 0x00 | LSB | Receive confirmed address LSB |
| 0x00 | ... | Receive Data Sector (128 bytes) |
| 0x00 | CHK | Receive Checksum (MSB xor LSB xor Data bytes) |
| 0x00 | 0x47 | Receive Memory End byte |

### Writing Data to Memory Card
| Send | Reply | Comment |
| ---- | ----- | ------- |
| 0x81 | N/A | Memory Card Access |
| 0x57 | FLAG | Send Write Command (ASCII "W"), Receive FLAG byte |
| 0x00 | 0x5a | Receive Memory Card ID1 |
| 0x00 | 0x5d | Receive Memory Card ID2 |
| MSB | 0x00 | Send Address MSB (sector Number 0x00..0x3ff) |
| LSB | 0x00 | Send Address LSB |
| ... | 0x00 | Send Data Sector (128 bytes) |
| CHK | 0x00 | Send Checksum (MSB xor LSB xor Data bytes) |
| 0x00 | 0x5c | Receive Command Acknowledge 1 |
| 0x00 | 0x5d | Receive Command Acknowledge 2 |
| 0x00 | 0x4N | Receive Memory End Byte (0x47=Good, 0x4e=BadChecksum, 0xff=BadSector) |

### Get Memory Card ID Command
- this command is supported only by original Sony memory cards

| Send | Reply | Comment |
| ---- | ----- | ------- |
| 0x81 | N/A | Memory Card Access |
| 0x53 | FLAG | Send Get ID Command (ASCII "S"), recieve FLAG byte |
| 0x00 | 0x5a | recieve Memory Card ID1 |
| 0x00 | 0x5d | recieve Memory Card ID2 |
| 0x00 | 0x5c | receive command acknowledge 1 |
| 0x00 | 0x5d | receive command acknowledge 2 |
| 0x00 | 0x04 | receive 0x04 |
| 0x00 | 0x00 | receive 0x00 |
| 0x00 | 0x00 | receive 0x00 |
| 0x00 | 0x80 | receive 0x80 |

### Invalid Commands
- Transfer aborts immediately after faulty command byte
| Send | Reply | Comment |
| ---- | ----- | ------- |
| 0x81 | N/A | Memory Card Access |
| 0xNN | FLAG | Send Invalid Command (anything other that "R", "W", or "S") |

### FLAG Byte
- initial value of FLAG is `0x08`
- unsure exactly what this is for?

| Bit | 0 | 1 | Comment |
| --- | - | - | ------- |
| 2 | no write errors | write errors | Usually sent on the cmd after one that triggered|
| 3 | directory has been read | directory has not been read yet | Reset on writes |

### Timings
- IRQ7 usually triggered circa 1500 cycles after sending a byte
- writes shouldn't be done until after the checksum

