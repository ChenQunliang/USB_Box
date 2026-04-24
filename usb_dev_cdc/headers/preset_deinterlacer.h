/* Deinterlacer preset — shared across all output resolutions.
 * Ported from GBSCpro presetDeinterlacerSection.h.
 * This is written to TV5725 segment 2, offsets 0x00-0x3F.
 */
#ifndef PRESET_DEINTERLACER_H
#define PRESET_DEINTERLACER_H

const uint8_t preset_deinterlacer[] = {
    0xFF, // s2_00
    0x03, // s2_01
    0xEC, // s2_02
    0x00, // s2_03
    0xFF, // s2_04
    0xFF, // s2_05
    0x00, // s2_06
    0x1B, // s2_07
    0x00, // s2_08
    0x70, // s2_09
    0x00, // s2_0A
    0x00, // s2_0B
    0x0F, // s2_0C
    0x04, // s2_0D
    0x7F, // s2_0E
    0x14, // s2_0F
    0x18, // s2_10
    0x00, // s2_11
    0x8E, // s2_12
    0x00, // s2_13
    0x00, // s2_14
    0x00, // s2_15
    0x80, // s2_16
    0x00, // s2_17
    0xC0, // s2_18
    0x61, // s2_19
    0x04, // s2_1A
    0x15, // s2_1B
    0x00, // s2_1C
    0x00, // s2_1D
    0x00, // s2_1E
    0x10, // s2_1F
    0x30, // s2_20
    0x12, // s2_21
    0x04, // s2_22
    0x0F, // s2_23
    0x04, // s2_24
    0x00, // s2_25
    0x4C, // s2_26
    0x0C, // s2_27
    0x00, // s2_28
    0x00, // s2_29
    0x00, // s2_2A
    0x00, // s2_2B
    0x00, // s2_2C
    0x00, // s2_2D
    0x00, // s2_2E
    0x00, // s2_2F
    0x00, // s2_30
    0x00, // s2_31
    0x7F, // s2_32
    0x7F, // s2_33
    0x11, // s2_34
    0x10, // s2_35
    0x03, // s2_36
    0x0B, // s2_37
    0x04, // s2_38
    0x44, // s2_39
    0x60, // s2_3A
    0x04, // s2_3B
    0x0F, // s2_3C
    0x00, // s2_3D
    0x00, // s2_3E
    0x00, // s2_3F
};

#endif /* PRESET_DEINTERLACER_H */
