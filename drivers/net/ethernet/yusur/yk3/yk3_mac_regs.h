/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_MAC_REGS_H
#define _YK3_MAC_REGS_H

#define MACX_BASE(macx)	(0x80000 * (macx) + 0x00a00000)
#define MACX_INTER_SOC_LAN01(macx) (MACX_BASE(macx) + 0x8028)
#define MACX_INTER_HOST_LAN01(macx) (MACX_BASE(macx) + 0x802c)
#define MACX_INTER_SOC_LAN23(macx) (MACX_BASE(macx) + 0x8030)
#define MACX_INTER_HOST_LAN23(macx) (MACX_BASE(macx) + 0x8034)
#define MAC_INTER_LAN02_MASK GENMASK(11, 0)
#define MAC_INTER_LAN13_MASK GENMASK(27, 16)
#define MACX_INTER_ENABLE(macx) (MACX_BASE(macx) + 0x8038)
#define MACX_INTER_JITTER(macx) (MACX_BASE(macx) + 0x801c)
#define MACX_INTER_CHY_FUNC(macx, chy) (MACX_BASE(macx) + (0x4 * (chy) + 0x803c))
#define MAC_CH_INTER_HOST_VF_MASK GENMASK(8, 0)
#define MAC_CH_INTER_HOST_PF_MASK GENMASK(14, 9)
#define MAC_CH_INTER_SOC_VF_MASK GENMASK(23, 15)
#define MAC_CH_INTER_SOC_PF_MASK GENMASK(29, 24)
#define MAC_INTER_ENABLE_ALL 0xff
#define MAC_INTER_JITTER_DEF 0x1f

/** Start of UMAC **/
#define UMACX_CHY_STATUS(macx, chy) (MACX_BASE(macx) + (0x400 * (chy) + 0x0010))
#define UMAC_STATUS_EN 0x3ff
#define UMAC_STATUS_MASK 0x3ff
#define UMAC_CONFIG_STRIDE 0x400
#define UMAC_CONFIG(macx, chy) (MACX_BASE(macx) + (chy) * UMAC_CONFIG_STRIDE)
#define UMAC_OFFSET_CHCONFIG 0x3000
#define UMAC_EXTRAL_CONFIG(macx, chy) \
	(MACX_BASE(macx) + UMAC_OFFSET_CHCONFIG + (chy) * UMAC_CONFIG_STRIDE)
