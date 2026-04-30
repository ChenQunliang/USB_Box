# TV5725 架构知识树

## 1. 时钟树 (Clock Tree)

```
OSC (晶振)
├── PLL648 (系统PLL, S0_40)
│   ├── PLL_CKIS=0 → PLL使用OSC时钟
│   ├── PLL_DIVBY2Z=0 → OSC/2
│   ├── PLL_MS → 内存时钟 (108M/81M/FBCLK)
│   ├── PLL_IS=1 → ICLK使用输入时钟(ADC), 不使用PLL时钟
│   └── PLL_ADS=1 → 输入时钟来自ADC, 不来自PCLKIN
│
├── PCLKIN (引脚40, 外部像素时钟)
│   ├── PAD_CKIN_ENZ=1 → PCLKIN输入禁用(低有效)
│   └── 本项目中: Si5351 CLK0 → 27MHz (当前已禁用)
│
├── PLLAD (ADC PLL, S5_11-S5_16)
│   ├── PLLAD_BPS=1 → 旁路(ADC PLL不工作)
│   ├── PLLAD_PDZ=1 → 上电
│   ├── ADC_CLK_PLLAD=0 → PLLAD输入来自同步处理器
│   └── ADC_CLK_PA=00 → PA_ADC时钟来自PLLAD的CLKO2
│
├── ADC时钟 (S5_00)
│   ├── ADC_CLK_PA → 选择PA_ADC时钟源
│   │   ├── 00 = PLLAD的CLKO2 (当前)
│   │   ├── 01 = PCLKIN
│   │   └── 10 = V4CLK
│   ├── ADC_CLK_ICLK2X → ICLK2X = ADC输出时钟
│   └── ADC_CLK_ICLK1X → ICLK1X = ICLK2X (或/2)
│
└── 输出时钟 (S0_4F)
    ├── OUT_CLK_MUX → CLKOUT选择 (V4CLK/V2CLK/VCLK/ADC)
    └── OUT_CLK_EN → CLKOUT输出使能
```

## 2. 视频数据流 (Video Data Path)

