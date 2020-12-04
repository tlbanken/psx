# Controllers
## Communication Sequence
| Send | Reply | Comment |
| ---- | ----- | ------- |
| 0x01 | Hi-z | Controller Access |
| 0x42 | idlo | Receive ID bit0..7 (variable) and Send Read Command (ASCII "B") |
| TAP | idhi | Receive ID bit8..15 (usally/always 0x5a) |
| MOT | swlo | Receive Digital switches bit0..7 |
| MOT | swhi | Receive Digital switches bit8..15 |
| ***END*** | ***OF*** | ***DIGITAL PAD (or analog pad in digital mode)*** |
| 0x00 | adc0 | Receive Analog Input 0 (if any) (eg. analog joypad or mouse) |
| 0x00 | adc1 | Receive Analog Input 1 (if any) (eg. analog joypad or mouse) |
| ***END*** | ***OF*** | ***ANALOG MOUSE*** |
| 0x00 | adc2 | Receive Analog Input 2 (if any) (eg. analog joypad) |
| 0x00 | adc3 | Receive Analog Input 3 (if any) (eg. analog joypad) |

- TAP byte should usually zero (else Multitap)
- the two MOT bytes are meant to control the rumble motors
    - for non-rumble controllers, the bytes should be 0x00

## Contoller ID (Halfword Number 0)
```
0-3     Number of following halfwords (0x01..0x0f=1..15, or 0x00=16)
4-7     Controller type (or currently selected controller mode)
8-15    Fixed (0x5a)
```

### Known 16bit ID values
| ID | Name | Comment |
| -- | ---- | ------- |
| 0xnn00 | N/A | initial buffer value from InitPad BIOS function |
| 0x5a12 | Mouse | two button mouse |
| 0x5a23 | NegCon | steering twist/wheel/paddle |
| 0x5a31 | Konami Lightgun | IRQ10-type |
| 0x5a41 | Digital Pad | Or analog pad in digital mode; LED=Off |
| 0x5a53 | Analog Stick | or analog pad in "flight mode"; LED=Green |
| 0x5a63 | Namco Lightgun | Cinch-type |
| 0x5a73 | Analog Pad | in normal analog mode; LED=RED |
| 0x5a80 | Multitap | multiplayer adaptor |
| 0x5ae3 | Jogcon | steering dial |
| 0x5af3 | Config Mode | when in config mode, see rumble command 0x43 |
| 0xffff | High-z | No controller connected |


## Standard Digital/Analog Controllers
| Bit(s) | Button | comment |
| ------ | ------ | ------- |
| ***HALFWORD 0*** | - | - |
| 0-15 | Contoller Info | Eg. 0x5a41=digital, 0x5a73=analog/pad, 0x5a53=analog/stick |
| ***HALFWORD 1*** | - | - |
| 0 | Select Button | 0=pressed, 1=released |
| 1 | L3/Joy-button | 0=pressed, 1=released/None/Disabled |
| 2 | R3/Joy-button | 0=pressed, 1=released/None/Disabled |
| 3 | Start Button | 0=pressed, 1=released |
| 4 | Joypad Up | 0=pressed, 1=released |
| 5 | Joypad Right | 0=pressed, 1=released |
| 6 | Joypad Down | 0=pressed, 1=released |
| 7 | Joypad Left | 0=pressed, 1=released |
| 8 | L2 Button | 0=pressed, 1=released |
| 9 | R2 Button | 0=pressed, 1=released |
| 10 | L1 Button | 0=pressed, 1=released |
| 11 | R1 Button | 0=pressed, 1=released |
| 12 | Triangle Button | 0=pressed, 1=released |
| 13 | Circle Button | 0=pressed, 1=released |
| 14 | Cross Button | 0=pressed, 1=released |
| 15 | Square Button | 0=pressed, 1=released |
| ***HALFWORD 2*** | ***RIGHT JOYSTICK*** | - |
| 0-7 | RightJoyX (adc0) | 0x00=left, 0x80=center, 0xff=right |
| 8-15 | RightJoyY (adc1) | 0x00=Up, 0x80=center, 0xff=down |
| ***HALFWORD 3*** | ***LEFT JOYSTICK*** | - |
| 0-7 | LeftJoyX (adc2) | 0x00=left, 0x80=center, 0xff=right |
| 8-15 | LeftJoyY (adc3) | 0x00=Up, 0x80=center, 0xff=down |