/* chmode registers */
#define UMAC_CHMODE_L(macx, chy) (UMAC_CONFIG(macx, chy) + 0x0000)
#define UMAC_CHMODE_H(macx, chy) (UMAC_CONFIG(macx, chy) + 0x0004)
/* maccfg registers */
#define UMAC_MACCFG_L(macx, chy) (UMAC_CONFIG(macx, chy) + 0x0008)
#define UMAC_MACCFG_H(macx, chy) (UMAC_CONFIG(macx, chy) + 0x000C)
/* chsts registers */
#define UMAC_CHSTS_L(macx, chy) (UMAC_CONFIG(macx, chy) + 0x0010)
#define UMAC_CHSTS_H(macx, chy) (UMAC_CONFIG(macx, chy) + 0x0014)
/* chconfig3 registers */
#define UMAC_CHCONFIG3_L(macx, chy) (UMAC_CONFIG(macx, chy) + 0x0018)
#define UMAC_CHCONFIG3_H(macx, chy) (UMAC_CONFIG(macx, chy) + 0x001c)
/* chconfig4 registers */
#define UMAC_CHCONFIG4_L(macx, chy) (UMAC_CONFIG(macx, chy) + 0x0020)
#define UMAC_CHCONFIG4_H(macx, chy) (UMAC_CONFIG(macx, chy) + 0x0024)
/* chconfig5 registers */
#define UMAC_CHCONFIG5_L(macx, chy) (UMAC_CONFIG(macx, chy) + 0x0028)
#define UMAC_CHCONFIG5_H(macx, chy) (UMAC_CONFIG(macx, chy) + 0x002c)
/* chconfig6 registers */
#define UMAC_CHCONFIG6_L(macx, chy) (UMAC_CONFIG(macx, chy) + 0x0030)
#define UMAC_CHCONFIG6_H(macx, chy) (UMAC_CONFIG(macx, chy) + 0x0034)
/* chconfig8 registers */
#define UMAC_CHCONFIG8_L(macx, chy) (UMAC_CONFIG(macx, chy) + 0x0040)
#define UMAC_CHCONFIG8_H(macx, chy) (UMAC_CONFIG(macx, chy) + 0x0044)
/* chconfig12 registers */
#define UMAC_CHCONFIG12_L(macx, chy) (UMAC_CONFIG(macx, chy) + 0x0060)
#define UMAC_CHCONFIG12_H(macx, chy) (UMAC_CONFIG(macx, chy) + 0x0064)
/* txfifocfg registers */
#define UMAC_TXFIFOCFG_L(macx, chy) (UMAC_CONFIG(macx, chy) + 0x00c0)
#define UMAC_TXFIFOCFG_H(macx, chy) (UMAC_CONFIG(macx, chy) + 0x00c4)
/* pcstxoverride1 registers */
#define UMAC_PCSTXOVERRIDE1_L(macx, chy) (UMAC_CONFIG(macx, chy) + 0x00c8)
#define UMAC_PCSTXOVERRIDE1_H(macx, chy) (UMAC_CONFIG(macx, chy) + 0x00cc)
/* pcsrxoverride0 registers */
#define UMAC_PCSRXOVERRIDE0_L(macx, chy) (UMAC_CONFIG(macx, chy) + 0x00e0)
#define UMAC_PCSRXOVERRIDE0_H(macx, chy) (UMAC_CONFIG(macx, chy) + 0x00e4)
/* chconfig31 registers */
#define UMAC_CHCONFIG31_L(macx, chy) (UMAC_CONFIG(macx, chy) + 0x00f8)
#define UMAC_CHCONFIG31_H(macx, chy) (UMAC_CONFIG(macx, chy) + 0x00fc)
/* chconfig32 registers */
#define UMAC_CHCONFIG32_L(macx, chy) (UMAC_EXTRAL_CONFIG(macx, chy) + 0x0)
#define UMAC_CHCONFIG32_H(macx, chy) (UMAC_EXTRAL_CONFIG(macx, chy) + 0x4)
/* chconfig33 registers */
#define UMAC_CHCONFIG33_L(macx, chy) (UMAC_EXTRAL_CONFIG(macx, chy) + 0x8)
#define UMAC_CHCONFIG33_H(macx, chy) (UMAC_EXTRAL_CONFIG(macx, chy) + 0xc)
/* sdcfg0 registers */
#define UMAC_SDCFG0_L(macx, chy) (UMAC_CONFIG(macx, chy) + 0x1000)
#define UMAC_SDCFG0_H(macx, chy) (UMAC_CONFIG(macx, chy) + 0x1004)
/* sdcfg1 registers */
#define UMAC_SDCFG1_L(macx, chy) (UMAC_CONFIG(macx, chy) + 0x1080)
#define UMAC_SDCFG1_H(macx, chy) (UMAC_CONFIG(macx, chy) + 0x1084)
/* sdcfg2 registers */
#define UMAC_SDCFG2_L(macx, chy) (UMAC_CONFIG(macx, chy) + 0x1100)
#define UMAC_SDCFG2_H(macx, chy) (UMAC_CONFIG(macx, chy) + 0x1104)
/* sdcfg3 registers */
#define UMAC_SDCFG3_L(macx, chy) (UMAC_CONFIG(macx, chy) + 0x1180)
#define UMAC_SDCFG3_H(macx, chy) (UMAC_CONFIG(macx, chy) + 0x1184)
/* chmode_0 field mask */
#define CHMODE_MODE_MASK 0x3f
/* state statistics register */
#define UMAC_FRAMES_XMIT_OK(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC000)
#define UMAC_FRAMES_XMIT_ALL(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC008)
#define UMAC_FRAMES_XMIT_ERROR(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC010)
#define UMAC_OCTETS_XMIT_OK(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC018)
#define UMAC_OCTETS_XMIT_ALL(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC020)
#define UMAC_FRAMES_XMIT_UNICAST(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC028)
#define UMAC_FRAMES_XMIT_MULTICAST(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC030)
#define UMAC_FRAMES_XMIT_BROADCAST(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC038)
#define UMAC_FRAMES_XMIT_PAUSE(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC040)
#define UMAC_FRAMES_XMIT_PRIPAUSE(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC048)
#define UMAC_FRAMES_XMIT_VLAN(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC050)
#define UMAC_FRAMES_XMIT_SIZELT64(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC058)
#define UMAC_FRAMES_XMIT_SIZEEQ64(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC060)
#define UMAC_FRAMES_XMIT_SIZE65TO127(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC068)
#define UMAC_FRAMES_XMIT_SIZE128TO255(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC070)
#define UMAC_FRAMES_XMIT_SIZE256TO511(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC078)
#define UMAC_FRAMES_XMIT_SIZE512TO1023(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC080)
#define UMAC_FRAMES_XMIT_SIZE1024TO1518(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC088)
#define UMAC_FRAMES_XMIT_SIZE1519TO2047(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC090)
#define UMAC_FRAMES_XMIT_SIZE2048TO4095(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC098)
#define UMAC_FRAMES_XMIT_SIZE4096TO8191(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC0A0)
#define UMAC_FRAMES_SMIT_SIZE8192TO9215(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC0A8)
#define UMAC_FRAMES_XMIT_SIZEGT9216(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC0B0)
#define UMAC_FRAMES_XMIT_PRI0(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC0B8)
#define UMAC_FRAMES_XMIT_PRI1(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC0C0)
#define UMAC_FRAMES_XMIT_PRI2(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC0C8)
#define UMAC_FRAMES_XMIT_PRI3(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC0D0)
#define UMAC_FRAMES_XMIT_PRI4(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC0D8)
#define UMAC_FRAMES_XMIT_PRI5(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC0E0)
#define UMAC_FRAMES_XMIT_PRI6(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC0E8)
#define UMAC_FRAMES_XMIT_PRI7(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC0F0)
#define UMAC_XMIT_PRI0_PAUSE1US(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC0F8)
#define UMAC_XMIT_PRI1_PAUSE1US(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC100)
#define UMAC_XMIT_PRI2_PAUSE1US(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC108)
#define UMAC_XMIT_PRI3_PAUSE1US(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC110)
#define UMAC_XMIT_PRI4_PAUSE1US(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC118)
#define UMAC_XMIT_PRI5_PAUSE1US(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC120)
#define UMAC_XMIT_PRI6_PAUSE1US(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC128)
#define UMAC_XMIT_PRI7_PAUSE1US(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC130)
#define UMAC_FRAMES_XMIT_DRAINED(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC138)
#define UMAC_FRAMES_XMIT_JABBERED(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC140)
#define UMAC_FRAMES_XMIT_PADDED(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC148)
#define UMAC_FRAMES_XMIT_TRUNCATED(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC150)
#define UMAC_FRAMES_RCVD_OK(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC158)
#define UMAC_OCTETS_RCVD_OK(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC160)
#define UMAC_FRAMES_RCVD_ALL(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC168)
#define UMAC_OCTETS_RCVD_ALL(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC170)
#define UMAC_FRAMES_RCVD_CRCERROR(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC178)
#define UMAC_FRAMES_RCVD_ERROR(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC180)
#define UMAC_FRAMES_RCVD_UNICAST(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC188)
#define UMAC_FRAMES_RCVD_MULTICAST(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC190)
#define UMAC_FRAMES_RCVD_BROADCAST(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC198)
#define UMAC_FRAMES_RCVD_PAUSE(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC1A0)
#define UMAC_FRAMES_RCVD_LENERROR(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC1A8)
#define UMAC_FRAMES_RCVD_OVERSIZED(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC1B8)
#define UMAC_FRAMES_RCVD_FRAGMENTS(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC1C0)
#define UMAC_FRAMES_RCVD_JABBER(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC1C8)
#define UMAC_FRAMES_RCVD_PRIPAUSE(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC1D0)
#define UMAC_FRAMES_RCVD_CRCERRSTOMP(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC1D8)
#define UMAC_FRAMES_RCVD_MAXFRMSIZEVIO(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC1E0)
#define UMAC_FRAMES_RCVD_VLAN(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC1E8)
#define UMAC_FRAMES_RCVD_SIZELT64(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC1F8)
#define UMAC_FRAMES_RCVD_SIZEEQ64(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC200)
#define UMAC_FRAMES_RCVD_SIZE65TO127(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC208)
#define UMAC_FRAMES_RCVD_SIZE128TO255(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC210)
#define UMAC_FRAMES_RCVD_SIZE256TO511(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC218)
#define UMAC_FRAMES_RCVD_SIZE512TO1023(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC220)
#define UMAC_FRAMES_RCVD_SIZE1024TO1518(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC228)
#define UMAC_FRAMES_RCVD_SIZE1419TO2047(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC230)
#define UMAC_FRAMES_RCVD_SIZE2048TO4095(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC238)
#define UMAC_FRAMES_RCVD_SIZE4096TO8191(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC240)
#define UMAC_FRAMES_RCVD_SIZE8192TO9215(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC248)
#define UMAC_FRAMES_RCVD_SIZEGT9216(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC250)
#define UMAC_FRAMES_RCVD_PRI0(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC258)
#define UMAC_FRAMES_RCVD_PRI1(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC260)
#define UMAC_FRAMES_RCVD_PRI2(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC268)
#define UMAC_FRAMES_RCVD_PRI3(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC270)
#define UMAC_FRAMES_RCVD_PRI4(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC278)
#define UMAC_FRAMES_RCVD_PRI5(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC280)
#define UMAC_FRAMES_RCVD_PRI6(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC288)
#define UMAC_FRAMES_RCVD_PRI7(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC290)
#define UMAC_RCVD_PRI0_PAUSE1US(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC298)
#define UMAC_RCVD_PRI1_PAUSE1US(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC2A0)
#define UMAC_RCVD_PRI2_PAUSE1US(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC2A8)
#define UMAC_RCVD_PRI3_PAUSE1US(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC2B0)
#define UMAC_RCVD_PRI4_PAUSE1US(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC2B8)
#define UMAC_RCVD_PRI5_PAUSE1US(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC2C0)
#define UMAC_RCVD_PRI6_PAUSE1US(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC2C8)
#define UMAC_RCVD_PRI7_PAUSE1US(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC2D0)
#define UMAC_RCVD_STD_PAUSE1US(macx, chy) (UMAC_CONFIG(macx, chy) + 0xC2D8)
#define UMAC_STAT_ERROR 0xffffffff
#define UMAC_PHY_STAT_SET_MASK GENMASK(54, 54)
#define UMAC_PHY_STAT_SET_REG(macx, chy) (UMAC_CONFIG(macx, chy) + 0x8)
/** End of UMAC **/

