#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <iostream>
#include <string>
#include <zlib.h>
#include "nes.h"

const uint32_t CRC_check = 0xCBF43926;

uint32_t getROM_CRC(unsigned long atFrame);
uint32_t getROM_CRC(std::string ROMfile, unsigned long atFrame, bool load = true);
void loadROM(std::string ROMfile);
void runUntil(unsigned long atFrame);

uint32_t getROM_CRC(std::string ROMfile, unsigned long atFrame, bool load);

//Preliminary CRC32 Check
TEST_CASE( "CRC function working correctly - Needed for other tests", "[Working]" ) {

    std::string input = "123456789";
    unsigned int crc = crc32(0L, (Bytef*)(input.c_str()), input.size());
    
    REQUIRE( crc == CRC_check );
}

//CPU Tests
TEST_CASE( "Branch Timing Tests", "[Working]" ) {
    CHECK( getROM_CRC("roms/branch_timing_tests/1.Branch_Basics.nes", 200) == 0x649bfc95 );
    CHECK( getROM_CRC("roms/branch_timing_tests/2.Backward_Branch.nes", 200) == 0x5754c04e );
    CHECK( getROM_CRC("roms/branch_timing_tests/3.Forward_Branch.nes", 200) == 0x835114a0 );
}

TEST_CASE( "CPU Dummy Reads", "[Working]" ) {
    CHECK( getROM_CRC("roms/cpu_dummy_reads/cpu_dummy_reads.nes", 200) == 0x31a9c633 );
}

TEST_CASE( "CPU Dummy Writes", "[Working]" ) {
    CHECK( getROM_CRC("roms/cpu_dummy_writes/cpu_dummy_writes_oam.nes", 447) == 0xb5948c6 );
    CHECK( getROM_CRC("roms/cpu_dummy_writes/cpu_dummy_writes_ppumem.nes", 347) == 0xb09291e8 );
}

TEST_CASE( "CPU Exec Space", "[Working]" ) {
    CHECK( getROM_CRC("roms/cpu_exec_space/test_cpu_exec_space_apu.nes", 354) == 0x9c303a4c );
    CHECK( getROM_CRC("roms/cpu_exec_space/test_cpu_exec_space_ppuio.nes",133 ) == 0xb94462a7 );
}

TEST_CASE( "CPU Interrupts", "[!mayfail][notWorking]" ) {
    CHECK( getROM_CRC("roms/cpu_interrupts_v2/cpu_interrupts.nes", 400) == 0 );
}

TEST_CASE( "CPU Reset", "[Working]" ) {
    loadROM("roms/cpu_reset/ram_after_reset.nes");
    runUntil(190);
    NES::reset();
    CHECK( getROM_CRC(130) == 0x36575c6f );
    loadROM("roms/cpu_reset/registers.nes");
    runUntil(190);
    NES::reset();
    CHECK( getROM_CRC(130) == 0xc344ac79 );
}

TEST_CASE( "CPU Timing Tests", "[Working]" ) {
    CHECK( getROM_CRC("roms/cpu_timing_test6/cpu_timing_test.nes",670 ) == 0x381de675 );
}

TEST_CASE( "Instruction Misc Behavior", "[Working]" ) {
    CHECK( getROM_CRC("roms/instr_misc/instr_misc.nes", 277) == 0xcf9e4b34 );
}

TEST_CASE( "Instruction Test", "[Working]" ) {
    CHECK( getROM_CRC("roms/instr_test-v3/official_only.nes", 1853) == 0x5f32983d );
    CHECK( getROM_CRC("roms/instr_test-v3/all_instrs.nes", 2401) == 0x5f32983d );
}

TEST_CASE( "Instruction Timing", "[Working]" ) {
    CHECK( getROM_CRC("roms/instr_timing/instr_timing.nes", 1351) == 0xa3a72a27 );
}

//PPU Tests

