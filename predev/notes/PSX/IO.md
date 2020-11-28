# IO
IO for memory cards and controllers.

## I/O Ports
### JOY_TX_DATA (0x1f80_1040)(W)
- writing to this register starts the transfer
    - or as soon as `TXEN=1` and `JOY_STAT.2=READY`
- the written value is sent to the controller or memory card and a byte is received
    - stored in RX FIFO if `JOY_CTRL.1` or `JOY_CTRL.2` is set
- For "TXEN=1"
    - writing to SIO_TX_DATA latches the current TXEN value
    - transfer starts if current TXEN value OR the latched TXEN value is set
```
0-7     Data to be sent
8-31    Not used
```

### JOY_RX_DATA (0x1f80_1040)(R)
- a data byte can be read when `JOY_STAT.1 = 1`
```
0-7     Received Data       (1st RX FIFO entry) (oldest entry)
8-15    Preview             (2nd RX FIFO entry)
16-23   Preview             (3rd RX FIFO entry)
24-31   Preview             (4th RX FIFO entry) (5th..8th cannot be previewed)
```

### JOY_STAT (0x1f80_1044)(R)
```
0       TX Ready Flag 1     (1=Ready/Started)
1       RX FIFO Not Empty   (0=Empty, 1=Not Empty)
2       TX Ready Flag 2     (1=Ready/Finished)
3       RX Parity Error     (0=No, 1=Error; Wrong parity, when enabled) (sticky)
4       Unknown (zero)      (unlike SIO, this isn't RX FIFO Overrun flag)
5       Unknown (zero)      (for SIO this would be RX Bad Stop Bit)
6       Unknown (zero)      (for SIO this would be RX Input Level AFTER Stop bit)
7       ACK Input Level     (0=High, 1=Low)
8       Unknown (zero)      (for SIO this would be CTS Input Level)
9       Interrupt Request   (0=None, 1=IRQ7) (See JOY_CTRL.Bit4,10-12) (sticky)
10      Unknown (always zero)
11-31   Baudrate Timer      (21bit timer, decrementing at 33MHz)
```

### JOY_MODE (0x1f80_1048)(R/W)
- usually 0x000d
```
0-1     Baudrate Reload Factor  (1=MUL1, 2=MUL16, 3=MUL64) (or 0=MUL1, too)
2-3     Character Length        (0=5bits, 1=6bits, 2=7bits, 3=8bits)
4       Parity Enable           (0=No, 1=Enable)
5       Parity Type             (0=Even, 1=Odd) (seems to be vice-versa...?)
6-7     Unkown (always zero)
8       CLK output Polarity     (0=Normal:High=Idle, 1=Inverse:Low=Idle)
9-15    Unknown (always zero)
```

### JOY_CTRL (0x1f80_104a)(R/W)
- usually `0x1003,0x3003,0x0000`
```
0       TX Enable (TXEN)        (0=Disable, 1=Enable)
1       JOYn Output             (0=High, 1=Low/Select) (JOYn as defined in Bit13)
2       RX Enable (RXEN)        (0=Normal, when JOYn=Low, 1=Force Enable Once)
3       Unknown?                (for SIO, this would be TX Output Level)
4       Acknowledge             (0=No change, 1=Reset JOY_STAT.Bits 3,9)
5       Unknown                 (for SIO, this would be RTS Output level)
6       Reset                   (0=No change, 1=reset most JOY_registers to zero)
7       Not used                (always zero)
8-9     RX interrupt mode       (0..3=IRQ when RX FIFO contains 1,2,4,8 bytes)
10      TX interrupt enable     (0=Disable, 1=Enable) ;when JOY_STAT.0-or-2 ; Ready
11      RX interrupt enable     (0=Disable, 1=Enable) ;when N bytes in RX FIFO
12      ACK interrupt enable    (0=Disable, 1=Enable) ;when JOY_STAT.7 ;ACK=LOW
13      Desired Slot Number     (0=JOY1, 1=JOY2) (set to LOW when Bit1=1)
14-15   Not used                (always zero)
```

### JOY_BAUD (0x1f80_104e)(R/W)
- usually `0x0088` (250kHz when Factor = MUL1)
- timer reload occurs when writing to this register
    - and automatically when the baudrate timer reaches zero
- the 16bit reload value is multiplied by the baudrate factor, divided by 2, and then copied to the 21bit Baudrate Timer
- default BAUD value is `0x0088` (equivalent to 0x44 cpu cycles)
```
0-15    Baudrate reload value for decrementing baudrate timer
```

### IRQ7 (ACK) Contoller and Memory Card - Byte Received Interrupt
- gets set after receiving data byte
- I_STAT.7 is edge triggered
    - **can** be acknowledged before or after acknowledging JOY_STAT.9
- JOY_STAT.9 is NOT edge triggered
    - must first wait until JOY_STAT.7=0 and then set JOY_CTRL.4=1
    - this is a HW glitch
> Note: After sending a byte, the kernel waits 100 cycles or so before acknowledging any old IRQ7 and then wiats for the new IRQ7. This means emulators cannot trigger IRQ7 immediately within 0 cycles after sending the byte.

### RX FIFO / TX FIFO Notes
- can hold up to 8 bytes in RX direction
- almost 2 bytes in TX direction
- normally, only 1 byte should be in the RX/TX registers
    - shouldn't send a 2nd byte until ACK received


