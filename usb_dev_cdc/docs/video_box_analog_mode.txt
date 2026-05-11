System Solution 4: Analog YUV/RGB Input with Analog Output Mode
IN：{
R0/R1/R2:Pin55/Pin57/Pin59
G0/G1/G2:Pin62/Pin65/Pin67
B0/B1/B2:Pin69/Pin71/Pin73
SOG0/SOG1:Pin62/Pin64
HSIN1/HSIN2:Pin44/Pin46
VSIN1/VSIN2:Pin45/Pin47
}
OUT：{
AGPb:Pin158
AGY:Pin159
AGPr:Pin160
ASVM:Pin154
HSOUT:Pin1
VSOUT:Pin2
HBOUT:Pin6
VBOUT:Pin7
}

{
Input Pin | Description | Output Pin | Description |
R0/R1/R2 | Analog R/V input of CHN0/1/2 | AGPb | Analog Blue/Pb output |  
G0/G1/G2 | Analog G/Y input of CHN0/1/2 | AGY | Analog Green/Y output |  
B0/B1/B2 | Analog B/U input of CHN0/1/2 | AGPr | Analog Red/Pr output |  
SOG0/SOG1 | Analog SOG/Y input of CHN0/1 | ASVM | Analog SVM output |  
HSIN1/HSIN2 | Video H-sync input 1/2* | HSOUT | Video H-sync output |  
VSIN1/VSIN2 | Video V-sync input 1/2* | VSOUT | Video V-sync output |  
 | | HBOUT | Video H-blank output |  
 | | VBOUT | Video V-blank output |}

5725 Key Register Setting:{
Register Name | Address | Value |
pad_bout_en | Reg_S0_48[0] | X |
pad_bin_enz | Reg_S0_48[1] | X |
pad_rout_en | Reg_S0_48[2] | X |
pad_rin_enz | Reg_S0_48[3] | X |
pad_gout_en | Reg_S0_48[4] | X |
pad_gin_enz | Reg_S0_48[5] | X |
pad_ckout_enz | Reg_S0_49[1] | X |
pad_sync_out_enz | Reg_S0_49[2] | 1'b0 |
pad_blk_out_enz | Reg_S0_49[3] | 1'b0 |
vds_do_16b_en | Reg_S3_50[7] | 1'b0 |
out_blank_sel_0 | Reg_S0_50[0] | 1'b0 |
if_sel_adc_sync | Reg_S1_28[2] | 1'b1 |
if_sel_656 | Reg_S1_00[3] | 1'b0 |
if_sel16bit | Reg_S1_00[4] | X |
if_sel24bit | Reg_S1_01[7] | 1'b1 |
Note: "X" means either "0" or "1" is OK.
}