TEST_CASE( "Blargg PPU Tests", "[Working]" ) {
    CHECK( getROM_CRC("roms/blargg_ppu_tests_2005.09.15b/palette_ram.nes", 200) == 0x1b9ca643 );
    CHECK( getROM_CRC("roms/blargg_ppu_tests_2005.09.15b/sprite_ram.nes", 200) == 0x1b9ca643 );
    CHECK( getROM_CRC("roms/blargg_ppu_tests_2005.09.15b/vbl_clear_time.nes", 200) == 0x1b9ca643 );
    CHECK( getROM_CRC("roms/blargg_ppu_tests_2005.09.15b/vram_access.nes", 200) == 0x1b9ca643 );
}

TEST_CASE( "Blargg PPU Tests - IN WORK", "[!mayfail][notWorking]" ) {
    CHECK( getROM_CRC("roms/blargg_ppu_tests_2005.09.15b/power_up_palette.nes", 200) == 0x1b9ca643 );
}

TEST_CASE( "Full Palette", "[!mayfail][notWorking]" ) {
    CHECK( getROM_CRC("roms/full_palette/full_palette.nes", 100) == 0 );
    CHECK( getROM_CRC("roms/full_palette/full_palette_smooth.nes", 100) == 0 );
    CHECK( getROM_CRC("roms/full_palette/flowing_palette.nes", 100) == 0 );
}

TEST_CASE( "OAM Read", "[Working]" ) {
    CHECK( getROM_CRC("roms/oam_read/oam_read.nes", 122) == 0x9a8b7ac2 );
}

TEST_CASE( "OAM Stress", "[!mayfail][notWorking]" ) {
    CHECK( getROM_CRC("roms/oam_stress/oam_stress.nes", 100) == 0 );
}

TEST_CASE( "PPU Open Bus", "[!mayfail][notWorking]" ) {
    CHECK( getROM_CRC("roms/ppu_open_bus/ppu_open_bus.nes", 100) == 0 );
}

TEST_CASE( "PPU Read Buffer", "[!mayfail][notWorking]" ) {
    CHECK( getROM_CRC("roms/ppu_read_buffer/test_ppu_read_buffer.nes", 100) == 0 );
}

TEST_CASE( "PPU Sprite 0 Hit v2", "[!mayfail][notWorking]" ) {
    CHECK( getROM_CRC("roms/ppu_sprite_hit/ppu_sprite_hit.nes", 100) == 0 );
}

TEST_CASE( "PPU Sprite 0 Hit v1", "[Working]" ) {
    CHECK( getROM_CRC("roms/sprite_hit_tests_2005.10.05/01.basics.nes", 123) == 0x7e1a18f0 );
    CHECK( getROM_CRC("roms/sprite_hit_tests_2005.10.05/02.alignment.nes", 100) == 0xdee3a156 );
    CHECK( getROM_CRC("roms/sprite_hit_tests_2005.10.05/03.corners.nes", 100) == 0x201f0749 );
    CHECK( getROM_CRC("roms/sprite_hit_tests_2005.10.05/04.flip.nes", 100) == 0x2d2fe297 );
    CHECK( getROM_CRC("roms/sprite_hit_tests_2005.10.05/05.left_clip.nes", 100) == 0x875fbd94 );
    CHECK( getROM_CRC("roms/sprite_hit_tests_2005.10.05/06.right_edge.nes", 100) == 0xd1467d07 );
    CHECK( getROM_CRC("roms/sprite_hit_tests_2005.10.05/07.screen_bottom.nes", 100) == 0x63fc7ec6 );
    CHECK( getROM_CRC("roms/sprite_hit_tests_2005.10.05/08.double_height.nes", 100) == 0xdd4ddac );
    CHECK( getROM_CRC("roms/sprite_hit_tests_2005.10.05/09.timing_basics.nes", 130) == 0xc8744ae1 );
    CHECK( getROM_CRC("roms/sprite_hit_tests_2005.10.05/10.timing_order.nes", 130) == 0xff13e6a5 );
    CHECK( getROM_CRC("roms/sprite_hit_tests_2005.10.05/11.edge_timing.nes", 130) == 0x46bcd1ff );
}

TEST_CASE( "PPU Sprite Overflow v2", "[!mayfail][notWorking]" ) {
    CHECK( getROM_CRC("roms/ppu_sprite_overflow/ppu_sprite_overflow.nes", 100) == 0 );
}