/** Start of XMAC **/
#define XMACX_CHY_STATUS(macx, chy) (MACX_BASE(macx) + (0x2000 * (chy) + 0x404))
#define XMAC_STATUS_EN 0x1
#define XMAC_STATUS_MASK 0x1
/** End of XMAC **/

#define NP_BASE 0x01000000
#define NP_TM_QOS_BLK_BASE 0x3a0000
#define NP_TM_QOS_BASE (NP_BASE + NP_TM_QOS_BLK_BASE)
#define NP_TM_QOS_ENABLE(lane) (NP_TM_QOS_BASE + (0x8040 + (lane) * 0x30))
#define NP_TM_QOS_TK_WD(lane) (NP_TM_QOS_BASE + (0x8044 + (lane) * 0x30))
#define NP_TM_QOS_TK_BYTES(lane) (NP_TM_QOS_BASE + (0x8048 + (lane) * 0x30))
#define NP_TM_QOS_TK_BS(lane) (NP_TM_QOS_BASE + (0x804c + (lane) * 0x30))

#define XCVR_STATUS_CLEAR (0x00)
#define XCVR_STATUS_READY (0x80)
#define XCVR_STATUS_PRESENT BIT(0)
#define XCVR_STATUS_TYPE BIT(1)
#define XCVR_STATUS_SPEED BIT(2)
#define XCVR_STATUS_EEPROM BIT(3)

#endif /* _YK3_MAC_REGS_H */

