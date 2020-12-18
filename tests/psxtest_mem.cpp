/*
 * psxtest_mem.cpp
 * 
 * Travis Banken
 * 12/6/2020
 * 
 * Memory tests for the psx.
 */

#include <cassert>
#include <iostream>

#include "util/psxlog.h"
#include "util/psxutil.h"
#include "mem/bus.h"
#include "mem/ram.h"

#include "psxtest.h"

#define TMEM_INFO(msg) PSXLOG_INFO("Test-Mem", msg)
#define TMEM_WARN(msg) PSXLOG_WARN("Test-Mem", msg)
#define TMEM_ERROR(msg) PSXLOG_ERROR("Test-Mem", msg)

static void ramTests()
{
    TMEM_INFO("Creating Bus for RAM tests");
    Bus bus;

    TMEM_INFO("Creating RAM for RAM tests");
    std::shared_ptr<Ram> ram(new Ram());

    TMEM_INFO("Connecting RAM to bus");
    bus.AddAddressSpace(ram, BusPriority::First);

    TMEM_INFO("Starting random access tests");
    TMEM_INFO("Testing Random Access R8/W8 on RAM");
    // r/w 8 tests KUSEG
    u8 data8 = 0;
    // test 1
    bus.Write8(42, 0x0000'0000);
    data8 = bus.Read8(0x0000'0000);
    assert(data8 == 42);
    // test 2
    bus.Write8(11, 0x0000'0001);
    data8 = bus.Read8(0x0000'0001);
    assert(data8 != 42);
    // test 3
    bus.Write8(77, 0x0000'0a23);
    data8 = bus.Read8(0x0000'0a23);
    assert(data8 == 77);

    // r/w 16 tests KSEG0
    TMEM_INFO("Testing Random Access R16/W16 on RAM");
    u16 data16 = 0;
    // test 1
    bus.Write16(2482, 0x8000'0000);
    data16 = bus.Read16(0x8000'0000);
    assert(data16 == 2482);
    data16 = bus.Read16(0x0000'0000);
    assert(data16 == 2482);
    // test 2
    bus.Write16(26, 0x8000'a302);
    data16 = bus.Read16(0x8000'a302);
    assert(data16 == 26);
    data16 = bus.Read16(0x0000'a302);
    assert(data16 == 26);
    // test 3
    bus.Write16(21, 0x8000'a302);
    data16 = bus.Read16(0x8000'f2fc);
    assert(data16 != 21);
    data16 = bus.Read16(0x0000'f2fc);
    assert(data16 != 21);

    // r/w 32 tests KSEG1
    TMEM_INFO("Testing Random Access R32/W32 on RAM");
    u32 data32 = 0;
    // test 1
    bus.Write32(0x12'3456, 0xa000'0000);
    data32 = bus.Read32(0xa000'0000);
    assert(data32 == 0x12'3456);
    data32 = bus.Read32(0x0000'0000);
    assert(data32 == 0x12'3456);
    data32 = bus.Read32(0x8000'0000);
    assert(data32 == 0x12'3456);
    // test 2
    bus.Write32(3456, 0xa000'0000);
    data32 = bus.Read32(0xa000'0000);
    assert( data32 == 3456);
    data32 = bus.Read32(0x0000'0000);
    assert( data32 == 3456);
    data32 = bus.Read32(0x8000'0000);
    assert( data32 == 3456);

    // SEQ ACCESS TESTS
    TMEM_INFO("Starting sequencial access tests for RAM");
    // zeroize ram
    TMEM_INFO("Zeroizing ram");
    for (u32 addr = 0; addr < 0x0020'0000; addr++) {
        bus.Write8(0x00, addr);
    }

    // r8/w8
    u8 write8 = 0x3f;
    u8 read8 = 0;
    TMEM_INFO("Testing Sequencial Access R8/W8");
    for (u32 addr = 0; addr < 0x0020'0000; addr++) {
        bus.Write8(write8, addr);
        read8 = bus.Read8(addr); // KUSEG
        assert(read8 == write8);
        read8 = bus.Read8(addr | 0x8000'0000); // KSEG0
        assert(read8 == write8);
        read8 = bus.Read8(addr | 0xa000'0000); // KSEG1
        assert(read8 == write8);
    }

    // r16/w16
    u16 write16 = 0x3ab2;
    u16 read16 = 0;
    TMEM_INFO("Testing Sequencial Access R16/W16");
    for (u32 addr = 0; addr < 0x0020'0000; addr += 2) {
        bus.Write16(write16, addr);
        read16 = bus.Read16(addr); // KUSEG
        assert(read16 == write16);
        read16 = bus.Read16(addr | 0x8000'0000); // KSEG0
        assert(read16 == write16);
        read16 = bus.Read16(addr | 0xa000'0000); // KSEG1
        assert(read16 == write16);
    }

    // r32/w32
    u32 write32 = 0x892a'b1ce;
    u32 read32 = 0;
    TMEM_INFO("Testing Sequencial Access R32/W32");
    for (u32 addr = 0; addr < 0x0020'0000; addr += 4) {
        bus.Write32(write32, addr);
        read32 = bus.Read32(addr); // KUSEG
        assert(read32 == write32);
        read32 = bus.Read32(addr | 0x8000'0000); // KSEG0
        assert(read32 == write32);
        read32 = bus.Read32(addr | 0xa000'0000); // KSEG1
        assert(read32 == write32);
    }


    TMEM_INFO("Finished RAM tests");
    return;
}

namespace psxtest {
    void MemTests()
    {
        std::cout << PSX_FANCYTITLE("MEM TESTS");
        TMEM_INFO("Performing RAM tests");
        ramTests();
    }
}