TEST_CASE( "PPU Sprite Overflow v1", "[!mayfail][notWorking]" ) {
    CHECK( getROM_CRC("roms/sprite_overflow_tests/1.Basics.nes", 100) == 0x25f47614 );
    CHECK( getROM_CRC("roms/sprite_overflow_tests/2.Details.nes", 0) == 0 );
    CHECK( getROM_CRC("roms/sprite_overflow_tests/3.Timing.nes", 0) == 0 );
    CHECK( getROM_CRC("roms/sprite_overflow_tests/4.Obscure.nes", 0) == 0 );
    CHECK( getROM_CRC("roms/sprite_overflow_tests/5.Emulator.nes", 0) == 0 );
}

TEST_CASE( "PPU VBL NMI", "[!mayfail][notWorking]" ) {
    CHECK( getROM_CRC("roms/ppu_vbl_nmi/ppu_vbl_nmi.nes", 100) == 0 );
}

TEST_CASE( "Scanline Test", "[!mayfail][notWorking]" ) {
    CHECK( getROM_CRC("roms/scanline/scanline.nes", 100) == 0 );
}

TEST_CASE( "SPR DMA and DMC DMA", "[!mayfail][notWorking]" ) {
    CHECK( getROM_CRC("roms/sprdma_and_dmc_dma/sprdma_and_dmc_dma.nes", 100) == 0 );
    CHECK( getROM_CRC("roms/sprdma_and_dmc_dma/sprdma_and_dmc_dma_512.nes", 100) == 0 );
}

//APU Tests
TEST_CASE( "APU Reset", "[!mayfail][notWorking]" ) {
    loadROM("roms/apu_reset/4015_cleared.nes");
    runUntil(60);
    NES::reset();
    CHECK( getROM_CRC(100) == 0x273F7839 );
    loadROM("roms/apu_reset/4017_timing.nes");
    runUntil(60);
    NES::reset();
    CHECK( getROM_CRC(100) == 0 );
    loadROM("roms/apu_reset/4017_written.nes");
    runUntil(60);
    NES::reset();
    CHECK( getROM_CRC(100) == 0 );
    loadROM("roms/apu_reset/irq_flag_cleared.nes");
    runUntil(60);
    NES::reset();
    CHECK( getROM_CRC(100) == 0 );
    loadROM("roms/apu_reset/len_ctrs_enabled.nes");
    runUntil(60);
    NES::reset();
    CHECK( getROM_CRC(100) == 0xea82aa12 );
    loadROM("roms/apu_reset/works_immediately.nes");
    runUntil(60);
    NES::reset();
    CHECK( getROM_CRC(100) == 0 );
}

TEST_CASE( "APU Test", "[!mayfail][notWorking]" ) {
    CHECK( getROM_CRC("roms/apu_test/apu_test.nes", 100) ==  0);
}

TEST_CASE( "Blargg APU Tests", "[Working]" ) {
    CHECK( getROM_CRC("roms/blargg_apu_2005.07.30/01.len_ctr.nes", 200) == 0x1b9ca643 );
    CHECK( getROM_CRC("roms/blargg_apu_2005.07.30/02.len_table.nes", 200) == 0x1b9ca643 );
    CHECK( getROM_CRC("roms/blargg_apu_2005.07.30/03.irq_flag.nes", 200) == 0x1b9ca643 );
    CHECK( getROM_CRC("roms/blargg_apu_2005.07.30/04.clock_jitter.nes", 200) == 0x1b9ca643 );
    CHECK( getROM_CRC("roms/blargg_apu_2005.07.30/05.len_timing_mode0.nes", 200) == 0x1b9ca643 );
    CHECK( getROM_CRC("roms/blargg_apu_2005.07.30/06.len_timing_mode1.nes", 200) == 0x1b9ca643 );
    CHECK( getROM_CRC("roms/blargg_apu_2005.07.30/07.irq_flag_timing.nes", 200) == 0x1b9ca643 );
    CHECK( getROM_CRC("roms/blargg_apu_2005.07.30/08.irq_timing.nes", 200) == 0x1b9ca643 );
}

