# The Motion Decoder (MDEC)
- special controller chip that takes a compressed JPEG-like image and decompresses them into 24-bit bitmapped images for display by the GPU
- can only decompress a 16x16 pixel 24-bit image at a time
    - called "Macroblocks"
- Macroblocks are an encoded block that uses the YUV (YCbCr) color scheme with Discrete Cosine Transformation (DCT) and Run Length Encoding (RLE) applied
- also performs 24 to 16 bit color conversion to prepare it for whatever color depth the GPU is in
- due to extremely high speed that the decompression is done, the decompressed RGB bitmaps can be combined to form larger pictures which can be used to produce movies
    - max speed is about 9,000 macroblocks per second
    - a 320x240 movie can be played at 30 frames per second
- MDEC data can only be sent/recieved via DMA channels 0 and 1
- DMA channel 0 is for uncompressed data going in and channel 1 is for retrieval of the uncompressed macroblocks
- MDEC gets controlled via the MDEC control register at location `0x1f80_1820`
- current status of the MDEC can be checked using the MDEC status register at `0x1f80_1824`

## Registers
### mdec0 -- 0x1f80_1820 (write)
```
|31   28|  27   | 26| 25  |24                          0|
---------------------------------------------------------
|   u   | RGB24 | u | STP |             u               |
---------------------------------------------------------

Note: The first word of every data segment in str-file is a control word written to this register.

*** KEY ***
u           Unknown
RGB24       Should be set to 0 for 24-bit color and to 1 for 16-bit. In 16-bit mode
STP         toggles whether to set bit 15 of the decompressed data (semi-transparency)
```

### mdec1 -- 0x1f80_1824 (read)
```
|31       30|   29   |28    27| 26  |  25   |    24   | 23  |            0|
---------------------------------------------------------------------------
|   FIFO    | InSync |  DREQ  |  u  | RGB24 | OutSync | STP |     u       |
---------------------------------------------------------------------------

*** KEY ***
u           Unknown
FIFO        First-In-First-Out buffer state
InSync      MDEC is busy decompressing data
OutSync     MDEC is transferring data to man memory
DREQ        Data Request
RGB24       0 for 24-bit color and to 1 for 16-bit. In 16-bit mode
STP         toggles whether to set bit 15 of the decompressed data (semi-transparency)
```

### mdec1 -- 0x1f80_1824 (write)
```
|  31   |30                        0|
-------------------------------------
| reset |           u               |
-------------------------------------

*** KEY ***
u           Unknown
reset       reset MDEC
```

## MDEC Data Format
- the MDEC uses a 'lossy' picture format similar to that of the JPEG file format

| FORMAT |
| ------ |
| header |
| macroblock |
| ... |
| macroblock |
| footer |

### Header
- 32 byte word
```
|31               16|15                0|
-----------------------------------------
|       0x3800      |       size        |
-----------------------------------------

*** KEY ***
0x3800      Data ID
size        size if data after the header
```

### Macroblocks
```
------------
| Cb block |
| Cr block |
| Y0 block |
| Y1 block |
| Y2 block |
| Y3 block |
------------

*** KEY ***
Cb,Cr           The color difference blocks
Y0,Y1,Y2,Y3     The luminescence blocks
```

#### Within each block
```
|15        0|
-------------
|   DCT     |
|   RLE     |
|   ...     |
|   RLE     |
|   EOD     |
```

#### DCT
- DCT data, it has the quantization factor and the Direct Current (DC) reference

```
|15           10|9                         0|
---------------------------------------------
|       Q       |           DC              |
---------------------------------------------

*** KEY ***
Q           Quantization factor (6 bits, unsigned)
DC          Direct Current reference (10 bits, signed)
```

#### RLE
- Run length data

```
|15           10|9                 0|
-------------------------------------
|   LENGTH      |       DATA        |
-------------------------------------

*** KEY ***
LENGTH      The number if zeros between data (6 bits, unsigned)
DATA        The data (10 bits, signed)
```

### EOD (Footer)
- End of Data

```
|15                0|
---------------------
|       0xfe00      |
---------------------

Lets the MDEC know a block is done. The footer is also the same thing.
```