```
模拟输入
  │
  ▼
┌─────────────────────────────────────────────────────┐
│  ADC (模数转换, Segment 5)                          │
│  ├── ADC_INPUT_SEL → 选择输入端口                   │
│  │   ├── Port 0: R0/G0/B0 (YPbPr输入: PR/Y/PB)     │
│  │   └── Port 1: R1/G1/B1 (RGB输入: R/G/B) ← 当前  │
│  ├── ADC_SOGEN → SOG使能                            │
│  ├── ADC_RYSEL → 钳位选择 (GND/中值)                │
│  ├── ADC_FLTR → 滤波                                │
│  └── ADC gain/offset → 增益/偏移 (S5_06-S5_0B)     │
└─────────────────────┬───────────────────────────────┘
                      │ RGB数据 (24-bit)
                      ▼
┌─────────────────────────────────────────────────────┐
│  DEC (解码器, Segment 5)                            │
│  ├── DEC_MATRIX_BYPS=0 → RGB→YUV转换 (当前)        │
│  │   ├── 0 = RGB输入做RGB2YUV ← 当前               │
│  │   └── 1 = YUV输入旁路矩阵                        │
│  └── 输出: YUV数据                                  │
└─────────────────────┬───────────────────────────────┘
                      │ YUV数据
                      ▼
┌─────────────────────────────────────────────────────┐
│  IF (输入格式器, Segment 1, 0x00-0x2F)              │
│  ├── IF_MATRIX_BYPS=1 → 旁路IF矩阵                 │
│  │   ├── 0 = 做RGB2YUV(源是24bit RGB时)            │
│  │   └── 1 = 数据旁路 ← 当前                       │
│  ├── IF_IN_DREG_BYPS=1 → 输入重定时旁路            │
│  ├── IF_UV_REVERT → Y/UV顺序翻转                   │
│  ├── IF_SEL16BIT → 16位数据路径选择                 │
│  └── 输出: YUV数据                                  │
└─────────────────────┬───────────────────────────────┘
                      │ YUV数据
                      ▼
┌─────────────────────────────────────────────────────┐
│  MEM (存储器, Segment 4)                            │
│  ├── Write FIFO → 写入FIFO (S4_00-S4_2F)           │
│  │   ├── WFF_YUV_DEINTERLACE → YUV去交错           │
│  │   └── WFF_LINE_FLIP → 行翻转                    │
│  ├── Read FIFO → 读取FIFO (S4_30-S4_5F)            │
│  │   ├── RFF_YUV_DEINTERLACE → YUV去交错           │
│  │   └── RFF_LINE_FLIP → 行翻转                    │
│  ├── Playback → 回放 (S4_60-S4_6F)                 │
│  └── 功能: 去交错、帧缓冲、缩放                     │
└─────────────────────┬───────────────────────────────┘
                      │ YUV数据
                      ▼
┌─────────────────────────────────────────────────────┐
│  VDS (视频缩放处理器, Segment 3)                    │
│  ├── 第1级插值 (YUV域) - S3_40[0]                  │
│  │   └── VDS_1ST_INT_BYPS=1 → 已旁路               │
│  ├── 色彩空间转换 - S3_3E[3]                       │
│  │   ├── VDS_CONVT_BYPS=1 → 旁路YUV→RGB,保持YUV ← 当前尝试 │
│  │   └── VDS_CONVT_BYPS=0 → 做YUV→RGB,输出RGB      │
│  ├── 第2级插值 (RGB域) - S3_40[1]                  │
│  │   └── VDS_2ND_INT_BYPS=1 → 已旁路               │
│  ├── 动态范围扩展 - S3_3E[4]                       │
│  │   └── VDS_DYN_BYPS=1 → 旁路(来自预设)           │
│  ├── Y/C增益/偏移 (S3_38-S3_3C)                    │
│  │   ├── Y_GAIN=100, UCOS_GAIN=25, VCOS_GAIN=25    │
│  │   └── Y_OFST=254, U_OFST=1, V_OFST=0            │
│  ├── 同步电平 - S3_3D+S3_3E[0]                     │
│  │   └── VDS_SYNC_LEV → 叠加在Y数据上               │
│  └── 黑电平扩展 - S3_28                            │
└─────────────────────┬───────────────────────────────┘
                      │ YUV数据 (当前尝试)
                      ▼
┌─────────────────────────────────────────────────────┐
│  DAC (数模转换, Segment 0, 0x44-0x45)              │
│  ├── R0/G0/B0输出使能 → Pr/Y/Pb                   │
│  ├── SDAC (同步DAC)                                 │
│  │   ├── SPD=0 → 正常上电                          │
│  │   ├── S0ENZ=0 → 输出最小电压(低有效)            │
│  │   └── S1EN=1 → 最大电压输出使能                 │
│  └── PWDNZ=1 → DAC上电                             │
└─────────────────────┬───────────────────────────────┘
                      │ 模拟YPbPr
                      ▼
                TV (电视机)
```

## 3. 同步信号流 (Sync Path)

```
输入同步 (来自RGB信号)
  │
  ▼
┌─────────────────────────────────────────────────────┐
│  同步处理器 (Sync Processor, Segment 0, 0x46-0x47) │
│  ├── SFTRST_SYNC_RSTZ → 软复位                      │
│  ├── H_PROTECT → 行保护                             │
│  ├── COAST_PRE/POST → 海岸模式                       │
│  └── EXT_SYNC_SEL → 外部同步选择                     │
└─────────────────────────────────────────────────────┘
  │
  ├──→ HD_Bypass (高清旁路, S1_30-S1_53)
  │     ├── HD_MATRIX_BYPS=1 → 旁路YUV2RGB
  │     ├── HD_DYN_BYPS=1 → 旁路动态范围
  │     └── OUT_SYNC_SEL=0 → VDS proc同步源 ← 当前
  │
  └──→ VDS (内部同步)
        └── VDS_SYNC_LEV → 叠加到Y数据上(用于YPbPr)
```

## 4. 模拟开关 (ASW, PB12-PB15)