TEST_CASE( "Blargg APU Tests - IN WORK", "[!mayfail][notWorking]" ) {
    CHECK( getROM_CRC("roms/blargg_apu_2005.07.30/09.reset_timing.nes", 200) == 0x1b9ca643 );
    CHECK( getROM_CRC("roms/blargg_apu_2005.07.30/10.len_halt_timing.nes", 200) == 0x1b9ca643 );
    CHECK( getROM_CRC("roms/blargg_apu_2005.07.30/11.len_reload_timing.nes", 200) == 0x1b9ca643 );
}

TEST_CASE( "DMC DMA During Read", "[!mayfail][notWorking]" ) {
    CHECK( getROM_CRC("roms/dmc_dma_during_read4/dma_2007_read.nes", 0) == 0 );
    CHECK( getROM_CRC("roms/dmc_dma_during_read4/dma_2007_write.nes", 125) == 0xa2f1a886 );
    CHECK( getROM_CRC("roms/dmc_dma_during_read4/dma_4016_read.nes", 0) == 0 );
    CHECK( getROM_CRC("roms/dmc_dma_during_read4/double_2007_read.nes", 0) == 0 );
    CHECK( getROM_CRC("roms/dmc_dma_during_read4/read_write_2007.nes", 100) == 0xd050cf9e );
}

TEST_CASE( "Test APU", "[Working]" ) {
    CHECK( getROM_CRC("roms/test_apu_2/test_1.nes", 100) == 0x480cdf78 );
    CHECK( getROM_CRC("roms/test_apu_2/test_2.nes", 100) == 0x480cdf78 );
    CHECK( getROM_CRC("roms/test_apu_2/test_4.nes", 100) == 0x480cdf78 );
    CHECK( getROM_CRC("roms/test_apu_2/test_5.nes", 100) == 0x480cdf78 );
    CHECK( getROM_CRC("roms/test_apu_2/test_7.nes", 100) == 0x480cdf78 );
    CHECK( getROM_CRC("roms/test_apu_2/test_8.nes", 100) == 0x480cdf78 );
}

TEST_CASE( "Test APU - IN WORK", "[!mayfail][notWorking]" ) {
    CHECK( getROM_CRC("roms/test_apu_2/test_3.nes", 100) == 0x480cdf78 );
    CHECK( getROM_CRC("roms/test_apu_2/test_6.nes", 100) == 0x480cdf78 );
    CHECK( getROM_CRC("roms/test_apu_2/test_9.nes", 100) == 0x480cdf78 );
    CHECK( getROM_CRC("roms/test_apu_2/test_10.nes", 100) == 0x480cdf78 );
}

//Mappers
TEST_CASE( "MMC3 Test", "[!mayfail][notWorking]" ) {
    CHECK( getROM_CRC("roms/mmc3_test_2/rom_singles/1-clocking.nes", 0) == 0 );
    CHECK( getROM_CRC("roms/mmc3_test_2/rom_singles/2-details.nes", 0) == 0 );
    CHECK( getROM_CRC("roms/mmc3_test_2/rom_singles/3-A12_clocking.nes", 0) == 0 );
    CHECK( getROM_CRC("roms/mmc3_test_2/rom_singles/4-scanline_timing.nes", 0) == 0 );
    CHECK( getROM_CRC("roms/mmc3_test_2/rom_singles/5-MMC3.nes", 0) == 0 );
    CHECK( getROM_CRC("roms/mmc3_test_2/rom_singles/6-MMC3_alt.nes", 0) == 0 );
}

void loadROM(std::string ROMfile)
{
    if(NES::loadROM(ROMfile) != 0) {
        throw std::runtime_error("Error when loading file: "+ROMfile);
    }
    NES::powerOn();
}

void runUntil(unsigned long atFrame)
{
    while(NES::getFrameNum() < atFrame && NES::running) {
        NES::frameStep();
    }
    if(NES::running == false) {
        throw std::runtime_error("NES operation ended early");
    }
}

uint32_t getROM_CRC(unsigned long atFrame)
{
    return getROM_CRC("", atFrame, false);
}

uint32_t getROM_CRC(std::string ROMfile, unsigned long atFrame, bool load)
{
    if(load)
        loadROM(ROMfile);
    runUntil(atFrame);
    uint8_t *screenOutput = NES::getPixelMap();
    return crc32(0L, screenOutput, 240*256);;
}