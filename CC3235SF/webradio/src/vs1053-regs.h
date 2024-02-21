#ifndef VS1053_REGS_H_
#define VS1053_REGS_H_

// Registers
#define VS1053_MODE         0x00 // Mode control
#define VS1053_STATUS       0x01 // Status of VS1053b
#define VS1053_BASS         0x02 // Built-in bass/treble control
#define VS1053_CLOCKF       0x03 // Clock freq + multiplier
#define VS1053_DECODE_TIME  0x04 // Decode time in seconds
#define VS1053_AUDATA       0x05 // Misc. audio data
#define VS1053_WRAM         0x06 // RAM write/read
#define VS1053_WRAMADDR     0x07 // Base address for RAM write/read
#define VS1053_HDAT0        0x08 // Stream header data 0
#define VS1053_HDAT1        0x09 // Stream header data 1
#define VS1053_AIADDR       0x0A // Start address of application
#define VS1053_VOL          0x0B // Volume control
#define VS1053_AICTRL0      0x0C // Application control register 0
#define VS1053_AICTRL1      0x0D // Application control register 1
#define VS1053_AICTRL2      0x0E // Application control register 2
#define VS1053_AICTRL3      0x0F // Application control register 3

// Commands
#define VS1053_CMD_READ     0x03 // Read Opcode
#define VS1053_CMD_WRITE    0x02 // Write Opcode

// VS1053_MODE bits
#define VS1053_MODE_DIFF             0x0001 // Differential
#define VS1053_MODE_LAYER12          0x0002 // Allow MPEG layers I & II
#define VS1053_MODE_RESET            0x0004 // Soft Reset
#define VS1053_MODE_CANCEL           0x0008 // Cancel decoding current file
#define VS1053_MODE_EARSPEAKER_LO    0x0010 // EarSpeaker low setting
#define VS1053_MODE_TESTS            0x0020 // Allow SDI tests
#define VS1053_MODE_STREAM           0x0040 // Stream mode
#define VS1053_MODE_EARSPEAKER_HI    0x0080 // EarSpeaker high setting
#define VS1053_MODE_DACT             0x0100 // DCLK active edge
#define VS1053_MODE_SDIORD           0x0200 // SDI bit order
#define VS1053_MODE_SDISHARE         0x0400 // Share SPI chip select
#define VS1053_MODE_SDINEW           0x0800 // VS10xx native SPI modes
#define VS1053_MODE_ADPCM            0x1000 // PCM/ADPCM recording active
#define VS1053_MODE_SETTOZERO        0x2000 // Not using, Set to zero
#define VS1053_MODE_MIC_LINE1        0x4000 // MIC / LINE1 selector: 0 - MIC, 1 - Line
#define VS1053_MODE_CLK_RANGE        0x8000 // Input clock range: 0 - 12..13 MHz; 1 - 24..26 MHz

// VS1053_CLOCKF bits
#define VS1053_CLOCKF_MULT_XTALI     0x0000 // CLKI = XTALI
#define VS1053_CLOCKF_MULT_XTALIx20  0x2000 // CLKI = XTALI * 2.0
#define VS1053_CLOCKF_MULT_XTALIx25  0x4000 // CLKI = XTALI * 2.5
#define VS1053_CLOCKF_MULT_XTALIx30  0x6000 // CLKI = XTALI * 3.0
#define VS1053_CLOCKF_MULT_XTALIx35  0x8000 // CLKI = XTALI * 3.5
#define VS1053_CLOCKF_MULT_XTALIx40  0xA000 // CLKI = XTALI * 4.0
#define VS1053_CLOCKF_MULT_XTALIx45  0xC000 // CLKI = XTALI * 4.5
#define VS1053_CLOCKF_MULT_XTALIx50  0xE000 // CLKI = XTALI * 5.0
#define VS1053_CLOCKF_ADDx10         0x0800 // 1.0x
#define VS1053_CLOCKF_ADDx15         0x1000 // 1.5x
#define VS1053_CLOCKF_ADDx20         0x1800 // 2.0x

#endif // VS1053_REGS_H_
