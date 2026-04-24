/* Mode-detect section preset — shared across all output resolutions.
 * Ported from GBSCpro presetMdSection.h.
 * Written to TV5725 segment 1, offsets 0x60-0x83.
 */
#ifndef PRESET_MD_SECTION_H
#define PRESET_MD_SECTION_H

const uint8_t preset_md_section[] = {
    0xB6, // s1_60 H: unlock: 5, lock: 22
    0x84, // s1_61 V: unlock: 4, lock: 4
    96,   // s1_62 MD_NTSC_INT_CNTRL
    38,   // s1_63
    65,   // s1_64
    62,   // s1_65
    178,  // s1_66
    154,  // s1_67
    78,   // s1_68
    214,  // s1_69
    177,  // s1_6A
    142,  // s1_6B
    124,  // s1_6C
    99,   // s1_6D
    139,  // s1_6E
    118,  // s1_6F
    112,  // s1_70
    98,   // s1_71
    133,  // s1_72
    105,  // s1_73
    83,   // s1_74
    72,   // s1_75
    93,   // s1_76
    148,  // s1_77
    178,  // s1_78
    70,   // s1_79
    198,  // s1_7A
    238,  // s1_7B
    140,  // s1_7C
    98,   // s1_7D
    118,  // s1_7E
    44,   // s1_7F
    0xFF, // s1_80 custom mode H
    0xFF, // s1_81 custom mode V
    0x05, // s1_82 SP timer detect config
    0x0C, // s1_83 MD_UNSTABLE_LOCK_VALUE = 3
};

#endif /* PRESET_MD_SECTION_H */