```
┌─────────────────────────────────────────────────────┐
│  ASW (模拟开关)                                     │
│  ├── PB15 = ASW1                                    │
│  ├── PB14 = ASW2                                    │
│  ├── PB13 = ASW3                                    │
│  └── PB12 = ASW4                                    │
│                                                     │
│  输入模式 → ASW配置                                  │
│  ├── VGA:  ASW1=1, ASW2=var, ASW3=1,  ASW4=1        │
│  ├── RGBS: ASW1=0, ASW2=0,   ASW3=0,  ASW4=1 ← 当前 │
│  ├── RGsB: ASW1=0, ASW2=0,   ASW3=1,  ASW4=0        │
│  └── YPbPr:ASW1=0, ASW2=0,   ASW3=1,  ASW4=0        │
└─────────────────────────────────────────────────────┘
```

## 5. 色彩空间转换关系

```
输入: RGB (来自ADC采样)
  │
  ▼
DEC_MATRIX_BYPS=0:  RGB → YUV
    Y = 0.299×R + 0.587×G + 0.114×B
    U = -0.147×R - 0.289×G + 0.436×B  (→ Pb)
    V = 0.615×R - 0.515×G - 0.100×B   (→ Pr)
  │
  ▼
VDS_CONVT_BYPS=0:   YUV → RGB (反向转换, 产生绿屏)
VDS_CONVT_BYPS=1:   旁路, 保持YUV (输出YPbPr所需)
  │
  ▼
DAC输出 → Pr/Y/Pb 引脚
    CONVT_BYPS=0: R=红, G=绿, B=蓝 → TV看到(Y=绿, 绿屏!)
    CONVT_BYPS=1: DAC通道映射问题 → 同步可能在错误通道
```

## 6. 当前配置 vs 目标

| 模块 | 寄存器 | 原始值 | 当前值 | 目标值 | 说明 |
|------|--------|--------|--------|--------|------|
| DEC | DEC_MATRIX_BYPS | 1(旁路) | **0** | 0 | RGB→YUV转换 |
| VDS | VDS_CONVT_BYPS | 0(转换) | **1** | 1 | 旁路YUV→RGB |
| VDS | VDS_DYN_BYPS | 1(预设) | 1 | 1 | 旁路动态范围 |
| VDS | VDS_1ST_INT_BYPS | 1(预设) | 1 | 1 | 旁路YUV插值 |
| VDS | VDS_2ND_INT_BYPS | 1(预设) | 1 | 1 | 旁路RGB插值 |
| PLLAD | PLLAD_BPS | 0(预设) | **1**(代码覆写) | 1 | 旁路ADC PLL |
| 时钟 | PAD_CKIN_ENZ | 1(预设) | 1 | 1 | 禁用PCLKIN输入 |
| 时钟 | SCLK | - | 内部OSC | 内部OSC | 不使用Si5351 |
| 同步 | OUT_SYNC_SEL | 0(预设) | **0** | 0 | VDS proc同步 |
| Si5351 | CLK0 | 81MHz且禁用 | 27MHz且禁用 | 禁用 | 暂时不用 |

## 7. 当前存在的问题

```
VDS_CONVT_BYPS=1 + OUT_SYNC_SEL=0 组合的潜在问题:
  ├── Y数据(含同步) → DAC的R通道 → Pr引脚
  ├── U数据 → DAC的G通道 → Y引脚 (无同步!)
  ├── V数据 → DAC的B通道 → Pb引脚
  └── 结果: TV在Y引脚上找不到同步 → 循环检测信号
        ↑ 这是"有信号/没信号"循环的根本原因

需要解决的问题:
  ├── 方案A: 找到VDS输出通道交换寄存器 (未发现)
  ├── 方案B: 改变DAC通道映射 (未发现相关寄存器)
  ├── 方案C: 保持VDS_CONVT_BYPS=0, 但调整输出配置
  ├── 方案D: 使用Si5351提供PCLKIN并正确配置PLL648
  └── 方案E: 通过HD_MATRIX路径输出YUV (HD_MATRIX_BYPS=1)
```
