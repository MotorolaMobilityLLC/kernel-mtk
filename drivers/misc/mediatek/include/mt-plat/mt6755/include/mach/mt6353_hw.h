#ifndef _MT_PMIC_UPMU_HW_MT6353_H_
#define _MT_PMIC_UPMU_HW_MT6353_H_

/*MT6353*/
#define MT6353_PMIC_REG_BASE (0x0000)

#define MT6353_STRUP_CON0                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0000))
#define MT6353_STRUP_CON1                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0002))
#define MT6353_STRUP_CON2                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0004))
#define MT6353_STRUP_CON3                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0006))
#define MT6353_STRUP_CON4                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0008))
#define MT6353_STRUP_CON5                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x000A))
#define MT6353_STRUP_CON6                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x000C))
#define MT6353_STRUP_CON7                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x000E))
#define MT6353_STRUP_CON8                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0010))
#define MT6353_STRUP_CON9                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0012))
#define MT6353_STRUP_CON10                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0014))
#define MT6353_STRUP_CON11                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0016))
#define MT6353_STRUP_CON12                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0018))
#define MT6353_STRUP_CON13                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x001A))
#define MT6353_STRUP_CON14                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x001C))
#define MT6353_STRUP_CON15                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x001E))
#define MT6353_STRUP_CON16                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0020))
#define MT6353_STRUP_CON17                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0022))
#define MT6353_STRUP_ANA_CON0                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0024))
#define MT6353_STRUP_ANA_CON1                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0026))
#define MT6353_TYPE_C_PHY_RG_0                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0100))
#define MT6353_TYPE_C_PHY_RG_CC_RESERVE_CSR    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0102))
#define MT6353_TYPE_C_VCMP_CTRL                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0104))
#define MT6353_TYPE_C_CTRL                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0106))
#define MT6353_TYPE_C_CC_SW_CTRL               ((unsigned int)(MT6353_PMIC_REG_BASE+0x010a))
#define MT6353_TYPE_C_CC_VOL_PERIODIC_MEAS_VAL ((unsigned int)(MT6353_PMIC_REG_BASE+0x010c))
#define MT6353_TYPE_C_CC_VOL_DEBOUCE_CNT_VAL   ((unsigned int)(MT6353_PMIC_REG_BASE+0x010e))
#define MT6353_TYPE_C_DRP_SRC_CNT_VAL_0        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0110))
#define MT6353_TYPE_C_DRP_SNK_CNT_VAL_0        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0114))
#define MT6353_TYPE_C_DRP_TRY_CNT_VAL_0        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0118))
#define MT6353_TYPE_C_DRP_TRY_WAIT_CNT_VAL_0   ((unsigned int)(MT6353_PMIC_REG_BASE+0x011c))
#define MT6353_TYPE_C_DRP_TRY_WAIT_CNT_VAL_1   ((unsigned int)(MT6353_PMIC_REG_BASE+0x011e))
#define MT6353_TYPE_C_CC_SRC_DEFAULT_DAC_VAL   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0120))
#define MT6353_TYPE_C_CC_SRC_15_DAC_VAL        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0122))
#define MT6353_TYPE_C_CC_SRC_30_DAC_VAL        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0124))
#define MT6353_TYPE_C_CC_SNK_DAC_VAL_0         ((unsigned int)(MT6353_PMIC_REG_BASE+0x0128))
#define MT6353_TYPE_C_CC_SNK_DAC_VAL_1         ((unsigned int)(MT6353_PMIC_REG_BASE+0x012a))
#define MT6353_TYPE_C_INTR_EN_0                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0130))
#define MT6353_TYPE_C_INTR_EN_2                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0134))
#define MT6353_TYPE_C_INTR_0                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0138))
#define MT6353_TYPE_C_INTR_2                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x013C))
#define MT6353_TYPE_C_CC_STATUS                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0140))
#define MT6353_TYPE_C_PWR_STATUS               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0142))
#define MT6353_TYPE_C_PHY_RG_CC1_RESISTENCE_0  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0144))
#define MT6353_TYPE_C_PHY_RG_CC1_RESISTENCE_1  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0146))
#define MT6353_TYPE_C_PHY_RG_CC2_RESISTENCE_0  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0148))
#define MT6353_TYPE_C_PHY_RG_CC2_RESISTENCE_1  ((unsigned int)(MT6353_PMIC_REG_BASE+0x014a))
#define MT6353_TYPE_C_CC_SW_FORCE_MODE_ENABLE  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0160))
#define MT6353_TYPE_C_CC_SW_FORCE_MODE_VAL_0   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0164))
#define MT6353_TYPE_C_CC_SW_FORCE_MODE_VAL_1   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0166))
#define MT6353_TYPE_C_CC_DAC_CALI_CTRL         ((unsigned int)(MT6353_PMIC_REG_BASE+0x0170))
#define MT6353_TYPE_C_CC_DAC_CALI_RESULT       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0172))
#define MT6353_TYPE_C_DEBUG_PORT_SELECT_0      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0180))
#define MT6353_TYPE_C_DEBUG_PORT_SELECT_1      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0182))
#define MT6353_TYPE_C_DEBUG_MODE_SELECT        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0184))
#define MT6353_TYPE_C_DEBUG_OUT_READ_0         ((unsigned int)(MT6353_PMIC_REG_BASE+0x0188))
#define MT6353_TYPE_C_DEBUG_OUT_READ_1         ((unsigned int)(MT6353_PMIC_REG_BASE+0x018a))
#define MT6353_HWCID                           ((unsigned int)(MT6353_PMIC_REG_BASE+0x0200))
#define MT6353_SWCID                           ((unsigned int)(MT6353_PMIC_REG_BASE+0x0202))
#define MT6353_TOP_CON                         ((unsigned int)(MT6353_PMIC_REG_BASE+0x0204))
#define MT6353_TEST_OUT                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0206))
#define MT6353_TEST_CON0                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0208))
#define MT6353_TEST_CON1                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x020A))
#define MT6353_TESTMODE_SW                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x020C))
#define MT6353_EN_STATUS1                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x020E))
#define MT6353_EN_STATUS2                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0210))
#define MT6353_OCSTATUS1                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0212))
#define MT6353_OCSTATUS2                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0214))
#define MT6353_PGDEBSTATUS0                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0216))
#define MT6353_PGSTATUS0                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0218))
#define MT6353_THERMALSTATUS                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x021A))
#define MT6353_TOPSTATUS                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x021C))
#define MT6353_TDSEL_CON                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x021E))
#define MT6353_RDSEL_CON                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0220))
#define MT6353_SMT_CON0                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0222))
#define MT6353_SMT_CON1                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0224))
#define MT6353_SMT_CON2                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0226))
#define MT6353_DRV_CON0                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0228))
#define MT6353_DRV_CON1                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x022A))
#define MT6353_DRV_CON2                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x022C))
#define MT6353_DRV_CON3                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x022E))
#define MT6353_TOP_STATUS                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0230))
#define MT6353_TOP_STATUS_SET                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0232))
#define MT6353_TOP_STATUS_CLR                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0234))
#define MT6353_CLK_RSV_CON0                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0236))
#define MT6353_CLK_BUCK_CON0                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0238))
#define MT6353_CLK_BUCK_CON0_SET               ((unsigned int)(MT6353_PMIC_REG_BASE+0x023A))
#define MT6353_CLK_BUCK_CON0_CLR               ((unsigned int)(MT6353_PMIC_REG_BASE+0x023C))
#define MT6353_CLK_BUCK_CON1                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x023E))
#define MT6353_CLK_BUCK_CON1_SET               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0240))
#define MT6353_CLK_BUCK_CON1_CLR               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0242))
#define MT6353_TOP_CLKSQ                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0244))
#define MT6353_TOP_CLKSQ_SET                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0246))
#define MT6353_TOP_CLKSQ_CLR                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0248))
#define MT6353_TOP_CLKSQ_RTC                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x024A))
#define MT6353_TOP_CLKSQ_RTC_SET               ((unsigned int)(MT6353_PMIC_REG_BASE+0x024C))
#define MT6353_TOP_CLKSQ_RTC_CLR               ((unsigned int)(MT6353_PMIC_REG_BASE+0x024E))
#define MT6353_TOP_CLK_TRIM                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0250))
#define MT6353_CLK_CKROOTTST_CON0              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0252))
#define MT6353_CLK_CKROOTTST_CON1              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0254))
#define MT6353_CLK_PDN_CON0                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0256))
#define MT6353_CLK_PDN_CON0_SET                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0258))
#define MT6353_CLK_PDN_CON0_CLR                ((unsigned int)(MT6353_PMIC_REG_BASE+0x025A))
#define MT6353_CLK_PDN_CON1                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x025C))
#define MT6353_CLK_PDN_CON1_SET                ((unsigned int)(MT6353_PMIC_REG_BASE+0x025E))
#define MT6353_CLK_PDN_CON1_CLR                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0260))
#define MT6353_CLK_SEL_CON0                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0262))
#define MT6353_CLK_SEL_CON0_SET                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0264))
#define MT6353_CLK_SEL_CON0_CLR                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0266))
#define MT6353_CLK_SEL_CON1                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0268))
#define MT6353_CLK_SEL_CON1_SET                ((unsigned int)(MT6353_PMIC_REG_BASE+0x026A))
#define MT6353_CLK_SEL_CON1_CLR                ((unsigned int)(MT6353_PMIC_REG_BASE+0x026C))
#define MT6353_CLK_CKPDN_CON0                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x026E))
#define MT6353_CLK_CKPDN_CON0_SET              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0270))
#define MT6353_CLK_CKPDN_CON0_CLR              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0272))
#define MT6353_CLK_CKPDN_CON1                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0274))
#define MT6353_CLK_CKPDN_CON1_SET              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0276))
#define MT6353_CLK_CKPDN_CON1_CLR              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0278))
#define MT6353_CLK_CKPDN_CON2                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x027A))
#define MT6353_CLK_CKPDN_CON2_SET              ((unsigned int)(MT6353_PMIC_REG_BASE+0x027C))
#define MT6353_CLK_CKPDN_CON2_CLR              ((unsigned int)(MT6353_PMIC_REG_BASE+0x027E))
#define MT6353_CLK_CKPDN_CON3                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0280))
#define MT6353_CLK_CKPDN_CON3_SET              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0282))
#define MT6353_CLK_CKPDN_CON3_CLR              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0284))
#define MT6353_CLK_CKPDN_HWEN_CON0             ((unsigned int)(MT6353_PMIC_REG_BASE+0x0286))
#define MT6353_CLK_CKPDN_HWEN_CON0_SET         ((unsigned int)(MT6353_PMIC_REG_BASE+0x0288))
#define MT6353_CLK_CKPDN_HWEN_CON0_CLR         ((unsigned int)(MT6353_PMIC_REG_BASE+0x028A))
#define MT6353_CLK_CKSEL_CON0                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x028C))
#define MT6353_CLK_CKSEL_CON0_SET              ((unsigned int)(MT6353_PMIC_REG_BASE+0x028E))
#define MT6353_CLK_CKSEL_CON0_CLR              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0290))
#define MT6353_CLK_CKDIVSEL_CON0               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0292))
#define MT6353_CLK_CKDIVSEL_CON0_SET           ((unsigned int)(MT6353_PMIC_REG_BASE+0x0294))
#define MT6353_CLK_CKDIVSEL_CON0_CLR           ((unsigned int)(MT6353_PMIC_REG_BASE+0x0296))
#define MT6353_CLK_CKTSTSEL_CON0               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0298))
#define MT6353_TOP_RST_CON0                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x029A))
#define MT6353_TOP_RST_CON0_SET                ((unsigned int)(MT6353_PMIC_REG_BASE+0x029C))
#define MT6353_TOP_RST_CON0_CLR                ((unsigned int)(MT6353_PMIC_REG_BASE+0x029E))
#define MT6353_TOP_RST_CON1                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x02A0))
#define MT6353_TOP_RST_CON1_SET                ((unsigned int)(MT6353_PMIC_REG_BASE+0x02A2))
#define MT6353_TOP_RST_CON1_CLR                ((unsigned int)(MT6353_PMIC_REG_BASE+0x02A4))
#define MT6353_TOP_RST_CON2                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x02A6))
#define MT6353_TOP_RST_MISC                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x02A8))
#define MT6353_TOP_RST_MISC_SET                ((unsigned int)(MT6353_PMIC_REG_BASE+0x02AA))
#define MT6353_TOP_RST_MISC_CLR                ((unsigned int)(MT6353_PMIC_REG_BASE+0x02AC))
#define MT6353_TOP_RST_STATUS                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x02AE))
#define MT6353_TOP_RST_STATUS_SET              ((unsigned int)(MT6353_PMIC_REG_BASE+0x02B0))
#define MT6353_TOP_RST_STATUS_CLR              ((unsigned int)(MT6353_PMIC_REG_BASE+0x02B2))
#define MT6353_TOP_RST_RSV_CON0                ((unsigned int)(MT6353_PMIC_REG_BASE+0x02B4))
#define MT6353_TOP_RST_RSV_CON1                ((unsigned int)(MT6353_PMIC_REG_BASE+0x02B6))
#define MT6353_INT_CON0                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x02B8))
#define MT6353_INT_CON0_SET                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x02BA))
#define MT6353_INT_CON0_CLR                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x02BC))
#define MT6353_INT_CON1                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x02BE))
#define MT6353_INT_CON1_SET                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x02C0))
#define MT6353_INT_CON1_CLR                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x02C2))
#define MT6353_INT_CON2                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x02C4))
#define MT6353_INT_CON2_SET                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x02C6))
#define MT6353_INT_CON2_CLR                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x02C8))
#define MT6353_INT_CON3                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x02CA))
#define MT6353_INT_CON3_SET                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x02CC))
#define MT6353_INT_CON3_CLR                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x02CE))
#define MT6353_INT_MISC_CON                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x02D0))
#define MT6353_INT_MISC_CON_SET                ((unsigned int)(MT6353_PMIC_REG_BASE+0x02D2))
#define MT6353_INT_MISC_CON_CLR                ((unsigned int)(MT6353_PMIC_REG_BASE+0x02D4))
#define MT6353_INT_STATUS0                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x02D6))
#define MT6353_INT_STATUS1                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x02D8))
#define MT6353_INT_STATUS2                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x02DA))
#define MT6353_INT_STATUS3                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x02DC))
#define MT6353_OC_GEAR_0                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x02DE))
#define MT6353_SPK_CON0                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x02E0))
#define MT6353_SPK_CON1                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x02E2))
#define MT6353_SPK_CON2                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x02E4))
#define MT6353_SPK_CON3                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x02E6))
#define MT6353_SPK_CON4                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x02E8))
#define MT6353_SPK_CON5                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x02EA))
#define MT6353_SPK_CON6                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x02EC))
#define MT6353_SPK_CON7                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x02EE))
#define MT6353_SPK_CON8                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x02F0))
#define MT6353_SPK_CON9                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x02F2))
#define MT6353_SPK_CON10                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x02F4))
#define MT6353_SPK_CON11                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x02F6))
#define MT6353_SPK_CON12                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x02F8))
#define MT6353_SPK_CON13                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x02FA))
#define MT6353_SPK_CON14                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x02FC))
#define MT6353_SPK_CON15                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x02FE))
#define MT6353_SPK_CON16                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0300))
#define MT6353_SPK_ANA_CON0                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0302))
#define MT6353_SPK_ANA_CON1                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0304))
#define MT6353_SPK_ANA_CON3                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0306))
#define MT6353_FQMTR_CON0                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0308))
#define MT6353_FQMTR_CON1                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x030A))
#define MT6353_FQMTR_CON2                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x030C))
#define MT6353_ISINK_ANA_CON_0                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x030E))
#define MT6353_ISINK0_CON0                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0310))
#define MT6353_ISINK0_CON1                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0312))
#define MT6353_ISINK0_CON2                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0314))
#define MT6353_ISINK0_CON3                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0316))
#define MT6353_ISINK1_CON0                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0318))
#define MT6353_ISINK1_CON1                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x031A))
#define MT6353_ISINK1_CON2                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x031C))
#define MT6353_ISINK1_CON3                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x031E))
#define MT6353_ISINK_ANA1                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0320))
#define MT6353_ISINK_PHASE_DLY                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0322))
#define MT6353_ISINK_SFSTR                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0324))
#define MT6353_ISINK_EN_CTRL                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0326))
#define MT6353_ISINK_MODE_CTRL                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0328))
#define MT6353_ISINK_ANA_CON0                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x032A))
#define MT6353_ISINK2_CON1                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x032C))
#define MT6353_ISINK3_CON1                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x032E))
#define MT6353_ISINK_PHASE_DLY_SMPL            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0330))
#define MT6353_ISINK_EN_CTRL_SMPL              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0332))
#define MT6353_CHRIND_CON0                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0334))
#define MT6353_CHRIND_CON1                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0336))
#define MT6353_CHRIND_CON2                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0338))
#define MT6353_CHRIND_CON3                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x033A))
#define MT6353_CHRIND_EN_CTRL                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x033C))
#define MT6353_RG_SPI_CON                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x033E))
#define MT6353_DEW_DIO_EN                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0340))
#define MT6353_DEW_READ_TEST                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0342))
#define MT6353_DEW_WRITE_TEST                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0344))
#define MT6353_DEW_CRC_SWRST                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0346))
#define MT6353_DEW_CRC_EN                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0348))
#define MT6353_DEW_CRC_VAL                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x034A))
#define MT6353_DEW_DBG_MON_SEL                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x034C))
#define MT6353_DEW_CIPHER_KEY_SEL              ((unsigned int)(MT6353_PMIC_REG_BASE+0x034E))
#define MT6353_DEW_CIPHER_IV_SEL               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0350))
#define MT6353_DEW_CIPHER_EN                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0352))
#define MT6353_DEW_CIPHER_RDY                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0354))
#define MT6353_DEW_CIPHER_MODE                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0356))
#define MT6353_DEW_CIPHER_SWRST                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0358))
#define MT6353_DEW_RDDMY_NO                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x035A))
#define MT6353_INT_TYPE_CON0                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x035C))
#define MT6353_INT_TYPE_CON0_SET               ((unsigned int)(MT6353_PMIC_REG_BASE+0x035E))
#define MT6353_INT_TYPE_CON0_CLR               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0360))
#define MT6353_INT_TYPE_CON1                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0362))
#define MT6353_INT_TYPE_CON1_SET               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0364))
#define MT6353_INT_TYPE_CON1_CLR               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0366))
#define MT6353_INT_TYPE_CON2                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0368))
#define MT6353_INT_TYPE_CON2_SET               ((unsigned int)(MT6353_PMIC_REG_BASE+0x036A))
#define MT6353_INT_TYPE_CON2_CLR               ((unsigned int)(MT6353_PMIC_REG_BASE+0x036C))
#define MT6353_INT_TYPE_CON3                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x036E))
#define MT6353_INT_TYPE_CON3_SET               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0370))
#define MT6353_INT_TYPE_CON3_CLR               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0372))
#define MT6353_INT_STA                         ((unsigned int)(MT6353_PMIC_REG_BASE+0x0374))
#define MT6353_BUCK_ALL_CON0                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0400))
#define MT6353_BUCK_ALL_CON1                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0402))
#define MT6353_BUCK_ALL_CON2                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0404))
#define MT6353_BUCK_ALL_CON3                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0406))
#define MT6353_BUCK_ALL_CON4                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0408))
#define MT6353_BUCK_ALL_CON5                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x040A))
#define MT6353_BUCK_DLC_VPA_CON0               ((unsigned int)(MT6353_PMIC_REG_BASE+0x040C))
#define MT6353_BUCK_DLC_VPA_CON1               ((unsigned int)(MT6353_PMIC_REG_BASE+0x040E))
#define MT6353_BUCK_DLC_VPA_CON2               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0410))
#define MT6353_BUCK_TRACKING_CON0              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0412))
#define MT6353_BUCK_TRACKING_CON1              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0414))
#define MT6353_BUCK_TRACKING_CON2              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0416))
#define MT6353_BUCK_TRACKING_CON3              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0418))
#define MT6353_BUCK_ANA_MON_CON0               ((unsigned int)(MT6353_PMIC_REG_BASE+0x041A))
#define MT6353_BUCK_ANA_MON_CON1               ((unsigned int)(MT6353_PMIC_REG_BASE+0x041C))
#define MT6353_BUCK_OC_CON0                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x041E))
#define MT6353_BUCK_OC_CON1                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0420))
#define MT6353_BUCK_OC_CON2                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0422))
#define MT6353_BUCK_OC_CON3                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0424))
#define MT6353_BUCK_OC_CON4                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0426))
#define MT6353_BUCK_OC_VCORE_CON0              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0428))
#define MT6353_BUCK_OC_VCORE2_CON0             ((unsigned int)(MT6353_PMIC_REG_BASE+0x042A))
#define MT6353_BUCK_OC_VPROC_CON0              ((unsigned int)(MT6353_PMIC_REG_BASE+0x042C))
#define MT6353_BUCK_OC_VS1_CON0                ((unsigned int)(MT6353_PMIC_REG_BASE+0x042E))
#define MT6353_BUCK_OC_VPA_CON0                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0430))
#define MT6353_SMPS_TOP_ANA_CON0               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0432))
#define MT6353_SMPS_TOP_ANA_CON1               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0434))
#define MT6353_SMPS_TOP_ANA_CON2               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0436))
#define MT6353_SMPS_TOP_ANA_CON3               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0438))
#define MT6353_SMPS_TOP_ANA_CON4               ((unsigned int)(MT6353_PMIC_REG_BASE+0x043A))
#define MT6353_SMPS_TOP_ANA_CON5               ((unsigned int)(MT6353_PMIC_REG_BASE+0x043C))
#define MT6353_SMPS_TOP_ANA_CON6               ((unsigned int)(MT6353_PMIC_REG_BASE+0x043E))
#define MT6353_VPA_ANA_CON0                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0440))
#define MT6353_VPA_ANA_CON1                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0442))
#define MT6353_VPA_ANA_CON2                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0444))
#define MT6353_VPA_ANA_CON3                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0446))
#define MT6353_VCORE_ANA_CON0                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0448))
#define MT6353_VCORE_ANA_CON1                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x044A))
#define MT6353_VCORE_ANA_CON2                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x044C))
#define MT6353_VCORE_ANA_CON3                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x044E))
#define MT6353_VCORE_ANA_CON4                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0450))
#define MT6353_VPROC_ANA_CON0                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0452))
#define MT6353_VPROC_ANA_CON1                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0454))
#define MT6353_VPROC_ANA_CON2                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0456))
#define MT6353_VPROC_ANA_CON3                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0458))
#define MT6353_VPROC_ANA_CON4                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x045A))
#define MT6353_VS1_ANA_CON0                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x045C))
#define MT6353_VS1_ANA_CON1                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x045E))
#define MT6353_VS1_ANA_CON2                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0460))
#define MT6353_VS1_ANA_CON3                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0462))
#define MT6353_VS1_ANA_CON4                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0464))
#define MT6353_BUCK_ANA_CON0                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0466))
#define MT6353_BUCK_ANA_CON1                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0468))
#define MT6353_VCORE2_ANA_CON0                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x046A))
#define MT6353_VCORE2_ANA_CON1                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x046C))
#define MT6353_VCORE2_ANA_CON2                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x046E))
#define MT6353_VCORE2_ANA_CON3                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0470))
#define MT6353_VCORE2_ANA_CON4                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0472))
#define MT6353_BUCK_VPROC_HWM_CON0             ((unsigned int)(MT6353_PMIC_REG_BASE+0x0600))
#define MT6353_BUCK_VPROC_HWM_CON1             ((unsigned int)(MT6353_PMIC_REG_BASE+0x0602))
#define MT6353_BUCK_VPROC_EN_CON0              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0604))
#define MT6353_BUCK_VPROC_SFCHG_CON0           ((unsigned int)(MT6353_PMIC_REG_BASE+0x0606))
#define MT6353_BUCK_VPROC_VOL_CON0             ((unsigned int)(MT6353_PMIC_REG_BASE+0x0608))
#define MT6353_BUCK_VPROC_VOL_CON1             ((unsigned int)(MT6353_PMIC_REG_BASE+0x060A))
#define MT6353_BUCK_VPROC_VOL_CON2             ((unsigned int)(MT6353_PMIC_REG_BASE+0x060C))
#define MT6353_BUCK_VPROC_VOL_CON3             ((unsigned int)(MT6353_PMIC_REG_BASE+0x060E))
#define MT6353_BUCK_VPROC_LPW_CON0             ((unsigned int)(MT6353_PMIC_REG_BASE+0x0610))
#define MT6353_BUCK_VS1_HWM_CON0               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0612))
#define MT6353_BUCK_VS1_HWM_CON0_SET           ((unsigned int)(MT6353_PMIC_REG_BASE+0x0614))
#define MT6353_BUCK_VS1_HWM_CON0_CLR           ((unsigned int)(MT6353_PMIC_REG_BASE+0x0616))
#define MT6353_BUCK_VS1_HWM_CON1               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0618))
#define MT6353_BUCK_VS1_EN_CON0                ((unsigned int)(MT6353_PMIC_REG_BASE+0x061A))
#define MT6353_BUCK_VS1_SFCHG_CON0             ((unsigned int)(MT6353_PMIC_REG_BASE+0x061C))
#define MT6353_BUCK_VS1_VOL_CON0               ((unsigned int)(MT6353_PMIC_REG_BASE+0x061E))
#define MT6353_BUCK_VS1_VOL_CON1               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0620))
#define MT6353_BUCK_VS1_VOL_CON2               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0622))
#define MT6353_BUCK_VS1_VOL_CON3               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0624))
#define MT6353_BUCK_VS1_LPW_CON0               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0626))
#define MT6353_BUCK_VCORE_HWM_CON0             ((unsigned int)(MT6353_PMIC_REG_BASE+0x0628))
#define MT6353_BUCK_VCORE_HWM_CON1             ((unsigned int)(MT6353_PMIC_REG_BASE+0x062A))
#define MT6353_BUCK_VCORE_EN_CON0              ((unsigned int)(MT6353_PMIC_REG_BASE+0x062C))
#define MT6353_BUCK_VCORE_SFCHG_CON0           ((unsigned int)(MT6353_PMIC_REG_BASE+0x062E))
#define MT6353_BUCK_VCORE_VOL_CON0             ((unsigned int)(MT6353_PMIC_REG_BASE+0x0630))
#define MT6353_BUCK_VCORE_VOL_CON1             ((unsigned int)(MT6353_PMIC_REG_BASE+0x0632))
#define MT6353_BUCK_VCORE_VOL_CON2             ((unsigned int)(MT6353_PMIC_REG_BASE+0x0634))
#define MT6353_BUCK_VCORE_VOL_CON3             ((unsigned int)(MT6353_PMIC_REG_BASE+0x0636))
#define MT6353_BUCK_VCORE_LPW_CON0             ((unsigned int)(MT6353_PMIC_REG_BASE+0x0638))
#define MT6353_BUCK_VCORE2_HWM_CON0            ((unsigned int)(MT6353_PMIC_REG_BASE+0x063A))
#define MT6353_BUCK_VCORE2_HWM_CON1            ((unsigned int)(MT6353_PMIC_REG_BASE+0x063C))
#define MT6353_BUCK_VCORE2_EN_CON0             ((unsigned int)(MT6353_PMIC_REG_BASE+0x063E))
#define MT6353_BUCK_VCORE2_SFCHG_CON0          ((unsigned int)(MT6353_PMIC_REG_BASE+0x0640))
#define MT6353_BUCK_VCORE2_VOL_CON0            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0642))
#define MT6353_BUCK_VCORE2_VOL_CON1            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0644))
#define MT6353_BUCK_VCORE2_VOL_CON2            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0646))
#define MT6353_BUCK_VCORE2_VOL_CON3            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0648))
#define MT6353_BUCK_VCORE2_LPW_CON0            ((unsigned int)(MT6353_PMIC_REG_BASE+0x064A))
#define MT6353_BUCK_VPA_EN_CON0                ((unsigned int)(MT6353_PMIC_REG_BASE+0x064C))
#define MT6353_BUCK_VPA_SFCHG_CON0             ((unsigned int)(MT6353_PMIC_REG_BASE+0x064E))
#define MT6353_BUCK_VPA_VOL_CON0               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0650))
#define MT6353_BUCK_VPA_VOL_CON1               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0652))
#define MT6353_BUCK_VPA_LPW_CON0               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0654))
#define MT6353_BUCK_K_CON0                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0656))
#define MT6353_BUCK_K_CON1                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0658))
#define MT6353_BUCK_K_CON2                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x065A))
#define MT6353_BUCK_K_CON3                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x065C))
#define MT6353_WDTDBG_CON0                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x065E))
#define MT6353_WDTDBG_CON1                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0660))
#define MT6353_WDTDBG_CON2                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0662))
#define MT6353_WDTDBG_CON3                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0664))
#define MT6353_ZCD_CON0                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0800))
#define MT6353_ZCD_CON1                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0802))
#define MT6353_ZCD_CON2                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0804))
#define MT6353_ZCD_CON3                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0806))
#define MT6353_ZCD_CON4                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0808))
#define MT6353_ZCD_CON5                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x080A))
#define MT6353_LDO_RSV_CON0                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A00))
#define MT6353_LDO_RSV_CON1                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A02))
#define MT6353_LDO_TOP_CON0                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A04))
#define MT6353_LDO_VTCXO24_SW_CON0             ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A06))
#define MT6353_LDO_VXO22_SW_CON0               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A08))
#define MT6353_LDO_OCFB0                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A0A))
#define MT6353_VRTC_CON0                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A0C))
#define MT6353_LDO_SHARE_VCN33_CON0            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A0E))
#define MT6353_LDO_SHARE_VCN33_CON1            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A10))
#define MT6353_LDO3_VCN33_CON0                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A12))
#define MT6353_LDO3_VCN33_OCFB_CON0            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A14))
#define MT6353_LDO_SHARE_VLDO28_CON0           ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A16))
#define MT6353_LDO_SHARE_VLDO28_CON1           ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A18))
#define MT6353_LDO1_VLDO28_CON0                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A1A))
#define MT6353_LDO1_VLDO28_OCFB_CON0           ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A1C))
#define MT6353_LDO_VLDO28_FAST_TRAN_CON0       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A1E))
#define MT6353_LDO_VSRAM_PROC_HWM_CON0         ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A20))
#define MT6353_LDO_VSRAM_PROC_SFCHG_CON0       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A22))
#define MT6353_LDO_VSRAM_PROC_VOL_CON0         ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A24))
#define MT6353_LDO_VSRAM_PROC_VOL_CON1         ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A26))
#define MT6353_LDO_VSRAM_PROC_VOL_CON2         ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A28))
#define MT6353_LDO_VSRAM_PROC_VOL_CON3         ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A2A))
#define MT6353_LDO_VSRAM_PROC_LPW_CON0         ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A2C))
#define MT6353_LDO_BATON_CON0                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A2E))
#define MT6353_LDO2_TREF_CON0                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A30))
#define MT6353_LDO3_VTCXO28_CON0               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A32))
#define MT6353_LDO3_VTCXO28_OCFB_CON0          ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A34))
#define MT6353_LDO3_VTCXO24_CON0               ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A36))
#define MT6353_LDO3_VTCXO24_OCFB_CON0          ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A38))
#define MT6353_LDO3_VXO22_CON0                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A3A))
#define MT6353_LDO3_VXO22_OCFB_CON0            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A3C))
#define MT6353_LDO3_VRF18_CON0                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A3E))
#define MT6353_LDO3_VRF18_OCFB_CON0            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A40))
#define MT6353_LDO3_VRF12_CON0                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A42))
#define MT6353_LDO3_VRF12_OCFB_CON0            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A44))
#define MT6353_LDO_VRF12_FAST_TRAN_CON0        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A46))
#define MT6353_LDO3_VCN28_CON0                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A48))
#define MT6353_LDO3_VCN28_OCFB_CON0            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A4A))
#define MT6353_LDO3_VCN18_CON0                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A4C))
#define MT6353_LDO3_VCN18_OCFB_CON0            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A4E))
#define MT6353_LDO0_VCAMA_CON0                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A50))
#define MT6353_LDO0_VCAMA_OCFB_CON0            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A52))
#define MT6353_LDO0_VCAMIO_CON0                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A54))
#define MT6353_LDO0_VCAMIO_OCFB_CON0           ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A56))
#define MT6353_LDO0_VCAMD_CON0                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A58))
#define MT6353_LDO0_VCAMD_OCFB_CON0            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A5A))
#define MT6353_LDO3_VAUX18_CON0                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A5C))
#define MT6353_LDO3_VAUX18_OCFB_CON0           ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A5E))
#define MT6353_LDO3_VAUD28_CON0                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A60))
#define MT6353_LDO3_VAUD28_OCFB_CON0           ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A62))
#define MT6353_LDO3_VSRAM_PROC_CON0            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A64))
#define MT6353_LDO3_VSRAM_PROC_OCFB_CON0       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A66))
#define MT6353_LDO1_VDRAM_CON0                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A68))
#define MT6353_LDO1_VDRAM_OCFB_CON0            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A6A))
#define MT6353_LDO_VDRAM_FAST_TRAN_CON0        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A6C))
#define MT6353_LDO1_VSIM1_CON0                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A6E))
#define MT6353_LDO1_VSIM1_OCFB_CON0            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A70))
#define MT6353_LDO1_VSIM2_CON0                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A72))
#define MT6353_LDO1_VSIM2_OCFB_CON0            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A74))
#define MT6353_LDO1_VIO28_CON0                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A76))
#define MT6353_LDO1_VIO28_OCFB_CON0            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A78))
#define MT6353_LDO1_VMC_CON0                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A7A))
#define MT6353_LDO1_VMC_OCFB_CON0              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A7C))
#define MT6353_LDO_VMC_FAST_TRAN_CON0          ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A7E))
#define MT6353_LDO1_VMCH_CON0                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A80))
#define MT6353_LDO1_VMCH_OCFB_CON0             ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A82))
#define MT6353_LDO_VMCH_FAST_TRAN_CON0         ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A84))
#define MT6353_LDO1_VUSB33_CON0                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A86))
#define MT6353_LDO1_VUSB33_OCFB_CON0           ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A88))
#define MT6353_LDO1_VEMC33_CON0                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A8A))
#define MT6353_LDO1_VEMC33_OCFB_CON0           ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A8C))
#define MT6353_LDO_VEMC33_FAST_TRAN_CON0       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A8E))
#define MT6353_LDO1_VIO18_CON0                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A90))
#define MT6353_LDO1_VIO18_OCFB_CON0            ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A92))
#define MT6353_LDO0_VIBR_CON0                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A94))
#define MT6353_LDO0_VIBR_OCFB_CON0             ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A96))
#define MT6353_ALDO_ANA_CON0                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A98))
#define MT6353_ALDO_ANA_CON1                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A9A))
#define MT6353_ALDO_ANA_CON2                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A9C))
#define MT6353_ALDO_ANA_CON3                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0A9E))
#define MT6353_ALDO_ANA_CON4                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AA0))
#define MT6353_ALDO_ANA_CON5                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AA2))
#define MT6353_ALDO_ANA_CON6                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AA4))
#define MT6353_DLDO_ANA_CON0                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AA6))
#define MT6353_DLDO_ANA_CON1                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AA8))
#define MT6353_DLDO_ANA_CON2                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AAA))
#define MT6353_DLDO_ANA_CON3                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AAC))
#define MT6353_DLDO_ANA_CON4                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AAE))
#define MT6353_DLDO_ANA_CON5                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AB0))
#define MT6353_DLDO_ANA_CON6                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AB2))
#define MT6353_DLDO_ANA_CON7                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AB4))
#define MT6353_DLDO_ANA_CON8                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AB6))
#define MT6353_DLDO_ANA_CON9                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AB8))
#define MT6353_DLDO_ANA_CON10                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0ABA))
#define MT6353_DLDO_ANA_CON11                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0ABC))
#define MT6353_DLDO_ANA_CON12                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0ABE))
#define MT6353_DLDO_ANA_CON13                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AC0))
#define MT6353_DLDO_ANA_CON14                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AC2))
#define MT6353_DLDO_ANA_CON15                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AC4))
#define MT6353_DLDO_ANA_CON16                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AC6))
#define MT6353_DLDO_ANA_CON17                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AC8))
#define MT6353_SLDO_ANA_CON0                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0ACA))
#define MT6353_SLDO_ANA_CON1                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0ACC))
#define MT6353_SLDO_ANA_CON2                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0ACE))
#define MT6353_SLDO_ANA_CON3                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AD0))
#define MT6353_SLDO_ANA_CON4                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AD2))
#define MT6353_SLDO_ANA_CON5                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AD4))
#define MT6353_SLDO_ANA_CON6                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AD6))
#define MT6353_SLDO_ANA_CON7                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0AD8))
#define MT6353_SLDO_ANA_CON8                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0ADA))
#define MT6353_SLDO_ANA_CON9                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0ADC))
#define MT6353_SLDO_ANA_CON10                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0ADE))
#define MT6353_OTP_CON0                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C00))
#define MT6353_OTP_CON1                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C02))
#define MT6353_OTP_CON2                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C04))
#define MT6353_OTP_CON3                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C06))
#define MT6353_OTP_CON4                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C08))
#define MT6353_OTP_CON5                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C0A))
#define MT6353_OTP_CON6                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C0C))
#define MT6353_OTP_CON7                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C0E))
#define MT6353_OTP_CON8                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C10))
#define MT6353_OTP_CON9                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C12))
#define MT6353_OTP_CON10                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C14))
#define MT6353_OTP_CON11                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C16))
#define MT6353_OTP_CON12                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C18))
#define MT6353_OTP_CON13                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C1A))
#define MT6353_OTP_CON14                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C1C))
#define MT6353_OTP_DOUT_0_15                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C1E))
#define MT6353_OTP_DOUT_16_31                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C20))
#define MT6353_OTP_DOUT_32_47                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C22))
#define MT6353_OTP_DOUT_48_63                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C24))
#define MT6353_OTP_DOUT_64_79                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C26))
#define MT6353_OTP_DOUT_80_95                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C28))
#define MT6353_OTP_DOUT_96_111                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C2A))
#define MT6353_OTP_DOUT_112_127                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C2C))
#define MT6353_OTP_DOUT_128_143                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C2E))
#define MT6353_OTP_DOUT_144_159                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C30))
#define MT6353_OTP_DOUT_160_175                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C32))
#define MT6353_OTP_DOUT_176_191                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C34))
#define MT6353_OTP_DOUT_192_207                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C36))
#define MT6353_OTP_DOUT_208_223                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C38))
#define MT6353_OTP_DOUT_224_239                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C3A))
#define MT6353_OTP_DOUT_240_255                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C3C))
#define MT6353_OTP_DOUT_256_271                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C3E))
#define MT6353_OTP_DOUT_272_287                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C40))
#define MT6353_OTP_DOUT_288_303                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C42))
#define MT6353_OTP_DOUT_304_319                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C44))
#define MT6353_OTP_DOUT_320_335                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C46))
#define MT6353_OTP_DOUT_336_351                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C48))
#define MT6353_OTP_DOUT_352_367                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C4A))
#define MT6353_OTP_DOUT_368_383                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C4C))
#define MT6353_OTP_DOUT_384_399                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C4E))
#define MT6353_OTP_DOUT_400_415                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C50))
#define MT6353_OTP_DOUT_416_431                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C52))
#define MT6353_OTP_DOUT_432_447                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C54))
#define MT6353_OTP_DOUT_448_463                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C56))
#define MT6353_OTP_DOUT_464_479                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C58))
#define MT6353_OTP_DOUT_480_495                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C5A))
#define MT6353_OTP_DOUT_496_511                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C5C))
#define MT6353_OTP_VAL_0_15                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C5E))
#define MT6353_OTP_VAL_16_31                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C60))
#define MT6353_OTP_VAL_32_47                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C62))
#define MT6353_OTP_VAL_48_63                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C64))
#define MT6353_OTP_VAL_64_79                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C66))
#define MT6353_OTP_VAL_80_95                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C68))
#define MT6353_OTP_VAL_96_111                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C6A))
#define MT6353_OTP_VAL_112_127                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C6C))
#define MT6353_OTP_VAL_128_143                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C6E))
#define MT6353_OTP_VAL_144_159                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C70))
#define MT6353_OTP_VAL_160_175                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C72))
#define MT6353_OTP_VAL_176_191                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C74))
#define MT6353_OTP_VAL_192_207                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C76))
#define MT6353_OTP_VAL_208_223                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C78))
#define MT6353_OTP_VAL_224_239                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C7A))
#define MT6353_OTP_VAL_240_255                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C7C))
#define MT6353_OTP_VAL_256_271                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C7E))
#define MT6353_OTP_VAL_272_287                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C80))
#define MT6353_OTP_VAL_288_303                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C82))
#define MT6353_OTP_VAL_304_319                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C84))
#define MT6353_OTP_VAL_320_335                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C86))
#define MT6353_OTP_VAL_336_351                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C88))
#define MT6353_OTP_VAL_352_367                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C8A))
#define MT6353_OTP_VAL_368_383                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C8C))
#define MT6353_OTP_VAL_384_399                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C8E))
#define MT6353_OTP_VAL_400_415                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C90))
#define MT6353_OTP_VAL_416_431                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C92))
#define MT6353_OTP_VAL_432_447                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C94))
#define MT6353_OTP_VAL_448_463                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C96))
#define MT6353_OTP_VAL_464_479                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C98))
#define MT6353_OTP_VAL_480_495                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C9A))
#define MT6353_OTP_VAL_496_511                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C9C))
#define MT6353_RTC_MIX_CON0                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0C9E))
#define MT6353_RTC_MIX_CON1                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CA0))
#define MT6353_RTC_MIX_CON2                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CA2))
#define MT6353_FGADC_CON0                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CA4))
#define MT6353_FGADC_CON1                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CA6))
#define MT6353_FGADC_CON2                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CA8))
#define MT6353_FGADC_CON3                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CAA))
#define MT6353_FGADC_CON4                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CAC))
#define MT6353_FGADC_CON5                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CAE))
#define MT6353_FGADC_CON6                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CB0))
#define MT6353_FGADC_CON7                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CB2))
#define MT6353_FGADC_CON8                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CB4))
#define MT6353_FGADC_CON9                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CB6))
#define MT6353_FGADC_CON10                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CB8))
#define MT6353_FGADC_CON11                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CBA))
#define MT6353_FGADC_CON12                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CBC))
#define MT6353_FGADC_CON13                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CBE))
#define MT6353_FGADC_CON14                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CC0))
#define MT6353_FGADC_CON15                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CC2))
#define MT6353_FGADC_CON16                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CC4))
#define MT6353_FGADC_CON17                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CC6))
#define MT6353_FGADC_CON18                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CC8))
#define MT6353_FGADC_CON19                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CCA))
#define MT6353_FGADC_CON20                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CCC))
#define MT6353_FGADC_CON21                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CCE))
#define MT6353_FGADC_CON22                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CD0))
#define MT6353_FGADC_CON23                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CD2))
#define MT6353_FGADC_CON24                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CD4))
#define MT6353_FGADC_CON25                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CD6))
#define MT6353_FGADC_CON26                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CD8))
#define MT6353_FGADC_CON27                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CDA))
#define MT6353_FGADC_CON28                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CDC))
#define MT6353_FGADC_CON29                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CDE))
#define MT6353_FGADC_CON30                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CE0))
#define MT6353_FGADC_CON31                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CE2))
#define MT6353_FGADC_CON32                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CE4))
#define MT6353_FGADC_CON33                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CE6))
#define MT6353_SYSTEM_INFO_CON0                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CE8))
#define MT6353_SYSTEM_INFO_CON1                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CEA))
#define MT6353_SYSTEM_INFO_CON2                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CEC))
#define MT6353_SYSTEM_INFO_CON3                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CEE))
#define MT6353_SYSTEM_INFO_CON4                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CF0))
#define MT6353_AUDDEC_ANA_CON0                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CF2))
#define MT6353_AUDDEC_ANA_CON1                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CF4))
#define MT6353_AUDDEC_ANA_CON2                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CF6))
#define MT6353_AUDDEC_ANA_CON3                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CF8))
#define MT6353_AUDDEC_ANA_CON4                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CFA))
#define MT6353_AUDDEC_ANA_CON5                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CFC))
#define MT6353_AUDDEC_ANA_CON6                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0CFE))
#define MT6353_AUDDEC_ANA_CON7                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D00))
#define MT6353_AUDDEC_ANA_CON8                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D02))
#define MT6353_AUDENC_ANA_CON0                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D04))
#define MT6353_AUDENC_ANA_CON1                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D06))
#define MT6353_AUDENC_ANA_CON2                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D08))
#define MT6353_AUDENC_ANA_CON3                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D0A))
#define MT6353_AUDENC_ANA_CON4                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D0C))
#define MT6353_AUDENC_ANA_CON5                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D0E))
#define MT6353_AUDENC_ANA_CON6                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D10))
#define MT6353_AUDENC_ANA_CON7                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D12))
#define MT6353_AUDENC_ANA_CON8                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D14))
#define MT6353_AUDENC_ANA_CON9                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D16))
#define MT6353_AUDENC_ANA_CON10                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D18))
#define MT6353_AUDNCP_CLKDIV_CON0              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D1A))
#define MT6353_AUDNCP_CLKDIV_CON1              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D1C))
#define MT6353_AUDNCP_CLKDIV_CON2              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D1E))
#define MT6353_AUDNCP_CLKDIV_CON3              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D20))
#define MT6353_AUDNCP_CLKDIV_CON4              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D22))
#define MT6353_DCXO_CW00                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D24))
#define MT6353_DCXO_CW00_SET                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D26))
#define MT6353_DCXO_CW00_CLR                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D28))
#define MT6353_DCXO_CW01                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D2A))
#define MT6353_DCXO_CW02                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D2C))
#define MT6353_DCXO_CW03                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D2E))
#define MT6353_DCXO_CW04                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D30))
#define MT6353_DCXO_CW05                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D32))
#define MT6353_DCXO_CW06                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D34))
#define MT6353_DCXO_CW07                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D36))
#define MT6353_DCXO_CW08                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D38))
#define MT6353_DCXO_CW09                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D3A))
#define MT6353_DCXO_CW10                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D3C))
#define MT6353_DCXO_CW11                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D3E))
#define MT6353_DCXO_CW12                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D40))
#define MT6353_DCXO_CW13                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D42))
#define MT6353_DCXO_CW14                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D44))
#define MT6353_DCXO_CW15                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D46))
#define MT6353_DCXO_CW16                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0D48))
#define MT6353_AUXADC_ADC0                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E00))
#define MT6353_AUXADC_ADC1                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E02))
#define MT6353_AUXADC_ADC2                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E04))
#define MT6353_AUXADC_ADC3                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E06))
#define MT6353_AUXADC_ADC4                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E08))
#define MT6353_AUXADC_ADC5                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E0A))
#define MT6353_AUXADC_ADC6                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E0C))
#define MT6353_AUXADC_ADC7                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E0E))
#define MT6353_AUXADC_ADC8                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E10))
#define MT6353_AUXADC_ADC9                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E12))
#define MT6353_AUXADC_ADC10                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E14))
#define MT6353_AUXADC_ADC11                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E16))
#define MT6353_AUXADC_ADC12                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E18))
#define MT6353_AUXADC_ADC13                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E1A))
#define MT6353_AUXADC_ADC14                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E1C))
#define MT6353_AUXADC_ADC15                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E1E))
#define MT6353_AUXADC_ADC16                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E20))
#define MT6353_AUXADC_ADC17                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E22))
#define MT6353_AUXADC_ADC18                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E24))
#define MT6353_AUXADC_ADC19                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E26))
#define MT6353_AUXADC_ADC20                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E28))
#define MT6353_AUXADC_ADC21                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E2A))
#define MT6353_AUXADC_ADC22                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E2C))
#define MT6353_AUXADC_ADC23                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E2E))
#define MT6353_AUXADC_ADC24                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E30))
#define MT6353_AUXADC_ADC25                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E32))
#define MT6353_AUXADC_ADC26                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E34))
#define MT6353_AUXADC_ADC27                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E36))
#define MT6353_AUXADC_ADC28                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E38))
#define MT6353_AUXADC_ADC29                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E3A))
#define MT6353_AUXADC_ADC30                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E3C))
#define MT6353_AUXADC_ADC31                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E3E))
#define MT6353_AUXADC_ADC32                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E40))
#define MT6353_AUXADC_ADC33                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E42))
#define MT6353_AUXADC_ADC34                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E44))
#define MT6353_AUXADC_ADC35                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E46))
#define MT6353_AUXADC_ADC36                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E48))
#define MT6353_AUXADC_ADC37                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E4A))
#define MT6353_AUXADC_ADC38                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E4C))
#define MT6353_AUXADC_ADC39                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E4E))
#define MT6353_AUXADC_ADC40                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E50))
#define MT6353_AUXADC_BUF0                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E52))
#define MT6353_AUXADC_BUF1                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E54))
#define MT6353_AUXADC_BUF2                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E56))
#define MT6353_AUXADC_BUF3                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E58))
#define MT6353_AUXADC_BUF4                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E5A))
#define MT6353_AUXADC_BUF5                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E5C))
#define MT6353_AUXADC_BUF6                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E5E))
#define MT6353_AUXADC_BUF7                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E60))
#define MT6353_AUXADC_BUF8                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E62))
#define MT6353_AUXADC_BUF9                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E64))
#define MT6353_AUXADC_BUF10                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E66))
#define MT6353_AUXADC_BUF11                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E68))
#define MT6353_AUXADC_BUF12                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E6A))
#define MT6353_AUXADC_BUF13                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E6C))
#define MT6353_AUXADC_BUF14                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E6E))
#define MT6353_AUXADC_BUF15                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E70))
#define MT6353_AUXADC_BUF16                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E72))
#define MT6353_AUXADC_BUF17                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E74))
#define MT6353_AUXADC_BUF18                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E76))
#define MT6353_AUXADC_BUF19                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E78))
#define MT6353_AUXADC_BUF20                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E7A))
#define MT6353_AUXADC_BUF21                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E7C))
#define MT6353_AUXADC_BUF22                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E7E))
#define MT6353_AUXADC_BUF23                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E80))
#define MT6353_AUXADC_BUF24                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E82))
#define MT6353_AUXADC_BUF25                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E84))
#define MT6353_AUXADC_BUF26                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E86))
#define MT6353_AUXADC_BUF27                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E88))
#define MT6353_AUXADC_BUF28                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E8A))
#define MT6353_AUXADC_BUF29                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E8C))
#define MT6353_AUXADC_BUF30                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E8E))
#define MT6353_AUXADC_BUF31                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E90))
#define MT6353_AUXADC_STA0                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E92))
#define MT6353_AUXADC_STA1                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E94))
#define MT6353_AUXADC_STA2                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E96))
#define MT6353_AUXADC_RQST0                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E98))
#define MT6353_AUXADC_RQST0_SET                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E9A))
#define MT6353_AUXADC_RQST0_CLR                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E9C))
#define MT6353_AUXADC_RQST1                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0E9E))
#define MT6353_AUXADC_RQST1_SET                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EA0))
#define MT6353_AUXADC_RQST1_CLR                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EA2))
#define MT6353_AUXADC_CON0                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EA4))
#define MT6353_AUXADC_CON0_SET                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EA6))
#define MT6353_AUXADC_CON0_CLR                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EA8))
#define MT6353_AUXADC_CON1                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EAA))
#define MT6353_AUXADC_CON2                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EAC))
#define MT6353_AUXADC_CON3                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EAE))
#define MT6353_AUXADC_CON4                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EB0))
#define MT6353_AUXADC_CON5                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EB2))
#define MT6353_AUXADC_CON6                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EB4))
#define MT6353_AUXADC_CON7                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EB6))
#define MT6353_AUXADC_CON8                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EB8))
#define MT6353_AUXADC_CON9                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EBA))
#define MT6353_AUXADC_CON10                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EBC))
#define MT6353_AUXADC_CON11                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EBE))
#define MT6353_AUXADC_CON12                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EC0))
#define MT6353_AUXADC_CON13                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EC2))
#define MT6353_AUXADC_CON14                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EC4))
#define MT6353_AUXADC_CON15                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EC6))
#define MT6353_AUXADC_CON16                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EC8))
#define MT6353_AUXADC_AUTORPT0                 ((unsigned int)(MT6353_PMIC_REG_BASE+0x0ECA))
#define MT6353_AUXADC_LBAT0                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0ECC))
#define MT6353_AUXADC_LBAT1                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0ECE))
#define MT6353_AUXADC_LBAT2                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0ED0))
#define MT6353_AUXADC_LBAT3                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0ED2))
#define MT6353_AUXADC_LBAT4                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0ED4))
#define MT6353_AUXADC_LBAT5                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0ED6))
#define MT6353_AUXADC_LBAT6                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0ED8))
#define MT6353_AUXADC_ACCDET                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EDA))
#define MT6353_AUXADC_THR0                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EDC))
#define MT6353_AUXADC_THR1                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EDE))
#define MT6353_AUXADC_THR2                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EE0))
#define MT6353_AUXADC_THR3                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EE2))
#define MT6353_AUXADC_THR4                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EE4))
#define MT6353_AUXADC_THR5                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EE6))
#define MT6353_AUXADC_THR6                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EE8))
#define MT6353_AUXADC_EFUSE0                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EEA))
#define MT6353_AUXADC_EFUSE1                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EEC))
#define MT6353_AUXADC_EFUSE2                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EEE))
#define MT6353_AUXADC_EFUSE3                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EF0))
#define MT6353_AUXADC_EFUSE4                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EF2))
#define MT6353_AUXADC_EFUSE5                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EF4))
#define MT6353_AUXADC_DBG0                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EF6))
#define MT6353_AUXADC_IMP0                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EF8))
#define MT6353_AUXADC_IMP1                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EFA))
#define MT6353_AUXADC_VISMPS0_1                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EFC))
#define MT6353_AUXADC_VISMPS0_2                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0EFE))
#define MT6353_AUXADC_VISMPS0_3                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F00))
#define MT6353_AUXADC_VISMPS0_4                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F02))
#define MT6353_AUXADC_VISMPS0_5                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F04))
#define MT6353_AUXADC_VISMPS0_6                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F06))
#define MT6353_AUXADC_VISMPS0_7                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F08))
#define MT6353_AUXADC_LBAT2_1                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F0A))
#define MT6353_AUXADC_LBAT2_2                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F0C))
#define MT6353_AUXADC_LBAT2_3                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F0E))
#define MT6353_AUXADC_LBAT2_4                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F10))
#define MT6353_AUXADC_LBAT2_5                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F12))
#define MT6353_AUXADC_LBAT2_6                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F14))
#define MT6353_AUXADC_LBAT2_7                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F16))
#define MT6353_AUXADC_MDBG_0                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F18))
#define MT6353_AUXADC_MDBG_1                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F1A))
#define MT6353_AUXADC_MDBG_2                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F1C))
#define MT6353_AUXADC_MDRT_0                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F1E))
#define MT6353_AUXADC_MDRT_1                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F20))
#define MT6353_AUXADC_MDRT_2                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F22))
#define MT6353_AUXADC_DCXO_MDRT_0              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F24))
#define MT6353_AUXADC_DCXO_MDRT_1              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F26))
#define MT6353_AUXADC_DCXO_MDRT_2              ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F28))
#define MT6353_AUXADC_NAG_0                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F2A))
#define MT6353_AUXADC_NAG_1                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F2C))
#define MT6353_AUXADC_NAG_2                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F2E))
#define MT6353_AUXADC_NAG_3                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F30))
#define MT6353_AUXADC_NAG_4                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F32))
#define MT6353_AUXADC_NAG_5                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F34))
#define MT6353_AUXADC_NAG_6                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F36))
#define MT6353_AUXADC_NAG_7                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F38))
#define MT6353_AUXADC_NAG_8                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F3A))
#define MT6353_AUXADC_TYPEC_H_1                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F3C))
#define MT6353_AUXADC_TYPEC_H_2                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F3E))
#define MT6353_AUXADC_TYPEC_H_3                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F40))
#define MT6353_AUXADC_TYPEC_H_4                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F42))
#define MT6353_AUXADC_TYPEC_H_5                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F44))
#define MT6353_AUXADC_TYPEC_H_6                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F46))
#define MT6353_AUXADC_TYPEC_H_7                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F48))
#define MT6353_AUXADC_TYPEC_L_1                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F4A))
#define MT6353_AUXADC_TYPEC_L_2                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F4C))
#define MT6353_AUXADC_TYPEC_L_3                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F4E))
#define MT6353_AUXADC_TYPEC_L_4                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F50))
#define MT6353_AUXADC_TYPEC_L_5                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F52))
#define MT6353_AUXADC_TYPEC_L_6                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F54))
#define MT6353_AUXADC_TYPEC_L_7                ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F56))
#define MT6353_ACCDET_CON0                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F58))
#define MT6353_ACCDET_CON1                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F5A))
#define MT6353_ACCDET_CON2                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F5C))
#define MT6353_ACCDET_CON3                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F5E))
#define MT6353_ACCDET_CON4                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F60))
#define MT6353_ACCDET_CON5                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F62))
#define MT6353_ACCDET_CON6                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F64))
#define MT6353_ACCDET_CON7                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F66))
#define MT6353_ACCDET_CON8                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F68))
#define MT6353_ACCDET_CON9                     ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F6A))
#define MT6353_ACCDET_CON10                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F6C))
#define MT6353_ACCDET_CON11                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F6E))
#define MT6353_ACCDET_CON12                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F70))
#define MT6353_ACCDET_CON13                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F72))
#define MT6353_ACCDET_CON14                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F74))
#define MT6353_ACCDET_CON15                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F76))
#define MT6353_ACCDET_CON16                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F78))
#define MT6353_ACCDET_CON17                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F7A))
#define MT6353_ACCDET_CON18                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F7C))
#define MT6353_ACCDET_CON19                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F7E))
#define MT6353_ACCDET_CON20                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F80))
#define MT6353_ACCDET_CON21                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F82))
#define MT6353_ACCDET_CON22                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F84))
#define MT6353_ACCDET_CON23                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F86))
#define MT6353_ACCDET_CON24                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F88))
#define MT6353_ACCDET_CON25                    ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F8A))
#define MT6353_CHR_CON0                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F8C))
#define MT6353_CHR_CON1                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F8E))
#define MT6353_CHR_CON2                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F90))
#define MT6353_CHR_CON3                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F92))
#define MT6353_CHR_CON4                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F94))
#define MT6353_CHR_CON5                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F96))
#define MT6353_CHR_CON6                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F98))
#define MT6353_CHR_CON7                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F9A))
#define MT6353_CHR_CON8                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F9C))
#define MT6353_CHR_CON9                        ((unsigned int)(MT6353_PMIC_REG_BASE+0x0F9E))
#define MT6353_CHR_CON10                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FA0))
#define MT6353_CHR_CON11                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FA2))
#define MT6353_CHR_CON12                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FA4))
#define MT6353_CHR_CON13                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FA6))
#define MT6353_CHR_CON14                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FA8))
#define MT6353_CHR_CON15                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FAA))
#define MT6353_CHR_CON16                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FAC))
#define MT6353_CHR_CON17                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FAE))
#define MT6353_CHR_CON18                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FB0))
#define MT6353_CHR_CON19                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FB2))
#define MT6353_CHR_CON20                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FB4))
#define MT6353_CHR_CON21                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FB6))
#define MT6353_CHR_CON22                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FB8))
#define MT6353_CHR_CON23                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FBA))
#define MT6353_CHR_CON24                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FBC))
#define MT6353_CHR_CON25                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FBE))
#define MT6353_CHR_CON26                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FC0))
#define MT6353_CHR_CON27                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FC2))
#define MT6353_CHR_CON28                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FC4))
#define MT6353_CHR_CON29                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FC6))
#define MT6353_CHR_CON30                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FC8))
#define MT6353_CHR_CON31                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FCA))
#define MT6353_CHR_CON32                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FCC))
#define MT6353_CHR_CON33                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FCE))
#define MT6353_CHR_CON34                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FD0))
#define MT6353_CHR_CON35                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FD2))
#define MT6353_CHR_CON36                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FD4))
#define MT6353_CHR_CON37                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FD6))
#define MT6353_CHR_CON38                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FD8))
#define MT6353_CHR_CON39                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FDA))
#define MT6353_CHR_CON40                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FDC))
#define MT6353_CHR_CON41                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FDE))
#define MT6353_CHR_CON42                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FE0))
#define MT6353_BATON_CON0                      ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FE2))
#define MT6353_CHR_CON43                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FE4))
#define MT6353_CHR_CON44                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FE6))
#define MT6353_CHR_CON48                       ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FE8))
#define MT6353_EOSC_CALI_CON0                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FEA))
#define MT6353_EOSC_CALI_CON1                  ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FEC))
#define MT6353_VRTC_PWM_CON0                   ((unsigned int)(MT6353_PMIC_REG_BASE+0x0FEE))
/* mask is HEX;  shift is Integer */
#define PMIC_USBDL_ADDR                                           MT6353_STRUP_CON0
#define PMIC_USBDL_MASK                                           0x1
#define PMIC_USBDL_SHIFT                                          0
#define PMIC_VTCXO_CONFIG_ADDR                                    MT6353_STRUP_CON0
#define PMIC_VTCXO_CONFIG_MASK                                    0x1
#define PMIC_VTCXO_CONFIG_SHIFT                                   1
#define PMIC_RG_THR_DET_DIS_ADDR                                  MT6353_STRUP_CON0
#define PMIC_RG_THR_DET_DIS_MASK                                  0x1
#define PMIC_RG_THR_DET_DIS_SHIFT                                 12
#define PMIC_RG_THR_TEST_ADDR                                     MT6353_STRUP_CON0
#define PMIC_RG_THR_TEST_MASK                                     0x3
#define PMIC_RG_THR_TEST_SHIFT                                    13
#define PMIC_RG_STRUP_THER_DEB_RMAX_ADDR                          MT6353_STRUP_CON1
#define PMIC_RG_STRUP_THER_DEB_RMAX_MASK                          0xFFFF
#define PMIC_RG_STRUP_THER_DEB_RMAX_SHIFT                         0
#define PMIC_RG_STRUP_THER_DEB_FMAX_ADDR                          MT6353_STRUP_CON2
#define PMIC_RG_STRUP_THER_DEB_FMAX_MASK                          0xFFFF
#define PMIC_RG_STRUP_THER_DEB_FMAX_SHIFT                         0
#define PMIC_DDUVLO_DEB_EN_ADDR                                   MT6353_STRUP_CON3
#define PMIC_DDUVLO_DEB_EN_MASK                                   0x1
#define PMIC_DDUVLO_DEB_EN_SHIFT                                  0
#define PMIC_RG_PWRBB_DEB_EN_ADDR                                 MT6353_STRUP_CON3
#define PMIC_RG_PWRBB_DEB_EN_MASK                                 0x1
#define PMIC_RG_PWRBB_DEB_EN_SHIFT                                1
#define PMIC_RG_STRUP_OSC_EN_ADDR                                 MT6353_STRUP_CON3
#define PMIC_RG_STRUP_OSC_EN_MASK                                 0x1
#define PMIC_RG_STRUP_OSC_EN_SHIFT                                2
#define PMIC_RG_STRUP_OSC_EN_SEL_ADDR                             MT6353_STRUP_CON3
#define PMIC_RG_STRUP_OSC_EN_SEL_MASK                             0x1
#define PMIC_RG_STRUP_OSC_EN_SEL_SHIFT                            3
#define PMIC_RG_STRUP_FT_CTRL_ADDR                                MT6353_STRUP_CON3
#define PMIC_RG_STRUP_FT_CTRL_MASK                                0x3
#define PMIC_RG_STRUP_FT_CTRL_SHIFT                               4
#define PMIC_RG_STRUP_PWRON_FORCE_ADDR                            MT6353_STRUP_CON3
#define PMIC_RG_STRUP_PWRON_FORCE_MASK                            0x1
#define PMIC_RG_STRUP_PWRON_FORCE_SHIFT                           6
#define PMIC_RG_BIASGEN_FORCE_ADDR                                MT6353_STRUP_CON3
#define PMIC_RG_BIASGEN_FORCE_MASK                                0x1
#define PMIC_RG_BIASGEN_FORCE_SHIFT                               7
#define PMIC_RG_STRUP_PWRON_ADDR                                  MT6353_STRUP_CON3
#define PMIC_RG_STRUP_PWRON_MASK                                  0x1
#define PMIC_RG_STRUP_PWRON_SHIFT                                 8
#define PMIC_RG_STRUP_PWRON_SEL_ADDR                              MT6353_STRUP_CON3
#define PMIC_RG_STRUP_PWRON_SEL_MASK                              0x1
#define PMIC_RG_STRUP_PWRON_SEL_SHIFT                             9
#define PMIC_RG_BIASGEN_ADDR                                      MT6353_STRUP_CON3
#define PMIC_RG_BIASGEN_MASK                                      0x1
#define PMIC_RG_BIASGEN_SHIFT                                     10
#define PMIC_RG_BIASGEN_SEL_ADDR                                  MT6353_STRUP_CON3
#define PMIC_RG_BIASGEN_SEL_MASK                                  0x1
#define PMIC_RG_BIASGEN_SEL_SHIFT                                 11
#define PMIC_RG_RTC_XOSC32_ENB_ADDR                               MT6353_STRUP_CON3
#define PMIC_RG_RTC_XOSC32_ENB_MASK                               0x1
#define PMIC_RG_RTC_XOSC32_ENB_SHIFT                              12
#define PMIC_RG_RTC_XOSC32_ENB_SEL_ADDR                           MT6353_STRUP_CON3
#define PMIC_RG_RTC_XOSC32_ENB_SEL_MASK                           0x1
#define PMIC_RG_RTC_XOSC32_ENB_SEL_SHIFT                          13
#define PMIC_STRUP_DIG_IO_PG_FORCE_ADDR                           MT6353_STRUP_CON3
#define PMIC_STRUP_DIG_IO_PG_FORCE_MASK                           0x1
#define PMIC_STRUP_DIG_IO_PG_FORCE_SHIFT                          15
#define PMIC_CLR_JUST_RST_ADDR                                    MT6353_STRUP_CON4
#define PMIC_CLR_JUST_RST_MASK                                    0x1
#define PMIC_CLR_JUST_RST_SHIFT                                   4
#define PMIC_UVLO_L2H_DEB_EN_ADDR                                 MT6353_STRUP_CON4
#define PMIC_UVLO_L2H_DEB_EN_MASK                                 0x1
#define PMIC_UVLO_L2H_DEB_EN_SHIFT                                5
#define PMIC_JUST_PWRKEY_RST_ADDR                                 MT6353_STRUP_CON4
#define PMIC_JUST_PWRKEY_RST_MASK                                 0x1
#define PMIC_JUST_PWRKEY_RST_SHIFT                                14
#define PMIC_DA_QI_OSC_EN_ADDR                                    MT6353_STRUP_CON4
#define PMIC_DA_QI_OSC_EN_MASK                                    0x1
#define PMIC_DA_QI_OSC_EN_SHIFT                                   15
#define PMIC_RG_STRUP_EXT_PMIC_EN_ADDR                            MT6353_STRUP_CON5
#define PMIC_RG_STRUP_EXT_PMIC_EN_MASK                            0x1
#define PMIC_RG_STRUP_EXT_PMIC_EN_SHIFT                           0
#define PMIC_RG_STRUP_EXT_PMIC_SEL_ADDR                           MT6353_STRUP_CON5
#define PMIC_RG_STRUP_EXT_PMIC_SEL_MASK                           0x1
#define PMIC_RG_STRUP_EXT_PMIC_SEL_SHIFT                          1
#define PMIC_STRUP_CON8_RSV0_ADDR                                 MT6353_STRUP_CON5
#define PMIC_STRUP_CON8_RSV0_MASK                                 0x7F
#define PMIC_STRUP_CON8_RSV0_SHIFT                                8
#define PMIC_DA_QI_EXT_PMIC_EN_ADDR                               MT6353_STRUP_CON5
#define PMIC_DA_QI_EXT_PMIC_EN_MASK                               0x1
#define PMIC_DA_QI_EXT_PMIC_EN_SHIFT                              15
#define PMIC_RG_STRUP_AUXADC_START_SW_ADDR                        MT6353_STRUP_CON6
#define PMIC_RG_STRUP_AUXADC_START_SW_MASK                        0x1
#define PMIC_RG_STRUP_AUXADC_START_SW_SHIFT                       0
#define PMIC_RG_STRUP_AUXADC_RSTB_SW_ADDR                         MT6353_STRUP_CON6
#define PMIC_RG_STRUP_AUXADC_RSTB_SW_MASK                         0x1
#define PMIC_RG_STRUP_AUXADC_RSTB_SW_SHIFT                        1
#define PMIC_RG_STRUP_AUXADC_START_SEL_ADDR                       MT6353_STRUP_CON6
#define PMIC_RG_STRUP_AUXADC_START_SEL_MASK                       0x1
#define PMIC_RG_STRUP_AUXADC_START_SEL_SHIFT                      2
#define PMIC_RG_STRUP_AUXADC_RSTB_SEL_ADDR                        MT6353_STRUP_CON6
#define PMIC_RG_STRUP_AUXADC_RSTB_SEL_MASK                        0x1
#define PMIC_RG_STRUP_AUXADC_RSTB_SEL_SHIFT                       3
#define PMIC_RG_STRUP_AUXADC_RPCNT_MAX_ADDR                       MT6353_STRUP_CON6
#define PMIC_RG_STRUP_AUXADC_RPCNT_MAX_MASK                       0x7F
#define PMIC_RG_STRUP_AUXADC_RPCNT_MAX_SHIFT                      4
#define PMIC_STRUP_PWROFF_SEQ_EN_ADDR                             MT6353_STRUP_CON7
#define PMIC_STRUP_PWROFF_SEQ_EN_MASK                             0x1
#define PMIC_STRUP_PWROFF_SEQ_EN_SHIFT                            0
#define PMIC_STRUP_PWROFF_PREOFF_EN_ADDR                          MT6353_STRUP_CON7
#define PMIC_STRUP_PWROFF_PREOFF_EN_MASK                          0x1
#define PMIC_STRUP_PWROFF_PREOFF_EN_SHIFT                         1
#define PMIC_STRUP_DIG0_RSV0_ADDR                                 MT6353_STRUP_CON8
#define PMIC_STRUP_DIG0_RSV0_MASK                                 0xF
#define PMIC_STRUP_DIG0_RSV0_SHIFT                                2
#define PMIC_STRUP_DIG1_RSV0_ADDR                                 MT6353_STRUP_CON8
#define PMIC_STRUP_DIG1_RSV0_MASK                                 0x1F
#define PMIC_STRUP_DIG1_RSV0_SHIFT                                6
#define PMIC_RG_RSV_SWREG_ADDR                                    MT6353_STRUP_CON9
#define PMIC_RG_RSV_SWREG_MASK                                    0xFFFF
#define PMIC_RG_RSV_SWREG_SHIFT                                   0
#define PMIC_RG_STRUP_UVLO_U1U2_SEL_ADDR                          MT6353_STRUP_CON10
#define PMIC_RG_STRUP_UVLO_U1U2_SEL_MASK                          0x1
#define PMIC_RG_STRUP_UVLO_U1U2_SEL_SHIFT                         0
#define PMIC_RG_STRUP_UVLO_U1U2_SEL_SWCTRL_ADDR                   MT6353_STRUP_CON10
#define PMIC_RG_STRUP_UVLO_U1U2_SEL_SWCTRL_MASK                   0x1
#define PMIC_RG_STRUP_UVLO_U1U2_SEL_SWCTRL_SHIFT                  1
#define PMIC_RG_STRUP_THR_CLR_ADDR                                MT6353_STRUP_CON11
#define PMIC_RG_STRUP_THR_CLR_MASK                                0x1
#define PMIC_RG_STRUP_THR_CLR_SHIFT                               0
#define PMIC_RG_STRUP_LONG_PRESS_EXT_SEL_ADDR                     MT6353_STRUP_CON12
#define PMIC_RG_STRUP_LONG_PRESS_EXT_SEL_MASK                     0x3
#define PMIC_RG_STRUP_LONG_PRESS_EXT_SEL_SHIFT                    0
#define PMIC_RG_STRUP_LONG_PRESS_EXT_TD_ADDR                      MT6353_STRUP_CON12
#define PMIC_RG_STRUP_LONG_PRESS_EXT_TD_MASK                      0x3
#define PMIC_RG_STRUP_LONG_PRESS_EXT_TD_SHIFT                     2
#define PMIC_RG_STRUP_LONG_PRESS_EXT_EN_ADDR                      MT6353_STRUP_CON12
#define PMIC_RG_STRUP_LONG_PRESS_EXT_EN_MASK                      0x1
#define PMIC_RG_STRUP_LONG_PRESS_EXT_EN_SHIFT                     4
#define PMIC_RG_STRUP_LONG_PRESS_EXT_CHR_CTRL_ADDR                MT6353_STRUP_CON12
#define PMIC_RG_STRUP_LONG_PRESS_EXT_CHR_CTRL_MASK                0x1
#define PMIC_RG_STRUP_LONG_PRESS_EXT_CHR_CTRL_SHIFT               5
#define PMIC_RG_STRUP_LONG_PRESS_EXT_PWRKEY_CTRL_ADDR             MT6353_STRUP_CON12
#define PMIC_RG_STRUP_LONG_PRESS_EXT_PWRKEY_CTRL_MASK             0x1
#define PMIC_RG_STRUP_LONG_PRESS_EXT_PWRKEY_CTRL_SHIFT            6
#define PMIC_RG_STRUP_LONG_PRESS_EXT_PWRBB_CTRL_ADDR              MT6353_STRUP_CON12
#define PMIC_RG_STRUP_LONG_PRESS_EXT_PWRBB_CTRL_MASK              0x1
#define PMIC_RG_STRUP_LONG_PRESS_EXT_PWRBB_CTRL_SHIFT             7
#define PMIC_RG_STRUP_ENVTEM_ADDR                                 MT6353_STRUP_CON12
#define PMIC_RG_STRUP_ENVTEM_MASK                                 0x1
#define PMIC_RG_STRUP_ENVTEM_SHIFT                                14
#define PMIC_RG_STRUP_ENVTEM_CTRL_ADDR                            MT6353_STRUP_CON12
#define PMIC_RG_STRUP_ENVTEM_CTRL_MASK                            0x1
#define PMIC_RG_STRUP_ENVTEM_CTRL_SHIFT                           15
#define PMIC_RG_STRUP_PWRKEY_COUNT_RESET_ADDR                     MT6353_STRUP_CON13
#define PMIC_RG_STRUP_PWRKEY_COUNT_RESET_MASK                     0x1
#define PMIC_RG_STRUP_PWRKEY_COUNT_RESET_SHIFT                    0
#define PMIC_RG_STRUP_VCORE2_PG_H2L_EN_ADDR                       MT6353_STRUP_CON14
#define PMIC_RG_STRUP_VCORE2_PG_H2L_EN_MASK                       0x1
#define PMIC_RG_STRUP_VCORE2_PG_H2L_EN_SHIFT                      4
#define PMIC_RG_STRUP_VMCH_PG_H2L_EN_ADDR                         MT6353_STRUP_CON14
#define PMIC_RG_STRUP_VMCH_PG_H2L_EN_MASK                         0x1
#define PMIC_RG_STRUP_VMCH_PG_H2L_EN_SHIFT                        5
#define PMIC_RG_STRUP_VMC_PG_H2L_EN_ADDR                          MT6353_STRUP_CON14
#define PMIC_RG_STRUP_VMC_PG_H2L_EN_MASK                          0x1
#define PMIC_RG_STRUP_VMC_PG_H2L_EN_SHIFT                         6
#define PMIC_RG_STRUP_VUSB33_PG_H2L_EN_ADDR                       MT6353_STRUP_CON14
#define PMIC_RG_STRUP_VUSB33_PG_H2L_EN_MASK                       0x1
#define PMIC_RG_STRUP_VUSB33_PG_H2L_EN_SHIFT                      7
#define PMIC_RG_STRUP_VSRAM_PROC_PG_H2L_EN_ADDR                   MT6353_STRUP_CON14
#define PMIC_RG_STRUP_VSRAM_PROC_PG_H2L_EN_MASK                   0x1
#define PMIC_RG_STRUP_VSRAM_PROC_PG_H2L_EN_SHIFT                  8
#define PMIC_RG_STRUP_VPROC_PG_H2L_EN_ADDR                        MT6353_STRUP_CON14
#define PMIC_RG_STRUP_VPROC_PG_H2L_EN_MASK                        0x1
#define PMIC_RG_STRUP_VPROC_PG_H2L_EN_SHIFT                       9
#define PMIC_RG_STRUP_VDRAM_PG_H2L_EN_ADDR                        MT6353_STRUP_CON14
#define PMIC_RG_STRUP_VDRAM_PG_H2L_EN_MASK                        0x1
#define PMIC_RG_STRUP_VDRAM_PG_H2L_EN_SHIFT                       10
#define PMIC_RG_STRUP_VAUD28_PG_H2L_EN_ADDR                       MT6353_STRUP_CON14
#define PMIC_RG_STRUP_VAUD28_PG_H2L_EN_MASK                       0x1
#define PMIC_RG_STRUP_VAUD28_PG_H2L_EN_SHIFT                      11
#define PMIC_RG_STRUP_VEMC_PG_H2L_EN_ADDR                         MT6353_STRUP_CON14
#define PMIC_RG_STRUP_VEMC_PG_H2L_EN_MASK                         0x1
#define PMIC_RG_STRUP_VEMC_PG_H2L_EN_SHIFT                        12
#define PMIC_RG_STRUP_VS1_PG_H2L_EN_ADDR                          MT6353_STRUP_CON14
#define PMIC_RG_STRUP_VS1_PG_H2L_EN_MASK                          0x1
#define PMIC_RG_STRUP_VS1_PG_H2L_EN_SHIFT                         13
#define PMIC_RG_STRUP_VCORE_PG_H2L_EN_ADDR                        MT6353_STRUP_CON14
#define PMIC_RG_STRUP_VCORE_PG_H2L_EN_MASK                        0x1
#define PMIC_RG_STRUP_VCORE_PG_H2L_EN_SHIFT                       14
#define PMIC_RG_STRUP_VAUX18_PG_H2L_EN_ADDR                       MT6353_STRUP_CON14
#define PMIC_RG_STRUP_VAUX18_PG_H2L_EN_MASK                       0x1
#define PMIC_RG_STRUP_VAUX18_PG_H2L_EN_SHIFT                      15
#define PMIC_RG_STRUP_VCORE2_PG_ENB_ADDR                          MT6353_STRUP_CON15
#define PMIC_RG_STRUP_VCORE2_PG_ENB_MASK                          0x1
#define PMIC_RG_STRUP_VCORE2_PG_ENB_SHIFT                         0
#define PMIC_RG_STRUP_VMCH_PG_ENB_ADDR                            MT6353_STRUP_CON15
#define PMIC_RG_STRUP_VMCH_PG_ENB_MASK                            0x1
#define PMIC_RG_STRUP_VMCH_PG_ENB_SHIFT                           1
#define PMIC_RG_STRUP_VMC_PG_ENB_ADDR                             MT6353_STRUP_CON15
#define PMIC_RG_STRUP_VMC_PG_ENB_MASK                             0x1
#define PMIC_RG_STRUP_VMC_PG_ENB_SHIFT                            2
#define PMIC_RG_STRUP_VXO22_PG_ENB_ADDR                           MT6353_STRUP_CON15
#define PMIC_RG_STRUP_VXO22_PG_ENB_MASK                           0x1
#define PMIC_RG_STRUP_VXO22_PG_ENB_SHIFT                          3
#define PMIC_RG_STRUP_VTCXO_PG_ENB_ADDR                           MT6353_STRUP_CON15
#define PMIC_RG_STRUP_VTCXO_PG_ENB_MASK                           0x1
#define PMIC_RG_STRUP_VTCXO_PG_ENB_SHIFT                          4
#define PMIC_RG_STRUP_VUSB33_PG_ENB_ADDR                          MT6353_STRUP_CON15
#define PMIC_RG_STRUP_VUSB33_PG_ENB_MASK                          0x1
#define PMIC_RG_STRUP_VUSB33_PG_ENB_SHIFT                         5
#define PMIC_RG_STRUP_VSRAM_PROC_PG_ENB_ADDR                      MT6353_STRUP_CON15
#define PMIC_RG_STRUP_VSRAM_PROC_PG_ENB_MASK                      0x1
#define PMIC_RG_STRUP_VSRAM_PROC_PG_ENB_SHIFT                     6
#define PMIC_RG_STRUP_VPROC_PG_ENB_ADDR                           MT6353_STRUP_CON15
#define PMIC_RG_STRUP_VPROC_PG_ENB_MASK                           0x1
#define PMIC_RG_STRUP_VPROC_PG_ENB_SHIFT                          7
#define PMIC_RG_STRUP_VDRAM_PG_ENB_ADDR                           MT6353_STRUP_CON15
#define PMIC_RG_STRUP_VDRAM_PG_ENB_MASK                           0x1
#define PMIC_RG_STRUP_VDRAM_PG_ENB_SHIFT                          8
#define PMIC_RG_STRUP_VAUD28_PG_ENB_ADDR                          MT6353_STRUP_CON15
#define PMIC_RG_STRUP_VAUD28_PG_ENB_MASK                          0x1
#define PMIC_RG_STRUP_VAUD28_PG_ENB_SHIFT                         9
#define PMIC_RG_STRUP_VIO28_PG_ENB_ADDR                           MT6353_STRUP_CON15
#define PMIC_RG_STRUP_VIO28_PG_ENB_MASK                           0x1
#define PMIC_RG_STRUP_VIO28_PG_ENB_SHIFT                          10
#define PMIC_RG_STRUP_VEMC_PG_ENB_ADDR                            MT6353_STRUP_CON15
#define PMIC_RG_STRUP_VEMC_PG_ENB_MASK                            0x1
#define PMIC_RG_STRUP_VEMC_PG_ENB_SHIFT                           11
#define PMIC_RG_STRUP_VIO18_PG_ENB_ADDR                           MT6353_STRUP_CON15
#define PMIC_RG_STRUP_VIO18_PG_ENB_MASK                           0x1
#define PMIC_RG_STRUP_VIO18_PG_ENB_SHIFT                          12
#define PMIC_RG_STRUP_VS1_PG_ENB_ADDR                             MT6353_STRUP_CON15
#define PMIC_RG_STRUP_VS1_PG_ENB_MASK                             0x1
#define PMIC_RG_STRUP_VS1_PG_ENB_SHIFT                            13
#define PMIC_RG_STRUP_VCORE_PG_ENB_ADDR                           MT6353_STRUP_CON15
#define PMIC_RG_STRUP_VCORE_PG_ENB_MASK                           0x1
#define PMIC_RG_STRUP_VCORE_PG_ENB_SHIFT                          14
#define PMIC_RG_STRUP_VAUX18_PG_ENB_ADDR                          MT6353_STRUP_CON15
#define PMIC_RG_STRUP_VAUX18_PG_ENB_MASK                          0x1
#define PMIC_RG_STRUP_VAUX18_PG_ENB_SHIFT                         15
#define PMIC_RG_STRUP_VCORE2_OC_ENB_ADDR                          MT6353_STRUP_CON16
#define PMIC_RG_STRUP_VCORE2_OC_ENB_MASK                          0x1
#define PMIC_RG_STRUP_VCORE2_OC_ENB_SHIFT                         12
#define PMIC_RG_STRUP_VPROC_OC_ENB_ADDR                           MT6353_STRUP_CON16
#define PMIC_RG_STRUP_VPROC_OC_ENB_MASK                           0x1
#define PMIC_RG_STRUP_VPROC_OC_ENB_SHIFT                          13
#define PMIC_RG_STRUP_VS1_OC_ENB_ADDR                             MT6353_STRUP_CON16
#define PMIC_RG_STRUP_VS1_OC_ENB_MASK                             0x1
#define PMIC_RG_STRUP_VS1_OC_ENB_SHIFT                            14
#define PMIC_RG_STRUP_VCORE_OC_ENB_ADDR                           MT6353_STRUP_CON16
#define PMIC_RG_STRUP_VCORE_OC_ENB_MASK                           0x1
#define PMIC_RG_STRUP_VCORE_OC_ENB_SHIFT                          15
#define PMIC_RG_STRUP_LONG_PRESS_RESET_EXTEND_ADDR                MT6353_STRUP_CON17
#define PMIC_RG_STRUP_LONG_PRESS_RESET_EXTEND_MASK                0x1
#define PMIC_RG_STRUP_LONG_PRESS_RESET_EXTEND_SHIFT               0
#define PMIC_RG_THRDET_SEL_ADDR                                   MT6353_STRUP_ANA_CON0
#define PMIC_RG_THRDET_SEL_MASK                                   0x1
#define PMIC_RG_THRDET_SEL_SHIFT                                  0
#define PMIC_RG_STRUP_THR_SEL_ADDR                                MT6353_STRUP_ANA_CON0
#define PMIC_RG_STRUP_THR_SEL_MASK                                0x3
#define PMIC_RG_STRUP_THR_SEL_SHIFT                               1
#define PMIC_RG_THR_TMODE_ADDR                                    MT6353_STRUP_ANA_CON0
#define PMIC_RG_THR_TMODE_MASK                                    0x1
#define PMIC_RG_THR_TMODE_SHIFT                                   3
#define PMIC_RG_VREF_BG_ADDR                                      MT6353_STRUP_ANA_CON0
#define PMIC_RG_VREF_BG_MASK                                      0x7
#define PMIC_RG_VREF_BG_SHIFT                                     4
#define PMIC_RG_STRUP_IREF_TRIM_ADDR                              MT6353_STRUP_ANA_CON0
#define PMIC_RG_STRUP_IREF_TRIM_MASK                              0x1F
#define PMIC_RG_STRUP_IREF_TRIM_SHIFT                             7
#define PMIC_RG_RST_DRVSEL_ADDR                                   MT6353_STRUP_ANA_CON0
#define PMIC_RG_RST_DRVSEL_MASK                                   0x1
#define PMIC_RG_RST_DRVSEL_SHIFT                                  12
#define PMIC_RG_EN_DRVSEL_ADDR                                    MT6353_STRUP_ANA_CON0
#define PMIC_RG_EN_DRVSEL_MASK                                    0x1
#define PMIC_RG_EN_DRVSEL_SHIFT                                   13
#define PMIC_RG_FCHR_KEYDET_EN_ADDR                               MT6353_STRUP_ANA_CON0
#define PMIC_RG_FCHR_KEYDET_EN_MASK                               0x1
#define PMIC_RG_FCHR_KEYDET_EN_SHIFT                              14
#define PMIC_RG_FCHR_PU_EN_ADDR                                   MT6353_STRUP_ANA_CON0
#define PMIC_RG_FCHR_PU_EN_MASK                                   0x1
#define PMIC_RG_FCHR_PU_EN_SHIFT                                  15
#define PMIC_RG_PMU_RSV_ADDR                                      MT6353_STRUP_ANA_CON1
#define PMIC_RG_PMU_RSV_MASK                                      0xF
#define PMIC_RG_PMU_RSV_SHIFT                                     0
#define PMIC_TYPE_C_PHY_RG_CC_RP_SEL_ADDR                         MT6353_TYPE_C_PHY_RG_0
#define PMIC_TYPE_C_PHY_RG_CC_RP_SEL_MASK                         0x3
#define PMIC_TYPE_C_PHY_RG_CC_RP_SEL_SHIFT                        0
#define PMIC_REG_TYPE_C_PHY_RG_PD_TX_SLEW_CALEN_ADDR              MT6353_TYPE_C_PHY_RG_0
#define PMIC_REG_TYPE_C_PHY_RG_PD_TX_SLEW_CALEN_MASK              0x1
#define PMIC_REG_TYPE_C_PHY_RG_PD_TX_SLEW_CALEN_SHIFT             3
#define PMIC_REG_TYPE_C_PHY_RG_PD_TXSLEW_I_ADDR                   MT6353_TYPE_C_PHY_RG_0
#define PMIC_REG_TYPE_C_PHY_RG_PD_TXSLEW_I_MASK                   0x7
#define PMIC_REG_TYPE_C_PHY_RG_PD_TXSLEW_I_SHIFT                  4
#define PMIC_REG_TYPE_C_PHY_RG_CC_MPX_SEL_ADDR                    MT6353_TYPE_C_PHY_RG_0
#define PMIC_REG_TYPE_C_PHY_RG_CC_MPX_SEL_MASK                    0xFF
#define PMIC_REG_TYPE_C_PHY_RG_CC_MPX_SEL_SHIFT                   8
#define PMIC_REG_TYPE_C_PHY_RG_CC_RESERVE_ADDR                    MT6353_TYPE_C_PHY_RG_CC_RESERVE_CSR
#define PMIC_REG_TYPE_C_PHY_RG_CC_RESERVE_MASK                    0xFF
#define PMIC_REG_TYPE_C_PHY_RG_CC_RESERVE_SHIFT                   0
#define PMIC_REG_TYPE_C_VCMP_CC2_SW_SEL_ST_CNT_VAL_ADDR           MT6353_TYPE_C_VCMP_CTRL
#define PMIC_REG_TYPE_C_VCMP_CC2_SW_SEL_ST_CNT_VAL_MASK           0x1F
#define PMIC_REG_TYPE_C_VCMP_CC2_SW_SEL_ST_CNT_VAL_SHIFT          0
#define PMIC_REG_TYPE_C_VCMP_BIAS_EN_ST_CNT_VAL_ADDR              MT6353_TYPE_C_VCMP_CTRL
#define PMIC_REG_TYPE_C_VCMP_BIAS_EN_ST_CNT_VAL_MASK              0x3
#define PMIC_REG_TYPE_C_VCMP_BIAS_EN_ST_CNT_VAL_SHIFT             5
#define PMIC_REG_TYPE_C_VCMP_DAC_EN_ST_CNT_VAL_ADDR               MT6353_TYPE_C_VCMP_CTRL
#define PMIC_REG_TYPE_C_VCMP_DAC_EN_ST_CNT_VAL_MASK               0x1
#define PMIC_REG_TYPE_C_VCMP_DAC_EN_ST_CNT_VAL_SHIFT              7
#define PMIC_REG_TYPE_C_PORT_SUPPORT_ROLE_ADDR                    MT6353_TYPE_C_CTRL
#define PMIC_REG_TYPE_C_PORT_SUPPORT_ROLE_MASK                    0x3
#define PMIC_REG_TYPE_C_PORT_SUPPORT_ROLE_SHIFT                   0
#define PMIC_REG_TYPE_C_ADC_EN_ADDR                               MT6353_TYPE_C_CTRL
#define PMIC_REG_TYPE_C_ADC_EN_MASK                               0x1
#define PMIC_REG_TYPE_C_ADC_EN_SHIFT                              2
#define PMIC_REG_TYPE_C_ACC_EN_ADDR                               MT6353_TYPE_C_CTRL
#define PMIC_REG_TYPE_C_ACC_EN_MASK                               0x1
#define PMIC_REG_TYPE_C_ACC_EN_SHIFT                              3
#define PMIC_REG_TYPE_C_AUDIO_ACC_EN_ADDR                         MT6353_TYPE_C_CTRL
#define PMIC_REG_TYPE_C_AUDIO_ACC_EN_MASK                         0x1
#define PMIC_REG_TYPE_C_AUDIO_ACC_EN_SHIFT                        4
#define PMIC_REG_TYPE_C_DEBUG_ACC_EN_ADDR                         MT6353_TYPE_C_CTRL
#define PMIC_REG_TYPE_C_DEBUG_ACC_EN_MASK                         0x1
#define PMIC_REG_TYPE_C_DEBUG_ACC_EN_SHIFT                        5
#define PMIC_REG_TYPE_C_TRY_SRC_ST_EN_ADDR                        MT6353_TYPE_C_CTRL
#define PMIC_REG_TYPE_C_TRY_SRC_ST_EN_MASK                        0x1
#define PMIC_REG_TYPE_C_TRY_SRC_ST_EN_SHIFT                       6
#define PMIC_REG_TYPE_C_ATTACH_SRC_2_TRY_WAIT_SNK_ST_EN_ADDR      MT6353_TYPE_C_CTRL
#define PMIC_REG_TYPE_C_ATTACH_SRC_2_TRY_WAIT_SNK_ST_EN_MASK      0x1
#define PMIC_REG_TYPE_C_ATTACH_SRC_2_TRY_WAIT_SNK_ST_EN_SHIFT     7
#define PMIC_REG_TYPE_C_PD2CC_DET_DISABLE_EN_ADDR                 MT6353_TYPE_C_CTRL
#define PMIC_REG_TYPE_C_PD2CC_DET_DISABLE_EN_MASK                 0x1
#define PMIC_REG_TYPE_C_PD2CC_DET_DISABLE_EN_SHIFT                8
#define PMIC_REG_TYPE_C_ATTACH_SRC_OPEN_PDDEBOUNCE_EN_ADDR        MT6353_TYPE_C_CTRL
#define PMIC_REG_TYPE_C_ATTACH_SRC_OPEN_PDDEBOUNCE_EN_MASK        0x1
#define PMIC_REG_TYPE_C_ATTACH_SRC_OPEN_PDDEBOUNCE_EN_SHIFT       9
#define PMIC_REG_TYPE_C_DISABLE_ST_RD_EN_ADDR                     MT6353_TYPE_C_CTRL
#define PMIC_REG_TYPE_C_DISABLE_ST_RD_EN_MASK                     0x1
#define PMIC_REG_TYPE_C_DISABLE_ST_RD_EN_SHIFT                    10
#define PMIC_W1_TYPE_C_SW_ENT_DISABLE_CMD_ADDR                    MT6353_TYPE_C_CC_SW_CTRL
#define PMIC_W1_TYPE_C_SW_ENT_DISABLE_CMD_MASK                    0x1
#define PMIC_W1_TYPE_C_SW_ENT_DISABLE_CMD_SHIFT                   0
#define PMIC_W1_TYPE_C_SW_ENT_UNATCH_SRC_CMD_ADDR                 MT6353_TYPE_C_CC_SW_CTRL
#define PMIC_W1_TYPE_C_SW_ENT_UNATCH_SRC_CMD_MASK                 0x1
#define PMIC_W1_TYPE_C_SW_ENT_UNATCH_SRC_CMD_SHIFT                1
#define PMIC_W1_TYPE_C_SW_ENT_UNATCH_SNK_CMD_ADDR                 MT6353_TYPE_C_CC_SW_CTRL
#define PMIC_W1_TYPE_C_SW_ENT_UNATCH_SNK_CMD_MASK                 0x1
#define PMIC_W1_TYPE_C_SW_ENT_UNATCH_SNK_CMD_SHIFT                2
#define PMIC_W1_TYPE_C_SW_PR_SWAP_INDICATE_CMD_ADDR               MT6353_TYPE_C_CC_SW_CTRL
#define PMIC_W1_TYPE_C_SW_PR_SWAP_INDICATE_CMD_MASK               0x1
#define PMIC_W1_TYPE_C_SW_PR_SWAP_INDICATE_CMD_SHIFT              3
#define PMIC_W1_TYPE_C_SW_VCONN_SWAP_INDICATE_CMD_ADDR            MT6353_TYPE_C_CC_SW_CTRL
#define PMIC_W1_TYPE_C_SW_VCONN_SWAP_INDICATE_CMD_MASK            0x1
#define PMIC_W1_TYPE_C_SW_VCONN_SWAP_INDICATE_CMD_SHIFT           4
#define PMIC_W1_TYPE_C_SW_ADC_RESULT_MET_VRD_DEFAULT_CMD_ADDR     MT6353_TYPE_C_CC_SW_CTRL
#define PMIC_W1_TYPE_C_SW_ADC_RESULT_MET_VRD_DEFAULT_CMD_MASK     0x1
#define PMIC_W1_TYPE_C_SW_ADC_RESULT_MET_VRD_DEFAULT_CMD_SHIFT    5
#define PMIC_W1_TYPE_C_SW_ADC_RESULT_MET_VRD_15_CMD_ADDR          MT6353_TYPE_C_CC_SW_CTRL
#define PMIC_W1_TYPE_C_SW_ADC_RESULT_MET_VRD_15_CMD_MASK          0x1
#define PMIC_W1_TYPE_C_SW_ADC_RESULT_MET_VRD_15_CMD_SHIFT         6
#define PMIC_W1_TYPE_C_SW_ADC_RESULT_MET_VRD_30_CMD_ADDR          MT6353_TYPE_C_CC_SW_CTRL
#define PMIC_W1_TYPE_C_SW_ADC_RESULT_MET_VRD_30_CMD_MASK          0x1
#define PMIC_W1_TYPE_C_SW_ADC_RESULT_MET_VRD_30_CMD_SHIFT         7
#define PMIC_TYPE_C_SW_VBUS_PRESENT_ADDR                          MT6353_TYPE_C_CC_SW_CTRL
#define PMIC_TYPE_C_SW_VBUS_PRESENT_MASK                          0x1
#define PMIC_TYPE_C_SW_VBUS_PRESENT_SHIFT                         8
#define PMIC_TYPE_C_SW_DA_DRIVE_VCONN_EN_ADDR                     MT6353_TYPE_C_CC_SW_CTRL
#define PMIC_TYPE_C_SW_DA_DRIVE_VCONN_EN_MASK                     0x1
#define PMIC_TYPE_C_SW_DA_DRIVE_VCONN_EN_SHIFT                    9
#define PMIC_TYPE_C_SW_VBUS_DET_DIS_ADDR                          MT6353_TYPE_C_CC_SW_CTRL
#define PMIC_TYPE_C_SW_VBUS_DET_DIS_MASK                          0x1
#define PMIC_TYPE_C_SW_VBUS_DET_DIS_SHIFT                         10
#define PMIC_TYPE_C_SW_CC_DET_DIS_ADDR                            MT6353_TYPE_C_CC_SW_CTRL
#define PMIC_TYPE_C_SW_CC_DET_DIS_MASK                            0x1
#define PMIC_TYPE_C_SW_CC_DET_DIS_SHIFT                           11
#define PMIC_TYPE_C_SW_PD_EN_ADDR                                 MT6353_TYPE_C_CC_SW_CTRL
#define PMIC_TYPE_C_SW_PD_EN_MASK                                 0x1
#define PMIC_TYPE_C_SW_PD_EN_SHIFT                                12
#define PMIC_W1_TYPE_C_SW_ENT_SNK_PWR_REDETECT_CMD_ADDR           MT6353_TYPE_C_CC_SW_CTRL
#define PMIC_W1_TYPE_C_SW_ENT_SNK_PWR_REDETECT_CMD_MASK           0x1
#define PMIC_W1_TYPE_C_SW_ENT_SNK_PWR_REDETECT_CMD_SHIFT          15
#define PMIC_REG_TYPE_C_CC_VOL_PERIODIC_MEAS_VAL_ADDR             MT6353_TYPE_C_CC_VOL_PERIODIC_MEAS_VAL
#define PMIC_REG_TYPE_C_CC_VOL_PERIODIC_MEAS_VAL_MASK             0x1FFF
#define PMIC_REG_TYPE_C_CC_VOL_PERIODIC_MEAS_VAL_SHIFT            0
#define PMIC_REG_TYPE_C_CC_VOL_CC_DEBOUNCE_CNT_VAL_ADDR           MT6353_TYPE_C_CC_VOL_DEBOUCE_CNT_VAL
#define PMIC_REG_TYPE_C_CC_VOL_CC_DEBOUNCE_CNT_VAL_MASK           0xFF
#define PMIC_REG_TYPE_C_CC_VOL_CC_DEBOUNCE_CNT_VAL_SHIFT          0
#define PMIC_REG_TYPE_C_CC_VOL_PD_DEBOUNCE_CNT_VAL_ADDR           MT6353_TYPE_C_CC_VOL_DEBOUCE_CNT_VAL
#define PMIC_REG_TYPE_C_CC_VOL_PD_DEBOUNCE_CNT_VAL_MASK           0x1F
#define PMIC_REG_TYPE_C_CC_VOL_PD_DEBOUNCE_CNT_VAL_SHIFT          8
#define PMIC_REG_TYPE_C_DRP_SRC_CNT_VAL_0_ADDR                    MT6353_TYPE_C_DRP_SRC_CNT_VAL_0
#define PMIC_REG_TYPE_C_DRP_SRC_CNT_VAL_0_MASK                    0xFFFF
#define PMIC_REG_TYPE_C_DRP_SRC_CNT_VAL_0_SHIFT                   0
#define PMIC_REG_TYPE_C_DRP_SNK_CNT_VAL_0_ADDR                    MT6353_TYPE_C_DRP_SNK_CNT_VAL_0
#define PMIC_REG_TYPE_C_DRP_SNK_CNT_VAL_0_MASK                    0xFFFF
#define PMIC_REG_TYPE_C_DRP_SNK_CNT_VAL_0_SHIFT                   0
#define PMIC_REG_TYPE_C_DRP_TRY_CNT_VAL_0_ADDR                    MT6353_TYPE_C_DRP_TRY_CNT_VAL_0
#define PMIC_REG_TYPE_C_DRP_TRY_CNT_VAL_0_MASK                    0xFFFF
#define PMIC_REG_TYPE_C_DRP_TRY_CNT_VAL_0_SHIFT                   0
#define PMIC_REG_TYPE_C_DRP_TRY_WAIT_CNT_VAL_0_ADDR               MT6353_TYPE_C_DRP_TRY_WAIT_CNT_VAL_0
#define PMIC_REG_TYPE_C_DRP_TRY_WAIT_CNT_VAL_0_MASK               0xFFFF
#define PMIC_REG_TYPE_C_DRP_TRY_WAIT_CNT_VAL_0_SHIFT              0
#define PMIC_REG_TYPE_C_DRP_TRY_WAIT_CNT_VAL_1_ADDR               MT6353_TYPE_C_DRP_TRY_WAIT_CNT_VAL_1
#define PMIC_REG_TYPE_C_DRP_TRY_WAIT_CNT_VAL_1_MASK               0xF
#define PMIC_REG_TYPE_C_DRP_TRY_WAIT_CNT_VAL_1_SHIFT              0
#define PMIC_REG_TYPE_C_CC_SRC_VOPEN_DEFAULT_DAC_VAL_ADDR         MT6353_TYPE_C_CC_SRC_DEFAULT_DAC_VAL
#define PMIC_REG_TYPE_C_CC_SRC_VOPEN_DEFAULT_DAC_VAL_MASK         0x3F
#define PMIC_REG_TYPE_C_CC_SRC_VOPEN_DEFAULT_DAC_VAL_SHIFT        0
#define PMIC_REG_TYPE_C_CC_SRC_VRD_DEFAULT_DAC_VAL_ADDR           MT6353_TYPE_C_CC_SRC_DEFAULT_DAC_VAL
#define PMIC_REG_TYPE_C_CC_SRC_VRD_DEFAULT_DAC_VAL_MASK           0x3F
#define PMIC_REG_TYPE_C_CC_SRC_VRD_DEFAULT_DAC_VAL_SHIFT          8
#define PMIC_REG_TYPE_C_CC_SRC_VOPEN_15_DAC_VAL_ADDR              MT6353_TYPE_C_CC_SRC_15_DAC_VAL
#define PMIC_REG_TYPE_C_CC_SRC_VOPEN_15_DAC_VAL_MASK              0x3F
#define PMIC_REG_TYPE_C_CC_SRC_VOPEN_15_DAC_VAL_SHIFT             0
#define PMIC_REG_TYPE_C_CC_SRC_VRD_15_DAC_VAL_ADDR                MT6353_TYPE_C_CC_SRC_15_DAC_VAL
#define PMIC_REG_TYPE_C_CC_SRC_VRD_15_DAC_VAL_MASK                0x3F
#define PMIC_REG_TYPE_C_CC_SRC_VRD_15_DAC_VAL_SHIFT               8
#define PMIC_REG_TYPE_C_CC_SRC_VOPEN_30_DAC_VAL_ADDR              MT6353_TYPE_C_CC_SRC_30_DAC_VAL
#define PMIC_REG_TYPE_C_CC_SRC_VOPEN_30_DAC_VAL_MASK              0x3F
#define PMIC_REG_TYPE_C_CC_SRC_VOPEN_30_DAC_VAL_SHIFT             0
#define PMIC_REG_TYPE_C_CC_SRC_VRD_30_DAC_VAL_ADDR                MT6353_TYPE_C_CC_SRC_30_DAC_VAL
#define PMIC_REG_TYPE_C_CC_SRC_VRD_30_DAC_VAL_MASK                0x3F
#define PMIC_REG_TYPE_C_CC_SRC_VRD_30_DAC_VAL_SHIFT               8
#define PMIC_REG_TYPE_C_CC_SNK_VRP30_DAC_VAL_ADDR                 MT6353_TYPE_C_CC_SNK_DAC_VAL_0
#define PMIC_REG_TYPE_C_CC_SNK_VRP30_DAC_VAL_MASK                 0x3F
#define PMIC_REG_TYPE_C_CC_SNK_VRP30_DAC_VAL_SHIFT                0
#define PMIC_REG_TYPE_C_CC_SNK_VRP15_DAC_VAL_ADDR                 MT6353_TYPE_C_CC_SNK_DAC_VAL_0
#define PMIC_REG_TYPE_C_CC_SNK_VRP15_DAC_VAL_MASK                 0x3F
#define PMIC_REG_TYPE_C_CC_SNK_VRP15_DAC_VAL_SHIFT                8
#define PMIC_REG_TYPE_C_CC_SNK_VRPUSB_DAC_VAL_ADDR                MT6353_TYPE_C_CC_SNK_DAC_VAL_1
#define PMIC_REG_TYPE_C_CC_SNK_VRPUSB_DAC_VAL_MASK                0x3F
#define PMIC_REG_TYPE_C_CC_SNK_VRPUSB_DAC_VAL_SHIFT               0
#define PMIC_REG_TYPE_C_CC_ENT_ATTACH_SRC_INTR_EN_ADDR            MT6353_TYPE_C_INTR_EN_0
#define PMIC_REG_TYPE_C_CC_ENT_ATTACH_SRC_INTR_EN_MASK            0x1
#define PMIC_REG_TYPE_C_CC_ENT_ATTACH_SRC_INTR_EN_SHIFT           0
#define PMIC_REG_TYPE_C_CC_ENT_ATTACH_SNK_INTR_EN_ADDR            MT6353_TYPE_C_INTR_EN_0
#define PMIC_REG_TYPE_C_CC_ENT_ATTACH_SNK_INTR_EN_MASK            0x1
#define PMIC_REG_TYPE_C_CC_ENT_ATTACH_SNK_INTR_EN_SHIFT           1
#define PMIC_REG_TYPE_C_CC_ENT_AUDIO_ACC_INTR_EN_ADDR             MT6353_TYPE_C_INTR_EN_0
#define PMIC_REG_TYPE_C_CC_ENT_AUDIO_ACC_INTR_EN_MASK             0x1
#define PMIC_REG_TYPE_C_CC_ENT_AUDIO_ACC_INTR_EN_SHIFT            2
#define PMIC_REG_TYPE_C_CC_ENT_DBG_ACC_INTR_EN_ADDR               MT6353_TYPE_C_INTR_EN_0
#define PMIC_REG_TYPE_C_CC_ENT_DBG_ACC_INTR_EN_MASK               0x1
#define PMIC_REG_TYPE_C_CC_ENT_DBG_ACC_INTR_EN_SHIFT              3
#define PMIC_REG_TYPE_C_CC_ENT_DISABLE_INTR_EN_ADDR               MT6353_TYPE_C_INTR_EN_0
#define PMIC_REG_TYPE_C_CC_ENT_DISABLE_INTR_EN_MASK               0x1
#define PMIC_REG_TYPE_C_CC_ENT_DISABLE_INTR_EN_SHIFT              4
#define PMIC_REG_TYPE_C_CC_ENT_UNATTACH_SRC_INTR_EN_ADDR          MT6353_TYPE_C_INTR_EN_0
#define PMIC_REG_TYPE_C_CC_ENT_UNATTACH_SRC_INTR_EN_MASK          0x1
#define PMIC_REG_TYPE_C_CC_ENT_UNATTACH_SRC_INTR_EN_SHIFT         5
#define PMIC_REG_TYPE_C_CC_ENT_UNATTACH_SNK_INTR_EN_ADDR          MT6353_TYPE_C_INTR_EN_0
#define PMIC_REG_TYPE_C_CC_ENT_UNATTACH_SNK_INTR_EN_MASK          0x1
#define PMIC_REG_TYPE_C_CC_ENT_UNATTACH_SNK_INTR_EN_SHIFT         6
#define PMIC_REG_TYPE_C_CC_ENT_ATTACH_WAIT_SRC_INTR_EN_ADDR       MT6353_TYPE_C_INTR_EN_0
#define PMIC_REG_TYPE_C_CC_ENT_ATTACH_WAIT_SRC_INTR_EN_MASK       0x1
#define PMIC_REG_TYPE_C_CC_ENT_ATTACH_WAIT_SRC_INTR_EN_SHIFT      7
#define PMIC_REG_TYPE_C_CC_ENT_ATTACH_WAIT_SNK_INTR_EN_ADDR       MT6353_TYPE_C_INTR_EN_0
#define PMIC_REG_TYPE_C_CC_ENT_ATTACH_WAIT_SNK_INTR_EN_MASK       0x1
#define PMIC_REG_TYPE_C_CC_ENT_ATTACH_WAIT_SNK_INTR_EN_SHIFT      8
#define PMIC_REG_TYPE_C_CC_ENT_TRY_SRC_INTR_EN_ADDR               MT6353_TYPE_C_INTR_EN_0
#define PMIC_REG_TYPE_C_CC_ENT_TRY_SRC_INTR_EN_MASK               0x1
#define PMIC_REG_TYPE_C_CC_ENT_TRY_SRC_INTR_EN_SHIFT              9
#define PMIC_REG_TYPE_C_CC_ENT_TRY_WAIT_SNK_INTR_EN_ADDR          MT6353_TYPE_C_INTR_EN_0
#define PMIC_REG_TYPE_C_CC_ENT_TRY_WAIT_SNK_INTR_EN_MASK          0x1
#define PMIC_REG_TYPE_C_CC_ENT_TRY_WAIT_SNK_INTR_EN_SHIFT         10
#define PMIC_REG_TYPE_C_CC_ENT_UNATTACH_ACC_INTR_EN_ADDR          MT6353_TYPE_C_INTR_EN_0
#define PMIC_REG_TYPE_C_CC_ENT_UNATTACH_ACC_INTR_EN_MASK          0x1
#define PMIC_REG_TYPE_C_CC_ENT_UNATTACH_ACC_INTR_EN_SHIFT         11
#define PMIC_REG_TYPE_C_CC_ENT_ATTACH_WAIT_ACC_INTR_EN_ADDR       MT6353_TYPE_C_INTR_EN_0
#define PMIC_REG_TYPE_C_CC_ENT_ATTACH_WAIT_ACC_INTR_EN_MASK       0x1
#define PMIC_REG_TYPE_C_CC_ENT_ATTACH_WAIT_ACC_INTR_EN_SHIFT      12
#define PMIC_REG_TYPE_C_CC_ENT_SNK_PWR_IDLE_INTR_EN_ADDR          MT6353_TYPE_C_INTR_EN_2
#define PMIC_REG_TYPE_C_CC_ENT_SNK_PWR_IDLE_INTR_EN_MASK          0x1
#define PMIC_REG_TYPE_C_CC_ENT_SNK_PWR_IDLE_INTR_EN_SHIFT         0
#define PMIC_REG_TYPE_C_CC_ENT_SNK_PWR_DEFAULT_INTR_EN_ADDR       MT6353_TYPE_C_INTR_EN_2
#define PMIC_REG_TYPE_C_CC_ENT_SNK_PWR_DEFAULT_INTR_EN_MASK       0x1
#define PMIC_REG_TYPE_C_CC_ENT_SNK_PWR_DEFAULT_INTR_EN_SHIFT      1
#define PMIC_REG_TYPE_C_CC_ENT_SNK_PWR_15_INTR_EN_ADDR            MT6353_TYPE_C_INTR_EN_2
#define PMIC_REG_TYPE_C_CC_ENT_SNK_PWR_15_INTR_EN_MASK            0x1
#define PMIC_REG_TYPE_C_CC_ENT_SNK_PWR_15_INTR_EN_SHIFT           2
#define PMIC_REG_TYPE_C_CC_ENT_SNK_PWR_30_INTR_EN_ADDR            MT6353_TYPE_C_INTR_EN_2
#define PMIC_REG_TYPE_C_CC_ENT_SNK_PWR_30_INTR_EN_MASK            0x1
#define PMIC_REG_TYPE_C_CC_ENT_SNK_PWR_30_INTR_EN_SHIFT           3
#define PMIC_REG_TYPE_C_CC_ENT_SNK_PWR_REDETECT_INTR_EN_ADDR      MT6353_TYPE_C_INTR_EN_2
#define PMIC_REG_TYPE_C_CC_ENT_SNK_PWR_REDETECT_INTR_EN_MASK      0x1
#define PMIC_REG_TYPE_C_CC_ENT_SNK_PWR_REDETECT_INTR_EN_SHIFT     4
#define PMIC_TYPE_C_CC_ENT_ATTACH_SRC_INTR_ADDR                   MT6353_TYPE_C_INTR_0
#define PMIC_TYPE_C_CC_ENT_ATTACH_SRC_INTR_MASK                   0x1
#define PMIC_TYPE_C_CC_ENT_ATTACH_SRC_INTR_SHIFT                  0
#define PMIC_TYPE_C_CC_ENT_ATTACH_SNK_INTR_ADDR                   MT6353_TYPE_C_INTR_0
#define PMIC_TYPE_C_CC_ENT_ATTACH_SNK_INTR_MASK                   0x1
#define PMIC_TYPE_C_CC_ENT_ATTACH_SNK_INTR_SHIFT                  1
#define PMIC_TYPE_C_CC_ENT_AUDIO_ACC_INTR_ADDR                    MT6353_TYPE_C_INTR_0
#define PMIC_TYPE_C_CC_ENT_AUDIO_ACC_INTR_MASK                    0x1
#define PMIC_TYPE_C_CC_ENT_AUDIO_ACC_INTR_SHIFT                   2
#define PMIC_TYPE_C_CC_ENT_DBG_ACC_INTR_ADDR                      MT6353_TYPE_C_INTR_0
#define PMIC_TYPE_C_CC_ENT_DBG_ACC_INTR_MASK                      0x1
#define PMIC_TYPE_C_CC_ENT_DBG_ACC_INTR_SHIFT                     3
#define PMIC_TYPE_C_CC_ENT_DISABLE_INTR_ADDR                      MT6353_TYPE_C_INTR_0
#define PMIC_TYPE_C_CC_ENT_DISABLE_INTR_MASK                      0x1
#define PMIC_TYPE_C_CC_ENT_DISABLE_INTR_SHIFT                     4
#define PMIC_TYPE_C_CC_ENT_UNATTACH_SRC_INTR_ADDR                 MT6353_TYPE_C_INTR_0
#define PMIC_TYPE_C_CC_ENT_UNATTACH_SRC_INTR_MASK                 0x1
#define PMIC_TYPE_C_CC_ENT_UNATTACH_SRC_INTR_SHIFT                5
#define PMIC_TYPE_C_CC_ENT_UNATTACH_SNK_INTR_ADDR                 MT6353_TYPE_C_INTR_0
#define PMIC_TYPE_C_CC_ENT_UNATTACH_SNK_INTR_MASK                 0x1
#define PMIC_TYPE_C_CC_ENT_UNATTACH_SNK_INTR_SHIFT                6
#define PMIC_TYPE_C_CC_ENT_ATTACH_WAIT_SRC_INTR_ADDR              MT6353_TYPE_C_INTR_0
#define PMIC_TYPE_C_CC_ENT_ATTACH_WAIT_SRC_INTR_MASK              0x1
#define PMIC_TYPE_C_CC_ENT_ATTACH_WAIT_SRC_INTR_SHIFT             7
#define PMIC_TYPE_C_CC_ENT_ATTACH_WAIT_SNK_INTR_ADDR              MT6353_TYPE_C_INTR_0
#define PMIC_TYPE_C_CC_ENT_ATTACH_WAIT_SNK_INTR_MASK              0x1
#define PMIC_TYPE_C_CC_ENT_ATTACH_WAIT_SNK_INTR_SHIFT             8
#define PMIC_TYPE_C_CC_ENT_TRY_SRC_INTR_ADDR                      MT6353_TYPE_C_INTR_0
#define PMIC_TYPE_C_CC_ENT_TRY_SRC_INTR_MASK                      0x1
#define PMIC_TYPE_C_CC_ENT_TRY_SRC_INTR_SHIFT                     9
#define PMIC_TYPE_C_CC_ENT_TRY_WAIT_SNK_INTR_ADDR                 MT6353_TYPE_C_INTR_0
#define PMIC_TYPE_C_CC_ENT_TRY_WAIT_SNK_INTR_MASK                 0x1
#define PMIC_TYPE_C_CC_ENT_TRY_WAIT_SNK_INTR_SHIFT                10
#define PMIC_TYPE_C_CC_ENT_UNATTACH_ACC_INTR_ADDR                 MT6353_TYPE_C_INTR_0
#define PMIC_TYPE_C_CC_ENT_UNATTACH_ACC_INTR_MASK                 0x1
#define PMIC_TYPE_C_CC_ENT_UNATTACH_ACC_INTR_SHIFT                11
#define PMIC_TYPE_C_CC_ENT_ATTACH_WAIT_ACC_INTR_ADDR              MT6353_TYPE_C_INTR_0
#define PMIC_TYPE_C_CC_ENT_ATTACH_WAIT_ACC_INTR_MASK              0x1
#define PMIC_TYPE_C_CC_ENT_ATTACH_WAIT_ACC_INTR_SHIFT             12
#define PMIC_TYPE_C_CC_ENT_SNK_PWR_IDLE_INTR_ADDR                 MT6353_TYPE_C_INTR_2
#define PMIC_TYPE_C_CC_ENT_SNK_PWR_IDLE_INTR_MASK                 0x1
#define PMIC_TYPE_C_CC_ENT_SNK_PWR_IDLE_INTR_SHIFT                0
#define PMIC_TYPE_C_CC_ENT_SNK_PWR_DEFAULT_INTR_ADDR              MT6353_TYPE_C_INTR_2
#define PMIC_TYPE_C_CC_ENT_SNK_PWR_DEFAULT_INTR_MASK              0x1
#define PMIC_TYPE_C_CC_ENT_SNK_PWR_DEFAULT_INTR_SHIFT             1
#define PMIC_TYPE_C_CC_ENT_SNK_PWR_15_INTR_ADDR                   MT6353_TYPE_C_INTR_2
#define PMIC_TYPE_C_CC_ENT_SNK_PWR_15_INTR_MASK                   0x1
#define PMIC_TYPE_C_CC_ENT_SNK_PWR_15_INTR_SHIFT                  2
#define PMIC_TYPE_C_CC_ENT_SNK_PWR_30_INTR_ADDR                   MT6353_TYPE_C_INTR_2
#define PMIC_TYPE_C_CC_ENT_SNK_PWR_30_INTR_MASK                   0x1
#define PMIC_TYPE_C_CC_ENT_SNK_PWR_30_INTR_SHIFT                  3
#define PMIC_TYPE_C_CC_ENT_SNK_PWR_REDETECT_INTR_ADDR             MT6353_TYPE_C_INTR_2
#define PMIC_TYPE_C_CC_ENT_SNK_PWR_REDETECT_INTR_MASK             0x1
#define PMIC_TYPE_C_CC_ENT_SNK_PWR_REDETECT_INTR_SHIFT            4
#define PMIC_RO_TYPE_C_CC_ST_ADDR                                 MT6353_TYPE_C_CC_STATUS
#define PMIC_RO_TYPE_C_CC_ST_MASK                                 0xF
#define PMIC_RO_TYPE_C_CC_ST_SHIFT                                0
#define PMIC_RO_TYPE_C_ROUTED_CC_ADDR                             MT6353_TYPE_C_CC_STATUS
#define PMIC_RO_TYPE_C_ROUTED_CC_MASK                             0x1
#define PMIC_RO_TYPE_C_ROUTED_CC_SHIFT                            15
#define PMIC_RO_TYPE_C_CC_SNK_PWR_ST_ADDR                         MT6353_TYPE_C_PWR_STATUS
#define PMIC_RO_TYPE_C_CC_SNK_PWR_ST_MASK                         0x7
#define PMIC_RO_TYPE_C_CC_SNK_PWR_ST_SHIFT                        0
#define PMIC_RO_TYPE_C_CC_PWR_ROLE_ADDR                           MT6353_TYPE_C_PWR_STATUS
#define PMIC_RO_TYPE_C_CC_PWR_ROLE_MASK                           0x1
#define PMIC_RO_TYPE_C_CC_PWR_ROLE_SHIFT                          4
#define PMIC_RO_TYPE_C_DRIVE_VCONN_CAPABLE_ADDR                   MT6353_TYPE_C_PWR_STATUS
#define PMIC_RO_TYPE_C_DRIVE_VCONN_CAPABLE_MASK                   0x1
#define PMIC_RO_TYPE_C_DRIVE_VCONN_CAPABLE_SHIFT                  5
#define PMIC_RO_TYPE_C_AD_CC_CMP_OUT_ADDR                         MT6353_TYPE_C_PWR_STATUS
#define PMIC_RO_TYPE_C_AD_CC_CMP_OUT_MASK                         0x1
#define PMIC_RO_TYPE_C_AD_CC_CMP_OUT_SHIFT                        6
#define PMIC_RO_AD_CC_VUSB33_RDY_ADDR                             MT6353_TYPE_C_PWR_STATUS
#define PMIC_RO_AD_CC_VUSB33_RDY_MASK                             0x1
#define PMIC_RO_AD_CC_VUSB33_RDY_SHIFT                            7
#define PMIC_REG_TYPE_C_PHY_RG_CC1_RPDE_ADDR                      MT6353_TYPE_C_PHY_RG_CC1_RESISTENCE_0
#define PMIC_REG_TYPE_C_PHY_RG_CC1_RPDE_MASK                      0x7
#define PMIC_REG_TYPE_C_PHY_RG_CC1_RPDE_SHIFT                     0
#define PMIC_REG_TYPE_C_PHY_RG_CC1_RP15_ADDR                      MT6353_TYPE_C_PHY_RG_CC1_RESISTENCE_0
#define PMIC_REG_TYPE_C_PHY_RG_CC1_RP15_MASK                      0x1F
#define PMIC_REG_TYPE_C_PHY_RG_CC1_RP15_SHIFT                     8
#define PMIC_REG_TYPE_C_PHY_RG_CC1_RP3_ADDR                       MT6353_TYPE_C_PHY_RG_CC1_RESISTENCE_1
#define PMIC_REG_TYPE_C_PHY_RG_CC1_RP3_MASK                       0x1F
#define PMIC_REG_TYPE_C_PHY_RG_CC1_RP3_SHIFT                      0
#define PMIC_REG_TYPE_C_PHY_RG_CC1_RD_ADDR                        MT6353_TYPE_C_PHY_RG_CC1_RESISTENCE_1
#define PMIC_REG_TYPE_C_PHY_RG_CC1_RD_MASK                        0x1F
#define PMIC_REG_TYPE_C_PHY_RG_CC1_RD_SHIFT                       8
#define PMIC_REG_TYPE_C_PHY_RG_CC2_RPDE_ADDR                      MT6353_TYPE_C_PHY_RG_CC2_RESISTENCE_0
#define PMIC_REG_TYPE_C_PHY_RG_CC2_RPDE_MASK                      0x7
#define PMIC_REG_TYPE_C_PHY_RG_CC2_RPDE_SHIFT                     0
#define PMIC_REG_TYPE_C_PHY_RG_CC2_RP15_ADDR                      MT6353_TYPE_C_PHY_RG_CC2_RESISTENCE_0
#define PMIC_REG_TYPE_C_PHY_RG_CC2_RP15_MASK                      0x1F
#define PMIC_REG_TYPE_C_PHY_RG_CC2_RP15_SHIFT                     8
#define PMIC_REG_TYPE_C_PHY_RG_CC2_RP3_ADDR                       MT6353_TYPE_C_PHY_RG_CC2_RESISTENCE_1
#define PMIC_REG_TYPE_C_PHY_RG_CC2_RP3_MASK                       0x1F
#define PMIC_REG_TYPE_C_PHY_RG_CC2_RP3_SHIFT                      0
#define PMIC_REG_TYPE_C_PHY_RG_CC2_RD_ADDR                        MT6353_TYPE_C_PHY_RG_CC2_RESISTENCE_1
#define PMIC_REG_TYPE_C_PHY_RG_CC2_RD_MASK                        0x1F
#define PMIC_REG_TYPE_C_PHY_RG_CC2_RD_SHIFT                       8
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_RCC1_ADDR          MT6353_TYPE_C_CC_SW_FORCE_MODE_ENABLE
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_RCC1_MASK          0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_RCC1_SHIFT         0
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_RCC2_ADDR          MT6353_TYPE_C_CC_SW_FORCE_MODE_ENABLE
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_RCC2_MASK          0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_RCC2_SHIFT         1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_LEV_EN_ADDR        MT6353_TYPE_C_CC_SW_FORCE_MODE_ENABLE
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_LEV_EN_MASK        0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_LEV_EN_SHIFT       2
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_SW_SEL_ADDR        MT6353_TYPE_C_CC_SW_FORCE_MODE_ENABLE
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_SW_SEL_MASK        0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_SW_SEL_SHIFT       3
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_BIAS_EN_ADDR       MT6353_TYPE_C_CC_SW_FORCE_MODE_ENABLE
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_BIAS_EN_MASK       0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_BIAS_EN_SHIFT      4
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_LPF_EN_ADDR        MT6353_TYPE_C_CC_SW_FORCE_MODE_ENABLE
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_LPF_EN_MASK        0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_LPF_EN_SHIFT       5
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_ADCSW_EN_ADDR      MT6353_TYPE_C_CC_SW_FORCE_MODE_ENABLE
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_ADCSW_EN_MASK      0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_ADCSW_EN_SHIFT     6
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_SASW_EN_ADDR       MT6353_TYPE_C_CC_SW_FORCE_MODE_ENABLE
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_SASW_EN_MASK       0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_SASW_EN_SHIFT      7
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_DAC_EN_ADDR        MT6353_TYPE_C_CC_SW_FORCE_MODE_ENABLE
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_DAC_EN_MASK        0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_DAC_EN_SHIFT       8
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_SACLK_ADDR         MT6353_TYPE_C_CC_SW_FORCE_MODE_ENABLE
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_SACLK_MASK         0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_SACLK_SHIFT        9
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_DAC_IN_ADDR        MT6353_TYPE_C_CC_SW_FORCE_MODE_ENABLE
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_DAC_IN_MASK        0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_DAC_IN_SHIFT       10
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_DAC_CAL_ADDR       MT6353_TYPE_C_CC_SW_FORCE_MODE_ENABLE
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_DAC_CAL_MASK       0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_DAC_CAL_SHIFT      11
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_DAC_GAIN_CAL_ADDR  MT6353_TYPE_C_CC_SW_FORCE_MODE_ENABLE
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_DAC_GAIN_CAL_MASK  0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_DAC_GAIN_CAL_SHIFT 12
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RPCC1_EN_ADDR         MT6353_TYPE_C_CC_SW_FORCE_MODE_VAL_0
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RPCC1_EN_MASK         0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RPCC1_EN_SHIFT        0
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RDCC1_EN_ADDR         MT6353_TYPE_C_CC_SW_FORCE_MODE_VAL_0
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RDCC1_EN_MASK         0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RDCC1_EN_SHIFT        1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RACC1_EN_ADDR         MT6353_TYPE_C_CC_SW_FORCE_MODE_VAL_0
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RACC1_EN_MASK         0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RACC1_EN_SHIFT        2
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RPCC2_EN_ADDR         MT6353_TYPE_C_CC_SW_FORCE_MODE_VAL_0
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RPCC2_EN_MASK         0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RPCC2_EN_SHIFT        3
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RDCC2_EN_ADDR         MT6353_TYPE_C_CC_SW_FORCE_MODE_VAL_0
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RDCC2_EN_MASK         0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RDCC2_EN_SHIFT        4
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RACC2_EN_ADDR         MT6353_TYPE_C_CC_SW_FORCE_MODE_VAL_0
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RACC2_EN_MASK         0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RACC2_EN_SHIFT        5
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_LEV_EN_ADDR           MT6353_TYPE_C_CC_SW_FORCE_MODE_VAL_0
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_LEV_EN_MASK           0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_LEV_EN_SHIFT          6
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_SW_SEL_ADDR           MT6353_TYPE_C_CC_SW_FORCE_MODE_VAL_0
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_SW_SEL_MASK           0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_SW_SEL_SHIFT          7
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_BIAS_EN_ADDR          MT6353_TYPE_C_CC_SW_FORCE_MODE_VAL_0
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_BIAS_EN_MASK          0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_BIAS_EN_SHIFT         8
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_LPF_EN_ADDR           MT6353_TYPE_C_CC_SW_FORCE_MODE_VAL_0
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_LPF_EN_MASK           0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_LPF_EN_SHIFT          9
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_ADCSW_EN_ADDR         MT6353_TYPE_C_CC_SW_FORCE_MODE_VAL_0
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_ADCSW_EN_MASK         0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_ADCSW_EN_SHIFT        10
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_SASW_EN_ADDR          MT6353_TYPE_C_CC_SW_FORCE_MODE_VAL_0
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_SASW_EN_MASK          0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_SASW_EN_SHIFT         11
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_DAC_EN_ADDR           MT6353_TYPE_C_CC_SW_FORCE_MODE_VAL_0
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_DAC_EN_MASK           0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_DAC_EN_SHIFT          12
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_SACLK_ADDR            MT6353_TYPE_C_CC_SW_FORCE_MODE_VAL_0
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_SACLK_MASK            0x1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_SACLK_SHIFT           13
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_DAC_IN_ADDR           MT6353_TYPE_C_CC_SW_FORCE_MODE_VAL_1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_DAC_IN_MASK           0x3F
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_DAC_IN_SHIFT          0
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_DAC_CAL_ADDR          MT6353_TYPE_C_CC_SW_FORCE_MODE_VAL_1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_DAC_CAL_MASK          0xF
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_DAC_CAL_SHIFT         8
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_DAC_GAIN_CAL_ADDR     MT6353_TYPE_C_CC_SW_FORCE_MODE_VAL_1
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_DAC_GAIN_CAL_MASK     0xF
#define PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_DAC_GAIN_CAL_SHIFT    12
#define PMIC_TYPE_C_DAC_CAL_START_ADDR                            MT6353_TYPE_C_CC_DAC_CALI_CTRL
#define PMIC_TYPE_C_DAC_CAL_START_MASK                            0x1
#define PMIC_TYPE_C_DAC_CAL_START_SHIFT                           0
#define PMIC_REG_TYPE_C_DAC_CAL_STAGE_ADDR                        MT6353_TYPE_C_CC_DAC_CALI_CTRL
#define PMIC_REG_TYPE_C_DAC_CAL_STAGE_MASK                        0x1
#define PMIC_REG_TYPE_C_DAC_CAL_STAGE_SHIFT                       1
#define PMIC_RO_TYPE_C_DAC_OK_ADDR                                MT6353_TYPE_C_CC_DAC_CALI_CTRL
#define PMIC_RO_TYPE_C_DAC_OK_MASK                                0x1
#define PMIC_RO_TYPE_C_DAC_OK_SHIFT                               4
#define PMIC_RO_TYPE_C_DAC_FAIL_ADDR                              MT6353_TYPE_C_CC_DAC_CALI_CTRL
#define PMIC_RO_TYPE_C_DAC_FAIL_MASK                              0x1
#define PMIC_RO_TYPE_C_DAC_FAIL_SHIFT                             5
#define PMIC_RO_DA_CC_DAC_CAL_ADDR                                MT6353_TYPE_C_CC_DAC_CALI_RESULT
#define PMIC_RO_DA_CC_DAC_CAL_MASK                                0xF
#define PMIC_RO_DA_CC_DAC_CAL_SHIFT                               0
#define PMIC_RO_DA_CC_DAC_GAIN_CAL_ADDR                           MT6353_TYPE_C_CC_DAC_CALI_RESULT
#define PMIC_RO_DA_CC_DAC_GAIN_CAL_MASK                           0xF
#define PMIC_RO_DA_CC_DAC_GAIN_CAL_SHIFT                          4
#define PMIC_REG_TYPE_C_DBG_PORT_SEL_0_ADDR                       MT6353_TYPE_C_DEBUG_PORT_SELECT_0
#define PMIC_REG_TYPE_C_DBG_PORT_SEL_0_MASK                       0xFFFF
#define PMIC_REG_TYPE_C_DBG_PORT_SEL_0_SHIFT                      0
#define PMIC_REG_TYPE_C_DBG_PORT_SEL_1_ADDR                       MT6353_TYPE_C_DEBUG_PORT_SELECT_1
#define PMIC_REG_TYPE_C_DBG_PORT_SEL_1_MASK                       0xFFFF
#define PMIC_REG_TYPE_C_DBG_PORT_SEL_1_SHIFT                      0
#define PMIC_REG_TYPE_C_DBG_MOD_SEL_ADDR                          MT6353_TYPE_C_DEBUG_MODE_SELECT
#define PMIC_REG_TYPE_C_DBG_MOD_SEL_MASK                          0xFFFF
#define PMIC_REG_TYPE_C_DBG_MOD_SEL_SHIFT                         0
#define PMIC_RO_TYPE_C_DBG_OUT_READ_0_ADDR                        MT6353_TYPE_C_DEBUG_OUT_READ_0
#define PMIC_RO_TYPE_C_DBG_OUT_READ_0_MASK                        0xFFFF
#define PMIC_RO_TYPE_C_DBG_OUT_READ_0_SHIFT                       0
#define PMIC_RO_TYPE_C_DBG_OUT_READ_1_ADDR                        MT6353_TYPE_C_DEBUG_OUT_READ_1
#define PMIC_RO_TYPE_C_DBG_OUT_READ_1_MASK                        0xFFFF
#define PMIC_RO_TYPE_C_DBG_OUT_READ_1_SHIFT                       0
#define PMIC_HWCID_ADDR                                           MT6353_HWCID
#define PMIC_HWCID_MASK                                           0xFFFF
#define PMIC_HWCID_SHIFT                                          0
#define PMIC_SWCID_ADDR                                           MT6353_SWCID
#define PMIC_SWCID_MASK                                           0xFFFF
#define PMIC_SWCID_SHIFT                                          0
#define PMIC_RG_SRCLKEN_IN0_EN_ADDR                               MT6353_TOP_CON
#define PMIC_RG_SRCLKEN_IN0_EN_MASK                               0x1
#define PMIC_RG_SRCLKEN_IN0_EN_SHIFT                              0
#define PMIC_RG_SRCLKEN_IN1_EN_ADDR                               MT6353_TOP_CON
#define PMIC_RG_SRCLKEN_IN1_EN_MASK                               0x1
#define PMIC_RG_SRCLKEN_IN1_EN_SHIFT                              1
#define PMIC_RG_OSC_SEL_ADDR                                      MT6353_TOP_CON
#define PMIC_RG_OSC_SEL_MASK                                      0x1
#define PMIC_RG_OSC_SEL_SHIFT                                     2
#define PMIC_RG_SRCLKEN_IN2_EN_ADDR                               MT6353_TOP_CON
#define PMIC_RG_SRCLKEN_IN2_EN_MASK                               0x1
#define PMIC_RG_SRCLKEN_IN2_EN_SHIFT                              3
#define PMIC_RG_SRCLKEN_IN0_HW_MODE_ADDR                          MT6353_TOP_CON
#define PMIC_RG_SRCLKEN_IN0_HW_MODE_MASK                          0x1
#define PMIC_RG_SRCLKEN_IN0_HW_MODE_SHIFT                         4
#define PMIC_RG_SRCLKEN_IN1_HW_MODE_ADDR                          MT6353_TOP_CON
#define PMIC_RG_SRCLKEN_IN1_HW_MODE_MASK                          0x1
#define PMIC_RG_SRCLKEN_IN1_HW_MODE_SHIFT                         5
#define PMIC_RG_OSC_SEL_HW_MODE_ADDR                              MT6353_TOP_CON
#define PMIC_RG_OSC_SEL_HW_MODE_MASK                              0x1
#define PMIC_RG_OSC_SEL_HW_MODE_SHIFT                             6
#define PMIC_RG_SRCLKEN_IN_SYNC_EN_ADDR                           MT6353_TOP_CON
#define PMIC_RG_SRCLKEN_IN_SYNC_EN_MASK                           0x1
#define PMIC_RG_SRCLKEN_IN_SYNC_EN_SHIFT                          8
#define PMIC_RG_OSC_EN_AUTO_OFF_ADDR                              MT6353_TOP_CON
#define PMIC_RG_OSC_EN_AUTO_OFF_MASK                              0x1
#define PMIC_RG_OSC_EN_AUTO_OFF_SHIFT                             9
#define PMIC_TEST_OUT_ADDR                                        MT6353_TEST_OUT
#define PMIC_TEST_OUT_MASK                                        0xFF
#define PMIC_TEST_OUT_SHIFT                                       0
#define PMIC_RG_MON_FLAG_SEL_ADDR                                 MT6353_TEST_CON0
#define PMIC_RG_MON_FLAG_SEL_MASK                                 0xFF
#define PMIC_RG_MON_FLAG_SEL_SHIFT                                0
#define PMIC_RG_MON_GRP_SEL_ADDR                                  MT6353_TEST_CON0
#define PMIC_RG_MON_GRP_SEL_MASK                                  0x1F
#define PMIC_RG_MON_GRP_SEL_SHIFT                                 8
#define PMIC_RG_NANDTREE_MODE_ADDR                                MT6353_TEST_CON1
#define PMIC_RG_NANDTREE_MODE_MASK                                0x1
#define PMIC_RG_NANDTREE_MODE_SHIFT                               0
#define PMIC_RG_TEST_AUXADC_ADDR                                  MT6353_TEST_CON1
#define PMIC_RG_TEST_AUXADC_MASK                                  0x1
#define PMIC_RG_TEST_AUXADC_SHIFT                                 1
#define PMIC_RG_EFUSE_MODE_ADDR                                   MT6353_TEST_CON1
#define PMIC_RG_EFUSE_MODE_MASK                                   0x1
#define PMIC_RG_EFUSE_MODE_SHIFT                                  2
#define PMIC_RG_TEST_STRUP_ADDR                                   MT6353_TEST_CON1
#define PMIC_RG_TEST_STRUP_MASK                                   0x1
#define PMIC_RG_TEST_STRUP_SHIFT                                  3
#define PMIC_TESTMODE_SW_ADDR                                     MT6353_TESTMODE_SW
#define PMIC_TESTMODE_SW_MASK                                     0x1
#define PMIC_TESTMODE_SW_SHIFT                                    0
#define PMIC_EN_STATUS_VDRAM_ADDR                                 MT6353_EN_STATUS1
#define PMIC_EN_STATUS_VDRAM_MASK                                 0x1
#define PMIC_EN_STATUS_VDRAM_SHIFT                                0
#define PMIC_EN_STATUS_VSRAM_PROC_ADDR                            MT6353_EN_STATUS1
#define PMIC_EN_STATUS_VSRAM_PROC_MASK                            0x1
#define PMIC_EN_STATUS_VSRAM_PROC_SHIFT                           1
#define PMIC_EN_STATUS_VAUD28_ADDR                                MT6353_EN_STATUS1
#define PMIC_EN_STATUS_VAUD28_MASK                                0x1
#define PMIC_EN_STATUS_VAUD28_SHIFT                               2
#define PMIC_EN_STATUS_VAUX18_ADDR                                MT6353_EN_STATUS1
#define PMIC_EN_STATUS_VAUX18_MASK                                0x1
#define PMIC_EN_STATUS_VAUX18_SHIFT                               3
#define PMIC_EN_STATUS_VCAMD_ADDR                                 MT6353_EN_STATUS1
#define PMIC_EN_STATUS_VCAMD_MASK                                 0x1
#define PMIC_EN_STATUS_VCAMD_SHIFT                                4
#define PMIC_EN_STATUS_VLDO28_ADDR                                MT6353_EN_STATUS1
#define PMIC_EN_STATUS_VLDO28_MASK                                0x1
#define PMIC_EN_STATUS_VLDO28_SHIFT                               5
#define PMIC_EN_STATUS_VCAMIO_ADDR                                MT6353_EN_STATUS1
#define PMIC_EN_STATUS_VCAMIO_MASK                                0x1
#define PMIC_EN_STATUS_VCAMIO_SHIFT                               6
#define PMIC_EN_STATUS_VCAMA_ADDR                                 MT6353_EN_STATUS1
#define PMIC_EN_STATUS_VCAMA_MASK                                 0x1
#define PMIC_EN_STATUS_VCAMA_SHIFT                                7
#define PMIC_EN_STATUS_VCN18_ADDR                                 MT6353_EN_STATUS1
#define PMIC_EN_STATUS_VCN18_MASK                                 0x1
#define PMIC_EN_STATUS_VCN18_SHIFT                                8
#define PMIC_EN_STATUS_VCN28_ADDR                                 MT6353_EN_STATUS1
#define PMIC_EN_STATUS_VCN28_MASK                                 0x1
#define PMIC_EN_STATUS_VCN28_SHIFT                                9
#define PMIC_EN_STATUS_VCN33_ADDR                                 MT6353_EN_STATUS1
#define PMIC_EN_STATUS_VCN33_MASK                                 0x1
#define PMIC_EN_STATUS_VCN33_SHIFT                                10
#define PMIC_EN_STATUS_VRF12_ADDR                                 MT6353_EN_STATUS1
#define PMIC_EN_STATUS_VRF12_MASK                                 0x1
#define PMIC_EN_STATUS_VRF12_SHIFT                                11
#define PMIC_EN_STATUS_VRF18_ADDR                                 MT6353_EN_STATUS1
#define PMIC_EN_STATUS_VRF18_MASK                                 0x1
#define PMIC_EN_STATUS_VRF18_SHIFT                                12
#define PMIC_EN_STATUS_VXO22_ADDR                                 MT6353_EN_STATUS1
#define PMIC_EN_STATUS_VXO22_MASK                                 0x1
#define PMIC_EN_STATUS_VXO22_SHIFT                                13
#define PMIC_EN_STATUS_VTCXO24_ADDR                               MT6353_EN_STATUS1
#define PMIC_EN_STATUS_VTCXO24_MASK                               0x1
#define PMIC_EN_STATUS_VTCXO24_SHIFT                              14
#define PMIC_EN_STATUS_VTCXO28_ADDR                               MT6353_EN_STATUS1
#define PMIC_EN_STATUS_VTCXO28_MASK                               0x1
#define PMIC_EN_STATUS_VTCXO28_SHIFT                              15
#define PMIC_EN_STATUS_VS1_ADDR                                   MT6353_EN_STATUS2
#define PMIC_EN_STATUS_VS1_MASK                                   0x1
#define PMIC_EN_STATUS_VS1_SHIFT                                  1
#define PMIC_EN_STATUS_VCORE_ADDR                                 MT6353_EN_STATUS2
#define PMIC_EN_STATUS_VCORE_MASK                                 0x1
#define PMIC_EN_STATUS_VCORE_SHIFT                                2
#define PMIC_EN_STATUS_VPROC_ADDR                                 MT6353_EN_STATUS2
#define PMIC_EN_STATUS_VPROC_MASK                                 0x1
#define PMIC_EN_STATUS_VPROC_SHIFT                                3
#define PMIC_EN_STATUS_VPA_ADDR                                   MT6353_EN_STATUS2
#define PMIC_EN_STATUS_VPA_MASK                                   0x1
#define PMIC_EN_STATUS_VPA_SHIFT                                  4
#define PMIC_EN_STATUS_VRTC_ADDR                                  MT6353_EN_STATUS2
#define PMIC_EN_STATUS_VRTC_MASK                                  0x1
#define PMIC_EN_STATUS_VRTC_SHIFT                                 5
#define PMIC_EN_STATUS_TREF_ADDR                                  MT6353_EN_STATUS2
#define PMIC_EN_STATUS_TREF_MASK                                  0x1
#define PMIC_EN_STATUS_TREF_SHIFT                                 6
#define PMIC_EN_STATUS_VIBR_ADDR                                  MT6353_EN_STATUS2
#define PMIC_EN_STATUS_VIBR_MASK                                  0x1
#define PMIC_EN_STATUS_VIBR_SHIFT                                 7
#define PMIC_EN_STATUS_VIO18_ADDR                                 MT6353_EN_STATUS2
#define PMIC_EN_STATUS_VIO18_MASK                                 0x1
#define PMIC_EN_STATUS_VIO18_SHIFT                                8
#define PMIC_EN_STATUS_VEMC33_ADDR                                MT6353_EN_STATUS2
#define PMIC_EN_STATUS_VEMC33_MASK                                0x1
#define PMIC_EN_STATUS_VEMC33_SHIFT                               9
#define PMIC_EN_STATUS_VUSB33_ADDR                                MT6353_EN_STATUS2
#define PMIC_EN_STATUS_VUSB33_MASK                                0x1
#define PMIC_EN_STATUS_VUSB33_SHIFT                               10
#define PMIC_EN_STATUS_VMCH_ADDR                                  MT6353_EN_STATUS2
#define PMIC_EN_STATUS_VMCH_MASK                                  0x1
#define PMIC_EN_STATUS_VMCH_SHIFT                                 11
#define PMIC_EN_STATUS_VMC_ADDR                                   MT6353_EN_STATUS2
#define PMIC_EN_STATUS_VMC_MASK                                   0x1
#define PMIC_EN_STATUS_VMC_SHIFT                                  12
#define PMIC_EN_STATUS_VIO28_ADDR                                 MT6353_EN_STATUS2
#define PMIC_EN_STATUS_VIO28_MASK                                 0x1
#define PMIC_EN_STATUS_VIO28_SHIFT                                13
#define PMIC_EN_STATUS_VSIM2_ADDR                                 MT6353_EN_STATUS2
#define PMIC_EN_STATUS_VSIM2_MASK                                 0x1
#define PMIC_EN_STATUS_VSIM2_SHIFT                                14
#define PMIC_EN_STATUS_VSIM1_ADDR                                 MT6353_EN_STATUS2
#define PMIC_EN_STATUS_VSIM1_MASK                                 0x1
#define PMIC_EN_STATUS_VSIM1_SHIFT                                15
#define PMIC_OC_STATUS_VDRAM_ADDR                                 MT6353_OCSTATUS1
#define PMIC_OC_STATUS_VDRAM_MASK                                 0x1
#define PMIC_OC_STATUS_VDRAM_SHIFT                                0
#define PMIC_OC_STATUS_VSRAM_PROC_ADDR                            MT6353_OCSTATUS1
#define PMIC_OC_STATUS_VSRAM_PROC_MASK                            0x1
#define PMIC_OC_STATUS_VSRAM_PROC_SHIFT                           1
#define PMIC_OC_STATUS_VAUD28_ADDR                                MT6353_OCSTATUS1
#define PMIC_OC_STATUS_VAUD28_MASK                                0x1
#define PMIC_OC_STATUS_VAUD28_SHIFT                               2
#define PMIC_OC_STATUS_VAUX18_ADDR                                MT6353_OCSTATUS1
#define PMIC_OC_STATUS_VAUX18_MASK                                0x1
#define PMIC_OC_STATUS_VAUX18_SHIFT                               3
#define PMIC_OC_STATUS_VCAMD_ADDR                                 MT6353_OCSTATUS1
#define PMIC_OC_STATUS_VCAMD_MASK                                 0x1
#define PMIC_OC_STATUS_VCAMD_SHIFT                                4
#define PMIC_OC_STATUS_VLDO28_ADDR                                MT6353_OCSTATUS1
#define PMIC_OC_STATUS_VLDO28_MASK                                0x1
#define PMIC_OC_STATUS_VLDO28_SHIFT                               5
#define PMIC_OC_STATUS_VCAMIO_ADDR                                MT6353_OCSTATUS1
#define PMIC_OC_STATUS_VCAMIO_MASK                                0x1
#define PMIC_OC_STATUS_VCAMIO_SHIFT                               6
#define PMIC_OC_STATUS_VCAMA_ADDR                                 MT6353_OCSTATUS1
#define PMIC_OC_STATUS_VCAMA_MASK                                 0x1
#define PMIC_OC_STATUS_VCAMA_SHIFT                                7
#define PMIC_OC_STATUS_VCN18_ADDR                                 MT6353_OCSTATUS1
#define PMIC_OC_STATUS_VCN18_MASK                                 0x1
#define PMIC_OC_STATUS_VCN18_SHIFT                                8
#define PMIC_OC_STATUS_VCN28_ADDR                                 MT6353_OCSTATUS1
#define PMIC_OC_STATUS_VCN28_MASK                                 0x1
#define PMIC_OC_STATUS_VCN28_SHIFT                                9
#define PMIC_OC_STATUS_VCN33_ADDR                                 MT6353_OCSTATUS1
#define PMIC_OC_STATUS_VCN33_MASK                                 0x1
#define PMIC_OC_STATUS_VCN33_SHIFT                                10
#define PMIC_OC_STATUS_VRF12_ADDR                                 MT6353_OCSTATUS1
#define PMIC_OC_STATUS_VRF12_MASK                                 0x1
#define PMIC_OC_STATUS_VRF12_SHIFT                                11
#define PMIC_OC_STATUS_VRF18_ADDR                                 MT6353_OCSTATUS1
#define PMIC_OC_STATUS_VRF18_MASK                                 0x1
#define PMIC_OC_STATUS_VRF18_SHIFT                                12
#define PMIC_OC_STATUS_VXO22_ADDR                                 MT6353_OCSTATUS1
#define PMIC_OC_STATUS_VXO22_MASK                                 0x1
#define PMIC_OC_STATUS_VXO22_SHIFT                                13
#define PMIC_OC_STATUS_VTCXO24_ADDR                               MT6353_OCSTATUS1
#define PMIC_OC_STATUS_VTCXO24_MASK                               0x1
#define PMIC_OC_STATUS_VTCXO24_SHIFT                              14
#define PMIC_OC_STATUS_VTCXO28_ADDR                               MT6353_OCSTATUS1
#define PMIC_OC_STATUS_VTCXO28_MASK                               0x1
#define PMIC_OC_STATUS_VTCXO28_SHIFT                              15
#define PMIC_OC_STATUS_VS1_ADDR                                   MT6353_OCSTATUS2
#define PMIC_OC_STATUS_VS1_MASK                                   0x1
#define PMIC_OC_STATUS_VS1_SHIFT                                  2
#define PMIC_OC_STATUS_VCORE_ADDR                                 MT6353_OCSTATUS2
#define PMIC_OC_STATUS_VCORE_MASK                                 0x1
#define PMIC_OC_STATUS_VCORE_SHIFT                                3
#define PMIC_OC_STATUS_VPROC_ADDR                                 MT6353_OCSTATUS2
#define PMIC_OC_STATUS_VPROC_MASK                                 0x1
#define PMIC_OC_STATUS_VPROC_SHIFT                                4
#define PMIC_OC_STATUS_VPA_ADDR                                   MT6353_OCSTATUS2
#define PMIC_OC_STATUS_VPA_MASK                                   0x1
#define PMIC_OC_STATUS_VPA_SHIFT                                  5
#define PMIC_OC_STATUS_TREF_ADDR                                  MT6353_OCSTATUS2
#define PMIC_OC_STATUS_TREF_MASK                                  0x1
#define PMIC_OC_STATUS_TREF_SHIFT                                 6
#define PMIC_OC_STATUS_VIBR_ADDR                                  MT6353_OCSTATUS2
#define PMIC_OC_STATUS_VIBR_MASK                                  0x1
#define PMIC_OC_STATUS_VIBR_SHIFT                                 7
#define PMIC_OC_STATUS_VIO18_ADDR                                 MT6353_OCSTATUS2
#define PMIC_OC_STATUS_VIO18_MASK                                 0x1
#define PMIC_OC_STATUS_VIO18_SHIFT                                8
#define PMIC_OC_STATUS_VEMC33_ADDR                                MT6353_OCSTATUS2
#define PMIC_OC_STATUS_VEMC33_MASK                                0x1
#define PMIC_OC_STATUS_VEMC33_SHIFT                               9
#define PMIC_OC_STATUS_VUSB33_ADDR                                MT6353_OCSTATUS2
#define PMIC_OC_STATUS_VUSB33_MASK                                0x1
#define PMIC_OC_STATUS_VUSB33_SHIFT                               10
#define PMIC_OC_STATUS_VMCH_ADDR                                  MT6353_OCSTATUS2
#define PMIC_OC_STATUS_VMCH_MASK                                  0x1
#define PMIC_OC_STATUS_VMCH_SHIFT                                 11
#define PMIC_OC_STATUS_VMC_ADDR                                   MT6353_OCSTATUS2
#define PMIC_OC_STATUS_VMC_MASK                                   0x1
#define PMIC_OC_STATUS_VMC_SHIFT                                  12
#define PMIC_OC_STATUS_VIO28_ADDR                                 MT6353_OCSTATUS2
#define PMIC_OC_STATUS_VIO28_MASK                                 0x1
#define PMIC_OC_STATUS_VIO28_SHIFT                                13
#define PMIC_OC_STATUS_VSIM2_ADDR                                 MT6353_OCSTATUS2
#define PMIC_OC_STATUS_VSIM2_MASK                                 0x1
#define PMIC_OC_STATUS_VSIM2_SHIFT                                14
#define PMIC_OC_STATUS_VSIM1_ADDR                                 MT6353_OCSTATUS2
#define PMIC_OC_STATUS_VSIM1_MASK                                 0x1
#define PMIC_OC_STATUS_VSIM1_SHIFT                                15
#define PMIC_VCORE2_PG_DEB_ADDR                                   MT6353_PGDEBSTATUS0
#define PMIC_VCORE2_PG_DEB_MASK                                   0x1
#define PMIC_VCORE2_PG_DEB_SHIFT                                  0
#define PMIC_VS1_PG_DEB_ADDR                                      MT6353_PGDEBSTATUS0
#define PMIC_VS1_PG_DEB_MASK                                      0x1
#define PMIC_VS1_PG_DEB_SHIFT                                     1
#define PMIC_VCORE_PG_DEB_ADDR                                    MT6353_PGDEBSTATUS0
#define PMIC_VCORE_PG_DEB_MASK                                    0x1
#define PMIC_VCORE_PG_DEB_SHIFT                                   2
#define PMIC_VPROC_PG_DEB_ADDR                                    MT6353_PGDEBSTATUS0
#define PMIC_VPROC_PG_DEB_MASK                                    0x1
#define PMIC_VPROC_PG_DEB_SHIFT                                   3
#define PMIC_VIO18_PG_DEB_ADDR                                    MT6353_PGDEBSTATUS0
#define PMIC_VIO18_PG_DEB_MASK                                    0x1
#define PMIC_VIO18_PG_DEB_SHIFT                                   4
#define PMIC_VEMC33_PG_DEB_ADDR                                   MT6353_PGDEBSTATUS0
#define PMIC_VEMC33_PG_DEB_MASK                                   0x1
#define PMIC_VEMC33_PG_DEB_SHIFT                                  5
#define PMIC_VUSB33_PG_DEB_ADDR                                   MT6353_PGDEBSTATUS0
#define PMIC_VUSB33_PG_DEB_MASK                                   0x1
#define PMIC_VUSB33_PG_DEB_SHIFT                                  6
#define PMIC_VMCH_PG_DEB_ADDR                                     MT6353_PGDEBSTATUS0
#define PMIC_VMCH_PG_DEB_MASK                                     0x1
#define PMIC_VMCH_PG_DEB_SHIFT                                    7
#define PMIC_VMC_PG_DEB_ADDR                                      MT6353_PGDEBSTATUS0
#define PMIC_VMC_PG_DEB_MASK                                      0x1
#define PMIC_VMC_PG_DEB_SHIFT                                     8
#define PMIC_VIO28_PG_DEB_ADDR                                    MT6353_PGDEBSTATUS0
#define PMIC_VIO28_PG_DEB_MASK                                    0x1
#define PMIC_VIO28_PG_DEB_SHIFT                                   9
#define PMIC_VDRAM_PG_DEB_ADDR                                    MT6353_PGDEBSTATUS0
#define PMIC_VDRAM_PG_DEB_MASK                                    0x1
#define PMIC_VDRAM_PG_DEB_SHIFT                                   10
#define PMIC_VSRAM_PROC_PG_DEB_ADDR                               MT6353_PGDEBSTATUS0
#define PMIC_VSRAM_PROC_PG_DEB_MASK                               0x1
#define PMIC_VSRAM_PROC_PG_DEB_SHIFT                              11
#define PMIC_VAUD28_PG_DEB_ADDR                                   MT6353_PGDEBSTATUS0
#define PMIC_VAUD28_PG_DEB_MASK                                   0x1
#define PMIC_VAUD28_PG_DEB_SHIFT                                  12
#define PMIC_VAUX18_PG_DEB_ADDR                                   MT6353_PGDEBSTATUS0
#define PMIC_VAUX18_PG_DEB_MASK                                   0x1
#define PMIC_VAUX18_PG_DEB_SHIFT                                  13
#define PMIC_VXO22_PG_DEB_ADDR                                    MT6353_PGDEBSTATUS0
#define PMIC_VXO22_PG_DEB_MASK                                    0x1
#define PMIC_VXO22_PG_DEB_SHIFT                                   14
#define PMIC_VTCXO24_PG_DEB_ADDR                                  MT6353_PGDEBSTATUS0
#define PMIC_VTCXO24_PG_DEB_MASK                                  0x1
#define PMIC_VTCXO24_PG_DEB_SHIFT                                 15
#define PMIC_STRUP_VMCH_PG_STATUS_GATED_ADDR                      MT6353_PGSTATUS0
#define PMIC_STRUP_VMCH_PG_STATUS_GATED_MASK                      0x1
#define PMIC_STRUP_VMCH_PG_STATUS_GATED_SHIFT                     0
#define PMIC_STRUP_VMC_PG_STATUS_GATED_ADDR                       MT6353_PGSTATUS0
#define PMIC_STRUP_VMC_PG_STATUS_GATED_MASK                       0x1
#define PMIC_STRUP_VMC_PG_STATUS_GATED_SHIFT                      1
#define PMIC_STRUP_VXO22_PG_STATUS_GATED_ADDR                     MT6353_PGSTATUS0
#define PMIC_STRUP_VXO22_PG_STATUS_GATED_MASK                     0x1
#define PMIC_STRUP_VXO22_PG_STATUS_GATED_SHIFT                    2
#define PMIC_STRUP_VTCXO24_PG_STATUS_GATED_ADDR                   MT6353_PGSTATUS0
#define PMIC_STRUP_VTCXO24_PG_STATUS_GATED_MASK                   0x1
#define PMIC_STRUP_VTCXO24_PG_STATUS_GATED_SHIFT                  3
#define PMIC_STRUP_VUSB33_PG_STATUS_GATED_ADDR                    MT6353_PGSTATUS0
#define PMIC_STRUP_VUSB33_PG_STATUS_GATED_MASK                    0x1
#define PMIC_STRUP_VUSB33_PG_STATUS_GATED_SHIFT                   4
#define PMIC_STRUP_VSRAM_PROC_PG_STATUS_GATED_ADDR                MT6353_PGSTATUS0
#define PMIC_STRUP_VSRAM_PROC_PG_STATUS_GATED_MASK                0x1
#define PMIC_STRUP_VSRAM_PROC_PG_STATUS_GATED_SHIFT               5
#define PMIC_STRUP_VPROC_PG_STATUS_GATED_ADDR                     MT6353_PGSTATUS0
#define PMIC_STRUP_VPROC_PG_STATUS_GATED_MASK                     0x1
#define PMIC_STRUP_VPROC_PG_STATUS_GATED_SHIFT                    6
#define PMIC_STRUP_VDRAM_PG_STATUS_GATED_ADDR                     MT6353_PGSTATUS0
#define PMIC_STRUP_VDRAM_PG_STATUS_GATED_MASK                     0x1
#define PMIC_STRUP_VDRAM_PG_STATUS_GATED_SHIFT                    7
#define PMIC_STRUP_VAUD28_PG_STATUS_GATED_ADDR                    MT6353_PGSTATUS0
#define PMIC_STRUP_VAUD28_PG_STATUS_GATED_MASK                    0x1
#define PMIC_STRUP_VAUD28_PG_STATUS_GATED_SHIFT                   8
#define PMIC_STRUP_VIO28_PG_STATUS_GATED_ADDR                     MT6353_PGSTATUS0
#define PMIC_STRUP_VIO28_PG_STATUS_GATED_MASK                     0x1
#define PMIC_STRUP_VIO28_PG_STATUS_GATED_SHIFT                    9
#define PMIC_STRUP_VEMC33_PG_STATUS_GATED_ADDR                    MT6353_PGSTATUS0
#define PMIC_STRUP_VEMC33_PG_STATUS_GATED_MASK                    0x1
#define PMIC_STRUP_VEMC33_PG_STATUS_GATED_SHIFT                   10
#define PMIC_STRUP_VIO18_PG_STATUS_GATED_ADDR                     MT6353_PGSTATUS0
#define PMIC_STRUP_VIO18_PG_STATUS_GATED_MASK                     0x1
#define PMIC_STRUP_VIO18_PG_STATUS_GATED_SHIFT                    11
#define PMIC_STRUP_VS1_PG_STATUS_GATED_ADDR                       MT6353_PGSTATUS0
#define PMIC_STRUP_VS1_PG_STATUS_GATED_MASK                       0x1
#define PMIC_STRUP_VS1_PG_STATUS_GATED_SHIFT                      12
#define PMIC_STRUP_VCORE_PG_STATUS_GATED_ADDR                     MT6353_PGSTATUS0
#define PMIC_STRUP_VCORE_PG_STATUS_GATED_MASK                     0x1
#define PMIC_STRUP_VCORE_PG_STATUS_GATED_SHIFT                    13
#define PMIC_STRUP_VAUX18_PG_STATUS_GATED_ADDR                    MT6353_PGSTATUS0
#define PMIC_STRUP_VAUX18_PG_STATUS_GATED_MASK                    0x1
#define PMIC_STRUP_VAUX18_PG_STATUS_GATED_SHIFT                   14
#define PMIC_STRUP_VCORE2_PG_STATUS_GATED_ADDR                    MT6353_PGSTATUS0
#define PMIC_STRUP_VCORE2_PG_STATUS_GATED_MASK                    0x1
#define PMIC_STRUP_VCORE2_PG_STATUS_GATED_SHIFT                   15
#define PMIC_PMU_THERMAL_DEB_ADDR                                 MT6353_THERMALSTATUS
#define PMIC_PMU_THERMAL_DEB_MASK                                 0x1
#define PMIC_PMU_THERMAL_DEB_SHIFT                                14
#define PMIC_STRUP_THERMAL_STATUS_ADDR                            MT6353_THERMALSTATUS
#define PMIC_STRUP_THERMAL_STATUS_MASK                            0x1
#define PMIC_STRUP_THERMAL_STATUS_SHIFT                           15
#define PMIC_PMU_TEST_MODE_SCAN_ADDR                              MT6353_TOPSTATUS
#define PMIC_PMU_TEST_MODE_SCAN_MASK                              0x1
#define PMIC_PMU_TEST_MODE_SCAN_SHIFT                             0
#define PMIC_PWRKEY_DEB_ADDR                                      MT6353_TOPSTATUS
#define PMIC_PWRKEY_DEB_MASK                                      0x1
#define PMIC_PWRKEY_DEB_SHIFT                                     1
#define PMIC_HOMEKEY_DEB_ADDR                                     MT6353_TOPSTATUS
#define PMIC_HOMEKEY_DEB_MASK                                     0x1
#define PMIC_HOMEKEY_DEB_SHIFT                                    2
#define PMIC_RTC_XTAL_DET_DONE_ADDR                               MT6353_TOPSTATUS
#define PMIC_RTC_XTAL_DET_DONE_MASK                               0x1
#define PMIC_RTC_XTAL_DET_DONE_SHIFT                              6
#define PMIC_XOSC32_ENB_DET_ADDR                                  MT6353_TOPSTATUS
#define PMIC_XOSC32_ENB_DET_MASK                                  0x1
#define PMIC_XOSC32_ENB_DET_SHIFT                                 7
#define PMIC_RTC_XTAL_DET_RSV_ADDR                                MT6353_TOPSTATUS
#define PMIC_RTC_XTAL_DET_RSV_MASK                                0xF
#define PMIC_RTC_XTAL_DET_RSV_SHIFT                               8
#define PMIC_RG_PMU_TDSEL_ADDR                                    MT6353_TDSEL_CON
#define PMIC_RG_PMU_TDSEL_MASK                                    0x1
#define PMIC_RG_PMU_TDSEL_SHIFT                                   0
#define PMIC_RG_SPI_TDSEL_ADDR                                    MT6353_TDSEL_CON
#define PMIC_RG_SPI_TDSEL_MASK                                    0x1
#define PMIC_RG_SPI_TDSEL_SHIFT                                   1
#define PMIC_RG_AUD_TDSEL_ADDR                                    MT6353_TDSEL_CON
#define PMIC_RG_AUD_TDSEL_MASK                                    0x1
#define PMIC_RG_AUD_TDSEL_SHIFT                                   2
#define PMIC_RG_E32CAL_TDSEL_ADDR                                 MT6353_TDSEL_CON
#define PMIC_RG_E32CAL_TDSEL_MASK                                 0x1
#define PMIC_RG_E32CAL_TDSEL_SHIFT                                3
#define PMIC_RG_PMU_RDSEL_ADDR                                    MT6353_RDSEL_CON
#define PMIC_RG_PMU_RDSEL_MASK                                    0x1
#define PMIC_RG_PMU_RDSEL_SHIFT                                   0
#define PMIC_RG_SPI_RDSEL_ADDR                                    MT6353_RDSEL_CON
#define PMIC_RG_SPI_RDSEL_MASK                                    0x1
#define PMIC_RG_SPI_RDSEL_SHIFT                                   1
#define PMIC_RG_AUD_RDSEL_ADDR                                    MT6353_RDSEL_CON
#define PMIC_RG_AUD_RDSEL_MASK                                    0x1
#define PMIC_RG_AUD_RDSEL_SHIFT                                   2
#define PMIC_RG_E32CAL_RDSEL_ADDR                                 MT6353_RDSEL_CON
#define PMIC_RG_E32CAL_RDSEL_MASK                                 0x1
#define PMIC_RG_E32CAL_RDSEL_SHIFT                                3
#define PMIC_RG_SMT_WDTRSTB_IN_ADDR                               MT6353_SMT_CON0
#define PMIC_RG_SMT_WDTRSTB_IN_MASK                               0x1
#define PMIC_RG_SMT_WDTRSTB_IN_SHIFT                              0
#define PMIC_RG_SMT_HOMEKEY_ADDR                                  MT6353_SMT_CON0
#define PMIC_RG_SMT_HOMEKEY_MASK                                  0x1
#define PMIC_RG_SMT_HOMEKEY_SHIFT                                 1
#define PMIC_RG_SMT_SRCLKEN_IN0_ADDR                              MT6353_SMT_CON0
#define PMIC_RG_SMT_SRCLKEN_IN0_MASK                              0x1
#define PMIC_RG_SMT_SRCLKEN_IN0_SHIFT                             2
#define PMIC_RG_SMT_SRCLKEN_IN1_ADDR                              MT6353_SMT_CON0
#define PMIC_RG_SMT_SRCLKEN_IN1_MASK                              0x1
#define PMIC_RG_SMT_SRCLKEN_IN1_SHIFT                             3
#define PMIC_RG_SMT_RTC_32K1V8_0_ADDR                             MT6353_SMT_CON0
#define PMIC_RG_SMT_RTC_32K1V8_0_MASK                             0x1
#define PMIC_RG_SMT_RTC_32K1V8_0_SHIFT                            4
#define PMIC_RG_SMT_RTC_32K1V8_1_ADDR                             MT6353_SMT_CON0
#define PMIC_RG_SMT_RTC_32K1V8_1_MASK                             0x1
#define PMIC_RG_SMT_RTC_32K1V8_1_SHIFT                            5
#define PMIC_RG_SMT_SPI_CLK_ADDR                                  MT6353_SMT_CON1
#define PMIC_RG_SMT_SPI_CLK_MASK                                  0x1
#define PMIC_RG_SMT_SPI_CLK_SHIFT                                 0
#define PMIC_RG_SMT_SPI_CSN_ADDR                                  MT6353_SMT_CON1
#define PMIC_RG_SMT_SPI_CSN_MASK                                  0x1
#define PMIC_RG_SMT_SPI_CSN_SHIFT                                 1
#define PMIC_RG_SMT_SPI_MOSI_ADDR                                 MT6353_SMT_CON1
#define PMIC_RG_SMT_SPI_MOSI_MASK                                 0x1
#define PMIC_RG_SMT_SPI_MOSI_SHIFT                                2
#define PMIC_RG_SMT_SPI_MISO_ADDR                                 MT6353_SMT_CON1
#define PMIC_RG_SMT_SPI_MISO_MASK                                 0x1
#define PMIC_RG_SMT_SPI_MISO_SHIFT                                3
#define PMIC_RG_SMT_AUD_CLK_ADDR                                  MT6353_SMT_CON2
#define PMIC_RG_SMT_AUD_CLK_MASK                                  0x1
#define PMIC_RG_SMT_AUD_CLK_SHIFT                                 0
#define PMIC_RG_SMT_AUD_DAT_MOSI_ADDR                             MT6353_SMT_CON2
#define PMIC_RG_SMT_AUD_DAT_MOSI_MASK                             0x1
#define PMIC_RG_SMT_AUD_DAT_MOSI_SHIFT                            1
#define PMIC_RG_SMT_AUD_DAT_MISO_ADDR                             MT6353_SMT_CON2
#define PMIC_RG_SMT_AUD_DAT_MISO_MASK                             0x1
#define PMIC_RG_SMT_AUD_DAT_MISO_SHIFT                            2
#define PMIC_RG_SMT_ANC_DAT_MOSI_ADDR                             MT6353_SMT_CON2
#define PMIC_RG_SMT_ANC_DAT_MOSI_MASK                             0x1
#define PMIC_RG_SMT_ANC_DAT_MOSI_SHIFT                            3
#define PMIC_RG_SMT_VOW_CLK_MISO_ADDR                             MT6353_SMT_CON2
#define PMIC_RG_SMT_VOW_CLK_MISO_MASK                             0x1
#define PMIC_RG_SMT_VOW_CLK_MISO_SHIFT                            4
#define PMIC_RG_SMT_ENBB_ADDR                                     MT6353_SMT_CON2
#define PMIC_RG_SMT_ENBB_MASK                                     0x1
#define PMIC_RG_SMT_ENBB_SHIFT                                    5
#define PMIC_RG_SMT_XOSC_EN_ADDR                                  MT6353_SMT_CON2
#define PMIC_RG_SMT_XOSC_EN_MASK                                  0x1
#define PMIC_RG_SMT_XOSC_EN_SHIFT                                 6
#define PMIC_RG_OCTL_SRCLKEN_IN0_ADDR                             MT6353_DRV_CON0
#define PMIC_RG_OCTL_SRCLKEN_IN0_MASK                             0xF
#define PMIC_RG_OCTL_SRCLKEN_IN0_SHIFT                            0
#define PMIC_RG_OCTL_SRCLKEN_IN1_ADDR                             MT6353_DRV_CON0
#define PMIC_RG_OCTL_SRCLKEN_IN1_MASK                             0xF
#define PMIC_RG_OCTL_SRCLKEN_IN1_SHIFT                            4
#define PMIC_RG_OCTL_RTC_32K1V8_0_ADDR                            MT6353_DRV_CON0
#define PMIC_RG_OCTL_RTC_32K1V8_0_MASK                            0xF
#define PMIC_RG_OCTL_RTC_32K1V8_0_SHIFT                           8
#define PMIC_RG_OCTL_RTC_32K1V8_1_ADDR                            MT6353_DRV_CON0
#define PMIC_RG_OCTL_RTC_32K1V8_1_MASK                            0xF
#define PMIC_RG_OCTL_RTC_32K1V8_1_SHIFT                           12
#define PMIC_RG_OCTL_SPI_CLK_ADDR                                 MT6353_DRV_CON1
#define PMIC_RG_OCTL_SPI_CLK_MASK                                 0xF
#define PMIC_RG_OCTL_SPI_CLK_SHIFT                                0
#define PMIC_RG_OCTL_SPI_CSN_ADDR                                 MT6353_DRV_CON1
#define PMIC_RG_OCTL_SPI_CSN_MASK                                 0xF
#define PMIC_RG_OCTL_SPI_CSN_SHIFT                                4
#define PMIC_RG_OCTL_SPI_MOSI_ADDR                                MT6353_DRV_CON1
#define PMIC_RG_OCTL_SPI_MOSI_MASK                                0xF
#define PMIC_RG_OCTL_SPI_MOSI_SHIFT                               8
#define PMIC_RG_OCTL_SPI_MISO_ADDR                                MT6353_DRV_CON1
#define PMIC_RG_OCTL_SPI_MISO_MASK                                0xF
#define PMIC_RG_OCTL_SPI_MISO_SHIFT                               12
#define PMIC_RG_OCTL_AUD_DAT_MOSI_ADDR                            MT6353_DRV_CON2
#define PMIC_RG_OCTL_AUD_DAT_MOSI_MASK                            0xF
#define PMIC_RG_OCTL_AUD_DAT_MOSI_SHIFT                           0
#define PMIC_RG_OCTL_AUD_DAT_MISO_ADDR                            MT6353_DRV_CON2
#define PMIC_RG_OCTL_AUD_DAT_MISO_MASK                            0xF
#define PMIC_RG_OCTL_AUD_DAT_MISO_SHIFT                           4
#define PMIC_RG_OCTL_AUD_CLK_ADDR                                 MT6353_DRV_CON2
#define PMIC_RG_OCTL_AUD_CLK_MASK                                 0xF
#define PMIC_RG_OCTL_AUD_CLK_SHIFT                                8
#define PMIC_RG_OCTL_ANC_DAT_MOSI_ADDR                            MT6353_DRV_CON2
#define PMIC_RG_OCTL_ANC_DAT_MOSI_MASK                            0xF
#define PMIC_RG_OCTL_ANC_DAT_MOSI_SHIFT                           12
#define PMIC_RG_OCTL_HOMEKEY_ADDR                                 MT6353_DRV_CON3
#define PMIC_RG_OCTL_HOMEKEY_MASK                                 0xF
#define PMIC_RG_OCTL_HOMEKEY_SHIFT                                0
#define PMIC_RG_OCTL_ENBB_ADDR                                    MT6353_DRV_CON3
#define PMIC_RG_OCTL_ENBB_MASK                                    0xF
#define PMIC_RG_OCTL_ENBB_SHIFT                                   4
#define PMIC_RG_OCTL_XOSC_EN_ADDR                                 MT6353_DRV_CON3
#define PMIC_RG_OCTL_XOSC_EN_MASK                                 0xF
#define PMIC_RG_OCTL_XOSC_EN_SHIFT                                8
#define PMIC_RG_OCTL_VOW_CLK_MISO_ADDR                            MT6353_DRV_CON3
#define PMIC_RG_OCTL_VOW_CLK_MISO_MASK                            0xF
#define PMIC_RG_OCTL_VOW_CLK_MISO_SHIFT                           12
#define PMIC_TOP_STATUS_ADDR                                      MT6353_TOP_STATUS
#define PMIC_TOP_STATUS_MASK                                      0xF
#define PMIC_TOP_STATUS_SHIFT                                     0
#define PMIC_TOP_STATUS_SET_ADDR                                  MT6353_TOP_STATUS_SET
#define PMIC_TOP_STATUS_SET_MASK                                  0x3
#define PMIC_TOP_STATUS_SET_SHIFT                                 0
#define PMIC_TOP_STATUS_CLR_ADDR                                  MT6353_TOP_STATUS_CLR
#define PMIC_TOP_STATUS_CLR_MASK                                  0x3
#define PMIC_TOP_STATUS_CLR_SHIFT                                 0
#define PMIC_CLK_RSV_CON0_RSV_ADDR                                MT6353_CLK_RSV_CON0
#define PMIC_CLK_RSV_CON0_RSV_MASK                                0x3FFF
#define PMIC_CLK_RSV_CON0_RSV_SHIFT                               0
#define PMIC_RG_DCXO_PWRKEY_RSTB_SEL_ADDR                         MT6353_CLK_RSV_CON0
#define PMIC_RG_DCXO_PWRKEY_RSTB_SEL_MASK                         0x1
#define PMIC_RG_DCXO_PWRKEY_RSTB_SEL_SHIFT                        14
#define PMIC_CLK_SRCVOLTEN_SW_ADDR                                MT6353_CLK_BUCK_CON0
#define PMIC_CLK_SRCVOLTEN_SW_MASK                                0x1
#define PMIC_CLK_SRCVOLTEN_SW_SHIFT                               0
#define PMIC_CLK_BUCK_OSC_SEL_SW_ADDR                             MT6353_CLK_BUCK_CON0
#define PMIC_CLK_BUCK_OSC_SEL_SW_MASK                             0x1
#define PMIC_CLK_BUCK_OSC_SEL_SW_SHIFT                            2
#define PMIC_CLK_VOWEN_SW_ADDR                                    MT6353_CLK_BUCK_CON0
#define PMIC_CLK_VOWEN_SW_MASK                                    0x1
#define PMIC_CLK_VOWEN_SW_SHIFT                                   3
#define PMIC_CLK_SRCVOLTEN_MODE_ADDR                              MT6353_CLK_BUCK_CON0
#define PMIC_CLK_SRCVOLTEN_MODE_MASK                              0x1
#define PMIC_CLK_SRCVOLTEN_MODE_SHIFT                             4
#define PMIC_CLK_BUCK_OSC_SEL_MODE_ADDR                           MT6353_CLK_BUCK_CON0
#define PMIC_CLK_BUCK_OSC_SEL_MODE_MASK                           0x1
#define PMIC_CLK_BUCK_OSC_SEL_MODE_SHIFT                          6
#define PMIC_CLK_VOWEN_MODE_ADDR                                  MT6353_CLK_BUCK_CON0
#define PMIC_CLK_VOWEN_MODE_MASK                                  0x1
#define PMIC_CLK_VOWEN_MODE_SHIFT                                 7
#define PMIC_RG_TOP_CKSEL_CON2_RSV_ADDR                           MT6353_CLK_BUCK_CON0
#define PMIC_RG_TOP_CKSEL_CON2_RSV_MASK                           0x3F
#define PMIC_RG_TOP_CKSEL_CON2_RSV_SHIFT                          8
#define PMIC_CLK_BUCK_CON0_RSV_ADDR                               MT6353_CLK_BUCK_CON0
#define PMIC_CLK_BUCK_CON0_RSV_MASK                               0x3
#define PMIC_CLK_BUCK_CON0_RSV_SHIFT                              14
#define PMIC_CLK_BUCK_CON0_SET_ADDR                               MT6353_CLK_BUCK_CON0_SET
#define PMIC_CLK_BUCK_CON0_SET_MASK                               0xFFFF
#define PMIC_CLK_BUCK_CON0_SET_SHIFT                              0
#define PMIC_CLK_BUCK_CON0_CLR_ADDR                               MT6353_CLK_BUCK_CON0_CLR
#define PMIC_CLK_BUCK_CON0_CLR_MASK                               0xFFFF
#define PMIC_CLK_BUCK_CON0_CLR_SHIFT                              0
#define PMIC_CLK_BUCK_VCORE_OSC_SEL_SW_ADDR                       MT6353_CLK_BUCK_CON1
#define PMIC_CLK_BUCK_VCORE_OSC_SEL_SW_MASK                       0x1
#define PMIC_CLK_BUCK_VCORE_OSC_SEL_SW_SHIFT                      0
#define PMIC_CLK_BUCK_VCORE2_OSC_SEL_SW_ADDR                      MT6353_CLK_BUCK_CON1
#define PMIC_CLK_BUCK_VCORE2_OSC_SEL_SW_MASK                      0x1
#define PMIC_CLK_BUCK_VCORE2_OSC_SEL_SW_SHIFT                     1
#define PMIC_CLK_BUCK_VPROC_OSC_SEL_SW_ADDR                       MT6353_CLK_BUCK_CON1
#define PMIC_CLK_BUCK_VPROC_OSC_SEL_SW_MASK                       0x1
#define PMIC_CLK_BUCK_VPROC_OSC_SEL_SW_SHIFT                      2
#define PMIC_CLK_BUCK_VPA_OSC_SEL_SW_ADDR                         MT6353_CLK_BUCK_CON1
#define PMIC_CLK_BUCK_VPA_OSC_SEL_SW_MASK                         0x1
#define PMIC_CLK_BUCK_VPA_OSC_SEL_SW_SHIFT                        3
#define PMIC_CLK_BUCK_VS1_OSC_SEL_SW_ADDR                         MT6353_CLK_BUCK_CON1
#define PMIC_CLK_BUCK_VS1_OSC_SEL_SW_MASK                         0x1
#define PMIC_CLK_BUCK_VS1_OSC_SEL_SW_SHIFT                        4
#define PMIC_CLK_LDO_VSRAM_PROC_OSC_SEL_SW_ADDR                   MT6353_CLK_BUCK_CON1
#define PMIC_CLK_LDO_VSRAM_PROC_OSC_SEL_SW_MASK                   0x1
#define PMIC_CLK_LDO_VSRAM_PROC_OSC_SEL_SW_SHIFT                  5
#define PMIC_CLK_BUCK_VCORE_OSC_SEL_MODE_ADDR                     MT6353_CLK_BUCK_CON1
#define PMIC_CLK_BUCK_VCORE_OSC_SEL_MODE_MASK                     0x1
#define PMIC_CLK_BUCK_VCORE_OSC_SEL_MODE_SHIFT                    8
#define PMIC_CLK_BUCK_VCORE2_OSC_SEL_MODE_ADDR                    MT6353_CLK_BUCK_CON1
#define PMIC_CLK_BUCK_VCORE2_OSC_SEL_MODE_MASK                    0x1
#define PMIC_CLK_BUCK_VCORE2_OSC_SEL_MODE_SHIFT                   9
#define PMIC_CLK_BUCK_VPROC_OSC_SEL_MODE_ADDR                     MT6353_CLK_BUCK_CON1
#define PMIC_CLK_BUCK_VPROC_OSC_SEL_MODE_MASK                     0x1
#define PMIC_CLK_BUCK_VPROC_OSC_SEL_MODE_SHIFT                    10
#define PMIC_CLK_BUCK_VPA_OSC_SEL_MODE_ADDR                       MT6353_CLK_BUCK_CON1
#define PMIC_CLK_BUCK_VPA_OSC_SEL_MODE_MASK                       0x1
#define PMIC_CLK_BUCK_VPA_OSC_SEL_MODE_SHIFT                      11
#define PMIC_CLK_BUCK_VS1_OSC_SEL_MODE_ADDR                       MT6353_CLK_BUCK_CON1
#define PMIC_CLK_BUCK_VS1_OSC_SEL_MODE_MASK                       0x1
#define PMIC_CLK_BUCK_VS1_OSC_SEL_MODE_SHIFT                      12
#define PMIC_CLK_LDO_VSRAM_PROC_OSC_SEL_MODE_ADDR                 MT6353_CLK_BUCK_CON1
#define PMIC_CLK_LDO_VSRAM_PROC_OSC_SEL_MODE_MASK                 0x1
#define PMIC_CLK_LDO_VSRAM_PROC_OSC_SEL_MODE_SHIFT                13
#define PMIC_CLK_BUCK_CON1_RSV_ADDR                               MT6353_CLK_BUCK_CON1
#define PMIC_CLK_BUCK_CON1_RSV_MASK                               0x3
#define PMIC_CLK_BUCK_CON1_RSV_SHIFT                              14
#define PMIC_CLK_BUCK_CON1_SET_ADDR                               MT6353_CLK_BUCK_CON1_SET
#define PMIC_CLK_BUCK_CON1_SET_MASK                               0xFFFF
#define PMIC_CLK_BUCK_CON1_SET_SHIFT                              0
#define PMIC_CLK_BUCK_CON1_CLR_ADDR                               MT6353_CLK_BUCK_CON1_CLR
#define PMIC_CLK_BUCK_CON1_CLR_MASK                               0xFFFF
#define PMIC_CLK_BUCK_CON1_CLR_SHIFT                              0
#define PMIC_RG_CLKSQ_EN_AUD_ADDR                                 MT6353_TOP_CLKSQ
#define PMIC_RG_CLKSQ_EN_AUD_MASK                                 0x1
#define PMIC_RG_CLKSQ_EN_AUD_SHIFT                                0
#define PMIC_RG_CLKSQ_EN_FQR_ADDR                                 MT6353_TOP_CLKSQ
#define PMIC_RG_CLKSQ_EN_FQR_MASK                                 0x1
#define PMIC_RG_CLKSQ_EN_FQR_SHIFT                                1
#define PMIC_RG_CLKSQ_EN_AUX_AP_ADDR                              MT6353_TOP_CLKSQ
#define PMIC_RG_CLKSQ_EN_AUX_AP_MASK                              0x1
#define PMIC_RG_CLKSQ_EN_AUX_AP_SHIFT                             2
#define PMIC_RG_CLKSQ_EN_AUX_MD_ADDR                              MT6353_TOP_CLKSQ
#define PMIC_RG_CLKSQ_EN_AUX_MD_MASK                              0x1
#define PMIC_RG_CLKSQ_EN_AUX_MD_SHIFT                             3
#define PMIC_RG_CLKSQ_EN_AUX_GPS_ADDR                             MT6353_TOP_CLKSQ
#define PMIC_RG_CLKSQ_EN_AUX_GPS_MASK                             0x1
#define PMIC_RG_CLKSQ_EN_AUX_GPS_SHIFT                            4
#define PMIC_RG_CLKSQ_EN_AUX_RSV_ADDR                             MT6353_TOP_CLKSQ
#define PMIC_RG_CLKSQ_EN_AUX_RSV_MASK                             0x1
#define PMIC_RG_CLKSQ_EN_AUX_RSV_SHIFT                            5
#define PMIC_RG_CLKSQ_EN_AUX_AP_MODE_ADDR                         MT6353_TOP_CLKSQ
#define PMIC_RG_CLKSQ_EN_AUX_AP_MODE_MASK                         0x1
#define PMIC_RG_CLKSQ_EN_AUX_AP_MODE_SHIFT                        8
#define PMIC_RG_CLKSQ_EN_AUX_MD_MODE_ADDR                         MT6353_TOP_CLKSQ
#define PMIC_RG_CLKSQ_EN_AUX_MD_MODE_MASK                         0x1
#define PMIC_RG_CLKSQ_EN_AUX_MD_MODE_SHIFT                        9
#define PMIC_RG_CLKSQ_IN_SEL_VA18_ADDR                            MT6353_TOP_CLKSQ
#define PMIC_RG_CLKSQ_IN_SEL_VA18_MASK                            0x1
#define PMIC_RG_CLKSQ_IN_SEL_VA18_SHIFT                           10
#define PMIC_RG_CLKSQ_IN_SEL_VA18_SWCTRL_ADDR                     MT6353_TOP_CLKSQ
#define PMIC_RG_CLKSQ_IN_SEL_VA18_SWCTRL_MASK                     0x1
#define PMIC_RG_CLKSQ_IN_SEL_VA18_SWCTRL_SHIFT                    11
#define PMIC_TOP_CLKSQ_RSV_ADDR                                   MT6353_TOP_CLKSQ
#define PMIC_TOP_CLKSQ_RSV_MASK                                   0x7
#define PMIC_TOP_CLKSQ_RSV_SHIFT                                  12
#define PMIC_DA_CLKSQ_EN_VA28_ADDR                                MT6353_TOP_CLKSQ
#define PMIC_DA_CLKSQ_EN_VA28_MASK                                0x1
#define PMIC_DA_CLKSQ_EN_VA28_SHIFT                               15
#define PMIC_TOP_CLKSQ_SET_ADDR                                   MT6353_TOP_CLKSQ_SET
#define PMIC_TOP_CLKSQ_SET_MASK                                   0xFFFF
#define PMIC_TOP_CLKSQ_SET_SHIFT                                  0
#define PMIC_TOP_CLKSQ_CLR_ADDR                                   MT6353_TOP_CLKSQ_CLR
#define PMIC_TOP_CLKSQ_CLR_MASK                                   0xFFFF
#define PMIC_TOP_CLKSQ_CLR_SHIFT                                  0
#define PMIC_RG_CLKSQ_RTC_EN_ADDR                                 MT6353_TOP_CLKSQ_RTC
#define PMIC_RG_CLKSQ_RTC_EN_MASK                                 0x1
#define PMIC_RG_CLKSQ_RTC_EN_SHIFT                                0
#define PMIC_RG_CLKSQ_RTC_EN_HW_MODE_ADDR                         MT6353_TOP_CLKSQ_RTC
#define PMIC_RG_CLKSQ_RTC_EN_HW_MODE_MASK                         0x1
#define PMIC_RG_CLKSQ_RTC_EN_HW_MODE_SHIFT                        1
#define PMIC_TOP_CLKSQ_RTC_RSV0_ADDR                              MT6353_TOP_CLKSQ_RTC
#define PMIC_TOP_CLKSQ_RTC_RSV0_MASK                              0xF
#define PMIC_TOP_CLKSQ_RTC_RSV0_SHIFT                             2
#define PMIC_RG_ENBB_SEL_ADDR                                     MT6353_TOP_CLKSQ_RTC
#define PMIC_RG_ENBB_SEL_MASK                                     0x1
#define PMIC_RG_ENBB_SEL_SHIFT                                    8
#define PMIC_RG_XOSC_EN_SEL_ADDR                                  MT6353_TOP_CLKSQ_RTC
#define PMIC_RG_XOSC_EN_SEL_MASK                                  0x1
#define PMIC_RG_XOSC_EN_SEL_SHIFT                                 9
#define PMIC_TOP_CLKSQ_RTC_RSV1_ADDR                              MT6353_TOP_CLKSQ_RTC
#define PMIC_TOP_CLKSQ_RTC_RSV1_MASK                              0x3
#define PMIC_TOP_CLKSQ_RTC_RSV1_SHIFT                             10
#define PMIC_DA_CLKSQ_EN_VDIG18_ADDR                              MT6353_TOP_CLKSQ_RTC
#define PMIC_DA_CLKSQ_EN_VDIG18_MASK                              0x1
#define PMIC_DA_CLKSQ_EN_VDIG18_SHIFT                             15
#define PMIC_TOP_CLKSQ_RTC_SET_ADDR                               MT6353_TOP_CLKSQ_RTC_SET
#define PMIC_TOP_CLKSQ_RTC_SET_MASK                               0xFFFF
#define PMIC_TOP_CLKSQ_RTC_SET_SHIFT                              0
#define PMIC_TOP_CLKSQ_RTC_CLR_ADDR                               MT6353_TOP_CLKSQ_RTC_CLR
#define PMIC_TOP_CLKSQ_RTC_CLR_MASK                               0xFFFF
#define PMIC_TOP_CLKSQ_RTC_CLR_SHIFT                              0
#define PMIC_OSC_75K_TRIM_ADDR                                    MT6353_TOP_CLK_TRIM
#define PMIC_OSC_75K_TRIM_MASK                                    0x1F
#define PMIC_OSC_75K_TRIM_SHIFT                                   0
#define PMIC_RG_OSC_75K_TRIM_EN_ADDR                              MT6353_TOP_CLK_TRIM
#define PMIC_RG_OSC_75K_TRIM_EN_MASK                              0x1
#define PMIC_RG_OSC_75K_TRIM_EN_SHIFT                             5
#define PMIC_RG_OSC_75K_TRIM_RATE_ADDR                            MT6353_TOP_CLK_TRIM
#define PMIC_RG_OSC_75K_TRIM_RATE_MASK                            0x3
#define PMIC_RG_OSC_75K_TRIM_RATE_SHIFT                           6
#define PMIC_DA_OSC_75K_TRIM_ADDR                                 MT6353_TOP_CLK_TRIM
#define PMIC_DA_OSC_75K_TRIM_MASK                                 0x1F
#define PMIC_DA_OSC_75K_TRIM_SHIFT                                8
#define PMIC_CLK_PMU75K_CK_TST_DIS_ADDR                           MT6353_CLK_CKROOTTST_CON0
#define PMIC_CLK_PMU75K_CK_TST_DIS_MASK                           0x1
#define PMIC_CLK_PMU75K_CK_TST_DIS_SHIFT                          0
#define PMIC_CLK_SMPS_CK_TST_DIS_ADDR                             MT6353_CLK_CKROOTTST_CON0
#define PMIC_CLK_SMPS_CK_TST_DIS_MASK                             0x1
#define PMIC_CLK_SMPS_CK_TST_DIS_SHIFT                            1
#define PMIC_CLK_AUD26M_CK_TST_DIS_ADDR                           MT6353_CLK_CKROOTTST_CON0
#define PMIC_CLK_AUD26M_CK_TST_DIS_MASK                           0x1
#define PMIC_CLK_AUD26M_CK_TST_DIS_SHIFT                          2
#define PMIC_CLK_VOW12M_CK_TST_DIS_ADDR                           MT6353_CLK_CKROOTTST_CON0
#define PMIC_CLK_VOW12M_CK_TST_DIS_MASK                           0x1
#define PMIC_CLK_VOW12M_CK_TST_DIS_SHIFT                          3
#define PMIC_CLK_RTC32K_CK_TST_DIS_ADDR                           MT6353_CLK_CKROOTTST_CON0
#define PMIC_CLK_RTC32K_CK_TST_DIS_MASK                           0x1
#define PMIC_CLK_RTC32K_CK_TST_DIS_SHIFT                          4
#define PMIC_CLK_RTC26M_CK_TST_DIS_ADDR                           MT6353_CLK_CKROOTTST_CON0
#define PMIC_CLK_RTC26M_CK_TST_DIS_MASK                           0x1
#define PMIC_CLK_RTC26M_CK_TST_DIS_SHIFT                          5
#define PMIC_CLK_FG_CK_TST_DIS_ADDR                               MT6353_CLK_CKROOTTST_CON0
#define PMIC_CLK_FG_CK_TST_DIS_MASK                               0x1
#define PMIC_CLK_FG_CK_TST_DIS_SHIFT                              6
#define PMIC_CLK_SPK_CK_TST_DIS_ADDR                              MT6353_CLK_CKROOTTST_CON0
#define PMIC_CLK_SPK_CK_TST_DIS_MASK                              0x1
#define PMIC_CLK_SPK_CK_TST_DIS_SHIFT                             7
#define PMIC_CLK_CKROOTTST_CON0_RSV_ADDR                          MT6353_CLK_CKROOTTST_CON0
#define PMIC_CLK_CKROOTTST_CON0_RSV_MASK                          0x7F
#define PMIC_CLK_CKROOTTST_CON0_RSV_SHIFT                         8
#define PMIC_CLK_BUCK_ANA_AUTO_OFF_DIS_ADDR                       MT6353_CLK_CKROOTTST_CON0
#define PMIC_CLK_BUCK_ANA_AUTO_OFF_DIS_MASK                       0x1
#define PMIC_CLK_BUCK_ANA_AUTO_OFF_DIS_SHIFT                      15
#define PMIC_CLK_PMU75K_CK_TSTSEL_ADDR                            MT6353_CLK_CKROOTTST_CON1
#define PMIC_CLK_PMU75K_CK_TSTSEL_MASK                            0x1
#define PMIC_CLK_PMU75K_CK_TSTSEL_SHIFT                           0
#define PMIC_CLK_SMPS_CK_TSTSEL_ADDR                              MT6353_CLK_CKROOTTST_CON1
#define PMIC_CLK_SMPS_CK_TSTSEL_MASK                              0x1
#define PMIC_CLK_SMPS_CK_TSTSEL_SHIFT                             1
#define PMIC_CLK_AUD26M_CK_TSTSEL_ADDR                            MT6353_CLK_CKROOTTST_CON1
#define PMIC_CLK_AUD26M_CK_TSTSEL_MASK                            0x1
#define PMIC_CLK_AUD26M_CK_TSTSEL_SHIFT                           2
#define PMIC_CLK_VOW12M_CK_TSTSEL_ADDR                            MT6353_CLK_CKROOTTST_CON1
#define PMIC_CLK_VOW12M_CK_TSTSEL_MASK                            0x1
#define PMIC_CLK_VOW12M_CK_TSTSEL_SHIFT                           3
#define PMIC_CLK_RTC32K_CK_TSTSEL_ADDR                            MT6353_CLK_CKROOTTST_CON1
#define PMIC_CLK_RTC32K_CK_TSTSEL_MASK                            0x1
#define PMIC_CLK_RTC32K_CK_TSTSEL_SHIFT                           4
#define PMIC_CLK_RTC26M_CK_TSTSEL_ADDR                            MT6353_CLK_CKROOTTST_CON1
#define PMIC_CLK_RTC26M_CK_TSTSEL_MASK                            0x1
#define PMIC_CLK_RTC26M_CK_TSTSEL_SHIFT                           5
#define PMIC_CLK_FG_CK_TSTSEL_ADDR                                MT6353_CLK_CKROOTTST_CON1
#define PMIC_CLK_FG_CK_TSTSEL_MASK                                0x1
#define PMIC_CLK_FG_CK_TSTSEL_SHIFT                               6
#define PMIC_CLK_SPK_CK_TSTSEL_ADDR                               MT6353_CLK_CKROOTTST_CON1
#define PMIC_CLK_SPK_CK_TSTSEL_MASK                               0x1
#define PMIC_CLK_SPK_CK_TSTSEL_SHIFT                              7
#define PMIC_CLK_CKROOTTST_CON1_RSV_ADDR                          MT6353_CLK_CKROOTTST_CON1
#define PMIC_CLK_CKROOTTST_CON1_RSV_MASK                          0xFF
#define PMIC_CLK_CKROOTTST_CON1_RSV_SHIFT                         8
#define PMIC_CLK_RTC32K_1V8_0_O_PDN_ADDR                          MT6353_CLK_PDN_CON0
#define PMIC_CLK_RTC32K_1V8_0_O_PDN_MASK                          0x1
#define PMIC_CLK_RTC32K_1V8_0_O_PDN_SHIFT                         0
#define PMIC_CLK_RTC32K_1V8_1_O_PDN_ADDR                          MT6353_CLK_PDN_CON0
#define PMIC_CLK_RTC32K_1V8_1_O_PDN_MASK                          0x1
#define PMIC_CLK_RTC32K_1V8_1_O_PDN_SHIFT                         1
#define PMIC_CLK_RTC_2SEC_OFF_DET_PDN_ADDR                        MT6353_CLK_PDN_CON0
#define PMIC_CLK_RTC_2SEC_OFF_DET_PDN_MASK                        0x1
#define PMIC_CLK_RTC_2SEC_OFF_DET_PDN_SHIFT                       2
#define PMIC_CLK_PDN_CON0_RSV_ADDR                                MT6353_CLK_PDN_CON0
#define PMIC_CLK_PDN_CON0_RSV_MASK                                0x1FFF
#define PMIC_CLK_PDN_CON0_RSV_SHIFT                               3
#define PMIC_CLK_PDN_CON0_SET_ADDR                                MT6353_CLK_PDN_CON0_SET
#define PMIC_CLK_PDN_CON0_SET_MASK                                0xFFFF
#define PMIC_CLK_PDN_CON0_SET_SHIFT                               0
#define PMIC_CLK_PDN_CON0_CLR_ADDR                                MT6353_CLK_PDN_CON0_CLR
#define PMIC_CLK_PDN_CON0_CLR_MASK                                0xFFFF
#define PMIC_CLK_PDN_CON0_CLR_SHIFT                               0
#define PMIC_CLK_ZCD13M_CK_PDN_ADDR                               MT6353_CLK_PDN_CON1
#define PMIC_CLK_ZCD13M_CK_PDN_MASK                               0x1
#define PMIC_CLK_ZCD13M_CK_PDN_SHIFT                              0
#define PMIC_CLK_SMPS_CK_DIV_PDN_ADDR                             MT6353_CLK_PDN_CON1
#define PMIC_CLK_SMPS_CK_DIV_PDN_MASK                             0x1
#define PMIC_CLK_SMPS_CK_DIV_PDN_SHIFT                            1
#define PMIC_CLK_BUCK_ANA_CK_PDN_ADDR                             MT6353_CLK_PDN_CON1
#define PMIC_CLK_BUCK_ANA_CK_PDN_MASK                             0x1
#define PMIC_CLK_BUCK_ANA_CK_PDN_SHIFT                            3
#define PMIC_CLK_PDN_CON1_RSV_ADDR                                MT6353_CLK_PDN_CON1
#define PMIC_CLK_PDN_CON1_RSV_MASK                                0xFFF
#define PMIC_CLK_PDN_CON1_RSV_SHIFT                               4
#define PMIC_CLK_PDN_CON1_SET_ADDR                                MT6353_CLK_PDN_CON1_SET
#define PMIC_CLK_PDN_CON1_SET_MASK                                0xFFFF
#define PMIC_CLK_PDN_CON1_SET_SHIFT                               0
#define PMIC_CLK_PDN_CON1_CLR_ADDR                                MT6353_CLK_PDN_CON1_CLR
#define PMIC_CLK_PDN_CON1_CLR_MASK                                0xFFFF
#define PMIC_CLK_PDN_CON1_CLR_SHIFT                               0
#define PMIC_CLK_OSC_SEL_HW_SRC_SEL_ADDR                          MT6353_CLK_SEL_CON0
#define PMIC_CLK_OSC_SEL_HW_SRC_SEL_MASK                          0x3
#define PMIC_CLK_OSC_SEL_HW_SRC_SEL_SHIFT                         0
#define PMIC_CLK_SRCLKEN_SRC_SEL_ADDR                             MT6353_CLK_SEL_CON0
#define PMIC_CLK_SRCLKEN_SRC_SEL_MASK                             0x3
#define PMIC_CLK_SRCLKEN_SRC_SEL_SHIFT                            2
#define PMIC_CLK_SEL_CON0_RSV_ADDR                                MT6353_CLK_SEL_CON0
#define PMIC_CLK_SEL_CON0_RSV_MASK                                0xFFF
#define PMIC_CLK_SEL_CON0_RSV_SHIFT                               4
#define PMIC_CLK_SEL_CON0_SET_ADDR                                MT6353_CLK_SEL_CON0_SET
#define PMIC_CLK_SEL_CON0_SET_MASK                                0xFFFF
#define PMIC_CLK_SEL_CON0_SET_SHIFT                               0
#define PMIC_CLK_SEL_CON0_CLR_ADDR                                MT6353_CLK_SEL_CON0_CLR
#define PMIC_CLK_SEL_CON0_CLR_MASK                                0xFFFF
#define PMIC_CLK_SEL_CON0_CLR_SHIFT                               0
#define PMIC_CLK_AUXADC_CK_CKSEL_HWEN_ADDR                        MT6353_CLK_SEL_CON1
#define PMIC_CLK_AUXADC_CK_CKSEL_HWEN_MASK                        0x1
#define PMIC_CLK_AUXADC_CK_CKSEL_HWEN_SHIFT                       0
#define PMIC_CLK_AUXADC_CK_CKSEL_ADDR                             MT6353_CLK_SEL_CON1
#define PMIC_CLK_AUXADC_CK_CKSEL_MASK                             0x1
#define PMIC_CLK_AUXADC_CK_CKSEL_SHIFT                            1
#define PMIC_CLK_75K_32K_SEL_ADDR                                 MT6353_CLK_SEL_CON1
#define PMIC_CLK_75K_32K_SEL_MASK                                 0x1
#define PMIC_CLK_75K_32K_SEL_SHIFT                                2
#define PMIC_CLK_SEL_CON1_RSV_ADDR                                MT6353_CLK_SEL_CON1
#define PMIC_CLK_SEL_CON1_RSV_MASK                                0xFFF
#define PMIC_CLK_SEL_CON1_RSV_SHIFT                               4
#define PMIC_CLK_SEL_CON1_SET_ADDR                                MT6353_CLK_SEL_CON1_SET
#define PMIC_CLK_SEL_CON1_SET_MASK                                0xFFFF
#define PMIC_CLK_SEL_CON1_SET_SHIFT                               0
#define PMIC_CLK_SEL_CON1_CLR_ADDR                                MT6353_CLK_SEL_CON1_CLR
#define PMIC_CLK_SEL_CON1_CLR_MASK                                0xFFFF
#define PMIC_CLK_SEL_CON1_CLR_SHIFT                               0
#define PMIC_CLK_G_SMPS_PD_CK_PDN_ADDR                            MT6353_CLK_CKPDN_CON0
#define PMIC_CLK_G_SMPS_PD_CK_PDN_MASK                            0x1
#define PMIC_CLK_G_SMPS_PD_CK_PDN_SHIFT                           0
#define PMIC_CLK_G_SMPS_AUD_CK_PDN_ADDR                           MT6353_CLK_CKPDN_CON0
#define PMIC_CLK_G_SMPS_AUD_CK_PDN_MASK                           0x1
#define PMIC_CLK_G_SMPS_AUD_CK_PDN_SHIFT                          1
#define PMIC_CLK_G_SMPS_TEST_CK_PDN_ADDR                          MT6353_CLK_CKPDN_CON0
#define PMIC_CLK_G_SMPS_TEST_CK_PDN_MASK                          0x1
#define PMIC_CLK_G_SMPS_TEST_CK_PDN_SHIFT                         2
#define PMIC_CLK_ACCDET_CK_PDN_ADDR                               MT6353_CLK_CKPDN_CON0
#define PMIC_CLK_ACCDET_CK_PDN_MASK                               0x1
#define PMIC_CLK_ACCDET_CK_PDN_SHIFT                              3
#define PMIC_CLK_AUDIF_CK_PDN_ADDR                                MT6353_CLK_CKPDN_CON0
#define PMIC_CLK_AUDIF_CK_PDN_MASK                                0x1
#define PMIC_CLK_AUDIF_CK_PDN_SHIFT                               4
#define PMIC_CLK_AUD_CK_PDN_ADDR                                  MT6353_CLK_CKPDN_CON0
#define PMIC_CLK_AUD_CK_PDN_MASK                                  0x1
#define PMIC_CLK_AUD_CK_PDN_SHIFT                                 5
#define PMIC_CLK_AUDNCP_CK_PDN_ADDR                               MT6353_CLK_CKPDN_CON0
#define PMIC_CLK_AUDNCP_CK_PDN_MASK                               0x1
#define PMIC_CLK_AUDNCP_CK_PDN_SHIFT                              6
#define PMIC_CLK_AUXADC_1M_CK_PDN_ADDR                            MT6353_CLK_CKPDN_CON0
#define PMIC_CLK_AUXADC_1M_CK_PDN_MASK                            0x1
#define PMIC_CLK_AUXADC_1M_CK_PDN_SHIFT                           7
#define PMIC_CLK_AUXADC_SMPS_CK_PDN_ADDR                          MT6353_CLK_CKPDN_CON0
#define PMIC_CLK_AUXADC_SMPS_CK_PDN_MASK                          0x1
#define PMIC_CLK_AUXADC_SMPS_CK_PDN_SHIFT                         8
#define PMIC_CLK_AUXADC_RNG_CK_PDN_ADDR                           MT6353_CLK_CKPDN_CON0
#define PMIC_CLK_AUXADC_RNG_CK_PDN_MASK                           0x1
#define PMIC_CLK_AUXADC_RNG_CK_PDN_SHIFT                          9
#define PMIC_CLK_AUXADC_26M_CK_PDN_ADDR                           MT6353_CLK_CKPDN_CON0
#define PMIC_CLK_AUXADC_26M_CK_PDN_MASK                           0x1
#define PMIC_CLK_AUXADC_26M_CK_PDN_SHIFT                          10
#define PMIC_CLK_AUXADC_CK_PDN_ADDR                               MT6353_CLK_CKPDN_CON0
#define PMIC_CLK_AUXADC_CK_PDN_MASK                               0x1
#define PMIC_CLK_AUXADC_CK_PDN_SHIFT                              11
#define PMIC_CLK_DRV_ISINK0_CK_PDN_ADDR                           MT6353_CLK_CKPDN_CON0
#define PMIC_CLK_DRV_ISINK0_CK_PDN_MASK                           0x1
#define PMIC_CLK_DRV_ISINK0_CK_PDN_SHIFT                          12
#define PMIC_CLK_DRV_ISINK1_CK_PDN_ADDR                           MT6353_CLK_CKPDN_CON0
#define PMIC_CLK_DRV_ISINK1_CK_PDN_MASK                           0x1
#define PMIC_CLK_DRV_ISINK1_CK_PDN_SHIFT                          13
#define PMIC_CLK_DRV_ISINK2_CK_PDN_ADDR                           MT6353_CLK_CKPDN_CON0
#define PMIC_CLK_DRV_ISINK2_CK_PDN_MASK                           0x1
#define PMIC_CLK_DRV_ISINK2_CK_PDN_SHIFT                          14
#define PMIC_CLK_DRV_ISINK3_CK_PDN_ADDR                           MT6353_CLK_CKPDN_CON0
#define PMIC_CLK_DRV_ISINK3_CK_PDN_MASK                           0x1
#define PMIC_CLK_DRV_ISINK3_CK_PDN_SHIFT                          15
#define PMIC_CLK_CKPDN_CON0_SET_ADDR                              MT6353_CLK_CKPDN_CON0_SET
#define PMIC_CLK_CKPDN_CON0_SET_MASK                              0xFFFF
#define PMIC_CLK_CKPDN_CON0_SET_SHIFT                             0
#define PMIC_CLK_CKPDN_CON0_CLR_ADDR                              MT6353_CLK_CKPDN_CON0_CLR
#define PMIC_CLK_CKPDN_CON0_CLR_MASK                              0xFFFF
#define PMIC_CLK_CKPDN_CON0_CLR_SHIFT                             0
#define PMIC_CLK_DRV_CHRIND_CK_PDN_ADDR                           MT6353_CLK_CKPDN_CON1
#define PMIC_CLK_DRV_CHRIND_CK_PDN_MASK                           0x1
#define PMIC_CLK_DRV_CHRIND_CK_PDN_SHIFT                          0
#define PMIC_CLK_DRV_32K_CK_PDN_ADDR                              MT6353_CLK_CKPDN_CON1
#define PMIC_CLK_DRV_32K_CK_PDN_MASK                              0x1
#define PMIC_CLK_DRV_32K_CK_PDN_SHIFT                             1
#define PMIC_CLK_BUCK_1M_CK_PDN_ADDR                              MT6353_CLK_CKPDN_CON1
#define PMIC_CLK_BUCK_1M_CK_PDN_MASK                              0x1
#define PMIC_CLK_BUCK_1M_CK_PDN_SHIFT                             2
#define PMIC_CLK_BUCK_32K_CK_PDN_ADDR                             MT6353_CLK_CKPDN_CON1
#define PMIC_CLK_BUCK_32K_CK_PDN_MASK                             0x1
#define PMIC_CLK_BUCK_32K_CK_PDN_SHIFT                            3
#define PMIC_CLK_BUCK_9M_CK_PDN_ADDR                              MT6353_CLK_CKPDN_CON1
#define PMIC_CLK_BUCK_9M_CK_PDN_MASK                              0x1
#define PMIC_CLK_BUCK_9M_CK_PDN_SHIFT                             4
#define PMIC_CLK_PWMOC_6M_CK_PDN_ADDR                             MT6353_CLK_CKPDN_CON1
#define PMIC_CLK_PWMOC_6M_CK_PDN_MASK                             0x1
#define PMIC_CLK_PWMOC_6M_CK_PDN_SHIFT                            5
#define PMIC_CLK_BUCK_VPROC_9M_CK_PDN_ADDR                        MT6353_CLK_CKPDN_CON1
#define PMIC_CLK_BUCK_VPROC_9M_CK_PDN_MASK                        0x1
#define PMIC_CLK_BUCK_VPROC_9M_CK_PDN_SHIFT                       6
#define PMIC_CLK_BUCK_VCORE_9M_CK_PDN_ADDR                        MT6353_CLK_CKPDN_CON1
#define PMIC_CLK_BUCK_VCORE_9M_CK_PDN_MASK                        0x1
#define PMIC_CLK_BUCK_VCORE_9M_CK_PDN_SHIFT                       7
#define PMIC_CLK_BUCK_VCORE2_9M_CK_PDN_ADDR                       MT6353_CLK_CKPDN_CON1
#define PMIC_CLK_BUCK_VCORE2_9M_CK_PDN_MASK                       0x1
#define PMIC_CLK_BUCK_VCORE2_9M_CK_PDN_SHIFT                      8
#define PMIC_CLK_BUCK_VPA_9M_CK_PDN_ADDR                          MT6353_CLK_CKPDN_CON1
#define PMIC_CLK_BUCK_VPA_9M_CK_PDN_MASK                          0x1
#define PMIC_CLK_BUCK_VPA_9M_CK_PDN_SHIFT                         9
#define PMIC_CLK_BUCK_VS1_9M_CK_PDN_ADDR                          MT6353_CLK_CKPDN_CON1
#define PMIC_CLK_BUCK_VS1_9M_CK_PDN_MASK                          0x1
#define PMIC_CLK_BUCK_VS1_9M_CK_PDN_SHIFT                         10
#define PMIC_CLK_FQMTR_CK_PDN_ADDR                                MT6353_CLK_CKPDN_CON1
#define PMIC_CLK_FQMTR_CK_PDN_MASK                                0x1
#define PMIC_CLK_FQMTR_CK_PDN_SHIFT                               11
#define PMIC_CLK_FQMTR_32K_CK_PDN_ADDR                            MT6353_CLK_CKPDN_CON1
#define PMIC_CLK_FQMTR_32K_CK_PDN_MASK                            0x1
#define PMIC_CLK_FQMTR_32K_CK_PDN_SHIFT                           12
#define PMIC_CLK_INTRP_CK_PDN_ADDR                                MT6353_CLK_CKPDN_CON1
#define PMIC_CLK_INTRP_CK_PDN_MASK                                0x1
#define PMIC_CLK_INTRP_CK_PDN_SHIFT                               13
#define PMIC_CLK_INTRP_PRE_OC_CK_PDN_ADDR                         MT6353_CLK_CKPDN_CON1
#define PMIC_CLK_INTRP_PRE_OC_CK_PDN_MASK                         0x1
#define PMIC_CLK_INTRP_PRE_OC_CK_PDN_SHIFT                        14
#define PMIC_CLK_STB_1M_CK_PDN_ADDR                               MT6353_CLK_CKPDN_CON1
#define PMIC_CLK_STB_1M_CK_PDN_MASK                               0x1
#define PMIC_CLK_STB_1M_CK_PDN_SHIFT                              15
#define PMIC_CLK_CKPDN_CON1_SET_ADDR                              MT6353_CLK_CKPDN_CON1_SET
#define PMIC_CLK_CKPDN_CON1_SET_MASK                              0xFFFF
#define PMIC_CLK_CKPDN_CON1_SET_SHIFT                             0
#define PMIC_CLK_CKPDN_CON1_CLR_ADDR                              MT6353_CLK_CKPDN_CON1_CLR
#define PMIC_CLK_CKPDN_CON1_CLR_MASK                              0xFFFF
#define PMIC_CLK_CKPDN_CON1_CLR_SHIFT                             0
#define PMIC_CLK_LDO_CALI_75K_CK_PDN_ADDR                         MT6353_CLK_CKPDN_CON2
#define PMIC_CLK_LDO_CALI_75K_CK_PDN_MASK                         0x1
#define PMIC_CLK_LDO_CALI_75K_CK_PDN_SHIFT                        0
#define PMIC_CLK_LDO_VSRAM_PROC_9M_CK_PDN_ADDR                    MT6353_CLK_CKPDN_CON2
#define PMIC_CLK_LDO_VSRAM_PROC_9M_CK_PDN_MASK                    0x1
#define PMIC_CLK_LDO_VSRAM_PROC_9M_CK_PDN_SHIFT                   1
#define PMIC_CLK_RTC_26M_CK_PDN_ADDR                              MT6353_CLK_CKPDN_CON2
#define PMIC_CLK_RTC_26M_CK_PDN_MASK                              0x1
#define PMIC_CLK_RTC_26M_CK_PDN_SHIFT                             2
#define PMIC_CLK_RTC_32K_CK_PDN_ADDR                              MT6353_CLK_CKPDN_CON2
#define PMIC_CLK_RTC_32K_CK_PDN_MASK                              0x1
#define PMIC_CLK_RTC_32K_CK_PDN_SHIFT                             3
#define PMIC_CLK_RTC_MCLK_PDN_ADDR                                MT6353_CLK_CKPDN_CON2
#define PMIC_CLK_RTC_MCLK_PDN_MASK                                0x1
#define PMIC_CLK_RTC_MCLK_PDN_SHIFT                               4
#define PMIC_CLK_RTC_75K_CK_PDN_ADDR                              MT6353_CLK_CKPDN_CON2
#define PMIC_CLK_RTC_75K_CK_PDN_MASK                              0x1
#define PMIC_CLK_RTC_75K_CK_PDN_SHIFT                             5
#define PMIC_CLK_RTCDET_CK_PDN_ADDR                               MT6353_CLK_CKPDN_CON2
#define PMIC_CLK_RTCDET_CK_PDN_MASK                               0x1
#define PMIC_CLK_RTCDET_CK_PDN_SHIFT                              6
#define PMIC_CLK_RTC_EOSC32_CK_PDN_ADDR                           MT6353_CLK_CKPDN_CON2
#define PMIC_CLK_RTC_EOSC32_CK_PDN_MASK                           0x1
#define PMIC_CLK_RTC_EOSC32_CK_PDN_SHIFT                          7
#define PMIC_CLK_EOSC_CALI_TEST_CK_PDN_ADDR                       MT6353_CLK_CKPDN_CON2
#define PMIC_CLK_EOSC_CALI_TEST_CK_PDN_MASK                       0x1
#define PMIC_CLK_EOSC_CALI_TEST_CK_PDN_SHIFT                      8
#define PMIC_CLK_FGADC_ANA_CK_PDN_ADDR                            MT6353_CLK_CKPDN_CON2
#define PMIC_CLK_FGADC_ANA_CK_PDN_MASK                            0x1
#define PMIC_CLK_FGADC_ANA_CK_PDN_SHIFT                           9
#define PMIC_CLK_FGADC_DIG_CK_PDN_ADDR                            MT6353_CLK_CKPDN_CON2
#define PMIC_CLK_FGADC_DIG_CK_PDN_MASK                            0x1
#define PMIC_CLK_FGADC_DIG_CK_PDN_SHIFT                           10
#define PMIC_CLK_FGADC_FT_CK_PDN_ADDR                             MT6353_CLK_CKPDN_CON2
#define PMIC_CLK_FGADC_FT_CK_PDN_MASK                             0x1
#define PMIC_CLK_FGADC_FT_CK_PDN_SHIFT                            11
#define PMIC_CLK_TRIM_75K_CK_PDN_ADDR                             MT6353_CLK_CKPDN_CON2
#define PMIC_CLK_TRIM_75K_CK_PDN_MASK                             0x1
#define PMIC_CLK_TRIM_75K_CK_PDN_SHIFT                            12
#define PMIC_CLK_EFUSE_CK_PDN_ADDR                                MT6353_CLK_CKPDN_CON2
#define PMIC_CLK_EFUSE_CK_PDN_MASK                                0x1
#define PMIC_CLK_EFUSE_CK_PDN_SHIFT                               13
#define PMIC_CLK_STRUP_75K_CK_PDN_ADDR                            MT6353_CLK_CKPDN_CON2
#define PMIC_CLK_STRUP_75K_CK_PDN_MASK                            0x1
#define PMIC_CLK_STRUP_75K_CK_PDN_SHIFT                           14
#define PMIC_CLK_STRUP_32K_CK_PDN_ADDR                            MT6353_CLK_CKPDN_CON2
#define PMIC_CLK_STRUP_32K_CK_PDN_MASK                            0x1
#define PMIC_CLK_STRUP_32K_CK_PDN_SHIFT                           15
#define PMIC_CLK_CKPDN_CON2_SET_ADDR                              MT6353_CLK_CKPDN_CON2_SET
#define PMIC_CLK_CKPDN_CON2_SET_MASK                              0xFFFF
#define PMIC_CLK_CKPDN_CON2_SET_SHIFT                             0
#define PMIC_CLK_CKPDN_CON2_CLR_ADDR                              MT6353_CLK_CKPDN_CON2_CLR
#define PMIC_CLK_CKPDN_CON2_CLR_MASK                              0xFFFF
#define PMIC_CLK_CKPDN_CON2_CLR_SHIFT                             0
#define PMIC_CLK_PCHR_32K_CK_PDN_ADDR                             MT6353_CLK_CKPDN_CON3
#define PMIC_CLK_PCHR_32K_CK_PDN_MASK                             0x1
#define PMIC_CLK_PCHR_32K_CK_PDN_SHIFT                            0
#define PMIC_CLK_PCHR_TEST_CK_PDN_ADDR                            MT6353_CLK_CKPDN_CON3
#define PMIC_CLK_PCHR_TEST_CK_PDN_MASK                            0x1
#define PMIC_CLK_PCHR_TEST_CK_PDN_SHIFT                           1
#define PMIC_CLK_SPI_CK_PDN_ADDR                                  MT6353_CLK_CKPDN_CON3
#define PMIC_CLK_SPI_CK_PDN_MASK                                  0x1
#define PMIC_CLK_SPI_CK_PDN_SHIFT                                 2
#define PMIC_CLK_BGR_TEST_CK_PDN_ADDR                             MT6353_CLK_CKPDN_CON3
#define PMIC_CLK_BGR_TEST_CK_PDN_MASK                             0x1
#define PMIC_CLK_BGR_TEST_CK_PDN_SHIFT                            3
#define PMIC_CLK_TYPE_C_CC_CK_PDN_ADDR                            MT6353_CLK_CKPDN_CON3
#define PMIC_CLK_TYPE_C_CC_CK_PDN_MASK                            0x1
#define PMIC_CLK_TYPE_C_CC_CK_PDN_SHIFT                           4
#define PMIC_CLK_TYPE_C_CSR_CK_PDN_ADDR                           MT6353_CLK_CKPDN_CON3
#define PMIC_CLK_TYPE_C_CSR_CK_PDN_MASK                           0x1
#define PMIC_CLK_TYPE_C_CSR_CK_PDN_SHIFT                          5
#define PMIC_CLK_SPK_CK_PDN_ADDR                                  MT6353_CLK_CKPDN_CON3
#define PMIC_CLK_SPK_CK_PDN_MASK                                  0x1
#define PMIC_CLK_SPK_CK_PDN_SHIFT                                 6
#define PMIC_CLK_SPK_PWM_CK_PDN_ADDR                              MT6353_CLK_CKPDN_CON3
#define PMIC_CLK_SPK_PWM_CK_PDN_MASK                              0x1
#define PMIC_CLK_SPK_PWM_CK_PDN_SHIFT                             7
#define PMIC_CLK_RSV_PDN_ADDR                                     MT6353_CLK_CKPDN_CON3
#define PMIC_CLK_RSV_PDN_MASK                                     0xFF
#define PMIC_CLK_RSV_PDN_SHIFT                                    8
#define PMIC_CLK_CKPDN_CON3_SET_ADDR                              MT6353_CLK_CKPDN_CON3_SET
#define PMIC_CLK_CKPDN_CON3_SET_MASK                              0xFFFF
#define PMIC_CLK_CKPDN_CON3_SET_SHIFT                             0
#define PMIC_CLK_CKPDN_CON3_CLR_ADDR                              MT6353_CLK_CKPDN_CON3_CLR
#define PMIC_CLK_CKPDN_CON3_CLR_MASK                              0xFFFF
#define PMIC_CLK_CKPDN_CON3_CLR_SHIFT                             0
#define PMIC_CLK_G_SMPS_PD_CK_PDN_HWEN_ADDR                       MT6353_CLK_CKPDN_HWEN_CON0
#define PMIC_CLK_G_SMPS_PD_CK_PDN_HWEN_MASK                       0x1
#define PMIC_CLK_G_SMPS_PD_CK_PDN_HWEN_SHIFT                      0
#define PMIC_CLK_G_SMPS_AUD_CK_PDN_HWEN_ADDR                      MT6353_CLK_CKPDN_HWEN_CON0
#define PMIC_CLK_G_SMPS_AUD_CK_PDN_HWEN_MASK                      0x1
#define PMIC_CLK_G_SMPS_AUD_CK_PDN_HWEN_SHIFT                     1
#define PMIC_CLK_AUXADC_SMPS_CK_PDN_HWEN_ADDR                     MT6353_CLK_CKPDN_HWEN_CON0
#define PMIC_CLK_AUXADC_SMPS_CK_PDN_HWEN_MASK                     0x1
#define PMIC_CLK_AUXADC_SMPS_CK_PDN_HWEN_SHIFT                    2
#define PMIC_CLK_AUXADC_26M_CK_PDN_HWEN_ADDR                      MT6353_CLK_CKPDN_HWEN_CON0
#define PMIC_CLK_AUXADC_26M_CK_PDN_HWEN_MASK                      0x1
#define PMIC_CLK_AUXADC_26M_CK_PDN_HWEN_SHIFT                     3
#define PMIC_CLK_AUXADC_CK_PDN_HWEN_ADDR                          MT6353_CLK_CKPDN_HWEN_CON0
#define PMIC_CLK_AUXADC_CK_PDN_HWEN_MASK                          0x1
#define PMIC_CLK_AUXADC_CK_PDN_HWEN_SHIFT                         4
#define PMIC_CLK_BUCK_1M_CK_PDN_HWEN_ADDR                         MT6353_CLK_CKPDN_HWEN_CON0
#define PMIC_CLK_BUCK_1M_CK_PDN_HWEN_MASK                         0x1
#define PMIC_CLK_BUCK_1M_CK_PDN_HWEN_SHIFT                        5
#define PMIC_CLK_BUCK_VPROC_9M_CK_PDN_HWEN_ADDR                   MT6353_CLK_CKPDN_HWEN_CON0
#define PMIC_CLK_BUCK_VPROC_9M_CK_PDN_HWEN_MASK                   0x1
#define PMIC_CLK_BUCK_VPROC_9M_CK_PDN_HWEN_SHIFT                  6
#define PMIC_CLK_BUCK_VCORE_9M_CK_PDN_HWEN_ADDR                   MT6353_CLK_CKPDN_HWEN_CON0
#define PMIC_CLK_BUCK_VCORE_9M_CK_PDN_HWEN_MASK                   0x1
#define PMIC_CLK_BUCK_VCORE_9M_CK_PDN_HWEN_SHIFT                  7
#define PMIC_CLK_BUCK_VCORE2_9M_CK_PDN_HWEN_ADDR                  MT6353_CLK_CKPDN_HWEN_CON0
#define PMIC_CLK_BUCK_VCORE2_9M_CK_PDN_HWEN_MASK                  0x1
#define PMIC_CLK_BUCK_VCORE2_9M_CK_PDN_HWEN_SHIFT                 8
#define PMIC_CLK_BUCK_VPA_9M_CK_PDN_HWEN_ADDR                     MT6353_CLK_CKPDN_HWEN_CON0
#define PMIC_CLK_BUCK_VPA_9M_CK_PDN_HWEN_MASK                     0x1
#define PMIC_CLK_BUCK_VPA_9M_CK_PDN_HWEN_SHIFT                    9
#define PMIC_CLK_BUCK_VS1_9M_CK_PDN_HWEN_ADDR                     MT6353_CLK_CKPDN_HWEN_CON0
#define PMIC_CLK_BUCK_VS1_9M_CK_PDN_HWEN_MASK                     0x1
#define PMIC_CLK_BUCK_VS1_9M_CK_PDN_HWEN_SHIFT                    10
#define PMIC_CLK_STB_1M_CK_PDN_HWEN_ADDR                          MT6353_CLK_CKPDN_HWEN_CON0
#define PMIC_CLK_STB_1M_CK_PDN_HWEN_MASK                          0x1
#define PMIC_CLK_STB_1M_CK_PDN_HWEN_SHIFT                         11
#define PMIC_CLK_LDO_VSRAM_PROC_9M_CK_PDN_HWEN_ADDR               MT6353_CLK_CKPDN_HWEN_CON0
#define PMIC_CLK_LDO_VSRAM_PROC_9M_CK_PDN_HWEN_MASK               0x1
#define PMIC_CLK_LDO_VSRAM_PROC_9M_CK_PDN_HWEN_SHIFT              12
#define PMIC_CLK_RTC_26M_CK_PDN_HWEN_ADDR                         MT6353_CLK_CKPDN_HWEN_CON0
#define PMIC_CLK_RTC_26M_CK_PDN_HWEN_MASK                         0x1
#define PMIC_CLK_RTC_26M_CK_PDN_HWEN_SHIFT                        13
#define PMIC_CLK_RSV_PDN_HWEN_ADDR                                MT6353_CLK_CKPDN_HWEN_CON0
#define PMIC_CLK_RSV_PDN_HWEN_MASK                                0x3
#define PMIC_CLK_RSV_PDN_HWEN_SHIFT                               14
#define PMIC_CLK_CKPDN_HWEN_CON0_SET_ADDR                         MT6353_CLK_CKPDN_HWEN_CON0_SET
#define PMIC_CLK_CKPDN_HWEN_CON0_SET_MASK                         0xFFFF
#define PMIC_CLK_CKPDN_HWEN_CON0_SET_SHIFT                        0
#define PMIC_CLK_CKPDN_HWEN_CON0_CLR_ADDR                         MT6353_CLK_CKPDN_HWEN_CON0_CLR
#define PMIC_CLK_CKPDN_HWEN_CON0_CLR_MASK                         0xFFFF
#define PMIC_CLK_CKPDN_HWEN_CON0_CLR_SHIFT                        0
#define PMIC_CLK_AUDIF_CK_CKSEL_ADDR                              MT6353_CLK_CKSEL_CON0
#define PMIC_CLK_AUDIF_CK_CKSEL_MASK                              0x1
#define PMIC_CLK_AUDIF_CK_CKSEL_SHIFT                             0
#define PMIC_CLK_AUD_CK_CKSEL_ADDR                                MT6353_CLK_CKSEL_CON0
#define PMIC_CLK_AUD_CK_CKSEL_MASK                                0x1
#define PMIC_CLK_AUD_CK_CKSEL_SHIFT                               1
#define PMIC_CLK_FQMTR_CK_CKSEL_ADDR                              MT6353_CLK_CKSEL_CON0
#define PMIC_CLK_FQMTR_CK_CKSEL_MASK                              0x7
#define PMIC_CLK_FQMTR_CK_CKSEL_SHIFT                             2
#define PMIC_CLK_FGADC_ANA_CK_CKSEL_ADDR                          MT6353_CLK_CKSEL_CON0
#define PMIC_CLK_FGADC_ANA_CK_CKSEL_MASK                          0x1
#define PMIC_CLK_FGADC_ANA_CK_CKSEL_SHIFT                         5
#define PMIC_CLK_PCHR_TEST_CK_CKSEL_ADDR                          MT6353_CLK_CKSEL_CON0
#define PMIC_CLK_PCHR_TEST_CK_CKSEL_MASK                          0x1
#define PMIC_CLK_PCHR_TEST_CK_CKSEL_SHIFT                         6
#define PMIC_CLK_BGR_TEST_CK_CKSEL_ADDR                           MT6353_CLK_CKSEL_CON0
#define PMIC_CLK_BGR_TEST_CK_CKSEL_MASK                           0x1
#define PMIC_CLK_BGR_TEST_CK_CKSEL_SHIFT                          7
#define PMIC_CLK_EFUSE_CK_PDN_HWEN_ADDR                           MT6353_CLK_CKSEL_CON0
#define PMIC_CLK_EFUSE_CK_PDN_HWEN_MASK                           0x1
#define PMIC_CLK_EFUSE_CK_PDN_HWEN_SHIFT                          8
#define PMIC_CLK_RSV_CKSEL_ADDR                                   MT6353_CLK_CKSEL_CON0
#define PMIC_CLK_RSV_CKSEL_MASK                                   0x7F
#define PMIC_CLK_RSV_CKSEL_SHIFT                                  9
#define PMIC_CLK_CKSEL_CON0_SET_ADDR                              MT6353_CLK_CKSEL_CON0_SET
#define PMIC_CLK_CKSEL_CON0_SET_MASK                              0xFFFF
#define PMIC_CLK_CKSEL_CON0_SET_SHIFT                             0
#define PMIC_CLK_CKSEL_CON0_CLR_ADDR                              MT6353_CLK_CKSEL_CON0_CLR
#define PMIC_CLK_CKSEL_CON0_CLR_MASK                              0xFFFF
#define PMIC_CLK_CKSEL_CON0_CLR_SHIFT                             0
#define PMIC_CLK_AUXADC_SMPS_CK_DIVSEL_ADDR                       MT6353_CLK_CKDIVSEL_CON0
#define PMIC_CLK_AUXADC_SMPS_CK_DIVSEL_MASK                       0x3
#define PMIC_CLK_AUXADC_SMPS_CK_DIVSEL_SHIFT                      0
#define PMIC_CLK_AUXADC_26M_CK_DIVSEL_ADDR                        MT6353_CLK_CKDIVSEL_CON0
#define PMIC_CLK_AUXADC_26M_CK_DIVSEL_MASK                        0x3
#define PMIC_CLK_AUXADC_26M_CK_DIVSEL_SHIFT                       2
#define PMIC_CLK_BUCK_9M_CK_DIVSEL_ADDR                           MT6353_CLK_CKDIVSEL_CON0
#define PMIC_CLK_BUCK_9M_CK_DIVSEL_MASK                           0x1
#define PMIC_CLK_BUCK_9M_CK_DIVSEL_SHIFT                          4
#define PMIC_CLK_REG_CK_DIVSEL_ADDR                               MT6353_CLK_CKDIVSEL_CON0
#define PMIC_CLK_REG_CK_DIVSEL_MASK                               0x3
#define PMIC_CLK_REG_CK_DIVSEL_SHIFT                              7
#define PMIC_CLK_SPK_CK_DIVSEL_ADDR                               MT6353_CLK_CKDIVSEL_CON0
#define PMIC_CLK_SPK_CK_DIVSEL_MASK                               0x3
#define PMIC_CLK_SPK_CK_DIVSEL_SHIFT                              9
#define PMIC_CLK_SPK_PWM_CK_DIVSEL_ADDR                           MT6353_CLK_CKDIVSEL_CON0
#define PMIC_CLK_SPK_PWM_CK_DIVSEL_MASK                           0x3
#define PMIC_CLK_SPK_PWM_CK_DIVSEL_SHIFT                          11
#define PMIC_CLK_RSV_DIVSEL_ADDR                                  MT6353_CLK_CKDIVSEL_CON0
#define PMIC_CLK_RSV_DIVSEL_MASK                                  0x7
#define PMIC_CLK_RSV_DIVSEL_SHIFT                                 13
#define PMIC_CLK_CKDIVSEL_CON0_SET_ADDR                           MT6353_CLK_CKDIVSEL_CON0_SET
#define PMIC_CLK_CKDIVSEL_CON0_SET_MASK                           0xFFFF
#define PMIC_CLK_CKDIVSEL_CON0_SET_SHIFT                          0
#define PMIC_CLK_CKDIVSEL_CON0_CLR_ADDR                           MT6353_CLK_CKDIVSEL_CON0_CLR
#define PMIC_CLK_CKDIVSEL_CON0_CLR_MASK                           0xFFFF
#define PMIC_CLK_CKDIVSEL_CON0_CLR_SHIFT                          0
#define PMIC_CLK_AUDIF_CK_TSTSEL_ADDR                             MT6353_CLK_CKTSTSEL_CON0
#define PMIC_CLK_AUDIF_CK_TSTSEL_MASK                             0x1
#define PMIC_CLK_AUDIF_CK_TSTSEL_SHIFT                            0
#define PMIC_CLK_AUD_CK_TSTSEL_ADDR                               MT6353_CLK_CKTSTSEL_CON0
#define PMIC_CLK_AUD_CK_TSTSEL_MASK                               0x1
#define PMIC_CLK_AUD_CK_TSTSEL_SHIFT                              1
#define PMIC_CLK_AUXADC_SMPS_CK_TSTSEL_ADDR                       MT6353_CLK_CKTSTSEL_CON0
#define PMIC_CLK_AUXADC_SMPS_CK_TSTSEL_MASK                       0x1
#define PMIC_CLK_AUXADC_SMPS_CK_TSTSEL_SHIFT                      2
#define PMIC_CLK_AUXADC_26M_CK_TSTSEL_ADDR                        MT6353_CLK_CKTSTSEL_CON0
#define PMIC_CLK_AUXADC_26M_CK_TSTSEL_MASK                        0x1
#define PMIC_CLK_AUXADC_26M_CK_TSTSEL_SHIFT                       3
#define PMIC_CLK_AUXADC_CK_TSTSEL_ADDR                            MT6353_CLK_CKTSTSEL_CON0
#define PMIC_CLK_AUXADC_CK_TSTSEL_MASK                            0x1
#define PMIC_CLK_AUXADC_CK_TSTSEL_SHIFT                           4
#define PMIC_CLK_DRV_CHRIND_CK_TSTSEL_ADDR                        MT6353_CLK_CKTSTSEL_CON0
#define PMIC_CLK_DRV_CHRIND_CK_TSTSEL_MASK                        0x1
#define PMIC_CLK_DRV_CHRIND_CK_TSTSEL_SHIFT                       5
#define PMIC_CLK_FQMTR_CK_TSTSEL_ADDR                             MT6353_CLK_CKTSTSEL_CON0
#define PMIC_CLK_FQMTR_CK_TSTSEL_MASK                             0x1
#define PMIC_CLK_FQMTR_CK_TSTSEL_SHIFT                            6
#define PMIC_CLK_RTCDET_CK_TSTSEL_ADDR                            MT6353_CLK_CKTSTSEL_CON0
#define PMIC_CLK_RTCDET_CK_TSTSEL_MASK                            0x1
#define PMIC_CLK_RTCDET_CK_TSTSEL_SHIFT                           7
#define PMIC_CLK_RTC_EOSC32_CK_TSTSEL_ADDR                        MT6353_CLK_CKTSTSEL_CON0
#define PMIC_CLK_RTC_EOSC32_CK_TSTSEL_MASK                        0x1
#define PMIC_CLK_RTC_EOSC32_CK_TSTSEL_SHIFT                       8
#define PMIC_CLK_EOSC_CALI_TEST_CK_TSTSEL_ADDR                    MT6353_CLK_CKTSTSEL_CON0
#define PMIC_CLK_EOSC_CALI_TEST_CK_TSTSEL_MASK                    0x1
#define PMIC_CLK_EOSC_CALI_TEST_CK_TSTSEL_SHIFT                   9
#define PMIC_CLK_FGADC_ANA_CK_TSTSEL_ADDR                         MT6353_CLK_CKTSTSEL_CON0
#define PMIC_CLK_FGADC_ANA_CK_TSTSEL_MASK                         0x1
#define PMIC_CLK_FGADC_ANA_CK_TSTSEL_SHIFT                        10
#define PMIC_CLK_PCHR_TEST_CK_TSTSEL_ADDR                         MT6353_CLK_CKTSTSEL_CON0
#define PMIC_CLK_PCHR_TEST_CK_TSTSEL_MASK                         0x1
#define PMIC_CLK_PCHR_TEST_CK_TSTSEL_SHIFT                        11
#define PMIC_CLK_BGR_TEST_CK_TSTSEL_ADDR                          MT6353_CLK_CKTSTSEL_CON0
#define PMIC_CLK_BGR_TEST_CK_TSTSEL_MASK                          0x1
#define PMIC_CLK_BGR_TEST_CK_TSTSEL_SHIFT                         12
#define PMIC_CLK_RSV_TSTSEL_ADDR                                  MT6353_CLK_CKTSTSEL_CON0
#define PMIC_CLK_RSV_TSTSEL_MASK                                  0x7
#define PMIC_CLK_RSV_TSTSEL_SHIFT                                 13
#define PMIC_RG_EFUSE_MAN_RST_ADDR                                MT6353_TOP_RST_CON0
#define PMIC_RG_EFUSE_MAN_RST_MASK                                0x1
#define PMIC_RG_EFUSE_MAN_RST_SHIFT                               0
#define PMIC_RG_AUXADC_RST_ADDR                                   MT6353_TOP_RST_CON0
#define PMIC_RG_AUXADC_RST_MASK                                   0x1
#define PMIC_RG_AUXADC_RST_SHIFT                                  1
#define PMIC_RG_AUXADC_REG_RST_ADDR                               MT6353_TOP_RST_CON0
#define PMIC_RG_AUXADC_REG_RST_MASK                               0x1
#define PMIC_RG_AUXADC_REG_RST_SHIFT                              2
#define PMIC_RG_AUDIO_RST_ADDR                                    MT6353_TOP_RST_CON0
#define PMIC_RG_AUDIO_RST_MASK                                    0x1
#define PMIC_RG_AUDIO_RST_SHIFT                                   3
#define PMIC_RG_ACCDET_RST_ADDR                                   MT6353_TOP_RST_CON0
#define PMIC_RG_ACCDET_RST_MASK                                   0x1
#define PMIC_RG_ACCDET_RST_SHIFT                                  4
#define PMIC_RG_BIF_RST_ADDR                                      MT6353_TOP_RST_CON0
#define PMIC_RG_BIF_RST_MASK                                      0x1
#define PMIC_RG_BIF_RST_SHIFT                                     5
#define PMIC_RG_DRIVER_RST_ADDR                                   MT6353_TOP_RST_CON0
#define PMIC_RG_DRIVER_RST_MASK                                   0x1
#define PMIC_RG_DRIVER_RST_SHIFT                                  6
#define PMIC_RG_FGADC_RST_ADDR                                    MT6353_TOP_RST_CON0
#define PMIC_RG_FGADC_RST_MASK                                    0x1
#define PMIC_RG_FGADC_RST_SHIFT                                   7
#define PMIC_RG_FQMTR_RST_ADDR                                    MT6353_TOP_RST_CON0
#define PMIC_RG_FQMTR_RST_MASK                                    0x1
#define PMIC_RG_FQMTR_RST_SHIFT                                   8
#define PMIC_RG_RTC_RST_ADDR                                      MT6353_TOP_RST_CON0
#define PMIC_RG_RTC_RST_MASK                                      0x1
#define PMIC_RG_RTC_RST_SHIFT                                     9
#define PMIC_RG_TYPE_C_CC_RST_ADDR                                MT6353_TOP_RST_CON0
#define PMIC_RG_TYPE_C_CC_RST_MASK                                0x1
#define PMIC_RG_TYPE_C_CC_RST_SHIFT                               10
#define PMIC_RG_CHRWDT_RST_ADDR                                   MT6353_TOP_RST_CON0
#define PMIC_RG_CHRWDT_RST_MASK                                   0x1
#define PMIC_RG_CHRWDT_RST_SHIFT                                  11
#define PMIC_RG_ZCD_RST_ADDR                                      MT6353_TOP_RST_CON0
#define PMIC_RG_ZCD_RST_MASK                                      0x1
#define PMIC_RG_ZCD_RST_SHIFT                                     12
#define PMIC_RG_AUDNCP_RST_ADDR                                   MT6353_TOP_RST_CON0
#define PMIC_RG_AUDNCP_RST_MASK                                   0x1
#define PMIC_RG_AUDNCP_RST_SHIFT                                  13
#define PMIC_RG_CLK_TRIM_RST_ADDR                                 MT6353_TOP_RST_CON0
#define PMIC_RG_CLK_TRIM_RST_MASK                                 0x1
#define PMIC_RG_CLK_TRIM_RST_SHIFT                                14
#define PMIC_RG_BUCK_SRCLKEN_RST_ADDR                             MT6353_TOP_RST_CON0
#define PMIC_RG_BUCK_SRCLKEN_RST_MASK                             0x1
#define PMIC_RG_BUCK_SRCLKEN_RST_SHIFT                            15
#define PMIC_TOP_RST_CON0_SET_ADDR                                MT6353_TOP_RST_CON0_SET
#define PMIC_TOP_RST_CON0_SET_MASK                                0xFFFF
#define PMIC_TOP_RST_CON0_SET_SHIFT                               0
#define PMIC_TOP_RST_CON0_CLR_ADDR                                MT6353_TOP_RST_CON0_CLR
#define PMIC_TOP_RST_CON0_CLR_MASK                                0xFFFF
#define PMIC_TOP_RST_CON0_CLR_SHIFT                               0
#define PMIC_RG_STRUP_LONG_PRESS_RST_ADDR                         MT6353_TOP_RST_CON1
#define PMIC_RG_STRUP_LONG_PRESS_RST_MASK                         0x1
#define PMIC_RG_STRUP_LONG_PRESS_RST_SHIFT                        0
#define PMIC_RG_BUCK_PROT_PMPP_RST_ADDR                           MT6353_TOP_RST_CON1
#define PMIC_RG_BUCK_PROT_PMPP_RST_MASK                           0x1
#define PMIC_RG_BUCK_PROT_PMPP_RST_SHIFT                          1
#define PMIC_RG_SPK_RST_ADDR                                      MT6353_TOP_RST_CON1
#define PMIC_RG_SPK_RST_MASK                                      0x1
#define PMIC_RG_SPK_RST_SHIFT                                     2
#define PMIC_TOP_RST_CON1_RSV_ADDR                                MT6353_TOP_RST_CON1
#define PMIC_TOP_RST_CON1_RSV_MASK                                0x1F
#define PMIC_TOP_RST_CON1_RSV_SHIFT                               3
#define PMIC_TOP_RST_CON1_SET_ADDR                                MT6353_TOP_RST_CON1_SET
#define PMIC_TOP_RST_CON1_SET_MASK                                0xFFFF
#define PMIC_TOP_RST_CON1_SET_SHIFT                               0
#define PMIC_TOP_RST_CON1_CLR_ADDR                                MT6353_TOP_RST_CON1_CLR
#define PMIC_TOP_RST_CON1_CLR_MASK                                0xFFFF
#define PMIC_TOP_RST_CON1_CLR_SHIFT                               0
#define PMIC_RG_CHR_LDO_DET_MODE_ADDR                             MT6353_TOP_RST_CON2
#define PMIC_RG_CHR_LDO_DET_MODE_MASK                             0x1
#define PMIC_RG_CHR_LDO_DET_MODE_SHIFT                            0
#define PMIC_RG_CHR_LDO_DET_SW_ADDR                               MT6353_TOP_RST_CON2
#define PMIC_RG_CHR_LDO_DET_SW_MASK                               0x1
#define PMIC_RG_CHR_LDO_DET_SW_SHIFT                              1
#define PMIC_RG_CHRWDT_FLAG_MODE_ADDR                             MT6353_TOP_RST_CON2
#define PMIC_RG_CHRWDT_FLAG_MODE_MASK                             0x1
#define PMIC_RG_CHRWDT_FLAG_MODE_SHIFT                            2
#define PMIC_RG_CHRWDT_FLAG_SW_ADDR                               MT6353_TOP_RST_CON2
#define PMIC_RG_CHRWDT_FLAG_SW_MASK                               0x1
#define PMIC_RG_CHRWDT_FLAG_SW_SHIFT                              3
#define PMIC_TOP_RST_CON2_RSV_ADDR                                MT6353_TOP_RST_CON2
#define PMIC_TOP_RST_CON2_RSV_MASK                                0xF
#define PMIC_TOP_RST_CON2_RSV_SHIFT                               4
#define PMIC_RG_WDTRSTB_EN_ADDR                                   MT6353_TOP_RST_MISC
#define PMIC_RG_WDTRSTB_EN_MASK                                   0x1
#define PMIC_RG_WDTRSTB_EN_SHIFT                                  0
#define PMIC_RG_WDTRSTB_MODE_ADDR                                 MT6353_TOP_RST_MISC
#define PMIC_RG_WDTRSTB_MODE_MASK                                 0x1
#define PMIC_RG_WDTRSTB_MODE_SHIFT                                1
#define PMIC_WDTRSTB_STATUS_ADDR                                  MT6353_TOP_RST_MISC
#define PMIC_WDTRSTB_STATUS_MASK                                  0x1
#define PMIC_WDTRSTB_STATUS_SHIFT                                 2
#define PMIC_WDTRSTB_STATUS_CLR_ADDR                              MT6353_TOP_RST_MISC
#define PMIC_WDTRSTB_STATUS_CLR_MASK                              0x1
#define PMIC_WDTRSTB_STATUS_CLR_SHIFT                             3
#define PMIC_RG_WDTRSTB_FB_EN_ADDR                                MT6353_TOP_RST_MISC
#define PMIC_RG_WDTRSTB_FB_EN_MASK                                0x1
#define PMIC_RG_WDTRSTB_FB_EN_SHIFT                               4
#define PMIC_RG_WDTRSTB_DEB_ADDR                                  MT6353_TOP_RST_MISC
#define PMIC_RG_WDTRSTB_DEB_MASK                                  0x1
#define PMIC_RG_WDTRSTB_DEB_SHIFT                                 5
#define PMIC_RG_HOMEKEY_RST_EN_ADDR                               MT6353_TOP_RST_MISC
#define PMIC_RG_HOMEKEY_RST_EN_MASK                               0x1
#define PMIC_RG_HOMEKEY_RST_EN_SHIFT                              8
#define PMIC_RG_PWRKEY_RST_EN_ADDR                                MT6353_TOP_RST_MISC
#define PMIC_RG_PWRKEY_RST_EN_MASK                                0x1
#define PMIC_RG_PWRKEY_RST_EN_SHIFT                               9
#define PMIC_RG_PWRRST_TMR_DIS_ADDR                               MT6353_TOP_RST_MISC
#define PMIC_RG_PWRRST_TMR_DIS_MASK                               0x1
#define PMIC_RG_PWRRST_TMR_DIS_SHIFT                              10
#define PMIC_RG_PWRKEY_RST_TD_ADDR                                MT6353_TOP_RST_MISC
#define PMIC_RG_PWRKEY_RST_TD_MASK                                0x3
#define PMIC_RG_PWRKEY_RST_TD_SHIFT                               12
#define PMIC_TOP_RST_MISC_RSV_ADDR                                MT6353_TOP_RST_MISC
#define PMIC_TOP_RST_MISC_RSV_MASK                                0x3
#define PMIC_TOP_RST_MISC_RSV_SHIFT                               14
#define PMIC_TOP_RST_MISC_SET_ADDR                                MT6353_TOP_RST_MISC_SET
#define PMIC_TOP_RST_MISC_SET_MASK                                0xFFFF
#define PMIC_TOP_RST_MISC_SET_SHIFT                               0
#define PMIC_TOP_RST_MISC_CLR_ADDR                                MT6353_TOP_RST_MISC_CLR
#define PMIC_TOP_RST_MISC_CLR_MASK                                0xFFFF
#define PMIC_TOP_RST_MISC_CLR_SHIFT                               0
#define PMIC_VPWRIN_RSTB_STATUS_ADDR                              MT6353_TOP_RST_STATUS
#define PMIC_VPWRIN_RSTB_STATUS_MASK                              0x1
#define PMIC_VPWRIN_RSTB_STATUS_SHIFT                             0
#define PMIC_DDLO_RSTB_STATUS_ADDR                                MT6353_TOP_RST_STATUS
#define PMIC_DDLO_RSTB_STATUS_MASK                                0x1
#define PMIC_DDLO_RSTB_STATUS_SHIFT                               1
#define PMIC_UVLO_RSTB_STATUS_ADDR                                MT6353_TOP_RST_STATUS
#define PMIC_UVLO_RSTB_STATUS_MASK                                0x1
#define PMIC_UVLO_RSTB_STATUS_SHIFT                               2
#define PMIC_RTC_DDLO_RSTB_STATUS_ADDR                            MT6353_TOP_RST_STATUS
#define PMIC_RTC_DDLO_RSTB_STATUS_MASK                            0x1
#define PMIC_RTC_DDLO_RSTB_STATUS_SHIFT                           3
#define PMIC_CHRWDT_REG_RSTB_STATUS_ADDR                          MT6353_TOP_RST_STATUS
#define PMIC_CHRWDT_REG_RSTB_STATUS_MASK                          0x1
#define PMIC_CHRWDT_REG_RSTB_STATUS_SHIFT                         4
#define PMIC_CHRDET_REG_RSTB_STATUS_ADDR                          MT6353_TOP_RST_STATUS
#define PMIC_CHRDET_REG_RSTB_STATUS_MASK                          0x1
#define PMIC_CHRDET_REG_RSTB_STATUS_SHIFT                         5
#define PMIC_TOP_RST_STATUS_RSV_ADDR                              MT6353_TOP_RST_STATUS
#define PMIC_TOP_RST_STATUS_RSV_MASK                              0x3
#define PMIC_TOP_RST_STATUS_RSV_SHIFT                             6
#define PMIC_TOP_RST_STATUS_SET_ADDR                              MT6353_TOP_RST_STATUS_SET
#define PMIC_TOP_RST_STATUS_SET_MASK                              0xFFFF
#define PMIC_TOP_RST_STATUS_SET_SHIFT                             0
#define PMIC_TOP_RST_STATUS_CLR_ADDR                              MT6353_TOP_RST_STATUS_CLR
#define PMIC_TOP_RST_STATUS_CLR_MASK                              0xFFFF
#define PMIC_TOP_RST_STATUS_CLR_SHIFT                             0
#define PMIC_TOP_RST_RSV_CON0_ADDR                                MT6353_TOP_RST_RSV_CON0
#define PMIC_TOP_RST_RSV_CON0_MASK                                0xFFFF
#define PMIC_TOP_RST_RSV_CON0_SHIFT                               0
#define PMIC_TOP_RST_RSV_CON1_ADDR                                MT6353_TOP_RST_RSV_CON1
#define PMIC_TOP_RST_RSV_CON1_MASK                                0xFFFF
#define PMIC_TOP_RST_RSV_CON1_SHIFT                               0
#define PMIC_RG_INT_EN_PWRKEY_ADDR                                MT6353_INT_CON0
#define PMIC_RG_INT_EN_PWRKEY_MASK                                0x1
#define PMIC_RG_INT_EN_PWRKEY_SHIFT                               0
#define PMIC_RG_INT_EN_HOMEKEY_ADDR                               MT6353_INT_CON0
#define PMIC_RG_INT_EN_HOMEKEY_MASK                               0x1
#define PMIC_RG_INT_EN_HOMEKEY_SHIFT                              1
#define PMIC_RG_INT_EN_PWRKEY_R_ADDR                              MT6353_INT_CON0
#define PMIC_RG_INT_EN_PWRKEY_R_MASK                              0x1
#define PMIC_RG_INT_EN_PWRKEY_R_SHIFT                             2
#define PMIC_RG_INT_EN_HOMEKEY_R_ADDR                             MT6353_INT_CON0
#define PMIC_RG_INT_EN_HOMEKEY_R_MASK                             0x1
#define PMIC_RG_INT_EN_HOMEKEY_R_SHIFT                            3
#define PMIC_RG_INT_EN_THR_H_ADDR                                 MT6353_INT_CON0
#define PMIC_RG_INT_EN_THR_H_MASK                                 0x1
#define PMIC_RG_INT_EN_THR_H_SHIFT                                4
#define PMIC_RG_INT_EN_THR_L_ADDR                                 MT6353_INT_CON0
#define PMIC_RG_INT_EN_THR_L_MASK                                 0x1
#define PMIC_RG_INT_EN_THR_L_SHIFT                                5
#define PMIC_RG_INT_EN_BAT_H_ADDR                                 MT6353_INT_CON0
#define PMIC_RG_INT_EN_BAT_H_MASK                                 0x1
#define PMIC_RG_INT_EN_BAT_H_SHIFT                                6
#define PMIC_RG_INT_EN_BAT_L_ADDR                                 MT6353_INT_CON0
#define PMIC_RG_INT_EN_BAT_L_MASK                                 0x1
#define PMIC_RG_INT_EN_BAT_L_SHIFT                                7
#define PMIC_RG_INT_EN_RTC_ADDR                                   MT6353_INT_CON0
#define PMIC_RG_INT_EN_RTC_MASK                                   0x1
#define PMIC_RG_INT_EN_RTC_SHIFT                                  9
#define PMIC_RG_INT_EN_AUDIO_ADDR                                 MT6353_INT_CON0
#define PMIC_RG_INT_EN_AUDIO_MASK                                 0x1
#define PMIC_RG_INT_EN_AUDIO_SHIFT                                10
#define PMIC_RG_INT_EN_ACCDET_ADDR                                MT6353_INT_CON0
#define PMIC_RG_INT_EN_ACCDET_MASK                                0x1
#define PMIC_RG_INT_EN_ACCDET_SHIFT                               12
#define PMIC_RG_INT_EN_ACCDET_EINT_ADDR                           MT6353_INT_CON0
#define PMIC_RG_INT_EN_ACCDET_EINT_MASK                           0x1
#define PMIC_RG_INT_EN_ACCDET_EINT_SHIFT                          13
#define PMIC_RG_INT_EN_ACCDET_NEGV_ADDR                           MT6353_INT_CON0
#define PMIC_RG_INT_EN_ACCDET_NEGV_MASK                           0x1
#define PMIC_RG_INT_EN_ACCDET_NEGV_SHIFT                          14
#define PMIC_RG_INT_EN_NI_LBAT_INT_ADDR                           MT6353_INT_CON0
#define PMIC_RG_INT_EN_NI_LBAT_INT_MASK                           0x1
#define PMIC_RG_INT_EN_NI_LBAT_INT_SHIFT                          15
#define PMIC_INT_CON0_SET_ADDR                                    MT6353_INT_CON0_SET
#define PMIC_INT_CON0_SET_MASK                                    0xFFFF
#define PMIC_INT_CON0_SET_SHIFT                                   0
#define PMIC_INT_CON0_CLR_ADDR                                    MT6353_INT_CON0_CLR
#define PMIC_INT_CON0_CLR_MASK                                    0xFFFF
#define PMIC_INT_CON0_CLR_SHIFT                                   0
#define PMIC_RG_INT_EN_VCORE_OC_ADDR                              MT6353_INT_CON1
#define PMIC_RG_INT_EN_VCORE_OC_MASK                              0x1
#define PMIC_RG_INT_EN_VCORE_OC_SHIFT                             0
#define PMIC_RG_INT_EN_VPROC_OC_ADDR                              MT6353_INT_CON1
#define PMIC_RG_INT_EN_VPROC_OC_MASK                              0x1
#define PMIC_RG_INT_EN_VPROC_OC_SHIFT                             1
#define PMIC_RG_INT_EN_VS1_OC_ADDR                                MT6353_INT_CON1
#define PMIC_RG_INT_EN_VS1_OC_MASK                                0x1
#define PMIC_RG_INT_EN_VS1_OC_SHIFT                               2
#define PMIC_RG_INT_EN_VPA_OC_ADDR                                MT6353_INT_CON1
#define PMIC_RG_INT_EN_VPA_OC_MASK                                0x1
#define PMIC_RG_INT_EN_VPA_OC_SHIFT                               3
#define PMIC_RG_INT_EN_SPKL_D_ADDR                                MT6353_INT_CON1
#define PMIC_RG_INT_EN_SPKL_D_MASK                                0x1
#define PMIC_RG_INT_EN_SPKL_D_SHIFT                               4
#define PMIC_RG_INT_EN_SPKL_AB_ADDR                               MT6353_INT_CON1
#define PMIC_RG_INT_EN_SPKL_AB_MASK                               0x1
#define PMIC_RG_INT_EN_SPKL_AB_SHIFT                              5
#define PMIC_RG_INT_EN_LDO_OC_ADDR                                MT6353_INT_CON1
#define PMIC_RG_INT_EN_LDO_OC_MASK                                0x1
#define PMIC_RG_INT_EN_LDO_OC_SHIFT                               15
#define PMIC_INT_CON1_SET_ADDR                                    MT6353_INT_CON1_SET
#define PMIC_INT_CON1_SET_MASK                                    0xFFFF
#define PMIC_INT_CON1_SET_SHIFT                                   0
#define PMIC_INT_CON1_CLR_ADDR                                    MT6353_INT_CON1_CLR
#define PMIC_INT_CON1_CLR_MASK                                    0xFFFF
#define PMIC_INT_CON1_CLR_SHIFT                                   0
#define PMIC_RG_INT_EN_TYPE_C_L_MIN_ADDR                          MT6353_INT_CON2
#define PMIC_RG_INT_EN_TYPE_C_L_MIN_MASK                          0x1
#define PMIC_RG_INT_EN_TYPE_C_L_MIN_SHIFT                         0
#define PMIC_RG_INT_EN_TYPE_C_L_MAX_ADDR                          MT6353_INT_CON2
#define PMIC_RG_INT_EN_TYPE_C_L_MAX_MASK                          0x1
#define PMIC_RG_INT_EN_TYPE_C_L_MAX_SHIFT                         1
#define PMIC_RG_INT_EN_TYPE_C_H_MIN_ADDR                          MT6353_INT_CON2
#define PMIC_RG_INT_EN_TYPE_C_H_MIN_MASK                          0x1
#define PMIC_RG_INT_EN_TYPE_C_H_MIN_SHIFT                         2
#define PMIC_RG_INT_EN_TYPE_C_H_MAX_ADDR                          MT6353_INT_CON2
#define PMIC_RG_INT_EN_TYPE_C_H_MAX_MASK                          0x1
#define PMIC_RG_INT_EN_TYPE_C_H_MAX_SHIFT                         3
#define PMIC_RG_INT_EN_AUXADC_IMP_ADDR                            MT6353_INT_CON2
#define PMIC_RG_INT_EN_AUXADC_IMP_MASK                            0x1
#define PMIC_RG_INT_EN_AUXADC_IMP_SHIFT                           4
#define PMIC_RG_INT_EN_NAG_C_DLTV_ADDR                            MT6353_INT_CON2
#define PMIC_RG_INT_EN_NAG_C_DLTV_MASK                            0x1
#define PMIC_RG_INT_EN_NAG_C_DLTV_SHIFT                           5
#define PMIC_RG_INT_EN_TYPE_C_CC_IRQ_ADDR                         MT6353_INT_CON2
#define PMIC_RG_INT_EN_TYPE_C_CC_IRQ_MASK                         0x1
#define PMIC_RG_INT_EN_TYPE_C_CC_IRQ_SHIFT                        6
#define PMIC_RG_INT_EN_CHRDET_EDGE_ADDR                           MT6353_INT_CON2
#define PMIC_RG_INT_EN_CHRDET_EDGE_MASK                           0x1
#define PMIC_RG_INT_EN_CHRDET_EDGE_SHIFT                          7
#define PMIC_RG_INT_EN_OV_ADDR                                    MT6353_INT_CON2
#define PMIC_RG_INT_EN_OV_MASK                                    0x1
#define PMIC_RG_INT_EN_OV_SHIFT                                   8
#define PMIC_RG_INT_EN_BVALID_DET_ADDR                            MT6353_INT_CON2
#define PMIC_RG_INT_EN_BVALID_DET_MASK                            0x1
#define PMIC_RG_INT_EN_BVALID_DET_SHIFT                           9
#define PMIC_RG_INT_EN_RGS_BATON_HV_ADDR                          MT6353_INT_CON2
#define PMIC_RG_INT_EN_RGS_BATON_HV_MASK                          0x1
#define PMIC_RG_INT_EN_RGS_BATON_HV_SHIFT                         10
#define PMIC_RG_INT_EN_VBATON_UNDET_ADDR                          MT6353_INT_CON2
#define PMIC_RG_INT_EN_VBATON_UNDET_MASK                          0x1
#define PMIC_RG_INT_EN_VBATON_UNDET_SHIFT                         11
#define PMIC_RG_INT_EN_WATCHDOG_ADDR                              MT6353_INT_CON2
#define PMIC_RG_INT_EN_WATCHDOG_MASK                              0x1
#define PMIC_RG_INT_EN_WATCHDOG_SHIFT                             12
#define PMIC_RG_INT_EN_PCHR_CM_VDEC_ADDR                          MT6353_INT_CON2
#define PMIC_RG_INT_EN_PCHR_CM_VDEC_MASK                          0x1
#define PMIC_RG_INT_EN_PCHR_CM_VDEC_SHIFT                         13
#define PMIC_RG_INT_EN_CHRDET_ADDR                                MT6353_INT_CON2
#define PMIC_RG_INT_EN_CHRDET_MASK                                0x1
#define PMIC_RG_INT_EN_CHRDET_SHIFT                               14
#define PMIC_RG_INT_EN_PCHR_CM_VINC_ADDR                          MT6353_INT_CON2
#define PMIC_RG_INT_EN_PCHR_CM_VINC_MASK                          0x1
#define PMIC_RG_INT_EN_PCHR_CM_VINC_SHIFT                         15
#define PMIC_INT_CON2_SET_ADDR                                    MT6353_INT_CON2_SET
#define PMIC_INT_CON2_SET_MASK                                    0xFFFF
#define PMIC_INT_CON2_SET_SHIFT                                   0
#define PMIC_INT_CON2_CLR_ADDR                                    MT6353_INT_CON2_CLR
#define PMIC_INT_CON2_CLR_MASK                                    0xFFFF
#define PMIC_INT_CON2_CLR_SHIFT                                   0
#define PMIC_RG_INT_EN_FG_BAT_H_ADDR                              MT6353_INT_CON3
#define PMIC_RG_INT_EN_FG_BAT_H_MASK                              0x1
#define PMIC_RG_INT_EN_FG_BAT_H_SHIFT                             0
#define PMIC_RG_INT_EN_FG_BAT_L_ADDR                              MT6353_INT_CON3
#define PMIC_RG_INT_EN_FG_BAT_L_MASK                              0x1
#define PMIC_RG_INT_EN_FG_BAT_L_SHIFT                             1
#define PMIC_RG_INT_EN_FG_CUR_H_ADDR                              MT6353_INT_CON3
#define PMIC_RG_INT_EN_FG_CUR_H_MASK                              0x1
#define PMIC_RG_INT_EN_FG_CUR_H_SHIFT                             2
#define PMIC_RG_INT_EN_FG_CUR_L_ADDR                              MT6353_INT_CON3
#define PMIC_RG_INT_EN_FG_CUR_L_MASK                              0x1
#define PMIC_RG_INT_EN_FG_CUR_L_SHIFT                             3
#define PMIC_RG_INT_EN_FG_ZCV_ADDR                                MT6353_INT_CON3
#define PMIC_RG_INT_EN_FG_ZCV_MASK                                0x1
#define PMIC_RG_INT_EN_FG_ZCV_SHIFT                               4
#define PMIC_INT_CON3_SET_ADDR                                    MT6353_INT_CON3_SET
#define PMIC_INT_CON3_SET_MASK                                    0xFFFF
#define PMIC_INT_CON3_SET_SHIFT                                   0
#define PMIC_INT_CON3_CLR_ADDR                                    MT6353_INT_CON3_CLR
#define PMIC_INT_CON3_CLR_MASK                                    0xFFFF
#define PMIC_INT_CON3_CLR_SHIFT                                   0
#define PMIC_POLARITY_ADDR                                        MT6353_INT_MISC_CON
#define PMIC_POLARITY_MASK                                        0x1
#define PMIC_POLARITY_SHIFT                                       0
#define PMIC_RG_HOMEKEY_INT_SEL_ADDR                              MT6353_INT_MISC_CON
#define PMIC_RG_HOMEKEY_INT_SEL_MASK                              0x1
#define PMIC_RG_HOMEKEY_INT_SEL_SHIFT                             1
#define PMIC_RG_PWRKEY_INT_SEL_ADDR                               MT6353_INT_MISC_CON
#define PMIC_RG_PWRKEY_INT_SEL_MASK                               0x1
#define PMIC_RG_PWRKEY_INT_SEL_SHIFT                              2
#define PMIC_RG_CHRDET_INT_SEL_ADDR                               MT6353_INT_MISC_CON
#define PMIC_RG_CHRDET_INT_SEL_MASK                               0x1
#define PMIC_RG_CHRDET_INT_SEL_SHIFT                              3
#define PMIC_RG_PCHR_CM_VINC_POLARITY_RSV_ADDR                    MT6353_INT_MISC_CON
#define PMIC_RG_PCHR_CM_VINC_POLARITY_RSV_MASK                    0x1
#define PMIC_RG_PCHR_CM_VINC_POLARITY_RSV_SHIFT                   4
#define PMIC_RG_PCHR_CM_VDEC_POLARITY_RSV_ADDR                    MT6353_INT_MISC_CON
#define PMIC_RG_PCHR_CM_VDEC_POLARITY_RSV_MASK                    0x1
#define PMIC_RG_PCHR_CM_VDEC_POLARITY_RSV_SHIFT                   5
#define PMIC_INT_MISC_CON_SET_ADDR                                MT6353_INT_MISC_CON_SET
#define PMIC_INT_MISC_CON_SET_MASK                                0xFFFF
#define PMIC_INT_MISC_CON_SET_SHIFT                               0
#define PMIC_INT_MISC_CON_CLR_ADDR                                MT6353_INT_MISC_CON_CLR
#define PMIC_INT_MISC_CON_CLR_MASK                                0xFFFF
#define PMIC_INT_MISC_CON_CLR_SHIFT                               0
#define PMIC_RG_INT_STATUS_PWRKEY_ADDR                            MT6353_INT_STATUS0
#define PMIC_RG_INT_STATUS_PWRKEY_MASK                            0x1
#define PMIC_RG_INT_STATUS_PWRKEY_SHIFT                           0
#define PMIC_RG_INT_STATUS_HOMEKEY_ADDR                           MT6353_INT_STATUS0
#define PMIC_RG_INT_STATUS_HOMEKEY_MASK                           0x1
#define PMIC_RG_INT_STATUS_HOMEKEY_SHIFT                          1
#define PMIC_RG_INT_STATUS_PWRKEY_R_ADDR                          MT6353_INT_STATUS0
#define PMIC_RG_INT_STATUS_PWRKEY_R_MASK                          0x1
#define PMIC_RG_INT_STATUS_PWRKEY_R_SHIFT                         2
#define PMIC_RG_INT_STATUS_HOMEKEY_R_ADDR                         MT6353_INT_STATUS0
#define PMIC_RG_INT_STATUS_HOMEKEY_R_MASK                         0x1
#define PMIC_RG_INT_STATUS_HOMEKEY_R_SHIFT                        3
#define PMIC_RG_INT_STATUS_THR_H_ADDR                             MT6353_INT_STATUS0
#define PMIC_RG_INT_STATUS_THR_H_MASK                             0x1
#define PMIC_RG_INT_STATUS_THR_H_SHIFT                            4
#define PMIC_RG_INT_STATUS_THR_L_ADDR                             MT6353_INT_STATUS0
#define PMIC_RG_INT_STATUS_THR_L_MASK                             0x1
#define PMIC_RG_INT_STATUS_THR_L_SHIFT                            5
#define PMIC_RG_INT_STATUS_BAT_H_ADDR                             MT6353_INT_STATUS0
#define PMIC_RG_INT_STATUS_BAT_H_MASK                             0x1
#define PMIC_RG_INT_STATUS_BAT_H_SHIFT                            6
#define PMIC_RG_INT_STATUS_BAT_L_ADDR                             MT6353_INT_STATUS0
#define PMIC_RG_INT_STATUS_BAT_L_MASK                             0x1
#define PMIC_RG_INT_STATUS_BAT_L_SHIFT                            7
#define PMIC_RG_INT_STATUS_RTC_ADDR                               MT6353_INT_STATUS0
#define PMIC_RG_INT_STATUS_RTC_MASK                               0x1
#define PMIC_RG_INT_STATUS_RTC_SHIFT                              9
#define PMIC_RG_INT_STATUS_AUDIO_ADDR                             MT6353_INT_STATUS0
#define PMIC_RG_INT_STATUS_AUDIO_MASK                             0x1
#define PMIC_RG_INT_STATUS_AUDIO_SHIFT                            10
#define PMIC_RG_INT_STATUS_ACCDET_ADDR                            MT6353_INT_STATUS0
#define PMIC_RG_INT_STATUS_ACCDET_MASK                            0x1
#define PMIC_RG_INT_STATUS_ACCDET_SHIFT                           12
#define PMIC_RG_INT_STATUS_ACCDET_EINT_ADDR                       MT6353_INT_STATUS0
#define PMIC_RG_INT_STATUS_ACCDET_EINT_MASK                       0x1
#define PMIC_RG_INT_STATUS_ACCDET_EINT_SHIFT                      13
#define PMIC_RG_INT_STATUS_ACCDET_NEGV_ADDR                       MT6353_INT_STATUS0
#define PMIC_RG_INT_STATUS_ACCDET_NEGV_MASK                       0x1
#define PMIC_RG_INT_STATUS_ACCDET_NEGV_SHIFT                      14
#define PMIC_RG_INT_STATUS_NI_LBAT_INT_ADDR                       MT6353_INT_STATUS0
#define PMIC_RG_INT_STATUS_NI_LBAT_INT_MASK                       0x1
#define PMIC_RG_INT_STATUS_NI_LBAT_INT_SHIFT                      15
#define PMIC_RG_INT_STATUS_VCORE_OC_ADDR                          MT6353_INT_STATUS1
#define PMIC_RG_INT_STATUS_VCORE_OC_MASK                          0x1
#define PMIC_RG_INT_STATUS_VCORE_OC_SHIFT                         0
#define PMIC_RG_INT_STATUS_VPROC_OC_ADDR                          MT6353_INT_STATUS1
#define PMIC_RG_INT_STATUS_VPROC_OC_MASK                          0x1
#define PMIC_RG_INT_STATUS_VPROC_OC_SHIFT                         1
#define PMIC_RG_INT_STATUS_VS1_OC_ADDR                            MT6353_INT_STATUS1
#define PMIC_RG_INT_STATUS_VS1_OC_MASK                            0x1
#define PMIC_RG_INT_STATUS_VS1_OC_SHIFT                           2
#define PMIC_RG_INT_STATUS_VPA_OC_ADDR                            MT6353_INT_STATUS1
#define PMIC_RG_INT_STATUS_VPA_OC_MASK                            0x1
#define PMIC_RG_INT_STATUS_VPA_OC_SHIFT                           3
#define PMIC_RG_INT_STATUS_SPKL_D_ADDR                            MT6353_INT_STATUS1
#define PMIC_RG_INT_STATUS_SPKL_D_MASK                            0x1
#define PMIC_RG_INT_STATUS_SPKL_D_SHIFT                           4
#define PMIC_RG_INT_STATUS_SPKL_AB_ADDR                           MT6353_INT_STATUS1
#define PMIC_RG_INT_STATUS_SPKL_AB_MASK                           0x1
#define PMIC_RG_INT_STATUS_SPKL_AB_SHIFT                          5
#define PMIC_RG_INT_STATUS_LDO_OC_ADDR                            MT6353_INT_STATUS1
#define PMIC_RG_INT_STATUS_LDO_OC_MASK                            0x1
#define PMIC_RG_INT_STATUS_LDO_OC_SHIFT                           15
#define PMIC_RG_INT_STATUS_TYPE_C_L_MIN_ADDR                      MT6353_INT_STATUS2
#define PMIC_RG_INT_STATUS_TYPE_C_L_MIN_MASK                      0x1
#define PMIC_RG_INT_STATUS_TYPE_C_L_MIN_SHIFT                     0
#define PMIC_RG_INT_STATUS_TYPE_C_L_MAX_ADDR                      MT6353_INT_STATUS2
#define PMIC_RG_INT_STATUS_TYPE_C_L_MAX_MASK                      0x1
#define PMIC_RG_INT_STATUS_TYPE_C_L_MAX_SHIFT                     1
#define PMIC_RG_INT_STATUS_TYPE_C_H_MIN_ADDR                      MT6353_INT_STATUS2
#define PMIC_RG_INT_STATUS_TYPE_C_H_MIN_MASK                      0x1
#define PMIC_RG_INT_STATUS_TYPE_C_H_MIN_SHIFT                     2
#define PMIC_RG_INT_STATUS_TYPE_C_H_MAX_ADDR                      MT6353_INT_STATUS2
#define PMIC_RG_INT_STATUS_TYPE_C_H_MAX_MASK                      0x1
#define PMIC_RG_INT_STATUS_TYPE_C_H_MAX_SHIFT                     3
#define PMIC_RG_INT_STATUS_AUXADC_IMP_ADDR                        MT6353_INT_STATUS2
#define PMIC_RG_INT_STATUS_AUXADC_IMP_MASK                        0x1
#define PMIC_RG_INT_STATUS_AUXADC_IMP_SHIFT                       4
#define PMIC_RG_INT_STATUS_NAG_C_DLTV_ADDR                        MT6353_INT_STATUS2
#define PMIC_RG_INT_STATUS_NAG_C_DLTV_MASK                        0x1
#define PMIC_RG_INT_STATUS_NAG_C_DLTV_SHIFT                       5
#define PMIC_RG_INT_STATUS_TYPE_C_CC_IRQ_ADDR                     MT6353_INT_STATUS2
#define PMIC_RG_INT_STATUS_TYPE_C_CC_IRQ_MASK                     0x1
#define PMIC_RG_INT_STATUS_TYPE_C_CC_IRQ_SHIFT                    6
#define PMIC_RG_INT_STATUS_CHRDET_EDGE_ADDR                       MT6353_INT_STATUS2
#define PMIC_RG_INT_STATUS_CHRDET_EDGE_MASK                       0x1
#define PMIC_RG_INT_STATUS_CHRDET_EDGE_SHIFT                      7
#define PMIC_RG_INT_STATUS_OV_ADDR                                MT6353_INT_STATUS2
#define PMIC_RG_INT_STATUS_OV_MASK                                0x1
#define PMIC_RG_INT_STATUS_OV_SHIFT                               8
#define PMIC_RG_INT_STATUS_BVALID_DET_ADDR                        MT6353_INT_STATUS2
#define PMIC_RG_INT_STATUS_BVALID_DET_MASK                        0x1
#define PMIC_RG_INT_STATUS_BVALID_DET_SHIFT                       9
#define PMIC_RG_INT_STATUS_RGS_BATON_HV_ADDR                      MT6353_INT_STATUS2
#define PMIC_RG_INT_STATUS_RGS_BATON_HV_MASK                      0x1
#define PMIC_RG_INT_STATUS_RGS_BATON_HV_SHIFT                     10
#define PMIC_RG_INT_STATUS_VBATON_UNDET_ADDR                      MT6353_INT_STATUS2
#define PMIC_RG_INT_STATUS_VBATON_UNDET_MASK                      0x1
#define PMIC_RG_INT_STATUS_VBATON_UNDET_SHIFT                     11
#define PMIC_RG_INT_STATUS_WATCHDOG_ADDR                          MT6353_INT_STATUS2
#define PMIC_RG_INT_STATUS_WATCHDOG_MASK                          0x1
#define PMIC_RG_INT_STATUS_WATCHDOG_SHIFT                         12
#define PMIC_RG_INT_STATUS_PCHR_CM_VDEC_ADDR                      MT6353_INT_STATUS2
#define PMIC_RG_INT_STATUS_PCHR_CM_VDEC_MASK                      0x1
#define PMIC_RG_INT_STATUS_PCHR_CM_VDEC_SHIFT                     13
#define PMIC_RG_INT_STATUS_CHRDET_ADDR                            MT6353_INT_STATUS2
#define PMIC_RG_INT_STATUS_CHRDET_MASK                            0x1
#define PMIC_RG_INT_STATUS_CHRDET_SHIFT                           14
#define PMIC_RG_INT_STATUS_PCHR_CM_VINC_ADDR                      MT6353_INT_STATUS2
#define PMIC_RG_INT_STATUS_PCHR_CM_VINC_MASK                      0x1
#define PMIC_RG_INT_STATUS_PCHR_CM_VINC_SHIFT                     15
#define PMIC_RG_INT_STATUS_FG_BAT_H_ADDR                          MT6353_INT_STATUS3
#define PMIC_RG_INT_STATUS_FG_BAT_H_MASK                          0x1
#define PMIC_RG_INT_STATUS_FG_BAT_H_SHIFT                         0
#define PMIC_RG_INT_STATUS_FG_BAT_L_ADDR                          MT6353_INT_STATUS3
#define PMIC_RG_INT_STATUS_FG_BAT_L_MASK                          0x1
#define PMIC_RG_INT_STATUS_FG_BAT_L_SHIFT                         1
#define PMIC_RG_INT_STATUS_FG_CUR_H_ADDR                          MT6353_INT_STATUS3
#define PMIC_RG_INT_STATUS_FG_CUR_H_MASK                          0x1
#define PMIC_RG_INT_STATUS_FG_CUR_H_SHIFT                         2
#define PMIC_RG_INT_STATUS_FG_CUR_L_ADDR                          MT6353_INT_STATUS3
#define PMIC_RG_INT_STATUS_FG_CUR_L_MASK                          0x1
#define PMIC_RG_INT_STATUS_FG_CUR_L_SHIFT                         3
#define PMIC_RG_INT_STATUS_FG_ZCV_ADDR                            MT6353_INT_STATUS3
#define PMIC_RG_INT_STATUS_FG_ZCV_MASK                            0x1
#define PMIC_RG_INT_STATUS_FG_ZCV_SHIFT                           4
#define PMIC_OC_GEAR_LDO_ADDR                                     MT6353_OC_GEAR_0
#define PMIC_OC_GEAR_LDO_MASK                                     0x3
#define PMIC_OC_GEAR_LDO_SHIFT                                    0
#define PMIC_SPK_EN_L_ADDR                                        MT6353_SPK_CON0
#define PMIC_SPK_EN_L_MASK                                        0x1
#define PMIC_SPK_EN_L_SHIFT                                       0
#define PMIC_SPKMODE_L_ADDR                                       MT6353_SPK_CON0
#define PMIC_SPKMODE_L_MASK                                       0x1
#define PMIC_SPKMODE_L_SHIFT                                      2
#define PMIC_SPK_TRIM_EN_L_ADDR                                   MT6353_SPK_CON0
#define PMIC_SPK_TRIM_EN_L_MASK                                   0x1
#define PMIC_SPK_TRIM_EN_L_SHIFT                                  3
#define PMIC_SPK_OC_SHDN_DL_ADDR                                  MT6353_SPK_CON0
#define PMIC_SPK_OC_SHDN_DL_MASK                                  0x1
#define PMIC_SPK_OC_SHDN_DL_SHIFT                                 8
#define PMIC_SPK_THER_SHDN_L_EN_ADDR                              MT6353_SPK_CON0
#define PMIC_SPK_THER_SHDN_L_EN_MASK                              0x1
#define PMIC_SPK_THER_SHDN_L_EN_SHIFT                             9
#define PMIC_SPK_OUT_STAGE_SEL_ADDR                               MT6353_SPK_CON0
#define PMIC_SPK_OUT_STAGE_SEL_MASK                               0x1
#define PMIC_SPK_OUT_STAGE_SEL_SHIFT                              10
#define PMIC_RG_SPK_GAINL_ADDR                                    MT6353_SPK_CON0
#define PMIC_RG_SPK_GAINL_MASK                                    0x3
#define PMIC_RG_SPK_GAINL_SHIFT                                   12
#define PMIC_DA_SPK_OFFSET_L_ADDR                                 MT6353_SPK_CON1
#define PMIC_DA_SPK_OFFSET_L_MASK                                 0x1F
#define PMIC_DA_SPK_OFFSET_L_SHIFT                                0
#define PMIC_DA_SPK_LEAD_DGLH_L_ADDR                              MT6353_SPK_CON1
#define PMIC_DA_SPK_LEAD_DGLH_L_MASK                              0x1
#define PMIC_DA_SPK_LEAD_DGLH_L_SHIFT                             5
#define PMIC_AD_NI_SPK_LEAD_L_ADDR                                MT6353_SPK_CON1
#define PMIC_AD_NI_SPK_LEAD_L_MASK                                0x1
#define PMIC_AD_NI_SPK_LEAD_L_SHIFT                               6
#define PMIC_SPK_OFFSET_L_OV_ADDR                                 MT6353_SPK_CON1
#define PMIC_SPK_OFFSET_L_OV_MASK                                 0x1
#define PMIC_SPK_OFFSET_L_OV_SHIFT                                7
#define PMIC_SPK_OFFSET_L_SW_ADDR                                 MT6353_SPK_CON1
#define PMIC_SPK_OFFSET_L_SW_MASK                                 0x1F
#define PMIC_SPK_OFFSET_L_SW_SHIFT                                8
#define PMIC_SPK_LEAD_L_SW_ADDR                                   MT6353_SPK_CON1
#define PMIC_SPK_LEAD_L_SW_MASK                                   0x1
#define PMIC_SPK_LEAD_L_SW_SHIFT                                  13
#define PMIC_SPK_OFFSET_L_MODE_ADDR                               MT6353_SPK_CON1
#define PMIC_SPK_OFFSET_L_MODE_MASK                               0x1
#define PMIC_SPK_OFFSET_L_MODE_SHIFT                              14
#define PMIC_SPK_TRIM_DONE_L_ADDR                                 MT6353_SPK_CON1
#define PMIC_SPK_TRIM_DONE_L_MASK                                 0x1
#define PMIC_SPK_TRIM_DONE_L_SHIFT                                15
#define PMIC_RG_SPK_INTG_RST_L_ADDR                               MT6353_SPK_CON2
#define PMIC_RG_SPK_INTG_RST_L_MASK                               0x1
#define PMIC_RG_SPK_INTG_RST_L_SHIFT                              0
#define PMIC_RG_SPK_FORCE_EN_L_ADDR                               MT6353_SPK_CON2
#define PMIC_RG_SPK_FORCE_EN_L_MASK                               0x1
#define PMIC_RG_SPK_FORCE_EN_L_SHIFT                              1
#define PMIC_RG_SPK_SLEW_L_ADDR                                   MT6353_SPK_CON2
#define PMIC_RG_SPK_SLEW_L_MASK                                   0x3
#define PMIC_RG_SPK_SLEW_L_SHIFT                                  2
#define PMIC_RG_SPKAB_OBIAS_L_ADDR                                MT6353_SPK_CON2
#define PMIC_RG_SPKAB_OBIAS_L_MASK                                0x3
#define PMIC_RG_SPKAB_OBIAS_L_SHIFT                               4
#define PMIC_RG_SPKRCV_EN_L_ADDR                                  MT6353_SPK_CON2
#define PMIC_RG_SPKRCV_EN_L_MASK                                  0x1
#define PMIC_RG_SPKRCV_EN_L_SHIFT                                 6
#define PMIC_RG_SPK_DRC_EN_L_ADDR                                 MT6353_SPK_CON2
#define PMIC_RG_SPK_DRC_EN_L_MASK                                 0x1
#define PMIC_RG_SPK_DRC_EN_L_SHIFT                                7
#define PMIC_RG_SPK_TEST_EN_L_ADDR                                MT6353_SPK_CON2
#define PMIC_RG_SPK_TEST_EN_L_MASK                                0x1
#define PMIC_RG_SPK_TEST_EN_L_SHIFT                               8
#define PMIC_RG_SPKAB_OC_EN_L_ADDR                                MT6353_SPK_CON2
#define PMIC_RG_SPKAB_OC_EN_L_MASK                                0x1
#define PMIC_RG_SPKAB_OC_EN_L_SHIFT                               9
#define PMIC_RG_SPK_OC_EN_L_ADDR                                  MT6353_SPK_CON2
#define PMIC_RG_SPK_OC_EN_L_MASK                                  0x1
#define PMIC_RG_SPK_OC_EN_L_SHIFT                                 10
#define PMIC_SPK_EN_R_ADDR                                        MT6353_SPK_CON3
#define PMIC_SPK_EN_R_MASK                                        0x1
#define PMIC_SPK_EN_R_SHIFT                                       0
#define PMIC_SPKMODE_R_ADDR                                       MT6353_SPK_CON3
#define PMIC_SPKMODE_R_MASK                                       0x1
#define PMIC_SPKMODE_R_SHIFT                                      2
#define PMIC_SPK_TRIM_EN_R_ADDR                                   MT6353_SPK_CON3
#define PMIC_SPK_TRIM_EN_R_MASK                                   0x1
#define PMIC_SPK_TRIM_EN_R_SHIFT                                  3
#define PMIC_SPK_OC_SHDN_DR_ADDR                                  MT6353_SPK_CON3
#define PMIC_SPK_OC_SHDN_DR_MASK                                  0x1
#define PMIC_SPK_OC_SHDN_DR_SHIFT                                 8
#define PMIC_SPK_THER_SHDN_R_EN_ADDR                              MT6353_SPK_CON3
#define PMIC_SPK_THER_SHDN_R_EN_MASK                              0x1
#define PMIC_SPK_THER_SHDN_R_EN_SHIFT                             9
#define PMIC_RG_SPK_GAINR_ADDR                                    MT6353_SPK_CON3
#define PMIC_RG_SPK_GAINR_MASK                                    0x3
#define PMIC_RG_SPK_GAINR_SHIFT                                   12
#define PMIC_DA_SPK_OFFSET_R_ADDR                                 MT6353_SPK_CON4
#define PMIC_DA_SPK_OFFSET_R_MASK                                 0x1F
#define PMIC_DA_SPK_OFFSET_R_SHIFT                                0
#define PMIC_DA_SPK_LEAD_DGLH_R_ADDR                              MT6353_SPK_CON4
#define PMIC_DA_SPK_LEAD_DGLH_R_MASK                              0x1
#define PMIC_DA_SPK_LEAD_DGLH_R_SHIFT                             5
#define PMIC_NI_SPK_LEAD_R_ADDR                                   MT6353_SPK_CON4
#define PMIC_NI_SPK_LEAD_R_MASK                                   0x1
#define PMIC_NI_SPK_LEAD_R_SHIFT                                  6
#define PMIC_SPK_OFFSET_R_OV_ADDR                                 MT6353_SPK_CON4
#define PMIC_SPK_OFFSET_R_OV_MASK                                 0x1
#define PMIC_SPK_OFFSET_R_OV_SHIFT                                7
#define PMIC_SPK_OFFSET_R_SW_ADDR                                 MT6353_SPK_CON4
#define PMIC_SPK_OFFSET_R_SW_MASK                                 0x1F
#define PMIC_SPK_OFFSET_R_SW_SHIFT                                8
#define PMIC_SPK_LEAD_R_SW_ADDR                                   MT6353_SPK_CON4
#define PMIC_SPK_LEAD_R_SW_MASK                                   0x1
#define PMIC_SPK_LEAD_R_SW_SHIFT                                  13
#define PMIC_SPK_OFFSET_R_MODE_ADDR                               MT6353_SPK_CON4
#define PMIC_SPK_OFFSET_R_MODE_MASK                               0x1
#define PMIC_SPK_OFFSET_R_MODE_SHIFT                              14
#define PMIC_SPK_TRIM_DONE_R_ADDR                                 MT6353_SPK_CON4
#define PMIC_SPK_TRIM_DONE_R_MASK                                 0x1
#define PMIC_SPK_TRIM_DONE_R_SHIFT                                15
#define PMIC_RG_SPK_INTG_RST_R_ADDR                               MT6353_SPK_CON5
#define PMIC_RG_SPK_INTG_RST_R_MASK                               0x1
#define PMIC_RG_SPK_INTG_RST_R_SHIFT                              0
#define PMIC_RG_SPK_FORCE_EN_R_ADDR                               MT6353_SPK_CON5
#define PMIC_RG_SPK_FORCE_EN_R_MASK                               0x1
#define PMIC_RG_SPK_FORCE_EN_R_SHIFT                              1
#define PMIC_RG_SPK_SLEW_R_ADDR                                   MT6353_SPK_CON5
#define PMIC_RG_SPK_SLEW_R_MASK                                   0x3
#define PMIC_RG_SPK_SLEW_R_SHIFT                                  2
#define PMIC_RG_SPKAB_OBIAS_R_ADDR                                MT6353_SPK_CON5
#define PMIC_RG_SPKAB_OBIAS_R_MASK                                0x3
#define PMIC_RG_SPKAB_OBIAS_R_SHIFT                               4
#define PMIC_RG_SPKRCV_EN_R_ADDR                                  MT6353_SPK_CON5
#define PMIC_RG_SPKRCV_EN_R_MASK                                  0x1
#define PMIC_RG_SPKRCV_EN_R_SHIFT                                 6
#define PMIC_RG_SPK_DRC_EN_R_ADDR                                 MT6353_SPK_CON5
#define PMIC_RG_SPK_DRC_EN_R_MASK                                 0x1
#define PMIC_RG_SPK_DRC_EN_R_SHIFT                                7
#define PMIC_RG_SPK_TEST_EN_R_ADDR                                MT6353_SPK_CON5
#define PMIC_RG_SPK_TEST_EN_R_MASK                                0x1
#define PMIC_RG_SPK_TEST_EN_R_SHIFT                               8
#define PMIC_RG_SPKAB_OC_EN_R_ADDR                                MT6353_SPK_CON5
#define PMIC_RG_SPKAB_OC_EN_R_MASK                                0x1
#define PMIC_RG_SPKAB_OC_EN_R_SHIFT                               9
#define PMIC_RG_SPK_OC_EN_R_ADDR                                  MT6353_SPK_CON5
#define PMIC_RG_SPK_OC_EN_R_MASK                                  0x1
#define PMIC_RG_SPK_OC_EN_R_SHIFT                                 10
#define PMIC_RG_SPKPGA_GAINR_ADDR                                 MT6353_SPK_CON5
#define PMIC_RG_SPKPGA_GAINR_MASK                                 0xF
#define PMIC_RG_SPKPGA_GAINR_SHIFT                                11
#define PMIC_SPK_TRIM_WND_ADDR                                    MT6353_SPK_CON6
#define PMIC_SPK_TRIM_WND_MASK                                    0x7
#define PMIC_SPK_TRIM_WND_SHIFT                                   0
#define PMIC_SPK_TRIM_THD_ADDR                                    MT6353_SPK_CON6
#define PMIC_SPK_TRIM_THD_MASK                                    0x3
#define PMIC_SPK_TRIM_THD_SHIFT                                   4
#define PMIC_SPK_OC_WND_ADDR                                      MT6353_SPK_CON6
#define PMIC_SPK_OC_WND_MASK                                      0x3
#define PMIC_SPK_OC_WND_SHIFT                                     8
#define PMIC_SPK_OC_THD_ADDR                                      MT6353_SPK_CON6
#define PMIC_SPK_OC_THD_MASK                                      0x3
#define PMIC_SPK_OC_THD_SHIFT                                     10
#define PMIC_SPK_D_OC_R_DEG_ADDR                                  MT6353_SPK_CON6
#define PMIC_SPK_D_OC_R_DEG_MASK                                  0x1
#define PMIC_SPK_D_OC_R_DEG_SHIFT                                 12
#define PMIC_SPK_AB_OC_R_DEG_ADDR                                 MT6353_SPK_CON6
#define PMIC_SPK_AB_OC_R_DEG_MASK                                 0x1
#define PMIC_SPK_AB_OC_R_DEG_SHIFT                                13
#define PMIC_SPK_D_OC_L_DEG_ADDR                                  MT6353_SPK_CON6
#define PMIC_SPK_D_OC_L_DEG_MASK                                  0x1
#define PMIC_SPK_D_OC_L_DEG_SHIFT                                 14
#define PMIC_SPK_AB_OC_L_DEG_ADDR                                 MT6353_SPK_CON6
#define PMIC_SPK_AB_OC_L_DEG_MASK                                 0x1
#define PMIC_SPK_AB_OC_L_DEG_SHIFT                                15
#define PMIC_SPK_TD1_ADDR                                         MT6353_SPK_CON7
#define PMIC_SPK_TD1_MASK                                         0xF
#define PMIC_SPK_TD1_SHIFT                                        0
#define PMIC_SPK_TD2_ADDR                                         MT6353_SPK_CON7
#define PMIC_SPK_TD2_MASK                                         0xF
#define PMIC_SPK_TD2_SHIFT                                        4
#define PMIC_SPK_TD3_ADDR                                         MT6353_SPK_CON7
#define PMIC_SPK_TD3_MASK                                         0xF
#define PMIC_SPK_TD3_SHIFT                                        8
#define PMIC_SPK_TRIM_DIV_ADDR                                    MT6353_SPK_CON7
#define PMIC_SPK_TRIM_DIV_MASK                                    0x7
#define PMIC_SPK_TRIM_DIV_SHIFT                                   12
#define PMIC_RG_BTL_SET_ADDR                                      MT6353_SPK_CON8
#define PMIC_RG_BTL_SET_MASK                                      0x3
#define PMIC_RG_BTL_SET_SHIFT                                     0
#define PMIC_RG_SPK_IBIAS_SEL_ADDR                                MT6353_SPK_CON8
#define PMIC_RG_SPK_IBIAS_SEL_MASK                                0x3
#define PMIC_RG_SPK_IBIAS_SEL_SHIFT                               2
#define PMIC_RG_SPK_CCODE_ADDR                                    MT6353_SPK_CON8
#define PMIC_RG_SPK_CCODE_MASK                                    0xF
#define PMIC_RG_SPK_CCODE_SHIFT                                   4
#define PMIC_RG_SPK_EN_VIEW_VCM_ADDR                              MT6353_SPK_CON8
#define PMIC_RG_SPK_EN_VIEW_VCM_MASK                              0x1
#define PMIC_RG_SPK_EN_VIEW_VCM_SHIFT                             8
#define PMIC_RG_SPK_EN_VIEW_CLK_ADDR                              MT6353_SPK_CON8
#define PMIC_RG_SPK_EN_VIEW_CLK_MASK                              0x1
#define PMIC_RG_SPK_EN_VIEW_CLK_SHIFT                             9
#define PMIC_RG_SPK_VCM_SEL_ADDR                                  MT6353_SPK_CON8
#define PMIC_RG_SPK_VCM_SEL_MASK                                  0x1
#define PMIC_RG_SPK_VCM_SEL_SHIFT                                 10
#define PMIC_RG_SPK_VCM_IBSEL_ADDR                                MT6353_SPK_CON8
#define PMIC_RG_SPK_VCM_IBSEL_MASK                                0x1
#define PMIC_RG_SPK_VCM_IBSEL_SHIFT                               11
#define PMIC_RG_SPK_FBRC_EN_ADDR                                  MT6353_SPK_CON8
#define PMIC_RG_SPK_FBRC_EN_MASK                                  0x1
#define PMIC_RG_SPK_FBRC_EN_SHIFT                                 12
#define PMIC_RG_SPKAB_OVDRV_ADDR                                  MT6353_SPK_CON8
#define PMIC_RG_SPKAB_OVDRV_MASK                                  0x1
#define PMIC_RG_SPKAB_OVDRV_SHIFT                                 13
#define PMIC_RG_SPK_OCTH_D_ADDR                                   MT6353_SPK_CON8
#define PMIC_RG_SPK_OCTH_D_MASK                                   0x1
#define PMIC_RG_SPK_OCTH_D_SHIFT                                  14
#define PMIC_RG_SPKPGA_GAINL_ADDR                                 MT6353_SPK_CON9
#define PMIC_RG_SPKPGA_GAINL_MASK                                 0xF
#define PMIC_RG_SPKPGA_GAINL_SHIFT                                8
#define PMIC_SPK_RSV0_ADDR                                        MT6353_SPK_CON9
#define PMIC_SPK_RSV0_MASK                                        0x1
#define PMIC_SPK_RSV0_SHIFT                                       12
#define PMIC_SPK_VCM_FAST_EN_ADDR                                 MT6353_SPK_CON9
#define PMIC_SPK_VCM_FAST_EN_MASK                                 0x1
#define PMIC_SPK_VCM_FAST_EN_SHIFT                                13
#define PMIC_SPK_TEST_MODE0_ADDR                                  MT6353_SPK_CON9
#define PMIC_SPK_TEST_MODE0_MASK                                  0x1
#define PMIC_SPK_TEST_MODE0_SHIFT                                 14
#define PMIC_SPK_TEST_MODE1_ADDR                                  MT6353_SPK_CON9
#define PMIC_SPK_TEST_MODE1_MASK                                  0x1
#define PMIC_SPK_TEST_MODE1_SHIFT                                 15
#define PMIC_SPK_TD_WAIT_ADDR                                     MT6353_SPK_CON10
#define PMIC_SPK_TD_WAIT_MASK                                     0x7
#define PMIC_SPK_TD_WAIT_SHIFT                                    0
#define PMIC_SPK_TD_DONE_ADDR                                     MT6353_SPK_CON10
#define PMIC_SPK_TD_DONE_MASK                                     0x7
#define PMIC_SPK_TD_DONE_SHIFT                                    4
#define PMIC_SPK_EN_MODE_ADDR                                     MT6353_SPK_CON11
#define PMIC_SPK_EN_MODE_MASK                                     0x1
#define PMIC_SPK_EN_MODE_SHIFT                                    0
#define PMIC_SPK_VCM_FAST_SW_ADDR                                 MT6353_SPK_CON11
#define PMIC_SPK_VCM_FAST_SW_MASK                                 0x1
#define PMIC_SPK_VCM_FAST_SW_SHIFT                                1
#define PMIC_SPK_RST_R_SW_ADDR                                    MT6353_SPK_CON11
#define PMIC_SPK_RST_R_SW_MASK                                    0x1
#define PMIC_SPK_RST_R_SW_SHIFT                                   2
#define PMIC_SPK_RST_L_SW_ADDR                                    MT6353_SPK_CON11
#define PMIC_SPK_RST_L_SW_MASK                                    0x1
#define PMIC_SPK_RST_L_SW_SHIFT                                   3
#define PMIC_SPKMODE_R_SW_ADDR                                    MT6353_SPK_CON11
#define PMIC_SPKMODE_R_SW_MASK                                    0x1
#define PMIC_SPKMODE_R_SW_SHIFT                                   4
#define PMIC_SPKMODE_L_SW_ADDR                                    MT6353_SPK_CON11
#define PMIC_SPKMODE_L_SW_MASK                                    0x1
#define PMIC_SPKMODE_L_SW_SHIFT                                   5
#define PMIC_SPK_DEPOP_EN_R_SW_ADDR                               MT6353_SPK_CON11
#define PMIC_SPK_DEPOP_EN_R_SW_MASK                               0x1
#define PMIC_SPK_DEPOP_EN_R_SW_SHIFT                              6
#define PMIC_SPK_DEPOP_EN_L_SW_ADDR                               MT6353_SPK_CON11
#define PMIC_SPK_DEPOP_EN_L_SW_MASK                               0x1
#define PMIC_SPK_DEPOP_EN_L_SW_SHIFT                              7
#define PMIC_SPK_EN_R_SW_ADDR                                     MT6353_SPK_CON11
#define PMIC_SPK_EN_R_SW_MASK                                     0x1
#define PMIC_SPK_EN_R_SW_SHIFT                                    8
#define PMIC_SPK_EN_L_SW_ADDR                                     MT6353_SPK_CON11
#define PMIC_SPK_EN_L_SW_MASK                                     0x1
#define PMIC_SPK_EN_L_SW_SHIFT                                    9
#define PMIC_SPK_OUTSTG_EN_R_SW_ADDR                              MT6353_SPK_CON11
#define PMIC_SPK_OUTSTG_EN_R_SW_MASK                              0x1
#define PMIC_SPK_OUTSTG_EN_R_SW_SHIFT                             10
#define PMIC_SPK_OUTSTG_EN_L_SW_ADDR                              MT6353_SPK_CON11
#define PMIC_SPK_OUTSTG_EN_L_SW_MASK                              0x1
#define PMIC_SPK_OUTSTG_EN_L_SW_SHIFT                             11
#define PMIC_SPK_TRIM_EN_R_SW_ADDR                                MT6353_SPK_CON11
#define PMIC_SPK_TRIM_EN_R_SW_MASK                                0x1
#define PMIC_SPK_TRIM_EN_R_SW_SHIFT                               12
#define PMIC_SPK_TRIM_EN_L_SW_ADDR                                MT6353_SPK_CON11
#define PMIC_SPK_TRIM_EN_L_SW_MASK                                0x1
#define PMIC_SPK_TRIM_EN_L_SW_SHIFT                               13
#define PMIC_SPK_TRIM_STOP_R_SW_ADDR                              MT6353_SPK_CON11
#define PMIC_SPK_TRIM_STOP_R_SW_MASK                              0x1
#define PMIC_SPK_TRIM_STOP_R_SW_SHIFT                             14
#define PMIC_SPK_TRIM_STOP_L_SW_ADDR                              MT6353_SPK_CON11
#define PMIC_SPK_TRIM_STOP_L_SW_MASK                              0x1
#define PMIC_SPK_TRIM_STOP_L_SW_SHIFT                             15
#define PMIC_RG_SPK_ISENSE_TEST_EN_ADDR                           MT6353_SPK_CON12
#define PMIC_RG_SPK_ISENSE_TEST_EN_MASK                           0x1
#define PMIC_RG_SPK_ISENSE_TEST_EN_SHIFT                          7
#define PMIC_RG_SPK_ISENSE_REFSEL_ADDR                            MT6353_SPK_CON12
#define PMIC_RG_SPK_ISENSE_REFSEL_MASK                            0x7
#define PMIC_RG_SPK_ISENSE_REFSEL_SHIFT                           8
#define PMIC_RG_SPK_ISENSE_GAINSEL_ADDR                           MT6353_SPK_CON12
#define PMIC_RG_SPK_ISENSE_GAINSEL_MASK                           0x7
#define PMIC_RG_SPK_ISENSE_GAINSEL_SHIFT                          11
#define PMIC_RG_SPK_ISENSE_PDRESET_ADDR                           MT6353_SPK_CON12
#define PMIC_RG_SPK_ISENSE_PDRESET_MASK                           0x1
#define PMIC_RG_SPK_ISENSE_PDRESET_SHIFT                          14
#define PMIC_RG_SPK_ISENSE_EN_ADDR                                MT6353_SPK_CON12
#define PMIC_RG_SPK_ISENSE_EN_MASK                                0x1
#define PMIC_RG_SPK_ISENSE_EN_SHIFT                               15
#define PMIC_RG_SPK_RSV1_ADDR                                     MT6353_SPK_CON13
#define PMIC_RG_SPK_RSV1_MASK                                     0xFF
#define PMIC_RG_SPK_RSV1_SHIFT                                    0
#define PMIC_RG_SPK_RSV0_ADDR                                     MT6353_SPK_CON13
#define PMIC_RG_SPK_RSV0_MASK                                     0xFF
#define PMIC_RG_SPK_RSV0_SHIFT                                    8
#define PMIC_RG_SPK_ABD_VOLSEN_GAIN_ADDR                          MT6353_SPK_CON14
#define PMIC_RG_SPK_ABD_VOLSEN_GAIN_MASK                          0x3
#define PMIC_RG_SPK_ABD_VOLSEN_GAIN_SHIFT                         4
#define PMIC_RG_SPK_ABD_VOLSEN_EN_ADDR                            MT6353_SPK_CON14
#define PMIC_RG_SPK_ABD_VOLSEN_EN_MASK                            0x1
#define PMIC_RG_SPK_ABD_VOLSEN_EN_SHIFT                           6
#define PMIC_RG_SPK_ABD_CURSEN_SEL_ADDR                           MT6353_SPK_CON14
#define PMIC_RG_SPK_ABD_CURSEN_SEL_MASK                           0x1
#define PMIC_RG_SPK_ABD_CURSEN_SEL_SHIFT                          7
#define PMIC_RG_SPK_RSV2_ADDR                                     MT6353_SPK_CON14
#define PMIC_RG_SPK_RSV2_MASK                                     0xFF
#define PMIC_RG_SPK_RSV2_SHIFT                                    8
#define PMIC_RG_SPK_TRIM2_ADDR                                    MT6353_SPK_CON15
#define PMIC_RG_SPK_TRIM2_MASK                                    0xFF
#define PMIC_RG_SPK_TRIM2_SHIFT                                   0
#define PMIC_RG_SPK_TRIM1_ADDR                                    MT6353_SPK_CON15
#define PMIC_RG_SPK_TRIM1_MASK                                    0xFF
#define PMIC_RG_SPK_TRIM1_SHIFT                                   8
#define PMIC_RG_SPK_D_CURSEN_RSETSEL_ADDR                         MT6353_SPK_CON16
#define PMIC_RG_SPK_D_CURSEN_RSETSEL_MASK                         0x1F
#define PMIC_RG_SPK_D_CURSEN_RSETSEL_SHIFT                        0
#define PMIC_RG_SPK_D_CURSEN_GAIN_ADDR                            MT6353_SPK_CON16
#define PMIC_RG_SPK_D_CURSEN_GAIN_MASK                            0x3
#define PMIC_RG_SPK_D_CURSEN_GAIN_SHIFT                           5
#define PMIC_RG_SPK_D_CURSEN_EN_ADDR                              MT6353_SPK_CON16
#define PMIC_RG_SPK_D_CURSEN_EN_MASK                              0x1
#define PMIC_RG_SPK_D_CURSEN_EN_SHIFT                             7
#define PMIC_RG_SPK_AB_CURSEN_RSETSEL_ADDR                        MT6353_SPK_CON16
#define PMIC_RG_SPK_AB_CURSEN_RSETSEL_MASK                        0x1F
#define PMIC_RG_SPK_AB_CURSEN_RSETSEL_SHIFT                       8
#define PMIC_RG_SPK_AB_CURSEN_GAIN_ADDR                           MT6353_SPK_CON16
#define PMIC_RG_SPK_AB_CURSEN_GAIN_MASK                           0x3
#define PMIC_RG_SPK_AB_CURSEN_GAIN_SHIFT                          13
#define PMIC_RG_SPK_AB_CURSEN_EN_ADDR                             MT6353_SPK_CON16
#define PMIC_RG_SPK_AB_CURSEN_EN_MASK                             0x1
#define PMIC_RG_SPK_AB_CURSEN_EN_SHIFT                            15
#define PMIC_RG_SPKPGA_GAIN_ADDR                                  MT6353_SPK_ANA_CON0
#define PMIC_RG_SPKPGA_GAIN_MASK                                  0xF
#define PMIC_RG_SPKPGA_GAIN_SHIFT                                 11
#define PMIC_RG_SPK_RSV_ADDR                                      MT6353_SPK_ANA_CON1
#define PMIC_RG_SPK_RSV_MASK                                      0xFF
#define PMIC_RG_SPK_RSV_SHIFT                                     0
#define PMIC_RG_ISENSE_PD_RESET_ADDR                              MT6353_SPK_ANA_CON1
#define PMIC_RG_ISENSE_PD_RESET_MASK                              0x1
#define PMIC_RG_ISENSE_PD_RESET_SHIFT                             11
#define PMIC_RG_AUDIVLPWRUP_VAUDP12_ADDR                          MT6353_SPK_ANA_CON3
#define PMIC_RG_AUDIVLPWRUP_VAUDP12_MASK                          0x1
#define PMIC_RG_AUDIVLPWRUP_VAUDP12_SHIFT                         4
#define PMIC_RG_AUDIVLSTARTUP_VAUDP12_ADDR                        MT6353_SPK_ANA_CON3
#define PMIC_RG_AUDIVLSTARTUP_VAUDP12_MASK                        0x1
#define PMIC_RG_AUDIVLSTARTUP_VAUDP12_SHIFT                       5
#define PMIC_RG_AUDIVLMUXSEL_VAUDP12_ADDR                         MT6353_SPK_ANA_CON3
#define PMIC_RG_AUDIVLMUXSEL_VAUDP12_MASK                         0x7
#define PMIC_RG_AUDIVLMUXSEL_VAUDP12_SHIFT                        6
#define PMIC_RG_AUDIVLMUTE_VAUDP12_ADDR                           MT6353_SPK_ANA_CON3
#define PMIC_RG_AUDIVLMUTE_VAUDP12_MASK                           0x1
#define PMIC_RG_AUDIVLMUTE_VAUDP12_SHIFT                          9
#define PMIC_FQMTR_TCKSEL_ADDR                                    MT6353_FQMTR_CON0
#define PMIC_FQMTR_TCKSEL_MASK                                    0x7
#define PMIC_FQMTR_TCKSEL_SHIFT                                   0
#define PMIC_FQMTR_BUSY_ADDR                                      MT6353_FQMTR_CON0
#define PMIC_FQMTR_BUSY_MASK                                      0x1
#define PMIC_FQMTR_BUSY_SHIFT                                     3
#define PMIC_FQMTR_DCXO26M_EN_ADDR                                MT6353_FQMTR_CON0
#define PMIC_FQMTR_DCXO26M_EN_MASK                                0x1
#define PMIC_FQMTR_DCXO26M_EN_SHIFT                               4
#define PMIC_FQMTR_EN_ADDR                                        MT6353_FQMTR_CON0
#define PMIC_FQMTR_EN_MASK                                        0x1
#define PMIC_FQMTR_EN_SHIFT                                       15
#define PMIC_FQMTR_WINSET_ADDR                                    MT6353_FQMTR_CON1
#define PMIC_FQMTR_WINSET_MASK                                    0xFFFF
#define PMIC_FQMTR_WINSET_SHIFT                                   0
#define PMIC_FQMTR_DATA_ADDR                                      MT6353_FQMTR_CON2
#define PMIC_FQMTR_DATA_MASK                                      0xFFFF
#define PMIC_FQMTR_DATA_SHIFT                                     0
#define PMIC_RG_TRIM_EN_ADDR                                      MT6353_ISINK_ANA_CON_0
#define PMIC_RG_TRIM_EN_MASK                                      0x1
#define PMIC_RG_TRIM_EN_SHIFT                                     0
#define PMIC_RG_TRIM_SEL_ADDR                                     MT6353_ISINK_ANA_CON_0
#define PMIC_RG_TRIM_SEL_MASK                                     0x7
#define PMIC_RG_TRIM_SEL_SHIFT                                    1
#define PMIC_RG_ISINKS_RSV_ADDR                                   MT6353_ISINK_ANA_CON_0
#define PMIC_RG_ISINKS_RSV_MASK                                   0xFF
#define PMIC_RG_ISINKS_RSV_SHIFT                                  4
#define PMIC_RG_ISINK0_DOUBLE_EN_ADDR                             MT6353_ISINK_ANA_CON_0
#define PMIC_RG_ISINK0_DOUBLE_EN_MASK                             0x1
#define PMIC_RG_ISINK0_DOUBLE_EN_SHIFT                            12
#define PMIC_RG_ISINK1_DOUBLE_EN_ADDR                             MT6353_ISINK_ANA_CON_0
#define PMIC_RG_ISINK1_DOUBLE_EN_MASK                             0x1
#define PMIC_RG_ISINK1_DOUBLE_EN_SHIFT                            13
#define PMIC_RG_ISINK2_DOUBLE_EN_ADDR                             MT6353_ISINK_ANA_CON_0
#define PMIC_RG_ISINK2_DOUBLE_EN_MASK                             0x1
#define PMIC_RG_ISINK2_DOUBLE_EN_SHIFT                            14
#define PMIC_RG_ISINK3_DOUBLE_EN_ADDR                             MT6353_ISINK_ANA_CON_0
#define PMIC_RG_ISINK3_DOUBLE_EN_MASK                             0x1
#define PMIC_RG_ISINK3_DOUBLE_EN_SHIFT                            15
#define PMIC_ISINK_DIM0_FSEL_ADDR                                 MT6353_ISINK0_CON0
#define PMIC_ISINK_DIM0_FSEL_MASK                                 0xFFFF
#define PMIC_ISINK_DIM0_FSEL_SHIFT                                0
#define PMIC_ISINK0_RSV1_ADDR                                     MT6353_ISINK0_CON1
#define PMIC_ISINK0_RSV1_MASK                                     0xF
#define PMIC_ISINK0_RSV1_SHIFT                                    0
#define PMIC_ISINK0_RSV0_ADDR                                     MT6353_ISINK0_CON1
#define PMIC_ISINK0_RSV0_MASK                                     0x7
#define PMIC_ISINK0_RSV0_SHIFT                                    4
#define PMIC_ISINK_DIM0_DUTY_ADDR                                 MT6353_ISINK0_CON1
#define PMIC_ISINK_DIM0_DUTY_MASK                                 0x1F
#define PMIC_ISINK_DIM0_DUTY_SHIFT                                7
#define PMIC_ISINK_CH0_STEP_ADDR                                  MT6353_ISINK0_CON1
#define PMIC_ISINK_CH0_STEP_MASK                                  0x7
#define PMIC_ISINK_CH0_STEP_SHIFT                                 12
#define PMIC_ISINK_BREATH0_TF2_SEL_ADDR                           MT6353_ISINK0_CON2
#define PMIC_ISINK_BREATH0_TF2_SEL_MASK                           0xF
#define PMIC_ISINK_BREATH0_TF2_SEL_SHIFT                          0
#define PMIC_ISINK_BREATH0_TF1_SEL_ADDR                           MT6353_ISINK0_CON2
#define PMIC_ISINK_BREATH0_TF1_SEL_MASK                           0xF
#define PMIC_ISINK_BREATH0_TF1_SEL_SHIFT                          4
#define PMIC_ISINK_BREATH0_TR2_SEL_ADDR                           MT6353_ISINK0_CON2
#define PMIC_ISINK_BREATH0_TR2_SEL_MASK                           0xF
#define PMIC_ISINK_BREATH0_TR2_SEL_SHIFT                          8
#define PMIC_ISINK_BREATH0_TR1_SEL_ADDR                           MT6353_ISINK0_CON2
#define PMIC_ISINK_BREATH0_TR1_SEL_MASK                           0xF
#define PMIC_ISINK_BREATH0_TR1_SEL_SHIFT                          12
#define PMIC_ISINK_BREATH0_TOFF_SEL_ADDR                          MT6353_ISINK0_CON3
#define PMIC_ISINK_BREATH0_TOFF_SEL_MASK                          0xF
#define PMIC_ISINK_BREATH0_TOFF_SEL_SHIFT                         0
#define PMIC_ISINK_BREATH0_TON_SEL_ADDR                           MT6353_ISINK0_CON3
#define PMIC_ISINK_BREATH0_TON_SEL_MASK                           0xF
#define PMIC_ISINK_BREATH0_TON_SEL_SHIFT                          8
#define PMIC_ISINK_DIM1_FSEL_ADDR                                 MT6353_ISINK1_CON0
#define PMIC_ISINK_DIM1_FSEL_MASK                                 0xFFFF
#define PMIC_ISINK_DIM1_FSEL_SHIFT                                0
#define PMIC_ISINK1_RSV1_ADDR                                     MT6353_ISINK1_CON1
#define PMIC_ISINK1_RSV1_MASK                                     0xF
#define PMIC_ISINK1_RSV1_SHIFT                                    0
#define PMIC_ISINK1_RSV0_ADDR                                     MT6353_ISINK1_CON1
#define PMIC_ISINK1_RSV0_MASK                                     0x7
#define PMIC_ISINK1_RSV0_SHIFT                                    4
#define PMIC_ISINK_DIM1_DUTY_ADDR                                 MT6353_ISINK1_CON1
#define PMIC_ISINK_DIM1_DUTY_MASK                                 0x1F
#define PMIC_ISINK_DIM1_DUTY_SHIFT                                7
#define PMIC_ISINK_CH1_STEP_ADDR                                  MT6353_ISINK1_CON1
#define PMIC_ISINK_CH1_STEP_MASK                                  0x7
#define PMIC_ISINK_CH1_STEP_SHIFT                                 12
#define PMIC_ISINK_BREATH1_TF2_SEL_ADDR                           MT6353_ISINK1_CON2
#define PMIC_ISINK_BREATH1_TF2_SEL_MASK                           0xF
#define PMIC_ISINK_BREATH1_TF2_SEL_SHIFT                          0
#define PMIC_ISINK_BREATH1_TF1_SEL_ADDR                           MT6353_ISINK1_CON2
#define PMIC_ISINK_BREATH1_TF1_SEL_MASK                           0xF
#define PMIC_ISINK_BREATH1_TF1_SEL_SHIFT                          4
#define PMIC_ISINK_BREATH1_TR2_SEL_ADDR                           MT6353_ISINK1_CON2
#define PMIC_ISINK_BREATH1_TR2_SEL_MASK                           0xF
#define PMIC_ISINK_BREATH1_TR2_SEL_SHIFT                          8
#define PMIC_ISINK_BREATH1_TR1_SEL_ADDR                           MT6353_ISINK1_CON2
#define PMIC_ISINK_BREATH1_TR1_SEL_MASK                           0xF
#define PMIC_ISINK_BREATH1_TR1_SEL_SHIFT                          12
#define PMIC_ISINK_BREATH1_TOFF_SEL_ADDR                          MT6353_ISINK1_CON3
#define PMIC_ISINK_BREATH1_TOFF_SEL_MASK                          0xF
#define PMIC_ISINK_BREATH1_TOFF_SEL_SHIFT                         0
#define PMIC_ISINK_BREATH1_TON_SEL_ADDR                           MT6353_ISINK1_CON3
#define PMIC_ISINK_BREATH1_TON_SEL_MASK                           0xF
#define PMIC_ISINK_BREATH1_TON_SEL_SHIFT                          8
#define PMIC_AD_NI_ISINK3_STATUS_ADDR                             MT6353_ISINK_ANA1
#define PMIC_AD_NI_ISINK3_STATUS_MASK                             0x1
#define PMIC_AD_NI_ISINK3_STATUS_SHIFT                            0
#define PMIC_AD_NI_ISINK2_STATUS_ADDR                             MT6353_ISINK_ANA1
#define PMIC_AD_NI_ISINK2_STATUS_MASK                             0x1
#define PMIC_AD_NI_ISINK2_STATUS_SHIFT                            1
#define PMIC_AD_NI_ISINK1_STATUS_ADDR                             MT6353_ISINK_ANA1
#define PMIC_AD_NI_ISINK1_STATUS_MASK                             0x1
#define PMIC_AD_NI_ISINK1_STATUS_SHIFT                            2
#define PMIC_AD_NI_ISINK0_STATUS_ADDR                             MT6353_ISINK_ANA1
#define PMIC_AD_NI_ISINK0_STATUS_MASK                             0x1
#define PMIC_AD_NI_ISINK0_STATUS_SHIFT                            3
#define PMIC_ISINK_PHASE0_DLY_EN_ADDR                             MT6353_ISINK_PHASE_DLY
#define PMIC_ISINK_PHASE0_DLY_EN_MASK                             0x1
#define PMIC_ISINK_PHASE0_DLY_EN_SHIFT                            0
#define PMIC_ISINK_PHASE1_DLY_EN_ADDR                             MT6353_ISINK_PHASE_DLY
#define PMIC_ISINK_PHASE1_DLY_EN_MASK                             0x1
#define PMIC_ISINK_PHASE1_DLY_EN_SHIFT                            1
#define PMIC_ISINK_PHASE_DLY_TC_ADDR                              MT6353_ISINK_PHASE_DLY
#define PMIC_ISINK_PHASE_DLY_TC_MASK                              0x3
#define PMIC_ISINK_PHASE_DLY_TC_SHIFT                             4
#define PMIC_ISINK_CHOP0_SW_ADDR                                  MT6353_ISINK_PHASE_DLY
#define PMIC_ISINK_CHOP0_SW_MASK                                  0x1
#define PMIC_ISINK_CHOP0_SW_SHIFT                                 12
#define PMIC_ISINK_CHOP1_SW_ADDR                                  MT6353_ISINK_PHASE_DLY
#define PMIC_ISINK_CHOP1_SW_MASK                                  0x1
#define PMIC_ISINK_CHOP1_SW_SHIFT                                 13
#define PMIC_ISINK_SFSTR1_EN_ADDR                                 MT6353_ISINK_SFSTR
#define PMIC_ISINK_SFSTR1_EN_MASK                                 0x1
#define PMIC_ISINK_SFSTR1_EN_SHIFT                                8
#define PMIC_ISINK_SFSTR1_TC_ADDR                                 MT6353_ISINK_SFSTR
#define PMIC_ISINK_SFSTR1_TC_MASK                                 0x3
#define PMIC_ISINK_SFSTR1_TC_SHIFT                                9
#define PMIC_ISINK_SFSTR0_EN_ADDR                                 MT6353_ISINK_SFSTR
#define PMIC_ISINK_SFSTR0_EN_MASK                                 0x1
#define PMIC_ISINK_SFSTR0_EN_SHIFT                                12
#define PMIC_ISINK_SFSTR0_TC_ADDR                                 MT6353_ISINK_SFSTR
#define PMIC_ISINK_SFSTR0_TC_MASK                                 0x3
#define PMIC_ISINK_SFSTR0_TC_SHIFT                                13
#define PMIC_ISINK_CH0_EN_ADDR                                    MT6353_ISINK_EN_CTRL
#define PMIC_ISINK_CH0_EN_MASK                                    0x1
#define PMIC_ISINK_CH0_EN_SHIFT                                   0
#define PMIC_ISINK_CH1_EN_ADDR                                    MT6353_ISINK_EN_CTRL
#define PMIC_ISINK_CH1_EN_MASK                                    0x1
#define PMIC_ISINK_CH1_EN_SHIFT                                   1
#define PMIC_ISINK_CHOP0_EN_ADDR                                  MT6353_ISINK_EN_CTRL
#define PMIC_ISINK_CHOP0_EN_MASK                                  0x1
#define PMIC_ISINK_CHOP0_EN_SHIFT                                 4
#define PMIC_ISINK_CHOP1_EN_ADDR                                  MT6353_ISINK_EN_CTRL
#define PMIC_ISINK_CHOP1_EN_MASK                                  0x1
#define PMIC_ISINK_CHOP1_EN_SHIFT                                 5
#define PMIC_ISINK_CH0_BIAS_EN_ADDR                               MT6353_ISINK_EN_CTRL
#define PMIC_ISINK_CH0_BIAS_EN_MASK                               0x1
#define PMIC_ISINK_CH0_BIAS_EN_SHIFT                              8
#define PMIC_ISINK_CH1_BIAS_EN_ADDR                               MT6353_ISINK_EN_CTRL
#define PMIC_ISINK_CH1_BIAS_EN_MASK                               0x1
#define PMIC_ISINK_CH1_BIAS_EN_SHIFT                              9
#define PMIC_ISINK_RSV_ADDR                                       MT6353_ISINK_MODE_CTRL
#define PMIC_ISINK_RSV_MASK                                       0x1F
#define PMIC_ISINK_RSV_SHIFT                                      0
#define PMIC_ISINK_CH1_MODE_ADDR                                  MT6353_ISINK_MODE_CTRL
#define PMIC_ISINK_CH1_MODE_MASK                                  0x3
#define PMIC_ISINK_CH1_MODE_SHIFT                                 12
#define PMIC_ISINK_CH0_MODE_ADDR                                  MT6353_ISINK_MODE_CTRL
#define PMIC_ISINK_CH0_MODE_MASK                                  0x3
#define PMIC_ISINK_CH0_MODE_SHIFT                                 14
#define PMIC_DA_QI_ISINKS_CH3_STEP_ADDR                           MT6353_ISINK_ANA_CON0
#define PMIC_DA_QI_ISINKS_CH3_STEP_MASK                           0x7
#define PMIC_DA_QI_ISINKS_CH3_STEP_SHIFT                          0
#define PMIC_DA_QI_ISINKS_CH2_STEP_ADDR                           MT6353_ISINK_ANA_CON0
#define PMIC_DA_QI_ISINKS_CH2_STEP_MASK                           0x7
#define PMIC_DA_QI_ISINKS_CH2_STEP_SHIFT                          3
#define PMIC_DA_QI_ISINKS_CH1_STEP_ADDR                           MT6353_ISINK_ANA_CON0
#define PMIC_DA_QI_ISINKS_CH1_STEP_MASK                           0x7
#define PMIC_DA_QI_ISINKS_CH1_STEP_SHIFT                          6
#define PMIC_DA_QI_ISINKS_CH0_STEP_ADDR                           MT6353_ISINK_ANA_CON0
#define PMIC_DA_QI_ISINKS_CH0_STEP_MASK                           0x7
#define PMIC_DA_QI_ISINKS_CH0_STEP_SHIFT                          9
#define PMIC_ISINK2_RSV1_ADDR                                     MT6353_ISINK2_CON1
#define PMIC_ISINK2_RSV1_MASK                                     0xF
#define PMIC_ISINK2_RSV1_SHIFT                                    0
#define PMIC_ISINK2_RSV0_ADDR                                     MT6353_ISINK2_CON1
#define PMIC_ISINK2_RSV0_MASK                                     0x7
#define PMIC_ISINK2_RSV0_SHIFT                                    4
#define PMIC_ISINK_CH2_STEP_ADDR                                  MT6353_ISINK2_CON1
#define PMIC_ISINK_CH2_STEP_MASK                                  0x7
#define PMIC_ISINK_CH2_STEP_SHIFT                                 12
#define PMIC_ISINK3_RSV1_ADDR                                     MT6353_ISINK3_CON1
#define PMIC_ISINK3_RSV1_MASK                                     0xF
#define PMIC_ISINK3_RSV1_SHIFT                                    0
#define PMIC_ISINK3_RSV0_ADDR                                     MT6353_ISINK3_CON1
#define PMIC_ISINK3_RSV0_MASK                                     0x7
#define PMIC_ISINK3_RSV0_SHIFT                                    4
#define PMIC_ISINK_CH3_STEP_ADDR                                  MT6353_ISINK3_CON1
#define PMIC_ISINK_CH3_STEP_MASK                                  0x7
#define PMIC_ISINK_CH3_STEP_SHIFT                                 12
#define PMIC_ISINK_CHOP3_SW_ADDR                                  MT6353_ISINK_PHASE_DLY_SMPL
#define PMIC_ISINK_CHOP3_SW_MASK                                  0x1
#define PMIC_ISINK_CHOP3_SW_SHIFT                                 14
#define PMIC_ISINK_CHOP2_SW_ADDR                                  MT6353_ISINK_PHASE_DLY_SMPL
#define PMIC_ISINK_CHOP2_SW_MASK                                  0x1
#define PMIC_ISINK_CHOP2_SW_SHIFT                                 15
#define PMIC_ISINK_CH3_EN_ADDR                                    MT6353_ISINK_EN_CTRL_SMPL
#define PMIC_ISINK_CH3_EN_MASK                                    0x1
#define PMIC_ISINK_CH3_EN_SHIFT                                   2
#define PMIC_ISINK_CH2_EN_ADDR                                    MT6353_ISINK_EN_CTRL_SMPL
#define PMIC_ISINK_CH2_EN_MASK                                    0x1
#define PMIC_ISINK_CH2_EN_SHIFT                                   3
#define PMIC_ISINK_CHOP3_EN_ADDR                                  MT6353_ISINK_EN_CTRL_SMPL
#define PMIC_ISINK_CHOP3_EN_MASK                                  0x1
#define PMIC_ISINK_CHOP3_EN_SHIFT                                 6
#define PMIC_ISINK_CHOP2_EN_ADDR                                  MT6353_ISINK_EN_CTRL_SMPL
#define PMIC_ISINK_CHOP2_EN_MASK                                  0x1
#define PMIC_ISINK_CHOP2_EN_SHIFT                                 7
#define PMIC_ISINK_CH3_BIAS_EN_ADDR                               MT6353_ISINK_EN_CTRL_SMPL
#define PMIC_ISINK_CH3_BIAS_EN_MASK                               0x1
#define PMIC_ISINK_CH3_BIAS_EN_SHIFT                              10
#define PMIC_ISINK_CH2_BIAS_EN_ADDR                               MT6353_ISINK_EN_CTRL_SMPL
#define PMIC_ISINK_CH2_BIAS_EN_MASK                               0x1
#define PMIC_ISINK_CH2_BIAS_EN_SHIFT                              11
#define PMIC_CHRIND_DIM_FSEL_ADDR                                 MT6353_CHRIND_CON0
#define PMIC_CHRIND_DIM_FSEL_MASK                                 0xFFFF
#define PMIC_CHRIND_DIM_FSEL_SHIFT                                0
#define PMIC_CHRIND_RSV1_ADDR                                     MT6353_CHRIND_CON1
#define PMIC_CHRIND_RSV1_MASK                                     0xF
#define PMIC_CHRIND_RSV1_SHIFT                                    0
#define PMIC_CHRIND_RSV0_ADDR                                     MT6353_CHRIND_CON1
#define PMIC_CHRIND_RSV0_MASK                                     0x7
#define PMIC_CHRIND_RSV0_SHIFT                                    4
#define PMIC_CHRIND_DIM_DUTY_ADDR                                 MT6353_CHRIND_CON1
#define PMIC_CHRIND_DIM_DUTY_MASK                                 0x1F
#define PMIC_CHRIND_DIM_DUTY_SHIFT                                7
#define PMIC_CHRIND_STEP_ADDR                                     MT6353_CHRIND_CON1
#define PMIC_CHRIND_STEP_MASK                                     0x7
#define PMIC_CHRIND_STEP_SHIFT                                    12
#define PMIC_CHRIND_BREATH_TF2_SEL_ADDR                           MT6353_CHRIND_CON2
#define PMIC_CHRIND_BREATH_TF2_SEL_MASK                           0xF
#define PMIC_CHRIND_BREATH_TF2_SEL_SHIFT                          0
#define PMIC_CHRIND_BREATH_TF1_SEL_ADDR                           MT6353_CHRIND_CON2
#define PMIC_CHRIND_BREATH_TF1_SEL_MASK                           0xF
#define PMIC_CHRIND_BREATH_TF1_SEL_SHIFT                          4
#define PMIC_CHRIND_BREATH_TR2_SEL_ADDR                           MT6353_CHRIND_CON2
#define PMIC_CHRIND_BREATH_TR2_SEL_MASK                           0xF
#define PMIC_CHRIND_BREATH_TR2_SEL_SHIFT                          8
#define PMIC_CHRIND_BREATH_TR1_SEL_ADDR                           MT6353_CHRIND_CON2
#define PMIC_CHRIND_BREATH_TR1_SEL_MASK                           0xF
#define PMIC_CHRIND_BREATH_TR1_SEL_SHIFT                          12
#define PMIC_CHRIND_BREATH_TOFF_SEL_ADDR                          MT6353_CHRIND_CON3
#define PMIC_CHRIND_BREATH_TOFF_SEL_MASK                          0xF
#define PMIC_CHRIND_BREATH_TOFF_SEL_SHIFT                         0
#define PMIC_CHRIND_BREATH_TON_SEL_ADDR                           MT6353_CHRIND_CON3
#define PMIC_CHRIND_BREATH_TON_SEL_MASK                           0xF
#define PMIC_CHRIND_BREATH_TON_SEL_SHIFT                          8
#define PMIC_CHRIND_SFSTR_EN_ADDR                                 MT6353_CHRIND_EN_CTRL
#define PMIC_CHRIND_SFSTR_EN_MASK                                 0x1
#define PMIC_CHRIND_SFSTR_EN_SHIFT                                0
#define PMIC_CHRIND_SFSTR_TC_ADDR                                 MT6353_CHRIND_EN_CTRL
#define PMIC_CHRIND_SFSTR_TC_MASK                                 0x3
#define PMIC_CHRIND_SFSTR_TC_SHIFT                                1
#define PMIC_CHRIND_EN_SEL_ADDR                                   MT6353_CHRIND_EN_CTRL
#define PMIC_CHRIND_EN_SEL_MASK                                   0x1
#define PMIC_CHRIND_EN_SEL_SHIFT                                  3
#define PMIC_CHRIND_EN_ADDR                                       MT6353_CHRIND_EN_CTRL
#define PMIC_CHRIND_EN_MASK                                       0x1
#define PMIC_CHRIND_EN_SHIFT                                      4
#define PMIC_CHRIND_CHOP_EN_ADDR                                  MT6353_CHRIND_EN_CTRL
#define PMIC_CHRIND_CHOP_EN_MASK                                  0x1
#define PMIC_CHRIND_CHOP_EN_SHIFT                                 5
#define PMIC_CHRIND_MODE_ADDR                                     MT6353_CHRIND_EN_CTRL
#define PMIC_CHRIND_MODE_MASK                                     0x3
#define PMIC_CHRIND_MODE_SHIFT                                    6
#define PMIC_CHRIND_CHOP_SW_ADDR                                  MT6353_CHRIND_EN_CTRL
#define PMIC_CHRIND_CHOP_SW_MASK                                  0x1
#define PMIC_CHRIND_CHOP_SW_SHIFT                                 8
#define PMIC_CHRIND_BIAS_EN_ADDR                                  MT6353_CHRIND_EN_CTRL
#define PMIC_CHRIND_BIAS_EN_MASK                                  0x1
#define PMIC_CHRIND_BIAS_EN_SHIFT                                 9
#define PMIC_RG_SLP_RW_EN_ADDR                                    MT6353_RG_SPI_CON
#define PMIC_RG_SLP_RW_EN_MASK                                    0x1
#define PMIC_RG_SLP_RW_EN_SHIFT                                   0
#define PMIC_RG_SPI_RSV_ADDR                                      MT6353_RG_SPI_CON
#define PMIC_RG_SPI_RSV_MASK                                      0x7FFF
#define PMIC_RG_SPI_RSV_SHIFT                                     1
#define PMIC_DEW_DIO_EN_ADDR                                      MT6353_DEW_DIO_EN
#define PMIC_DEW_DIO_EN_MASK                                      0x1
#define PMIC_DEW_DIO_EN_SHIFT                                     0
#define PMIC_DEW_READ_TEST_ADDR                                   MT6353_DEW_READ_TEST
#define PMIC_DEW_READ_TEST_MASK                                   0xFFFF
#define PMIC_DEW_READ_TEST_SHIFT                                  0
#define PMIC_DEW_WRITE_TEST_ADDR                                  MT6353_DEW_WRITE_TEST
#define PMIC_DEW_WRITE_TEST_MASK                                  0xFFFF
#define PMIC_DEW_WRITE_TEST_SHIFT                                 0
#define PMIC_DEW_CRC_SWRST_ADDR                                   MT6353_DEW_CRC_SWRST
#define PMIC_DEW_CRC_SWRST_MASK                                   0x1
#define PMIC_DEW_CRC_SWRST_SHIFT                                  0
#define PMIC_DEW_CRC_EN_ADDR                                      MT6353_DEW_CRC_EN
#define PMIC_DEW_CRC_EN_MASK                                      0x1
#define PMIC_DEW_CRC_EN_SHIFT                                     0
#define PMIC_DEW_CRC_VAL_ADDR                                     MT6353_DEW_CRC_VAL
#define PMIC_DEW_CRC_VAL_MASK                                     0xFF
#define PMIC_DEW_CRC_VAL_SHIFT                                    0
#define PMIC_DEW_DBG_MON_SEL_ADDR                                 MT6353_DEW_DBG_MON_SEL
#define PMIC_DEW_DBG_MON_SEL_MASK                                 0xF
#define PMIC_DEW_DBG_MON_SEL_SHIFT                                0
#define PMIC_DEW_CIPHER_KEY_SEL_ADDR                              MT6353_DEW_CIPHER_KEY_SEL
#define PMIC_DEW_CIPHER_KEY_SEL_MASK                              0x3
#define PMIC_DEW_CIPHER_KEY_SEL_SHIFT                             0
#define PMIC_DEW_CIPHER_IV_SEL_ADDR                               MT6353_DEW_CIPHER_IV_SEL
#define PMIC_DEW_CIPHER_IV_SEL_MASK                               0x3
#define PMIC_DEW_CIPHER_IV_SEL_SHIFT                              0
#define PMIC_DEW_CIPHER_EN_ADDR                                   MT6353_DEW_CIPHER_EN
#define PMIC_DEW_CIPHER_EN_MASK                                   0x1
#define PMIC_DEW_CIPHER_EN_SHIFT                                  0
#define PMIC_DEW_CIPHER_RDY_ADDR                                  MT6353_DEW_CIPHER_RDY
#define PMIC_DEW_CIPHER_RDY_MASK                                  0x1
#define PMIC_DEW_CIPHER_RDY_SHIFT                                 0
#define PMIC_DEW_CIPHER_MODE_ADDR                                 MT6353_DEW_CIPHER_MODE
#define PMIC_DEW_CIPHER_MODE_MASK                                 0x1
#define PMIC_DEW_CIPHER_MODE_SHIFT                                0
#define PMIC_DEW_CIPHER_SWRST_ADDR                                MT6353_DEW_CIPHER_SWRST
#define PMIC_DEW_CIPHER_SWRST_MASK                                0x1
#define PMIC_DEW_CIPHER_SWRST_SHIFT                               0
#define PMIC_DEW_RDDMY_NO_ADDR                                    MT6353_DEW_RDDMY_NO
#define PMIC_DEW_RDDMY_NO_MASK                                    0xF
#define PMIC_DEW_RDDMY_NO_SHIFT                                   0
#define PMIC_INT_TYPE_CON0_ADDR                                   MT6353_INT_TYPE_CON0
#define PMIC_INT_TYPE_CON0_MASK                                   0xFFFF
#define PMIC_INT_TYPE_CON0_SHIFT                                  0
#define PMIC_INT_TYPE_CON0_SET_ADDR                               MT6353_INT_TYPE_CON0_SET
#define PMIC_INT_TYPE_CON0_SET_MASK                               0xFFFF
#define PMIC_INT_TYPE_CON0_SET_SHIFT                              0
#define PMIC_INT_TYPE_CON0_CLR_ADDR                               MT6353_INT_TYPE_CON0_CLR
#define PMIC_INT_TYPE_CON0_CLR_MASK                               0xFFFF
#define PMIC_INT_TYPE_CON0_CLR_SHIFT                              0
#define PMIC_INT_TYPE_CON1_ADDR                                   MT6353_INT_TYPE_CON1
#define PMIC_INT_TYPE_CON1_MASK                                   0xFFFF
#define PMIC_INT_TYPE_CON1_SHIFT                                  0
#define PMIC_INT_TYPE_CON1_SET_ADDR                               MT6353_INT_TYPE_CON1_SET
#define PMIC_INT_TYPE_CON1_SET_MASK                               0xFFFF
#define PMIC_INT_TYPE_CON1_SET_SHIFT                              0
#define PMIC_INT_TYPE_CON1_CLR_ADDR                               MT6353_INT_TYPE_CON1_CLR
#define PMIC_INT_TYPE_CON1_CLR_MASK                               0xFFFF
#define PMIC_INT_TYPE_CON1_CLR_SHIFT                              0
#define PMIC_INT_TYPE_CON2_ADDR                                   MT6353_INT_TYPE_CON2
#define PMIC_INT_TYPE_CON2_MASK                                   0xFFFF
#define PMIC_INT_TYPE_CON2_SHIFT                                  0
#define PMIC_INT_TYPE_CON2_SET_ADDR                               MT6353_INT_TYPE_CON2_SET
#define PMIC_INT_TYPE_CON2_SET_MASK                               0xFFFF
#define PMIC_INT_TYPE_CON2_SET_SHIFT                              0
#define PMIC_INT_TYPE_CON2_CLR_ADDR                               MT6353_INT_TYPE_CON2_CLR
#define PMIC_INT_TYPE_CON2_CLR_MASK                               0xFFFF
#define PMIC_INT_TYPE_CON2_CLR_SHIFT                              0
#define PMIC_INT_TYPE_CON3_ADDR                                   MT6353_INT_TYPE_CON3
#define PMIC_INT_TYPE_CON3_MASK                                   0x1F
#define PMIC_INT_TYPE_CON3_SHIFT                                  0
#define PMIC_INT_TYPE_CON3_SET_ADDR                               MT6353_INT_TYPE_CON3_SET
#define PMIC_INT_TYPE_CON3_SET_MASK                               0x1F
#define PMIC_INT_TYPE_CON3_SET_SHIFT                              0
#define PMIC_INT_TYPE_CON3_CLR_ADDR                               MT6353_INT_TYPE_CON3_CLR
#define PMIC_INT_TYPE_CON3_CLR_MASK                               0x1F
#define PMIC_INT_TYPE_CON3_CLR_SHIFT                              0
#define PMIC_CPU_INT_STA_ADDR                                     MT6353_INT_STA
#define PMIC_CPU_INT_STA_MASK                                     0x1
#define PMIC_CPU_INT_STA_SHIFT                                    0
#define PMIC_MD32_INT_STA_ADDR                                    MT6353_INT_STA
#define PMIC_MD32_INT_STA_MASK                                    0x1
#define PMIC_MD32_INT_STA_SHIFT                                   1
#define PMIC_BUCK_LDO_FT_TESTMODE_EN_ADDR                         MT6353_BUCK_ALL_CON0
#define PMIC_BUCK_LDO_FT_TESTMODE_EN_MASK                         0x1
#define PMIC_BUCK_LDO_FT_TESTMODE_EN_SHIFT                        0
#define PMIC_EFUSE_VOSEL_LIMIT_ADDR                               MT6353_BUCK_ALL_CON0
#define PMIC_EFUSE_VOSEL_LIMIT_MASK                               0x1
#define PMIC_EFUSE_VOSEL_LIMIT_SHIFT                              1
#define PMIC_EFUSE_OC_EN_SEL_ADDR                                 MT6353_BUCK_ALL_CON0
#define PMIC_EFUSE_OC_EN_SEL_MASK                                 0x1
#define PMIC_EFUSE_OC_EN_SEL_SHIFT                                2
#define PMIC_EFUSE_OC_SDN_EN_ADDR                                 MT6353_BUCK_ALL_CON0
#define PMIC_EFUSE_OC_SDN_EN_MASK                                 0x1
#define PMIC_EFUSE_OC_SDN_EN_SHIFT                                3
#define PMIC_BUCK_FOR_LDO_OSC_SEL_ADDR                            MT6353_BUCK_ALL_CON0
#define PMIC_BUCK_FOR_LDO_OSC_SEL_MASK                            0x7
#define PMIC_BUCK_FOR_LDO_OSC_SEL_SHIFT                           4
#define PMIC_BUCK_ALL_CON0_RSV0_ADDR                              MT6353_BUCK_ALL_CON0
#define PMIC_BUCK_ALL_CON0_RSV0_MASK                              0xFF
#define PMIC_BUCK_ALL_CON0_RSV0_SHIFT                             8
#define PMIC_BUCK_BUCK_RSV_ADDR                                   MT6353_BUCK_ALL_CON1
#define PMIC_BUCK_BUCK_RSV_MASK                                   0xFFFF
#define PMIC_BUCK_BUCK_RSV_SHIFT                                  0
#define PMIC_BUCK_VCORE_VOSEL_CTRL_PROT_ADDR                      MT6353_BUCK_ALL_CON2
#define PMIC_BUCK_VCORE_VOSEL_CTRL_PROT_MASK                      0x1
#define PMIC_BUCK_VCORE_VOSEL_CTRL_PROT_SHIFT                     0
#define PMIC_BUCK_VCORE_VOSEL_CTRL_PROT_EN_ADDR                   MT6353_BUCK_ALL_CON2
#define PMIC_BUCK_VCORE_VOSEL_CTRL_PROT_EN_MASK                   0x1
#define PMIC_BUCK_VCORE_VOSEL_CTRL_PROT_EN_SHIFT                  1
#define PMIC_BUCK_ALL_CON2_RSV0_ADDR                              MT6353_BUCK_ALL_CON2
#define PMIC_BUCK_ALL_CON2_RSV0_MASK                              0x1F
#define PMIC_BUCK_ALL_CON2_RSV0_SHIFT                             11
#define PMIC_BUCK_VSLEEP_SRC0_ADDR                                MT6353_BUCK_ALL_CON3
#define PMIC_BUCK_VSLEEP_SRC0_MASK                                0x1FF
#define PMIC_BUCK_VSLEEP_SRC0_SHIFT                               0
#define PMIC_BUCK_VSLEEP_SRC1_ADDR                                MT6353_BUCK_ALL_CON3
#define PMIC_BUCK_VSLEEP_SRC1_MASK                                0xF
#define PMIC_BUCK_VSLEEP_SRC1_SHIFT                               12
#define PMIC_BUCK_R2R_SRC0_ADDR                                   MT6353_BUCK_ALL_CON4
#define PMIC_BUCK_R2R_SRC0_MASK                                   0x1FF
#define PMIC_BUCK_R2R_SRC0_SHIFT                                  0
#define PMIC_BUCK_R2R_SRC1_ADDR                                   MT6353_BUCK_ALL_CON4
#define PMIC_BUCK_R2R_SRC1_MASK                                   0xF
#define PMIC_BUCK_R2R_SRC1_SHIFT                                  12
#define PMIC_BUCK_BUCK_OSC_SEL_SRC0_ADDR                          MT6353_BUCK_ALL_CON5
#define PMIC_BUCK_BUCK_OSC_SEL_SRC0_MASK                          0x1FF
#define PMIC_BUCK_BUCK_OSC_SEL_SRC0_SHIFT                         0
#define PMIC_BUCK_SRCLKEN_DLY_SRC1_ADDR                           MT6353_BUCK_ALL_CON5
#define PMIC_BUCK_SRCLKEN_DLY_SRC1_MASK                           0xF
#define PMIC_BUCK_SRCLKEN_DLY_SRC1_SHIFT                          12
#define PMIC_BUCK_VPA_VOSEL_DLC011_ADDR                           MT6353_BUCK_DLC_VPA_CON0
#define PMIC_BUCK_VPA_VOSEL_DLC011_MASK                           0x3F
#define PMIC_BUCK_VPA_VOSEL_DLC011_SHIFT                          0
#define PMIC_BUCK_VPA_VOSEL_DLC111_ADDR                           MT6353_BUCK_DLC_VPA_CON0
#define PMIC_BUCK_VPA_VOSEL_DLC111_MASK                           0x3F
#define PMIC_BUCK_VPA_VOSEL_DLC111_SHIFT                          8
#define PMIC_BUCK_VPA_DLC_MAP_EN_ADDR                             MT6353_BUCK_DLC_VPA_CON1
#define PMIC_BUCK_VPA_DLC_MAP_EN_MASK                             0x1
#define PMIC_BUCK_VPA_DLC_MAP_EN_SHIFT                            0
#define PMIC_BUCK_VPA_VOSEL_DLC001_ADDR                           MT6353_BUCK_DLC_VPA_CON1
#define PMIC_BUCK_VPA_VOSEL_DLC001_MASK                           0x3F
#define PMIC_BUCK_VPA_VOSEL_DLC001_SHIFT                          8
#define PMIC_BUCK_VPA_DLC_ADDR                                    MT6353_BUCK_DLC_VPA_CON2
#define PMIC_BUCK_VPA_DLC_MASK                                    0x7
#define PMIC_BUCK_VPA_DLC_SHIFT                                   0
#define PMIC_DA_NI_VPA_DLC_ADDR                                   MT6353_BUCK_DLC_VPA_CON2
#define PMIC_DA_NI_VPA_DLC_MASK                                   0x7
#define PMIC_DA_NI_VPA_DLC_SHIFT                                  12
#define PMIC_BUCK_VSRAM_PROC_TRACK_SLEEP_CTRL_ADDR                MT6353_BUCK_TRACKING_CON0
#define PMIC_BUCK_VSRAM_PROC_TRACK_SLEEP_CTRL_MASK                0x1
#define PMIC_BUCK_VSRAM_PROC_TRACK_SLEEP_CTRL_SHIFT               0
#define PMIC_BUCK_VSRAM_PROC_TRACK_ON_CTRL_ADDR                   MT6353_BUCK_TRACKING_CON0
#define PMIC_BUCK_VSRAM_PROC_TRACK_ON_CTRL_MASK                   0x1
#define PMIC_BUCK_VSRAM_PROC_TRACK_ON_CTRL_SHIFT                  1
#define PMIC_BUCK_VPROC_TRACK_ON_CTRL_ADDR                        MT6353_BUCK_TRACKING_CON0
#define PMIC_BUCK_VPROC_TRACK_ON_CTRL_MASK                        0x1
#define PMIC_BUCK_VPROC_TRACK_ON_CTRL_SHIFT                       2
#define PMIC_BUCK_VSRAM_PROC_VOSEL_DELTA_ADDR                     MT6353_BUCK_TRACKING_CON1
#define PMIC_BUCK_VSRAM_PROC_VOSEL_DELTA_MASK                     0x7F
#define PMIC_BUCK_VSRAM_PROC_VOSEL_DELTA_SHIFT                    0
#define PMIC_BUCK_VSRAM_PROC_VOSEL_OFFSET_ADDR                    MT6353_BUCK_TRACKING_CON1
#define PMIC_BUCK_VSRAM_PROC_VOSEL_OFFSET_MASK                    0x7F
#define PMIC_BUCK_VSRAM_PROC_VOSEL_OFFSET_SHIFT                   8
#define PMIC_BUCK_VSRAM_PROC_VOSEL_ON_LB_ADDR                     MT6353_BUCK_TRACKING_CON2
#define PMIC_BUCK_VSRAM_PROC_VOSEL_ON_LB_MASK                     0x7F
#define PMIC_BUCK_VSRAM_PROC_VOSEL_ON_LB_SHIFT                    0
#define PMIC_BUCK_VSRAM_PROC_VOSEL_ON_HB_ADDR                     MT6353_BUCK_TRACKING_CON2
#define PMIC_BUCK_VSRAM_PROC_VOSEL_ON_HB_MASK                     0x7F
#define PMIC_BUCK_VSRAM_PROC_VOSEL_ON_HB_SHIFT                    8
#define PMIC_BUCK_VSRAM_PROC_VOSEL_SLEEP_LB_ADDR                  MT6353_BUCK_TRACKING_CON3
#define PMIC_BUCK_VSRAM_PROC_VOSEL_SLEEP_LB_MASK                  0x7F
#define PMIC_BUCK_VSRAM_PROC_VOSEL_SLEEP_LB_SHIFT                 0
#define PMIC_AD_QI_VCORE_OC_STATUS_ADDR                           MT6353_BUCK_ANA_MON_CON0
#define PMIC_AD_QI_VCORE_OC_STATUS_MASK                           0x1
#define PMIC_AD_QI_VCORE_OC_STATUS_SHIFT                          0
#define PMIC_AD_QI_VPROC_OC_STATUS_ADDR                           MT6353_BUCK_ANA_MON_CON0
#define PMIC_AD_QI_VPROC_OC_STATUS_MASK                           0x1
#define PMIC_AD_QI_VPROC_OC_STATUS_SHIFT                          1
#define PMIC_AD_QI_VS1_OC_STATUS_ADDR                             MT6353_BUCK_ANA_MON_CON0
#define PMIC_AD_QI_VS1_OC_STATUS_MASK                             0x1
#define PMIC_AD_QI_VS1_OC_STATUS_SHIFT                            2
#define PMIC_AD_QI_VPA_OC_STATUS_ADDR                             MT6353_BUCK_ANA_MON_CON0
#define PMIC_AD_QI_VPA_OC_STATUS_MASK                             0x1
#define PMIC_AD_QI_VPA_OC_STATUS_SHIFT                            3
#define PMIC_BUCK_ANA_STATUS_ADDR                                 MT6353_BUCK_ANA_MON_CON0
#define PMIC_BUCK_ANA_STATUS_MASK                                 0xF
#define PMIC_BUCK_ANA_STATUS_SHIFT                                12
#define PMIC_AD_QI_VCORE_DIG_MON_ADDR                             MT6353_BUCK_ANA_MON_CON1
#define PMIC_AD_QI_VCORE_DIG_MON_MASK                             0xF
#define PMIC_AD_QI_VCORE_DIG_MON_SHIFT                            0
#define PMIC_AD_QI_VPROC_DIG_MON_ADDR                             MT6353_BUCK_ANA_MON_CON1
#define PMIC_AD_QI_VPROC_DIG_MON_MASK                             0xF
#define PMIC_AD_QI_VPROC_DIG_MON_SHIFT                            4
#define PMIC_AD_QI_VS1_DIG_MON_ADDR                               MT6353_BUCK_ANA_MON_CON1
#define PMIC_AD_QI_VS1_DIG_MON_MASK                               0xF
#define PMIC_AD_QI_VS1_DIG_MON_SHIFT                              8
#define PMIC_BUCK_VCORE_OC_STATUS_ADDR                            MT6353_BUCK_OC_CON0
#define PMIC_BUCK_VCORE_OC_STATUS_MASK                            0x1
#define PMIC_BUCK_VCORE_OC_STATUS_SHIFT                           0
#define PMIC_BUCK_VCORE2_OC_STATUS_ADDR                           MT6353_BUCK_OC_CON0
#define PMIC_BUCK_VCORE2_OC_STATUS_MASK                           0x1
#define PMIC_BUCK_VCORE2_OC_STATUS_SHIFT                          1
#define PMIC_BUCK_VPROC_OC_STATUS_ADDR                            MT6353_BUCK_OC_CON0
#define PMIC_BUCK_VPROC_OC_STATUS_MASK                            0x1
#define PMIC_BUCK_VPROC_OC_STATUS_SHIFT                           2
#define PMIC_BUCK_VS1_OC_STATUS_ADDR                              MT6353_BUCK_OC_CON0
#define PMIC_BUCK_VS1_OC_STATUS_MASK                              0x1
#define PMIC_BUCK_VS1_OC_STATUS_SHIFT                             3
#define PMIC_BUCK_VPA_OC_STATUS_ADDR                              MT6353_BUCK_OC_CON0
#define PMIC_BUCK_VPA_OC_STATUS_MASK                              0x1
#define PMIC_BUCK_VPA_OC_STATUS_SHIFT                             4
#define PMIC_BUCK_VCORE_OC_INT_EN_ADDR                            MT6353_BUCK_OC_CON1
#define PMIC_BUCK_VCORE_OC_INT_EN_MASK                            0x1
#define PMIC_BUCK_VCORE_OC_INT_EN_SHIFT                           0
#define PMIC_BUCK_VCORE2_OC_INT_EN_ADDR                           MT6353_BUCK_OC_CON1
#define PMIC_BUCK_VCORE2_OC_INT_EN_MASK                           0x1
#define PMIC_BUCK_VCORE2_OC_INT_EN_SHIFT                          1
#define PMIC_BUCK_VPROC_OC_INT_EN_ADDR                            MT6353_BUCK_OC_CON1
#define PMIC_BUCK_VPROC_OC_INT_EN_MASK                            0x1
#define PMIC_BUCK_VPROC_OC_INT_EN_SHIFT                           2
#define PMIC_BUCK_VS1_OC_INT_EN_ADDR                              MT6353_BUCK_OC_CON1
#define PMIC_BUCK_VS1_OC_INT_EN_MASK                              0x1
#define PMIC_BUCK_VS1_OC_INT_EN_SHIFT                             3
#define PMIC_BUCK_VPA_OC_INT_EN_ADDR                              MT6353_BUCK_OC_CON1
#define PMIC_BUCK_VPA_OC_INT_EN_MASK                              0x1
#define PMIC_BUCK_VPA_OC_INT_EN_SHIFT                             4
#define PMIC_BUCK_VCORE_EN_OC_SDN_SEL_ADDR                        MT6353_BUCK_OC_CON2
#define PMIC_BUCK_VCORE_EN_OC_SDN_SEL_MASK                        0x1
#define PMIC_BUCK_VCORE_EN_OC_SDN_SEL_SHIFT                       0
#define PMIC_BUCK_VCORE2_EN_OC_SDN_SEL_ADDR                       MT6353_BUCK_OC_CON2
#define PMIC_BUCK_VCORE2_EN_OC_SDN_SEL_MASK                       0x1
#define PMIC_BUCK_VCORE2_EN_OC_SDN_SEL_SHIFT                      1
#define PMIC_BUCK_VPROC_EN_OC_SDN_SEL_ADDR                        MT6353_BUCK_OC_CON2
#define PMIC_BUCK_VPROC_EN_OC_SDN_SEL_MASK                        0x1
#define PMIC_BUCK_VPROC_EN_OC_SDN_SEL_SHIFT                       2
#define PMIC_BUCK_VS1_EN_OC_SDN_SEL_ADDR                          MT6353_BUCK_OC_CON2
#define PMIC_BUCK_VS1_EN_OC_SDN_SEL_MASK                          0x1
#define PMIC_BUCK_VS1_EN_OC_SDN_SEL_SHIFT                         3
#define PMIC_BUCK_VPA_EN_OC_SDN_SEL_ADDR                          MT6353_BUCK_OC_CON2
#define PMIC_BUCK_VPA_EN_OC_SDN_SEL_MASK                          0x1
#define PMIC_BUCK_VPA_EN_OC_SDN_SEL_SHIFT                         4
#define PMIC_BUCK_VCORE_OC_FLAG_CLR_ADDR                          MT6353_BUCK_OC_CON3
#define PMIC_BUCK_VCORE_OC_FLAG_CLR_MASK                          0x1
#define PMIC_BUCK_VCORE_OC_FLAG_CLR_SHIFT                         0
#define PMIC_BUCK_VCORE2_OC_FLAG_CLR_ADDR                         MT6353_BUCK_OC_CON3
#define PMIC_BUCK_VCORE2_OC_FLAG_CLR_MASK                         0x1
#define PMIC_BUCK_VCORE2_OC_FLAG_CLR_SHIFT                        1
#define PMIC_BUCK_VPROC_OC_FLAG_CLR_ADDR                          MT6353_BUCK_OC_CON3
#define PMIC_BUCK_VPROC_OC_FLAG_CLR_MASK                          0x1
#define PMIC_BUCK_VPROC_OC_FLAG_CLR_SHIFT                         2
#define PMIC_BUCK_VS1_OC_FLAG_CLR_ADDR                            MT6353_BUCK_OC_CON3
#define PMIC_BUCK_VS1_OC_FLAG_CLR_MASK                            0x1
#define PMIC_BUCK_VS1_OC_FLAG_CLR_SHIFT                           3
#define PMIC_BUCK_VPA_OC_FLAG_CLR_ADDR                            MT6353_BUCK_OC_CON3
#define PMIC_BUCK_VPA_OC_FLAG_CLR_MASK                            0x1
#define PMIC_BUCK_VPA_OC_FLAG_CLR_SHIFT                           4
#define PMIC_BUCK_VCORE_OC_FLAG_CLR_SEL_ADDR                      MT6353_BUCK_OC_CON4
#define PMIC_BUCK_VCORE_OC_FLAG_CLR_SEL_MASK                      0x1
#define PMIC_BUCK_VCORE_OC_FLAG_CLR_SEL_SHIFT                     0
#define PMIC_BUCK_VCORE2_OC_FLAG_CLR_SEL_ADDR                     MT6353_BUCK_OC_CON4
#define PMIC_BUCK_VCORE2_OC_FLAG_CLR_SEL_MASK                     0x1
#define PMIC_BUCK_VCORE2_OC_FLAG_CLR_SEL_SHIFT                    1
#define PMIC_BUCK_VPROC_OC_FLAG_CLR_SEL_ADDR                      MT6353_BUCK_OC_CON4
#define PMIC_BUCK_VPROC_OC_FLAG_CLR_SEL_MASK                      0x1
#define PMIC_BUCK_VPROC_OC_FLAG_CLR_SEL_SHIFT                     2
#define PMIC_BUCK_VS1_OC_FLAG_CLR_SEL_ADDR                        MT6353_BUCK_OC_CON4
#define PMIC_BUCK_VS1_OC_FLAG_CLR_SEL_MASK                        0x1
#define PMIC_BUCK_VS1_OC_FLAG_CLR_SEL_SHIFT                       3
#define PMIC_BUCK_VPA_OC_FLAG_CLR_SEL_ADDR                        MT6353_BUCK_OC_CON4
#define PMIC_BUCK_VPA_OC_FLAG_CLR_SEL_MASK                        0x1
#define PMIC_BUCK_VPA_OC_FLAG_CLR_SEL_SHIFT                       4
#define PMIC_BUCK_VCORE_OC_DEG_EN_ADDR                            MT6353_BUCK_OC_VCORE_CON0
#define PMIC_BUCK_VCORE_OC_DEG_EN_MASK                            0x1
#define PMIC_BUCK_VCORE_OC_DEG_EN_SHIFT                           1
#define PMIC_BUCK_VCORE_OC_WND_ADDR                               MT6353_BUCK_OC_VCORE_CON0
#define PMIC_BUCK_VCORE_OC_WND_MASK                               0x3
#define PMIC_BUCK_VCORE_OC_WND_SHIFT                              2
#define PMIC_BUCK_VCORE_OC_THD_ADDR                               MT6353_BUCK_OC_VCORE_CON0
#define PMIC_BUCK_VCORE_OC_THD_MASK                               0x3
#define PMIC_BUCK_VCORE_OC_THD_SHIFT                              6
#define PMIC_BUCK_VCORE2_OC_DEG_EN_ADDR                           MT6353_BUCK_OC_VCORE2_CON0
#define PMIC_BUCK_VCORE2_OC_DEG_EN_MASK                           0x1
#define PMIC_BUCK_VCORE2_OC_DEG_EN_SHIFT                          1
#define PMIC_BUCK_VCORE2_OC_WND_ADDR                              MT6353_BUCK_OC_VCORE2_CON0
#define PMIC_BUCK_VCORE2_OC_WND_MASK                              0x3
#define PMIC_BUCK_VCORE2_OC_WND_SHIFT                             2
#define PMIC_BUCK_VCORE2_OC_THD_ADDR                              MT6353_BUCK_OC_VCORE2_CON0
#define PMIC_BUCK_VCORE2_OC_THD_MASK                              0x3
#define PMIC_BUCK_VCORE2_OC_THD_SHIFT                             6
#define PMIC_BUCK_VPROC_OC_DEG_EN_ADDR                            MT6353_BUCK_OC_VPROC_CON0
#define PMIC_BUCK_VPROC_OC_DEG_EN_MASK                            0x1
#define PMIC_BUCK_VPROC_OC_DEG_EN_SHIFT                           1
#define PMIC_BUCK_VPROC_OC_WND_ADDR                               MT6353_BUCK_OC_VPROC_CON0
#define PMIC_BUCK_VPROC_OC_WND_MASK                               0x3
#define PMIC_BUCK_VPROC_OC_WND_SHIFT                              2
#define PMIC_BUCK_VPROC_OC_THD_ADDR                               MT6353_BUCK_OC_VPROC_CON0
#define PMIC_BUCK_VPROC_OC_THD_MASK                               0x3
#define PMIC_BUCK_VPROC_OC_THD_SHIFT                              6
#define PMIC_BUCK_VS1_OC_DEG_EN_ADDR                              MT6353_BUCK_OC_VS1_CON0
#define PMIC_BUCK_VS1_OC_DEG_EN_MASK                              0x1
#define PMIC_BUCK_VS1_OC_DEG_EN_SHIFT                             1
#define PMIC_BUCK_VS1_OC_WND_ADDR                                 MT6353_BUCK_OC_VS1_CON0
#define PMIC_BUCK_VS1_OC_WND_MASK                                 0x3
#define PMIC_BUCK_VS1_OC_WND_SHIFT                                2
#define PMIC_BUCK_VS1_OC_THD_ADDR                                 MT6353_BUCK_OC_VS1_CON0
#define PMIC_BUCK_VS1_OC_THD_MASK                                 0x3
#define PMIC_BUCK_VS1_OC_THD_SHIFT                                6
#define PMIC_BUCK_VPA_OC_DEG_EN_ADDR                              MT6353_BUCK_OC_VPA_CON0
#define PMIC_BUCK_VPA_OC_DEG_EN_MASK                              0x1
#define PMIC_BUCK_VPA_OC_DEG_EN_SHIFT                             1
#define PMIC_BUCK_VPA_OC_WND_ADDR                                 MT6353_BUCK_OC_VPA_CON0
#define PMIC_BUCK_VPA_OC_WND_MASK                                 0x3
#define PMIC_BUCK_VPA_OC_WND_SHIFT                                2
#define PMIC_BUCK_VPA_OC_THD_ADDR                                 MT6353_BUCK_OC_VPA_CON0
#define PMIC_BUCK_VPA_OC_THD_MASK                                 0x3
#define PMIC_BUCK_VPA_OC_THD_SHIFT                                6
#define PMIC_RG_SMPS_TESTMODE_B_ADDR                              MT6353_SMPS_TOP_ANA_CON0
#define PMIC_RG_SMPS_TESTMODE_B_MASK                              0x1FF
#define PMIC_RG_SMPS_TESTMODE_B_SHIFT                             0
#define PMIC_RG_VSRAM_PROC_TRIMH_ADDR                             MT6353_SMPS_TOP_ANA_CON0
#define PMIC_RG_VSRAM_PROC_TRIMH_MASK                             0x1F
#define PMIC_RG_VSRAM_PROC_TRIMH_SHIFT                            9
#define PMIC_RG_VSRAM_PROC_TRIML_ADDR                             MT6353_SMPS_TOP_ANA_CON1
#define PMIC_RG_VSRAM_PROC_TRIML_MASK                             0x1F
#define PMIC_RG_VSRAM_PROC_TRIML_SHIFT                            0
#define PMIC_RG_VPROC_TRIMH_ADDR                                  MT6353_SMPS_TOP_ANA_CON1
#define PMIC_RG_VPROC_TRIMH_MASK                                  0x1F
#define PMIC_RG_VPROC_TRIMH_SHIFT                                 5
#define PMIC_RG_VPROC_TRIML_ADDR                                  MT6353_SMPS_TOP_ANA_CON1
#define PMIC_RG_VPROC_TRIML_MASK                                  0x1F
#define PMIC_RG_VPROC_TRIML_SHIFT                                 10
#define PMIC_RG_VCORE_TRIMH_ADDR                                  MT6353_SMPS_TOP_ANA_CON2
#define PMIC_RG_VCORE_TRIMH_MASK                                  0x1F
#define PMIC_RG_VCORE_TRIMH_SHIFT                                 0
#define PMIC_RG_VCORE_TRIML_ADDR                                  MT6353_SMPS_TOP_ANA_CON2
#define PMIC_RG_VCORE_TRIML_MASK                                  0x1F
#define PMIC_RG_VCORE_TRIML_SHIFT                                 5
#define PMIC_RG_VCORE2_TRIMH_ADDR                                 MT6353_SMPS_TOP_ANA_CON2
#define PMIC_RG_VCORE2_TRIMH_MASK                                 0x1F
#define PMIC_RG_VCORE2_TRIMH_SHIFT                                10
#define PMIC_RG_VCORE2_TRIML_ADDR                                 MT6353_SMPS_TOP_ANA_CON3
#define PMIC_RG_VCORE2_TRIML_MASK                                 0x1F
#define PMIC_RG_VCORE2_TRIML_SHIFT                                0
#define PMIC_RG_VS1_TRIMH_ADDR                                    MT6353_SMPS_TOP_ANA_CON3
#define PMIC_RG_VS1_TRIMH_MASK                                    0xF
#define PMIC_RG_VS1_TRIMH_SHIFT                                   5
#define PMIC_RG_VS1_TRIML_ADDR                                    MT6353_SMPS_TOP_ANA_CON3
#define PMIC_RG_VS1_TRIML_MASK                                    0xF
#define PMIC_RG_VS1_TRIML_SHIFT                                   9
#define PMIC_RG_VPA_TRIMH_ADDR                                    MT6353_SMPS_TOP_ANA_CON4
#define PMIC_RG_VPA_TRIMH_MASK                                    0x1F
#define PMIC_RG_VPA_TRIMH_SHIFT                                   0
#define PMIC_RG_VPA_TRIML_ADDR                                    MT6353_SMPS_TOP_ANA_CON4
#define PMIC_RG_VPA_TRIML_MASK                                    0x1F
#define PMIC_RG_VPA_TRIML_SHIFT                                   5
#define PMIC_RG_VPA_TRIM_REF_ADDR                                 MT6353_SMPS_TOP_ANA_CON4
#define PMIC_RG_VPA_TRIM_REF_MASK                                 0x1F
#define PMIC_RG_VPA_TRIM_REF_SHIFT                                10
#define PMIC_RG_VSRAM_PROC_VSLEEP_ADDR                            MT6353_SMPS_TOP_ANA_CON5
#define PMIC_RG_VSRAM_PROC_VSLEEP_MASK                            0x7
#define PMIC_RG_VSRAM_PROC_VSLEEP_SHIFT                           0
#define PMIC_RG_VPROC_VSLEEP_ADDR                                 MT6353_SMPS_TOP_ANA_CON5
#define PMIC_RG_VPROC_VSLEEP_MASK                                 0x7
#define PMIC_RG_VPROC_VSLEEP_SHIFT                                3
#define PMIC_RG_VCORE_VSLEEP_ADDR                                 MT6353_SMPS_TOP_ANA_CON5
#define PMIC_RG_VCORE_VSLEEP_MASK                                 0x7
#define PMIC_RG_VCORE_VSLEEP_SHIFT                                6
#define PMIC_RG_VCORE2_VSLEEP_ADDR                                MT6353_SMPS_TOP_ANA_CON5
#define PMIC_RG_VCORE2_VSLEEP_MASK                                0x7
#define PMIC_RG_VCORE2_VSLEEP_SHIFT                               9
#define PMIC_RG_VPA_BURSTH_ADDR                                   MT6353_SMPS_TOP_ANA_CON5
#define PMIC_RG_VPA_BURSTH_MASK                                   0x3
#define PMIC_RG_VPA_BURSTH_SHIFT                                  12
#define PMIC_RG_VPA_BURSTL_ADDR                                   MT6353_SMPS_TOP_ANA_CON5
#define PMIC_RG_VPA_BURSTL_MASK                                   0x3
#define PMIC_RG_VPA_BURSTL_SHIFT                                  14
#define PMIC_RG_DMY100MA_EN_ADDR                                  MT6353_SMPS_TOP_ANA_CON6
#define PMIC_RG_DMY100MA_EN_MASK                                  0x1
#define PMIC_RG_DMY100MA_EN_SHIFT                                 0
#define PMIC_RG_DMY100MA_SEL_ADDR                                 MT6353_SMPS_TOP_ANA_CON6
#define PMIC_RG_DMY100MA_SEL_MASK                                 0x3
#define PMIC_RG_DMY100MA_SEL_SHIFT                                1
#define PMIC_RG_VSRAM_PROC_VSLEEP_VOLTAGE_ADDR                    MT6353_SMPS_TOP_ANA_CON6
#define PMIC_RG_VSRAM_PROC_VSLEEP_VOLTAGE_MASK                    0x3
#define PMIC_RG_VSRAM_PROC_VSLEEP_VOLTAGE_SHIFT                   3
#define PMIC_RG_VPROC_VSLEEP_VOLTAGE_ADDR                         MT6353_SMPS_TOP_ANA_CON6
#define PMIC_RG_VPROC_VSLEEP_VOLTAGE_MASK                         0x3
#define PMIC_RG_VPROC_VSLEEP_VOLTAGE_SHIFT                        5
#define PMIC_RG_VCORE_VSLEEP_VOLTAGE_ADDR                         MT6353_SMPS_TOP_ANA_CON6
#define PMIC_RG_VCORE_VSLEEP_VOLTAGE_MASK                         0x3
#define PMIC_RG_VCORE_VSLEEP_VOLTAGE_SHIFT                        7
#define PMIC_RG_VCORE2_VSLEEP_VOLTAGE_ADDR                        MT6353_SMPS_TOP_ANA_CON6
#define PMIC_RG_VCORE2_VSLEEP_VOLTAGE_MASK                        0x3
#define PMIC_RG_VCORE2_VSLEEP_VOLTAGE_SHIFT                       9
#define PMIC_RG_VPA_RZSEL_ADDR                                    MT6353_VPA_ANA_CON0
#define PMIC_RG_VPA_RZSEL_MASK                                    0x3
#define PMIC_RG_VPA_RZSEL_SHIFT                                   0
#define PMIC_RG_VPA_CC_ADDR                                       MT6353_VPA_ANA_CON0
#define PMIC_RG_VPA_CC_MASK                                       0x3
#define PMIC_RG_VPA_CC_SHIFT                                      2
#define PMIC_RG_VPA_CSR_ADDR                                      MT6353_VPA_ANA_CON0
#define PMIC_RG_VPA_CSR_MASK                                      0x3
#define PMIC_RG_VPA_CSR_SHIFT                                     4
#define PMIC_RG_VPA_CSMIR_ADDR                                    MT6353_VPA_ANA_CON0
#define PMIC_RG_VPA_CSMIR_MASK                                    0x3
#define PMIC_RG_VPA_CSMIR_SHIFT                                   6
#define PMIC_RG_VPA_CSL_ADDR                                      MT6353_VPA_ANA_CON0
#define PMIC_RG_VPA_CSL_MASK                                      0x3
#define PMIC_RG_VPA_CSL_SHIFT                                     8
#define PMIC_RG_VPA_SLP_ADDR                                      MT6353_VPA_ANA_CON0
#define PMIC_RG_VPA_SLP_MASK                                      0x3
#define PMIC_RG_VPA_SLP_SHIFT                                     10
#define PMIC_RG_VPA_ZX_OS_TRIM_ADDR                               MT6353_VPA_ANA_CON1
#define PMIC_RG_VPA_ZX_OS_TRIM_MASK                               0x3F
#define PMIC_RG_VPA_ZX_OS_TRIM_SHIFT                              0
#define PMIC_RG_VPA_ZX_OS_ADDR                                    MT6353_VPA_ANA_CON1
#define PMIC_RG_VPA_ZX_OS_MASK                                    0x3
#define PMIC_RG_VPA_ZX_OS_SHIFT                                   6
#define PMIC_RG_VPA_HZP_ADDR                                      MT6353_VPA_ANA_CON1
#define PMIC_RG_VPA_HZP_MASK                                      0x1
#define PMIC_RG_VPA_HZP_SHIFT                                     8
#define PMIC_RG_VPA_BWEX_GAT_ADDR                                 MT6353_VPA_ANA_CON1
#define PMIC_RG_VPA_BWEX_GAT_MASK                                 0x1
#define PMIC_RG_VPA_BWEX_GAT_SHIFT                                9
#define PMIC_RG_VPA_MODESET_ADDR                                  MT6353_VPA_ANA_CON1
#define PMIC_RG_VPA_MODESET_MASK                                  0x1
#define PMIC_RG_VPA_MODESET_SHIFT                                 10
#define PMIC_RG_VPA_SLEW_ADDR                                     MT6353_VPA_ANA_CON1
#define PMIC_RG_VPA_SLEW_MASK                                     0x3
#define PMIC_RG_VPA_SLEW_SHIFT                                    11
#define PMIC_RG_VPA_SLEW_NMOS_ADDR                                MT6353_VPA_ANA_CON1
#define PMIC_RG_VPA_SLEW_NMOS_MASK                                0x3
#define PMIC_RG_VPA_SLEW_NMOS_SHIFT                               13
#define PMIC_RG_VPA_NDIS_EN_ADDR                                  MT6353_VPA_ANA_CON1
#define PMIC_RG_VPA_NDIS_EN_MASK                                  0x1
#define PMIC_RG_VPA_NDIS_EN_SHIFT                                 15
#define PMIC_RG_VPA_MIN_ON_ADDR                                   MT6353_VPA_ANA_CON2
#define PMIC_RG_VPA_MIN_ON_MASK                                   0x3
#define PMIC_RG_VPA_MIN_ON_SHIFT                                  0
#define PMIC_RG_VPA_VBAT_DEL_ADDR                                 MT6353_VPA_ANA_CON2
#define PMIC_RG_VPA_VBAT_DEL_MASK                                 0x3
#define PMIC_RG_VPA_VBAT_DEL_SHIFT                                2
#define PMIC_RG_VPA_RSV1_ADDR                                     MT6353_VPA_ANA_CON2
#define PMIC_RG_VPA_RSV1_MASK                                     0xFF
#define PMIC_RG_VPA_RSV1_SHIFT                                    5
#define PMIC_RG_VPA_RSV2_ADDR                                     MT6353_VPA_ANA_CON3
#define PMIC_RG_VPA_RSV2_MASK                                     0xFF
#define PMIC_RG_VPA_RSV2_SHIFT                                    0
#define PMIC_RGS_VPA_ZX_STATUS_ADDR                               MT6353_VPA_ANA_CON3
#define PMIC_RGS_VPA_ZX_STATUS_MASK                               0x1
#define PMIC_RGS_VPA_ZX_STATUS_SHIFT                              8
#define PMIC_RG_VCORE_MIN_OFF_ADDR                                MT6353_VCORE_ANA_CON0
#define PMIC_RG_VCORE_MIN_OFF_MASK                                0x3
#define PMIC_RG_VCORE_MIN_OFF_SHIFT                               0
#define PMIC_RG_VCORE_VRF18_SSTART_EN_ADDR                        MT6353_VCORE_ANA_CON0
#define PMIC_RG_VCORE_VRF18_SSTART_EN_MASK                        0x1
#define PMIC_RG_VCORE_VRF18_SSTART_EN_SHIFT                       2
#define PMIC_RG_VCORE_1P35UP_SEL_EN_ADDR                          MT6353_VCORE_ANA_CON0
#define PMIC_RG_VCORE_1P35UP_SEL_EN_MASK                          0x1
#define PMIC_RG_VCORE_1P35UP_SEL_EN_SHIFT                         3
#define PMIC_RG_VCORE_RZSEL_ADDR                                  MT6353_VCORE_ANA_CON0
#define PMIC_RG_VCORE_RZSEL_MASK                                  0x7
#define PMIC_RG_VCORE_RZSEL_SHIFT                                 4
#define PMIC_RG_VCORE_CSR_ADDR                                    MT6353_VCORE_ANA_CON0
#define PMIC_RG_VCORE_CSR_MASK                                    0x7
#define PMIC_RG_VCORE_CSR_SHIFT                                   7
#define PMIC_RG_VCORE_CSL_ADDR                                    MT6353_VCORE_ANA_CON0
#define PMIC_RG_VCORE_CSL_MASK                                    0xF
#define PMIC_RG_VCORE_CSL_SHIFT                                   10
#define PMIC_RG_VCORE_SLP_ADDR                                    MT6353_VCORE_ANA_CON1
#define PMIC_RG_VCORE_SLP_MASK                                    0x7
#define PMIC_RG_VCORE_SLP_SHIFT                                   0
#define PMIC_RG_VCORE_ZX_OS_ADDR                                  MT6353_VCORE_ANA_CON1
#define PMIC_RG_VCORE_ZX_OS_MASK                                  0x3
#define PMIC_RG_VCORE_ZX_OS_SHIFT                                 3
#define PMIC_RG_VCORE_ZXOS_TRIM_ADDR                              MT6353_VCORE_ANA_CON1
#define PMIC_RG_VCORE_ZXOS_TRIM_MASK                              0x3F
#define PMIC_RG_VCORE_ZXOS_TRIM_SHIFT                             5
#define PMIC_RG_VCORE_MODESET_ADDR                                MT6353_VCORE_ANA_CON1
#define PMIC_RG_VCORE_MODESET_MASK                                0x1
#define PMIC_RG_VCORE_MODESET_SHIFT                               11
#define PMIC_RG_VCORE_NDIS_EN_ADDR                                MT6353_VCORE_ANA_CON1
#define PMIC_RG_VCORE_NDIS_EN_MASK                                0x1
#define PMIC_RG_VCORE_NDIS_EN_SHIFT                               12
#define PMIC_RG_VCORE_CSM_N_ADDR                                  MT6353_VCORE_ANA_CON2
#define PMIC_RG_VCORE_CSM_N_MASK                                  0x3F
#define PMIC_RG_VCORE_CSM_N_SHIFT                                 0
#define PMIC_RG_VCORE_CSM_P_ADDR                                  MT6353_VCORE_ANA_CON2
#define PMIC_RG_VCORE_CSM_P_MASK                                  0x3F
#define PMIC_RG_VCORE_CSM_P_SHIFT                                 6
#define PMIC_RG_VCORE_RSV_ADDR                                    MT6353_VCORE_ANA_CON3
#define PMIC_RG_VCORE_RSV_MASK                                    0xFF
#define PMIC_RG_VCORE_RSV_SHIFT                                   0
#define PMIC_RG_VCORE_PFM_RIP_ADDR                                MT6353_VCORE_ANA_CON3
#define PMIC_RG_VCORE_PFM_RIP_MASK                                0x7
#define PMIC_RG_VCORE_PFM_RIP_SHIFT                               8
#define PMIC_RG_VCORE_DTS_ENB_ADDR                                MT6353_VCORE_ANA_CON3
#define PMIC_RG_VCORE_DTS_ENB_MASK                                0x1
#define PMIC_RG_VCORE_DTS_ENB_SHIFT                               11
#define PMIC_RG_VCORE_AUTO_MODE_ADDR                              MT6353_VCORE_ANA_CON3
#define PMIC_RG_VCORE_AUTO_MODE_MASK                              0x1
#define PMIC_RG_VCORE_AUTO_MODE_SHIFT                             12
#define PMIC_RG_VCORE_PWM_TRIG_ADDR                               MT6353_VCORE_ANA_CON3
#define PMIC_RG_VCORE_PWM_TRIG_MASK                               0x1
#define PMIC_RG_VCORE_PWM_TRIG_SHIFT                              13
#define PMIC_RG_VCORE_TRAN_BST_ADDR                               MT6353_VCORE_ANA_CON4
#define PMIC_RG_VCORE_TRAN_BST_MASK                               0x3F
#define PMIC_RG_VCORE_TRAN_BST_SHIFT                              0
#define PMIC_RGS_VCORE_ENPWM_STATUS_ADDR                          MT6353_VCORE_ANA_CON4
#define PMIC_RGS_VCORE_ENPWM_STATUS_MASK                          0x1
#define PMIC_RGS_VCORE_ENPWM_STATUS_SHIFT                         6
#define PMIC_RG_VPROC_MIN_OFF_ADDR                                MT6353_VPROC_ANA_CON0
#define PMIC_RG_VPROC_MIN_OFF_MASK                                0x3
#define PMIC_RG_VPROC_MIN_OFF_SHIFT                               0
#define PMIC_RG_VPROC_VRF18_SSTART_EN_ADDR                        MT6353_VPROC_ANA_CON0
#define PMIC_RG_VPROC_VRF18_SSTART_EN_MASK                        0x1
#define PMIC_RG_VPROC_VRF18_SSTART_EN_SHIFT                       2
#define PMIC_RG_VPROC_1P35UP_SEL_EN_ADDR                          MT6353_VPROC_ANA_CON0
#define PMIC_RG_VPROC_1P35UP_SEL_EN_MASK                          0x1
#define PMIC_RG_VPROC_1P35UP_SEL_EN_SHIFT                         3
#define PMIC_RG_VPROC_RZSEL_ADDR                                  MT6353_VPROC_ANA_CON0
#define PMIC_RG_VPROC_RZSEL_MASK                                  0x7
#define PMIC_RG_VPROC_RZSEL_SHIFT                                 4
#define PMIC_RG_VPROC_CSR_ADDR                                    MT6353_VPROC_ANA_CON0
#define PMIC_RG_VPROC_CSR_MASK                                    0x7
#define PMIC_RG_VPROC_CSR_SHIFT                                   7
#define PMIC_RG_VPROC_CSL_ADDR                                    MT6353_VPROC_ANA_CON0
#define PMIC_RG_VPROC_CSL_MASK                                    0xF
#define PMIC_RG_VPROC_CSL_SHIFT                                   10
#define PMIC_RG_VPROC_SLP_ADDR                                    MT6353_VPROC_ANA_CON1
#define PMIC_RG_VPROC_SLP_MASK                                    0x7
#define PMIC_RG_VPROC_SLP_SHIFT                                   0
#define PMIC_RG_VPROC_ZX_OS_ADDR                                  MT6353_VPROC_ANA_CON1
#define PMIC_RG_VPROC_ZX_OS_MASK                                  0x3
#define PMIC_RG_VPROC_ZX_OS_SHIFT                                 3
#define PMIC_RG_VPROC_ZXOS_TRIM_ADDR                              MT6353_VPROC_ANA_CON1
#define PMIC_RG_VPROC_ZXOS_TRIM_MASK                              0x3F
#define PMIC_RG_VPROC_ZXOS_TRIM_SHIFT                             5
#define PMIC_RG_VPROC_MODESET_ADDR                                MT6353_VPROC_ANA_CON1
#define PMIC_RG_VPROC_MODESET_MASK                                0x1
#define PMIC_RG_VPROC_MODESET_SHIFT                               11
#define PMIC_RG_VPROC_NDIS_EN_ADDR                                MT6353_VPROC_ANA_CON1
#define PMIC_RG_VPROC_NDIS_EN_MASK                                0x1
#define PMIC_RG_VPROC_NDIS_EN_SHIFT                               12
#define PMIC_RG_VPROC_CSM_N_ADDR                                  MT6353_VPROC_ANA_CON2
#define PMIC_RG_VPROC_CSM_N_MASK                                  0x3F
#define PMIC_RG_VPROC_CSM_N_SHIFT                                 0
#define PMIC_RG_VPROC_CSM_P_ADDR                                  MT6353_VPROC_ANA_CON2
#define PMIC_RG_VPROC_CSM_P_MASK                                  0x3F
#define PMIC_RG_VPROC_CSM_P_SHIFT                                 6
#define PMIC_RG_VPROC_RSV_ADDR                                    MT6353_VPROC_ANA_CON3
#define PMIC_RG_VPROC_RSV_MASK                                    0xFF
#define PMIC_RG_VPROC_RSV_SHIFT                                   0
#define PMIC_RG_VPROC_PFM_RIP_ADDR                                MT6353_VPROC_ANA_CON3
#define PMIC_RG_VPROC_PFM_RIP_MASK                                0x7
#define PMIC_RG_VPROC_PFM_RIP_SHIFT                               8
#define PMIC_RG_VPROC_DTS_ENB_ADDR                                MT6353_VPROC_ANA_CON3
#define PMIC_RG_VPROC_DTS_ENB_MASK                                0x1
#define PMIC_RG_VPROC_DTS_ENB_SHIFT                               11
#define PMIC_RG_VPROC_BW_EXTEND_ADDR                              MT6353_VPROC_ANA_CON3
#define PMIC_RG_VPROC_BW_EXTEND_MASK                              0x1
#define PMIC_RG_VPROC_BW_EXTEND_SHIFT                             12
#define PMIC_RG_VPROC_PWM_TRIG_ADDR                               MT6353_VPROC_ANA_CON3
#define PMIC_RG_VPROC_PWM_TRIG_MASK                               0x1
#define PMIC_RG_VPROC_PWM_TRIG_SHIFT                              13
#define PMIC_RG_VPROC_TRAN_BST_ADDR                               MT6353_VPROC_ANA_CON4
#define PMIC_RG_VPROC_TRAN_BST_MASK                               0x3F
#define PMIC_RG_VPROC_TRAN_BST_SHIFT                              0
#define PMIC_RGS_VPROC_ENPWM_STATUS_ADDR                          MT6353_VPROC_ANA_CON4
#define PMIC_RGS_VPROC_ENPWM_STATUS_MASK                          0x1
#define PMIC_RGS_VPROC_ENPWM_STATUS_SHIFT                         6
#define PMIC_RG_VS1_MIN_OFF_ADDR                                  MT6353_VS1_ANA_CON0
#define PMIC_RG_VS1_MIN_OFF_MASK                                  0x3
#define PMIC_RG_VS1_MIN_OFF_SHIFT                                 0
#define PMIC_RG_VS1_NVT_BUFF_OFF_EN_ADDR                          MT6353_VS1_ANA_CON0
#define PMIC_RG_VS1_NVT_BUFF_OFF_EN_MASK                          0x1
#define PMIC_RG_VS1_NVT_BUFF_OFF_EN_SHIFT                         2
#define PMIC_RG_VS1_VRF18_SSTART_EN_ADDR                          MT6353_VS1_ANA_CON0
#define PMIC_RG_VS1_VRF18_SSTART_EN_MASK                          0x1
#define PMIC_RG_VS1_VRF18_SSTART_EN_SHIFT                         3
#define PMIC_RG_VS1_1P35UP_SEL_EN_ADDR                            MT6353_VS1_ANA_CON0
#define PMIC_RG_VS1_1P35UP_SEL_EN_MASK                            0x1
#define PMIC_RG_VS1_1P35UP_SEL_EN_SHIFT                           4
#define PMIC_RG_VS1_RZSEL_ADDR                                    MT6353_VS1_ANA_CON0
#define PMIC_RG_VS1_RZSEL_MASK                                    0x7
#define PMIC_RG_VS1_RZSEL_SHIFT                                   5
#define PMIC_RG_VS1_CSR_ADDR                                      MT6353_VS1_ANA_CON0
#define PMIC_RG_VS1_CSR_MASK                                      0x7
#define PMIC_RG_VS1_CSR_SHIFT                                     8
#define PMIC_RG_VS1_CSL_ADDR                                      MT6353_VS1_ANA_CON0
#define PMIC_RG_VS1_CSL_MASK                                      0xF
#define PMIC_RG_VS1_CSL_SHIFT                                     11
#define PMIC_RG_VS1_SLP_ADDR                                      MT6353_VS1_ANA_CON1
#define PMIC_RG_VS1_SLP_MASK                                      0x7
#define PMIC_RG_VS1_SLP_SHIFT                                     0
#define PMIC_RG_VS1_ZX_OS_ADDR                                    MT6353_VS1_ANA_CON1
#define PMIC_RG_VS1_ZX_OS_MASK                                    0x3
#define PMIC_RG_VS1_ZX_OS_SHIFT                                   3
#define PMIC_RG_VS1_NDIS_EN_ADDR                                  MT6353_VS1_ANA_CON1
#define PMIC_RG_VS1_NDIS_EN_MASK                                  0x1
#define PMIC_RG_VS1_NDIS_EN_SHIFT                                 5
#define PMIC_RG_VS1_CSM_N_ADDR                                    MT6353_VS1_ANA_CON1
#define PMIC_RG_VS1_CSM_N_MASK                                    0x3F
#define PMIC_RG_VS1_CSM_N_SHIFT                                   6
#define PMIC_RG_VS1_CSM_P_ADDR                                    MT6353_VS1_ANA_CON2
#define PMIC_RG_VS1_CSM_P_MASK                                    0x3F
#define PMIC_RG_VS1_CSM_P_SHIFT                                   0
#define PMIC_RG_VS1_RSV_ADDR                                      MT6353_VS1_ANA_CON2
#define PMIC_RG_VS1_RSV_MASK                                      0xFF
#define PMIC_RG_VS1_RSV_SHIFT                                     6
#define PMIC_RG_VS1_ZXOS_TRIM_ADDR                                MT6353_VS1_ANA_CON3
#define PMIC_RG_VS1_ZXOS_TRIM_MASK                                0x3F
#define PMIC_RG_VS1_ZXOS_TRIM_SHIFT                               0
#define PMIC_RG_VS1_MODESET_ADDR                                  MT6353_VS1_ANA_CON3
#define PMIC_RG_VS1_MODESET_MASK                                  0x1
#define PMIC_RG_VS1_MODESET_SHIFT                                 6
#define PMIC_RG_VS1_PFM_RIP_ADDR                                  MT6353_VS1_ANA_CON3
#define PMIC_RG_VS1_PFM_RIP_MASK                                  0x7
#define PMIC_RG_VS1_PFM_RIP_SHIFT                                 7
#define PMIC_RG_VS1_TRAN_BST_ADDR                                 MT6353_VS1_ANA_CON3
#define PMIC_RG_VS1_TRAN_BST_MASK                                 0x3F
#define PMIC_RG_VS1_TRAN_BST_SHIFT                                10
#define PMIC_RG_VS1_DTS_ENB_ADDR                                  MT6353_VS1_ANA_CON4
#define PMIC_RG_VS1_DTS_ENB_MASK                                  0x1
#define PMIC_RG_VS1_DTS_ENB_SHIFT                                 0
#define PMIC_RG_VS1_AUTO_MODE_ADDR                                MT6353_VS1_ANA_CON4
#define PMIC_RG_VS1_AUTO_MODE_MASK                                0x1
#define PMIC_RG_VS1_AUTO_MODE_SHIFT                               1
#define PMIC_RG_VS1_PWM_TRIG_ADDR                                 MT6353_VS1_ANA_CON4
#define PMIC_RG_VS1_PWM_TRIG_MASK                                 0x1
#define PMIC_RG_VS1_PWM_TRIG_SHIFT                                2
#define PMIC_RGS_VS1_ENPWM_STATUS_ADDR                            MT6353_VS1_ANA_CON4
#define PMIC_RGS_VS1_ENPWM_STATUS_MASK                            0x1
#define PMIC_RGS_VS1_ENPWM_STATUS_SHIFT                           3
#define PMIC_DA_QI_VCORE_BURST_ADDR                               MT6353_BUCK_ANA_CON0
#define PMIC_DA_QI_VCORE_BURST_MASK                               0x7
#define PMIC_DA_QI_VCORE_BURST_SHIFT                              0
#define PMIC_DA_QI_VCORE2_BURST_ADDR                              MT6353_BUCK_ANA_CON0
#define PMIC_DA_QI_VCORE2_BURST_MASK                              0x7
#define PMIC_DA_QI_VCORE2_BURST_SHIFT                             4
#define PMIC_DA_QI_VS1_BURST_ADDR                                 MT6353_BUCK_ANA_CON0
#define PMIC_DA_QI_VS1_BURST_MASK                                 0x7
#define PMIC_DA_QI_VS1_BURST_SHIFT                                8
#define PMIC_DA_QI_VPROC_BURST_ADDR                               MT6353_BUCK_ANA_CON0
#define PMIC_DA_QI_VPROC_BURST_MASK                               0x7
#define PMIC_DA_QI_VPROC_BURST_SHIFT                              11
#define PMIC_DA_QI_VCORE_DLC_ADDR                                 MT6353_BUCK_ANA_CON1
#define PMIC_DA_QI_VCORE_DLC_MASK                                 0x3
#define PMIC_DA_QI_VCORE_DLC_SHIFT                                0
#define PMIC_DA_QI_VCORE_DLC_N_ADDR                               MT6353_BUCK_ANA_CON1
#define PMIC_DA_QI_VCORE_DLC_N_MASK                               0x3
#define PMIC_DA_QI_VCORE_DLC_N_SHIFT                              2
#define PMIC_DA_QI_VCORE2_DLC_ADDR                                MT6353_BUCK_ANA_CON1
#define PMIC_DA_QI_VCORE2_DLC_MASK                                0x3
#define PMIC_DA_QI_VCORE2_DLC_SHIFT                               4
#define PMIC_DA_QI_VCORE2_DLC_N_ADDR                              MT6353_BUCK_ANA_CON1
#define PMIC_DA_QI_VCORE2_DLC_N_MASK                              0x3
#define PMIC_DA_QI_VCORE2_DLC_N_SHIFT                             6
#define PMIC_DA_QI_VS1_DLC_ADDR                                   MT6353_BUCK_ANA_CON1
#define PMIC_DA_QI_VS1_DLC_MASK                                   0x3
#define PMIC_DA_QI_VS1_DLC_SHIFT                                  8
#define PMIC_DA_QI_VS1_DLC_N_ADDR                                 MT6353_BUCK_ANA_CON1
#define PMIC_DA_QI_VS1_DLC_N_MASK                                 0x3
#define PMIC_DA_QI_VS1_DLC_N_SHIFT                                10
#define PMIC_DA_QI_VPROC_DLC_ADDR                                 MT6353_BUCK_ANA_CON1
#define PMIC_DA_QI_VPROC_DLC_MASK                                 0x3
#define PMIC_DA_QI_VPROC_DLC_SHIFT                                12
#define PMIC_DA_QI_VPROC_DLC_N_ADDR                               MT6353_BUCK_ANA_CON1
#define PMIC_DA_QI_VPROC_DLC_N_MASK                               0x3
#define PMIC_DA_QI_VPROC_DLC_N_SHIFT                              14
#define PMIC_RG_VCORE2_MIN_OFF_ADDR                               MT6353_VCORE2_ANA_CON0
#define PMIC_RG_VCORE2_MIN_OFF_MASK                               0x3
#define PMIC_RG_VCORE2_MIN_OFF_SHIFT                              0
#define PMIC_RG_VCORE2_VRF18_SSTART_EN_ADDR                       MT6353_VCORE2_ANA_CON0
#define PMIC_RG_VCORE2_VRF18_SSTART_EN_MASK                       0x1
#define PMIC_RG_VCORE2_VRF18_SSTART_EN_SHIFT                      2
#define PMIC_RG_VCORE2_1P35UP_SEL_EN_ADDR                         MT6353_VCORE2_ANA_CON0
#define PMIC_RG_VCORE2_1P35UP_SEL_EN_MASK                         0x1
#define PMIC_RG_VCORE2_1P35UP_SEL_EN_SHIFT                        3
#define PMIC_RG_VCORE2_RZSEL_ADDR                                 MT6353_VCORE2_ANA_CON0
#define PMIC_RG_VCORE2_RZSEL_MASK                                 0x7
#define PMIC_RG_VCORE2_RZSEL_SHIFT                                4
#define PMIC_RG_VCORE2_CSR_ADDR                                   MT6353_VCORE2_ANA_CON0
#define PMIC_RG_VCORE2_CSR_MASK                                   0x7
#define PMIC_RG_VCORE2_CSR_SHIFT                                  7
#define PMIC_RG_VCORE2_CSL_ADDR                                   MT6353_VCORE2_ANA_CON0
#define PMIC_RG_VCORE2_CSL_MASK                                   0xF
#define PMIC_RG_VCORE2_CSL_SHIFT                                  10
#define PMIC_RG_VCORE2_SLP_ADDR                                   MT6353_VCORE2_ANA_CON1
#define PMIC_RG_VCORE2_SLP_MASK                                   0x7
#define PMIC_RG_VCORE2_SLP_SHIFT                                  0
#define PMIC_RG_VCORE2_ZX_OS_ADDR                                 MT6353_VCORE2_ANA_CON1
#define PMIC_RG_VCORE2_ZX_OS_MASK                                 0x3
#define PMIC_RG_VCORE2_ZX_OS_SHIFT                                3
#define PMIC_RG_VCORE2_ZXOS_TRIM_ADDR                             MT6353_VCORE2_ANA_CON1
#define PMIC_RG_VCORE2_ZXOS_TRIM_MASK                             0x3F
#define PMIC_RG_VCORE2_ZXOS_TRIM_SHIFT                            5
#define PMIC_RG_VCORE2_MODESET_ADDR                               MT6353_VCORE2_ANA_CON1
#define PMIC_RG_VCORE2_MODESET_MASK                               0x1
#define PMIC_RG_VCORE2_MODESET_SHIFT                              11
#define PMIC_RG_VCORE2_NDIS_EN_ADDR                               MT6353_VCORE2_ANA_CON1
#define PMIC_RG_VCORE2_NDIS_EN_MASK                               0x1
#define PMIC_RG_VCORE2_NDIS_EN_SHIFT                              12
#define PMIC_RG_VCORE2_CSM_N_ADDR                                 MT6353_VCORE2_ANA_CON2
#define PMIC_RG_VCORE2_CSM_N_MASK                                 0x3F
#define PMIC_RG_VCORE2_CSM_N_SHIFT                                0
#define PMIC_RG_VCORE2_CSM_P_ADDR                                 MT6353_VCORE2_ANA_CON2
#define PMIC_RG_VCORE2_CSM_P_MASK                                 0x3F
#define PMIC_RG_VCORE2_CSM_P_SHIFT                                6
#define PMIC_RG_VCORE2_RSV_ADDR                                   MT6353_VCORE2_ANA_CON3
#define PMIC_RG_VCORE2_RSV_MASK                                   0xFF
#define PMIC_RG_VCORE2_RSV_SHIFT                                  0
#define PMIC_RG_VCORE2_PFM_RIP_ADDR                               MT6353_VCORE2_ANA_CON3
#define PMIC_RG_VCORE2_PFM_RIP_MASK                               0x7
#define PMIC_RG_VCORE2_PFM_RIP_SHIFT                              8
#define PMIC_RG_VCORE2_DTS_ENB_ADDR                               MT6353_VCORE2_ANA_CON3
#define PMIC_RG_VCORE2_DTS_ENB_MASK                               0x1
#define PMIC_RG_VCORE2_DTS_ENB_SHIFT                              11
#define PMIC_RG_VCORE2_AUTO_MODE_ADDR                             MT6353_VCORE2_ANA_CON3
#define PMIC_RG_VCORE2_AUTO_MODE_MASK                             0x1
#define PMIC_RG_VCORE2_AUTO_MODE_SHIFT                            12
#define PMIC_RG_VCORE2_PWM_TRIG_ADDR                              MT6353_VCORE2_ANA_CON3
#define PMIC_RG_VCORE2_PWM_TRIG_MASK                              0x1
#define PMIC_RG_VCORE2_PWM_TRIG_SHIFT                             13
#define PMIC_RG_VCORE2_TRAN_BST_ADDR                              MT6353_VCORE2_ANA_CON4
#define PMIC_RG_VCORE2_TRAN_BST_MASK                              0x3F
#define PMIC_RG_VCORE2_TRAN_BST_SHIFT                             0
#define PMIC_RGS_VCORE2_ENPWM_STATUS_ADDR                         MT6353_VCORE2_ANA_CON4
#define PMIC_RGS_VCORE2_ENPWM_STATUS_MASK                         0x1
#define PMIC_RGS_VCORE2_ENPWM_STATUS_SHIFT                        6
#define PMIC_BUCK_VPROC_EN_CTRL_ADDR                              MT6353_BUCK_VPROC_HWM_CON0
#define PMIC_BUCK_VPROC_EN_CTRL_MASK                              0x1
#define PMIC_BUCK_VPROC_EN_CTRL_SHIFT                             0
#define PMIC_BUCK_VPROC_VOSEL_CTRL_ADDR                           MT6353_BUCK_VPROC_HWM_CON0
#define PMIC_BUCK_VPROC_VOSEL_CTRL_MASK                           0x1
#define PMIC_BUCK_VPROC_VOSEL_CTRL_SHIFT                          1
#define PMIC_BUCK_VPROC_EN_SEL_ADDR                               MT6353_BUCK_VPROC_HWM_CON1
#define PMIC_BUCK_VPROC_EN_SEL_MASK                               0x7
#define PMIC_BUCK_VPROC_EN_SEL_SHIFT                              0
#define PMIC_BUCK_VPROC_VOSEL_SEL_ADDR                            MT6353_BUCK_VPROC_HWM_CON1
#define PMIC_BUCK_VPROC_VOSEL_SEL_MASK                            0x7
#define PMIC_BUCK_VPROC_VOSEL_SEL_SHIFT                           3
#define PMIC_BUCK_VPROC_EN_ADDR                                   MT6353_BUCK_VPROC_EN_CON0
#define PMIC_BUCK_VPROC_EN_MASK                                   0x1
#define PMIC_BUCK_VPROC_EN_SHIFT                                  0
#define PMIC_BUCK_VPROC_STBTD_ADDR                                MT6353_BUCK_VPROC_EN_CON0
#define PMIC_BUCK_VPROC_STBTD_MASK                                0x3
#define PMIC_BUCK_VPROC_STBTD_SHIFT                               4
#define PMIC_DA_VPROC_STB_ADDR                                    MT6353_BUCK_VPROC_EN_CON0
#define PMIC_DA_VPROC_STB_MASK                                    0x1
#define PMIC_DA_VPROC_STB_SHIFT                                   12
#define PMIC_DA_QI_VPROC_EN_ADDR                                  MT6353_BUCK_VPROC_EN_CON0
#define PMIC_DA_QI_VPROC_EN_MASK                                  0x1
#define PMIC_DA_QI_VPROC_EN_SHIFT                                 13
#define PMIC_BUCK_VPROC_SFCHG_FRATE_ADDR                          MT6353_BUCK_VPROC_SFCHG_CON0
#define PMIC_BUCK_VPROC_SFCHG_FRATE_MASK                          0x7F
#define PMIC_BUCK_VPROC_SFCHG_FRATE_SHIFT                         0
#define PMIC_BUCK_VPROC_SFCHG_FEN_ADDR                            MT6353_BUCK_VPROC_SFCHG_CON0
#define PMIC_BUCK_VPROC_SFCHG_FEN_MASK                            0x1
#define PMIC_BUCK_VPROC_SFCHG_FEN_SHIFT                           7
#define PMIC_BUCK_VPROC_SFCHG_RRATE_ADDR                          MT6353_BUCK_VPROC_SFCHG_CON0
#define PMIC_BUCK_VPROC_SFCHG_RRATE_MASK                          0x7F
#define PMIC_BUCK_VPROC_SFCHG_RRATE_SHIFT                         8
#define PMIC_BUCK_VPROC_SFCHG_REN_ADDR                            MT6353_BUCK_VPROC_SFCHG_CON0
#define PMIC_BUCK_VPROC_SFCHG_REN_MASK                            0x1
#define PMIC_BUCK_VPROC_SFCHG_REN_SHIFT                           15
#define PMIC_DA_NI_VPROC_VOSEL_ADDR                               MT6353_BUCK_VPROC_VOL_CON0
#define PMIC_DA_NI_VPROC_VOSEL_MASK                               0x7F
#define PMIC_DA_NI_VPROC_VOSEL_SHIFT                              0
#define PMIC_BUCK_VPROC_VOSEL_ADDR                                MT6353_BUCK_VPROC_VOL_CON1
#define PMIC_BUCK_VPROC_VOSEL_MASK                                0x7F
#define PMIC_BUCK_VPROC_VOSEL_SHIFT                               0
#define PMIC_BUCK_VPROC_VOSEL_ON_ADDR                             MT6353_BUCK_VPROC_VOL_CON2
#define PMIC_BUCK_VPROC_VOSEL_ON_MASK                             0x7F
#define PMIC_BUCK_VPROC_VOSEL_ON_SHIFT                            0
#define PMIC_BUCK_VPROC_VOSEL_SLEEP_ADDR                          MT6353_BUCK_VPROC_VOL_CON3
#define PMIC_BUCK_VPROC_VOSEL_SLEEP_MASK                          0x7F
#define PMIC_BUCK_VPROC_VOSEL_SLEEP_SHIFT                         0
#define PMIC_BUCK_VPROC_TRANS_TD_ADDR                             MT6353_BUCK_VPROC_LPW_CON0
#define PMIC_BUCK_VPROC_TRANS_TD_MASK                             0x3
#define PMIC_BUCK_VPROC_TRANS_TD_SHIFT                            0
#define PMIC_BUCK_VPROC_TRANS_CTRL_ADDR                           MT6353_BUCK_VPROC_LPW_CON0
#define PMIC_BUCK_VPROC_TRANS_CTRL_MASK                           0x3
#define PMIC_BUCK_VPROC_TRANS_CTRL_SHIFT                          4
#define PMIC_BUCK_VPROC_TRANS_ONCE_ADDR                           MT6353_BUCK_VPROC_LPW_CON0
#define PMIC_BUCK_VPROC_TRANS_ONCE_MASK                           0x1
#define PMIC_BUCK_VPROC_TRANS_ONCE_SHIFT                          6
#define PMIC_DA_QI_VPROC_DVS_EN_ADDR                              MT6353_BUCK_VPROC_LPW_CON0
#define PMIC_DA_QI_VPROC_DVS_EN_MASK                              0x1
#define PMIC_DA_QI_VPROC_DVS_EN_SHIFT                             7
#define PMIC_BUCK_VPROC_VSLEEP_EN_ADDR                            MT6353_BUCK_VPROC_LPW_CON0
#define PMIC_BUCK_VPROC_VSLEEP_EN_MASK                            0x1
#define PMIC_BUCK_VPROC_VSLEEP_EN_SHIFT                           8
#define PMIC_BUCK_VPROC_R2R_PDN_ADDR                              MT6353_BUCK_VPROC_LPW_CON0
#define PMIC_BUCK_VPROC_R2R_PDN_MASK                              0x1
#define PMIC_BUCK_VPROC_R2R_PDN_SHIFT                             10
#define PMIC_BUCK_VPROC_VSLEEP_SEL_ADDR                           MT6353_BUCK_VPROC_LPW_CON0
#define PMIC_BUCK_VPROC_VSLEEP_SEL_MASK                           0x1
#define PMIC_BUCK_VPROC_VSLEEP_SEL_SHIFT                          11
#define PMIC_DA_NI_VPROC_R2R_PDN_ADDR                             MT6353_BUCK_VPROC_LPW_CON0
#define PMIC_DA_NI_VPROC_R2R_PDN_MASK                             0x1
#define PMIC_DA_NI_VPROC_R2R_PDN_SHIFT                            14
#define PMIC_DA_NI_VPROC_VSLEEP_SEL_ADDR                          MT6353_BUCK_VPROC_LPW_CON0
#define PMIC_DA_NI_VPROC_VSLEEP_SEL_MASK                          0x1
#define PMIC_DA_NI_VPROC_VSLEEP_SEL_SHIFT                         15
#define PMIC_BUCK_VS1_EN_CTRL_ADDR                                MT6353_BUCK_VS1_HWM_CON0
#define PMIC_BUCK_VS1_EN_CTRL_MASK                                0x1
#define PMIC_BUCK_VS1_EN_CTRL_SHIFT                               0
#define PMIC_BUCK_VS1_VOSEL_CTRL_ADDR                             MT6353_BUCK_VS1_HWM_CON0
#define PMIC_BUCK_VS1_VOSEL_CTRL_MASK                             0x1
#define PMIC_BUCK_VS1_VOSEL_CTRL_SHIFT                            1
#define PMIC_BUCK_VS1_VOSEL_CTRL_0_ADDR                           MT6353_BUCK_VS1_HWM_CON0
#define PMIC_BUCK_VS1_VOSEL_CTRL_0_MASK                           0x1
#define PMIC_BUCK_VS1_VOSEL_CTRL_0_SHIFT                          2
#define PMIC_BUCK_VS1_VOSEL_CTRL_1_ADDR                           MT6353_BUCK_VS1_HWM_CON0
#define PMIC_BUCK_VS1_VOSEL_CTRL_1_MASK                           0x1
#define PMIC_BUCK_VS1_VOSEL_CTRL_1_SHIFT                          3
#define PMIC_BUCK_VS1_VOSEL_CTRL_2_ADDR                           MT6353_BUCK_VS1_HWM_CON0
#define PMIC_BUCK_VS1_VOSEL_CTRL_2_MASK                           0x1
#define PMIC_BUCK_VS1_VOSEL_CTRL_2_SHIFT                          4
#define PMIC_BUCK_VS1_MODESET_0_ADDR                              MT6353_BUCK_VS1_HWM_CON0
#define PMIC_BUCK_VS1_MODESET_0_MASK                              0x1
#define PMIC_BUCK_VS1_MODESET_0_SHIFT                             5
#define PMIC_BUCK_VS1_MODESET_1_ADDR                              MT6353_BUCK_VS1_HWM_CON0
#define PMIC_BUCK_VS1_MODESET_1_MASK                              0x1
#define PMIC_BUCK_VS1_MODESET_1_SHIFT                             6
#define PMIC_BUCK_VS1_MODESET_2_ADDR                              MT6353_BUCK_VS1_HWM_CON0
#define PMIC_BUCK_VS1_MODESET_2_MASK                              0x1
#define PMIC_BUCK_VS1_MODESET_2_SHIFT                             7
#define PMIC_DA_VS1_MODESET_ADDR                                  MT6353_BUCK_VS1_HWM_CON0
#define PMIC_DA_VS1_MODESET_MASK                                  0x1
#define PMIC_DA_VS1_MODESET_SHIFT                                 8
#define PMIC_BUCK_VS1_VOSEL_CTRL_INV_ADDR                         MT6353_BUCK_VS1_HWM_CON0
#define PMIC_BUCK_VS1_VOSEL_CTRL_INV_MASK                         0x1
#define PMIC_BUCK_VS1_VOSEL_CTRL_INV_SHIFT                        9
#define PMIC_BUCK_VS1_HWM_CON0_SET_ADDR                           MT6353_BUCK_VS1_HWM_CON0_SET
#define PMIC_BUCK_VS1_HWM_CON0_SET_MASK                           0xFFFF
#define PMIC_BUCK_VS1_HWM_CON0_SET_SHIFT                          0
#define PMIC_BUCK_VS1_HWM_CON0_CLR_ADDR                           MT6353_BUCK_VS1_HWM_CON0_CLR
#define PMIC_BUCK_VS1_HWM_CON0_CLR_MASK                           0xFFFF
#define PMIC_BUCK_VS1_HWM_CON0_CLR_SHIFT                          0
#define PMIC_BUCK_VS1_EN_SEL_ADDR                                 MT6353_BUCK_VS1_HWM_CON1
#define PMIC_BUCK_VS1_EN_SEL_MASK                                 0x7
#define PMIC_BUCK_VS1_EN_SEL_SHIFT                                0
#define PMIC_BUCK_VS1_VOSEL_SEL_ADDR                              MT6353_BUCK_VS1_HWM_CON1
#define PMIC_BUCK_VS1_VOSEL_SEL_MASK                              0x7
#define PMIC_BUCK_VS1_VOSEL_SEL_SHIFT                             3
#define PMIC_BUCK_VS1_EN_ADDR                                     MT6353_BUCK_VS1_EN_CON0
#define PMIC_BUCK_VS1_EN_MASK                                     0x1
#define PMIC_BUCK_VS1_EN_SHIFT                                    0
#define PMIC_BUCK_VS1_STBTD_ADDR                                  MT6353_BUCK_VS1_EN_CON0
#define PMIC_BUCK_VS1_STBTD_MASK                                  0x3
#define PMIC_BUCK_VS1_STBTD_SHIFT                                 4
#define PMIC_DA_VS1_STB_ADDR                                      MT6353_BUCK_VS1_EN_CON0
#define PMIC_DA_VS1_STB_MASK                                      0x1
#define PMIC_DA_VS1_STB_SHIFT                                     12
#define PMIC_DA_QI_VS1_EN_ADDR                                    MT6353_BUCK_VS1_EN_CON0
#define PMIC_DA_QI_VS1_EN_MASK                                    0x1
#define PMIC_DA_QI_VS1_EN_SHIFT                                   13
#define PMIC_BUCK_VS1_SFCHG_FRATE_ADDR                            MT6353_BUCK_VS1_SFCHG_CON0
#define PMIC_BUCK_VS1_SFCHG_FRATE_MASK                            0x7F
#define PMIC_BUCK_VS1_SFCHG_FRATE_SHIFT                           0
#define PMIC_BUCK_VS1_SFCHG_FEN_ADDR                              MT6353_BUCK_VS1_SFCHG_CON0
#define PMIC_BUCK_VS1_SFCHG_FEN_MASK                              0x1
#define PMIC_BUCK_VS1_SFCHG_FEN_SHIFT                             7
#define PMIC_BUCK_VS1_SFCHG_RRATE_ADDR                            MT6353_BUCK_VS1_SFCHG_CON0
#define PMIC_BUCK_VS1_SFCHG_RRATE_MASK                            0x7F
#define PMIC_BUCK_VS1_SFCHG_RRATE_SHIFT                           8
#define PMIC_BUCK_VS1_SFCHG_REN_ADDR                              MT6353_BUCK_VS1_SFCHG_CON0
#define PMIC_BUCK_VS1_SFCHG_REN_MASK                              0x1
#define PMIC_BUCK_VS1_SFCHG_REN_SHIFT                             15
#define PMIC_DA_NI_VS1_VOSEL_ADDR                                 MT6353_BUCK_VS1_VOL_CON0
#define PMIC_DA_NI_VS1_VOSEL_MASK                                 0x7F
#define PMIC_DA_NI_VS1_VOSEL_SHIFT                                0
#define PMIC_BUCK_VS1_VOSEL_ADDR                                  MT6353_BUCK_VS1_VOL_CON1
#define PMIC_BUCK_VS1_VOSEL_MASK                                  0x7F
#define PMIC_BUCK_VS1_VOSEL_SHIFT                                 0
#define PMIC_BUCK_VS1_VOSEL_ON_ADDR                               MT6353_BUCK_VS1_VOL_CON2
#define PMIC_BUCK_VS1_VOSEL_ON_MASK                               0x7F
#define PMIC_BUCK_VS1_VOSEL_ON_SHIFT                              0
#define PMIC_BUCK_VS1_VOSEL_SLEEP_ADDR                            MT6353_BUCK_VS1_VOL_CON3
#define PMIC_BUCK_VS1_VOSEL_SLEEP_MASK                            0x7F
#define PMIC_BUCK_VS1_VOSEL_SLEEP_SHIFT                           0
#define PMIC_BUCK_VS1_TRANS_TD_ADDR                               MT6353_BUCK_VS1_LPW_CON0
#define PMIC_BUCK_VS1_TRANS_TD_MASK                               0x3
#define PMIC_BUCK_VS1_TRANS_TD_SHIFT                              0
#define PMIC_BUCK_VS1_TRANS_CTRL_ADDR                             MT6353_BUCK_VS1_LPW_CON0
#define PMIC_BUCK_VS1_TRANS_CTRL_MASK                             0x3
#define PMIC_BUCK_VS1_TRANS_CTRL_SHIFT                            4
#define PMIC_BUCK_VS1_TRANS_ONCE_ADDR                             MT6353_BUCK_VS1_LPW_CON0
#define PMIC_BUCK_VS1_TRANS_ONCE_MASK                             0x1
#define PMIC_BUCK_VS1_TRANS_ONCE_SHIFT                            6
#define PMIC_DA_QI_VS1_DVS_EN_ADDR                                MT6353_BUCK_VS1_LPW_CON0
#define PMIC_DA_QI_VS1_DVS_EN_MASK                                0x1
#define PMIC_DA_QI_VS1_DVS_EN_SHIFT                               7
#define PMIC_BUCK_VS1_VSLEEP_EN_ADDR                              MT6353_BUCK_VS1_LPW_CON0
#define PMIC_BUCK_VS1_VSLEEP_EN_MASK                              0x1
#define PMIC_BUCK_VS1_VSLEEP_EN_SHIFT                             8
#define PMIC_BUCK_VS1_R2R_PDN_ADDR                                MT6353_BUCK_VS1_LPW_CON0
#define PMIC_BUCK_VS1_R2R_PDN_MASK                                0x1
#define PMIC_BUCK_VS1_R2R_PDN_SHIFT                               10
#define PMIC_BUCK_VS1_VSLEEP_SEL_ADDR                             MT6353_BUCK_VS1_LPW_CON0
#define PMIC_BUCK_VS1_VSLEEP_SEL_MASK                             0x1
#define PMIC_BUCK_VS1_VSLEEP_SEL_SHIFT                            11
#define PMIC_DA_NI_VS1_R2R_PDN_ADDR                               MT6353_BUCK_VS1_LPW_CON0
#define PMIC_DA_NI_VS1_R2R_PDN_MASK                               0x1
#define PMIC_DA_NI_VS1_R2R_PDN_SHIFT                              14
#define PMIC_DA_NI_VS1_VSLEEP_SEL_ADDR                            MT6353_BUCK_VS1_LPW_CON0
#define PMIC_DA_NI_VS1_VSLEEP_SEL_MASK                            0x1
#define PMIC_DA_NI_VS1_VSLEEP_SEL_SHIFT                           15
#define PMIC_BUCK_VCORE_EN_CTRL_ADDR                              MT6353_BUCK_VCORE_HWM_CON0
#define PMIC_BUCK_VCORE_EN_CTRL_MASK                              0x1
#define PMIC_BUCK_VCORE_EN_CTRL_SHIFT                             0
#define PMIC_BUCK_VCORE_VOSEL_CTRL_ADDR                           MT6353_BUCK_VCORE_HWM_CON0
#define PMIC_BUCK_VCORE_VOSEL_CTRL_MASK                           0x1
#define PMIC_BUCK_VCORE_VOSEL_CTRL_SHIFT                          1
#define PMIC_BUCK_VCORE_EN_SEL_ADDR                               MT6353_BUCK_VCORE_HWM_CON1
#define PMIC_BUCK_VCORE_EN_SEL_MASK                               0x7
#define PMIC_BUCK_VCORE_EN_SEL_SHIFT                              0
#define PMIC_BUCK_VCORE_VOSEL_SEL_ADDR                            MT6353_BUCK_VCORE_HWM_CON1
#define PMIC_BUCK_VCORE_VOSEL_SEL_MASK                            0x7
#define PMIC_BUCK_VCORE_VOSEL_SEL_SHIFT                           3
#define PMIC_BUCK_VCORE_EN_ADDR                                   MT6353_BUCK_VCORE_EN_CON0
#define PMIC_BUCK_VCORE_EN_MASK                                   0x1
#define PMIC_BUCK_VCORE_EN_SHIFT                                  0
#define PMIC_BUCK_VCORE_STBTD_ADDR                                MT6353_BUCK_VCORE_EN_CON0
#define PMIC_BUCK_VCORE_STBTD_MASK                                0x3
#define PMIC_BUCK_VCORE_STBTD_SHIFT                               4
#define PMIC_DA_VCORE_STB_ADDR                                    MT6353_BUCK_VCORE_EN_CON0
#define PMIC_DA_VCORE_STB_MASK                                    0x1
#define PMIC_DA_VCORE_STB_SHIFT                                   12
#define PMIC_DA_QI_VCORE_EN_ADDR                                  MT6353_BUCK_VCORE_EN_CON0
#define PMIC_DA_QI_VCORE_EN_MASK                                  0x1
#define PMIC_DA_QI_VCORE_EN_SHIFT                                 13
#define PMIC_BUCK_VCORE_SFCHG_FRATE_ADDR                          MT6353_BUCK_VCORE_SFCHG_CON0
#define PMIC_BUCK_VCORE_SFCHG_FRATE_MASK                          0x7F
#define PMIC_BUCK_VCORE_SFCHG_FRATE_SHIFT                         0
#define PMIC_BUCK_VCORE_SFCHG_FEN_ADDR                            MT6353_BUCK_VCORE_SFCHG_CON0
#define PMIC_BUCK_VCORE_SFCHG_FEN_MASK                            0x1
#define PMIC_BUCK_VCORE_SFCHG_FEN_SHIFT                           7
#define PMIC_BUCK_VCORE_SFCHG_RRATE_ADDR                          MT6353_BUCK_VCORE_SFCHG_CON0
#define PMIC_BUCK_VCORE_SFCHG_RRATE_MASK                          0x7F
#define PMIC_BUCK_VCORE_SFCHG_RRATE_SHIFT                         8
#define PMIC_BUCK_VCORE_SFCHG_REN_ADDR                            MT6353_BUCK_VCORE_SFCHG_CON0
#define PMIC_BUCK_VCORE_SFCHG_REN_MASK                            0x1
#define PMIC_BUCK_VCORE_SFCHG_REN_SHIFT                           15
#define PMIC_DA_NI_VCORE_VOSEL_ADDR                               MT6353_BUCK_VCORE_VOL_CON0
#define PMIC_DA_NI_VCORE_VOSEL_MASK                               0x7F
#define PMIC_DA_NI_VCORE_VOSEL_SHIFT                              0
#define PMIC_BUCK_VCORE_VOSEL_ADDR                                MT6353_BUCK_VCORE_VOL_CON1
#define PMIC_BUCK_VCORE_VOSEL_MASK                                0x7F
#define PMIC_BUCK_VCORE_VOSEL_SHIFT                               0
#define PMIC_BUCK_VCORE_VOSEL_ON_ADDR                             MT6353_BUCK_VCORE_VOL_CON2
#define PMIC_BUCK_VCORE_VOSEL_ON_MASK                             0x7F
#define PMIC_BUCK_VCORE_VOSEL_ON_SHIFT                            0
#define PMIC_BUCK_VCORE_VOSEL_SLEEP_ADDR                          MT6353_BUCK_VCORE_VOL_CON3
#define PMIC_BUCK_VCORE_VOSEL_SLEEP_MASK                          0x7F
#define PMIC_BUCK_VCORE_VOSEL_SLEEP_SHIFT                         0
#define PMIC_BUCK_VCORE_TRANS_TD_ADDR                             MT6353_BUCK_VCORE_LPW_CON0
#define PMIC_BUCK_VCORE_TRANS_TD_MASK                             0x3
#define PMIC_BUCK_VCORE_TRANS_TD_SHIFT                            0
#define PMIC_BUCK_VCORE_TRANS_CTRL_ADDR                           MT6353_BUCK_VCORE_LPW_CON0
#define PMIC_BUCK_VCORE_TRANS_CTRL_MASK                           0x3
#define PMIC_BUCK_VCORE_TRANS_CTRL_SHIFT                          4
#define PMIC_BUCK_VCORE_TRANS_ONCE_ADDR                           MT6353_BUCK_VCORE_LPW_CON0
#define PMIC_BUCK_VCORE_TRANS_ONCE_MASK                           0x1
#define PMIC_BUCK_VCORE_TRANS_ONCE_SHIFT                          6
#define PMIC_DA_QI_VCORE_DVS_EN_ADDR                              MT6353_BUCK_VCORE_LPW_CON0
#define PMIC_DA_QI_VCORE_DVS_EN_MASK                              0x1
#define PMIC_DA_QI_VCORE_DVS_EN_SHIFT                             7
#define PMIC_BUCK_VCORE_VSLEEP_EN_ADDR                            MT6353_BUCK_VCORE_LPW_CON0
#define PMIC_BUCK_VCORE_VSLEEP_EN_MASK                            0x1
#define PMIC_BUCK_VCORE_VSLEEP_EN_SHIFT                           8
#define PMIC_BUCK_VCORE_R2R_PDN_ADDR                              MT6353_BUCK_VCORE_LPW_CON0
#define PMIC_BUCK_VCORE_R2R_PDN_MASK                              0x1
#define PMIC_BUCK_VCORE_R2R_PDN_SHIFT                             10
#define PMIC_BUCK_VCORE_VSLEEP_SEL_ADDR                           MT6353_BUCK_VCORE_LPW_CON0
#define PMIC_BUCK_VCORE_VSLEEP_SEL_MASK                           0x1
#define PMIC_BUCK_VCORE_VSLEEP_SEL_SHIFT                          11
#define PMIC_DA_NI_VCORE_R2R_PDN_ADDR                             MT6353_BUCK_VCORE_LPW_CON0
#define PMIC_DA_NI_VCORE_R2R_PDN_MASK                             0x1
#define PMIC_DA_NI_VCORE_R2R_PDN_SHIFT                            14
#define PMIC_DA_NI_VCORE_VSLEEP_SEL_ADDR                          MT6353_BUCK_VCORE_LPW_CON0
#define PMIC_DA_NI_VCORE_VSLEEP_SEL_MASK                          0x1
#define PMIC_DA_NI_VCORE_VSLEEP_SEL_SHIFT                         15
#define PMIC_BUCK_VCORE2_EN_CTRL_ADDR                             MT6353_BUCK_VCORE2_HWM_CON0
#define PMIC_BUCK_VCORE2_EN_CTRL_MASK                             0x1
#define PMIC_BUCK_VCORE2_EN_CTRL_SHIFT                            0
#define PMIC_BUCK_VCORE2_VOSEL_CTRL_ADDR                          MT6353_BUCK_VCORE2_HWM_CON0
#define PMIC_BUCK_VCORE2_VOSEL_CTRL_MASK                          0x1
#define PMIC_BUCK_VCORE2_VOSEL_CTRL_SHIFT                         1
#define PMIC_BUCK_VCORE2_EN_SEL_ADDR                              MT6353_BUCK_VCORE2_HWM_CON1
#define PMIC_BUCK_VCORE2_EN_SEL_MASK                              0x7
#define PMIC_BUCK_VCORE2_EN_SEL_SHIFT                             0
#define PMIC_BUCK_VCORE2_VOSEL_SEL_ADDR                           MT6353_BUCK_VCORE2_HWM_CON1
#define PMIC_BUCK_VCORE2_VOSEL_SEL_MASK                           0x7
#define PMIC_BUCK_VCORE2_VOSEL_SEL_SHIFT                          3
#define PMIC_BUCK_VCORE2_EN_ADDR                                  MT6353_BUCK_VCORE2_EN_CON0
#define PMIC_BUCK_VCORE2_EN_MASK                                  0x1
#define PMIC_BUCK_VCORE2_EN_SHIFT                                 0
#define PMIC_BUCK_VCORE2_STBTD_ADDR                               MT6353_BUCK_VCORE2_EN_CON0
#define PMIC_BUCK_VCORE2_STBTD_MASK                               0x3
#define PMIC_BUCK_VCORE2_STBTD_SHIFT                              4
#define PMIC_DA_VCORE2_STB_ADDR                                   MT6353_BUCK_VCORE2_EN_CON0
#define PMIC_DA_VCORE2_STB_MASK                                   0x1
#define PMIC_DA_VCORE2_STB_SHIFT                                  12
#define PMIC_DA_QI_VCORE2_EN_ADDR                                 MT6353_BUCK_VCORE2_EN_CON0
#define PMIC_DA_QI_VCORE2_EN_MASK                                 0x1
#define PMIC_DA_QI_VCORE2_EN_SHIFT                                13
#define PMIC_BUCK_VCORE2_SFCHG_FRATE_ADDR                         MT6353_BUCK_VCORE2_SFCHG_CON0
#define PMIC_BUCK_VCORE2_SFCHG_FRATE_MASK                         0x7F
#define PMIC_BUCK_VCORE2_SFCHG_FRATE_SHIFT                        0
#define PMIC_BUCK_VCORE2_SFCHG_FEN_ADDR                           MT6353_BUCK_VCORE2_SFCHG_CON0
#define PMIC_BUCK_VCORE2_SFCHG_FEN_MASK                           0x1
#define PMIC_BUCK_VCORE2_SFCHG_FEN_SHIFT                          7
#define PMIC_BUCK_VCORE2_SFCHG_RRATE_ADDR                         MT6353_BUCK_VCORE2_SFCHG_CON0
#define PMIC_BUCK_VCORE2_SFCHG_RRATE_MASK                         0x7F
#define PMIC_BUCK_VCORE2_SFCHG_RRATE_SHIFT                        8
#define PMIC_BUCK_VCORE2_SFCHG_REN_ADDR                           MT6353_BUCK_VCORE2_SFCHG_CON0
#define PMIC_BUCK_VCORE2_SFCHG_REN_MASK                           0x1
#define PMIC_BUCK_VCORE2_SFCHG_REN_SHIFT                          15
#define PMIC_DA_NI_VCORE2_VOSEL_ADDR                              MT6353_BUCK_VCORE2_VOL_CON0
#define PMIC_DA_NI_VCORE2_VOSEL_MASK                              0x7F
#define PMIC_DA_NI_VCORE2_VOSEL_SHIFT                             0
#define PMIC_BUCK_VCORE2_VOSEL_ADDR                               MT6353_BUCK_VCORE2_VOL_CON1
#define PMIC_BUCK_VCORE2_VOSEL_MASK                               0x7F
#define PMIC_BUCK_VCORE2_VOSEL_SHIFT                              0
#define PMIC_BUCK_VCORE2_VOSEL_ON_ADDR                            MT6353_BUCK_VCORE2_VOL_CON2
#define PMIC_BUCK_VCORE2_VOSEL_ON_MASK                            0x7F
#define PMIC_BUCK_VCORE2_VOSEL_ON_SHIFT                           0
#define PMIC_BUCK_VCORE2_VOSEL_SLEEP_ADDR                         MT6353_BUCK_VCORE2_VOL_CON3
#define PMIC_BUCK_VCORE2_VOSEL_SLEEP_MASK                         0x7F
#define PMIC_BUCK_VCORE2_VOSEL_SLEEP_SHIFT                        0
#define PMIC_BUCK_VCORE2_TRANS_TD_ADDR                            MT6353_BUCK_VCORE2_LPW_CON0
#define PMIC_BUCK_VCORE2_TRANS_TD_MASK                            0x3
#define PMIC_BUCK_VCORE2_TRANS_TD_SHIFT                           0
#define PMIC_BUCK_VCORE2_TRANS_CTRL_ADDR                          MT6353_BUCK_VCORE2_LPW_CON0
#define PMIC_BUCK_VCORE2_TRANS_CTRL_MASK                          0x3
#define PMIC_BUCK_VCORE2_TRANS_CTRL_SHIFT                         4
#define PMIC_BUCK_VCORE2_TRANS_ONCE_ADDR                          MT6353_BUCK_VCORE2_LPW_CON0
#define PMIC_BUCK_VCORE2_TRANS_ONCE_MASK                          0x1
#define PMIC_BUCK_VCORE2_TRANS_ONCE_SHIFT                         6
#define PMIC_DA_QI_VCORE2_DVS_EN_ADDR                             MT6353_BUCK_VCORE2_LPW_CON0
#define PMIC_DA_QI_VCORE2_DVS_EN_MASK                             0x1
#define PMIC_DA_QI_VCORE2_DVS_EN_SHIFT                            7
#define PMIC_BUCK_VCORE2_VSLEEP_EN_ADDR                           MT6353_BUCK_VCORE2_LPW_CON0
#define PMIC_BUCK_VCORE2_VSLEEP_EN_MASK                           0x1
#define PMIC_BUCK_VCORE2_VSLEEP_EN_SHIFT                          8
#define PMIC_BUCK_VCORE2_R2R_PDN_ADDR                             MT6353_BUCK_VCORE2_LPW_CON0
#define PMIC_BUCK_VCORE2_R2R_PDN_MASK                             0x1
#define PMIC_BUCK_VCORE2_R2R_PDN_SHIFT                            10
#define PMIC_BUCK_VCORE2_VSLEEP_SEL_ADDR                          MT6353_BUCK_VCORE2_LPW_CON0
#define PMIC_BUCK_VCORE2_VSLEEP_SEL_MASK                          0x1
#define PMIC_BUCK_VCORE2_VSLEEP_SEL_SHIFT                         11
#define PMIC_DA_NI_VCORE2_R2R_PDN_ADDR                            MT6353_BUCK_VCORE2_LPW_CON0
#define PMIC_DA_NI_VCORE2_R2R_PDN_MASK                            0x1
#define PMIC_DA_NI_VCORE2_R2R_PDN_SHIFT                           14
#define PMIC_DA_NI_VCORE2_VSLEEP_SEL_ADDR                         MT6353_BUCK_VCORE2_LPW_CON0
#define PMIC_DA_NI_VCORE2_VSLEEP_SEL_MASK                         0x1
#define PMIC_DA_NI_VCORE2_VSLEEP_SEL_SHIFT                        15
#define PMIC_BUCK_VPA_EN_ADDR                                     MT6353_BUCK_VPA_EN_CON0
#define PMIC_BUCK_VPA_EN_MASK                                     0x1
#define PMIC_BUCK_VPA_EN_SHIFT                                    0
#define PMIC_BUCK_VPA_STBTD_ADDR                                  MT6353_BUCK_VPA_EN_CON0
#define PMIC_BUCK_VPA_STBTD_MASK                                  0x3
#define PMIC_BUCK_VPA_STBTD_SHIFT                                 4
#define PMIC_DA_VPA_STB_ADDR                                      MT6353_BUCK_VPA_EN_CON0
#define PMIC_DA_VPA_STB_MASK                                      0x1
#define PMIC_DA_VPA_STB_SHIFT                                     12
#define PMIC_DA_QI_VPA_EN_ADDR                                    MT6353_BUCK_VPA_EN_CON0
#define PMIC_DA_QI_VPA_EN_MASK                                    0x1
#define PMIC_DA_QI_VPA_EN_SHIFT                                   13
#define PMIC_BUCK_VPA_SFCHG_FRATE_ADDR                            MT6353_BUCK_VPA_SFCHG_CON0
#define PMIC_BUCK_VPA_SFCHG_FRATE_MASK                            0x7F
#define PMIC_BUCK_VPA_SFCHG_FRATE_SHIFT                           0
#define PMIC_BUCK_VPA_SFCHG_FEN_ADDR                              MT6353_BUCK_VPA_SFCHG_CON0
#define PMIC_BUCK_VPA_SFCHG_FEN_MASK                              0x1
#define PMIC_BUCK_VPA_SFCHG_FEN_SHIFT                             7
#define PMIC_BUCK_VPA_SFCHG_RRATE_ADDR                            MT6353_BUCK_VPA_SFCHG_CON0
#define PMIC_BUCK_VPA_SFCHG_RRATE_MASK                            0x7F
#define PMIC_BUCK_VPA_SFCHG_RRATE_SHIFT                           8
#define PMIC_BUCK_VPA_SFCHG_REN_ADDR                              MT6353_BUCK_VPA_SFCHG_CON0
#define PMIC_BUCK_VPA_SFCHG_REN_MASK                              0x1
#define PMIC_BUCK_VPA_SFCHG_REN_SHIFT                             15
#define PMIC_DA_NI_VPA_VOSEL_ADDR                                 MT6353_BUCK_VPA_VOL_CON0
#define PMIC_DA_NI_VPA_VOSEL_MASK                                 0x3F
#define PMIC_DA_NI_VPA_VOSEL_SHIFT                                0
#define PMIC_BUCK_VPA_VOSEL_ADDR                                  MT6353_BUCK_VPA_VOL_CON1
#define PMIC_BUCK_VPA_VOSEL_MASK                                  0x3F
#define PMIC_BUCK_VPA_VOSEL_SHIFT                                 0
#define PMIC_BUCK_VPA_TRANS_TD_ADDR                               MT6353_BUCK_VPA_LPW_CON0
#define PMIC_BUCK_VPA_TRANS_TD_MASK                               0x3
#define PMIC_BUCK_VPA_TRANS_TD_SHIFT                              0
#define PMIC_BUCK_VPA_TRANS_CTRL_ADDR                             MT6353_BUCK_VPA_LPW_CON0
#define PMIC_BUCK_VPA_TRANS_CTRL_MASK                             0x3
#define PMIC_BUCK_VPA_TRANS_CTRL_SHIFT                            4
#define PMIC_BUCK_VPA_TRANS_ONCE_ADDR                             MT6353_BUCK_VPA_LPW_CON0
#define PMIC_BUCK_VPA_TRANS_ONCE_MASK                             0x1
#define PMIC_BUCK_VPA_TRANS_ONCE_SHIFT                            6
#define PMIC_DA_NI_VPA_DVS_TRANST_ADDR                            MT6353_BUCK_VPA_LPW_CON0
#define PMIC_DA_NI_VPA_DVS_TRANST_MASK                            0x1
#define PMIC_DA_NI_VPA_DVS_TRANST_SHIFT                           7
#define PMIC_BUCK_VPA_DVS_BW_TD_ADDR                              MT6353_BUCK_VPA_LPW_CON0
#define PMIC_BUCK_VPA_DVS_BW_TD_MASK                              0x3
#define PMIC_BUCK_VPA_DVS_BW_TD_SHIFT                             8
#define PMIC_BUCK_VPA_DVS_BW_CTRL_ADDR                            MT6353_BUCK_VPA_LPW_CON0
#define PMIC_BUCK_VPA_DVS_BW_CTRL_MASK                            0x3
#define PMIC_BUCK_VPA_DVS_BW_CTRL_SHIFT                           12
#define PMIC_BUCK_VPA_DVS_BW_ONCE_ADDR                            MT6353_BUCK_VPA_LPW_CON0
#define PMIC_BUCK_VPA_DVS_BW_ONCE_MASK                            0x1
#define PMIC_BUCK_VPA_DVS_BW_ONCE_SHIFT                           14
#define PMIC_DA_NI_VPA_DVS_BW_ADDR                                MT6353_BUCK_VPA_LPW_CON0
#define PMIC_DA_NI_VPA_DVS_BW_MASK                                0x1
#define PMIC_DA_NI_VPA_DVS_BW_SHIFT                               15
#define PMIC_BUCK_K_RST_DONE_ADDR                                 MT6353_BUCK_K_CON0
#define PMIC_BUCK_K_RST_DONE_MASK                                 0x1
#define PMIC_BUCK_K_RST_DONE_SHIFT                                0
#define PMIC_BUCK_K_MAP_SEL_ADDR                                  MT6353_BUCK_K_CON0
#define PMIC_BUCK_K_MAP_SEL_MASK                                  0x1
#define PMIC_BUCK_K_MAP_SEL_SHIFT                                 1
#define PMIC_BUCK_K_ONCE_EN_ADDR                                  MT6353_BUCK_K_CON0
#define PMIC_BUCK_K_ONCE_EN_MASK                                  0x1
#define PMIC_BUCK_K_ONCE_EN_SHIFT                                 2
#define PMIC_BUCK_K_ONCE_ADDR                                     MT6353_BUCK_K_CON0
#define PMIC_BUCK_K_ONCE_MASK                                     0x1
#define PMIC_BUCK_K_ONCE_SHIFT                                    3
#define PMIC_BUCK_K_START_MANUAL_ADDR                             MT6353_BUCK_K_CON0
#define PMIC_BUCK_K_START_MANUAL_MASK                             0x1
#define PMIC_BUCK_K_START_MANUAL_SHIFT                            4
#define PMIC_BUCK_K_SRC_SEL_ADDR                                  MT6353_BUCK_K_CON0
#define PMIC_BUCK_K_SRC_SEL_MASK                                  0x1
#define PMIC_BUCK_K_SRC_SEL_SHIFT                                 5
#define PMIC_BUCK_K_AUTO_EN_ADDR                                  MT6353_BUCK_K_CON0
#define PMIC_BUCK_K_AUTO_EN_MASK                                  0x1
#define PMIC_BUCK_K_AUTO_EN_SHIFT                                 6
#define PMIC_BUCK_K_INV_ADDR                                      MT6353_BUCK_K_CON0
#define PMIC_BUCK_K_INV_MASK                                      0x1
#define PMIC_BUCK_K_INV_SHIFT                                     7
#define PMIC_BUCK_K_CONTROL_SMPS_ADDR                             MT6353_BUCK_K_CON1
#define PMIC_BUCK_K_CONTROL_SMPS_MASK                             0x3F
#define PMIC_BUCK_K_CONTROL_SMPS_SHIFT                            8
#define PMIC_K_RESULT_ADDR                                        MT6353_BUCK_K_CON2
#define PMIC_K_RESULT_MASK                                        0x1
#define PMIC_K_RESULT_SHIFT                                       0
#define PMIC_K_DONE_ADDR                                          MT6353_BUCK_K_CON2
#define PMIC_K_DONE_MASK                                          0x1
#define PMIC_K_DONE_SHIFT                                         1
#define PMIC_K_CONTROL_ADDR                                       MT6353_BUCK_K_CON2
#define PMIC_K_CONTROL_MASK                                       0x3F
#define PMIC_K_CONTROL_SHIFT                                      2
#define PMIC_DA_QI_SMPS_OSC_CAL_ADDR                              MT6353_BUCK_K_CON2
#define PMIC_DA_QI_SMPS_OSC_CAL_MASK                              0x3F
#define PMIC_DA_QI_SMPS_OSC_CAL_SHIFT                             8
#define PMIC_BUCK_K_BUCK_CK_CNT_ADDR                              MT6353_BUCK_K_CON3
#define PMIC_BUCK_K_BUCK_CK_CNT_MASK                              0x3FF
#define PMIC_BUCK_K_BUCK_CK_CNT_SHIFT                             0
#define PMIC_WDTDBG_CLR_ADDR                                      MT6353_WDTDBG_CON0
#define PMIC_WDTDBG_CLR_MASK                                      0x1
#define PMIC_WDTDBG_CLR_SHIFT                                     0
#define PMIC_WDTDBG_CON0_RSV0_ADDR                                MT6353_WDTDBG_CON0
#define PMIC_WDTDBG_CON0_RSV0_MASK                                0x1
#define PMIC_WDTDBG_CON0_RSV0_SHIFT                               1
#define PMIC_VPROC_VOSEL_WDTDBG_ADDR                              MT6353_WDTDBG_CON1
#define PMIC_VPROC_VOSEL_WDTDBG_MASK                              0x7F
#define PMIC_VPROC_VOSEL_WDTDBG_SHIFT                             0
#define PMIC_VCORE_VOSEL_WDTDBG_ADDR                              MT6353_WDTDBG_CON1
#define PMIC_VCORE_VOSEL_WDTDBG_MASK                              0x7F
#define PMIC_VCORE_VOSEL_WDTDBG_SHIFT                             8
#define PMIC_VPA_VOSEL_WDTDBG_ADDR                                MT6353_WDTDBG_CON2
#define PMIC_VPA_VOSEL_WDTDBG_MASK                                0x3F
#define PMIC_VPA_VOSEL_WDTDBG_SHIFT                               0
#define PMIC_VS1_VOSEL_WDTDBG_ADDR                                MT6353_WDTDBG_CON2
#define PMIC_VS1_VOSEL_WDTDBG_MASK                                0x7F
#define PMIC_VS1_VOSEL_WDTDBG_SHIFT                               8
#define PMIC_VSRAM_PROC_VOSEL_WDTDBG_ADDR                         MT6353_WDTDBG_CON3
#define PMIC_VSRAM_PROC_VOSEL_WDTDBG_MASK                         0x7F
#define PMIC_VSRAM_PROC_VOSEL_WDTDBG_SHIFT                        0
#define PMIC_RG_AUDZCDENABLE_ADDR                                 MT6353_ZCD_CON0
#define PMIC_RG_AUDZCDENABLE_MASK                                 0x1
#define PMIC_RG_AUDZCDENABLE_SHIFT                                0
#define PMIC_RG_AUDZCDGAINSTEPTIME_ADDR                           MT6353_ZCD_CON0
#define PMIC_RG_AUDZCDGAINSTEPTIME_MASK                           0x7
#define PMIC_RG_AUDZCDGAINSTEPTIME_SHIFT                          1
#define PMIC_RG_AUDZCDGAINSTEPSIZE_ADDR                           MT6353_ZCD_CON0
#define PMIC_RG_AUDZCDGAINSTEPSIZE_MASK                           0x3
#define PMIC_RG_AUDZCDGAINSTEPSIZE_SHIFT                          4
#define PMIC_RG_AUDZCDTIMEOUTMODESEL_ADDR                         MT6353_ZCD_CON0
#define PMIC_RG_AUDZCDTIMEOUTMODESEL_MASK                         0x1
#define PMIC_RG_AUDZCDTIMEOUTMODESEL_SHIFT                        6
#define PMIC_RG_AUDZCDCLKSEL_VAUDP15_ADDR                         MT6353_ZCD_CON0
#define PMIC_RG_AUDZCDCLKSEL_VAUDP15_MASK                         0x1
#define PMIC_RG_AUDZCDCLKSEL_VAUDP15_SHIFT                        7
#define PMIC_RG_AUDZCDMUXSEL_VAUDP15_ADDR                         MT6353_ZCD_CON0
#define PMIC_RG_AUDZCDMUXSEL_VAUDP15_MASK                         0x7
#define PMIC_RG_AUDZCDMUXSEL_VAUDP15_SHIFT                        8
#define PMIC_RG_AUDLOLGAIN_ADDR                                   MT6353_ZCD_CON1
#define PMIC_RG_AUDLOLGAIN_MASK                                   0x1F
#define PMIC_RG_AUDLOLGAIN_SHIFT                                  0
#define PMIC_RG_AUDLORGAIN_ADDR                                   MT6353_ZCD_CON1
#define PMIC_RG_AUDLORGAIN_MASK                                   0x1F
#define PMIC_RG_AUDLORGAIN_SHIFT                                  7
#define PMIC_RG_AUDHPLGAIN_ADDR                                   MT6353_ZCD_CON2
#define PMIC_RG_AUDHPLGAIN_MASK                                   0x1F
#define PMIC_RG_AUDHPLGAIN_SHIFT                                  0
#define PMIC_RG_AUDHPRGAIN_ADDR                                   MT6353_ZCD_CON2
#define PMIC_RG_AUDHPRGAIN_MASK                                   0x1F
#define PMIC_RG_AUDHPRGAIN_SHIFT                                  7
#define PMIC_RG_AUDHSGAIN_ADDR                                    MT6353_ZCD_CON3
#define PMIC_RG_AUDHSGAIN_MASK                                    0x1F
#define PMIC_RG_AUDHSGAIN_SHIFT                                   0
#define PMIC_RG_AUDIVLGAIN_ADDR                                   MT6353_ZCD_CON4
#define PMIC_RG_AUDIVLGAIN_MASK                                   0x7
#define PMIC_RG_AUDIVLGAIN_SHIFT                                  0
#define PMIC_RG_AUDIVRGAIN_ADDR                                   MT6353_ZCD_CON4
#define PMIC_RG_AUDIVRGAIN_MASK                                   0x7
#define PMIC_RG_AUDIVRGAIN_SHIFT                                  8
#define PMIC_RG_AUDINTGAIN1_ADDR                                  MT6353_ZCD_CON5
#define PMIC_RG_AUDINTGAIN1_MASK                                  0x3F
#define PMIC_RG_AUDINTGAIN1_SHIFT                                 0
#define PMIC_RG_AUDINTGAIN2_ADDR                                  MT6353_ZCD_CON5
#define PMIC_RG_AUDINTGAIN2_MASK                                  0x3F
#define PMIC_RG_AUDINTGAIN2_SHIFT                                 8
#define PMIC_LDO_LDO_RSV1_ADDR                                    MT6353_LDO_RSV_CON0
#define PMIC_LDO_LDO_RSV1_MASK                                    0x3FF
#define PMIC_LDO_LDO_RSV1_SHIFT                                   0
#define PMIC_LDO_LDO_RSV0_ADDR                                    MT6353_LDO_RSV_CON0
#define PMIC_LDO_LDO_RSV0_MASK                                    0x3F
#define PMIC_LDO_LDO_RSV0_SHIFT                                   10
#define PMIC_LDO_LDO_RSV1_RO_ADDR                                 MT6353_LDO_RSV_CON1
#define PMIC_LDO_LDO_RSV1_RO_MASK                                 0xFF
#define PMIC_LDO_LDO_RSV1_RO_SHIFT                                0
#define PMIC_LDO_LDO_RSV2_ADDR                                    MT6353_LDO_RSV_CON1
#define PMIC_LDO_LDO_RSV2_MASK                                    0xFF
#define PMIC_LDO_LDO_RSV2_SHIFT                                   8
#define PMIC_LDO_VAUX18_AUXADC_PWDB_EN_ADDR                       MT6353_LDO_TOP_CON0
#define PMIC_LDO_VAUX18_AUXADC_PWDB_EN_MASK                       0x1
#define PMIC_LDO_VAUX18_AUXADC_PWDB_EN_SHIFT                      0
#define PMIC_LDO_VIBR_THER_SDN_EN_ADDR                            MT6353_LDO_TOP_CON0
#define PMIC_LDO_VIBR_THER_SDN_EN_MASK                            0x1
#define PMIC_LDO_VIBR_THER_SDN_EN_SHIFT                           1
#define PMIC_LDO_TOP_CON0_RSV_ADDR                                MT6353_LDO_TOP_CON0
#define PMIC_LDO_TOP_CON0_RSV_MASK                                0x3FFF
#define PMIC_LDO_TOP_CON0_RSV_SHIFT                               2
#define PMIC_LDO_VTCXO24_SWITCH_ADDR                              MT6353_LDO_VTCXO24_SW_CON0
#define PMIC_LDO_VTCXO24_SWITCH_MASK                              0x1
#define PMIC_LDO_VTCXO24_SWITCH_SHIFT                             4
#define PMIC_LDO_VXO22_SWITCH_ADDR                                MT6353_LDO_VXO22_SW_CON0
#define PMIC_LDO_VXO22_SWITCH_MASK                                0x1
#define PMIC_LDO_VXO22_SWITCH_SHIFT                               4
#define PMIC_LDO_DEGTD_SEL_ADDR                                   MT6353_LDO_OCFB0
#define PMIC_LDO_DEGTD_SEL_MASK                                   0x1
#define PMIC_LDO_DEGTD_SEL_SHIFT                                  0
#define PMIC_LDO_VRTC_EN_ADDR                                     MT6353_VRTC_CON0
#define PMIC_LDO_VRTC_EN_MASK                                     0x1
#define PMIC_LDO_VRTC_EN_SHIFT                                    1
#define PMIC_DA_QI_VRTC_EN_ADDR                                   MT6353_VRTC_CON0
#define PMIC_DA_QI_VRTC_EN_MASK                                   0x1
#define PMIC_DA_QI_VRTC_EN_SHIFT                                  15
#define PMIC_LDO_VCN33_EN_BT_ADDR                                 MT6353_LDO_SHARE_VCN33_CON0
#define PMIC_LDO_VCN33_EN_BT_MASK                                 0x1
#define PMIC_LDO_VCN33_EN_BT_SHIFT                                1
#define PMIC_LDO_VCN33_EN_CTRL_BT_ADDR                            MT6353_LDO_SHARE_VCN33_CON0
#define PMIC_LDO_VCN33_EN_CTRL_BT_MASK                            0x1
#define PMIC_LDO_VCN33_EN_CTRL_BT_SHIFT                           3
#define PMIC_LDO_VCN33_EN_SEL_BT_ADDR                             MT6353_LDO_SHARE_VCN33_CON0
#define PMIC_LDO_VCN33_EN_SEL_BT_MASK                             0x7
#define PMIC_LDO_VCN33_EN_SEL_BT_SHIFT                            11
#define PMIC_LDO_VCN33_EN_WIFI_ADDR                               MT6353_LDO_SHARE_VCN33_CON1
#define PMIC_LDO_VCN33_EN_WIFI_MASK                               0x1
#define PMIC_LDO_VCN33_EN_WIFI_SHIFT                              1
#define PMIC_LDO_VCN33_EN_CTRL_WIFI_ADDR                          MT6353_LDO_SHARE_VCN33_CON1
#define PMIC_LDO_VCN33_EN_CTRL_WIFI_MASK                          0x1
#define PMIC_LDO_VCN33_EN_CTRL_WIFI_SHIFT                         3
#define PMIC_LDO_VCN33_EN_SEL_WIFI_ADDR                           MT6353_LDO_SHARE_VCN33_CON1
#define PMIC_LDO_VCN33_EN_SEL_WIFI_MASK                           0x7
#define PMIC_LDO_VCN33_EN_SEL_WIFI_SHIFT                          11
#define PMIC_LDO_VCN33_LP_MODE_ADDR                               MT6353_LDO3_VCN33_CON0
#define PMIC_LDO_VCN33_LP_MODE_MASK                               0x1
#define PMIC_LDO_VCN33_LP_MODE_SHIFT                              0
#define PMIC_LDO_VCN33_LP_CTRL_ADDR                               MT6353_LDO3_VCN33_CON0
#define PMIC_LDO_VCN33_LP_CTRL_MASK                               0x1
#define PMIC_LDO_VCN33_LP_CTRL_SHIFT                              2
#define PMIC_LDO_VCN33_LP_SEL_ADDR                                MT6353_LDO3_VCN33_CON0
#define PMIC_LDO_VCN33_LP_SEL_MASK                                0x7
#define PMIC_LDO_VCN33_LP_SEL_SHIFT                               5
#define PMIC_DA_QI_VCN33_MODE_ADDR                                MT6353_LDO3_VCN33_CON0
#define PMIC_DA_QI_VCN33_MODE_MASK                                0x1
#define PMIC_DA_QI_VCN33_MODE_SHIFT                               8
#define PMIC_LDO_VCN33_STBTD_ADDR                                 MT6353_LDO3_VCN33_CON0
#define PMIC_LDO_VCN33_STBTD_MASK                                 0x1
#define PMIC_LDO_VCN33_STBTD_SHIFT                                9
#define PMIC_DA_QI_VCN33_STB_ADDR                                 MT6353_LDO3_VCN33_CON0
#define PMIC_DA_QI_VCN33_STB_MASK                                 0x1
#define PMIC_DA_QI_VCN33_STB_SHIFT                                14
#define PMIC_DA_QI_VCN33_EN_ADDR                                  MT6353_LDO3_VCN33_CON0
#define PMIC_DA_QI_VCN33_EN_MASK                                  0x1
#define PMIC_DA_QI_VCN33_EN_SHIFT                                 15
#define PMIC_LDO_VCN33_OCFB_EN_ADDR                               MT6353_LDO3_VCN33_OCFB_CON0
#define PMIC_LDO_VCN33_OCFB_EN_MASK                               0x1
#define PMIC_LDO_VCN33_OCFB_EN_SHIFT                              9
#define PMIC_DA_QI_VCN33_OCFB_EN_ADDR                             MT6353_LDO3_VCN33_OCFB_CON0
#define PMIC_DA_QI_VCN33_OCFB_EN_MASK                             0x1
#define PMIC_DA_QI_VCN33_OCFB_EN_SHIFT                            10
#define PMIC_LDO_VLDO28_EN_0_ADDR                                 MT6353_LDO_SHARE_VLDO28_CON0
#define PMIC_LDO_VLDO28_EN_0_MASK                                 0x1
#define PMIC_LDO_VLDO28_EN_0_SHIFT                                1
#define PMIC_LDO_VLDO28_EN_1_ADDR                                 MT6353_LDO_SHARE_VLDO28_CON1
#define PMIC_LDO_VLDO28_EN_1_MASK                                 0x1
#define PMIC_LDO_VLDO28_EN_1_SHIFT                                1
#define PMIC_LDO_VLDO28_LP_MODE_ADDR                              MT6353_LDO1_VLDO28_CON0
#define PMIC_LDO_VLDO28_LP_MODE_MASK                              0x1
#define PMIC_LDO_VLDO28_LP_MODE_SHIFT                             0
#define PMIC_LDO_VLDO28_LP_CTRL_ADDR                              MT6353_LDO1_VLDO28_CON0
#define PMIC_LDO_VLDO28_LP_CTRL_MASK                              0x1
#define PMIC_LDO_VLDO28_LP_CTRL_SHIFT                             2
#define PMIC_LDO_VLDO28_LP_SEL_ADDR                               MT6353_LDO1_VLDO28_CON0
#define PMIC_LDO_VLDO28_LP_SEL_MASK                               0x7
#define PMIC_LDO_VLDO28_LP_SEL_SHIFT                              5
#define PMIC_DA_QI_VLDO28_MODE_ADDR                               MT6353_LDO1_VLDO28_CON0
#define PMIC_DA_QI_VLDO28_MODE_MASK                               0x1
#define PMIC_DA_QI_VLDO28_MODE_SHIFT                              8
#define PMIC_LDO_VLDO28_STBTD_ADDR                                MT6353_LDO1_VLDO28_CON0
#define PMIC_LDO_VLDO28_STBTD_MASK                                0x1
#define PMIC_LDO_VLDO28_STBTD_SHIFT                               9
#define PMIC_DA_QI_VLDO28_STB_ADDR                                MT6353_LDO1_VLDO28_CON0
#define PMIC_DA_QI_VLDO28_STB_MASK                                0x1
#define PMIC_DA_QI_VLDO28_STB_SHIFT                               14
#define PMIC_DA_QI_VLDO28_EN_ADDR                                 MT6353_LDO1_VLDO28_CON0
#define PMIC_DA_QI_VLDO28_EN_MASK                                 0x1
#define PMIC_DA_QI_VLDO28_EN_SHIFT                                15
#define PMIC_LDO_VLDO28_OCFB_EN_ADDR                              MT6353_LDO1_VLDO28_OCFB_CON0
#define PMIC_LDO_VLDO28_OCFB_EN_MASK                              0x1
#define PMIC_LDO_VLDO28_OCFB_EN_SHIFT                             9
#define PMIC_DA_QI_VLDO28_OCFB_EN_ADDR                            MT6353_LDO1_VLDO28_OCFB_CON0
#define PMIC_DA_QI_VLDO28_OCFB_EN_MASK                            0x1
#define PMIC_DA_QI_VLDO28_OCFB_EN_SHIFT                           10
#define PMIC_LDO_VLDO28_FAST_TRAN_DL_EN_ADDR                      MT6353_LDO_VLDO28_FAST_TRAN_CON0
#define PMIC_LDO_VLDO28_FAST_TRAN_DL_EN_MASK                      0x1
#define PMIC_LDO_VLDO28_FAST_TRAN_DL_EN_SHIFT                     0
#define PMIC_LDO_VLDO28_FAST_TRAN_DL_CTRL_ADDR                    MT6353_LDO_VLDO28_FAST_TRAN_CON0
#define PMIC_LDO_VLDO28_FAST_TRAN_DL_CTRL_MASK                    0x1
#define PMIC_LDO_VLDO28_FAST_TRAN_DL_CTRL_SHIFT                   1
#define PMIC_LDO_VLDO28_FAST_TRAN_DL_SRCLKEN_SEL_ADDR             MT6353_LDO_VLDO28_FAST_TRAN_CON0
#define PMIC_LDO_VLDO28_FAST_TRAN_DL_SRCLKEN_SEL_MASK             0x7
#define PMIC_LDO_VLDO28_FAST_TRAN_DL_SRCLKEN_SEL_SHIFT            2
#define PMIC_DA_QI_VLDO28_FAST_TRAN_DL_EN_ADDR                    MT6353_LDO_VLDO28_FAST_TRAN_CON0
#define PMIC_DA_QI_VLDO28_FAST_TRAN_DL_EN_MASK                    0x1
#define PMIC_DA_QI_VLDO28_FAST_TRAN_DL_EN_SHIFT                   5
#define PMIC_LDO_VLDO28_FAST_TRAN_CL_EN_ADDR                      MT6353_LDO_VLDO28_FAST_TRAN_CON0
#define PMIC_LDO_VLDO28_FAST_TRAN_CL_EN_MASK                      0x1
#define PMIC_LDO_VLDO28_FAST_TRAN_CL_EN_SHIFT                     8
#define PMIC_LDO_VLDO28_FAST_TRAN_CL_CTRL_ADDR                    MT6353_LDO_VLDO28_FAST_TRAN_CON0
#define PMIC_LDO_VLDO28_FAST_TRAN_CL_CTRL_MASK                    0x1
#define PMIC_LDO_VLDO28_FAST_TRAN_CL_CTRL_SHIFT                   9
#define PMIC_LDO_VLDO28_FAST_TRAN_CL_SRCLKEN_SEL_ADDR             MT6353_LDO_VLDO28_FAST_TRAN_CON0
#define PMIC_LDO_VLDO28_FAST_TRAN_CL_SRCLKEN_SEL_MASK             0x7
#define PMIC_LDO_VLDO28_FAST_TRAN_CL_SRCLKEN_SEL_SHIFT            10
#define PMIC_DA_QI_VLDO28_FAST_TRAN_CL_EN_ADDR                    MT6353_LDO_VLDO28_FAST_TRAN_CON0
#define PMIC_DA_QI_VLDO28_FAST_TRAN_CL_EN_MASK                    0x1
#define PMIC_DA_QI_VLDO28_FAST_TRAN_CL_EN_SHIFT                   13
#define PMIC_LDO_VSRAM_PROC_VOSEL_CTRL_ADDR                       MT6353_LDO_VSRAM_PROC_HWM_CON0
#define PMIC_LDO_VSRAM_PROC_VOSEL_CTRL_MASK                       0x1
#define PMIC_LDO_VSRAM_PROC_VOSEL_CTRL_SHIFT                      1
#define PMIC_LDO_VSRAM_PROC_VOSEL_SEL_ADDR                        MT6353_LDO_VSRAM_PROC_HWM_CON0
#define PMIC_LDO_VSRAM_PROC_VOSEL_SEL_MASK                        0x7
#define PMIC_LDO_VSRAM_PROC_VOSEL_SEL_SHIFT                       3
#define PMIC_LDO_VSRAM_PROC_SFCHG_FRATE_ADDR                      MT6353_LDO_VSRAM_PROC_SFCHG_CON0
#define PMIC_LDO_VSRAM_PROC_SFCHG_FRATE_MASK                      0x7F
#define PMIC_LDO_VSRAM_PROC_SFCHG_FRATE_SHIFT                     0
#define PMIC_LDO_VSRAM_PROC_SFCHG_FEN_ADDR                        MT6353_LDO_VSRAM_PROC_SFCHG_CON0
#define PMIC_LDO_VSRAM_PROC_SFCHG_FEN_MASK                        0x1
#define PMIC_LDO_VSRAM_PROC_SFCHG_FEN_SHIFT                       7
#define PMIC_LDO_VSRAM_PROC_SFCHG_RRATE_ADDR                      MT6353_LDO_VSRAM_PROC_SFCHG_CON0
#define PMIC_LDO_VSRAM_PROC_SFCHG_RRATE_MASK                      0x7F
#define PMIC_LDO_VSRAM_PROC_SFCHG_RRATE_SHIFT                     8
#define PMIC_LDO_VSRAM_PROC_SFCHG_REN_ADDR                        MT6353_LDO_VSRAM_PROC_SFCHG_CON0
#define PMIC_LDO_VSRAM_PROC_SFCHG_REN_MASK                        0x1
#define PMIC_LDO_VSRAM_PROC_SFCHG_REN_SHIFT                       15
#define PMIC_DA_NI_VSRAM_PROC_VOSEL_ADDR                          MT6353_LDO_VSRAM_PROC_VOL_CON0
#define PMIC_DA_NI_VSRAM_PROC_VOSEL_MASK                          0x7F
#define PMIC_DA_NI_VSRAM_PROC_VOSEL_SHIFT                         0
#define PMIC_LDO_VSRAM_PROC_VOSEL_ADDR                            MT6353_LDO_VSRAM_PROC_VOL_CON1
#define PMIC_LDO_VSRAM_PROC_VOSEL_MASK                            0x7F
#define PMIC_LDO_VSRAM_PROC_VOSEL_SHIFT                           0
#define PMIC_LDO_VSRAM_PROC_VOSEL_ON_ADDR                         MT6353_LDO_VSRAM_PROC_VOL_CON2
#define PMIC_LDO_VSRAM_PROC_VOSEL_ON_MASK                         0x7F
#define PMIC_LDO_VSRAM_PROC_VOSEL_ON_SHIFT                        0
#define PMIC_LDO_VSRAM_PROC_VOSEL_SLEEP_ADDR                      MT6353_LDO_VSRAM_PROC_VOL_CON3
#define PMIC_LDO_VSRAM_PROC_VOSEL_SLEEP_MASK                      0x7F
#define PMIC_LDO_VSRAM_PROC_VOSEL_SLEEP_SHIFT                     0
#define PMIC_LDO_VSRAM_PROC_TRANS_TD_ADDR                         MT6353_LDO_VSRAM_PROC_LPW_CON0
#define PMIC_LDO_VSRAM_PROC_TRANS_TD_MASK                         0x3
#define PMIC_LDO_VSRAM_PROC_TRANS_TD_SHIFT                        0
#define PMIC_LDO_VSRAM_PROC_TRANS_CTRL_ADDR                       MT6353_LDO_VSRAM_PROC_LPW_CON0
#define PMIC_LDO_VSRAM_PROC_TRANS_CTRL_MASK                       0x3
#define PMIC_LDO_VSRAM_PROC_TRANS_CTRL_SHIFT                      4
#define PMIC_LDO_VSRAM_PROC_TRANS_ONCE_ADDR                       MT6353_LDO_VSRAM_PROC_LPW_CON0
#define PMIC_LDO_VSRAM_PROC_TRANS_ONCE_MASK                       0x1
#define PMIC_LDO_VSRAM_PROC_TRANS_ONCE_SHIFT                      6
#define PMIC_DA_QI_VSRAM_PROC_DVS_EN_ADDR                         MT6353_LDO_VSRAM_PROC_LPW_CON0
#define PMIC_DA_QI_VSRAM_PROC_DVS_EN_MASK                         0x1
#define PMIC_DA_QI_VSRAM_PROC_DVS_EN_SHIFT                        7
#define PMIC_LDO_VSRAM_PROC_VSLEEP_EN_ADDR                        MT6353_LDO_VSRAM_PROC_LPW_CON0
#define PMIC_LDO_VSRAM_PROC_VSLEEP_EN_MASK                        0x1
#define PMIC_LDO_VSRAM_PROC_VSLEEP_EN_SHIFT                       8
#define PMIC_LDO_VSRAM_PROC_R2R_PDN_ADDR                          MT6353_LDO_VSRAM_PROC_LPW_CON0
#define PMIC_LDO_VSRAM_PROC_R2R_PDN_MASK                          0x1
#define PMIC_LDO_VSRAM_PROC_R2R_PDN_SHIFT                         10
#define PMIC_LDO_VSRAM_PROC_VSLEEP_SEL_ADDR                       MT6353_LDO_VSRAM_PROC_LPW_CON0
#define PMIC_LDO_VSRAM_PROC_VSLEEP_SEL_MASK                       0x1
#define PMIC_LDO_VSRAM_PROC_VSLEEP_SEL_SHIFT                      11
#define PMIC_DA_NI_VSRAM_PROC_R2R_PDN_ADDR                        MT6353_LDO_VSRAM_PROC_LPW_CON0
#define PMIC_DA_NI_VSRAM_PROC_R2R_PDN_MASK                        0x1
#define PMIC_DA_NI_VSRAM_PROC_R2R_PDN_SHIFT                       14
#define PMIC_DA_NI_VSRAM_PROC_VSLEEP_SEL_ADDR                     MT6353_LDO_VSRAM_PROC_LPW_CON0
#define PMIC_DA_NI_VSRAM_PROC_VSLEEP_SEL_MASK                     0x1
#define PMIC_DA_NI_VSRAM_PROC_VSLEEP_SEL_SHIFT                    15
#define PMIC_LDO_BATON_HT_EN_ADDR                                 MT6353_LDO_BATON_CON0
#define PMIC_LDO_BATON_HT_EN_MASK                                 0x1
#define PMIC_LDO_BATON_HT_EN_SHIFT                                0
#define PMIC_LDO_BATON_HT_EN_DLY_TIME_ADDR                        MT6353_LDO_BATON_CON0
#define PMIC_LDO_BATON_HT_EN_DLY_TIME_MASK                        0x1
#define PMIC_LDO_BATON_HT_EN_DLY_TIME_SHIFT                       4
#define PMIC_DA_QI_BATON_HT_EN_ADDR                               MT6353_LDO_BATON_CON0
#define PMIC_DA_QI_BATON_HT_EN_MASK                               0x1
#define PMIC_DA_QI_BATON_HT_EN_SHIFT                              5
#define PMIC_LDO_TREF_EN_ADDR                                     MT6353_LDO2_TREF_CON0
#define PMIC_LDO_TREF_EN_MASK                                     0x1
#define PMIC_LDO_TREF_EN_SHIFT                                    1
#define PMIC_LDO_TREF_EN_CTRL_ADDR                                MT6353_LDO2_TREF_CON0
#define PMIC_LDO_TREF_EN_CTRL_MASK                                0x1
#define PMIC_LDO_TREF_EN_CTRL_SHIFT                               3
#define PMIC_LDO_TREF_EN_SEL_ADDR                                 MT6353_LDO2_TREF_CON0
#define PMIC_LDO_TREF_EN_SEL_MASK                                 0x7
#define PMIC_LDO_TREF_EN_SEL_SHIFT                                11
#define PMIC_DA_QI_TREF_EN_ADDR                                   MT6353_LDO2_TREF_CON0
#define PMIC_DA_QI_TREF_EN_MASK                                   0x1
#define PMIC_DA_QI_TREF_EN_SHIFT                                  15
#define PMIC_LDO_VTCXO28_LP_MODE_ADDR                             MT6353_LDO3_VTCXO28_CON0
#define PMIC_LDO_VTCXO28_LP_MODE_MASK                             0x1
#define PMIC_LDO_VTCXO28_LP_MODE_SHIFT                            0
#define PMIC_LDO_VTCXO28_EN_ADDR                                  MT6353_LDO3_VTCXO28_CON0
#define PMIC_LDO_VTCXO28_EN_MASK                                  0x1
#define PMIC_LDO_VTCXO28_EN_SHIFT                                 1
#define PMIC_LDO_VTCXO28_LP_CTRL_ADDR                             MT6353_LDO3_VTCXO28_CON0
#define PMIC_LDO_VTCXO28_LP_CTRL_MASK                             0x1
#define PMIC_LDO_VTCXO28_LP_CTRL_SHIFT                            2
#define PMIC_LDO_VTCXO28_EN_CTRL_ADDR                             MT6353_LDO3_VTCXO28_CON0
#define PMIC_LDO_VTCXO28_EN_CTRL_MASK                             0x1
#define PMIC_LDO_VTCXO28_EN_CTRL_SHIFT                            3
#define PMIC_LDO_VTCXO28_LP_SEL_ADDR                              MT6353_LDO3_VTCXO28_CON0
#define PMIC_LDO_VTCXO28_LP_SEL_MASK                              0x7
#define PMIC_LDO_VTCXO28_LP_SEL_SHIFT                             5
#define PMIC_DA_QI_VTCXO28_MODE_ADDR                              MT6353_LDO3_VTCXO28_CON0
#define PMIC_DA_QI_VTCXO28_MODE_MASK                              0x1
#define PMIC_DA_QI_VTCXO28_MODE_SHIFT                             8
#define PMIC_LDO_VTCXO28_STBTD_ADDR                               MT6353_LDO3_VTCXO28_CON0
#define PMIC_LDO_VTCXO28_STBTD_MASK                               0x1
#define PMIC_LDO_VTCXO28_STBTD_SHIFT                              9
#define PMIC_LDO_VTCXO28_EN_SEL_ADDR                              MT6353_LDO3_VTCXO28_CON0
#define PMIC_LDO_VTCXO28_EN_SEL_MASK                              0x7
#define PMIC_LDO_VTCXO28_EN_SEL_SHIFT                             11
#define PMIC_DA_QI_VTCXO28_STB_ADDR                               MT6353_LDO3_VTCXO28_CON0
#define PMIC_DA_QI_VTCXO28_STB_MASK                               0x1
#define PMIC_DA_QI_VTCXO28_STB_SHIFT                              14
#define PMIC_DA_QI_VTCXO28_EN_ADDR                                MT6353_LDO3_VTCXO28_CON0
#define PMIC_DA_QI_VTCXO28_EN_MASK                                0x1
#define PMIC_DA_QI_VTCXO28_EN_SHIFT                               15
#define PMIC_LDO_VTCXO28_OCFB_EN_ADDR                             MT6353_LDO3_VTCXO28_OCFB_CON0
#define PMIC_LDO_VTCXO28_OCFB_EN_MASK                             0x1
#define PMIC_LDO_VTCXO28_OCFB_EN_SHIFT                            9
#define PMIC_DA_QI_VTCXO28_OCFB_EN_ADDR                           MT6353_LDO3_VTCXO28_OCFB_CON0
#define PMIC_DA_QI_VTCXO28_OCFB_EN_MASK                           0x1
#define PMIC_DA_QI_VTCXO28_OCFB_EN_SHIFT                          10
#define PMIC_LDO_VTCXO24_LP_MODE_ADDR                             MT6353_LDO3_VTCXO24_CON0
#define PMIC_LDO_VTCXO24_LP_MODE_MASK                             0x1
#define PMIC_LDO_VTCXO24_LP_MODE_SHIFT                            0
#define PMIC_LDO_VTCXO24_EN_ADDR                                  MT6353_LDO3_VTCXO24_CON0
#define PMIC_LDO_VTCXO24_EN_MASK                                  0x1
#define PMIC_LDO_VTCXO24_EN_SHIFT                                 1
#define PMIC_LDO_VTCXO24_LP_CTRL_ADDR                             MT6353_LDO3_VTCXO24_CON0
#define PMIC_LDO_VTCXO24_LP_CTRL_MASK                             0x1
#define PMIC_LDO_VTCXO24_LP_CTRL_SHIFT                            2
#define PMIC_LDO_VTCXO24_EN_CTRL_ADDR                             MT6353_LDO3_VTCXO24_CON0
#define PMIC_LDO_VTCXO24_EN_CTRL_MASK                             0x1
#define PMIC_LDO_VTCXO24_EN_CTRL_SHIFT                            3
#define PMIC_LDO_VTCXO24_LP_SEL_ADDR                              MT6353_LDO3_VTCXO24_CON0
#define PMIC_LDO_VTCXO24_LP_SEL_MASK                              0x7
#define PMIC_LDO_VTCXO24_LP_SEL_SHIFT                             5
#define PMIC_DA_QI_VTCXO24_MODE_ADDR                              MT6353_LDO3_VTCXO24_CON0
#define PMIC_DA_QI_VTCXO24_MODE_MASK                              0x1
#define PMIC_DA_QI_VTCXO24_MODE_SHIFT                             8
#define PMIC_LDO_VTCXO24_STBTD_ADDR                               MT6353_LDO3_VTCXO24_CON0
#define PMIC_LDO_VTCXO24_STBTD_MASK                               0x1
#define PMIC_LDO_VTCXO24_STBTD_SHIFT                              9
#define PMIC_LDO_VTCXO24_EN_SEL_ADDR                              MT6353_LDO3_VTCXO24_CON0
#define PMIC_LDO_VTCXO24_EN_SEL_MASK                              0x7
#define PMIC_LDO_VTCXO24_EN_SEL_SHIFT                             11
#define PMIC_DA_QI_VTCXO24_STB_ADDR                               MT6353_LDO3_VTCXO24_CON0
#define PMIC_DA_QI_VTCXO24_STB_MASK                               0x1
#define PMIC_DA_QI_VTCXO24_STB_SHIFT                              14
#define PMIC_DA_QI_VTCXO24_EN_ADDR                                MT6353_LDO3_VTCXO24_CON0
#define PMIC_DA_QI_VTCXO24_EN_MASK                                0x1
#define PMIC_DA_QI_VTCXO24_EN_SHIFT                               15
#define PMIC_LDO_VTCXO24_OCFB_EN_ADDR                             MT6353_LDO3_VTCXO24_OCFB_CON0
#define PMIC_LDO_VTCXO24_OCFB_EN_MASK                             0x1
#define PMIC_LDO_VTCXO24_OCFB_EN_SHIFT                            9
#define PMIC_DA_QI_VTCXO24_OCFB_EN_ADDR                           MT6353_LDO3_VTCXO24_OCFB_CON0
#define PMIC_DA_QI_VTCXO24_OCFB_EN_MASK                           0x1
#define PMIC_DA_QI_VTCXO24_OCFB_EN_SHIFT                          10
#define PMIC_LDO_VXO22_LP_MODE_ADDR                               MT6353_LDO3_VXO22_CON0
#define PMIC_LDO_VXO22_LP_MODE_MASK                               0x1
#define PMIC_LDO_VXO22_LP_MODE_SHIFT                              0
#define PMIC_LDO_VXO22_EN_ADDR                                    MT6353_LDO3_VXO22_CON0
#define PMIC_LDO_VXO22_EN_MASK                                    0x1
#define PMIC_LDO_VXO22_EN_SHIFT                                   1
#define PMIC_LDO_VXO22_LP_CTRL_ADDR                               MT6353_LDO3_VXO22_CON0
#define PMIC_LDO_VXO22_LP_CTRL_MASK                               0x1
#define PMIC_LDO_VXO22_LP_CTRL_SHIFT                              2
#define PMIC_LDO_VXO22_EN_CTRL_ADDR                               MT6353_LDO3_VXO22_CON0
#define PMIC_LDO_VXO22_EN_CTRL_MASK                               0x1
#define PMIC_LDO_VXO22_EN_CTRL_SHIFT                              3
#define PMIC_LDO_VXO22_LP_SEL_ADDR                                MT6353_LDO3_VXO22_CON0
#define PMIC_LDO_VXO22_LP_SEL_MASK                                0x7
#define PMIC_LDO_VXO22_LP_SEL_SHIFT                               5
#define PMIC_DA_QI_VXO22_MODE_ADDR                                MT6353_LDO3_VXO22_CON0
#define PMIC_DA_QI_VXO22_MODE_MASK                                0x1
#define PMIC_DA_QI_VXO22_MODE_SHIFT                               8
#define PMIC_LDO_VXO22_STBTD_ADDR                                 MT6353_LDO3_VXO22_CON0
#define PMIC_LDO_VXO22_STBTD_MASK                                 0x1
#define PMIC_LDO_VXO22_STBTD_SHIFT                                9
#define PMIC_LDO_VXO22_EN_SEL_ADDR                                MT6353_LDO3_VXO22_CON0
#define PMIC_LDO_VXO22_EN_SEL_MASK                                0x7
#define PMIC_LDO_VXO22_EN_SEL_SHIFT                               11
#define PMIC_DA_QI_VXO22_STB_ADDR                                 MT6353_LDO3_VXO22_CON0
#define PMIC_DA_QI_VXO22_STB_MASK                                 0x1
#define PMIC_DA_QI_VXO22_STB_SHIFT                                14
#define PMIC_DA_QI_VXO22_EN_ADDR                                  MT6353_LDO3_VXO22_CON0
#define PMIC_DA_QI_VXO22_EN_MASK                                  0x1
#define PMIC_DA_QI_VXO22_EN_SHIFT                                 15
#define PMIC_LDO_VXO22_OCFB_EN_ADDR                               MT6353_LDO3_VXO22_OCFB_CON0
#define PMIC_LDO_VXO22_OCFB_EN_MASK                               0x1
#define PMIC_LDO_VXO22_OCFB_EN_SHIFT                              9
#define PMIC_DA_QI_VXO22_OCFB_EN_ADDR                             MT6353_LDO3_VXO22_OCFB_CON0
#define PMIC_DA_QI_VXO22_OCFB_EN_MASK                             0x1
#define PMIC_DA_QI_VXO22_OCFB_EN_SHIFT                            10
#define PMIC_LDO_VRF18_LP_MODE_ADDR                               MT6353_LDO3_VRF18_CON0
#define PMIC_LDO_VRF18_LP_MODE_MASK                               0x1
#define PMIC_LDO_VRF18_LP_MODE_SHIFT                              0
#define PMIC_LDO_VRF18_EN_ADDR                                    MT6353_LDO3_VRF18_CON0
#define PMIC_LDO_VRF18_EN_MASK                                    0x1
#define PMIC_LDO_VRF18_EN_SHIFT                                   1
#define PMIC_LDO_VRF18_LP_CTRL_ADDR                               MT6353_LDO3_VRF18_CON0
#define PMIC_LDO_VRF18_LP_CTRL_MASK                               0x1
#define PMIC_LDO_VRF18_LP_CTRL_SHIFT                              2
#define PMIC_LDO_VRF18_EN_CTRL_ADDR                               MT6353_LDO3_VRF18_CON0
#define PMIC_LDO_VRF18_EN_CTRL_MASK                               0x1
#define PMIC_LDO_VRF18_EN_CTRL_SHIFT                              3
#define PMIC_LDO_VRF18_LP_SEL_ADDR                                MT6353_LDO3_VRF18_CON0
#define PMIC_LDO_VRF18_LP_SEL_MASK                                0x7
#define PMIC_LDO_VRF18_LP_SEL_SHIFT                               5
#define PMIC_DA_QI_VRF18_MODE_ADDR                                MT6353_LDO3_VRF18_CON0
#define PMIC_DA_QI_VRF18_MODE_MASK                                0x1
#define PMIC_DA_QI_VRF18_MODE_SHIFT                               8
#define PMIC_LDO_VRF18_STBTD_ADDR                                 MT6353_LDO3_VRF18_CON0
#define PMIC_LDO_VRF18_STBTD_MASK                                 0x1
#define PMIC_LDO_VRF18_STBTD_SHIFT                                9
#define PMIC_LDO_VRF18_EN_SEL_ADDR                                MT6353_LDO3_VRF18_CON0
#define PMIC_LDO_VRF18_EN_SEL_MASK                                0x7
#define PMIC_LDO_VRF18_EN_SEL_SHIFT                               11
#define PMIC_DA_QI_VRF18_STB_ADDR                                 MT6353_LDO3_VRF18_CON0
#define PMIC_DA_QI_VRF18_STB_MASK                                 0x1
#define PMIC_DA_QI_VRF18_STB_SHIFT                                14
#define PMIC_DA_QI_VRF18_EN_ADDR                                  MT6353_LDO3_VRF18_CON0
#define PMIC_DA_QI_VRF18_EN_MASK                                  0x1
#define PMIC_DA_QI_VRF18_EN_SHIFT                                 15
#define PMIC_LDO_VRF18_OCFB_EN_ADDR                               MT6353_LDO3_VRF18_OCFB_CON0
#define PMIC_LDO_VRF18_OCFB_EN_MASK                               0x1
#define PMIC_LDO_VRF18_OCFB_EN_SHIFT                              9
#define PMIC_DA_QI_VRF18_OCFB_EN_ADDR                             MT6353_LDO3_VRF18_OCFB_CON0
#define PMIC_DA_QI_VRF18_OCFB_EN_MASK                             0x1
#define PMIC_DA_QI_VRF18_OCFB_EN_SHIFT                            10
#define PMIC_LDO_VRF12_LP_MODE_ADDR                               MT6353_LDO3_VRF12_CON0
#define PMIC_LDO_VRF12_LP_MODE_MASK                               0x1
#define PMIC_LDO_VRF12_LP_MODE_SHIFT                              0
#define PMIC_LDO_VRF12_EN_ADDR                                    MT6353_LDO3_VRF12_CON0
#define PMIC_LDO_VRF12_EN_MASK                                    0x1
#define PMIC_LDO_VRF12_EN_SHIFT                                   1
#define PMIC_LDO_VRF12_LP_CTRL_ADDR                               MT6353_LDO3_VRF12_CON0
#define PMIC_LDO_VRF12_LP_CTRL_MASK                               0x1
#define PMIC_LDO_VRF12_LP_CTRL_SHIFT                              2
#define PMIC_LDO_VRF12_EN_CTRL_ADDR                               MT6353_LDO3_VRF12_CON0
#define PMIC_LDO_VRF12_EN_CTRL_MASK                               0x1
#define PMIC_LDO_VRF12_EN_CTRL_SHIFT                              3
#define PMIC_LDO_VRF12_LP_SEL_ADDR                                MT6353_LDO3_VRF12_CON0
#define PMIC_LDO_VRF12_LP_SEL_MASK                                0x7
#define PMIC_LDO_VRF12_LP_SEL_SHIFT                               5
#define PMIC_DA_QI_VRF12_MODE_ADDR                                MT6353_LDO3_VRF12_CON0
#define PMIC_DA_QI_VRF12_MODE_MASK                                0x1
#define PMIC_DA_QI_VRF12_MODE_SHIFT                               8
#define PMIC_LDO_VRF12_STBTD_ADDR                                 MT6353_LDO3_VRF12_CON0
#define PMIC_LDO_VRF12_STBTD_MASK                                 0x1
#define PMIC_LDO_VRF12_STBTD_SHIFT                                9
#define PMIC_LDO_VRF12_EN_SEL_ADDR                                MT6353_LDO3_VRF12_CON0
#define PMIC_LDO_VRF12_EN_SEL_MASK                                0x7
#define PMIC_LDO_VRF12_EN_SEL_SHIFT                               11
#define PMIC_DA_QI_VRF12_STB_ADDR                                 MT6353_LDO3_VRF12_CON0
#define PMIC_DA_QI_VRF12_STB_MASK                                 0x1
#define PMIC_DA_QI_VRF12_STB_SHIFT                                14
#define PMIC_DA_QI_VRF12_EN_ADDR                                  MT6353_LDO3_VRF12_CON0
#define PMIC_DA_QI_VRF12_EN_MASK                                  0x1
#define PMIC_DA_QI_VRF12_EN_SHIFT                                 15
#define PMIC_LDO_VRF12_OCFB_EN_ADDR                               MT6353_LDO3_VRF12_OCFB_CON0
#define PMIC_LDO_VRF12_OCFB_EN_MASK                               0x1
#define PMIC_LDO_VRF12_OCFB_EN_SHIFT                              9
#define PMIC_DA_QI_VRF12_OCFB_EN_ADDR                             MT6353_LDO3_VRF12_OCFB_CON0
#define PMIC_DA_QI_VRF12_OCFB_EN_MASK                             0x1
#define PMIC_DA_QI_VRF12_OCFB_EN_SHIFT                            10
#define PMIC_LDO_VRF12_FAST_TRAN_DL_EN_ADDR                       MT6353_LDO_VRF12_FAST_TRAN_CON0
#define PMIC_LDO_VRF12_FAST_TRAN_DL_EN_MASK                       0x1
#define PMIC_LDO_VRF12_FAST_TRAN_DL_EN_SHIFT                      0
#define PMIC_LDO_VRF12_FAST_TRAN_DL_CTRL_ADDR                     MT6353_LDO_VRF12_FAST_TRAN_CON0
#define PMIC_LDO_VRF12_FAST_TRAN_DL_CTRL_MASK                     0x1
#define PMIC_LDO_VRF12_FAST_TRAN_DL_CTRL_SHIFT                    1
#define PMIC_LDO_VRF12_FAST_TRAN_DL_SRCLKEN_SEL_ADDR              MT6353_LDO_VRF12_FAST_TRAN_CON0
#define PMIC_LDO_VRF12_FAST_TRAN_DL_SRCLKEN_SEL_MASK              0x7
#define PMIC_LDO_VRF12_FAST_TRAN_DL_SRCLKEN_SEL_SHIFT             2
#define PMIC_DA_QI_VRF12_FAST_TRAN_DL_EN_ADDR                     MT6353_LDO_VRF12_FAST_TRAN_CON0
#define PMIC_DA_QI_VRF12_FAST_TRAN_DL_EN_MASK                     0x1
#define PMIC_DA_QI_VRF12_FAST_TRAN_DL_EN_SHIFT                    5
#define PMIC_LDO_VRF12_FAST_TRAN_CL_EN_ADDR                       MT6353_LDO_VRF12_FAST_TRAN_CON0
#define PMIC_LDO_VRF12_FAST_TRAN_CL_EN_MASK                       0x1
#define PMIC_LDO_VRF12_FAST_TRAN_CL_EN_SHIFT                      8
#define PMIC_LDO_VRF12_FAST_TRAN_CL_CTRL_ADDR                     MT6353_LDO_VRF12_FAST_TRAN_CON0
#define PMIC_LDO_VRF12_FAST_TRAN_CL_CTRL_MASK                     0x1
#define PMIC_LDO_VRF12_FAST_TRAN_CL_CTRL_SHIFT                    9
#define PMIC_LDO_VRF12_FAST_TRAN_CL_SRCLKEN_SEL_ADDR              MT6353_LDO_VRF12_FAST_TRAN_CON0
#define PMIC_LDO_VRF12_FAST_TRAN_CL_SRCLKEN_SEL_MASK              0x7
#define PMIC_LDO_VRF12_FAST_TRAN_CL_SRCLKEN_SEL_SHIFT             10
#define PMIC_DA_QI_VRF12_FAST_TRAN_CL_EN_ADDR                     MT6353_LDO_VRF12_FAST_TRAN_CON0
#define PMIC_DA_QI_VRF12_FAST_TRAN_CL_EN_MASK                     0x1
#define PMIC_DA_QI_VRF12_FAST_TRAN_CL_EN_SHIFT                    13
#define PMIC_LDO_VCN28_LP_MODE_ADDR                               MT6353_LDO3_VCN28_CON0
#define PMIC_LDO_VCN28_LP_MODE_MASK                               0x1
#define PMIC_LDO_VCN28_LP_MODE_SHIFT                              0
#define PMIC_LDO_VCN28_EN_ADDR                                    MT6353_LDO3_VCN28_CON0
#define PMIC_LDO_VCN28_EN_MASK                                    0x1
#define PMIC_LDO_VCN28_EN_SHIFT                                   1
#define PMIC_LDO_VCN28_LP_CTRL_ADDR                               MT6353_LDO3_VCN28_CON0
#define PMIC_LDO_VCN28_LP_CTRL_MASK                               0x1
#define PMIC_LDO_VCN28_LP_CTRL_SHIFT                              2
#define PMIC_LDO_VCN28_EN_CTRL_ADDR                               MT6353_LDO3_VCN28_CON0
#define PMIC_LDO_VCN28_EN_CTRL_MASK                               0x1
#define PMIC_LDO_VCN28_EN_CTRL_SHIFT                              3
#define PMIC_LDO_VCN28_LP_SEL_ADDR                                MT6353_LDO3_VCN28_CON0
#define PMIC_LDO_VCN28_LP_SEL_MASK                                0x7
#define PMIC_LDO_VCN28_LP_SEL_SHIFT                               5
#define PMIC_DA_QI_VCN28_MODE_ADDR                                MT6353_LDO3_VCN28_CON0
#define PMIC_DA_QI_VCN28_MODE_MASK                                0x1
#define PMIC_DA_QI_VCN28_MODE_SHIFT                               8
#define PMIC_LDO_VCN28_STBTD_ADDR                                 MT6353_LDO3_VCN28_CON0
#define PMIC_LDO_VCN28_STBTD_MASK                                 0x1
#define PMIC_LDO_VCN28_STBTD_SHIFT                                9
#define PMIC_LDO_VCN28_EN_SEL_ADDR                                MT6353_LDO3_VCN28_CON0
#define PMIC_LDO_VCN28_EN_SEL_MASK                                0x7
#define PMIC_LDO_VCN28_EN_SEL_SHIFT                               11
#define PMIC_DA_QI_VCN28_STB_ADDR                                 MT6353_LDO3_VCN28_CON0
#define PMIC_DA_QI_VCN28_STB_MASK                                 0x1
#define PMIC_DA_QI_VCN28_STB_SHIFT                                14
#define PMIC_DA_QI_VCN28_EN_ADDR                                  MT6353_LDO3_VCN28_CON0
#define PMIC_DA_QI_VCN28_EN_MASK                                  0x1
#define PMIC_DA_QI_VCN28_EN_SHIFT                                 15
#define PMIC_LDO_VCN28_OCFB_EN_ADDR                               MT6353_LDO3_VCN28_OCFB_CON0
#define PMIC_LDO_VCN28_OCFB_EN_MASK                               0x1
#define PMIC_LDO_VCN28_OCFB_EN_SHIFT                              9
#define PMIC_DA_QI_VCN28_OCFB_EN_ADDR                             MT6353_LDO3_VCN28_OCFB_CON0
#define PMIC_DA_QI_VCN28_OCFB_EN_MASK                             0x1
#define PMIC_DA_QI_VCN28_OCFB_EN_SHIFT                            10
#define PMIC_LDO_VCN18_LP_MODE_ADDR                               MT6353_LDO3_VCN18_CON0
#define PMIC_LDO_VCN18_LP_MODE_MASK                               0x1
#define PMIC_LDO_VCN18_LP_MODE_SHIFT                              0
#define PMIC_LDO_VCN18_EN_ADDR                                    MT6353_LDO3_VCN18_CON0
#define PMIC_LDO_VCN18_EN_MASK                                    0x1
#define PMIC_LDO_VCN18_EN_SHIFT                                   1
#define PMIC_LDO_VCN18_LP_CTRL_ADDR                               MT6353_LDO3_VCN18_CON0
#define PMIC_LDO_VCN18_LP_CTRL_MASK                               0x1
#define PMIC_LDO_VCN18_LP_CTRL_SHIFT                              2
#define PMIC_LDO_VCN18_EN_CTRL_ADDR                               MT6353_LDO3_VCN18_CON0
#define PMIC_LDO_VCN18_EN_CTRL_MASK                               0x1
#define PMIC_LDO_VCN18_EN_CTRL_SHIFT                              3
#define PMIC_LDO_VCN18_LP_SEL_ADDR                                MT6353_LDO3_VCN18_CON0
#define PMIC_LDO_VCN18_LP_SEL_MASK                                0x7
#define PMIC_LDO_VCN18_LP_SEL_SHIFT                               5
#define PMIC_DA_QI_VCN18_MODE_ADDR                                MT6353_LDO3_VCN18_CON0
#define PMIC_DA_QI_VCN18_MODE_MASK                                0x1
#define PMIC_DA_QI_VCN18_MODE_SHIFT                               8
#define PMIC_LDO_VCN18_STBTD_ADDR                                 MT6353_LDO3_VCN18_CON0
#define PMIC_LDO_VCN18_STBTD_MASK                                 0x1
#define PMIC_LDO_VCN18_STBTD_SHIFT                                9
#define PMIC_LDO_VCN18_EN_SEL_ADDR                                MT6353_LDO3_VCN18_CON0
#define PMIC_LDO_VCN18_EN_SEL_MASK                                0x7
#define PMIC_LDO_VCN18_EN_SEL_SHIFT                               11
#define PMIC_DA_QI_VCN18_STB_ADDR                                 MT6353_LDO3_VCN18_CON0
#define PMIC_DA_QI_VCN18_STB_MASK                                 0x1
#define PMIC_DA_QI_VCN18_STB_SHIFT                                14
#define PMIC_DA_QI_VCN18_EN_ADDR                                  MT6353_LDO3_VCN18_CON0
#define PMIC_DA_QI_VCN18_EN_MASK                                  0x1
#define PMIC_DA_QI_VCN18_EN_SHIFT                                 15
#define PMIC_LDO_VCN18_OCFB_EN_ADDR                               MT6353_LDO3_VCN18_OCFB_CON0
#define PMIC_LDO_VCN18_OCFB_EN_MASK                               0x1
#define PMIC_LDO_VCN18_OCFB_EN_SHIFT                              9
#define PMIC_DA_QI_VCN18_OCFB_EN_ADDR                             MT6353_LDO3_VCN18_OCFB_CON0
#define PMIC_DA_QI_VCN18_OCFB_EN_MASK                             0x1
#define PMIC_DA_QI_VCN18_OCFB_EN_SHIFT                            10
#define PMIC_LDO_VCAMA_LP_MODE_ADDR                               MT6353_LDO0_VCAMA_CON0
#define PMIC_LDO_VCAMA_LP_MODE_MASK                               0x1
#define PMIC_LDO_VCAMA_LP_MODE_SHIFT                              0
#define PMIC_LDO_VCAMA_EN_ADDR                                    MT6353_LDO0_VCAMA_CON0
#define PMIC_LDO_VCAMA_EN_MASK                                    0x1
#define PMIC_LDO_VCAMA_EN_SHIFT                                   1
#define PMIC_DA_QI_VCAMA_MODE_ADDR                                MT6353_LDO0_VCAMA_CON0
#define PMIC_DA_QI_VCAMA_MODE_MASK                                0x1
#define PMIC_DA_QI_VCAMA_MODE_SHIFT                               8
#define PMIC_LDO_VCAMA_STBTD_ADDR                                 MT6353_LDO0_VCAMA_CON0
#define PMIC_LDO_VCAMA_STBTD_MASK                                 0x1
#define PMIC_LDO_VCAMA_STBTD_SHIFT                                9
#define PMIC_DA_QI_VCAMA_STB_ADDR                                 MT6353_LDO0_VCAMA_CON0
#define PMIC_DA_QI_VCAMA_STB_MASK                                 0x1
#define PMIC_DA_QI_VCAMA_STB_SHIFT                                14
#define PMIC_DA_QI_VCAMA_EN_ADDR                                  MT6353_LDO0_VCAMA_CON0
#define PMIC_DA_QI_VCAMA_EN_MASK                                  0x1
#define PMIC_DA_QI_VCAMA_EN_SHIFT                                 15
#define PMIC_LDO_VCAMA_OCFB_EN_ADDR                               MT6353_LDO0_VCAMA_OCFB_CON0
#define PMIC_LDO_VCAMA_OCFB_EN_MASK                               0x1
#define PMIC_LDO_VCAMA_OCFB_EN_SHIFT                              9
#define PMIC_DA_QI_VCAMA_OCFB_EN_ADDR                             MT6353_LDO0_VCAMA_OCFB_CON0
#define PMIC_DA_QI_VCAMA_OCFB_EN_MASK                             0x1
#define PMIC_DA_QI_VCAMA_OCFB_EN_SHIFT                            10
#define PMIC_LDO_VCAMIO_LP_MODE_ADDR                              MT6353_LDO0_VCAMIO_CON0
#define PMIC_LDO_VCAMIO_LP_MODE_MASK                              0x1
#define PMIC_LDO_VCAMIO_LP_MODE_SHIFT                             0
#define PMIC_LDO_VCAMIO_EN_ADDR                                   MT6353_LDO0_VCAMIO_CON0
#define PMIC_LDO_VCAMIO_EN_MASK                                   0x1
#define PMIC_LDO_VCAMIO_EN_SHIFT                                  1
#define PMIC_DA_QI_VCAMIO_MODE_ADDR                               MT6353_LDO0_VCAMIO_CON0
#define PMIC_DA_QI_VCAMIO_MODE_MASK                               0x1
#define PMIC_DA_QI_VCAMIO_MODE_SHIFT                              8
#define PMIC_LDO_VCAMIO_STBTD_ADDR                                MT6353_LDO0_VCAMIO_CON0
#define PMIC_LDO_VCAMIO_STBTD_MASK                                0x1
#define PMIC_LDO_VCAMIO_STBTD_SHIFT                               9
#define PMIC_DA_QI_VCAMIO_STB_ADDR                                MT6353_LDO0_VCAMIO_CON0
#define PMIC_DA_QI_VCAMIO_STB_MASK                                0x1
#define PMIC_DA_QI_VCAMIO_STB_SHIFT                               14
#define PMIC_DA_QI_VCAMIO_EN_ADDR                                 MT6353_LDO0_VCAMIO_CON0
#define PMIC_DA_QI_VCAMIO_EN_MASK                                 0x1
#define PMIC_DA_QI_VCAMIO_EN_SHIFT                                15
#define PMIC_LDO_VCAMIO_OCFB_EN_ADDR                              MT6353_LDO0_VCAMIO_OCFB_CON0
#define PMIC_LDO_VCAMIO_OCFB_EN_MASK                              0x1
#define PMIC_LDO_VCAMIO_OCFB_EN_SHIFT                             9
#define PMIC_DA_QI_VCAMIO_OCFB_EN_ADDR                            MT6353_LDO0_VCAMIO_OCFB_CON0
#define PMIC_DA_QI_VCAMIO_OCFB_EN_MASK                            0x1
#define PMIC_DA_QI_VCAMIO_OCFB_EN_SHIFT                           10
#define PMIC_LDO_VCAMD_LP_MODE_ADDR                               MT6353_LDO0_VCAMD_CON0
#define PMIC_LDO_VCAMD_LP_MODE_MASK                               0x1
#define PMIC_LDO_VCAMD_LP_MODE_SHIFT                              0
#define PMIC_LDO_VCAMD_EN_ADDR                                    MT6353_LDO0_VCAMD_CON0
#define PMIC_LDO_VCAMD_EN_MASK                                    0x1
#define PMIC_LDO_VCAMD_EN_SHIFT                                   1
#define PMIC_DA_QI_VCAMD_MODE_ADDR                                MT6353_LDO0_VCAMD_CON0
#define PMIC_DA_QI_VCAMD_MODE_MASK                                0x1
#define PMIC_DA_QI_VCAMD_MODE_SHIFT                               8
#define PMIC_LDO_VCAMD_STBTD_ADDR                                 MT6353_LDO0_VCAMD_CON0
#define PMIC_LDO_VCAMD_STBTD_MASK                                 0x1
#define PMIC_LDO_VCAMD_STBTD_SHIFT                                9
#define PMIC_DA_QI_VCAMD_STB_ADDR                                 MT6353_LDO0_VCAMD_CON0
#define PMIC_DA_QI_VCAMD_STB_MASK                                 0x1
#define PMIC_DA_QI_VCAMD_STB_SHIFT                                14
#define PMIC_DA_QI_VCAMD_EN_ADDR                                  MT6353_LDO0_VCAMD_CON0
#define PMIC_DA_QI_VCAMD_EN_MASK                                  0x1
#define PMIC_DA_QI_VCAMD_EN_SHIFT                                 15
#define PMIC_LDO_VCAMD_OCFB_EN_ADDR                               MT6353_LDO0_VCAMD_OCFB_CON0
#define PMIC_LDO_VCAMD_OCFB_EN_MASK                               0x1
#define PMIC_LDO_VCAMD_OCFB_EN_SHIFT                              9
#define PMIC_DA_QI_VCAMD_OCFB_EN_ADDR                             MT6353_LDO0_VCAMD_OCFB_CON0
#define PMIC_DA_QI_VCAMD_OCFB_EN_MASK                             0x1
#define PMIC_DA_QI_VCAMD_OCFB_EN_SHIFT                            10
#define PMIC_LDO_VAUX18_LP_MODE_ADDR                              MT6353_LDO3_VAUX18_CON0
#define PMIC_LDO_VAUX18_LP_MODE_MASK                              0x1
#define PMIC_LDO_VAUX18_LP_MODE_SHIFT                             0
#define PMIC_LDO_VAUX18_EN_ADDR                                   MT6353_LDO3_VAUX18_CON0
#define PMIC_LDO_VAUX18_EN_MASK                                   0x1
#define PMIC_LDO_VAUX18_EN_SHIFT                                  1
#define PMIC_LDO_VAUX18_LP_CTRL_ADDR                              MT6353_LDO3_VAUX18_CON0
#define PMIC_LDO_VAUX18_LP_CTRL_MASK                              0x1
#define PMIC_LDO_VAUX18_LP_CTRL_SHIFT                             2
#define PMIC_LDO_VAUX18_EN_CTRL_ADDR                              MT6353_LDO3_VAUX18_CON0
#define PMIC_LDO_VAUX18_EN_CTRL_MASK                              0x1
#define PMIC_LDO_VAUX18_EN_CTRL_SHIFT                             3
#define PMIC_LDO_VAUX18_LP_SEL_ADDR                               MT6353_LDO3_VAUX18_CON0
#define PMIC_LDO_VAUX18_LP_SEL_MASK                               0x7
#define PMIC_LDO_VAUX18_LP_SEL_SHIFT                              5
#define PMIC_DA_QI_VAUX18_MODE_ADDR                               MT6353_LDO3_VAUX18_CON0
#define PMIC_DA_QI_VAUX18_MODE_MASK                               0x1
#define PMIC_DA_QI_VAUX18_MODE_SHIFT                              8
#define PMIC_LDO_VAUX18_STBTD_ADDR                                MT6353_LDO3_VAUX18_CON0
#define PMIC_LDO_VAUX18_STBTD_MASK                                0x1
#define PMIC_LDO_VAUX18_STBTD_SHIFT                               9
#define PMIC_LDO_VAUX18_EN_SEL_ADDR                               MT6353_LDO3_VAUX18_CON0
#define PMIC_LDO_VAUX18_EN_SEL_MASK                               0x7
#define PMIC_LDO_VAUX18_EN_SEL_SHIFT                              11
#define PMIC_DA_QI_VAUX18_STB_ADDR                                MT6353_LDO3_VAUX18_CON0
#define PMIC_DA_QI_VAUX18_STB_MASK                                0x1
#define PMIC_DA_QI_VAUX18_STB_SHIFT                               14
#define PMIC_DA_QI_VAUX18_EN_ADDR                                 MT6353_LDO3_VAUX18_CON0
#define PMIC_DA_QI_VAUX18_EN_MASK                                 0x1
#define PMIC_DA_QI_VAUX18_EN_SHIFT                                15
#define PMIC_LDO_VAUX18_OCFB_EN_ADDR                              MT6353_LDO3_VAUX18_OCFB_CON0
#define PMIC_LDO_VAUX18_OCFB_EN_MASK                              0x1
#define PMIC_LDO_VAUX18_OCFB_EN_SHIFT                             9
#define PMIC_DA_QI_VAUX18_OCFB_EN_ADDR                            MT6353_LDO3_VAUX18_OCFB_CON0
#define PMIC_DA_QI_VAUX18_OCFB_EN_MASK                            0x1
#define PMIC_DA_QI_VAUX18_OCFB_EN_SHIFT                           10
#define PMIC_LDO_VAUD28_LP_MODE_ADDR                              MT6353_LDO3_VAUD28_CON0
#define PMIC_LDO_VAUD28_LP_MODE_MASK                              0x1
#define PMIC_LDO_VAUD28_LP_MODE_SHIFT                             0
#define PMIC_LDO_VAUD28_EN_ADDR                                   MT6353_LDO3_VAUD28_CON0
#define PMIC_LDO_VAUD28_EN_MASK                                   0x1
#define PMIC_LDO_VAUD28_EN_SHIFT                                  1
#define PMIC_LDO_VAUD28_LP_CTRL_ADDR                              MT6353_LDO3_VAUD28_CON0
#define PMIC_LDO_VAUD28_LP_CTRL_MASK                              0x1
#define PMIC_LDO_VAUD28_LP_CTRL_SHIFT                             2
#define PMIC_LDO_VAUD28_EN_CTRL_ADDR                              MT6353_LDO3_VAUD28_CON0
#define PMIC_LDO_VAUD28_EN_CTRL_MASK                              0x1
#define PMIC_LDO_VAUD28_EN_CTRL_SHIFT                             3
#define PMIC_LDO_VAUD28_LP_SEL_ADDR                               MT6353_LDO3_VAUD28_CON0
#define PMIC_LDO_VAUD28_LP_SEL_MASK                               0x7
#define PMIC_LDO_VAUD28_LP_SEL_SHIFT                              5
#define PMIC_DA_QI_VAUD28_MODE_ADDR                               MT6353_LDO3_VAUD28_CON0
#define PMIC_DA_QI_VAUD28_MODE_MASK                               0x1
#define PMIC_DA_QI_VAUD28_MODE_SHIFT                              8
#define PMIC_LDO_VAUD28_STBTD_ADDR                                MT6353_LDO3_VAUD28_CON0
#define PMIC_LDO_VAUD28_STBTD_MASK                                0x1
#define PMIC_LDO_VAUD28_STBTD_SHIFT                               9
#define PMIC_LDO_VAUD28_EN_SEL_ADDR                               MT6353_LDO3_VAUD28_CON0
#define PMIC_LDO_VAUD28_EN_SEL_MASK                               0x7
#define PMIC_LDO_VAUD28_EN_SEL_SHIFT                              11
#define PMIC_DA_QI_VAUD28_STB_ADDR                                MT6353_LDO3_VAUD28_CON0
#define PMIC_DA_QI_VAUD28_STB_MASK                                0x1
#define PMIC_DA_QI_VAUD28_STB_SHIFT                               14
#define PMIC_DA_QI_VAUD28_EN_ADDR                                 MT6353_LDO3_VAUD28_CON0
#define PMIC_DA_QI_VAUD28_EN_MASK                                 0x1
#define PMIC_DA_QI_VAUD28_EN_SHIFT                                15
#define PMIC_LDO_VAUD28_OCFB_EN_ADDR                              MT6353_LDO3_VAUD28_OCFB_CON0
#define PMIC_LDO_VAUD28_OCFB_EN_MASK                              0x1
#define PMIC_LDO_VAUD28_OCFB_EN_SHIFT                             9
#define PMIC_DA_QI_VAUD28_OCFB_EN_ADDR                            MT6353_LDO3_VAUD28_OCFB_CON0
#define PMIC_DA_QI_VAUD28_OCFB_EN_MASK                            0x1
#define PMIC_DA_QI_VAUD28_OCFB_EN_SHIFT                           10
#define PMIC_LDO_VSRAM_PROC_LP_MODE_ADDR                          MT6353_LDO3_VSRAM_PROC_CON0
#define PMIC_LDO_VSRAM_PROC_LP_MODE_MASK                          0x1
#define PMIC_LDO_VSRAM_PROC_LP_MODE_SHIFT                         0
#define PMIC_LDO_VSRAM_PROC_EN_ADDR                               MT6353_LDO3_VSRAM_PROC_CON0
#define PMIC_LDO_VSRAM_PROC_EN_MASK                               0x1
#define PMIC_LDO_VSRAM_PROC_EN_SHIFT                              1
#define PMIC_LDO_VSRAM_PROC_LP_CTRL_ADDR                          MT6353_LDO3_VSRAM_PROC_CON0
#define PMIC_LDO_VSRAM_PROC_LP_CTRL_MASK                          0x1
#define PMIC_LDO_VSRAM_PROC_LP_CTRL_SHIFT                         2
#define PMIC_LDO_VSRAM_PROC_EN_CTRL_ADDR                          MT6353_LDO3_VSRAM_PROC_CON0
#define PMIC_LDO_VSRAM_PROC_EN_CTRL_MASK                          0x1
#define PMIC_LDO_VSRAM_PROC_EN_CTRL_SHIFT                         3
#define PMIC_LDO_VSRAM_PROC_LP_SEL_ADDR                           MT6353_LDO3_VSRAM_PROC_CON0
#define PMIC_LDO_VSRAM_PROC_LP_SEL_MASK                           0x7
#define PMIC_LDO_VSRAM_PROC_LP_SEL_SHIFT                          5
#define PMIC_DA_QI_VSRAM_PROC_MODE_ADDR                           MT6353_LDO3_VSRAM_PROC_CON0
#define PMIC_DA_QI_VSRAM_PROC_MODE_MASK                           0x1
#define PMIC_DA_QI_VSRAM_PROC_MODE_SHIFT                          8
#define PMIC_LDO_VSRAM_PROC_STBTD_ADDR                            MT6353_LDO3_VSRAM_PROC_CON0
#define PMIC_LDO_VSRAM_PROC_STBTD_MASK                            0x1
#define PMIC_LDO_VSRAM_PROC_STBTD_SHIFT                           9
#define PMIC_LDO_VSRAM_PROC_EN_SEL_ADDR                           MT6353_LDO3_VSRAM_PROC_CON0
#define PMIC_LDO_VSRAM_PROC_EN_SEL_MASK                           0x7
#define PMIC_LDO_VSRAM_PROC_EN_SEL_SHIFT                          11
#define PMIC_DA_QI_VSRAM_PROC_STB_ADDR                            MT6353_LDO3_VSRAM_PROC_CON0
#define PMIC_DA_QI_VSRAM_PROC_STB_MASK                            0x1
#define PMIC_DA_QI_VSRAM_PROC_STB_SHIFT                           14
#define PMIC_DA_QI_VSRAM_PROC_EN_ADDR                             MT6353_LDO3_VSRAM_PROC_CON0
#define PMIC_DA_QI_VSRAM_PROC_EN_MASK                             0x1
#define PMIC_DA_QI_VSRAM_PROC_EN_SHIFT                            15
#define PMIC_LDO_VSRAM_PROC_OCFB_EN_ADDR                          MT6353_LDO3_VSRAM_PROC_OCFB_CON0
#define PMIC_LDO_VSRAM_PROC_OCFB_EN_MASK                          0x1
#define PMIC_LDO_VSRAM_PROC_OCFB_EN_SHIFT                         9
#define PMIC_DA_QI_VSRAM_PROC_OCFB_EN_ADDR                        MT6353_LDO3_VSRAM_PROC_OCFB_CON0
#define PMIC_DA_QI_VSRAM_PROC_OCFB_EN_MASK                        0x1
#define PMIC_DA_QI_VSRAM_PROC_OCFB_EN_SHIFT                       10
#define PMIC_LDO_VDRAM_LP_MODE_ADDR                               MT6353_LDO1_VDRAM_CON0
#define PMIC_LDO_VDRAM_LP_MODE_MASK                               0x1
#define PMIC_LDO_VDRAM_LP_MODE_SHIFT                              0
#define PMIC_LDO_VDRAM_EN_ADDR                                    MT6353_LDO1_VDRAM_CON0
#define PMIC_LDO_VDRAM_EN_MASK                                    0x1
#define PMIC_LDO_VDRAM_EN_SHIFT                                   1
#define PMIC_LDO_VDRAM_LP_CTRL_ADDR                               MT6353_LDO1_VDRAM_CON0
#define PMIC_LDO_VDRAM_LP_CTRL_MASK                               0x1
#define PMIC_LDO_VDRAM_LP_CTRL_SHIFT                              2
#define PMIC_LDO_VDRAM_LP_SEL_ADDR                                MT6353_LDO1_VDRAM_CON0
#define PMIC_LDO_VDRAM_LP_SEL_MASK                                0x7
#define PMIC_LDO_VDRAM_LP_SEL_SHIFT                               5
#define PMIC_DA_QI_VDRAM_MODE_ADDR                                MT6353_LDO1_VDRAM_CON0
#define PMIC_DA_QI_VDRAM_MODE_MASK                                0x1
#define PMIC_DA_QI_VDRAM_MODE_SHIFT                               8
#define PMIC_LDO_VDRAM_STBTD_ADDR                                 MT6353_LDO1_VDRAM_CON0
#define PMIC_LDO_VDRAM_STBTD_MASK                                 0x1
#define PMIC_LDO_VDRAM_STBTD_SHIFT                                9
#define PMIC_DA_QI_VDRAM_STB_ADDR                                 MT6353_LDO1_VDRAM_CON0
#define PMIC_DA_QI_VDRAM_STB_MASK                                 0x1
#define PMIC_DA_QI_VDRAM_STB_SHIFT                                14
#define PMIC_DA_QI_VDRAM_EN_ADDR                                  MT6353_LDO1_VDRAM_CON0
#define PMIC_DA_QI_VDRAM_EN_MASK                                  0x1
#define PMIC_DA_QI_VDRAM_EN_SHIFT                                 15
#define PMIC_LDO_VDRAM_OCFB_EN_ADDR                               MT6353_LDO1_VDRAM_OCFB_CON0
#define PMIC_LDO_VDRAM_OCFB_EN_MASK                               0x1
#define PMIC_LDO_VDRAM_OCFB_EN_SHIFT                              9
#define PMIC_DA_QI_VDRAM_OCFB_EN_ADDR                             MT6353_LDO1_VDRAM_OCFB_CON0
#define PMIC_DA_QI_VDRAM_OCFB_EN_MASK                             0x1
#define PMIC_DA_QI_VDRAM_OCFB_EN_SHIFT                            10
#define PMIC_LDO_VDRAM_FAST_TRAN_DL_EN_ADDR                       MT6353_LDO_VDRAM_FAST_TRAN_CON0
#define PMIC_LDO_VDRAM_FAST_TRAN_DL_EN_MASK                       0x1
#define PMIC_LDO_VDRAM_FAST_TRAN_DL_EN_SHIFT                      0
#define PMIC_LDO_VDRAM_FAST_TRAN_DL_CTRL_ADDR                     MT6353_LDO_VDRAM_FAST_TRAN_CON0
#define PMIC_LDO_VDRAM_FAST_TRAN_DL_CTRL_MASK                     0x1
#define PMIC_LDO_VDRAM_FAST_TRAN_DL_CTRL_SHIFT                    1
#define PMIC_LDO_VDRAM_FAST_TRAN_DL_SRCLKEN_SEL_ADDR              MT6353_LDO_VDRAM_FAST_TRAN_CON0
#define PMIC_LDO_VDRAM_FAST_TRAN_DL_SRCLKEN_SEL_MASK              0x7
#define PMIC_LDO_VDRAM_FAST_TRAN_DL_SRCLKEN_SEL_SHIFT             2
#define PMIC_DA_QI_VDRAM_FAST_TRAN_DL_EN_ADDR                     MT6353_LDO_VDRAM_FAST_TRAN_CON0
#define PMIC_DA_QI_VDRAM_FAST_TRAN_DL_EN_MASK                     0x1
#define PMIC_DA_QI_VDRAM_FAST_TRAN_DL_EN_SHIFT                    5
#define PMIC_LDO_VDRAM_FAST_TRAN_CL_EN_ADDR                       MT6353_LDO_VDRAM_FAST_TRAN_CON0
#define PMIC_LDO_VDRAM_FAST_TRAN_CL_EN_MASK                       0x1
#define PMIC_LDO_VDRAM_FAST_TRAN_CL_EN_SHIFT                      8
#define PMIC_LDO_VDRAM_FAST_TRAN_CL_CTRL_ADDR                     MT6353_LDO_VDRAM_FAST_TRAN_CON0
#define PMIC_LDO_VDRAM_FAST_TRAN_CL_CTRL_MASK                     0x1
#define PMIC_LDO_VDRAM_FAST_TRAN_CL_CTRL_SHIFT                    9
#define PMIC_LDO_VDRAM_FAST_TRAN_CL_SRCLKEN_SEL_ADDR              MT6353_LDO_VDRAM_FAST_TRAN_CON0
#define PMIC_LDO_VDRAM_FAST_TRAN_CL_SRCLKEN_SEL_MASK              0x7
#define PMIC_LDO_VDRAM_FAST_TRAN_CL_SRCLKEN_SEL_SHIFT             10
#define PMIC_DA_QI_VDRAM_FAST_TRAN_CL_EN_ADDR                     MT6353_LDO_VDRAM_FAST_TRAN_CON0
#define PMIC_DA_QI_VDRAM_FAST_TRAN_CL_EN_MASK                     0x1
#define PMIC_DA_QI_VDRAM_FAST_TRAN_CL_EN_SHIFT                    13
#define PMIC_LDO_VSIM1_LP_MODE_ADDR                               MT6353_LDO1_VSIM1_CON0
#define PMIC_LDO_VSIM1_LP_MODE_MASK                               0x1
#define PMIC_LDO_VSIM1_LP_MODE_SHIFT                              0
#define PMIC_LDO_VSIM1_EN_ADDR                                    MT6353_LDO1_VSIM1_CON0
#define PMIC_LDO_VSIM1_EN_MASK                                    0x1
#define PMIC_LDO_VSIM1_EN_SHIFT                                   1
#define PMIC_LDO_VSIM1_LP_CTRL_ADDR                               MT6353_LDO1_VSIM1_CON0
#define PMIC_LDO_VSIM1_LP_CTRL_MASK                               0x1
#define PMIC_LDO_VSIM1_LP_CTRL_SHIFT                              2
#define PMIC_LDO_VSIM1_LP_SEL_ADDR                                MT6353_LDO1_VSIM1_CON0
#define PMIC_LDO_VSIM1_LP_SEL_MASK                                0x7
#define PMIC_LDO_VSIM1_LP_SEL_SHIFT                               5
#define PMIC_DA_QI_VSIM1_MODE_ADDR                                MT6353_LDO1_VSIM1_CON0
#define PMIC_DA_QI_VSIM1_MODE_MASK                                0x1
#define PMIC_DA_QI_VSIM1_MODE_SHIFT                               8
#define PMIC_LDO_VSIM1_STBTD_ADDR                                 MT6353_LDO1_VSIM1_CON0
#define PMIC_LDO_VSIM1_STBTD_MASK                                 0x1
#define PMIC_LDO_VSIM1_STBTD_SHIFT                                9
#define PMIC_DA_QI_VSIM1_STB_ADDR                                 MT6353_LDO1_VSIM1_CON0
#define PMIC_DA_QI_VSIM1_STB_MASK                                 0x1
#define PMIC_DA_QI_VSIM1_STB_SHIFT                                14
#define PMIC_DA_QI_VSIM1_EN_ADDR                                  MT6353_LDO1_VSIM1_CON0
#define PMIC_DA_QI_VSIM1_EN_MASK                                  0x1
#define PMIC_DA_QI_VSIM1_EN_SHIFT                                 15
#define PMIC_LDO_VSIM1_OCFB_EN_ADDR                               MT6353_LDO1_VSIM1_OCFB_CON0
#define PMIC_LDO_VSIM1_OCFB_EN_MASK                               0x1
#define PMIC_LDO_VSIM1_OCFB_EN_SHIFT                              9
#define PMIC_DA_QI_VSIM1_OCFB_EN_ADDR                             MT6353_LDO1_VSIM1_OCFB_CON0
#define PMIC_DA_QI_VSIM1_OCFB_EN_MASK                             0x1
#define PMIC_DA_QI_VSIM1_OCFB_EN_SHIFT                            10
#define PMIC_LDO_VSIM2_LP_MODE_ADDR                               MT6353_LDO1_VSIM2_CON0
#define PMIC_LDO_VSIM2_LP_MODE_MASK                               0x1
#define PMIC_LDO_VSIM2_LP_MODE_SHIFT                              0
#define PMIC_LDO_VSIM2_EN_ADDR                                    MT6353_LDO1_VSIM2_CON0
#define PMIC_LDO_VSIM2_EN_MASK                                    0x1
#define PMIC_LDO_VSIM2_EN_SHIFT                                   1
#define PMIC_LDO_VSIM2_LP_CTRL_ADDR                               MT6353_LDO1_VSIM2_CON0
#define PMIC_LDO_VSIM2_LP_CTRL_MASK                               0x1
#define PMIC_LDO_VSIM2_LP_CTRL_SHIFT                              2
#define PMIC_LDO_VSIM2_LP_SEL_ADDR                                MT6353_LDO1_VSIM2_CON0
#define PMIC_LDO_VSIM2_LP_SEL_MASK                                0x7
#define PMIC_LDO_VSIM2_LP_SEL_SHIFT                               5
#define PMIC_DA_QI_VSIM2_MODE_ADDR                                MT6353_LDO1_VSIM2_CON0
#define PMIC_DA_QI_VSIM2_MODE_MASK                                0x1
#define PMIC_DA_QI_VSIM2_MODE_SHIFT                               8
#define PMIC_LDO_VSIM2_STBTD_ADDR                                 MT6353_LDO1_VSIM2_CON0
#define PMIC_LDO_VSIM2_STBTD_MASK                                 0x1
#define PMIC_LDO_VSIM2_STBTD_SHIFT                                9
#define PMIC_DA_QI_VSIM2_STB_ADDR                                 MT6353_LDO1_VSIM2_CON0
#define PMIC_DA_QI_VSIM2_STB_MASK                                 0x1
#define PMIC_DA_QI_VSIM2_STB_SHIFT                                14
#define PMIC_DA_QI_VSIM2_EN_ADDR                                  MT6353_LDO1_VSIM2_CON0
#define PMIC_DA_QI_VSIM2_EN_MASK                                  0x1
#define PMIC_DA_QI_VSIM2_EN_SHIFT                                 15
#define PMIC_LDO_VSIM2_OCFB_EN_ADDR                               MT6353_LDO1_VSIM2_OCFB_CON0
#define PMIC_LDO_VSIM2_OCFB_EN_MASK                               0x1
#define PMIC_LDO_VSIM2_OCFB_EN_SHIFT                              9
#define PMIC_DA_QI_VSIM2_OCFB_EN_ADDR                             MT6353_LDO1_VSIM2_OCFB_CON0
#define PMIC_DA_QI_VSIM2_OCFB_EN_MASK                             0x1
#define PMIC_DA_QI_VSIM2_OCFB_EN_SHIFT                            10
#define PMIC_LDO_VIO28_LP_MODE_ADDR                               MT6353_LDO1_VIO28_CON0
#define PMIC_LDO_VIO28_LP_MODE_MASK                               0x1
#define PMIC_LDO_VIO28_LP_MODE_SHIFT                              0
#define PMIC_LDO_VIO28_EN_ADDR                                    MT6353_LDO1_VIO28_CON0
#define PMIC_LDO_VIO28_EN_MASK                                    0x1
#define PMIC_LDO_VIO28_EN_SHIFT                                   1
#define PMIC_LDO_VIO28_LP_CTRL_ADDR                               MT6353_LDO1_VIO28_CON0
#define PMIC_LDO_VIO28_LP_CTRL_MASK                               0x1
#define PMIC_LDO_VIO28_LP_CTRL_SHIFT                              2
#define PMIC_LDO_VIO28_LP_SEL_ADDR                                MT6353_LDO1_VIO28_CON0
#define PMIC_LDO_VIO28_LP_SEL_MASK                                0x7
#define PMIC_LDO_VIO28_LP_SEL_SHIFT                               5
#define PMIC_DA_QI_VIO28_MODE_ADDR                                MT6353_LDO1_VIO28_CON0
#define PMIC_DA_QI_VIO28_MODE_MASK                                0x1
#define PMIC_DA_QI_VIO28_MODE_SHIFT                               8
#define PMIC_LDO_VIO28_STBTD_ADDR                                 MT6353_LDO1_VIO28_CON0
#define PMIC_LDO_VIO28_STBTD_MASK                                 0x1
#define PMIC_LDO_VIO28_STBTD_SHIFT                                9
#define PMIC_DA_QI_VIO28_STB_ADDR                                 MT6353_LDO1_VIO28_CON0
#define PMIC_DA_QI_VIO28_STB_MASK                                 0x1
#define PMIC_DA_QI_VIO28_STB_SHIFT                                14
#define PMIC_DA_QI_VIO28_EN_ADDR                                  MT6353_LDO1_VIO28_CON0
#define PMIC_DA_QI_VIO28_EN_MASK                                  0x1
#define PMIC_DA_QI_VIO28_EN_SHIFT                                 15
#define PMIC_LDO_VIO28_OCFB_EN_ADDR                               MT6353_LDO1_VIO28_OCFB_CON0
#define PMIC_LDO_VIO28_OCFB_EN_MASK                               0x1
#define PMIC_LDO_VIO28_OCFB_EN_SHIFT                              9
#define PMIC_DA_QI_VIO28_OCFB_EN_ADDR                             MT6353_LDO1_VIO28_OCFB_CON0
#define PMIC_DA_QI_VIO28_OCFB_EN_MASK                             0x1
#define PMIC_DA_QI_VIO28_OCFB_EN_SHIFT                            10
#define PMIC_LDO_VMC_LP_MODE_ADDR                                 MT6353_LDO1_VMC_CON0
#define PMIC_LDO_VMC_LP_MODE_MASK                                 0x1
#define PMIC_LDO_VMC_LP_MODE_SHIFT                                0
#define PMIC_LDO_VMC_EN_ADDR                                      MT6353_LDO1_VMC_CON0
#define PMIC_LDO_VMC_EN_MASK                                      0x1
#define PMIC_LDO_VMC_EN_SHIFT                                     1
#define PMIC_LDO_VMC_LP_CTRL_ADDR                                 MT6353_LDO1_VMC_CON0
#define PMIC_LDO_VMC_LP_CTRL_MASK                                 0x1
#define PMIC_LDO_VMC_LP_CTRL_SHIFT                                2
#define PMIC_LDO_VMC_LP_SEL_ADDR                                  MT6353_LDO1_VMC_CON0
#define PMIC_LDO_VMC_LP_SEL_MASK                                  0x7
#define PMIC_LDO_VMC_LP_SEL_SHIFT                                 5
#define PMIC_DA_QI_VMC_MODE_ADDR                                  MT6353_LDO1_VMC_CON0
#define PMIC_DA_QI_VMC_MODE_MASK                                  0x1
#define PMIC_DA_QI_VMC_MODE_SHIFT                                 8
#define PMIC_LDO_VMC_STBTD_ADDR                                   MT6353_LDO1_VMC_CON0
#define PMIC_LDO_VMC_STBTD_MASK                                   0x1
#define PMIC_LDO_VMC_STBTD_SHIFT                                  9
#define PMIC_DA_QI_VMC_STB_ADDR                                   MT6353_LDO1_VMC_CON0
#define PMIC_DA_QI_VMC_STB_MASK                                   0x1
#define PMIC_DA_QI_VMC_STB_SHIFT                                  14
#define PMIC_DA_QI_VMC_EN_ADDR                                    MT6353_LDO1_VMC_CON0
#define PMIC_DA_QI_VMC_EN_MASK                                    0x1
#define PMIC_DA_QI_VMC_EN_SHIFT                                   15
#define PMIC_LDO_VMC_OCFB_EN_ADDR                                 MT6353_LDO1_VMC_OCFB_CON0
#define PMIC_LDO_VMC_OCFB_EN_MASK                                 0x1
#define PMIC_LDO_VMC_OCFB_EN_SHIFT                                9
#define PMIC_DA_QI_VMC_OCFB_EN_ADDR                               MT6353_LDO1_VMC_OCFB_CON0
#define PMIC_DA_QI_VMC_OCFB_EN_MASK                               0x1
#define PMIC_DA_QI_VMC_OCFB_EN_SHIFT                              10
#define PMIC_LDO_VMC_FAST_TRAN_DL_EN_ADDR                         MT6353_LDO_VMC_FAST_TRAN_CON0
#define PMIC_LDO_VMC_FAST_TRAN_DL_EN_MASK                         0x1
#define PMIC_LDO_VMC_FAST_TRAN_DL_EN_SHIFT                        0
#define PMIC_LDO_VMC_FAST_TRAN_DL_CTRL_ADDR                       MT6353_LDO_VMC_FAST_TRAN_CON0
#define PMIC_LDO_VMC_FAST_TRAN_DL_CTRL_MASK                       0x1
#define PMIC_LDO_VMC_FAST_TRAN_DL_CTRL_SHIFT                      1
#define PMIC_LDO_VMC_FAST_TRAN_DL_SRCLKEN_SEL_ADDR                MT6353_LDO_VMC_FAST_TRAN_CON0
#define PMIC_LDO_VMC_FAST_TRAN_DL_SRCLKEN_SEL_MASK                0x7
#define PMIC_LDO_VMC_FAST_TRAN_DL_SRCLKEN_SEL_SHIFT               2
#define PMIC_DA_QI_VMC_FAST_TRAN_DL_EN_ADDR                       MT6353_LDO_VMC_FAST_TRAN_CON0
#define PMIC_DA_QI_VMC_FAST_TRAN_DL_EN_MASK                       0x1
#define PMIC_DA_QI_VMC_FAST_TRAN_DL_EN_SHIFT                      5
#define PMIC_LDO_VMC_FAST_TRAN_CL_EN_ADDR                         MT6353_LDO_VMC_FAST_TRAN_CON0
#define PMIC_LDO_VMC_FAST_TRAN_CL_EN_MASK                         0x1
#define PMIC_LDO_VMC_FAST_TRAN_CL_EN_SHIFT                        8
#define PMIC_LDO_VMC_FAST_TRAN_CL_CTRL_ADDR                       MT6353_LDO_VMC_FAST_TRAN_CON0
#define PMIC_LDO_VMC_FAST_TRAN_CL_CTRL_MASK                       0x1
#define PMIC_LDO_VMC_FAST_TRAN_CL_CTRL_SHIFT                      9
#define PMIC_LDO_VMC_FAST_TRAN_CL_SRCLKEN_SEL_ADDR                MT6353_LDO_VMC_FAST_TRAN_CON0
#define PMIC_LDO_VMC_FAST_TRAN_CL_SRCLKEN_SEL_MASK                0x7
#define PMIC_LDO_VMC_FAST_TRAN_CL_SRCLKEN_SEL_SHIFT               10
#define PMIC_DA_QI_VMC_FAST_TRAN_CL_EN_ADDR                       MT6353_LDO_VMC_FAST_TRAN_CON0
#define PMIC_DA_QI_VMC_FAST_TRAN_CL_EN_MASK                       0x1
#define PMIC_DA_QI_VMC_FAST_TRAN_CL_EN_SHIFT                      13
#define PMIC_LDO_VMCH_LP_MODE_ADDR                                MT6353_LDO1_VMCH_CON0
#define PMIC_LDO_VMCH_LP_MODE_MASK                                0x1
#define PMIC_LDO_VMCH_LP_MODE_SHIFT                               0
#define PMIC_LDO_VMCH_EN_ADDR                                     MT6353_LDO1_VMCH_CON0
#define PMIC_LDO_VMCH_EN_MASK                                     0x1
#define PMIC_LDO_VMCH_EN_SHIFT                                    1
#define PMIC_LDO_VMCH_LP_CTRL_ADDR                                MT6353_LDO1_VMCH_CON0
#define PMIC_LDO_VMCH_LP_CTRL_MASK                                0x1
#define PMIC_LDO_VMCH_LP_CTRL_SHIFT                               2
#define PMIC_LDO_VMCH_LP_SEL_ADDR                                 MT6353_LDO1_VMCH_CON0
#define PMIC_LDO_VMCH_LP_SEL_MASK                                 0x7
#define PMIC_LDO_VMCH_LP_SEL_SHIFT                                5
#define PMIC_DA_QI_VMCH_MODE_ADDR                                 MT6353_LDO1_VMCH_CON0
#define PMIC_DA_QI_VMCH_MODE_MASK                                 0x1
#define PMIC_DA_QI_VMCH_MODE_SHIFT                                8
#define PMIC_LDO_VMCH_STBTD_ADDR                                  MT6353_LDO1_VMCH_CON0
#define PMIC_LDO_VMCH_STBTD_MASK                                  0x1
#define PMIC_LDO_VMCH_STBTD_SHIFT                                 9
#define PMIC_DA_QI_VMCH_STB_ADDR                                  MT6353_LDO1_VMCH_CON0
#define PMIC_DA_QI_VMCH_STB_MASK                                  0x1
#define PMIC_DA_QI_VMCH_STB_SHIFT                                 14
#define PMIC_DA_QI_VMCH_EN_ADDR                                   MT6353_LDO1_VMCH_CON0
#define PMIC_DA_QI_VMCH_EN_MASK                                   0x1
#define PMIC_DA_QI_VMCH_EN_SHIFT                                  15
#define PMIC_LDO_VMCH_OCFB_EN_ADDR                                MT6353_LDO1_VMCH_OCFB_CON0
#define PMIC_LDO_VMCH_OCFB_EN_MASK                                0x1
#define PMIC_LDO_VMCH_OCFB_EN_SHIFT                               9
#define PMIC_DA_QI_VMCH_OCFB_EN_ADDR                              MT6353_LDO1_VMCH_OCFB_CON0
#define PMIC_DA_QI_VMCH_OCFB_EN_MASK                              0x1
#define PMIC_DA_QI_VMCH_OCFB_EN_SHIFT                             10
#define PMIC_LDO_VMCH_FAST_TRAN_DL_EN_ADDR                        MT6353_LDO_VMCH_FAST_TRAN_CON0
#define PMIC_LDO_VMCH_FAST_TRAN_DL_EN_MASK                        0x1
#define PMIC_LDO_VMCH_FAST_TRAN_DL_EN_SHIFT                       0
#define PMIC_LDO_VMCH_FAST_TRAN_DL_CTRL_ADDR                      MT6353_LDO_VMCH_FAST_TRAN_CON0
#define PMIC_LDO_VMCH_FAST_TRAN_DL_CTRL_MASK                      0x1
#define PMIC_LDO_VMCH_FAST_TRAN_DL_CTRL_SHIFT                     1
#define PMIC_LDO_VMCH_FAST_TRAN_DL_SRCLKEN_SEL_ADDR               MT6353_LDO_VMCH_FAST_TRAN_CON0
#define PMIC_LDO_VMCH_FAST_TRAN_DL_SRCLKEN_SEL_MASK               0x7
#define PMIC_LDO_VMCH_FAST_TRAN_DL_SRCLKEN_SEL_SHIFT              2
#define PMIC_DA_QI_VMCH_FAST_TRAN_DL_EN_ADDR                      MT6353_LDO_VMCH_FAST_TRAN_CON0
#define PMIC_DA_QI_VMCH_FAST_TRAN_DL_EN_MASK                      0x1
#define PMIC_DA_QI_VMCH_FAST_TRAN_DL_EN_SHIFT                     5
#define PMIC_LDO_VMCH_FAST_TRAN_CL_EN_ADDR                        MT6353_LDO_VMCH_FAST_TRAN_CON0
#define PMIC_LDO_VMCH_FAST_TRAN_CL_EN_MASK                        0x1
#define PMIC_LDO_VMCH_FAST_TRAN_CL_EN_SHIFT                       8
#define PMIC_LDO_VMCH_FAST_TRAN_CL_CTRL_ADDR                      MT6353_LDO_VMCH_FAST_TRAN_CON0
#define PMIC_LDO_VMCH_FAST_TRAN_CL_CTRL_MASK                      0x1
#define PMIC_LDO_VMCH_FAST_TRAN_CL_CTRL_SHIFT                     9
#define PMIC_LDO_VMCH_FAST_TRAN_CL_SRCLKEN_SEL_ADDR               MT6353_LDO_VMCH_FAST_TRAN_CON0
#define PMIC_LDO_VMCH_FAST_TRAN_CL_SRCLKEN_SEL_MASK               0x7
#define PMIC_LDO_VMCH_FAST_TRAN_CL_SRCLKEN_SEL_SHIFT              10
#define PMIC_DA_QI_VMCH_FAST_TRAN_CL_EN_ADDR                      MT6353_LDO_VMCH_FAST_TRAN_CON0
#define PMIC_DA_QI_VMCH_FAST_TRAN_CL_EN_MASK                      0x1
#define PMIC_DA_QI_VMCH_FAST_TRAN_CL_EN_SHIFT                     13
#define PMIC_LDO_VUSB33_LP_MODE_ADDR                              MT6353_LDO1_VUSB33_CON0
#define PMIC_LDO_VUSB33_LP_MODE_MASK                              0x1
#define PMIC_LDO_VUSB33_LP_MODE_SHIFT                             0
#define PMIC_LDO_VUSB33_EN_ADDR                                   MT6353_LDO1_VUSB33_CON0
#define PMIC_LDO_VUSB33_EN_MASK                                   0x1
#define PMIC_LDO_VUSB33_EN_SHIFT                                  1
#define PMIC_LDO_VUSB33_LP_CTRL_ADDR                              MT6353_LDO1_VUSB33_CON0
#define PMIC_LDO_VUSB33_LP_CTRL_MASK                              0x1
#define PMIC_LDO_VUSB33_LP_CTRL_SHIFT                             2
#define PMIC_LDO_VUSB33_LP_SEL_ADDR                               MT6353_LDO1_VUSB33_CON0
#define PMIC_LDO_VUSB33_LP_SEL_MASK                               0x7
#define PMIC_LDO_VUSB33_LP_SEL_SHIFT                              5
#define PMIC_DA_QI_VUSB33_MODE_ADDR                               MT6353_LDO1_VUSB33_CON0
#define PMIC_DA_QI_VUSB33_MODE_MASK                               0x1
#define PMIC_DA_QI_VUSB33_MODE_SHIFT                              8
#define PMIC_LDO_VUSB33_STBTD_ADDR                                MT6353_LDO1_VUSB33_CON0
#define PMIC_LDO_VUSB33_STBTD_MASK                                0x1
#define PMIC_LDO_VUSB33_STBTD_SHIFT                               9
#define PMIC_DA_QI_VUSB33_STB_ADDR                                MT6353_LDO1_VUSB33_CON0
#define PMIC_DA_QI_VUSB33_STB_MASK                                0x1
#define PMIC_DA_QI_VUSB33_STB_SHIFT                               14
#define PMIC_DA_QI_VUSB33_EN_ADDR                                 MT6353_LDO1_VUSB33_CON0
#define PMIC_DA_QI_VUSB33_EN_MASK                                 0x1
#define PMIC_DA_QI_VUSB33_EN_SHIFT                                15
#define PMIC_LDO_VUSB33_OCFB_EN_ADDR                              MT6353_LDO1_VUSB33_OCFB_CON0
#define PMIC_LDO_VUSB33_OCFB_EN_MASK                              0x1
#define PMIC_LDO_VUSB33_OCFB_EN_SHIFT                             9
#define PMIC_DA_QI_VUSB33_OCFB_EN_ADDR                            MT6353_LDO1_VUSB33_OCFB_CON0
#define PMIC_DA_QI_VUSB33_OCFB_EN_MASK                            0x1
#define PMIC_DA_QI_VUSB33_OCFB_EN_SHIFT                           10
#define PMIC_LDO_VEMC33_LP_MODE_ADDR                              MT6353_LDO1_VEMC33_CON0
#define PMIC_LDO_VEMC33_LP_MODE_MASK                              0x1
#define PMIC_LDO_VEMC33_LP_MODE_SHIFT                             0
#define PMIC_LDO_VEMC33_EN_ADDR                                   MT6353_LDO1_VEMC33_CON0
#define PMIC_LDO_VEMC33_EN_MASK                                   0x1
#define PMIC_LDO_VEMC33_EN_SHIFT                                  1
#define PMIC_LDO_VEMC33_LP_CTRL_ADDR                              MT6353_LDO1_VEMC33_CON0
#define PMIC_LDO_VEMC33_LP_CTRL_MASK                              0x1
#define PMIC_LDO_VEMC33_LP_CTRL_SHIFT                             2
#define PMIC_LDO_VEMC33_LP_SEL_ADDR                               MT6353_LDO1_VEMC33_CON0
#define PMIC_LDO_VEMC33_LP_SEL_MASK                               0x7
#define PMIC_LDO_VEMC33_LP_SEL_SHIFT                              5
#define PMIC_DA_QI_VEMC33_MODE_ADDR                               MT6353_LDO1_VEMC33_CON0
#define PMIC_DA_QI_VEMC33_MODE_MASK                               0x1
#define PMIC_DA_QI_VEMC33_MODE_SHIFT                              8
#define PMIC_LDO_VEMC33_STBTD_ADDR                                MT6353_LDO1_VEMC33_CON0
#define PMIC_LDO_VEMC33_STBTD_MASK                                0x1
#define PMIC_LDO_VEMC33_STBTD_SHIFT                               9
#define PMIC_DA_QI_VEMC33_STB_ADDR                                MT6353_LDO1_VEMC33_CON0
#define PMIC_DA_QI_VEMC33_STB_MASK                                0x1
#define PMIC_DA_QI_VEMC33_STB_SHIFT                               14
#define PMIC_DA_QI_VEMC33_EN_ADDR                                 MT6353_LDO1_VEMC33_CON0
#define PMIC_DA_QI_VEMC33_EN_MASK                                 0x1
#define PMIC_DA_QI_VEMC33_EN_SHIFT                                15
#define PMIC_LDO_VEMC33_OCFB_EN_ADDR                              MT6353_LDO1_VEMC33_OCFB_CON0
#define PMIC_LDO_VEMC33_OCFB_EN_MASK                              0x1
#define PMIC_LDO_VEMC33_OCFB_EN_SHIFT                             9
#define PMIC_DA_QI_VEMC33_OCFB_EN_ADDR                            MT6353_LDO1_VEMC33_OCFB_CON0
#define PMIC_DA_QI_VEMC33_OCFB_EN_MASK                            0x1
#define PMIC_DA_QI_VEMC33_OCFB_EN_SHIFT                           10
#define PMIC_LDO_VEMC33_FAST_TRAN_DL_EN_ADDR                      MT6353_LDO_VEMC33_FAST_TRAN_CON0
#define PMIC_LDO_VEMC33_FAST_TRAN_DL_EN_MASK                      0x1
#define PMIC_LDO_VEMC33_FAST_TRAN_DL_EN_SHIFT                     0
#define PMIC_LDO_VEMC33_FAST_TRAN_DL_CTRL_ADDR                    MT6353_LDO_VEMC33_FAST_TRAN_CON0
#define PMIC_LDO_VEMC33_FAST_TRAN_DL_CTRL_MASK                    0x1
#define PMIC_LDO_VEMC33_FAST_TRAN_DL_CTRL_SHIFT                   1
#define PMIC_LDO_VEMC33_FAST_TRAN_DL_SRCLKEN_SEL_ADDR             MT6353_LDO_VEMC33_FAST_TRAN_CON0
#define PMIC_LDO_VEMC33_FAST_TRAN_DL_SRCLKEN_SEL_MASK             0x7
#define PMIC_LDO_VEMC33_FAST_TRAN_DL_SRCLKEN_SEL_SHIFT            2
#define PMIC_DA_QI_VEMC33_FAST_TRAN_DL_EN_ADDR                    MT6353_LDO_VEMC33_FAST_TRAN_CON0
#define PMIC_DA_QI_VEMC33_FAST_TRAN_DL_EN_MASK                    0x1
#define PMIC_DA_QI_VEMC33_FAST_TRAN_DL_EN_SHIFT                   5
#define PMIC_LDO_VEMC33_FAST_TRAN_CL_EN_ADDR                      MT6353_LDO_VEMC33_FAST_TRAN_CON0
#define PMIC_LDO_VEMC33_FAST_TRAN_CL_EN_MASK                      0x1
#define PMIC_LDO_VEMC33_FAST_TRAN_CL_EN_SHIFT                     8
#define PMIC_LDO_VEMC33_FAST_TRAN_CL_CTRL_ADDR                    MT6353_LDO_VEMC33_FAST_TRAN_CON0
#define PMIC_LDO_VEMC33_FAST_TRAN_CL_CTRL_MASK                    0x1
#define PMIC_LDO_VEMC33_FAST_TRAN_CL_CTRL_SHIFT                   9
#define PMIC_LDO_VEMC33_FAST_TRAN_CL_SRCLKEN_SEL_ADDR             MT6353_LDO_VEMC33_FAST_TRAN_CON0
#define PMIC_LDO_VEMC33_FAST_TRAN_CL_SRCLKEN_SEL_MASK             0x7
#define PMIC_LDO_VEMC33_FAST_TRAN_CL_SRCLKEN_SEL_SHIFT            10
#define PMIC_DA_QI_VEMC33_FAST_TRAN_CL_EN_ADDR                    MT6353_LDO_VEMC33_FAST_TRAN_CON0
#define PMIC_DA_QI_VEMC33_FAST_TRAN_CL_EN_MASK                    0x1
#define PMIC_DA_QI_VEMC33_FAST_TRAN_CL_EN_SHIFT                   13
#define PMIC_LDO_VIO18_LP_MODE_ADDR                               MT6353_LDO1_VIO18_CON0
#define PMIC_LDO_VIO18_LP_MODE_MASK                               0x1
#define PMIC_LDO_VIO18_LP_MODE_SHIFT                              0
#define PMIC_LDO_VIO18_EN_ADDR                                    MT6353_LDO1_VIO18_CON0
#define PMIC_LDO_VIO18_EN_MASK                                    0x1
#define PMIC_LDO_VIO18_EN_SHIFT                                   1
#define PMIC_LDO_VIO18_LP_CTRL_ADDR                               MT6353_LDO1_VIO18_CON0
#define PMIC_LDO_VIO18_LP_CTRL_MASK                               0x1
#define PMIC_LDO_VIO18_LP_CTRL_SHIFT                              2
#define PMIC_LDO_VIO18_LP_SEL_ADDR                                MT6353_LDO1_VIO18_CON0
#define PMIC_LDO_VIO18_LP_SEL_MASK                                0x7
#define PMIC_LDO_VIO18_LP_SEL_SHIFT                               5
#define PMIC_DA_QI_VIO18_MODE_ADDR                                MT6353_LDO1_VIO18_CON0
#define PMIC_DA_QI_VIO18_MODE_MASK                                0x1
#define PMIC_DA_QI_VIO18_MODE_SHIFT                               8
#define PMIC_LDO_VIO18_STBTD_ADDR                                 MT6353_LDO1_VIO18_CON0
#define PMIC_LDO_VIO18_STBTD_MASK                                 0x1
#define PMIC_LDO_VIO18_STBTD_SHIFT                                9
#define PMIC_DA_QI_VIO18_STB_ADDR                                 MT6353_LDO1_VIO18_CON0
#define PMIC_DA_QI_VIO18_STB_MASK                                 0x1
#define PMIC_DA_QI_VIO18_STB_SHIFT                                14
#define PMIC_DA_QI_VIO18_EN_ADDR                                  MT6353_LDO1_VIO18_CON0
#define PMIC_DA_QI_VIO18_EN_MASK                                  0x1
#define PMIC_DA_QI_VIO18_EN_SHIFT                                 15
#define PMIC_LDO_VIO18_OCFB_EN_ADDR                               MT6353_LDO1_VIO18_OCFB_CON0
#define PMIC_LDO_VIO18_OCFB_EN_MASK                               0x1
#define PMIC_LDO_VIO18_OCFB_EN_SHIFT                              9
#define PMIC_DA_QI_VIO18_OCFB_EN_ADDR                             MT6353_LDO1_VIO18_OCFB_CON0
#define PMIC_DA_QI_VIO18_OCFB_EN_MASK                             0x1
#define PMIC_DA_QI_VIO18_OCFB_EN_SHIFT                            10
#define PMIC_LDO_VIBR_LP_MODE_ADDR                                MT6353_LDO0_VIBR_CON0
#define PMIC_LDO_VIBR_LP_MODE_MASK                                0x1
#define PMIC_LDO_VIBR_LP_MODE_SHIFT                               0
#define PMIC_LDO_VIBR_EN_ADDR                                     MT6353_LDO0_VIBR_CON0
#define PMIC_LDO_VIBR_EN_MASK                                     0x1
#define PMIC_LDO_VIBR_EN_SHIFT                                    1
#define PMIC_DA_QI_VIBR_MODE_ADDR                                 MT6353_LDO0_VIBR_CON0
#define PMIC_DA_QI_VIBR_MODE_MASK                                 0x1
#define PMIC_DA_QI_VIBR_MODE_SHIFT                                8
#define PMIC_LDO_VIBR_STBTD_ADDR                                  MT6353_LDO0_VIBR_CON0
#define PMIC_LDO_VIBR_STBTD_MASK                                  0x1
#define PMIC_LDO_VIBR_STBTD_SHIFT                                 9
#define PMIC_DA_QI_VIBR_STB_ADDR                                  MT6353_LDO0_VIBR_CON0
#define PMIC_DA_QI_VIBR_STB_MASK                                  0x1
#define PMIC_DA_QI_VIBR_STB_SHIFT                                 14
#define PMIC_DA_QI_VIBR_EN_ADDR                                   MT6353_LDO0_VIBR_CON0
#define PMIC_DA_QI_VIBR_EN_MASK                                   0x1
#define PMIC_DA_QI_VIBR_EN_SHIFT                                  15
#define PMIC_LDO_VIBR_OCFB_EN_ADDR                                MT6353_LDO0_VIBR_OCFB_CON0
#define PMIC_LDO_VIBR_OCFB_EN_MASK                                0x1
#define PMIC_LDO_VIBR_OCFB_EN_SHIFT                               9
#define PMIC_DA_QI_VIBR_OCFB_EN_ADDR                              MT6353_LDO0_VIBR_OCFB_CON0
#define PMIC_DA_QI_VIBR_OCFB_EN_MASK                              0x1
#define PMIC_DA_QI_VIBR_OCFB_EN_SHIFT                             10
#define PMIC_RG_VTCXO28_CAL_ADDR                                  MT6353_ALDO_ANA_CON0
#define PMIC_RG_VTCXO28_CAL_MASK                                  0xF
#define PMIC_RG_VTCXO28_CAL_SHIFT                                 0
#define PMIC_RG_VTCXO28_VOSEL_ADDR                                MT6353_ALDO_ANA_CON0
#define PMIC_RG_VTCXO28_VOSEL_MASK                                0x3
#define PMIC_RG_VTCXO28_VOSEL_SHIFT                               8
#define PMIC_RG_VTCXO28_NDIS_EN_ADDR                              MT6353_ALDO_ANA_CON0
#define PMIC_RG_VTCXO28_NDIS_EN_MASK                              0x1
#define PMIC_RG_VTCXO28_NDIS_EN_SHIFT                             12
#define PMIC_RG_VAUD28_CAL_ADDR                                   MT6353_ALDO_ANA_CON1
#define PMIC_RG_VAUD28_CAL_MASK                                   0xF
#define PMIC_RG_VAUD28_CAL_SHIFT                                  0
#define PMIC_RG_VAUD28_VOSEL_ADDR                                 MT6353_ALDO_ANA_CON1
#define PMIC_RG_VAUD28_VOSEL_MASK                                 0x3
#define PMIC_RG_VAUD28_VOSEL_SHIFT                                8
#define PMIC_RG_VAUD28_NDIS_EN_ADDR                               MT6353_ALDO_ANA_CON1
#define PMIC_RG_VAUD28_NDIS_EN_MASK                               0x1
#define PMIC_RG_VAUD28_NDIS_EN_SHIFT                              12
#define PMIC_RG_VCN28_CAL_ADDR                                    MT6353_ALDO_ANA_CON2
#define PMIC_RG_VCN28_CAL_MASK                                    0xF
#define PMIC_RG_VCN28_CAL_SHIFT                                   0
#define PMIC_RG_VCN28_VOSEL_ADDR                                  MT6353_ALDO_ANA_CON2
#define PMIC_RG_VCN28_VOSEL_MASK                                  0x3
#define PMIC_RG_VCN28_VOSEL_SHIFT                                 8
#define PMIC_RG_VCN28_NDIS_EN_ADDR                                MT6353_ALDO_ANA_CON2
#define PMIC_RG_VCN28_NDIS_EN_MASK                                0x1
#define PMIC_RG_VCN28_NDIS_EN_SHIFT                               12
#define PMIC_RG_VAUX18_CAL_ADDR                                   MT6353_ALDO_ANA_CON3
#define PMIC_RG_VAUX18_CAL_MASK                                   0xF
#define PMIC_RG_VAUX18_CAL_SHIFT                                  0
#define PMIC_RG_VAUX18_VOSEL_ADDR                                 MT6353_ALDO_ANA_CON3
#define PMIC_RG_VAUX18_VOSEL_MASK                                 0x3
#define PMIC_RG_VAUX18_VOSEL_SHIFT                                8
#define PMIC_RG_VAUX18_NDIS_EN_ADDR                               MT6353_ALDO_ANA_CON3
#define PMIC_RG_VAUX18_NDIS_EN_MASK                               0x1
#define PMIC_RG_VAUX18_NDIS_EN_SHIFT                              12
#define PMIC_RG_VCAMA_CAL_ADDR                                    MT6353_ALDO_ANA_CON4
#define PMIC_RG_VCAMA_CAL_MASK                                    0xF
#define PMIC_RG_VCAMA_CAL_SHIFT                                   0
#define PMIC_RG_VCAMA_VOSEL_ADDR                                  MT6353_ALDO_ANA_CON4
#define PMIC_RG_VCAMA_VOSEL_MASK                                  0x3
#define PMIC_RG_VCAMA_VOSEL_SHIFT                                 8
#define PMIC_RG_VCAMA_NDIS_EN_ADDR                                MT6353_ALDO_ANA_CON4
#define PMIC_RG_VCAMA_NDIS_EN_MASK                                0x1
#define PMIC_RG_VCAMA_NDIS_EN_SHIFT                               12
#define PMIC_RG_ALDO_RSV_H_ADDR                                   MT6353_ALDO_ANA_CON5
#define PMIC_RG_ALDO_RSV_H_MASK                                   0x3FF
#define PMIC_RG_ALDO_RSV_H_SHIFT                                  0
#define PMIC_RG_ALDO_RSV_L_ADDR                                   MT6353_ALDO_ANA_CON6
#define PMIC_RG_ALDO_RSV_L_MASK                                   0x3FF
#define PMIC_RG_ALDO_RSV_L_SHIFT                                  0
#define PMIC_RG_VXO22_CAL_ADDR                                    MT6353_DLDO_ANA_CON0
#define PMIC_RG_VXO22_CAL_MASK                                    0xF
#define PMIC_RG_VXO22_CAL_SHIFT                                   0
#define PMIC_RG_VXO22_VOSEL_ADDR                                  MT6353_DLDO_ANA_CON0
#define PMIC_RG_VXO22_VOSEL_MASK                                  0x3
#define PMIC_RG_VXO22_VOSEL_SHIFT                                 8
#define PMIC_RG_VXO22_NDIS_EN_ADDR                                MT6353_DLDO_ANA_CON0
#define PMIC_RG_VXO22_NDIS_EN_MASK                                0x1
#define PMIC_RG_VXO22_NDIS_EN_SHIFT                               12
#define PMIC_RG_VTCXO24_CAL_ADDR                                  MT6353_DLDO_ANA_CON1
#define PMIC_RG_VTCXO24_CAL_MASK                                  0xF
#define PMIC_RG_VTCXO24_CAL_SHIFT                                 0
#define PMIC_RG_VTCXO24_VOSEL_ADDR                                MT6353_DLDO_ANA_CON1
#define PMIC_RG_VTCXO24_VOSEL_MASK                                0x3
#define PMIC_RG_VTCXO24_VOSEL_SHIFT                               8
#define PMIC_RG_VTCXO24_NDIS_EN_ADDR                              MT6353_DLDO_ANA_CON1
#define PMIC_RG_VTCXO24_NDIS_EN_MASK                              0x1
#define PMIC_RG_VTCXO24_NDIS_EN_SHIFT                             12
#define PMIC_RG_VSIM1_CAL_ADDR                                    MT6353_DLDO_ANA_CON2
#define PMIC_RG_VSIM1_CAL_MASK                                    0xF
#define PMIC_RG_VSIM1_CAL_SHIFT                                   0
#define PMIC_RG_VSIM1_VOSEL_ADDR                                  MT6353_DLDO_ANA_CON2
#define PMIC_RG_VSIM1_VOSEL_MASK                                  0x7
#define PMIC_RG_VSIM1_VOSEL_SHIFT                                 8
#define PMIC_RG_VSIM1_NDIS_EN_ADDR                                MT6353_DLDO_ANA_CON2
#define PMIC_RG_VSIM1_NDIS_EN_MASK                                0x1
#define PMIC_RG_VSIM1_NDIS_EN_SHIFT                               12
#define PMIC_RG_VSIM2_CAL_ADDR                                    MT6353_DLDO_ANA_CON3
#define PMIC_RG_VSIM2_CAL_MASK                                    0xF
#define PMIC_RG_VSIM2_CAL_SHIFT                                   0
#define PMIC_RG_VSIM2_VOSEL_ADDR                                  MT6353_DLDO_ANA_CON3
#define PMIC_RG_VSIM2_VOSEL_MASK                                  0x7
#define PMIC_RG_VSIM2_VOSEL_SHIFT                                 8
#define PMIC_RG_VSIM2_NDIS_EN_ADDR                                MT6353_DLDO_ANA_CON3
#define PMIC_RG_VSIM2_NDIS_EN_MASK                                0x1
#define PMIC_RG_VSIM2_NDIS_EN_SHIFT                               12
#define PMIC_RG_VCN33_CAL_ADDR                                    MT6353_DLDO_ANA_CON4
#define PMIC_RG_VCN33_CAL_MASK                                    0xF
#define PMIC_RG_VCN33_CAL_SHIFT                                   0
#define PMIC_RG_VCN33_VOSEL_ADDR                                  MT6353_DLDO_ANA_CON4
#define PMIC_RG_VCN33_VOSEL_MASK                                  0x7
#define PMIC_RG_VCN33_VOSEL_SHIFT                                 8
#define PMIC_RG_VCN33_NDIS_EN_ADDR                                MT6353_DLDO_ANA_CON4
#define PMIC_RG_VCN33_NDIS_EN_MASK                                0x1
#define PMIC_RG_VCN33_NDIS_EN_SHIFT                               12
#define PMIC_RG_VUSB33_CAL_ADDR                                   MT6353_DLDO_ANA_CON5
#define PMIC_RG_VUSB33_CAL_MASK                                   0xF
#define PMIC_RG_VUSB33_CAL_SHIFT                                  0
#define PMIC_RG_VUSB33_NDIS_EN_ADDR                               MT6353_DLDO_ANA_CON5
#define PMIC_RG_VUSB33_NDIS_EN_MASK                               0x1
#define PMIC_RG_VUSB33_NDIS_EN_SHIFT                              12
#define PMIC_RG_VMCH_CAL_ADDR                                     MT6353_DLDO_ANA_CON6
#define PMIC_RG_VMCH_CAL_MASK                                     0xF
#define PMIC_RG_VMCH_CAL_SHIFT                                    0
#define PMIC_RG_VMCH_VOSEL_ADDR                                   MT6353_DLDO_ANA_CON6
#define PMIC_RG_VMCH_VOSEL_MASK                                   0x3
#define PMIC_RG_VMCH_VOSEL_SHIFT                                  8
#define PMIC_RG_VMCH_NDIS_EN_ADDR                                 MT6353_DLDO_ANA_CON6
#define PMIC_RG_VMCH_NDIS_EN_MASK                                 0x1
#define PMIC_RG_VMCH_NDIS_EN_SHIFT                                12
#define PMIC_RG_VMCH_OC_TRIM_ADDR                                 MT6353_DLDO_ANA_CON7
#define PMIC_RG_VMCH_OC_TRIM_MASK                                 0xF
#define PMIC_RG_VMCH_OC_TRIM_SHIFT                                0
#define PMIC_RG_VMCH_DUMMY_LOAD_ADDR                              MT6353_DLDO_ANA_CON7
#define PMIC_RG_VMCH_DUMMY_LOAD_MASK                              0xF
#define PMIC_RG_VMCH_DUMMY_LOAD_SHIFT                             4
#define PMIC_RG_VMCH_DL_EN_ADDR                                   MT6353_DLDO_ANA_CON7
#define PMIC_RG_VMCH_DL_EN_MASK                                   0x1
#define PMIC_RG_VMCH_DL_EN_SHIFT                                  8
#define PMIC_RG_VMCH_CL_EN_ADDR                                   MT6353_DLDO_ANA_CON7
#define PMIC_RG_VMCH_CL_EN_MASK                                   0x1
#define PMIC_RG_VMCH_CL_EN_SHIFT                                  9
#define PMIC_RG_VMCH_STB_SEL_ADDR                                 MT6353_DLDO_ANA_CON7
#define PMIC_RG_VMCH_STB_SEL_MASK                                 0x1
#define PMIC_RG_VMCH_STB_SEL_SHIFT                                10
#define PMIC_RG_VEMC33_CAL_ADDR                                   MT6353_DLDO_ANA_CON8
#define PMIC_RG_VEMC33_CAL_MASK                                   0xF
#define PMIC_RG_VEMC33_CAL_SHIFT                                  0
#define PMIC_RG_VEMC33_VOSEL_ADDR                                 MT6353_DLDO_ANA_CON8
#define PMIC_RG_VEMC33_VOSEL_MASK                                 0x3
#define PMIC_RG_VEMC33_VOSEL_SHIFT                                8
#define PMIC_RG_VEMC33_NDIS_EN_ADDR                               MT6353_DLDO_ANA_CON8
#define PMIC_RG_VEMC33_NDIS_EN_MASK                               0x1
#define PMIC_RG_VEMC33_NDIS_EN_SHIFT                              12
#define PMIC_RG_VEMC33_OC_TRIM_ADDR                               MT6353_DLDO_ANA_CON9
#define PMIC_RG_VEMC33_OC_TRIM_MASK                               0xF
#define PMIC_RG_VEMC33_OC_TRIM_SHIFT                              0
#define PMIC_RG_VEMC33_DUMMY_LOAD_ADDR                            MT6353_DLDO_ANA_CON9
#define PMIC_RG_VEMC33_DUMMY_LOAD_MASK                            0xF
#define PMIC_RG_VEMC33_DUMMY_LOAD_SHIFT                           4
#define PMIC_RG_VEMC33_DL_EN_ADDR                                 MT6353_DLDO_ANA_CON9
#define PMIC_RG_VEMC33_DL_EN_MASK                                 0x1
#define PMIC_RG_VEMC33_DL_EN_SHIFT                                8
#define PMIC_RG_VEMC33_CL_EN_ADDR                                 MT6353_DLDO_ANA_CON9
#define PMIC_RG_VEMC33_CL_EN_MASK                                 0x1
#define PMIC_RG_VEMC33_CL_EN_SHIFT                                9
#define PMIC_RG_VEMC33_STB_SEL_ADDR                               MT6353_DLDO_ANA_CON9
#define PMIC_RG_VEMC33_STB_SEL_MASK                               0x1
#define PMIC_RG_VEMC33_STB_SEL_SHIFT                              10
#define PMIC_RG_VIO28_CAL_ADDR                                    MT6353_DLDO_ANA_CON10
#define PMIC_RG_VIO28_CAL_MASK                                    0xF
#define PMIC_RG_VIO28_CAL_SHIFT                                   0
#define PMIC_RG_VIO28_NDIS_EN_ADDR                                MT6353_DLDO_ANA_CON10
#define PMIC_RG_VIO28_NDIS_EN_MASK                                0x1
#define PMIC_RG_VIO28_NDIS_EN_SHIFT                               12
#define PMIC_RG_VIBR_CAL_ADDR                                     MT6353_DLDO_ANA_CON11
#define PMIC_RG_VIBR_CAL_MASK                                     0xF
#define PMIC_RG_VIBR_CAL_SHIFT                                    0
#define PMIC_RG_VIBR_VOSEL_ADDR                                   MT6353_DLDO_ANA_CON11
#define PMIC_RG_VIBR_VOSEL_MASK                                   0x7
#define PMIC_RG_VIBR_VOSEL_SHIFT                                  8
#define PMIC_RG_VIBR_NDIS_EN_ADDR                                 MT6353_DLDO_ANA_CON11
#define PMIC_RG_VIBR_NDIS_EN_MASK                                 0x1
#define PMIC_RG_VIBR_NDIS_EN_SHIFT                                12
#define PMIC_RG_VLDO28_CAL_ADDR                                   MT6353_DLDO_ANA_CON12
#define PMIC_RG_VLDO28_CAL_MASK                                   0xF
#define PMIC_RG_VLDO28_CAL_SHIFT                                  0
#define PMIC_RG_VLDO28_VOSEL_ADDR                                 MT6353_DLDO_ANA_CON12
#define PMIC_RG_VLDO28_VOSEL_MASK                                 0x7
#define PMIC_RG_VLDO28_VOSEL_SHIFT                                8
#define PMIC_RG_VLDO28_NDIS_EN_ADDR                               MT6353_DLDO_ANA_CON12
#define PMIC_RG_VLDO28_NDIS_EN_MASK                               0x1
#define PMIC_RG_VLDO28_NDIS_EN_SHIFT                              12
#define PMIC_RG_VLDO28_DUMMY_LOAD_ADDR                            MT6353_DLDO_ANA_CON13
#define PMIC_RG_VLDO28_DUMMY_LOAD_MASK                            0xF
#define PMIC_RG_VLDO28_DUMMY_LOAD_SHIFT                           0
#define PMIC_RG_VLDO28_DL_EN_ADDR                                 MT6353_DLDO_ANA_CON13
#define PMIC_RG_VLDO28_DL_EN_MASK                                 0x1
#define PMIC_RG_VLDO28_DL_EN_SHIFT                                4
#define PMIC_RG_VLDO28_CL_EN_ADDR                                 MT6353_DLDO_ANA_CON13
#define PMIC_RG_VLDO28_CL_EN_MASK                                 0x1
#define PMIC_RG_VLDO28_CL_EN_SHIFT                                5
#define PMIC_RG_VMC_CAL_ADDR                                      MT6353_DLDO_ANA_CON14
#define PMIC_RG_VMC_CAL_MASK                                      0xF
#define PMIC_RG_VMC_CAL_SHIFT                                     0
#define PMIC_RG_VMC_VOSEL_ADDR                                    MT6353_DLDO_ANA_CON14
#define PMIC_RG_VMC_VOSEL_MASK                                    0x7
#define PMIC_RG_VMC_VOSEL_SHIFT                                   8
#define PMIC_RG_VMC_NDIS_EN_ADDR                                  MT6353_DLDO_ANA_CON14
#define PMIC_RG_VMC_NDIS_EN_MASK                                  0x1
#define PMIC_RG_VMC_NDIS_EN_SHIFT                                 13
#define PMIC_RG_VMC_DUMMY_LOAD_ADDR                               MT6353_DLDO_ANA_CON15
#define PMIC_RG_VMC_DUMMY_LOAD_MASK                               0xF
#define PMIC_RG_VMC_DUMMY_LOAD_SHIFT                              0
#define PMIC_RG_VMC_DL_EN_ADDR                                    MT6353_DLDO_ANA_CON15
#define PMIC_RG_VMC_DL_EN_MASK                                    0x1
#define PMIC_RG_VMC_DL_EN_SHIFT                                   4
#define PMIC_RG_VMC_CL_EN_ADDR                                    MT6353_DLDO_ANA_CON15
#define PMIC_RG_VMC_CL_EN_MASK                                    0x1
#define PMIC_RG_VMC_CL_EN_SHIFT                                   5
#define PMIC_RG_DLDO_RSV_H_ADDR                                   MT6353_DLDO_ANA_CON16
#define PMIC_RG_DLDO_RSV_H_MASK                                   0x3FF
#define PMIC_RG_DLDO_RSV_H_SHIFT                                  0
#define PMIC_RG_DLDO_RSV_L_ADDR                                   MT6353_DLDO_ANA_CON17
#define PMIC_RG_DLDO_RSV_L_MASK                                   0x3FF
#define PMIC_RG_DLDO_RSV_L_SHIFT                                  0
#define PMIC_RG_VCAMD_CAL_ADDR                                    MT6353_SLDO_ANA_CON0
#define PMIC_RG_VCAMD_CAL_MASK                                    0xF
#define PMIC_RG_VCAMD_CAL_SHIFT                                   0
#define PMIC_RG_VCAMD_VOSEL_ADDR                                  MT6353_SLDO_ANA_CON0
#define PMIC_RG_VCAMD_VOSEL_MASK                                  0x7
#define PMIC_RG_VCAMD_VOSEL_SHIFT                                 8
#define PMIC_RG_VCAMD_NDIS_EN_ADDR                                MT6353_SLDO_ANA_CON0
#define PMIC_RG_VCAMD_NDIS_EN_MASK                                0x1
#define PMIC_RG_VCAMD_NDIS_EN_SHIFT                               12
#define PMIC_RG_VRF18_CAL_ADDR                                    MT6353_SLDO_ANA_CON1
#define PMIC_RG_VRF18_CAL_MASK                                    0xF
#define PMIC_RG_VRF18_CAL_SHIFT                                   0
#define PMIC_RG_VRF18_NDIS_EN_ADDR                                MT6353_SLDO_ANA_CON1
#define PMIC_RG_VRF18_NDIS_EN_MASK                                0x1
#define PMIC_RG_VRF18_NDIS_EN_SHIFT                               12
#define PMIC_RG_VRF12_CAL_ADDR                                    MT6353_SLDO_ANA_CON2
#define PMIC_RG_VRF12_CAL_MASK                                    0xF
#define PMIC_RG_VRF12_CAL_SHIFT                                   0
#define PMIC_RG_VRF12_VOSEL_ADDR                                  MT6353_SLDO_ANA_CON2
#define PMIC_RG_VRF12_VOSEL_MASK                                  0x7
#define PMIC_RG_VRF12_VOSEL_SHIFT                                 8
#define PMIC_RG_VRF12_NDIS_EN_ADDR                                MT6353_SLDO_ANA_CON2
#define PMIC_RG_VRF12_NDIS_EN_MASK                                0x1
#define PMIC_RG_VRF12_NDIS_EN_SHIFT                               12
#define PMIC_RG_VRF12_STB_SEL_ADDR                                MT6353_SLDO_ANA_CON2
#define PMIC_RG_VRF12_STB_SEL_MASK                                0x1
#define PMIC_RG_VRF12_STB_SEL_SHIFT                               13
#define PMIC_RG_VRF12_DUMMY_LOAD_ADDR                             MT6353_SLDO_ANA_CON3
#define PMIC_RG_VRF12_DUMMY_LOAD_MASK                             0x1F
#define PMIC_RG_VRF12_DUMMY_LOAD_SHIFT                            0
#define PMIC_RG_VRF12_DL_EN_ADDR                                  MT6353_SLDO_ANA_CON3
#define PMIC_RG_VRF12_DL_EN_MASK                                  0x1
#define PMIC_RG_VRF12_DL_EN_SHIFT                                 5
#define PMIC_RG_VRF12_CL_EN_ADDR                                  MT6353_SLDO_ANA_CON3
#define PMIC_RG_VRF12_CL_EN_MASK                                  0x1
#define PMIC_RG_VRF12_CL_EN_SHIFT                                 6
#define PMIC_RG_VIO18_CAL_ADDR                                    MT6353_SLDO_ANA_CON4
#define PMIC_RG_VIO18_CAL_MASK                                    0xF
#define PMIC_RG_VIO18_CAL_SHIFT                                   0
#define PMIC_RG_VIO18_NDIS_EN_ADDR                                MT6353_SLDO_ANA_CON4
#define PMIC_RG_VIO18_NDIS_EN_MASK                                0x1
#define PMIC_RG_VIO18_NDIS_EN_SHIFT                               12
#define PMIC_RG_VDRAM_CAL_ADDR                                    MT6353_SLDO_ANA_CON5
#define PMIC_RG_VDRAM_CAL_MASK                                    0xF
#define PMIC_RG_VDRAM_CAL_SHIFT                                   0
#define PMIC_RG_VDRAM_VOSEL_ADDR                                  MT6353_SLDO_ANA_CON5
#define PMIC_RG_VDRAM_VOSEL_MASK                                  0x3
#define PMIC_RG_VDRAM_VOSEL_SHIFT                                 8
#define PMIC_RG_VDRAM_NDIS_EN_ADDR                                MT6353_SLDO_ANA_CON5
#define PMIC_RG_VDRAM_NDIS_EN_MASK                                0x1
#define PMIC_RG_VDRAM_NDIS_EN_SHIFT                               11
#define PMIC_RG_VDRAM_PCUR_CAL_ADDR                               MT6353_SLDO_ANA_CON5
#define PMIC_RG_VDRAM_PCUR_CAL_MASK                               0x3
#define PMIC_RG_VDRAM_PCUR_CAL_SHIFT                              12
#define PMIC_RG_VDRAM_DL_EN_ADDR                                  MT6353_SLDO_ANA_CON5
#define PMIC_RG_VDRAM_DL_EN_MASK                                  0x1
#define PMIC_RG_VDRAM_DL_EN_SHIFT                                 14
#define PMIC_RG_VDRAM_CL_EN_ADDR                                  MT6353_SLDO_ANA_CON5
#define PMIC_RG_VDRAM_CL_EN_MASK                                  0x1
#define PMIC_RG_VDRAM_CL_EN_SHIFT                                 15
#define PMIC_RG_VCAMIO_CAL_ADDR                                   MT6353_SLDO_ANA_CON6
#define PMIC_RG_VCAMIO_CAL_MASK                                   0xF
#define PMIC_RG_VCAMIO_CAL_SHIFT                                  0
#define PMIC_RG_VCAMIO_VOSEL_ADDR                                 MT6353_SLDO_ANA_CON6
#define PMIC_RG_VCAMIO_VOSEL_MASK                                 0x3
#define PMIC_RG_VCAMIO_VOSEL_SHIFT                                8
#define PMIC_RG_VCAMIO_NDIS_EN_ADDR                               MT6353_SLDO_ANA_CON6
#define PMIC_RG_VCAMIO_NDIS_EN_MASK                               0x1
#define PMIC_RG_VCAMIO_NDIS_EN_SHIFT                              12
#define PMIC_RG_VCN18_CAL_ADDR                                    MT6353_SLDO_ANA_CON7
#define PMIC_RG_VCN18_CAL_MASK                                    0xF
#define PMIC_RG_VCN18_CAL_SHIFT                                   0
#define PMIC_RG_VCN18_NDIS_EN_ADDR                                MT6353_SLDO_ANA_CON7
#define PMIC_RG_VCN18_NDIS_EN_MASK                                0x1
#define PMIC_RG_VCN18_NDIS_EN_SHIFT                               12
#define PMIC_RG_VSRAM_PROC_VOSEL_ADDR                             MT6353_SLDO_ANA_CON8
#define PMIC_RG_VSRAM_PROC_VOSEL_MASK                             0x7F
#define PMIC_RG_VSRAM_PROC_VOSEL_SHIFT                            0
#define PMIC_RG_VSRAM_PROC_NDIS_PLCUR_ADDR                        MT6353_SLDO_ANA_CON8
#define PMIC_RG_VSRAM_PROC_NDIS_PLCUR_MASK                        0x3
#define PMIC_RG_VSRAM_PROC_NDIS_PLCUR_SHIFT                       7
#define PMIC_RG_VSRAM_PROC_NDIS_EN_ADDR                           MT6353_SLDO_ANA_CON8
#define PMIC_RG_VSRAM_PROC_NDIS_EN_MASK                           0x1
#define PMIC_RG_VSRAM_PROC_NDIS_EN_SHIFT                          9
#define PMIC_RG_VSRAM_PROC_PLCUR_EN_ADDR                          MT6353_SLDO_ANA_CON8
#define PMIC_RG_VSRAM_PROC_PLCUR_EN_MASK                          0x1
#define PMIC_RG_VSRAM_PROC_PLCUR_EN_SHIFT                         10
#define PMIC_RG_SLDO_RSV_H_ADDR                                   MT6353_SLDO_ANA_CON9
#define PMIC_RG_SLDO_RSV_H_MASK                                   0x3FF
#define PMIC_RG_SLDO_RSV_H_SHIFT                                  0
#define PMIC_RG_SLDO_RSV_L_ADDR                                   MT6353_SLDO_ANA_CON10
#define PMIC_RG_SLDO_RSV_L_MASK                                   0x3FF
#define PMIC_RG_SLDO_RSV_L_SHIFT                                  0
#define PMIC_RG_OTP_PA_ADDR                                       MT6353_OTP_CON0
#define PMIC_RG_OTP_PA_MASK                                       0x3F
#define PMIC_RG_OTP_PA_SHIFT                                      0
#define PMIC_RG_OTP_PDIN_ADDR                                     MT6353_OTP_CON1
#define PMIC_RG_OTP_PDIN_MASK                                     0xFF
#define PMIC_RG_OTP_PDIN_SHIFT                                    0
#define PMIC_RG_OTP_PTM_ADDR                                      MT6353_OTP_CON2
#define PMIC_RG_OTP_PTM_MASK                                      0x3
#define PMIC_RG_OTP_PTM_SHIFT                                     0
#define PMIC_RG_OTP_PWE_ADDR                                      MT6353_OTP_CON3
#define PMIC_RG_OTP_PWE_MASK                                      0x3
#define PMIC_RG_OTP_PWE_SHIFT                                     0
#define PMIC_RG_OTP_PPROG_ADDR                                    MT6353_OTP_CON4
#define PMIC_RG_OTP_PPROG_MASK                                    0x1
#define PMIC_RG_OTP_PPROG_SHIFT                                   0
#define PMIC_RG_OTP_PWE_SRC_ADDR                                  MT6353_OTP_CON5
#define PMIC_RG_OTP_PWE_SRC_MASK                                  0x1
#define PMIC_RG_OTP_PWE_SRC_SHIFT                                 0
#define PMIC_RG_OTP_PROG_PKEY_ADDR                                MT6353_OTP_CON6
#define PMIC_RG_OTP_PROG_PKEY_MASK                                0xFFFF
#define PMIC_RG_OTP_PROG_PKEY_SHIFT                               0
#define PMIC_RG_OTP_RD_PKEY_ADDR                                  MT6353_OTP_CON7
#define PMIC_RG_OTP_RD_PKEY_MASK                                  0xFFFF
#define PMIC_RG_OTP_RD_PKEY_SHIFT                                 0
#define PMIC_RG_OTP_RD_TRIG_ADDR                                  MT6353_OTP_CON8
#define PMIC_RG_OTP_RD_TRIG_MASK                                  0x1
#define PMIC_RG_OTP_RD_TRIG_SHIFT                                 0
#define PMIC_RG_RD_RDY_BYPASS_ADDR                                MT6353_OTP_CON9
#define PMIC_RG_RD_RDY_BYPASS_MASK                                0x1
#define PMIC_RG_RD_RDY_BYPASS_SHIFT                               0
#define PMIC_RG_SKIP_OTP_OUT_ADDR                                 MT6353_OTP_CON10
#define PMIC_RG_SKIP_OTP_OUT_MASK                                 0x1
#define PMIC_RG_SKIP_OTP_OUT_SHIFT                                0
#define PMIC_RG_OTP_RD_SW_ADDR                                    MT6353_OTP_CON11
#define PMIC_RG_OTP_RD_SW_MASK                                    0x1
#define PMIC_RG_OTP_RD_SW_SHIFT                                   0
#define PMIC_RG_OTP_DOUT_SW_ADDR                                  MT6353_OTP_CON12
#define PMIC_RG_OTP_DOUT_SW_MASK                                  0xFFFF
#define PMIC_RG_OTP_DOUT_SW_SHIFT                                 0
#define PMIC_RG_OTP_RD_BUSY_ADDR                                  MT6353_OTP_CON13
#define PMIC_RG_OTP_RD_BUSY_MASK                                  0x1
#define PMIC_RG_OTP_RD_BUSY_SHIFT                                 0
#define PMIC_RG_OTP_RD_ACK_ADDR                                   MT6353_OTP_CON13
#define PMIC_RG_OTP_RD_ACK_MASK                                   0x1
#define PMIC_RG_OTP_RD_ACK_SHIFT                                  2
#define PMIC_RG_OTP_PA_SW_ADDR                                    MT6353_OTP_CON14
#define PMIC_RG_OTP_PA_SW_MASK                                    0x1F
#define PMIC_RG_OTP_PA_SW_SHIFT                                   0
#define PMIC_RG_OTP_DOUT_0_15_ADDR                                MT6353_OTP_DOUT_0_15
#define PMIC_RG_OTP_DOUT_0_15_MASK                                0xFFFF
#define PMIC_RG_OTP_DOUT_0_15_SHIFT                               0
#define PMIC_RG_OTP_DOUT_16_31_ADDR                               MT6353_OTP_DOUT_16_31
#define PMIC_RG_OTP_DOUT_16_31_MASK                               0xFFFF
#define PMIC_RG_OTP_DOUT_16_31_SHIFT                              0
#define PMIC_RG_OTP_DOUT_32_47_ADDR                               MT6353_OTP_DOUT_32_47
#define PMIC_RG_OTP_DOUT_32_47_MASK                               0xFFFF
#define PMIC_RG_OTP_DOUT_32_47_SHIFT                              0
#define PMIC_RG_OTP_DOUT_48_63_ADDR                               MT6353_OTP_DOUT_48_63
#define PMIC_RG_OTP_DOUT_48_63_MASK                               0xFFFF
#define PMIC_RG_OTP_DOUT_48_63_SHIFT                              0
#define PMIC_RG_OTP_DOUT_64_79_ADDR                               MT6353_OTP_DOUT_64_79
#define PMIC_RG_OTP_DOUT_64_79_MASK                               0xFFFF
#define PMIC_RG_OTP_DOUT_64_79_SHIFT                              0
#define PMIC_RG_OTP_DOUT_80_95_ADDR                               MT6353_OTP_DOUT_80_95
#define PMIC_RG_OTP_DOUT_80_95_MASK                               0xFFFF
#define PMIC_RG_OTP_DOUT_80_95_SHIFT                              0
#define PMIC_RG_OTP_DOUT_96_111_ADDR                              MT6353_OTP_DOUT_96_111
#define PMIC_RG_OTP_DOUT_96_111_MASK                              0xFFFF
#define PMIC_RG_OTP_DOUT_96_111_SHIFT                             0
#define PMIC_RG_OTP_DOUT_112_127_ADDR                             MT6353_OTP_DOUT_112_127
#define PMIC_RG_OTP_DOUT_112_127_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_112_127_SHIFT                            0
#define PMIC_RG_OTP_DOUT_128_143_ADDR                             MT6353_OTP_DOUT_128_143
#define PMIC_RG_OTP_DOUT_128_143_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_128_143_SHIFT                            0
#define PMIC_RG_OTP_DOUT_144_159_ADDR                             MT6353_OTP_DOUT_144_159
#define PMIC_RG_OTP_DOUT_144_159_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_144_159_SHIFT                            0
#define PMIC_RG_OTP_DOUT_160_175_ADDR                             MT6353_OTP_DOUT_160_175
#define PMIC_RG_OTP_DOUT_160_175_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_160_175_SHIFT                            0
#define PMIC_RG_OTP_DOUT_176_191_ADDR                             MT6353_OTP_DOUT_176_191
#define PMIC_RG_OTP_DOUT_176_191_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_176_191_SHIFT                            0
#define PMIC_RG_OTP_DOUT_192_207_ADDR                             MT6353_OTP_DOUT_192_207
#define PMIC_RG_OTP_DOUT_192_207_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_192_207_SHIFT                            0
#define PMIC_RG_OTP_DOUT_208_223_ADDR                             MT6353_OTP_DOUT_208_223
#define PMIC_RG_OTP_DOUT_208_223_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_208_223_SHIFT                            0
#define PMIC_RG_OTP_DOUT_224_239_ADDR                             MT6353_OTP_DOUT_224_239
#define PMIC_RG_OTP_DOUT_224_239_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_224_239_SHIFT                            0
#define PMIC_RG_OTP_DOUT_240_255_ADDR                             MT6353_OTP_DOUT_240_255
#define PMIC_RG_OTP_DOUT_240_255_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_240_255_SHIFT                            0
#define PMIC_RG_OTP_DOUT_256_271_ADDR                             MT6353_OTP_DOUT_256_271
#define PMIC_RG_OTP_DOUT_256_271_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_256_271_SHIFT                            0
#define PMIC_RG_OTP_DOUT_272_287_ADDR                             MT6353_OTP_DOUT_272_287
#define PMIC_RG_OTP_DOUT_272_287_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_272_287_SHIFT                            0
#define PMIC_RG_OTP_DOUT_288_303_ADDR                             MT6353_OTP_DOUT_288_303
#define PMIC_RG_OTP_DOUT_288_303_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_288_303_SHIFT                            0
#define PMIC_RG_OTP_DOUT_304_319_ADDR                             MT6353_OTP_DOUT_304_319
#define PMIC_RG_OTP_DOUT_304_319_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_304_319_SHIFT                            0
#define PMIC_RG_OTP_DOUT_320_335_ADDR                             MT6353_OTP_DOUT_320_335
#define PMIC_RG_OTP_DOUT_320_335_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_320_335_SHIFT                            0
#define PMIC_RG_OTP_DOUT_336_351_ADDR                             MT6353_OTP_DOUT_336_351
#define PMIC_RG_OTP_DOUT_336_351_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_336_351_SHIFT                            0
#define PMIC_RG_OTP_DOUT_352_367_ADDR                             MT6353_OTP_DOUT_352_367
#define PMIC_RG_OTP_DOUT_352_367_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_352_367_SHIFT                            0
#define PMIC_RG_OTP_DOUT_368_383_ADDR                             MT6353_OTP_DOUT_368_383
#define PMIC_RG_OTP_DOUT_368_383_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_368_383_SHIFT                            0
#define PMIC_RG_OTP_DOUT_384_399_ADDR                             MT6353_OTP_DOUT_384_399
#define PMIC_RG_OTP_DOUT_384_399_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_384_399_SHIFT                            0
#define PMIC_RG_OTP_DOUT_400_415_ADDR                             MT6353_OTP_DOUT_400_415
#define PMIC_RG_OTP_DOUT_400_415_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_400_415_SHIFT                            0
#define PMIC_RG_OTP_DOUT_416_431_ADDR                             MT6353_OTP_DOUT_416_431
#define PMIC_RG_OTP_DOUT_416_431_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_416_431_SHIFT                            0
#define PMIC_RG_OTP_DOUT_432_447_ADDR                             MT6353_OTP_DOUT_432_447
#define PMIC_RG_OTP_DOUT_432_447_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_432_447_SHIFT                            0
#define PMIC_RG_OTP_DOUT_448_463_ADDR                             MT6353_OTP_DOUT_448_463
#define PMIC_RG_OTP_DOUT_448_463_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_448_463_SHIFT                            0
#define PMIC_RG_OTP_DOUT_464_479_ADDR                             MT6353_OTP_DOUT_464_479
#define PMIC_RG_OTP_DOUT_464_479_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_464_479_SHIFT                            0
#define PMIC_RG_OTP_DOUT_480_495_ADDR                             MT6353_OTP_DOUT_480_495
#define PMIC_RG_OTP_DOUT_480_495_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_480_495_SHIFT                            0
#define PMIC_RG_OTP_DOUT_496_511_ADDR                             MT6353_OTP_DOUT_496_511
#define PMIC_RG_OTP_DOUT_496_511_MASK                             0xFFFF
#define PMIC_RG_OTP_DOUT_496_511_SHIFT                            0
#define PMIC_RG_OTP_VAL_0_15_ADDR                                 MT6353_OTP_VAL_0_15
#define PMIC_RG_OTP_VAL_0_15_MASK                                 0xFFFF
#define PMIC_RG_OTP_VAL_0_15_SHIFT                                0
#define PMIC_RG_OTP_VAL_16_31_ADDR                                MT6353_OTP_VAL_16_31
#define PMIC_RG_OTP_VAL_16_31_MASK                                0xFFFF
#define PMIC_RG_OTP_VAL_16_31_SHIFT                               0
#define PMIC_RG_OTP_VAL_32_47_ADDR                                MT6353_OTP_VAL_32_47
#define PMIC_RG_OTP_VAL_32_47_MASK                                0xFFFF
#define PMIC_RG_OTP_VAL_32_47_SHIFT                               0
#define PMIC_RG_OTP_VAL_48_63_ADDR                                MT6353_OTP_VAL_48_63
#define PMIC_RG_OTP_VAL_48_63_MASK                                0xFFFF
#define PMIC_RG_OTP_VAL_48_63_SHIFT                               0
#define PMIC_RG_OTP_VAL_64_79_ADDR                                MT6353_OTP_VAL_64_79
#define PMIC_RG_OTP_VAL_64_79_MASK                                0xFFFF
#define PMIC_RG_OTP_VAL_64_79_SHIFT                               0
#define PMIC_RG_OTP_VAL_80_95_ADDR                                MT6353_OTP_VAL_80_95
#define PMIC_RG_OTP_VAL_80_95_MASK                                0xFFFF
#define PMIC_RG_OTP_VAL_80_95_SHIFT                               0
#define PMIC_RG_OTP_VAL_96_111_ADDR                               MT6353_OTP_VAL_96_111
#define PMIC_RG_OTP_VAL_96_111_MASK                               0xFFFF
#define PMIC_RG_OTP_VAL_96_111_SHIFT                              0
#define PMIC_RG_OTP_VAL_112_127_ADDR                              MT6353_OTP_VAL_112_127
#define PMIC_RG_OTP_VAL_112_127_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_112_127_SHIFT                             0
#define PMIC_RG_OTP_VAL_128_143_ADDR                              MT6353_OTP_VAL_128_143
#define PMIC_RG_OTP_VAL_128_143_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_128_143_SHIFT                             0
#define PMIC_RG_OTP_VAL_144_159_ADDR                              MT6353_OTP_VAL_144_159
#define PMIC_RG_OTP_VAL_144_159_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_144_159_SHIFT                             0
#define PMIC_RG_OTP_VAL_160_175_ADDR                              MT6353_OTP_VAL_160_175
#define PMIC_RG_OTP_VAL_160_175_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_160_175_SHIFT                             0
#define PMIC_RG_OTP_VAL_176_191_ADDR                              MT6353_OTP_VAL_176_191
#define PMIC_RG_OTP_VAL_176_191_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_176_191_SHIFT                             0
#define PMIC_RG_OTP_VAL_192_207_ADDR                              MT6353_OTP_VAL_192_207
#define PMIC_RG_OTP_VAL_192_207_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_192_207_SHIFT                             0
#define PMIC_RG_OTP_VAL_208_223_ADDR                              MT6353_OTP_VAL_208_223
#define PMIC_RG_OTP_VAL_208_223_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_208_223_SHIFT                             0
#define PMIC_RG_OTP_VAL_224_239_ADDR                              MT6353_OTP_VAL_224_239
#define PMIC_RG_OTP_VAL_224_239_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_224_239_SHIFT                             0
#define PMIC_RG_OTP_VAL_240_255_ADDR                              MT6353_OTP_VAL_240_255
#define PMIC_RG_OTP_VAL_240_255_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_240_255_SHIFT                             0
#define PMIC_RG_OTP_VAL_256_271_ADDR                              MT6353_OTP_VAL_256_271
#define PMIC_RG_OTP_VAL_256_271_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_256_271_SHIFT                             0
#define PMIC_RG_OTP_VAL_272_287_ADDR                              MT6353_OTP_VAL_272_287
#define PMIC_RG_OTP_VAL_272_287_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_272_287_SHIFT                             0
#define PMIC_RG_OTP_VAL_288_303_ADDR                              MT6353_OTP_VAL_288_303
#define PMIC_RG_OTP_VAL_288_303_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_288_303_SHIFT                             0
#define PMIC_RG_OTP_VAL_304_319_ADDR                              MT6353_OTP_VAL_304_319
#define PMIC_RG_OTP_VAL_304_319_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_304_319_SHIFT                             0
#define PMIC_RG_OTP_VAL_320_335_ADDR                              MT6353_OTP_VAL_320_335
#define PMIC_RG_OTP_VAL_320_335_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_320_335_SHIFT                             0
#define PMIC_RG_OTP_VAL_336_351_ADDR                              MT6353_OTP_VAL_336_351
#define PMIC_RG_OTP_VAL_336_351_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_336_351_SHIFT                             0
#define PMIC_RG_OTP_VAL_352_367_ADDR                              MT6353_OTP_VAL_352_367
#define PMIC_RG_OTP_VAL_352_367_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_352_367_SHIFT                             0
#define PMIC_RG_OTP_VAL_368_383_ADDR                              MT6353_OTP_VAL_368_383
#define PMIC_RG_OTP_VAL_368_383_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_368_383_SHIFT                             0
#define PMIC_RG_OTP_VAL_384_399_ADDR                              MT6353_OTP_VAL_384_399
#define PMIC_RG_OTP_VAL_384_399_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_384_399_SHIFT                             0
#define PMIC_RG_OTP_VAL_400_415_ADDR                              MT6353_OTP_VAL_400_415
#define PMIC_RG_OTP_VAL_400_415_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_400_415_SHIFT                             0
#define PMIC_RG_OTP_VAL_416_431_ADDR                              MT6353_OTP_VAL_416_431
#define PMIC_RG_OTP_VAL_416_431_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_416_431_SHIFT                             0
#define PMIC_RG_OTP_VAL_432_447_ADDR                              MT6353_OTP_VAL_432_447
#define PMIC_RG_OTP_VAL_432_447_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_432_447_SHIFT                             0
#define PMIC_RG_OTP_VAL_448_463_ADDR                              MT6353_OTP_VAL_448_463
#define PMIC_RG_OTP_VAL_448_463_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_448_463_SHIFT                             0
#define PMIC_RG_OTP_VAL_464_479_ADDR                              MT6353_OTP_VAL_464_479
#define PMIC_RG_OTP_VAL_464_479_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_464_479_SHIFT                             0
#define PMIC_RG_OTP_VAL_480_495_ADDR                              MT6353_OTP_VAL_480_495
#define PMIC_RG_OTP_VAL_480_495_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_480_495_SHIFT                             0
#define PMIC_RG_OTP_VAL_496_511_ADDR                              MT6353_OTP_VAL_496_511
#define PMIC_RG_OTP_VAL_496_511_MASK                              0xFFFF
#define PMIC_RG_OTP_VAL_496_511_SHIFT                             0
#define PMIC_MIX_EOSC32_STP_LPDTB_ADDR                            MT6353_RTC_MIX_CON0
#define PMIC_MIX_EOSC32_STP_LPDTB_MASK                            0x1
#define PMIC_MIX_EOSC32_STP_LPDTB_SHIFT                           1
#define PMIC_MIX_EOSC32_STP_LPDEN_ADDR                            MT6353_RTC_MIX_CON0
#define PMIC_MIX_EOSC32_STP_LPDEN_MASK                            0x1
#define PMIC_MIX_EOSC32_STP_LPDEN_SHIFT                           2
#define PMIC_MIX_XOSC32_STP_PWDB_ADDR                             MT6353_RTC_MIX_CON0
#define PMIC_MIX_XOSC32_STP_PWDB_MASK                             0x1
#define PMIC_MIX_XOSC32_STP_PWDB_SHIFT                            3
#define PMIC_MIX_XOSC32_STP_LPDTB_ADDR                            MT6353_RTC_MIX_CON0
#define PMIC_MIX_XOSC32_STP_LPDTB_MASK                            0x1
#define PMIC_MIX_XOSC32_STP_LPDTB_SHIFT                           4
#define PMIC_MIX_XOSC32_STP_LPDEN_ADDR                            MT6353_RTC_MIX_CON0
#define PMIC_MIX_XOSC32_STP_LPDEN_MASK                            0x1
#define PMIC_MIX_XOSC32_STP_LPDEN_SHIFT                           5
#define PMIC_MIX_XOSC32_STP_LPDRST_ADDR                           MT6353_RTC_MIX_CON0
#define PMIC_MIX_XOSC32_STP_LPDRST_MASK                           0x1
#define PMIC_MIX_XOSC32_STP_LPDRST_SHIFT                          6
#define PMIC_MIX_XOSC32_STP_CALI_ADDR                             MT6353_RTC_MIX_CON0
#define PMIC_MIX_XOSC32_STP_CALI_MASK                             0x1F
#define PMIC_MIX_XOSC32_STP_CALI_SHIFT                            7
#define PMIC_STMP_MODE_ADDR                                       MT6353_RTC_MIX_CON0
#define PMIC_STMP_MODE_MASK                                       0x1
#define PMIC_STMP_MODE_SHIFT                                      12
#define PMIC_MIX_EOSC32_STP_CHOP_EN_ADDR                          MT6353_RTC_MIX_CON1
#define PMIC_MIX_EOSC32_STP_CHOP_EN_MASK                          0x1
#define PMIC_MIX_EOSC32_STP_CHOP_EN_SHIFT                         0
#define PMIC_MIX_DCXO_STP_LVSH_EN_ADDR                            MT6353_RTC_MIX_CON1
#define PMIC_MIX_DCXO_STP_LVSH_EN_MASK                            0x1
#define PMIC_MIX_DCXO_STP_LVSH_EN_SHIFT                           1
#define PMIC_MIX_PMU_STP_DDLO_VRTC_ADDR                           MT6353_RTC_MIX_CON1
#define PMIC_MIX_PMU_STP_DDLO_VRTC_MASK                           0x1
#define PMIC_MIX_PMU_STP_DDLO_VRTC_SHIFT                          2
#define PMIC_MIX_PMU_STP_DDLO_VRTC_EN_ADDR                        MT6353_RTC_MIX_CON1
#define PMIC_MIX_PMU_STP_DDLO_VRTC_EN_MASK                        0x1
#define PMIC_MIX_PMU_STP_DDLO_VRTC_EN_SHIFT                       3
#define PMIC_MIX_RTC_STP_XOSC32_ENB_ADDR                          MT6353_RTC_MIX_CON1
#define PMIC_MIX_RTC_STP_XOSC32_ENB_MASK                          0x1
#define PMIC_MIX_RTC_STP_XOSC32_ENB_SHIFT                         4
#define PMIC_MIX_DCXO_STP_TEST_DEGLITCH_MODE_ADDR                 MT6353_RTC_MIX_CON1
#define PMIC_MIX_DCXO_STP_TEST_DEGLITCH_MODE_MASK                 0x1
#define PMIC_MIX_DCXO_STP_TEST_DEGLITCH_MODE_SHIFT                5
#define PMIC_MIX_EOSC32_STP_RSV_ADDR                              MT6353_RTC_MIX_CON1
#define PMIC_MIX_EOSC32_STP_RSV_MASK                              0x3
#define PMIC_MIX_EOSC32_STP_RSV_SHIFT                             6
#define PMIC_MIX_EOSC32_VCT_EN_ADDR                               MT6353_RTC_MIX_CON1
#define PMIC_MIX_EOSC32_VCT_EN_MASK                               0x1
#define PMIC_MIX_EOSC32_VCT_EN_SHIFT                              8
#define PMIC_MIX_EOSC32_OPT_ADDR                                  MT6353_RTC_MIX_CON1
#define PMIC_MIX_EOSC32_OPT_MASK                                  0x3
#define PMIC_MIX_EOSC32_OPT_SHIFT                                 9
#define PMIC_MIX_DCXO_STP_LVSH_EN_INT_ADDR                        MT6353_RTC_MIX_CON1
#define PMIC_MIX_DCXO_STP_LVSH_EN_INT_MASK                        0x1
#define PMIC_MIX_DCXO_STP_LVSH_EN_INT_SHIFT                       11
#define PMIC_MIX_RTC_GPIO_COREDETB_ADDR                           MT6353_RTC_MIX_CON1
#define PMIC_MIX_RTC_GPIO_COREDETB_MASK                           0x1
#define PMIC_MIX_RTC_GPIO_COREDETB_SHIFT                          12
#define PMIC_MIX_RTC_GPIO_F32KOB_ADDR                             MT6353_RTC_MIX_CON1
#define PMIC_MIX_RTC_GPIO_F32KOB_MASK                             0x1
#define PMIC_MIX_RTC_GPIO_F32KOB_SHIFT                            13
#define PMIC_MIX_RTC_GPIO_GPO_ADDR                                MT6353_RTC_MIX_CON1
#define PMIC_MIX_RTC_GPIO_GPO_MASK                                0x1
#define PMIC_MIX_RTC_GPIO_GPO_SHIFT                               14
#define PMIC_MIX_RTC_GPIO_OE_ADDR                                 MT6353_RTC_MIX_CON1
#define PMIC_MIX_RTC_GPIO_OE_MASK                                 0x1
#define PMIC_MIX_RTC_GPIO_OE_SHIFT                                15
#define PMIC_MIX_RTC_STP_DEBUG_OUT_ADDR                           MT6353_RTC_MIX_CON2
#define PMIC_MIX_RTC_STP_DEBUG_OUT_MASK                           0x3
#define PMIC_MIX_RTC_STP_DEBUG_OUT_SHIFT                          0
#define PMIC_MIX_RTC_STP_DEBUG_SEL_ADDR                           MT6353_RTC_MIX_CON2
#define PMIC_MIX_RTC_STP_DEBUG_SEL_MASK                           0x3
#define PMIC_MIX_RTC_STP_DEBUG_SEL_SHIFT                          4
#define PMIC_MIX_RTC_STP_K_EOSC32_EN_ADDR                         MT6353_RTC_MIX_CON2
#define PMIC_MIX_RTC_STP_K_EOSC32_EN_MASK                         0x1
#define PMIC_MIX_RTC_STP_K_EOSC32_EN_SHIFT                        7
#define PMIC_MIX_RTC_STP_EMBCK_SEL_ADDR                           MT6353_RTC_MIX_CON2
#define PMIC_MIX_RTC_STP_EMBCK_SEL_MASK                           0x1
#define PMIC_MIX_RTC_STP_EMBCK_SEL_SHIFT                          8
#define PMIC_MIX_STP_BBWAKEUP_ADDR                                MT6353_RTC_MIX_CON2
#define PMIC_MIX_STP_BBWAKEUP_MASK                                0x1
#define PMIC_MIX_STP_BBWAKEUP_SHIFT                               9
#define PMIC_MIX_STP_RTC_DDLO_ADDR                                MT6353_RTC_MIX_CON2
#define PMIC_MIX_STP_RTC_DDLO_MASK                                0x1
#define PMIC_MIX_STP_RTC_DDLO_SHIFT                               10
#define PMIC_MIX_RTC_XOSC32_ENB_ADDR                              MT6353_RTC_MIX_CON2
#define PMIC_MIX_RTC_XOSC32_ENB_MASK                              0x1
#define PMIC_MIX_RTC_XOSC32_ENB_SHIFT                             11
#define PMIC_MIX_EFUSE_XOSC32_ENB_OPT_ADDR                        MT6353_RTC_MIX_CON2
#define PMIC_MIX_EFUSE_XOSC32_ENB_OPT_MASK                        0x1
#define PMIC_MIX_EFUSE_XOSC32_ENB_OPT_SHIFT                       12
#define PMIC_FG_ON_ADDR                                           MT6353_FGADC_CON0
#define PMIC_FG_ON_MASK                                           0x1
#define PMIC_FG_ON_SHIFT                                          0
#define PMIC_FG_CAL_ADDR                                          MT6353_FGADC_CON0
#define PMIC_FG_CAL_MASK                                          0x3
#define PMIC_FG_CAL_SHIFT                                         2
#define PMIC_FG_AUTOCALRATE_ADDR                                  MT6353_FGADC_CON0
#define PMIC_FG_AUTOCALRATE_MASK                                  0x7
#define PMIC_FG_AUTOCALRATE_SHIFT                                 4
#define PMIC_FG_SW_CR_ADDR                                        MT6353_FGADC_CON0
#define PMIC_FG_SW_CR_MASK                                        0x1
#define PMIC_FG_SW_CR_SHIFT                                       8
#define PMIC_FG_SW_READ_PRE_ADDR                                  MT6353_FGADC_CON0
#define PMIC_FG_SW_READ_PRE_MASK                                  0x1
#define PMIC_FG_SW_READ_PRE_SHIFT                                 9
#define PMIC_FG_LATCHDATA_ST_ADDR                                 MT6353_FGADC_CON0
#define PMIC_FG_LATCHDATA_ST_MASK                                 0x1
#define PMIC_FG_LATCHDATA_ST_SHIFT                                10
#define PMIC_FG_SW_CLEAR_ADDR                                     MT6353_FGADC_CON0
#define PMIC_FG_SW_CLEAR_MASK                                     0x1
#define PMIC_FG_SW_CLEAR_SHIFT                                    11
#define PMIC_FG_OFFSET_RST_ADDR                                   MT6353_FGADC_CON0
#define PMIC_FG_OFFSET_RST_MASK                                   0x1
#define PMIC_FG_OFFSET_RST_SHIFT                                  12
#define PMIC_FG_TIME_RST_ADDR                                     MT6353_FGADC_CON0
#define PMIC_FG_TIME_RST_MASK                                     0x1
#define PMIC_FG_TIME_RST_SHIFT                                    13
#define PMIC_FG_CHARGE_RST_ADDR                                   MT6353_FGADC_CON0
#define PMIC_FG_CHARGE_RST_MASK                                   0x1
#define PMIC_FG_CHARGE_RST_SHIFT                                  14
#define PMIC_FG_SW_RSTCLR_ADDR                                    MT6353_FGADC_CON0
#define PMIC_FG_SW_RSTCLR_MASK                                    0x1
#define PMIC_FG_SW_RSTCLR_SHIFT                                   15
#define PMIC_FG_CAR_34_19_ADDR                                    MT6353_FGADC_CON1
#define PMIC_FG_CAR_34_19_MASK                                    0xFFFF
#define PMIC_FG_CAR_34_19_SHIFT                                   0
#define PMIC_FG_CAR_18_03_ADDR                                    MT6353_FGADC_CON2
#define PMIC_FG_CAR_18_03_MASK                                    0xFFFF
#define PMIC_FG_CAR_18_03_SHIFT                                   0
#define PMIC_FG_CAR_02_00_ADDR                                    MT6353_FGADC_CON3
#define PMIC_FG_CAR_02_00_MASK                                    0x7
#define PMIC_FG_CAR_02_00_SHIFT                                   0
#define PMIC_FG_NTER_32_17_ADDR                                   MT6353_FGADC_CON4
#define PMIC_FG_NTER_32_17_MASK                                   0xFFFF
#define PMIC_FG_NTER_32_17_SHIFT                                  0
#define PMIC_FG_NTER_16_01_ADDR                                   MT6353_FGADC_CON5
#define PMIC_FG_NTER_16_01_MASK                                   0xFFFF
#define PMIC_FG_NTER_16_01_SHIFT                                  0
#define PMIC_FG_NTER_00_ADDR                                      MT6353_FGADC_CON6
#define PMIC_FG_NTER_00_MASK                                      0x1
#define PMIC_FG_NTER_00_SHIFT                                     0
#define PMIC_FG_BLTR_31_16_ADDR                                   MT6353_FGADC_CON7
#define PMIC_FG_BLTR_31_16_MASK                                   0xFFFF
#define PMIC_FG_BLTR_31_16_SHIFT                                  0
#define PMIC_FG_BLTR_15_00_ADDR                                   MT6353_FGADC_CON8
#define PMIC_FG_BLTR_15_00_MASK                                   0xFFFF
#define PMIC_FG_BLTR_15_00_SHIFT                                  0
#define PMIC_FG_BFTR_31_16_ADDR                                   MT6353_FGADC_CON9
#define PMIC_FG_BFTR_31_16_MASK                                   0xFFFF
#define PMIC_FG_BFTR_31_16_SHIFT                                  0
#define PMIC_FG_BFTR_15_00_ADDR                                   MT6353_FGADC_CON10
#define PMIC_FG_BFTR_15_00_MASK                                   0xFFFF
#define PMIC_FG_BFTR_15_00_SHIFT                                  0
#define PMIC_FG_CURRENT_OUT_ADDR                                  MT6353_FGADC_CON11
#define PMIC_FG_CURRENT_OUT_MASK                                  0xFFFF
#define PMIC_FG_CURRENT_OUT_SHIFT                                 0
#define PMIC_FG_ADJUST_OFFSET_VALUE_ADDR                          MT6353_FGADC_CON12
#define PMIC_FG_ADJUST_OFFSET_VALUE_MASK                          0xFFFF
#define PMIC_FG_ADJUST_OFFSET_VALUE_SHIFT                         0
#define PMIC_FG_OFFSET_ADDR                                       MT6353_FGADC_CON13
#define PMIC_FG_OFFSET_MASK                                       0xFFFF
#define PMIC_FG_OFFSET_SHIFT                                      0
#define PMIC_RG_FGANALOGTEST_ADDR                                 MT6353_FGADC_CON14
#define PMIC_RG_FGANALOGTEST_MASK                                 0xF
#define PMIC_RG_FGANALOGTEST_SHIFT                                0
#define PMIC_RG_FGINTMODE_ADDR                                    MT6353_FGADC_CON14
#define PMIC_RG_FGINTMODE_MASK                                    0x1
#define PMIC_RG_FGINTMODE_SHIFT                                   4
#define PMIC_RG_SPARE_ADDR                                        MT6353_FGADC_CON14
#define PMIC_RG_SPARE_MASK                                        0xFF
#define PMIC_RG_SPARE_SHIFT                                       5
#define PMIC_FG_OSR_ADDR                                          MT6353_FGADC_CON15
#define PMIC_FG_OSR_MASK                                          0xF
#define PMIC_FG_OSR_SHIFT                                         0
#define PMIC_FG_ADJ_OFFSET_EN_ADDR                                MT6353_FGADC_CON15
#define PMIC_FG_ADJ_OFFSET_EN_MASK                                0x1
#define PMIC_FG_ADJ_OFFSET_EN_SHIFT                               8
#define PMIC_FG_ADC_AUTORST_ADDR                                  MT6353_FGADC_CON15
#define PMIC_FG_ADC_AUTORST_MASK                                  0x1
#define PMIC_FG_ADC_AUTORST_SHIFT                                 9
#define PMIC_FG_FIR1BYPASS_ADDR                                   MT6353_FGADC_CON16
#define PMIC_FG_FIR1BYPASS_MASK                                   0x1
#define PMIC_FG_FIR1BYPASS_SHIFT                                  0
#define PMIC_FG_FIR2BYPASS_ADDR                                   MT6353_FGADC_CON16
#define PMIC_FG_FIR2BYPASS_MASK                                   0x1
#define PMIC_FG_FIR2BYPASS_SHIFT                                  1
#define PMIC_FG_L_CUR_INT_STS_ADDR                                MT6353_FGADC_CON16
#define PMIC_FG_L_CUR_INT_STS_MASK                                0x1
#define PMIC_FG_L_CUR_INT_STS_SHIFT                               2
#define PMIC_FG_H_CUR_INT_STS_ADDR                                MT6353_FGADC_CON16
#define PMIC_FG_H_CUR_INT_STS_MASK                                0x1
#define PMIC_FG_H_CUR_INT_STS_SHIFT                               3
#define PMIC_FG_L_INT_STS_ADDR                                    MT6353_FGADC_CON16
#define PMIC_FG_L_INT_STS_MASK                                    0x1
#define PMIC_FG_L_INT_STS_SHIFT                                   4
#define PMIC_FG_H_INT_STS_ADDR                                    MT6353_FGADC_CON16
#define PMIC_FG_H_INT_STS_MASK                                    0x1
#define PMIC_FG_H_INT_STS_SHIFT                                   5
#define PMIC_FG_ADC_RSTDETECT_ADDR                                MT6353_FGADC_CON16
#define PMIC_FG_ADC_RSTDETECT_MASK                                0x1
#define PMIC_FG_ADC_RSTDETECT_SHIFT                               7
#define PMIC_FG_SLP_EN_ADDR                                       MT6353_FGADC_CON16
#define PMIC_FG_SLP_EN_MASK                                       0x1
#define PMIC_FG_SLP_EN_SHIFT                                      8
#define PMIC_FG_ZCV_DET_EN_ADDR                                   MT6353_FGADC_CON16
#define PMIC_FG_ZCV_DET_EN_MASK                                   0x1
#define PMIC_FG_ZCV_DET_EN_SHIFT                                  9
#define PMIC_RG_FG_AUXADC_R_ADDR                                  MT6353_FGADC_CON16
#define PMIC_RG_FG_AUXADC_R_MASK                                  0x1
#define PMIC_RG_FG_AUXADC_R_SHIFT                                 10
#define PMIC_DA_FGADC_EN_ADDR                                     MT6353_FGADC_CON16
#define PMIC_DA_FGADC_EN_MASK                                     0x1
#define PMIC_DA_FGADC_EN_SHIFT                                    12
#define PMIC_DA_FGCAL_EN_ADDR                                     MT6353_FGADC_CON16
#define PMIC_DA_FGCAL_EN_MASK                                     0x1
#define PMIC_DA_FGCAL_EN_SHIFT                                    13
#define PMIC_DA_FG_RST_ADDR                                       MT6353_FGADC_CON16
#define PMIC_DA_FG_RST_MASK                                       0x1
#define PMIC_DA_FG_RST_SHIFT                                      14
#define PMIC_FG_CIC2_ADDR                                         MT6353_FGADC_CON17
#define PMIC_FG_CIC2_MASK                                         0xFFFF
#define PMIC_FG_CIC2_SHIFT                                        0
#define PMIC_FG_SLP_CUR_TH_ADDR                                   MT6353_FGADC_CON18
#define PMIC_FG_SLP_CUR_TH_MASK                                   0xFFFF
#define PMIC_FG_SLP_CUR_TH_SHIFT                                  0
#define PMIC_FG_SLP_TIME_ADDR                                     MT6353_FGADC_CON19
#define PMIC_FG_SLP_TIME_MASK                                     0xFF
#define PMIC_FG_SLP_TIME_SHIFT                                    0
#define PMIC_FG_SRCVOLTEN_FTIME_ADDR                              MT6353_FGADC_CON20
#define PMIC_FG_SRCVOLTEN_FTIME_MASK                              0xFF
#define PMIC_FG_SRCVOLTEN_FTIME_SHIFT                             0
#define PMIC_FG_DET_TIME_ADDR                                     MT6353_FGADC_CON20
#define PMIC_FG_DET_TIME_MASK                                     0xFF
#define PMIC_FG_DET_TIME_SHIFT                                    8
#define PMIC_FG_ZCV_CAR_34_19_ADDR                                MT6353_FGADC_CON21
#define PMIC_FG_ZCV_CAR_34_19_MASK                                0xFFFF
#define PMIC_FG_ZCV_CAR_34_19_SHIFT                               0
#define PMIC_FG_ZCV_CAR_18_03_ADDR                                MT6353_FGADC_CON22
#define PMIC_FG_ZCV_CAR_18_03_MASK                                0xFFFF
#define PMIC_FG_ZCV_CAR_18_03_SHIFT                               0
#define PMIC_FG_ZCV_CAR_02_00_ADDR                                MT6353_FGADC_CON23
#define PMIC_FG_ZCV_CAR_02_00_MASK                                0x7
#define PMIC_FG_ZCV_CAR_02_00_SHIFT                               0
#define PMIC_FG_ZCV_CURR_ADDR                                     MT6353_FGADC_CON24
#define PMIC_FG_ZCV_CURR_MASK                                     0xFFFF
#define PMIC_FG_ZCV_CURR_SHIFT                                    0
#define PMIC_FG_R_CURR_ADDR                                       MT6353_FGADC_CON25
#define PMIC_FG_R_CURR_MASK                                       0xFFFF
#define PMIC_FG_R_CURR_SHIFT                                      0
#define PMIC_FG_MODE_ADDR                                         MT6353_FGADC_CON26
#define PMIC_FG_MODE_MASK                                         0x1
#define PMIC_FG_MODE_SHIFT                                        0
#define PMIC_FG_RST_SW_ADDR                                       MT6353_FGADC_CON26
#define PMIC_FG_RST_SW_MASK                                       0x1
#define PMIC_FG_RST_SW_SHIFT                                      1
#define PMIC_FG_FGCAL_EN_SW_ADDR                                  MT6353_FGADC_CON26
#define PMIC_FG_FGCAL_EN_SW_MASK                                  0x1
#define PMIC_FG_FGCAL_EN_SW_SHIFT                                 2
#define PMIC_FG_FGADC_EN_SW_ADDR                                  MT6353_FGADC_CON26
#define PMIC_FG_FGADC_EN_SW_MASK                                  0x1
#define PMIC_FG_FGADC_EN_SW_SHIFT                                 3
#define PMIC_FG_RSV1_ADDR                                         MT6353_FGADC_CON26
#define PMIC_FG_RSV1_MASK                                         0xF
#define PMIC_FG_RSV1_SHIFT                                        4
#define PMIC_FG_TEST_MODE0_ADDR                                   MT6353_FGADC_CON26
#define PMIC_FG_TEST_MODE0_MASK                                   0x1
#define PMIC_FG_TEST_MODE0_SHIFT                                  14
#define PMIC_FG_TEST_MODE1_ADDR                                   MT6353_FGADC_CON26
#define PMIC_FG_TEST_MODE1_MASK                                   0x1
#define PMIC_FG_TEST_MODE1_SHIFT                                  15
#define PMIC_FG_GAIN_ADDR                                         MT6353_FGADC_CON27
#define PMIC_FG_GAIN_MASK                                         0x1FFF
#define PMIC_FG_GAIN_SHIFT                                        0
#define PMIC_FG_CUR_HTH_ADDR                                      MT6353_FGADC_CON28
#define PMIC_FG_CUR_HTH_MASK                                      0xFFFF
#define PMIC_FG_CUR_HTH_SHIFT                                     0
#define PMIC_FG_CUR_LTH_ADDR                                      MT6353_FGADC_CON29
#define PMIC_FG_CUR_LTH_MASK                                      0xFFFF
#define PMIC_FG_CUR_LTH_SHIFT                                     0
#define PMIC_FG_ZCV_DET_TIME_ADDR                                 MT6353_FGADC_CON30
#define PMIC_FG_ZCV_DET_TIME_MASK                                 0x3F
#define PMIC_FG_ZCV_DET_TIME_SHIFT                                0
#define PMIC_FG_ZCV_CAR_TH_33_19_ADDR                             MT6353_FGADC_CON31
#define PMIC_FG_ZCV_CAR_TH_33_19_MASK                             0x7FFF
#define PMIC_FG_ZCV_CAR_TH_33_19_SHIFT                            0
#define PMIC_FG_ZCV_CAR_TH_18_03_ADDR                             MT6353_FGADC_CON32
#define PMIC_FG_ZCV_CAR_TH_18_03_MASK                             0xFFFF
#define PMIC_FG_ZCV_CAR_TH_18_03_SHIFT                            0
#define PMIC_FG_ZCV_CAR_TH_02_00_ADDR                             MT6353_FGADC_CON33
#define PMIC_FG_ZCV_CAR_TH_02_00_MASK                             0x7
#define PMIC_FG_ZCV_CAR_TH_02_00_SHIFT                            0
#define PMIC_SYSTEM_INFO_CON0_ADDR                                MT6353_SYSTEM_INFO_CON0
#define PMIC_SYSTEM_INFO_CON0_MASK                                0xFFFF
#define PMIC_SYSTEM_INFO_CON0_SHIFT                               0
#define PMIC_SYSTEM_INFO_CON1_ADDR                                MT6353_SYSTEM_INFO_CON1
#define PMIC_SYSTEM_INFO_CON1_MASK                                0xFFFF
#define PMIC_SYSTEM_INFO_CON1_SHIFT                               0
#define PMIC_SYSTEM_INFO_CON2_ADDR                                MT6353_SYSTEM_INFO_CON2
#define PMIC_SYSTEM_INFO_CON2_MASK                                0xFFFF
#define PMIC_SYSTEM_INFO_CON2_SHIFT                               0
#define PMIC_SYSTEM_INFO_CON3_ADDR                                MT6353_SYSTEM_INFO_CON3
#define PMIC_SYSTEM_INFO_CON3_MASK                                0xFFFF
#define PMIC_SYSTEM_INFO_CON3_SHIFT                               0
#define PMIC_SYSTEM_INFO_CON4_ADDR                                MT6353_SYSTEM_INFO_CON4
#define PMIC_SYSTEM_INFO_CON4_MASK                                0xFFFF
#define PMIC_SYSTEM_INFO_CON4_SHIFT                               0
#define PMIC_RG_AUDDACLPWRUP_VAUDP15_ADDR                         MT6353_AUDDEC_ANA_CON0
#define PMIC_RG_AUDDACLPWRUP_VAUDP15_MASK                         0x1
#define PMIC_RG_AUDDACLPWRUP_VAUDP15_SHIFT                        0
#define PMIC_RG_AUDDACRPWRUP_VAUDP15_ADDR                         MT6353_AUDDEC_ANA_CON0
#define PMIC_RG_AUDDACRPWRUP_VAUDP15_MASK                         0x1
#define PMIC_RG_AUDDACRPWRUP_VAUDP15_SHIFT                        1
#define PMIC_RG_AUD_DAC_PWR_UP_VA28_ADDR                          MT6353_AUDDEC_ANA_CON0
#define PMIC_RG_AUD_DAC_PWR_UP_VA28_MASK                          0x1
#define PMIC_RG_AUD_DAC_PWR_UP_VA28_SHIFT                         2
#define PMIC_RG_AUD_DAC_PWL_UP_VA28_ADDR                          MT6353_AUDDEC_ANA_CON0
#define PMIC_RG_AUD_DAC_PWL_UP_VA28_MASK                          0x1
#define PMIC_RG_AUD_DAC_PWL_UP_VA28_SHIFT                         3
#define PMIC_RG_AUDHSPWRUP_VAUDP15_ADDR                           MT6353_AUDDEC_ANA_CON0
#define PMIC_RG_AUDHSPWRUP_VAUDP15_MASK                           0x1
#define PMIC_RG_AUDHSPWRUP_VAUDP15_SHIFT                          4
#define PMIC_RG_AUDHPLPWRUP_VAUDP15_ADDR                          MT6353_AUDDEC_ANA_CON0
#define PMIC_RG_AUDHPLPWRUP_VAUDP15_MASK                          0x1
#define PMIC_RG_AUDHPLPWRUP_VAUDP15_SHIFT                         5
#define PMIC_RG_AUDHPRPWRUP_VAUDP15_ADDR                          MT6353_AUDDEC_ANA_CON0
#define PMIC_RG_AUDHPRPWRUP_VAUDP15_MASK                          0x1
#define PMIC_RG_AUDHPRPWRUP_VAUDP15_SHIFT                         6
#define PMIC_RG_AUDHSMUXINPUTSEL_VAUDP15_ADDR                     MT6353_AUDDEC_ANA_CON0
#define PMIC_RG_AUDHSMUXINPUTSEL_VAUDP15_MASK                     0x3
#define PMIC_RG_AUDHSMUXINPUTSEL_VAUDP15_SHIFT                    7
#define PMIC_RG_AUDHPLMUXINPUTSEL_VAUDP15_ADDR                    MT6353_AUDDEC_ANA_CON0
#define PMIC_RG_AUDHPLMUXINPUTSEL_VAUDP15_MASK                    0x3
#define PMIC_RG_AUDHPLMUXINPUTSEL_VAUDP15_SHIFT                   9
#define PMIC_RG_AUDHPRMUXINPUTSEL_VAUDP15_ADDR                    MT6353_AUDDEC_ANA_CON0
#define PMIC_RG_AUDHPRMUXINPUTSEL_VAUDP15_MASK                    0x3
#define PMIC_RG_AUDHPRMUXINPUTSEL_VAUDP15_SHIFT                   11
#define PMIC_RG_AUDHSSCDISABLE_VAUDP15_ADDR                       MT6353_AUDDEC_ANA_CON0
#define PMIC_RG_AUDHSSCDISABLE_VAUDP15_MASK                       0x1
#define PMIC_RG_AUDHSSCDISABLE_VAUDP15_SHIFT                      13
#define PMIC_RG_AUDHPLSCDISABLE_VAUDP15_ADDR                      MT6353_AUDDEC_ANA_CON0
#define PMIC_RG_AUDHPLSCDISABLE_VAUDP15_MASK                      0x1
#define PMIC_RG_AUDHPLSCDISABLE_VAUDP15_SHIFT                     14
#define PMIC_RG_AUDHPRSCDISABLE_VAUDP15_ADDR                      MT6353_AUDDEC_ANA_CON0
#define PMIC_RG_AUDHPRSCDISABLE_VAUDP15_MASK                      0x1
#define PMIC_RG_AUDHPRSCDISABLE_VAUDP15_SHIFT                     15
#define PMIC_RG_AUDHPLBSCCURRENT_VAUDP15_ADDR                     MT6353_AUDDEC_ANA_CON1
#define PMIC_RG_AUDHPLBSCCURRENT_VAUDP15_MASK                     0x1
#define PMIC_RG_AUDHPLBSCCURRENT_VAUDP15_SHIFT                    0
#define PMIC_RG_AUDHPRBSCCURRENT_VAUDP15_ADDR                     MT6353_AUDDEC_ANA_CON1
#define PMIC_RG_AUDHPRBSCCURRENT_VAUDP15_MASK                     0x1
#define PMIC_RG_AUDHPRBSCCURRENT_VAUDP15_SHIFT                    1
#define PMIC_RG_AUDHSBSCCURRENT_VAUDP15_ADDR                      MT6353_AUDDEC_ANA_CON1
#define PMIC_RG_AUDHSBSCCURRENT_VAUDP15_MASK                      0x1
#define PMIC_RG_AUDHSBSCCURRENT_VAUDP15_SHIFT                     2
#define PMIC_RG_AUDHPSTARTUP_VAUDP15_ADDR                         MT6353_AUDDEC_ANA_CON1
#define PMIC_RG_AUDHPSTARTUP_VAUDP15_MASK                         0x1
#define PMIC_RG_AUDHPSTARTUP_VAUDP15_SHIFT                        3
#define PMIC_RG_AUDHSSTARTUP_VAUDP15_ADDR                         MT6353_AUDDEC_ANA_CON1
#define PMIC_RG_AUDHSSTARTUP_VAUDP15_MASK                         0x1
#define PMIC_RG_AUDHSSTARTUP_VAUDP15_SHIFT                        4
#define PMIC_RG_PRECHARGEBUF_EN_VAUDP15_ADDR                      MT6353_AUDDEC_ANA_CON1
#define PMIC_RG_PRECHARGEBUF_EN_VAUDP15_MASK                      0x1
#define PMIC_RG_PRECHARGEBUF_EN_VAUDP15_SHIFT                     5
#define PMIC_RG_HPINPUTSTBENH_VAUDP15_ADDR                        MT6353_AUDDEC_ANA_CON1
#define PMIC_RG_HPINPUTSTBENH_VAUDP15_MASK                        0x1
#define PMIC_RG_HPINPUTSTBENH_VAUDP15_SHIFT                       6
#define PMIC_RG_HPOUTPUTSTBENH_VAUDP15_ADDR                       MT6353_AUDDEC_ANA_CON1
#define PMIC_RG_HPOUTPUTSTBENH_VAUDP15_MASK                       0x1
#define PMIC_RG_HPOUTPUTSTBENH_VAUDP15_SHIFT                      7
#define PMIC_RG_HPINPUTRESET0_VAUDP15_ADDR                        MT6353_AUDDEC_ANA_CON1
#define PMIC_RG_HPINPUTRESET0_VAUDP15_MASK                        0x1
#define PMIC_RG_HPINPUTRESET0_VAUDP15_SHIFT                       8
#define PMIC_RG_HPOUTPUTRESET0_VAUDP15_ADDR                       MT6353_AUDDEC_ANA_CON1
#define PMIC_RG_HPOUTPUTRESET0_VAUDP15_MASK                       0x1
#define PMIC_RG_HPOUTPUTRESET0_VAUDP15_SHIFT                      9
#define PMIC_RG_HPOUT_SHORTVCM_VAUDP15_ADDR                       MT6353_AUDDEC_ANA_CON1
#define PMIC_RG_HPOUT_SHORTVCM_VAUDP15_MASK                       0x1
#define PMIC_RG_HPOUT_SHORTVCM_VAUDP15_SHIFT                      10
#define PMIC_RG_HSINPUTSTBENH_VAUDP15_ADDR                        MT6353_AUDDEC_ANA_CON1
#define PMIC_RG_HSINPUTSTBENH_VAUDP15_MASK                        0x1
#define PMIC_RG_HSINPUTSTBENH_VAUDP15_SHIFT                       11
#define PMIC_RG_HSOUTPUTSTBENH_VAUDP15_ADDR                       MT6353_AUDDEC_ANA_CON1
#define PMIC_RG_HSOUTPUTSTBENH_VAUDP15_MASK                       0x1
#define PMIC_RG_HSOUTPUTSTBENH_VAUDP15_SHIFT                      12
#define PMIC_RG_HSINPUTRESET0_VAUDP15_ADDR                        MT6353_AUDDEC_ANA_CON1
#define PMIC_RG_HSINPUTRESET0_VAUDP15_MASK                        0x1
#define PMIC_RG_HSINPUTRESET0_VAUDP15_SHIFT                       13
#define PMIC_RG_HSOUTPUTRESET0_VAUDP15_ADDR                       MT6353_AUDDEC_ANA_CON1
#define PMIC_RG_HSOUTPUTRESET0_VAUDP15_MASK                       0x1
#define PMIC_RG_HSOUTPUTRESET0_VAUDP15_SHIFT                      14
#define PMIC_RG_HPOUTSTB_RSEL_VAUDP15_ADDR                        MT6353_AUDDEC_ANA_CON2
#define PMIC_RG_HPOUTSTB_RSEL_VAUDP15_MASK                        0x3
#define PMIC_RG_HPOUTSTB_RSEL_VAUDP15_SHIFT                       0
#define PMIC_RG_HSOUT_SHORTVCM_VAUDP15_ADDR                       MT6353_AUDDEC_ANA_CON2
#define PMIC_RG_HSOUT_SHORTVCM_VAUDP15_MASK                       0x1
#define PMIC_RG_HSOUT_SHORTVCM_VAUDP15_SHIFT                      2
#define PMIC_RG_AUDHPLTRIM_VAUDP15_ADDR                           MT6353_AUDDEC_ANA_CON2
#define PMIC_RG_AUDHPLTRIM_VAUDP15_MASK                           0xF
#define PMIC_RG_AUDHPLTRIM_VAUDP15_SHIFT                          3
#define PMIC_RG_AUDHPRTRIM_VAUDP15_ADDR                           MT6353_AUDDEC_ANA_CON2
#define PMIC_RG_AUDHPRTRIM_VAUDP15_MASK                           0xF
#define PMIC_RG_AUDHPRTRIM_VAUDP15_SHIFT                          7
#define PMIC_RG_AUDHPTRIM_EN_VAUDP15_ADDR                         MT6353_AUDDEC_ANA_CON2
#define PMIC_RG_AUDHPTRIM_EN_VAUDP15_MASK                         0x1
#define PMIC_RG_AUDHPTRIM_EN_VAUDP15_SHIFT                        11
#define PMIC_RG_AUDHPLFINETRIM_VAUDP15_ADDR                       MT6353_AUDDEC_ANA_CON2
#define PMIC_RG_AUDHPLFINETRIM_VAUDP15_MASK                       0x3
#define PMIC_RG_AUDHPLFINETRIM_VAUDP15_SHIFT                      12
#define PMIC_RG_AUDHPRFINETRIM_VAUDP15_ADDR                       MT6353_AUDDEC_ANA_CON2
#define PMIC_RG_AUDHPRFINETRIM_VAUDP15_MASK                       0x3
#define PMIC_RG_AUDHPRFINETRIM_VAUDP15_SHIFT                      14
#define PMIC_RG_AUDTRIMBUF_EN_VAUDP15_ADDR                        MT6353_AUDDEC_ANA_CON3
#define PMIC_RG_AUDTRIMBUF_EN_VAUDP15_MASK                        0x1
#define PMIC_RG_AUDTRIMBUF_EN_VAUDP15_SHIFT                       0
#define PMIC_RG_AUDTRIMBUF_INPUTMUXSEL_VAUDP15_ADDR               MT6353_AUDDEC_ANA_CON3
#define PMIC_RG_AUDTRIMBUF_INPUTMUXSEL_VAUDP15_MASK               0xF
#define PMIC_RG_AUDTRIMBUF_INPUTMUXSEL_VAUDP15_SHIFT              1
#define PMIC_RG_AUDTRIMBUF_GAINSEL_VAUDP15_ADDR                   MT6353_AUDDEC_ANA_CON3
#define PMIC_RG_AUDTRIMBUF_GAINSEL_VAUDP15_MASK                   0x3
#define PMIC_RG_AUDTRIMBUF_GAINSEL_VAUDP15_SHIFT                  5
#define PMIC_RG_AUDHPSPKDET_EN_VAUDP15_ADDR                       MT6353_AUDDEC_ANA_CON3
#define PMIC_RG_AUDHPSPKDET_EN_VAUDP15_MASK                       0x1
#define PMIC_RG_AUDHPSPKDET_EN_VAUDP15_SHIFT                      7
#define PMIC_RG_AUDHPSPKDET_INPUTMUXSEL_VAUDP15_ADDR              MT6353_AUDDEC_ANA_CON3
#define PMIC_RG_AUDHPSPKDET_INPUTMUXSEL_VAUDP15_MASK              0x3
#define PMIC_RG_AUDHPSPKDET_INPUTMUXSEL_VAUDP15_SHIFT             8
#define PMIC_RG_AUDHPSPKDET_OUTPUTMUXSEL_VAUDP15_ADDR             MT6353_AUDDEC_ANA_CON3
#define PMIC_RG_AUDHPSPKDET_OUTPUTMUXSEL_VAUDP15_MASK             0x3
#define PMIC_RG_AUDHPSPKDET_OUTPUTMUXSEL_VAUDP15_SHIFT            10
#define PMIC_RG_ABIDEC_RESERVED_VA28_ADDR                         MT6353_AUDDEC_ANA_CON4
#define PMIC_RG_ABIDEC_RESERVED_VA28_MASK                         0xFF
#define PMIC_RG_ABIDEC_RESERVED_VA28_SHIFT                        0
#define PMIC_RG_ABIDEC_RESERVED_VAUDP15_ADDR                      MT6353_AUDDEC_ANA_CON4
#define PMIC_RG_ABIDEC_RESERVED_VAUDP15_MASK                      0xFF
#define PMIC_RG_ABIDEC_RESERVED_VAUDP15_SHIFT                     8
#define PMIC_RG_AUDBIASADJ_0_VAUDP15_ADDR                         MT6353_AUDDEC_ANA_CON5
#define PMIC_RG_AUDBIASADJ_0_VAUDP15_MASK                         0x3F
#define PMIC_RG_AUDBIASADJ_0_VAUDP15_SHIFT                        4
#define PMIC_RG_AUDBIASADJ_1_VAUDP15_ADDR                         MT6353_AUDDEC_ANA_CON5
#define PMIC_RG_AUDBIASADJ_1_VAUDP15_MASK                         0x3F
#define PMIC_RG_AUDBIASADJ_1_VAUDP15_SHIFT                        10
#define PMIC_RG_AUDIBIASPWRDN_VAUDP15_ADDR                        MT6353_AUDDEC_ANA_CON6
#define PMIC_RG_AUDIBIASPWRDN_VAUDP15_MASK                        0x1
#define PMIC_RG_AUDIBIASPWRDN_VAUDP15_SHIFT                       0
#define PMIC_RG_RSTB_DECODER_VA28_ADDR                            MT6353_AUDDEC_ANA_CON6
#define PMIC_RG_RSTB_DECODER_VA28_MASK                            0x1
#define PMIC_RG_RSTB_DECODER_VA28_SHIFT                           1
#define PMIC_RG_RSTB_ENCODER_VA28_ADDR                            MT6353_AUDDEC_ANA_CON6
#define PMIC_RG_RSTB_ENCODER_VA28_MASK                            0x1
#define PMIC_RG_RSTB_ENCODER_VA28_SHIFT                           2
#define PMIC_RG_SEL_DECODER_96K_VA28_ADDR                         MT6353_AUDDEC_ANA_CON6
#define PMIC_RG_SEL_DECODER_96K_VA28_MASK                         0x1
#define PMIC_RG_SEL_DECODER_96K_VA28_SHIFT                        3
#define PMIC_RG_SEL_ENCODER_96K_VA28_ADDR                         MT6353_AUDDEC_ANA_CON6
#define PMIC_RG_SEL_ENCODER_96K_VA28_MASK                         0x1
#define PMIC_RG_SEL_ENCODER_96K_VA28_SHIFT                        4
#define PMIC_RG_SEL_DELAY_VCORE_ADDR                              MT6353_AUDDEC_ANA_CON6
#define PMIC_RG_SEL_DELAY_VCORE_MASK                              0x1
#define PMIC_RG_SEL_DELAY_VCORE_SHIFT                             5
#define PMIC_RG_HCLDO_EN_VA18_ADDR                                MT6353_AUDDEC_ANA_CON6
#define PMIC_RG_HCLDO_EN_VA18_MASK                                0x1
#define PMIC_RG_HCLDO_EN_VA18_SHIFT                               6
#define PMIC_RG_LCLDO_EN_VA18_ADDR                                MT6353_AUDDEC_ANA_CON6
#define PMIC_RG_LCLDO_EN_VA18_MASK                                0x1
#define PMIC_RG_LCLDO_EN_VA18_SHIFT                               7
#define PMIC_RG_LCLDO_ENC_EN_VA28_ADDR                            MT6353_AUDDEC_ANA_CON6
#define PMIC_RG_LCLDO_ENC_EN_VA28_MASK                            0x1
#define PMIC_RG_LCLDO_ENC_EN_VA28_SHIFT                           8
#define PMIC_RG_VA33REFGEN_EN_VA18_ADDR                           MT6353_AUDDEC_ANA_CON6
#define PMIC_RG_VA33REFGEN_EN_VA18_MASK                           0x1
#define PMIC_RG_VA33REFGEN_EN_VA18_SHIFT                          9
#define PMIC_RG_HCLDO_PDDIS_EN_VA18_ADDR                          MT6353_AUDDEC_ANA_CON6
#define PMIC_RG_HCLDO_PDDIS_EN_VA18_MASK                          0x1
#define PMIC_RG_HCLDO_PDDIS_EN_VA18_SHIFT                         10
#define PMIC_RG_HCLDO_REMOTE_SENSE_VA18_ADDR                      MT6353_AUDDEC_ANA_CON6
#define PMIC_RG_HCLDO_REMOTE_SENSE_VA18_MASK                      0x1
#define PMIC_RG_HCLDO_REMOTE_SENSE_VA18_SHIFT                     11
#define PMIC_RG_LCLDO_PDDIS_EN_VA18_ADDR                          MT6353_AUDDEC_ANA_CON6
#define PMIC_RG_LCLDO_PDDIS_EN_VA18_MASK                          0x1
#define PMIC_RG_LCLDO_PDDIS_EN_VA18_SHIFT                         12
#define PMIC_RG_LCLDO_REMOTE_SENSE_VA18_ADDR                      MT6353_AUDDEC_ANA_CON6
#define PMIC_RG_LCLDO_REMOTE_SENSE_VA18_MASK                      0x1
#define PMIC_RG_LCLDO_REMOTE_SENSE_VA18_SHIFT                     13
#define PMIC_RG_LCLDO_VOSEL_VA18_ADDR                             MT6353_AUDDEC_ANA_CON6
#define PMIC_RG_LCLDO_VOSEL_VA18_MASK                             0x1
#define PMIC_RG_LCLDO_VOSEL_VA18_SHIFT                            14
#define PMIC_RG_HCLDO_VOSEL_VA18_ADDR                             MT6353_AUDDEC_ANA_CON6
#define PMIC_RG_HCLDO_VOSEL_VA18_MASK                             0x1
#define PMIC_RG_HCLDO_VOSEL_VA18_SHIFT                            15
#define PMIC_RG_LCLDO_ENC_PDDIS_EN_VA28_ADDR                      MT6353_AUDDEC_ANA_CON7
#define PMIC_RG_LCLDO_ENC_PDDIS_EN_VA28_MASK                      0x1
#define PMIC_RG_LCLDO_ENC_PDDIS_EN_VA28_SHIFT                     0
#define PMIC_RG_LCLDO_ENC_REMOTE_SENSE_VA28_ADDR                  MT6353_AUDDEC_ANA_CON7
#define PMIC_RG_LCLDO_ENC_REMOTE_SENSE_VA28_MASK                  0x1
#define PMIC_RG_LCLDO_ENC_REMOTE_SENSE_VA28_SHIFT                 1
#define PMIC_RG_VA28REFGEN_EN_VA28_ADDR                           MT6353_AUDDEC_ANA_CON7
#define PMIC_RG_VA28REFGEN_EN_VA28_MASK                           0x1
#define PMIC_RG_VA28REFGEN_EN_VA28_SHIFT                          2
#define PMIC_RG_AUDPMU_RESERVED_VA28_ADDR                         MT6353_AUDDEC_ANA_CON7
#define PMIC_RG_AUDPMU_RESERVED_VA28_MASK                         0xF
#define PMIC_RG_AUDPMU_RESERVED_VA28_SHIFT                        3
#define PMIC_RG_AUDPMU_RESERVED_VA18_ADDR                         MT6353_AUDDEC_ANA_CON7
#define PMIC_RG_AUDPMU_RESERVED_VA18_MASK                         0xF
#define PMIC_RG_AUDPMU_RESERVED_VA18_SHIFT                        7
#define PMIC_RG_AUDPMU_RESERVED_VAUDP15_ADDR                      MT6353_AUDDEC_ANA_CON7
#define PMIC_RG_AUDPMU_RESERVED_VAUDP15_MASK                      0xF
#define PMIC_RG_AUDPMU_RESERVED_VAUDP15_SHIFT                     11
#define PMIC_RG_NVREG_EN_VAUDP15_ADDR                             MT6353_AUDDEC_ANA_CON7
#define PMIC_RG_NVREG_EN_VAUDP15_MASK                             0x1
#define PMIC_RG_NVREG_EN_VAUDP15_SHIFT                            15
#define PMIC_RG_NVREG_PULL0V_VAUDP15_ADDR                         MT6353_AUDDEC_ANA_CON8
#define PMIC_RG_NVREG_PULL0V_VAUDP15_MASK                         0x1
#define PMIC_RG_NVREG_PULL0V_VAUDP15_SHIFT                        0
#define PMIC_RG_AUDGLB_PWRDN_VA28_ADDR                            MT6353_AUDDEC_ANA_CON8
#define PMIC_RG_AUDGLB_PWRDN_VA28_MASK                            0x1
#define PMIC_RG_AUDGLB_PWRDN_VA28_SHIFT                           1
#define PMIC_RG_AUDPREAMPLON_ADDR                                 MT6353_AUDENC_ANA_CON0
#define PMIC_RG_AUDPREAMPLON_MASK                                 0x1
#define PMIC_RG_AUDPREAMPLON_SHIFT                                0
#define PMIC_RG_AUDPREAMPLDCCEN_ADDR                              MT6353_AUDENC_ANA_CON0
#define PMIC_RG_AUDPREAMPLDCCEN_MASK                              0x1
#define PMIC_RG_AUDPREAMPLDCCEN_SHIFT                             1
#define PMIC_RG_AUDPREAMPLDCRPECHARGE_ADDR                        MT6353_AUDENC_ANA_CON0
#define PMIC_RG_AUDPREAMPLDCRPECHARGE_MASK                        0x1
#define PMIC_RG_AUDPREAMPLDCRPECHARGE_SHIFT                       2
#define PMIC_RG_AUDPREAMPLPGATEST_ADDR                            MT6353_AUDENC_ANA_CON0
#define PMIC_RG_AUDPREAMPLPGATEST_MASK                            0x1
#define PMIC_RG_AUDPREAMPLPGATEST_SHIFT                           3
#define PMIC_RG_AUDPREAMPLVSCALE_ADDR                             MT6353_AUDENC_ANA_CON0
#define PMIC_RG_AUDPREAMPLVSCALE_MASK                             0x3
#define PMIC_RG_AUDPREAMPLVSCALE_SHIFT                            4
#define PMIC_RG_AUDPREAMPLINPUTSEL_ADDR                           MT6353_AUDENC_ANA_CON0
#define PMIC_RG_AUDPREAMPLINPUTSEL_MASK                           0x3
#define PMIC_RG_AUDPREAMPLINPUTSEL_SHIFT                          6
#define PMIC_RG_AUDADCLPWRUP_ADDR                                 MT6353_AUDENC_ANA_CON0
#define PMIC_RG_AUDADCLPWRUP_MASK                                 0x1
#define PMIC_RG_AUDADCLPWRUP_SHIFT                                8
#define PMIC_RG_AUDADCLINPUTSEL_ADDR                              MT6353_AUDENC_ANA_CON0
#define PMIC_RG_AUDADCLINPUTSEL_MASK                              0x3
#define PMIC_RG_AUDADCLINPUTSEL_SHIFT                             9
#define PMIC_RG_AUDPREAMPRON_ADDR                                 MT6353_AUDENC_ANA_CON1
#define PMIC_RG_AUDPREAMPRON_MASK                                 0x1
#define PMIC_RG_AUDPREAMPRON_SHIFT                                0
#define PMIC_RG_AUDPREAMPRDCCEN_ADDR                              MT6353_AUDENC_ANA_CON1
#define PMIC_RG_AUDPREAMPRDCCEN_MASK                              0x1
#define PMIC_RG_AUDPREAMPRDCCEN_SHIFT                             1
#define PMIC_RG_AUDPREAMPRDCRPECHARGE_ADDR                        MT6353_AUDENC_ANA_CON1
#define PMIC_RG_AUDPREAMPRDCRPECHARGE_MASK                        0x1
#define PMIC_RG_AUDPREAMPRDCRPECHARGE_SHIFT                       2
#define PMIC_RG_AUDPREAMPRPGATEST_ADDR                            MT6353_AUDENC_ANA_CON1
#define PMIC_RG_AUDPREAMPRPGATEST_MASK                            0x1
#define PMIC_RG_AUDPREAMPRPGATEST_SHIFT                           3
#define PMIC_RG_AUDPREAMPRVSCALE_ADDR                             MT6353_AUDENC_ANA_CON1
#define PMIC_RG_AUDPREAMPRVSCALE_MASK                             0x3
#define PMIC_RG_AUDPREAMPRVSCALE_SHIFT                            4
#define PMIC_RG_AUDPREAMPRINPUTSEL_ADDR                           MT6353_AUDENC_ANA_CON1
#define PMIC_RG_AUDPREAMPRINPUTSEL_MASK                           0x3
#define PMIC_RG_AUDPREAMPRINPUTSEL_SHIFT                          6
#define PMIC_RG_AUDADCRPWRUP_ADDR                                 MT6353_AUDENC_ANA_CON1
#define PMIC_RG_AUDADCRPWRUP_MASK                                 0x1
#define PMIC_RG_AUDADCRPWRUP_SHIFT                                8
#define PMIC_RG_AUDADCRINPUTSEL_ADDR                              MT6353_AUDENC_ANA_CON1
#define PMIC_RG_AUDADCRINPUTSEL_MASK                              0x3
#define PMIC_RG_AUDADCRINPUTSEL_SHIFT                             9
#define PMIC_RG_AUDULHALFBIAS_ADDR                                MT6353_AUDENC_ANA_CON2
#define PMIC_RG_AUDULHALFBIAS_MASK                                0x1
#define PMIC_RG_AUDULHALFBIAS_SHIFT                               0
#define PMIC_RG_AUDGLBMADLPWEN_ADDR                               MT6353_AUDENC_ANA_CON2
#define PMIC_RG_AUDGLBMADLPWEN_MASK                               0x1
#define PMIC_RG_AUDGLBMADLPWEN_SHIFT                              1
#define PMIC_RG_AUDPREAMPLPEN_ADDR                                MT6353_AUDENC_ANA_CON2
#define PMIC_RG_AUDPREAMPLPEN_MASK                                0x1
#define PMIC_RG_AUDPREAMPLPEN_SHIFT                               2
#define PMIC_RG_AUDADC1STSTAGELPEN_ADDR                           MT6353_AUDENC_ANA_CON2
#define PMIC_RG_AUDADC1STSTAGELPEN_MASK                           0x1
#define PMIC_RG_AUDADC1STSTAGELPEN_SHIFT                          3
#define PMIC_RG_AUDADC2NDSTAGELPEN_ADDR                           MT6353_AUDENC_ANA_CON2
#define PMIC_RG_AUDADC2NDSTAGELPEN_MASK                           0x1
#define PMIC_RG_AUDADC2NDSTAGELPEN_SHIFT                          4
#define PMIC_RG_AUDADCFLASHLPEN_ADDR                              MT6353_AUDENC_ANA_CON2
#define PMIC_RG_AUDADCFLASHLPEN_MASK                              0x1
#define PMIC_RG_AUDADCFLASHLPEN_SHIFT                             5
#define PMIC_RG_AUDPREAMPIDDTEST_ADDR                             MT6353_AUDENC_ANA_CON2
#define PMIC_RG_AUDPREAMPIDDTEST_MASK                             0x3
#define PMIC_RG_AUDPREAMPIDDTEST_SHIFT                            6
#define PMIC_RG_AUDADC1STSTAGEIDDTEST_ADDR                        MT6353_AUDENC_ANA_CON2
#define PMIC_RG_AUDADC1STSTAGEIDDTEST_MASK                        0x3
#define PMIC_RG_AUDADC1STSTAGEIDDTEST_SHIFT                       8
#define PMIC_RG_AUDADC2NDSTAGEIDDTEST_ADDR                        MT6353_AUDENC_ANA_CON2
#define PMIC_RG_AUDADC2NDSTAGEIDDTEST_MASK                        0x3
#define PMIC_RG_AUDADC2NDSTAGEIDDTEST_SHIFT                       10
#define PMIC_RG_AUDADCREFBUFIDDTEST_ADDR                          MT6353_AUDENC_ANA_CON2
#define PMIC_RG_AUDADCREFBUFIDDTEST_MASK                          0x3
#define PMIC_RG_AUDADCREFBUFIDDTEST_SHIFT                         12
#define PMIC_RG_AUDADCFLASHIDDTEST_ADDR                           MT6353_AUDENC_ANA_CON2
#define PMIC_RG_AUDADCFLASHIDDTEST_MASK                           0x3
#define PMIC_RG_AUDADCFLASHIDDTEST_SHIFT                          14
#define PMIC_RG_AUDADCDAC0P25FS_ADDR                              MT6353_AUDENC_ANA_CON3
#define PMIC_RG_AUDADCDAC0P25FS_MASK                              0x1
#define PMIC_RG_AUDADCDAC0P25FS_SHIFT                             0
#define PMIC_RG_AUDADCCLKSEL_ADDR                                 MT6353_AUDENC_ANA_CON3
#define PMIC_RG_AUDADCCLKSEL_MASK                                 0x1
#define PMIC_RG_AUDADCCLKSEL_SHIFT                                1
#define PMIC_RG_AUDADCCLKSOURCE_ADDR                              MT6353_AUDENC_ANA_CON3
#define PMIC_RG_AUDADCCLKSOURCE_MASK                              0x3
#define PMIC_RG_AUDADCCLKSOURCE_SHIFT                             2
#define PMIC_RG_AUDADCCLKGENMODE_ADDR                             MT6353_AUDENC_ANA_CON3
#define PMIC_RG_AUDADCCLKGENMODE_MASK                             0x3
#define PMIC_RG_AUDADCCLKGENMODE_SHIFT                            4
#define PMIC_RG_AUDPREAMPAAFEN_ADDR                               MT6353_AUDENC_ANA_CON3
#define PMIC_RG_AUDPREAMPAAFEN_MASK                               0x1
#define PMIC_RG_AUDPREAMPAAFEN_SHIFT                              8
#define PMIC_RG_DCCVCMBUFLPMODSEL_ADDR                            MT6353_AUDENC_ANA_CON3
#define PMIC_RG_DCCVCMBUFLPMODSEL_MASK                            0x1
#define PMIC_RG_DCCVCMBUFLPMODSEL_SHIFT                           9
#define PMIC_RG_DCCVCMBUFLPSWEN_ADDR                              MT6353_AUDENC_ANA_CON3
#define PMIC_RG_DCCVCMBUFLPSWEN_MASK                              0x1
#define PMIC_RG_DCCVCMBUFLPSWEN_SHIFT                             10
#define PMIC_RG_AUDSPAREPGA_ADDR                                  MT6353_AUDENC_ANA_CON3
#define PMIC_RG_AUDSPAREPGA_MASK                                  0x1
#define PMIC_RG_AUDSPAREPGA_SHIFT                                 11
#define PMIC_RG_AUDADC1STSTAGESDENB_ADDR                          MT6353_AUDENC_ANA_CON4
#define PMIC_RG_AUDADC1STSTAGESDENB_MASK                          0x1
#define PMIC_RG_AUDADC1STSTAGESDENB_SHIFT                         0
#define PMIC_RG_AUDADC2NDSTAGERESET_ADDR                          MT6353_AUDENC_ANA_CON4
#define PMIC_RG_AUDADC2NDSTAGERESET_MASK                          0x1
#define PMIC_RG_AUDADC2NDSTAGERESET_SHIFT                         1
#define PMIC_RG_AUDADC3RDSTAGERESET_ADDR                          MT6353_AUDENC_ANA_CON4
#define PMIC_RG_AUDADC3RDSTAGERESET_MASK                          0x1
#define PMIC_RG_AUDADC3RDSTAGERESET_SHIFT                         2
#define PMIC_RG_AUDADCFSRESET_ADDR                                MT6353_AUDENC_ANA_CON4
#define PMIC_RG_AUDADCFSRESET_MASK                                0x1
#define PMIC_RG_AUDADCFSRESET_SHIFT                               3
#define PMIC_RG_AUDADCWIDECM_ADDR                                 MT6353_AUDENC_ANA_CON4
#define PMIC_RG_AUDADCWIDECM_MASK                                 0x1
#define PMIC_RG_AUDADCWIDECM_SHIFT                                4
#define PMIC_RG_AUDADCNOPATEST_ADDR                               MT6353_AUDENC_ANA_CON4
#define PMIC_RG_AUDADCNOPATEST_MASK                               0x1
#define PMIC_RG_AUDADCNOPATEST_SHIFT                              5
#define PMIC_RG_AUDADCBYPASS_ADDR                                 MT6353_AUDENC_ANA_CON4
#define PMIC_RG_AUDADCBYPASS_MASK                                 0x1
#define PMIC_RG_AUDADCBYPASS_SHIFT                                6
#define PMIC_RG_AUDADCFFBYPASS_ADDR                               MT6353_AUDENC_ANA_CON4
#define PMIC_RG_AUDADCFFBYPASS_MASK                               0x1
#define PMIC_RG_AUDADCFFBYPASS_SHIFT                              7
#define PMIC_RG_AUDADCDACFBCURRENT_ADDR                           MT6353_AUDENC_ANA_CON4
#define PMIC_RG_AUDADCDACFBCURRENT_MASK                           0x1
#define PMIC_RG_AUDADCDACFBCURRENT_SHIFT                          8
#define PMIC_RG_AUDADCDACIDDTEST_ADDR                             MT6353_AUDENC_ANA_CON4
#define PMIC_RG_AUDADCDACIDDTEST_MASK                             0x3
#define PMIC_RG_AUDADCDACIDDTEST_SHIFT                            9
#define PMIC_RG_AUDADCDACNRZ_ADDR                                 MT6353_AUDENC_ANA_CON4
#define PMIC_RG_AUDADCDACNRZ_MASK                                 0x1
#define PMIC_RG_AUDADCDACNRZ_SHIFT                                11
#define PMIC_RG_AUDADCNODEM_ADDR                                  MT6353_AUDENC_ANA_CON4
#define PMIC_RG_AUDADCNODEM_MASK                                  0x1
#define PMIC_RG_AUDADCNODEM_SHIFT                                 12
#define PMIC_RG_AUDADCDACTEST_ADDR                                MT6353_AUDENC_ANA_CON4
#define PMIC_RG_AUDADCDACTEST_MASK                                0x1
#define PMIC_RG_AUDADCDACTEST_SHIFT                               13
#define PMIC_RG_AUDADCTESTDATA_ADDR                               MT6353_AUDENC_ANA_CON5
#define PMIC_RG_AUDADCTESTDATA_MASK                               0xFFFF
#define PMIC_RG_AUDADCTESTDATA_SHIFT                              0
#define PMIC_RG_AUDRCTUNEL_ADDR                                   MT6353_AUDENC_ANA_CON6
#define PMIC_RG_AUDRCTUNEL_MASK                                   0x1F
#define PMIC_RG_AUDRCTUNEL_SHIFT                                  0
#define PMIC_RG_AUDRCTUNELSEL_ADDR                                MT6353_AUDENC_ANA_CON6
#define PMIC_RG_AUDRCTUNELSEL_MASK                                0x1
#define PMIC_RG_AUDRCTUNELSEL_SHIFT                               5
#define PMIC_RG_AUDRCTUNER_ADDR                                   MT6353_AUDENC_ANA_CON6
#define PMIC_RG_AUDRCTUNER_MASK                                   0x1F
#define PMIC_RG_AUDRCTUNER_SHIFT                                  6
#define PMIC_RG_AUDRCTUNERSEL_ADDR                                MT6353_AUDENC_ANA_CON6
#define PMIC_RG_AUDRCTUNERSEL_MASK                                0x1
#define PMIC_RG_AUDRCTUNERSEL_SHIFT                               13
#define PMIC_RG_AUDSPAREVA28_ADDR                                 MT6353_AUDENC_ANA_CON7
#define PMIC_RG_AUDSPAREVA28_MASK                                 0xF
#define PMIC_RG_AUDSPAREVA28_SHIFT                                0
#define PMIC_RG_AUDSPAREVA18_ADDR                                 MT6353_AUDENC_ANA_CON7
#define PMIC_RG_AUDSPAREVA18_MASK                                 0xF
#define PMIC_RG_AUDSPAREVA18_SHIFT                                4
#define PMIC_RG_AUDENCSPAREVA28_ADDR                              MT6353_AUDENC_ANA_CON7
#define PMIC_RG_AUDENCSPAREVA28_MASK                              0xF
#define PMIC_RG_AUDENCSPAREVA28_SHIFT                             8
#define PMIC_RG_AUDENCSPAREVA18_ADDR                              MT6353_AUDENC_ANA_CON7
#define PMIC_RG_AUDENCSPAREVA18_MASK                              0xF
#define PMIC_RG_AUDENCSPAREVA18_SHIFT                             12
#define PMIC_RG_AUDDIGMICEN_ADDR                                  MT6353_AUDENC_ANA_CON8
#define PMIC_RG_AUDDIGMICEN_MASK                                  0x1
#define PMIC_RG_AUDDIGMICEN_SHIFT                                 0
#define PMIC_RG_AUDDIGMICBIAS_ADDR                                MT6353_AUDENC_ANA_CON8
#define PMIC_RG_AUDDIGMICBIAS_MASK                                0x3
#define PMIC_RG_AUDDIGMICBIAS_SHIFT                               1
#define PMIC_RG_DMICHPCLKEN_ADDR                                  MT6353_AUDENC_ANA_CON8
#define PMIC_RG_DMICHPCLKEN_MASK                                  0x1
#define PMIC_RG_DMICHPCLKEN_SHIFT                                 3
#define PMIC_RG_AUDDIGMICPDUTY_ADDR                               MT6353_AUDENC_ANA_CON8
#define PMIC_RG_AUDDIGMICPDUTY_MASK                               0x3
#define PMIC_RG_AUDDIGMICPDUTY_SHIFT                              4
#define PMIC_RG_AUDDIGMICNDUTY_ADDR                               MT6353_AUDENC_ANA_CON8
#define PMIC_RG_AUDDIGMICNDUTY_MASK                               0x3
#define PMIC_RG_AUDDIGMICNDUTY_SHIFT                              6
#define PMIC_RG_DMICMONEN_ADDR                                    MT6353_AUDENC_ANA_CON8
#define PMIC_RG_DMICMONEN_MASK                                    0x1
#define PMIC_RG_DMICMONEN_SHIFT                                   8
#define PMIC_RG_DMICMONSEL_ADDR                                   MT6353_AUDENC_ANA_CON8
#define PMIC_RG_DMICMONSEL_MASK                                   0x7
#define PMIC_RG_DMICMONSEL_SHIFT                                  9
#define PMIC_RG_AUDSPAREVMIC_ADDR                                 MT6353_AUDENC_ANA_CON8
#define PMIC_RG_AUDSPAREVMIC_MASK                                 0xF
#define PMIC_RG_AUDSPAREVMIC_SHIFT                                12
#define PMIC_RG_AUDPWDBMICBIAS0_ADDR                              MT6353_AUDENC_ANA_CON9
#define PMIC_RG_AUDPWDBMICBIAS0_MASK                              0x1
#define PMIC_RG_AUDPWDBMICBIAS0_SHIFT                             0
#define PMIC_RG_AUDMICBIAS0DCSWPEN_ADDR                           MT6353_AUDENC_ANA_CON9
#define PMIC_RG_AUDMICBIAS0DCSWPEN_MASK                           0x1
#define PMIC_RG_AUDMICBIAS0DCSWPEN_SHIFT                          1
#define PMIC_RG_AUDMICBIAS0DCSWNEN_ADDR                           MT6353_AUDENC_ANA_CON9
#define PMIC_RG_AUDMICBIAS0DCSWNEN_MASK                           0x1
#define PMIC_RG_AUDMICBIAS0DCSWNEN_SHIFT                          2
#define PMIC_RG_AUDMICBIAS0BYPASSEN_ADDR                          MT6353_AUDENC_ANA_CON9
#define PMIC_RG_AUDMICBIAS0BYPASSEN_MASK                          0x1
#define PMIC_RG_AUDMICBIAS0BYPASSEN_SHIFT                         3
#define PMIC_RG_AUDPWDBMICBIAS1_ADDR                              MT6353_AUDENC_ANA_CON9
#define PMIC_RG_AUDPWDBMICBIAS1_MASK                              0x1
#define PMIC_RG_AUDPWDBMICBIAS1_SHIFT                             4
#define PMIC_RG_AUDMICBIAS1DCSWPEN_ADDR                           MT6353_AUDENC_ANA_CON9
#define PMIC_RG_AUDMICBIAS1DCSWPEN_MASK                           0x1
#define PMIC_RG_AUDMICBIAS1DCSWPEN_SHIFT                          5
#define PMIC_RG_AUDMICBIAS1DCSWNEN_ADDR                           MT6353_AUDENC_ANA_CON9
#define PMIC_RG_AUDMICBIAS1DCSWNEN_MASK                           0x1
#define PMIC_RG_AUDMICBIAS1DCSWNEN_SHIFT                          6
#define PMIC_RG_AUDMICBIAS1BYPASSEN_ADDR                          MT6353_AUDENC_ANA_CON9
#define PMIC_RG_AUDMICBIAS1BYPASSEN_MASK                          0x1
#define PMIC_RG_AUDMICBIAS1BYPASSEN_SHIFT                         7
#define PMIC_RG_AUDMICBIASVREF_ADDR                               MT6353_AUDENC_ANA_CON9
#define PMIC_RG_AUDMICBIASVREF_MASK                               0x7
#define PMIC_RG_AUDMICBIASVREF_SHIFT                              8
#define PMIC_RG_AUDMICBIASLOWPEN_ADDR                             MT6353_AUDENC_ANA_CON9
#define PMIC_RG_AUDMICBIASLOWPEN_MASK                             0x1
#define PMIC_RG_AUDMICBIASLOWPEN_SHIFT                            11
#define PMIC_RG_BANDGAPGEN_ADDR                                   MT6353_AUDENC_ANA_CON9
#define PMIC_RG_BANDGAPGEN_MASK                                   0x1
#define PMIC_RG_BANDGAPGEN_SHIFT                                  12
#define PMIC_RG_AUDPREAMPLGAIN_ADDR                               MT6353_AUDENC_ANA_CON10
#define PMIC_RG_AUDPREAMPLGAIN_MASK                               0x7
#define PMIC_RG_AUDPREAMPLGAIN_SHIFT                              0
#define PMIC_RG_AUDPREAMPRGAIN_ADDR                               MT6353_AUDENC_ANA_CON10
#define PMIC_RG_AUDPREAMPRGAIN_MASK                               0x7
#define PMIC_RG_AUDPREAMPRGAIN_SHIFT                              4
#define PMIC_RG_DIVCKS_CHG_ADDR                                   MT6353_AUDNCP_CLKDIV_CON0
#define PMIC_RG_DIVCKS_CHG_MASK                                   0x1
#define PMIC_RG_DIVCKS_CHG_SHIFT                                  0
#define PMIC_RG_DIVCKS_ON_ADDR                                    MT6353_AUDNCP_CLKDIV_CON1
#define PMIC_RG_DIVCKS_ON_MASK                                    0x1
#define PMIC_RG_DIVCKS_ON_SHIFT                                   0
#define PMIC_RG_DIVCKS_PRG_ADDR                                   MT6353_AUDNCP_CLKDIV_CON2
#define PMIC_RG_DIVCKS_PRG_MASK                                   0x1FF
#define PMIC_RG_DIVCKS_PRG_SHIFT                                  0
#define PMIC_RG_DIVCKS_PWD_NCP_ADDR                               MT6353_AUDNCP_CLKDIV_CON3
#define PMIC_RG_DIVCKS_PWD_NCP_MASK                               0x1
#define PMIC_RG_DIVCKS_PWD_NCP_SHIFT                              0
#define PMIC_RG_DIVCKS_PWD_NCP_ST_SEL_ADDR                        MT6353_AUDNCP_CLKDIV_CON4
#define PMIC_RG_DIVCKS_PWD_NCP_ST_SEL_MASK                        0x3
#define PMIC_RG_DIVCKS_PWD_NCP_ST_SEL_SHIFT                       0
#define PMIC_XO_EXTBUF1_MODE_ADDR                                 MT6353_DCXO_CW00
#define PMIC_XO_EXTBUF1_MODE_MASK                                 0x3
#define PMIC_XO_EXTBUF1_MODE_SHIFT                                0
#define PMIC_XO_EXTBUF1_EN_M_ADDR                                 MT6353_DCXO_CW00
#define PMIC_XO_EXTBUF1_EN_M_MASK                                 0x1
#define PMIC_XO_EXTBUF1_EN_M_SHIFT                                2
#define PMIC_XO_EXTBUF2_MODE_ADDR                                 MT6353_DCXO_CW00
#define PMIC_XO_EXTBUF2_MODE_MASK                                 0x3
#define PMIC_XO_EXTBUF2_MODE_SHIFT                                3
#define PMIC_XO_EXTBUF2_EN_M_ADDR                                 MT6353_DCXO_CW00
#define PMIC_XO_EXTBUF2_EN_M_MASK                                 0x1
#define PMIC_XO_EXTBUF2_EN_M_SHIFT                                5
#define PMIC_XO_EXTBUF3_MODE_ADDR                                 MT6353_DCXO_CW00
#define PMIC_XO_EXTBUF3_MODE_MASK                                 0x3
#define PMIC_XO_EXTBUF3_MODE_SHIFT                                6
#define PMIC_XO_EXTBUF3_EN_M_ADDR                                 MT6353_DCXO_CW00
#define PMIC_XO_EXTBUF3_EN_M_MASK                                 0x1
#define PMIC_XO_EXTBUF3_EN_M_SHIFT                                8
#define PMIC_XO_EXTBUF4_MODE_ADDR                                 MT6353_DCXO_CW00
#define PMIC_XO_EXTBUF4_MODE_MASK                                 0x3
#define PMIC_XO_EXTBUF4_MODE_SHIFT                                9
#define PMIC_XO_EXTBUF4_EN_M_ADDR                                 MT6353_DCXO_CW00
#define PMIC_XO_EXTBUF4_EN_M_MASK                                 0x1
#define PMIC_XO_EXTBUF4_EN_M_SHIFT                                11
#define PMIC_XO_BB_LPM_EN_ADDR                                    MT6353_DCXO_CW00
#define PMIC_XO_BB_LPM_EN_MASK                                    0x1
#define PMIC_XO_BB_LPM_EN_SHIFT                                   12
#define PMIC_XO_ENBB_MAN_ADDR                                     MT6353_DCXO_CW00
#define PMIC_XO_ENBB_MAN_MASK                                     0x1
#define PMIC_XO_ENBB_MAN_SHIFT                                    13
#define PMIC_XO_ENBB_EN_M_ADDR                                    MT6353_DCXO_CW00
#define PMIC_XO_ENBB_EN_M_MASK                                    0x1
#define PMIC_XO_ENBB_EN_M_SHIFT                                   14
#define PMIC_XO_CLKSEL_MAN_ADDR                                   MT6353_DCXO_CW00
#define PMIC_XO_CLKSEL_MAN_MASK                                   0x1
#define PMIC_XO_CLKSEL_MAN_SHIFT                                  15
#define PMIC_DCXO_CW00_SET_ADDR                                   MT6353_DCXO_CW00_SET
#define PMIC_DCXO_CW00_SET_MASK                                   0xFFFF
#define PMIC_DCXO_CW00_SET_SHIFT                                  0
#define PMIC_DCXO_CW00_CLR_ADDR                                   MT6353_DCXO_CW00_CLR
#define PMIC_DCXO_CW00_CLR_MASK                                   0xFFFF
#define PMIC_DCXO_CW00_CLR_SHIFT                                  0
#define PMIC_XO_CLKSEL_EN_M_ADDR                                  MT6353_DCXO_CW01
#define PMIC_XO_CLKSEL_EN_M_MASK                                  0x1
#define PMIC_XO_CLKSEL_EN_M_SHIFT                                 0
#define PMIC_XO_EXTBUF1_CKG_MAN_ADDR                              MT6353_DCXO_CW01
#define PMIC_XO_EXTBUF1_CKG_MAN_MASK                              0x1
#define PMIC_XO_EXTBUF1_CKG_MAN_SHIFT                             1
#define PMIC_XO_EXTBUF1_CKG_EN_M_ADDR                             MT6353_DCXO_CW01
#define PMIC_XO_EXTBUF1_CKG_EN_M_MASK                             0x1
#define PMIC_XO_EXTBUF1_CKG_EN_M_SHIFT                            2
#define PMIC_XO_EXTBUF2_CKG_MAN_ADDR                              MT6353_DCXO_CW01
#define PMIC_XO_EXTBUF2_CKG_MAN_MASK                              0x1
#define PMIC_XO_EXTBUF2_CKG_MAN_SHIFT                             3
#define PMIC_XO_EXTBUF2_CKG_EN_M_ADDR                             MT6353_DCXO_CW01
#define PMIC_XO_EXTBUF2_CKG_EN_M_MASK                             0x1
#define PMIC_XO_EXTBUF2_CKG_EN_M_SHIFT                            4
#define PMIC_XO_EXTBUF3_CKG_MAN_ADDR                              MT6353_DCXO_CW01
#define PMIC_XO_EXTBUF3_CKG_MAN_MASK                              0x1
#define PMIC_XO_EXTBUF3_CKG_MAN_SHIFT                             5
#define PMIC_XO_EXTBUF3_CKG_EN_M_ADDR                             MT6353_DCXO_CW01
#define PMIC_XO_EXTBUF3_CKG_EN_M_MASK                             0x1
#define PMIC_XO_EXTBUF3_CKG_EN_M_SHIFT                            6
#define PMIC_XO_EXTBUF4_CKG_MAN_ADDR                              MT6353_DCXO_CW01
#define PMIC_XO_EXTBUF4_CKG_MAN_MASK                              0x1
#define PMIC_XO_EXTBUF4_CKG_MAN_SHIFT                             7
#define PMIC_XO_EXTBUF4_CKG_EN_M_ADDR                             MT6353_DCXO_CW01
#define PMIC_XO_EXTBUF4_CKG_EN_M_MASK                             0x1
#define PMIC_XO_EXTBUF4_CKG_EN_M_SHIFT                            8
#define PMIC_XO_INTBUF_MAN_ADDR                                   MT6353_DCXO_CW01
#define PMIC_XO_INTBUF_MAN_MASK                                   0x1
#define PMIC_XO_INTBUF_MAN_SHIFT                                  9
#define PMIC_XO_PBUF_EN_M_ADDR                                    MT6353_DCXO_CW01
#define PMIC_XO_PBUF_EN_M_MASK                                    0x1
#define PMIC_XO_PBUF_EN_M_SHIFT                                   10
#define PMIC_XO_IBUF_EN_M_ADDR                                    MT6353_DCXO_CW01
#define PMIC_XO_IBUF_EN_M_MASK                                    0x1
#define PMIC_XO_IBUF_EN_M_SHIFT                                   11
#define PMIC_XO_LPMBUF_MAN_ADDR                                   MT6353_DCXO_CW01
#define PMIC_XO_LPMBUF_MAN_MASK                                   0x1
#define PMIC_XO_LPMBUF_MAN_SHIFT                                  12
#define PMIC_XO_LPM_PREBUF_EN_M_ADDR                              MT6353_DCXO_CW01
#define PMIC_XO_LPM_PREBUF_EN_M_MASK                              0x1
#define PMIC_XO_LPM_PREBUF_EN_M_SHIFT                             13
#define PMIC_XO_LPBUF_EN_M_ADDR                                   MT6353_DCXO_CW01
#define PMIC_XO_LPBUF_EN_M_MASK                                   0x1
#define PMIC_XO_LPBUF_EN_M_SHIFT                                  14
#define PMIC_XO_AUDIO_EN_M_ADDR                                   MT6353_DCXO_CW01
#define PMIC_XO_AUDIO_EN_M_MASK                                   0x1
#define PMIC_XO_AUDIO_EN_M_SHIFT                                  15
#define PMIC_XO_EN32K_MAN_ADDR                                    MT6353_DCXO_CW02
#define PMIC_XO_EN32K_MAN_MASK                                    0x1
#define PMIC_XO_EN32K_MAN_SHIFT                                   0
#define PMIC_XO_EN32K_M_ADDR                                      MT6353_DCXO_CW02
#define PMIC_XO_EN32K_M_MASK                                      0x1
#define PMIC_XO_EN32K_M_SHIFT                                     1
#define PMIC_XO_XMODE_MAN_ADDR                                    MT6353_DCXO_CW02
#define PMIC_XO_XMODE_MAN_MASK                                    0x1
#define PMIC_XO_XMODE_MAN_SHIFT                                   2
#define PMIC_XO_XMODE_M_ADDR                                      MT6353_DCXO_CW02
#define PMIC_XO_XMODE_M_MASK                                      0x1
#define PMIC_XO_XMODE_M_SHIFT                                     3
#define PMIC_XO_STRUP_MODE_ADDR                                   MT6353_DCXO_CW02
#define PMIC_XO_STRUP_MODE_MASK                                   0x1
#define PMIC_XO_STRUP_MODE_SHIFT                                  4
#define PMIC_XO_AAC_FPM_TIME_ADDR                                 MT6353_DCXO_CW02
#define PMIC_XO_AAC_FPM_TIME_MASK                                 0x3
#define PMIC_XO_AAC_FPM_TIME_SHIFT                                5
#define PMIC_XO_AAC_MODE_LPM_ADDR                                 MT6353_DCXO_CW02
#define PMIC_XO_AAC_MODE_LPM_MASK                                 0x3
#define PMIC_XO_AAC_MODE_LPM_SHIFT                                7
#define PMIC_XO_AAC_MODE_FPM_ADDR                                 MT6353_DCXO_CW02
#define PMIC_XO_AAC_MODE_FPM_MASK                                 0x3
#define PMIC_XO_AAC_MODE_FPM_SHIFT                                9
#define PMIC_XO_EN26M_OFFSQ_EN_ADDR                               MT6353_DCXO_CW02
#define PMIC_XO_EN26M_OFFSQ_EN_MASK                               0x1
#define PMIC_XO_EN26M_OFFSQ_EN_SHIFT                              11
#define PMIC_XO_LDOCAL_EN_ADDR                                    MT6353_DCXO_CW02
#define PMIC_XO_LDOCAL_EN_MASK                                    0x1
#define PMIC_XO_LDOCAL_EN_SHIFT                                   12
#define PMIC_XO_CBANK_SYNC_DYN_ADDR                               MT6353_DCXO_CW02
#define PMIC_XO_CBANK_SYNC_DYN_MASK                               0x1
#define PMIC_XO_CBANK_SYNC_DYN_SHIFT                              13
#define PMIC_XO_26MLP_MAN_EN_ADDR                                 MT6353_DCXO_CW02
#define PMIC_XO_26MLP_MAN_EN_MASK                                 0x1
#define PMIC_XO_26MLP_MAN_EN_SHIFT                                14
#define PMIC_XO_OPMODE_MAN_ADDR                                   MT6353_DCXO_CW02
#define PMIC_XO_OPMODE_MAN_MASK                                   0x1
#define PMIC_XO_OPMODE_MAN_SHIFT                                  15
#define PMIC_XO_BUFLDO13_VSET_M_ADDR                              MT6353_DCXO_CW03
#define PMIC_XO_BUFLDO13_VSET_M_MASK                              0xF
#define PMIC_XO_BUFLDO13_VSET_M_SHIFT                             0
#define PMIC_XO_RESERVED0_ADDR                                    MT6353_DCXO_CW03
#define PMIC_XO_RESERVED0_MASK                                    0x3
#define PMIC_XO_RESERVED0_SHIFT                                   4
#define PMIC_XO_LPM_ISEL_MAN_ADDR                                 MT6353_DCXO_CW03
#define PMIC_XO_LPM_ISEL_MAN_MASK                                 0x1F
#define PMIC_XO_LPM_ISEL_MAN_SHIFT                                6
#define PMIC_XO_FPM_ISEL_MAN_ADDR                                 MT6353_DCXO_CW03
#define PMIC_XO_FPM_ISEL_MAN_MASK                                 0x1F
#define PMIC_XO_FPM_ISEL_MAN_SHIFT                                11
#define PMIC_XO_CDAC_FPM_ADDR                                     MT6353_DCXO_CW04
#define PMIC_XO_CDAC_FPM_MASK                                     0xFF
#define PMIC_XO_CDAC_FPM_SHIFT                                    0
#define PMIC_XO_CDAC_LPM_ADDR                                     MT6353_DCXO_CW04
#define PMIC_XO_CDAC_LPM_MASK                                     0xFF
#define PMIC_XO_CDAC_LPM_SHIFT                                    8
#define PMIC_XO_32KDIV_NFRAC_FPM_ADDR                             MT6353_DCXO_CW05
#define PMIC_XO_32KDIV_NFRAC_FPM_MASK                             0x3FFF
#define PMIC_XO_32KDIV_NFRAC_FPM_SHIFT                            0
#define PMIC_XO_COFST_FPM_ADDR                                    MT6353_DCXO_CW05
#define PMIC_XO_COFST_FPM_MASK                                    0x3
#define PMIC_XO_COFST_FPM_SHIFT                                   14
#define PMIC_XO_32KDIV_NFRAC_LPM_ADDR                             MT6353_DCXO_CW06
#define PMIC_XO_32KDIV_NFRAC_LPM_MASK                             0x3FFF
#define PMIC_XO_32KDIV_NFRAC_LPM_SHIFT                            0
#define PMIC_XO_COFST_LPM_ADDR                                    MT6353_DCXO_CW06
#define PMIC_XO_COFST_LPM_MASK                                    0x3
#define PMIC_XO_COFST_LPM_SHIFT                                   14
#define PMIC_XO_CORE_MAN_ADDR                                     MT6353_DCXO_CW07
#define PMIC_XO_CORE_MAN_MASK                                     0x1
#define PMIC_XO_CORE_MAN_SHIFT                                    0
#define PMIC_XO_CORE_EN_M_ADDR                                    MT6353_DCXO_CW07
#define PMIC_XO_CORE_EN_M_MASK                                    0x1
#define PMIC_XO_CORE_EN_M_SHIFT                                   1
#define PMIC_XO_CORE_TURBO_EN_M_ADDR                              MT6353_DCXO_CW07
#define PMIC_XO_CORE_TURBO_EN_M_MASK                              0x1
#define PMIC_XO_CORE_TURBO_EN_M_SHIFT                             2
#define PMIC_XO_CORE_AAC_EN_M_ADDR                                MT6353_DCXO_CW07
#define PMIC_XO_CORE_AAC_EN_M_MASK                                0x1
#define PMIC_XO_CORE_AAC_EN_M_SHIFT                               3
#define PMIC_XO_STARTUP_EN_M_ADDR                                 MT6353_DCXO_CW07
#define PMIC_XO_STARTUP_EN_M_MASK                                 0x1
#define PMIC_XO_STARTUP_EN_M_SHIFT                                4
#define PMIC_XO_CORE_VBFPM_EN_M_ADDR                              MT6353_DCXO_CW07
#define PMIC_XO_CORE_VBFPM_EN_M_MASK                              0x1
#define PMIC_XO_CORE_VBFPM_EN_M_SHIFT                             5
#define PMIC_XO_CORE_VBLPM_EN_M_ADDR                              MT6353_DCXO_CW07
#define PMIC_XO_CORE_VBLPM_EN_M_MASK                              0x1
#define PMIC_XO_CORE_VBLPM_EN_M_SHIFT                             6
#define PMIC_XO_LPMBIAS_EN_M_ADDR                                 MT6353_DCXO_CW07
#define PMIC_XO_LPMBIAS_EN_M_MASK                                 0x1
#define PMIC_XO_LPMBIAS_EN_M_SHIFT                                7
#define PMIC_XO_VTCGEN_EN_M_ADDR                                  MT6353_DCXO_CW07
#define PMIC_XO_VTCGEN_EN_M_MASK                                  0x1
#define PMIC_XO_VTCGEN_EN_M_SHIFT                                 8
#define PMIC_XO_IAAC_COMP_EN_M_ADDR                               MT6353_DCXO_CW07
#define PMIC_XO_IAAC_COMP_EN_M_MASK                               0x1
#define PMIC_XO_IAAC_COMP_EN_M_SHIFT                              9
#define PMIC_XO_IFPM_COMP_EN_M_ADDR                               MT6353_DCXO_CW07
#define PMIC_XO_IFPM_COMP_EN_M_MASK                               0x1
#define PMIC_XO_IFPM_COMP_EN_M_SHIFT                              10
#define PMIC_XO_ILPM_COMP_EN_M_ADDR                               MT6353_DCXO_CW07
#define PMIC_XO_ILPM_COMP_EN_M_MASK                               0x1
#define PMIC_XO_ILPM_COMP_EN_M_SHIFT                              11
#define PMIC_XO_CORE_BYPCAS_FPM_ADDR                              MT6353_DCXO_CW07
#define PMIC_XO_CORE_BYPCAS_FPM_MASK                              0x1
#define PMIC_XO_CORE_BYPCAS_FPM_SHIFT                             12
#define PMIC_XO_CORE_GMX2_FPM_ADDR                                MT6353_DCXO_CW07
#define PMIC_XO_CORE_GMX2_FPM_MASK                                0x1
#define PMIC_XO_CORE_GMX2_FPM_SHIFT                               13
#define PMIC_XO_CORE_IDAC_FPM_ADDR                                MT6353_DCXO_CW07
#define PMIC_XO_CORE_IDAC_FPM_MASK                                0x3
#define PMIC_XO_CORE_IDAC_FPM_SHIFT                               14
#define PMIC_XO_AAC_COMP_MAN_ADDR                                 MT6353_DCXO_CW08
#define PMIC_XO_AAC_COMP_MAN_MASK                                 0x1
#define PMIC_XO_AAC_COMP_MAN_SHIFT                                0
#define PMIC_XO_AAC_EN_M_ADDR                                     MT6353_DCXO_CW08
#define PMIC_XO_AAC_EN_M_MASK                                     0x1
#define PMIC_XO_AAC_EN_M_SHIFT                                    1
#define PMIC_XO_AAC_MONEN_M_ADDR                                  MT6353_DCXO_CW08
#define PMIC_XO_AAC_MONEN_M_MASK                                  0x1
#define PMIC_XO_AAC_MONEN_M_SHIFT                                 2
#define PMIC_XO_COMP_EN_M_ADDR                                    MT6353_DCXO_CW08
#define PMIC_XO_COMP_EN_M_MASK                                    0x1
#define PMIC_XO_COMP_EN_M_SHIFT                                   3
#define PMIC_XO_COMP_TSTEN_M_ADDR                                 MT6353_DCXO_CW08
#define PMIC_XO_COMP_TSTEN_M_MASK                                 0x1
#define PMIC_XO_COMP_TSTEN_M_SHIFT                                4
#define PMIC_XO_AAC_HV_FPM_ADDR                                   MT6353_DCXO_CW08
#define PMIC_XO_AAC_HV_FPM_MASK                                   0x1
#define PMIC_XO_AAC_HV_FPM_SHIFT                                  5
#define PMIC_XO_AAC_IBIAS_FPM_ADDR                                MT6353_DCXO_CW08
#define PMIC_XO_AAC_IBIAS_FPM_MASK                                0x3
#define PMIC_XO_AAC_IBIAS_FPM_SHIFT                               6
#define PMIC_XO_AAC_VOFST_FPM_ADDR                                MT6353_DCXO_CW08
#define PMIC_XO_AAC_VOFST_FPM_MASK                                0x3
#define PMIC_XO_AAC_VOFST_FPM_SHIFT                               8
#define PMIC_XO_AAC_COMP_HV_FPM_ADDR                              MT6353_DCXO_CW08
#define PMIC_XO_AAC_COMP_HV_FPM_MASK                              0x1
#define PMIC_XO_AAC_COMP_HV_FPM_SHIFT                             10
#define PMIC_XO_AAC_VSEL_FPM_ADDR                                 MT6353_DCXO_CW08
#define PMIC_XO_AAC_VSEL_FPM_MASK                                 0xF
#define PMIC_XO_AAC_VSEL_FPM_SHIFT                                11
#define PMIC_XO_AAC_COMP_POL_ADDR                                 MT6353_DCXO_CW08
#define PMIC_XO_AAC_COMP_POL_MASK                                 0x1
#define PMIC_XO_AAC_COMP_POL_SHIFT                                15
#define PMIC_XO_CORE_BYPCAS_LPM_ADDR                              MT6353_DCXO_CW09
#define PMIC_XO_CORE_BYPCAS_LPM_MASK                              0x1
#define PMIC_XO_CORE_BYPCAS_LPM_SHIFT                             0
#define PMIC_XO_CORE_GMX2_LPM_ADDR                                MT6353_DCXO_CW09
#define PMIC_XO_CORE_GMX2_LPM_MASK                                0x1
#define PMIC_XO_CORE_GMX2_LPM_SHIFT                               1
#define PMIC_XO_CORE_IDAC_LPM_ADDR                                MT6353_DCXO_CW09
#define PMIC_XO_CORE_IDAC_LPM_MASK                                0x3
#define PMIC_XO_CORE_IDAC_LPM_SHIFT                               2
#define PMIC_XO_AAC_COMP_HV_LPM_ADDR                              MT6353_DCXO_CW09
#define PMIC_XO_AAC_COMP_HV_LPM_MASK                              0x1
#define PMIC_XO_AAC_COMP_HV_LPM_SHIFT                             4
#define PMIC_XO_AAC_VSEL_LPM_ADDR                                 MT6353_DCXO_CW09
#define PMIC_XO_AAC_VSEL_LPM_MASK                                 0xF
#define PMIC_XO_AAC_VSEL_LPM_SHIFT                                5
#define PMIC_XO_AAC_HV_LPM_ADDR                                   MT6353_DCXO_CW09
#define PMIC_XO_AAC_HV_LPM_MASK                                   0x1
#define PMIC_XO_AAC_HV_LPM_SHIFT                                  9
#define PMIC_XO_AAC_IBIAS_LPM_ADDR                                MT6353_DCXO_CW09
#define PMIC_XO_AAC_IBIAS_LPM_MASK                                0x3
#define PMIC_XO_AAC_IBIAS_LPM_SHIFT                               10
#define PMIC_XO_AAC_VOFST_LPM_ADDR                                MT6353_DCXO_CW09
#define PMIC_XO_AAC_VOFST_LPM_MASK                                0x3
#define PMIC_XO_AAC_VOFST_LPM_SHIFT                               12
#define PMIC_XO_AAC_FPM_SWEN_ADDR                                 MT6353_DCXO_CW09
#define PMIC_XO_AAC_FPM_SWEN_MASK                                 0x1
#define PMIC_XO_AAC_FPM_SWEN_SHIFT                                14
#define PMIC_XO_SWRST_ADDR                                        MT6353_DCXO_CW09
#define PMIC_XO_SWRST_MASK                                        0x1
#define PMIC_XO_SWRST_SHIFT                                       15
#define PMIC_XO_32KDIV_SWRST_ADDR                                 MT6353_DCXO_CW10
#define PMIC_XO_32KDIV_SWRST_MASK                                 0x1
#define PMIC_XO_32KDIV_SWRST_SHIFT                                0
#define PMIC_XO_32KDIV_RATIO_MAN_ADDR                             MT6353_DCXO_CW10
#define PMIC_XO_32KDIV_RATIO_MAN_MASK                             0x1
#define PMIC_XO_32KDIV_RATIO_MAN_SHIFT                            1
#define PMIC_XO_32KDIV_TEST_EN_ADDR                               MT6353_DCXO_CW10
#define PMIC_XO_32KDIV_TEST_EN_MASK                               0x1
#define PMIC_XO_32KDIV_TEST_EN_SHIFT                              2
#define PMIC_XO_CBANK_SYNC_MAN_ADDR                               MT6353_DCXO_CW10
#define PMIC_XO_CBANK_SYNC_MAN_MASK                               0x1
#define PMIC_XO_CBANK_SYNC_MAN_SHIFT                              3
#define PMIC_XO_CBANK_SYNC_EN_M_ADDR                              MT6353_DCXO_CW10
#define PMIC_XO_CBANK_SYNC_EN_M_MASK                              0x1
#define PMIC_XO_CBANK_SYNC_EN_M_SHIFT                             4
#define PMIC_XO_CTL_SYNC_MAN_ADDR                                 MT6353_DCXO_CW10
#define PMIC_XO_CTL_SYNC_MAN_MASK                                 0x1
#define PMIC_XO_CTL_SYNC_MAN_SHIFT                                5
#define PMIC_XO_CTL_SYNC_EN_M_ADDR                                MT6353_DCXO_CW10
#define PMIC_XO_CTL_SYNC_EN_M_MASK                                0x1
#define PMIC_XO_CTL_SYNC_EN_M_SHIFT                               6
#define PMIC_XO_LDO_MAN_ADDR                                      MT6353_DCXO_CW10
#define PMIC_XO_LDO_MAN_MASK                                      0x1
#define PMIC_XO_LDO_MAN_SHIFT                                     7
#define PMIC_XO_LDOPBUF_EN_M_ADDR                                 MT6353_DCXO_CW10
#define PMIC_XO_LDOPBUF_EN_M_MASK                                 0x1
#define PMIC_XO_LDOPBUF_EN_M_SHIFT                                8
#define PMIC_XO_LDOPBUF_VSET_M_ADDR                               MT6353_DCXO_CW10
#define PMIC_XO_LDOPBUF_VSET_M_MASK                               0xF
#define PMIC_XO_LDOPBUF_VSET_M_SHIFT                              9
#define PMIC_XO_LDOVTST_EN_M_ADDR                                 MT6353_DCXO_CW10
#define PMIC_XO_LDOVTST_EN_M_MASK                                 0x1
#define PMIC_XO_LDOVTST_EN_M_SHIFT                                13
#define PMIC_XO_TEST_VCAL_EN_M_ADDR                               MT6353_DCXO_CW10
#define PMIC_XO_TEST_VCAL_EN_M_MASK                               0x1
#define PMIC_XO_TEST_VCAL_EN_M_SHIFT                              14
#define PMIC_XO_VBIST_EN_M_ADDR                                   MT6353_DCXO_CW10
#define PMIC_XO_VBIST_EN_M_MASK                                   0x1
#define PMIC_XO_VBIST_EN_M_SHIFT                                  15
#define PMIC_XO_VTEST_SEL_MUX_ADDR                                MT6353_DCXO_CW11
#define PMIC_XO_VTEST_SEL_MUX_MASK                                0x1F
#define PMIC_XO_VTEST_SEL_MUX_SHIFT                               0
#define PMIC_RG_XO_CORE_OSCTD_ADDR                                MT6353_DCXO_CW11
#define PMIC_RG_XO_CORE_OSCTD_MASK                                0x3
#define PMIC_RG_XO_CORE_OSCTD_SHIFT                               5
#define PMIC_RG_XO_THADC_EN_ADDR                                  MT6353_DCXO_CW11
#define PMIC_RG_XO_THADC_EN_MASK                                  0x1
#define PMIC_RG_XO_THADC_EN_SHIFT                                 7
#define PMIC_RG_XO_SYNC_CKPOL_ADDR                                MT6353_DCXO_CW11
#define PMIC_RG_XO_SYNC_CKPOL_MASK                                0x1
#define PMIC_RG_XO_SYNC_CKPOL_SHIFT                               8
#define PMIC_RG_XO_CBANK_POL_ADDR                                 MT6353_DCXO_CW11
#define PMIC_RG_XO_CBANK_POL_MASK                                 0x1
#define PMIC_RG_XO_CBANK_POL_SHIFT                                9
#define PMIC_RG_XO_CBANK_SYNC_BYP_ADDR                            MT6353_DCXO_CW11
#define PMIC_RG_XO_CBANK_SYNC_BYP_MASK                            0x1
#define PMIC_RG_XO_CBANK_SYNC_BYP_SHIFT                           10
#define PMIC_RG_XO_CTL_POL_ADDR                                   MT6353_DCXO_CW11
#define PMIC_RG_XO_CTL_POL_MASK                                   0x1
#define PMIC_RG_XO_CTL_POL_SHIFT                                  11
#define PMIC_RG_XO_CTL_SYNC_BYP_ADDR                              MT6353_DCXO_CW11
#define PMIC_RG_XO_CTL_SYNC_BYP_MASK                              0x1
#define PMIC_RG_XO_CTL_SYNC_BYP_SHIFT                             12
#define PMIC_RG_XO_LPBUF_INV_ADDR                                 MT6353_DCXO_CW11
#define PMIC_RG_XO_LPBUF_INV_MASK                                 0x1
#define PMIC_RG_XO_LPBUF_INV_SHIFT                                13
#define PMIC_RG_XO_LDOPBUF_BYP_ADDR                               MT6353_DCXO_CW11
#define PMIC_RG_XO_LDOPBUF_BYP_MASK                               0x1
#define PMIC_RG_XO_LDOPBUF_BYP_SHIFT                              14
#define PMIC_RG_XO_LDOPBUF_ENCL_ADDR                              MT6353_DCXO_CW11
#define PMIC_RG_XO_LDOPBUF_ENCL_MASK                              0x1
#define PMIC_RG_XO_LDOPBUF_ENCL_SHIFT                             15
#define PMIC_RG_XO_VGBIAS_VSET_ADDR                               MT6353_DCXO_CW12
#define PMIC_RG_XO_VGBIAS_VSET_MASK                               0x3
#define PMIC_RG_XO_VGBIAS_VSET_SHIFT                              0
#define PMIC_RG_XO_PBUF_ISET_ADDR                                 MT6353_DCXO_CW12
#define PMIC_RG_XO_PBUF_ISET_MASK                                 0x3
#define PMIC_RG_XO_PBUF_ISET_SHIFT                                2
#define PMIC_RG_XO_IBUF_ISET_ADDR                                 MT6353_DCXO_CW12
#define PMIC_RG_XO_IBUF_ISET_MASK                                 0x3
#define PMIC_RG_XO_IBUF_ISET_SHIFT                                4
#define PMIC_RG_XO_BUFLDO13_ENCL_ADDR                             MT6353_DCXO_CW12
#define PMIC_RG_XO_BUFLDO13_ENCL_MASK                             0x1
#define PMIC_RG_XO_BUFLDO13_ENCL_SHIFT                            6
#define PMIC_RG_XO_BUFLDO13_IBX2_ADDR                             MT6353_DCXO_CW12
#define PMIC_RG_XO_BUFLDO13_IBX2_MASK                             0x1
#define PMIC_RG_XO_BUFLDO13_IBX2_SHIFT                            7
#define PMIC_RG_XO_BUFLDO13_IX2_ADDR                              MT6353_DCXO_CW12
#define PMIC_RG_XO_BUFLDO13_IX2_MASK                              0x1
#define PMIC_RG_XO_BUFLDO13_IX2_SHIFT                             8
#define PMIC_XO_RESERVED1_ADDR                                    MT6353_DCXO_CW12
#define PMIC_XO_RESERVED1_MASK                                    0x1
#define PMIC_XO_RESERVED1_SHIFT                                   9
#define PMIC_RG_XO_BUFLDO24_ENCL_ADDR                             MT6353_DCXO_CW12
#define PMIC_RG_XO_BUFLDO24_ENCL_MASK                             0x1
#define PMIC_RG_XO_BUFLDO24_ENCL_SHIFT                            10
#define PMIC_RG_XO_BUFLDO24_IBX2_ADDR                             MT6353_DCXO_CW12
#define PMIC_RG_XO_BUFLDO24_IBX2_MASK                             0x1
#define PMIC_RG_XO_BUFLDO24_IBX2_SHIFT                            11
#define PMIC_XO_BUFLDO24_VSET_M_ADDR                              MT6353_DCXO_CW12
#define PMIC_XO_BUFLDO24_VSET_M_MASK                              0xF
#define PMIC_XO_BUFLDO24_VSET_M_SHIFT                             12
#define PMIC_RG_XO_EXTBUF1_HD_ADDR                                MT6353_DCXO_CW13
#define PMIC_RG_XO_EXTBUF1_HD_MASK                                0x3
#define PMIC_RG_XO_EXTBUF1_HD_SHIFT                               0
#define PMIC_RG_XO_EXTBUF1_ISET_ADDR                              MT6353_DCXO_CW13
#define PMIC_RG_XO_EXTBUF1_ISET_MASK                              0x3
#define PMIC_RG_XO_EXTBUF1_ISET_SHIFT                             2
#define PMIC_RG_XO_EXTBUF2_HD_ADDR                                MT6353_DCXO_CW13
#define PMIC_RG_XO_EXTBUF2_HD_MASK                                0x3
#define PMIC_RG_XO_EXTBUF2_HD_SHIFT                               4
#define PMIC_RG_XO_EXTBUF2_ISET_ADDR                              MT6353_DCXO_CW13
#define PMIC_RG_XO_EXTBUF2_ISET_MASK                              0x3
#define PMIC_RG_XO_EXTBUF2_ISET_SHIFT                             6
#define PMIC_RG_XO_EXTBUF3_HD_ADDR                                MT6353_DCXO_CW13
#define PMIC_RG_XO_EXTBUF3_HD_MASK                                0x3
#define PMIC_RG_XO_EXTBUF3_HD_SHIFT                               8
#define PMIC_RG_XO_EXTBUF3_ISET_ADDR                              MT6353_DCXO_CW13
#define PMIC_RG_XO_EXTBUF3_ISET_MASK                              0x3
#define PMIC_RG_XO_EXTBUF3_ISET_SHIFT                             10
#define PMIC_RG_XO_EXTBUF4_HD_ADDR                                MT6353_DCXO_CW13
#define PMIC_RG_XO_EXTBUF4_HD_MASK                                0x3
#define PMIC_RG_XO_EXTBUF4_HD_SHIFT                               12
#define PMIC_RG_XO_EXTBUF4_ISET_ADDR                              MT6353_DCXO_CW13
#define PMIC_RG_XO_EXTBUF4_ISET_MASK                              0x3
#define PMIC_RG_XO_EXTBUF4_ISET_SHIFT                             14
#define PMIC_RG_XO_LPM_PREBUF_ISET_ADDR                           MT6353_DCXO_CW14
#define PMIC_RG_XO_LPM_PREBUF_ISET_MASK                           0x3
#define PMIC_RG_XO_LPM_PREBUF_ISET_SHIFT                          0
#define PMIC_RG_XO_AUDIO_ATTEN_ADDR                               MT6353_DCXO_CW14
#define PMIC_RG_XO_AUDIO_ATTEN_MASK                               0x3
#define PMIC_RG_XO_AUDIO_ATTEN_SHIFT                              2
#define PMIC_RG_XO_AUDIO_ISET_ADDR                                MT6353_DCXO_CW14
#define PMIC_RG_XO_AUDIO_ISET_MASK                                0x3
#define PMIC_RG_XO_AUDIO_ISET_SHIFT                               4
#define PMIC_XO_EXTBUF4_CLKSEL_MAN_ADDR                           MT6353_DCXO_CW14
#define PMIC_XO_EXTBUF4_CLKSEL_MAN_MASK                           0x1
#define PMIC_XO_EXTBUF4_CLKSEL_MAN_SHIFT                          6
#define PMIC_RG_XO_RESERVED0_ADDR                                 MT6353_DCXO_CW14
#define PMIC_RG_XO_RESERVED0_MASK                                 0x7
#define PMIC_RG_XO_RESERVED0_SHIFT                                7
#define PMIC_XO_VIO18PG_BUFEN_ADDR                                MT6353_DCXO_CW14
#define PMIC_XO_VIO18PG_BUFEN_MASK                                0x1
#define PMIC_XO_VIO18PG_BUFEN_SHIFT                               10
#define PMIC_RG_XO_RESERVED1_ADDR                                 MT6353_DCXO_CW14
#define PMIC_RG_XO_RESERVED1_MASK                                 0x7
#define PMIC_RG_XO_RESERVED1_SHIFT                                11
#define PMIC_XO_CAL_EN_MAN_ADDR                                   MT6353_DCXO_CW14
#define PMIC_XO_CAL_EN_MAN_MASK                                   0x1
#define PMIC_XO_CAL_EN_MAN_SHIFT                                  14
#define PMIC_XO_CAL_EN_M_ADDR                                     MT6353_DCXO_CW14
#define PMIC_XO_CAL_EN_M_MASK                                     0x1
#define PMIC_XO_CAL_EN_M_SHIFT                                    15
#define PMIC_XO_STATIC_AUXOUT_SEL_ADDR                            MT6353_DCXO_CW15
#define PMIC_XO_STATIC_AUXOUT_SEL_MASK                            0x3F
#define PMIC_XO_STATIC_AUXOUT_SEL_SHIFT                           0
#define PMIC_XO_AUXOUT_SEL_ADDR                                   MT6353_DCXO_CW15
#define PMIC_XO_AUXOUT_SEL_MASK                                   0x3FF
#define PMIC_XO_AUXOUT_SEL_SHIFT                                  6
#define PMIC_XO_STATIC_AUXOUT_ADDR                                MT6353_DCXO_CW16
#define PMIC_XO_STATIC_AUXOUT_MASK                                0xFFFF
#define PMIC_XO_STATIC_AUXOUT_SHIFT                               0
#define PMIC_AUXADC_ADC_OUT_CH0_ADDR                              MT6353_AUXADC_ADC0
#define PMIC_AUXADC_ADC_OUT_CH0_MASK                              0x7FFF
#define PMIC_AUXADC_ADC_OUT_CH0_SHIFT                             0
#define PMIC_AUXADC_ADC_RDY_CH0_ADDR                              MT6353_AUXADC_ADC0
#define PMIC_AUXADC_ADC_RDY_CH0_MASK                              0x1
#define PMIC_AUXADC_ADC_RDY_CH0_SHIFT                             15
#define PMIC_AUXADC_ADC_OUT_CH1_ADDR                              MT6353_AUXADC_ADC1
#define PMIC_AUXADC_ADC_OUT_CH1_MASK                              0x7FFF
#define PMIC_AUXADC_ADC_OUT_CH1_SHIFT                             0
#define PMIC_AUXADC_ADC_RDY_CH1_ADDR                              MT6353_AUXADC_ADC1
#define PMIC_AUXADC_ADC_RDY_CH1_MASK                              0x1
#define PMIC_AUXADC_ADC_RDY_CH1_SHIFT                             15
#define PMIC_AUXADC_ADC_OUT_CH2_ADDR                              MT6353_AUXADC_ADC2
#define PMIC_AUXADC_ADC_OUT_CH2_MASK                              0xFFF
#define PMIC_AUXADC_ADC_OUT_CH2_SHIFT                             0
#define PMIC_AUXADC_ADC_RDY_CH2_ADDR                              MT6353_AUXADC_ADC2
#define PMIC_AUXADC_ADC_RDY_CH2_MASK                              0x1
#define PMIC_AUXADC_ADC_RDY_CH2_SHIFT                             15
#define PMIC_AUXADC_ADC_OUT_CH3_ADDR                              MT6353_AUXADC_ADC3
#define PMIC_AUXADC_ADC_OUT_CH3_MASK                              0xFFF
#define PMIC_AUXADC_ADC_OUT_CH3_SHIFT                             0
#define PMIC_AUXADC_ADC_RDY_CH3_ADDR                              MT6353_AUXADC_ADC3
#define PMIC_AUXADC_ADC_RDY_CH3_MASK                              0x1
#define PMIC_AUXADC_ADC_RDY_CH3_SHIFT                             15
#define PMIC_AUXADC_ADC_OUT_CH4_ADDR                              MT6353_AUXADC_ADC4
#define PMIC_AUXADC_ADC_OUT_CH4_MASK                              0xFFF
#define PMIC_AUXADC_ADC_OUT_CH4_SHIFT                             0
#define PMIC_AUXADC_ADC_RDY_CH4_ADDR                              MT6353_AUXADC_ADC4
#define PMIC_AUXADC_ADC_RDY_CH4_MASK                              0x1
#define PMIC_AUXADC_ADC_RDY_CH4_SHIFT                             15
#define PMIC_AUXADC_ADC_OUT_CH5_ADDR                              MT6353_AUXADC_ADC5
#define PMIC_AUXADC_ADC_OUT_CH5_MASK                              0xFFF
#define PMIC_AUXADC_ADC_OUT_CH5_SHIFT                             0
#define PMIC_AUXADC_ADC_RDY_CH5_ADDR                              MT6353_AUXADC_ADC5
#define PMIC_AUXADC_ADC_RDY_CH5_MASK                              0x1
#define PMIC_AUXADC_ADC_RDY_CH5_SHIFT                             15
#define PMIC_AUXADC_ADC_OUT_CH6_ADDR                              MT6353_AUXADC_ADC6
#define PMIC_AUXADC_ADC_OUT_CH6_MASK                              0xFFF
#define PMIC_AUXADC_ADC_OUT_CH6_SHIFT                             0
#define PMIC_AUXADC_ADC_RDY_CH6_ADDR                              MT6353_AUXADC_ADC6
#define PMIC_AUXADC_ADC_RDY_CH6_MASK                              0x1
#define PMIC_AUXADC_ADC_RDY_CH6_SHIFT                             15
#define PMIC_AUXADC_ADC_OUT_CH7_ADDR                              MT6353_AUXADC_ADC7
#define PMIC_AUXADC_ADC_OUT_CH7_MASK                              0x7FFF
#define PMIC_AUXADC_ADC_OUT_CH7_SHIFT                             0
#define PMIC_AUXADC_ADC_RDY_CH7_ADDR                              MT6353_AUXADC_ADC7
#define PMIC_AUXADC_ADC_RDY_CH7_MASK                              0x1
#define PMIC_AUXADC_ADC_RDY_CH7_SHIFT                             15
#define PMIC_AUXADC_ADC_OUT_CH8_ADDR                              MT6353_AUXADC_ADC8
#define PMIC_AUXADC_ADC_OUT_CH8_MASK                              0xFFF
#define PMIC_AUXADC_ADC_OUT_CH8_SHIFT                             0
#define PMIC_AUXADC_ADC_RDY_CH8_ADDR                              MT6353_AUXADC_ADC8
#define PMIC_AUXADC_ADC_RDY_CH8_MASK                              0x1
#define PMIC_AUXADC_ADC_RDY_CH8_SHIFT                             15
#define PMIC_AUXADC_ADC_OUT_CH9_ADDR                              MT6353_AUXADC_ADC9
#define PMIC_AUXADC_ADC_OUT_CH9_MASK                              0xFFF
#define PMIC_AUXADC_ADC_OUT_CH9_SHIFT                             0
#define PMIC_AUXADC_ADC_RDY_CH9_ADDR                              MT6353_AUXADC_ADC9
#define PMIC_AUXADC_ADC_RDY_CH9_MASK                              0x1
#define PMIC_AUXADC_ADC_RDY_CH9_SHIFT                             15
#define PMIC_AUXADC_ADC_OUT_CH10_ADDR                             MT6353_AUXADC_ADC10
#define PMIC_AUXADC_ADC_OUT_CH10_MASK                             0xFFF
#define PMIC_AUXADC_ADC_OUT_CH10_SHIFT                            0
#define PMIC_AUXADC_ADC_RDY_CH10_ADDR                             MT6353_AUXADC_ADC10
#define PMIC_AUXADC_ADC_RDY_CH10_MASK                             0x1
#define PMIC_AUXADC_ADC_RDY_CH10_SHIFT                            15
#define PMIC_AUXADC_ADC_OUT_CH11_ADDR                             MT6353_AUXADC_ADC11
#define PMIC_AUXADC_ADC_OUT_CH11_MASK                             0xFFF
#define PMIC_AUXADC_ADC_OUT_CH11_SHIFT                            0
#define PMIC_AUXADC_ADC_RDY_CH11_ADDR                             MT6353_AUXADC_ADC11
#define PMIC_AUXADC_ADC_RDY_CH11_MASK                             0x1
#define PMIC_AUXADC_ADC_RDY_CH11_SHIFT                            15
#define PMIC_AUXADC_ADC_OUT_CH12_15_ADDR                          MT6353_AUXADC_ADC12
#define PMIC_AUXADC_ADC_OUT_CH12_15_MASK                          0xFFF
#define PMIC_AUXADC_ADC_OUT_CH12_15_SHIFT                         0
#define PMIC_AUXADC_ADC_RDY_CH12_15_ADDR                          MT6353_AUXADC_ADC12
#define PMIC_AUXADC_ADC_RDY_CH12_15_MASK                          0x1
#define PMIC_AUXADC_ADC_RDY_CH12_15_SHIFT                         15
#define PMIC_AUXADC_ADC_OUT_THR_HW_ADDR                           MT6353_AUXADC_ADC13
#define PMIC_AUXADC_ADC_OUT_THR_HW_MASK                           0xFFF
#define PMIC_AUXADC_ADC_OUT_THR_HW_SHIFT                          0
#define PMIC_AUXADC_ADC_RDY_THR_HW_ADDR                           MT6353_AUXADC_ADC13
#define PMIC_AUXADC_ADC_RDY_THR_HW_MASK                           0x1
#define PMIC_AUXADC_ADC_RDY_THR_HW_SHIFT                          15
#define PMIC_AUXADC_ADC_OUT_LBAT_ADDR                             MT6353_AUXADC_ADC14
#define PMIC_AUXADC_ADC_OUT_LBAT_MASK                             0xFFF
#define PMIC_AUXADC_ADC_OUT_LBAT_SHIFT                            0
#define PMIC_AUXADC_ADC_RDY_LBAT_ADDR                             MT6353_AUXADC_ADC14
#define PMIC_AUXADC_ADC_RDY_LBAT_MASK                             0x1
#define PMIC_AUXADC_ADC_RDY_LBAT_SHIFT                            15
#define PMIC_AUXADC_ADC_OUT_LBAT2_ADDR                            MT6353_AUXADC_ADC15
#define PMIC_AUXADC_ADC_OUT_LBAT2_MASK                            0xFFF
#define PMIC_AUXADC_ADC_OUT_LBAT2_SHIFT                           0
#define PMIC_AUXADC_ADC_RDY_LBAT2_ADDR                            MT6353_AUXADC_ADC15
#define PMIC_AUXADC_ADC_RDY_LBAT2_MASK                            0x1
#define PMIC_AUXADC_ADC_RDY_LBAT2_SHIFT                           15
#define PMIC_AUXADC_ADC_OUT_CH7_BY_GPS_ADDR                       MT6353_AUXADC_ADC16
#define PMIC_AUXADC_ADC_OUT_CH7_BY_GPS_MASK                       0x7FFF
#define PMIC_AUXADC_ADC_OUT_CH7_BY_GPS_SHIFT                      0
#define PMIC_AUXADC_ADC_RDY_CH7_BY_GPS_ADDR                       MT6353_AUXADC_ADC16
#define PMIC_AUXADC_ADC_RDY_CH7_BY_GPS_MASK                       0x1
#define PMIC_AUXADC_ADC_RDY_CH7_BY_GPS_SHIFT                      15
#define PMIC_AUXADC_ADC_OUT_CH7_BY_MD_ADDR                        MT6353_AUXADC_ADC17
#define PMIC_AUXADC_ADC_OUT_CH7_BY_MD_MASK                        0x7FFF
#define PMIC_AUXADC_ADC_OUT_CH7_BY_MD_SHIFT                       0
#define PMIC_AUXADC_ADC_RDY_CH7_BY_MD_ADDR                        MT6353_AUXADC_ADC17
#define PMIC_AUXADC_ADC_RDY_CH7_BY_MD_MASK                        0x1
#define PMIC_AUXADC_ADC_RDY_CH7_BY_MD_SHIFT                       15
#define PMIC_AUXADC_ADC_OUT_CH7_BY_AP_ADDR                        MT6353_AUXADC_ADC18
#define PMIC_AUXADC_ADC_OUT_CH7_BY_AP_MASK                        0x7FFF
#define PMIC_AUXADC_ADC_OUT_CH7_BY_AP_SHIFT                       0
#define PMIC_AUXADC_ADC_RDY_CH7_BY_AP_ADDR                        MT6353_AUXADC_ADC18
#define PMIC_AUXADC_ADC_RDY_CH7_BY_AP_MASK                        0x1
#define PMIC_AUXADC_ADC_RDY_CH7_BY_AP_SHIFT                       15
#define PMIC_AUXADC_ADC_OUT_CH4_BY_MD_ADDR                        MT6353_AUXADC_ADC19
#define PMIC_AUXADC_ADC_OUT_CH4_BY_MD_MASK                        0xFFF
#define PMIC_AUXADC_ADC_OUT_CH4_BY_MD_SHIFT                       0
#define PMIC_AUXADC_ADC_RDY_CH4_BY_MD_ADDR                        MT6353_AUXADC_ADC19
#define PMIC_AUXADC_ADC_RDY_CH4_BY_MD_MASK                        0x1
#define PMIC_AUXADC_ADC_RDY_CH4_BY_MD_SHIFT                       15
#define PMIC_AUXADC_ADC_OUT_WAKEUP_PCHR_ADDR                      MT6353_AUXADC_ADC20
#define PMIC_AUXADC_ADC_OUT_WAKEUP_PCHR_MASK                      0x7FFF
#define PMIC_AUXADC_ADC_OUT_WAKEUP_PCHR_SHIFT                     0
#define PMIC_AUXADC_ADC_RDY_WAKEUP_PCHR_ADDR                      MT6353_AUXADC_ADC20
#define PMIC_AUXADC_ADC_RDY_WAKEUP_PCHR_MASK                      0x1
#define PMIC_AUXADC_ADC_RDY_WAKEUP_PCHR_SHIFT                     15
#define PMIC_AUXADC_ADC_OUT_WAKEUP_SWCHR_ADDR                     MT6353_AUXADC_ADC21
#define PMIC_AUXADC_ADC_OUT_WAKEUP_SWCHR_MASK                     0x7FFF
#define PMIC_AUXADC_ADC_OUT_WAKEUP_SWCHR_SHIFT                    0
#define PMIC_AUXADC_ADC_RDY_WAKEUP_SWCHR_ADDR                     MT6353_AUXADC_ADC21
#define PMIC_AUXADC_ADC_RDY_WAKEUP_SWCHR_MASK                     0x1
#define PMIC_AUXADC_ADC_RDY_WAKEUP_SWCHR_SHIFT                    15
#define PMIC_AUXADC_ADC_OUT_CH0_BY_MD_ADDR                        MT6353_AUXADC_ADC22
#define PMIC_AUXADC_ADC_OUT_CH0_BY_MD_MASK                        0x7FFF
#define PMIC_AUXADC_ADC_OUT_CH0_BY_MD_SHIFT                       0
#define PMIC_AUXADC_ADC_RDY_CH0_BY_MD_ADDR                        MT6353_AUXADC_ADC22
#define PMIC_AUXADC_ADC_RDY_CH0_BY_MD_MASK                        0x1
#define PMIC_AUXADC_ADC_RDY_CH0_BY_MD_SHIFT                       15
#define PMIC_AUXADC_ADC_OUT_CH0_BY_AP_ADDR                        MT6353_AUXADC_ADC23
#define PMIC_AUXADC_ADC_OUT_CH0_BY_AP_MASK                        0x7FFF
#define PMIC_AUXADC_ADC_OUT_CH0_BY_AP_SHIFT                       0
#define PMIC_AUXADC_ADC_RDY_CH0_BY_AP_ADDR                        MT6353_AUXADC_ADC23
#define PMIC_AUXADC_ADC_RDY_CH0_BY_AP_MASK                        0x1
#define PMIC_AUXADC_ADC_RDY_CH0_BY_AP_SHIFT                       15
#define PMIC_AUXADC_ADC_OUT_CH1_BY_MD_ADDR                        MT6353_AUXADC_ADC24
#define PMIC_AUXADC_ADC_OUT_CH1_BY_MD_MASK                        0x7FFF
#define PMIC_AUXADC_ADC_OUT_CH1_BY_MD_SHIFT                       0
#define PMIC_AUXADC_ADC_RDY_CH1_BY_MD_ADDR                        MT6353_AUXADC_ADC24
#define PMIC_AUXADC_ADC_RDY_CH1_BY_MD_MASK                        0x1
#define PMIC_AUXADC_ADC_RDY_CH1_BY_MD_SHIFT                       15
#define PMIC_AUXADC_ADC_OUT_CH1_BY_AP_ADDR                        MT6353_AUXADC_ADC25
#define PMIC_AUXADC_ADC_OUT_CH1_BY_AP_MASK                        0x7FFF
#define PMIC_AUXADC_ADC_OUT_CH1_BY_AP_SHIFT                       0
#define PMIC_AUXADC_ADC_RDY_CH1_BY_AP_ADDR                        MT6353_AUXADC_ADC25
#define PMIC_AUXADC_ADC_RDY_CH1_BY_AP_MASK                        0x1
#define PMIC_AUXADC_ADC_RDY_CH1_BY_AP_SHIFT                       15
#define PMIC_AUXADC_ADC_OUT_VISMPS0_ADDR                          MT6353_AUXADC_ADC26
#define PMIC_AUXADC_ADC_OUT_VISMPS0_MASK                          0xFFF
#define PMIC_AUXADC_ADC_OUT_VISMPS0_SHIFT                         0
#define PMIC_AUXADC_ADC_RDY_VISMPS0_ADDR                          MT6353_AUXADC_ADC26
#define PMIC_AUXADC_ADC_RDY_VISMPS0_MASK                          0x1
#define PMIC_AUXADC_ADC_RDY_VISMPS0_SHIFT                         15
#define PMIC_AUXADC_ADC_OUT_FGADC1_ADDR                           MT6353_AUXADC_ADC27
#define PMIC_AUXADC_ADC_OUT_FGADC1_MASK                           0x7FFF
#define PMIC_AUXADC_ADC_OUT_FGADC1_SHIFT                          0
#define PMIC_AUXADC_ADC_RDY_FGADC1_ADDR                           MT6353_AUXADC_ADC27
#define PMIC_AUXADC_ADC_RDY_FGADC1_MASK                           0x1
#define PMIC_AUXADC_ADC_RDY_FGADC1_SHIFT                          15
#define PMIC_AUXADC_ADC_OUT_FGADC2_ADDR                           MT6353_AUXADC_ADC28
#define PMIC_AUXADC_ADC_OUT_FGADC2_MASK                           0x7FFF
#define PMIC_AUXADC_ADC_OUT_FGADC2_SHIFT                          0
#define PMIC_AUXADC_ADC_RDY_FGADC2_ADDR                           MT6353_AUXADC_ADC28
#define PMIC_AUXADC_ADC_RDY_FGADC2_MASK                           0x1
#define PMIC_AUXADC_ADC_RDY_FGADC2_SHIFT                          15
#define PMIC_AUXADC_ADC_OUT_IMP_ADDR                              MT6353_AUXADC_ADC29
#define PMIC_AUXADC_ADC_OUT_IMP_MASK                              0x7FFF
#define PMIC_AUXADC_ADC_OUT_IMP_SHIFT                             0
#define PMIC_AUXADC_ADC_RDY_IMP_ADDR                              MT6353_AUXADC_ADC29
#define PMIC_AUXADC_ADC_RDY_IMP_MASK                              0x1
#define PMIC_AUXADC_ADC_RDY_IMP_SHIFT                             15
#define PMIC_AUXADC_ADC_OUT_IMP_AVG_ADDR                          MT6353_AUXADC_ADC30
#define PMIC_AUXADC_ADC_OUT_IMP_AVG_MASK                          0x7FFF
#define PMIC_AUXADC_ADC_OUT_IMP_AVG_SHIFT                         0
#define PMIC_AUXADC_ADC_RDY_IMP_AVG_ADDR                          MT6353_AUXADC_ADC30
#define PMIC_AUXADC_ADC_RDY_IMP_AVG_MASK                          0x1
#define PMIC_AUXADC_ADC_RDY_IMP_AVG_SHIFT                         15
#define PMIC_AUXADC_ADC_OUT_RAW_ADDR                              MT6353_AUXADC_ADC31
#define PMIC_AUXADC_ADC_OUT_RAW_MASK                              0x7FFF
#define PMIC_AUXADC_ADC_OUT_RAW_SHIFT                             0
#define PMIC_AUXADC_ADC_OUT_MDRT_ADDR                             MT6353_AUXADC_ADC32
#define PMIC_AUXADC_ADC_OUT_MDRT_MASK                             0x7FFF
#define PMIC_AUXADC_ADC_OUT_MDRT_SHIFT                            0
#define PMIC_AUXADC_ADC_RDY_MDRT_ADDR                             MT6353_AUXADC_ADC32
#define PMIC_AUXADC_ADC_RDY_MDRT_MASK                             0x1
#define PMIC_AUXADC_ADC_RDY_MDRT_SHIFT                            15
#define PMIC_AUXADC_ADC_OUT_MDBG_ADDR                             MT6353_AUXADC_ADC33
#define PMIC_AUXADC_ADC_OUT_MDBG_MASK                             0x7FFF
#define PMIC_AUXADC_ADC_OUT_MDBG_SHIFT                            0
#define PMIC_AUXADC_ADC_RDY_MDBG_ADDR                             MT6353_AUXADC_ADC33
#define PMIC_AUXADC_ADC_RDY_MDBG_MASK                             0x1
#define PMIC_AUXADC_ADC_RDY_MDBG_SHIFT                            15
#define PMIC_AUXADC_ADC_OUT_DCXO_BY_GPS_ADDR                      MT6353_AUXADC_ADC34
#define PMIC_AUXADC_ADC_OUT_DCXO_BY_GPS_MASK                      0xFFF
#define PMIC_AUXADC_ADC_OUT_DCXO_BY_GPS_SHIFT                     0
#define PMIC_AUXADC_ADC_RDY_DCXO_BY_GPS_ADDR                      MT6353_AUXADC_ADC34
#define PMIC_AUXADC_ADC_RDY_DCXO_BY_GPS_MASK                      0x1
#define PMIC_AUXADC_ADC_RDY_DCXO_BY_GPS_SHIFT                     15
#define PMIC_AUXADC_ADC_OUT_DCXO_BY_MD_ADDR                       MT6353_AUXADC_ADC35
#define PMIC_AUXADC_ADC_OUT_DCXO_BY_MD_MASK                       0xFFF
#define PMIC_AUXADC_ADC_OUT_DCXO_BY_MD_SHIFT                      0
#define PMIC_AUXADC_ADC_RDY_DCXO_BY_MD_ADDR                       MT6353_AUXADC_ADC35
#define PMIC_AUXADC_ADC_RDY_DCXO_BY_MD_MASK                       0x1
#define PMIC_AUXADC_ADC_RDY_DCXO_BY_MD_SHIFT                      15
#define PMIC_AUXADC_ADC_OUT_DCXO_BY_AP_ADDR                       MT6353_AUXADC_ADC36
#define PMIC_AUXADC_ADC_OUT_DCXO_BY_AP_MASK                       0xFFF
#define PMIC_AUXADC_ADC_OUT_DCXO_BY_AP_SHIFT                      0
#define PMIC_AUXADC_ADC_RDY_DCXO_BY_AP_ADDR                       MT6353_AUXADC_ADC36
#define PMIC_AUXADC_ADC_RDY_DCXO_BY_AP_MASK                       0x1
#define PMIC_AUXADC_ADC_RDY_DCXO_BY_AP_SHIFT                      15
#define PMIC_AUXADC_ADC_OUT_DCXO_MDRT_ADDR                        MT6353_AUXADC_ADC37
#define PMIC_AUXADC_ADC_OUT_DCXO_MDRT_MASK                        0xFFF
#define PMIC_AUXADC_ADC_OUT_DCXO_MDRT_SHIFT                       0
#define PMIC_AUXADC_ADC_RDY_DCXO_MDRT_ADDR                        MT6353_AUXADC_ADC37
#define PMIC_AUXADC_ADC_RDY_DCXO_MDRT_MASK                        0x1
#define PMIC_AUXADC_ADC_RDY_DCXO_MDRT_SHIFT                       15
#define PMIC_AUXADC_ADC_OUT_NAG_ADDR                              MT6353_AUXADC_ADC38
#define PMIC_AUXADC_ADC_OUT_NAG_MASK                              0x7FFF
#define PMIC_AUXADC_ADC_OUT_NAG_SHIFT                             0
#define PMIC_AUXADC_ADC_RDY_NAG_ADDR                              MT6353_AUXADC_ADC38
#define PMIC_AUXADC_ADC_RDY_NAG_MASK                              0x1
#define PMIC_AUXADC_ADC_RDY_NAG_SHIFT                             15
#define PMIC_AUXADC_ADC_OUT_TYPEC_H_ADDR                          MT6353_AUXADC_ADC39
#define PMIC_AUXADC_ADC_OUT_TYPEC_H_MASK                          0xFFF
#define PMIC_AUXADC_ADC_OUT_TYPEC_H_SHIFT                         0
#define PMIC_AUXADC_ADC_RDY_TYPEC_H_ADDR                          MT6353_AUXADC_ADC39
#define PMIC_AUXADC_ADC_RDY_TYPEC_H_MASK                          0x1
#define PMIC_AUXADC_ADC_RDY_TYPEC_H_SHIFT                         15
#define PMIC_AUXADC_ADC_OUT_TYPEC_L_ADDR                          MT6353_AUXADC_ADC40
#define PMIC_AUXADC_ADC_OUT_TYPEC_L_MASK                          0xFFF
#define PMIC_AUXADC_ADC_OUT_TYPEC_L_SHIFT                         0
#define PMIC_AUXADC_ADC_RDY_TYPEC_L_ADDR                          MT6353_AUXADC_ADC40
#define PMIC_AUXADC_ADC_RDY_TYPEC_L_MASK                          0x1
#define PMIC_AUXADC_ADC_RDY_TYPEC_L_SHIFT                         15
#define PMIC_AUXADC_BUF_OUT_00_ADDR                               MT6353_AUXADC_BUF0
#define PMIC_AUXADC_BUF_OUT_00_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_00_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_00_ADDR                               MT6353_AUXADC_BUF0
#define PMIC_AUXADC_BUF_RDY_00_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_00_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_01_ADDR                               MT6353_AUXADC_BUF1
#define PMIC_AUXADC_BUF_OUT_01_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_01_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_01_ADDR                               MT6353_AUXADC_BUF1
#define PMIC_AUXADC_BUF_RDY_01_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_01_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_02_ADDR                               MT6353_AUXADC_BUF2
#define PMIC_AUXADC_BUF_OUT_02_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_02_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_02_ADDR                               MT6353_AUXADC_BUF2
#define PMIC_AUXADC_BUF_RDY_02_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_02_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_03_ADDR                               MT6353_AUXADC_BUF3
#define PMIC_AUXADC_BUF_OUT_03_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_03_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_03_ADDR                               MT6353_AUXADC_BUF3
#define PMIC_AUXADC_BUF_RDY_03_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_03_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_04_ADDR                               MT6353_AUXADC_BUF4
#define PMIC_AUXADC_BUF_OUT_04_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_04_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_04_ADDR                               MT6353_AUXADC_BUF4
#define PMIC_AUXADC_BUF_RDY_04_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_04_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_05_ADDR                               MT6353_AUXADC_BUF5
#define PMIC_AUXADC_BUF_OUT_05_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_05_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_05_ADDR                               MT6353_AUXADC_BUF5
#define PMIC_AUXADC_BUF_RDY_05_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_05_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_06_ADDR                               MT6353_AUXADC_BUF6
#define PMIC_AUXADC_BUF_OUT_06_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_06_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_06_ADDR                               MT6353_AUXADC_BUF6
#define PMIC_AUXADC_BUF_RDY_06_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_06_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_07_ADDR                               MT6353_AUXADC_BUF7
#define PMIC_AUXADC_BUF_OUT_07_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_07_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_07_ADDR                               MT6353_AUXADC_BUF7
#define PMIC_AUXADC_BUF_RDY_07_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_07_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_08_ADDR                               MT6353_AUXADC_BUF8
#define PMIC_AUXADC_BUF_OUT_08_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_08_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_08_ADDR                               MT6353_AUXADC_BUF8
#define PMIC_AUXADC_BUF_RDY_08_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_08_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_09_ADDR                               MT6353_AUXADC_BUF9
#define PMIC_AUXADC_BUF_OUT_09_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_09_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_09_ADDR                               MT6353_AUXADC_BUF9
#define PMIC_AUXADC_BUF_RDY_09_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_09_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_10_ADDR                               MT6353_AUXADC_BUF10
#define PMIC_AUXADC_BUF_OUT_10_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_10_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_10_ADDR                               MT6353_AUXADC_BUF10
#define PMIC_AUXADC_BUF_RDY_10_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_10_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_11_ADDR                               MT6353_AUXADC_BUF11
#define PMIC_AUXADC_BUF_OUT_11_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_11_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_11_ADDR                               MT6353_AUXADC_BUF11
#define PMIC_AUXADC_BUF_RDY_11_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_11_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_12_ADDR                               MT6353_AUXADC_BUF12
#define PMIC_AUXADC_BUF_OUT_12_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_12_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_12_ADDR                               MT6353_AUXADC_BUF12
#define PMIC_AUXADC_BUF_RDY_12_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_12_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_13_ADDR                               MT6353_AUXADC_BUF13
#define PMIC_AUXADC_BUF_OUT_13_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_13_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_13_ADDR                               MT6353_AUXADC_BUF13
#define PMIC_AUXADC_BUF_RDY_13_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_13_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_14_ADDR                               MT6353_AUXADC_BUF14
#define PMIC_AUXADC_BUF_OUT_14_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_14_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_14_ADDR                               MT6353_AUXADC_BUF14
#define PMIC_AUXADC_BUF_RDY_14_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_14_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_15_ADDR                               MT6353_AUXADC_BUF15
#define PMIC_AUXADC_BUF_OUT_15_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_15_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_15_ADDR                               MT6353_AUXADC_BUF15
#define PMIC_AUXADC_BUF_RDY_15_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_15_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_16_ADDR                               MT6353_AUXADC_BUF16
#define PMIC_AUXADC_BUF_OUT_16_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_16_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_16_ADDR                               MT6353_AUXADC_BUF16
#define PMIC_AUXADC_BUF_RDY_16_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_16_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_17_ADDR                               MT6353_AUXADC_BUF17
#define PMIC_AUXADC_BUF_OUT_17_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_17_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_17_ADDR                               MT6353_AUXADC_BUF17
#define PMIC_AUXADC_BUF_RDY_17_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_17_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_18_ADDR                               MT6353_AUXADC_BUF18
#define PMIC_AUXADC_BUF_OUT_18_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_18_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_18_ADDR                               MT6353_AUXADC_BUF18
#define PMIC_AUXADC_BUF_RDY_18_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_18_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_19_ADDR                               MT6353_AUXADC_BUF19
#define PMIC_AUXADC_BUF_OUT_19_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_19_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_19_ADDR                               MT6353_AUXADC_BUF19
#define PMIC_AUXADC_BUF_RDY_19_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_19_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_20_ADDR                               MT6353_AUXADC_BUF20
#define PMIC_AUXADC_BUF_OUT_20_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_20_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_20_ADDR                               MT6353_AUXADC_BUF20
#define PMIC_AUXADC_BUF_RDY_20_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_20_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_21_ADDR                               MT6353_AUXADC_BUF21
#define PMIC_AUXADC_BUF_OUT_21_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_21_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_21_ADDR                               MT6353_AUXADC_BUF21
#define PMIC_AUXADC_BUF_RDY_21_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_21_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_22_ADDR                               MT6353_AUXADC_BUF22
#define PMIC_AUXADC_BUF_OUT_22_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_22_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_22_ADDR                               MT6353_AUXADC_BUF22
#define PMIC_AUXADC_BUF_RDY_22_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_22_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_23_ADDR                               MT6353_AUXADC_BUF23
#define PMIC_AUXADC_BUF_OUT_23_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_23_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_23_ADDR                               MT6353_AUXADC_BUF23
#define PMIC_AUXADC_BUF_RDY_23_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_23_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_24_ADDR                               MT6353_AUXADC_BUF24
#define PMIC_AUXADC_BUF_OUT_24_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_24_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_24_ADDR                               MT6353_AUXADC_BUF24
#define PMIC_AUXADC_BUF_RDY_24_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_24_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_25_ADDR                               MT6353_AUXADC_BUF25
#define PMIC_AUXADC_BUF_OUT_25_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_25_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_25_ADDR                               MT6353_AUXADC_BUF25
#define PMIC_AUXADC_BUF_RDY_25_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_25_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_26_ADDR                               MT6353_AUXADC_BUF26
#define PMIC_AUXADC_BUF_OUT_26_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_26_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_26_ADDR                               MT6353_AUXADC_BUF26
#define PMIC_AUXADC_BUF_RDY_26_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_26_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_27_ADDR                               MT6353_AUXADC_BUF27
#define PMIC_AUXADC_BUF_OUT_27_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_27_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_27_ADDR                               MT6353_AUXADC_BUF27
#define PMIC_AUXADC_BUF_RDY_27_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_27_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_28_ADDR                               MT6353_AUXADC_BUF28
#define PMIC_AUXADC_BUF_OUT_28_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_28_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_28_ADDR                               MT6353_AUXADC_BUF28
#define PMIC_AUXADC_BUF_RDY_28_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_28_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_29_ADDR                               MT6353_AUXADC_BUF29
#define PMIC_AUXADC_BUF_OUT_29_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_29_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_29_ADDR                               MT6353_AUXADC_BUF29
#define PMIC_AUXADC_BUF_RDY_29_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_29_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_30_ADDR                               MT6353_AUXADC_BUF30
#define PMIC_AUXADC_BUF_OUT_30_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_30_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_30_ADDR                               MT6353_AUXADC_BUF30
#define PMIC_AUXADC_BUF_RDY_30_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_30_SHIFT                              15
#define PMIC_AUXADC_BUF_OUT_31_ADDR                               MT6353_AUXADC_BUF31
#define PMIC_AUXADC_BUF_OUT_31_MASK                               0x7FFF
#define PMIC_AUXADC_BUF_OUT_31_SHIFT                              0
#define PMIC_AUXADC_BUF_RDY_31_ADDR                               MT6353_AUXADC_BUF31
#define PMIC_AUXADC_BUF_RDY_31_MASK                               0x1
#define PMIC_AUXADC_BUF_RDY_31_SHIFT                              15
#define PMIC_AUXADC_ADC_BUSY_IN_ADDR                              MT6353_AUXADC_STA0
#define PMIC_AUXADC_ADC_BUSY_IN_MASK                              0xFFF
#define PMIC_AUXADC_ADC_BUSY_IN_SHIFT                             0
#define PMIC_AUXADC_ADC_BUSY_IN_LBAT_ADDR                         MT6353_AUXADC_STA0
#define PMIC_AUXADC_ADC_BUSY_IN_LBAT_MASK                         0x1
#define PMIC_AUXADC_ADC_BUSY_IN_LBAT_SHIFT                        12
#define PMIC_AUXADC_ADC_BUSY_IN_LBAT2_ADDR                        MT6353_AUXADC_STA0
#define PMIC_AUXADC_ADC_BUSY_IN_LBAT2_MASK                        0x1
#define PMIC_AUXADC_ADC_BUSY_IN_LBAT2_SHIFT                       13
#define PMIC_AUXADC_ADC_BUSY_IN_VISMPS0_ADDR                      MT6353_AUXADC_STA0
#define PMIC_AUXADC_ADC_BUSY_IN_VISMPS0_MASK                      0x1
#define PMIC_AUXADC_ADC_BUSY_IN_VISMPS0_SHIFT                     14
#define PMIC_AUXADC_ADC_BUSY_IN_WAKEUP_ADDR                       MT6353_AUXADC_STA0
#define PMIC_AUXADC_ADC_BUSY_IN_WAKEUP_MASK                       0x1
#define PMIC_AUXADC_ADC_BUSY_IN_WAKEUP_SHIFT                      15
#define PMIC_AUXADC_ADC_BUSY_IN_DCXO_MDRT_ADDR                    MT6353_AUXADC_STA1
#define PMIC_AUXADC_ADC_BUSY_IN_DCXO_MDRT_MASK                    0x1
#define PMIC_AUXADC_ADC_BUSY_IN_DCXO_MDRT_SHIFT                   0
#define PMIC_AUXADC_ADC_BUSY_IN_DCXO_GPS_AP_ADDR                  MT6353_AUXADC_STA1
#define PMIC_AUXADC_ADC_BUSY_IN_DCXO_GPS_AP_MASK                  0x1
#define PMIC_AUXADC_ADC_BUSY_IN_DCXO_GPS_AP_SHIFT                 1
#define PMIC_AUXADC_ADC_BUSY_IN_DCXO_GPS_MD_ADDR                  MT6353_AUXADC_STA1
#define PMIC_AUXADC_ADC_BUSY_IN_DCXO_GPS_MD_MASK                  0x1
#define PMIC_AUXADC_ADC_BUSY_IN_DCXO_GPS_MD_SHIFT                 2
#define PMIC_AUXADC_ADC_BUSY_IN_DCXO_GPS_ADDR                     MT6353_AUXADC_STA1
#define PMIC_AUXADC_ADC_BUSY_IN_DCXO_GPS_MASK                     0x1
#define PMIC_AUXADC_ADC_BUSY_IN_DCXO_GPS_SHIFT                    3
#define PMIC_AUXADC_ADC_BUSY_IN_MDRT_ADDR                         MT6353_AUXADC_STA1
#define PMIC_AUXADC_ADC_BUSY_IN_MDRT_MASK                         0x1
#define PMIC_AUXADC_ADC_BUSY_IN_MDRT_SHIFT                        5
#define PMIC_AUXADC_ADC_BUSY_IN_MDBG_ADDR                         MT6353_AUXADC_STA1
#define PMIC_AUXADC_ADC_BUSY_IN_MDBG_MASK                         0x1
#define PMIC_AUXADC_ADC_BUSY_IN_MDBG_SHIFT                        6
#define PMIC_AUXADC_ADC_BUSY_IN_SHARE_ADDR                        MT6353_AUXADC_STA1
#define PMIC_AUXADC_ADC_BUSY_IN_SHARE_MASK                        0x1
#define PMIC_AUXADC_ADC_BUSY_IN_SHARE_SHIFT                       7
#define PMIC_AUXADC_ADC_BUSY_IN_IMP_ADDR                          MT6353_AUXADC_STA1
#define PMIC_AUXADC_ADC_BUSY_IN_IMP_MASK                          0x1
#define PMIC_AUXADC_ADC_BUSY_IN_IMP_SHIFT                         8
#define PMIC_AUXADC_ADC_BUSY_IN_FGADC1_ADDR                       MT6353_AUXADC_STA1
#define PMIC_AUXADC_ADC_BUSY_IN_FGADC1_MASK                       0x1
#define PMIC_AUXADC_ADC_BUSY_IN_FGADC1_SHIFT                      9
#define PMIC_AUXADC_ADC_BUSY_IN_FGADC2_ADDR                       MT6353_AUXADC_STA1
#define PMIC_AUXADC_ADC_BUSY_IN_FGADC2_MASK                       0x1
#define PMIC_AUXADC_ADC_BUSY_IN_FGADC2_SHIFT                      10
#define PMIC_AUXADC_ADC_BUSY_IN_GPS_AP_ADDR                       MT6353_AUXADC_STA1
#define PMIC_AUXADC_ADC_BUSY_IN_GPS_AP_MASK                       0x1
#define PMIC_AUXADC_ADC_BUSY_IN_GPS_AP_SHIFT                      11
#define PMIC_AUXADC_ADC_BUSY_IN_GPS_MD_ADDR                       MT6353_AUXADC_STA1
#define PMIC_AUXADC_ADC_BUSY_IN_GPS_MD_MASK                       0x1
#define PMIC_AUXADC_ADC_BUSY_IN_GPS_MD_SHIFT                      12
#define PMIC_AUXADC_ADC_BUSY_IN_GPS_ADDR                          MT6353_AUXADC_STA1
#define PMIC_AUXADC_ADC_BUSY_IN_GPS_MASK                          0x1
#define PMIC_AUXADC_ADC_BUSY_IN_GPS_SHIFT                         13
#define PMIC_AUXADC_ADC_BUSY_IN_THR_HW_ADDR                       MT6353_AUXADC_STA1
#define PMIC_AUXADC_ADC_BUSY_IN_THR_HW_MASK                       0x1
#define PMIC_AUXADC_ADC_BUSY_IN_THR_HW_SHIFT                      14
#define PMIC_AUXADC_ADC_BUSY_IN_THR_MD_ADDR                       MT6353_AUXADC_STA1
#define PMIC_AUXADC_ADC_BUSY_IN_THR_MD_MASK                       0x1
#define PMIC_AUXADC_ADC_BUSY_IN_THR_MD_SHIFT                      15
#define PMIC_AUXADC_ADC_BUSY_IN_TYPEC_H_ADDR                      MT6353_AUXADC_STA2
#define PMIC_AUXADC_ADC_BUSY_IN_TYPEC_H_MASK                      0x1
#define PMIC_AUXADC_ADC_BUSY_IN_TYPEC_H_SHIFT                     13
#define PMIC_AUXADC_ADC_BUSY_IN_TYPEC_L_ADDR                      MT6353_AUXADC_STA2
#define PMIC_AUXADC_ADC_BUSY_IN_TYPEC_L_MASK                      0x1
#define PMIC_AUXADC_ADC_BUSY_IN_TYPEC_L_SHIFT                     14
#define PMIC_AUXADC_ADC_BUSY_IN_NAG_ADDR                          MT6353_AUXADC_STA2
#define PMIC_AUXADC_ADC_BUSY_IN_NAG_MASK                          0x1
#define PMIC_AUXADC_ADC_BUSY_IN_NAG_SHIFT                         15
#define PMIC_AUXADC_RQST_CH0_ADDR                                 MT6353_AUXADC_RQST0
#define PMIC_AUXADC_RQST_CH0_MASK                                 0x1
#define PMIC_AUXADC_RQST_CH0_SHIFT                                0
#define PMIC_AUXADC_RQST_CH1_ADDR                                 MT6353_AUXADC_RQST0
#define PMIC_AUXADC_RQST_CH1_MASK                                 0x1
#define PMIC_AUXADC_RQST_CH1_SHIFT                                1
#define PMIC_AUXADC_RQST_CH2_ADDR                                 MT6353_AUXADC_RQST0
#define PMIC_AUXADC_RQST_CH2_MASK                                 0x1
#define PMIC_AUXADC_RQST_CH2_SHIFT                                2
#define PMIC_AUXADC_RQST_CH3_ADDR                                 MT6353_AUXADC_RQST0
#define PMIC_AUXADC_RQST_CH3_MASK                                 0x1
#define PMIC_AUXADC_RQST_CH3_SHIFT                                3
#define PMIC_AUXADC_RQST_CH4_ADDR                                 MT6353_AUXADC_RQST0
#define PMIC_AUXADC_RQST_CH4_MASK                                 0x1
#define PMIC_AUXADC_RQST_CH4_SHIFT                                4
#define PMIC_AUXADC_RQST_CH5_ADDR                                 MT6353_AUXADC_RQST0
#define PMIC_AUXADC_RQST_CH5_MASK                                 0x1
#define PMIC_AUXADC_RQST_CH5_SHIFT                                5
#define PMIC_AUXADC_RQST_CH6_ADDR                                 MT6353_AUXADC_RQST0
#define PMIC_AUXADC_RQST_CH6_MASK                                 0x1
#define PMIC_AUXADC_RQST_CH6_SHIFT                                6
#define PMIC_AUXADC_RQST_CH7_ADDR                                 MT6353_AUXADC_RQST0
#define PMIC_AUXADC_RQST_CH7_MASK                                 0x1
#define PMIC_AUXADC_RQST_CH7_SHIFT                                7
#define PMIC_AUXADC_RQST_CH8_ADDR                                 MT6353_AUXADC_RQST0
#define PMIC_AUXADC_RQST_CH8_MASK                                 0x1
#define PMIC_AUXADC_RQST_CH8_SHIFT                                8
#define PMIC_AUXADC_RQST_CH9_ADDR                                 MT6353_AUXADC_RQST0
#define PMIC_AUXADC_RQST_CH9_MASK                                 0x1
#define PMIC_AUXADC_RQST_CH9_SHIFT                                9
#define PMIC_AUXADC_RQST_CH10_ADDR                                MT6353_AUXADC_RQST0
#define PMIC_AUXADC_RQST_CH10_MASK                                0x1
#define PMIC_AUXADC_RQST_CH10_SHIFT                               10
#define PMIC_AUXADC_RQST_CH11_ADDR                                MT6353_AUXADC_RQST0
#define PMIC_AUXADC_RQST_CH11_MASK                                0x1
#define PMIC_AUXADC_RQST_CH11_SHIFT                               11
#define PMIC_AUXADC_RQST_CH12_ADDR                                MT6353_AUXADC_RQST0
#define PMIC_AUXADC_RQST_CH12_MASK                                0x1
#define PMIC_AUXADC_RQST_CH12_SHIFT                               12
#define PMIC_AUXADC_RQST_CH13_ADDR                                MT6353_AUXADC_RQST0
#define PMIC_AUXADC_RQST_CH13_MASK                                0x1
#define PMIC_AUXADC_RQST_CH13_SHIFT                               13
#define PMIC_AUXADC_RQST_CH14_ADDR                                MT6353_AUXADC_RQST0
#define PMIC_AUXADC_RQST_CH14_MASK                                0x1
#define PMIC_AUXADC_RQST_CH14_SHIFT                               14
#define PMIC_AUXADC_RQST_CH15_ADDR                                MT6353_AUXADC_RQST0
#define PMIC_AUXADC_RQST_CH15_MASK                                0x1
#define PMIC_AUXADC_RQST_CH15_SHIFT                               15
#define PMIC_AUXADC_RQST0_SET_ADDR                                MT6353_AUXADC_RQST0_SET
#define PMIC_AUXADC_RQST0_SET_MASK                                0xFFFF
#define PMIC_AUXADC_RQST0_SET_SHIFT                               0
#define PMIC_AUXADC_RQST0_CLR_ADDR                                MT6353_AUXADC_RQST0_CLR
#define PMIC_AUXADC_RQST0_CLR_MASK                                0xFFFF
#define PMIC_AUXADC_RQST0_CLR_SHIFT                               0
#define PMIC_AUXADC_RQST_CH0_BY_MD_ADDR                           MT6353_AUXADC_RQST1
#define PMIC_AUXADC_RQST_CH0_BY_MD_MASK                           0x1
#define PMIC_AUXADC_RQST_CH0_BY_MD_SHIFT                          0
#define PMIC_AUXADC_RQST_CH1_BY_MD_ADDR                           MT6353_AUXADC_RQST1
#define PMIC_AUXADC_RQST_CH1_BY_MD_MASK                           0x1
#define PMIC_AUXADC_RQST_CH1_BY_MD_SHIFT                          1
#define PMIC_AUXADC_RQST_RSV0_ADDR                                MT6353_AUXADC_RQST1
#define PMIC_AUXADC_RQST_RSV0_MASK                                0x3
#define PMIC_AUXADC_RQST_RSV0_SHIFT                               2
#define PMIC_AUXADC_RQST_CH4_BY_MD_ADDR                           MT6353_AUXADC_RQST1
#define PMIC_AUXADC_RQST_CH4_BY_MD_MASK                           0x1
#define PMIC_AUXADC_RQST_CH4_BY_MD_SHIFT                          4
#define PMIC_AUXADC_RQST_CH7_BY_MD_ADDR                           MT6353_AUXADC_RQST1
#define PMIC_AUXADC_RQST_CH7_BY_MD_MASK                           0x1
#define PMIC_AUXADC_RQST_CH7_BY_MD_SHIFT                          7
#define PMIC_AUXADC_RQST_CH7_BY_GPS_ADDR                          MT6353_AUXADC_RQST1
#define PMIC_AUXADC_RQST_CH7_BY_GPS_MASK                          0x1
#define PMIC_AUXADC_RQST_CH7_BY_GPS_SHIFT                         8
#define PMIC_AUXADC_RQST_DCXO_BY_MD_ADDR                          MT6353_AUXADC_RQST1
#define PMIC_AUXADC_RQST_DCXO_BY_MD_MASK                          0x1
#define PMIC_AUXADC_RQST_DCXO_BY_MD_SHIFT                         9
#define PMIC_AUXADC_RQST_DCXO_BY_GPS_ADDR                         MT6353_AUXADC_RQST1
#define PMIC_AUXADC_RQST_DCXO_BY_GPS_MASK                         0x1
#define PMIC_AUXADC_RQST_DCXO_BY_GPS_SHIFT                        10
#define PMIC_AUXADC_RQST_RSV1_ADDR                                MT6353_AUXADC_RQST1
#define PMIC_AUXADC_RQST_RSV1_MASK                                0x1F
#define PMIC_AUXADC_RQST_RSV1_SHIFT                               11
#define PMIC_AUXADC_RQST1_SET_ADDR                                MT6353_AUXADC_RQST1_SET
#define PMIC_AUXADC_RQST1_SET_MASK                                0xFFFF
#define PMIC_AUXADC_RQST1_SET_SHIFT                               0
#define PMIC_AUXADC_RQST1_CLR_ADDR                                MT6353_AUXADC_RQST1_CLR
#define PMIC_AUXADC_RQST1_CLR_MASK                                0xFFFF
#define PMIC_AUXADC_RQST1_CLR_SHIFT                               0
#define PMIC_AUXADC_CK_ON_EXTD_ADDR                               MT6353_AUXADC_CON0
#define PMIC_AUXADC_CK_ON_EXTD_MASK                               0x3F
#define PMIC_AUXADC_CK_ON_EXTD_SHIFT                              0
#define PMIC_AUXADC_SRCLKEN_SRC_SEL_ADDR                          MT6353_AUXADC_CON0
#define PMIC_AUXADC_SRCLKEN_SRC_SEL_MASK                          0x3
#define PMIC_AUXADC_SRCLKEN_SRC_SEL_SHIFT                         6
#define PMIC_AUXADC_ADC_PWDB_ADDR                                 MT6353_AUXADC_CON0
#define PMIC_AUXADC_ADC_PWDB_MASK                                 0x1
#define PMIC_AUXADC_ADC_PWDB_SHIFT                                8
#define PMIC_AUXADC_ADC_PWDB_SWCTRL_ADDR                          MT6353_AUXADC_CON0
#define PMIC_AUXADC_ADC_PWDB_SWCTRL_MASK                          0x1
#define PMIC_AUXADC_ADC_PWDB_SWCTRL_SHIFT                         9
#define PMIC_AUXADC_STRUP_CK_ON_ENB_ADDR                          MT6353_AUXADC_CON0
#define PMIC_AUXADC_STRUP_CK_ON_ENB_MASK                          0x1
#define PMIC_AUXADC_STRUP_CK_ON_ENB_SHIFT                         10
#define PMIC_AUXADC_ADC_RDY_WAKEUP_CLR_ADDR                       MT6353_AUXADC_CON0
#define PMIC_AUXADC_ADC_RDY_WAKEUP_CLR_MASK                       0x1
#define PMIC_AUXADC_ADC_RDY_WAKEUP_CLR_SHIFT                      11
#define PMIC_AUXADC_SRCLKEN_CK_EN_ADDR                            MT6353_AUXADC_CON0
#define PMIC_AUXADC_SRCLKEN_CK_EN_MASK                            0x1
#define PMIC_AUXADC_SRCLKEN_CK_EN_SHIFT                           12
#define PMIC_AUXADC_CK_AON_GPS_ADDR                               MT6353_AUXADC_CON0
#define PMIC_AUXADC_CK_AON_GPS_MASK                               0x1
#define PMIC_AUXADC_CK_AON_GPS_SHIFT                              13
#define PMIC_AUXADC_CK_AON_MD_ADDR                                MT6353_AUXADC_CON0
#define PMIC_AUXADC_CK_AON_MD_MASK                                0x1
#define PMIC_AUXADC_CK_AON_MD_SHIFT                               14
#define PMIC_AUXADC_CK_AON_ADDR                                   MT6353_AUXADC_CON0
#define PMIC_AUXADC_CK_AON_MASK                                   0x1
#define PMIC_AUXADC_CK_AON_SHIFT                                  15
#define PMIC_AUXADC_CON0_SET_ADDR                                 MT6353_AUXADC_CON0_SET
#define PMIC_AUXADC_CON0_SET_MASK                                 0xFFFF
#define PMIC_AUXADC_CON0_SET_SHIFT                                0
#define PMIC_AUXADC_CON0_CLR_ADDR                                 MT6353_AUXADC_CON0_CLR
#define PMIC_AUXADC_CON0_CLR_MASK                                 0xFFFF
#define PMIC_AUXADC_CON0_CLR_SHIFT                                0
#define PMIC_AUXADC_AVG_NUM_SMALL_ADDR                            MT6353_AUXADC_CON1
#define PMIC_AUXADC_AVG_NUM_SMALL_MASK                            0x7
#define PMIC_AUXADC_AVG_NUM_SMALL_SHIFT                           0
#define PMIC_AUXADC_AVG_NUM_LARGE_ADDR                            MT6353_AUXADC_CON1
#define PMIC_AUXADC_AVG_NUM_LARGE_MASK                            0x7
#define PMIC_AUXADC_AVG_NUM_LARGE_SHIFT                           3
#define PMIC_AUXADC_SPL_NUM_ADDR                                  MT6353_AUXADC_CON1
#define PMIC_AUXADC_SPL_NUM_MASK                                  0x3FF
#define PMIC_AUXADC_SPL_NUM_SHIFT                                 6
#define PMIC_AUXADC_AVG_NUM_SEL_ADDR                              MT6353_AUXADC_CON2
#define PMIC_AUXADC_AVG_NUM_SEL_MASK                              0xFFF
#define PMIC_AUXADC_AVG_NUM_SEL_SHIFT                             0
#define PMIC_AUXADC_AVG_NUM_SEL_SHARE_ADDR                        MT6353_AUXADC_CON2
#define PMIC_AUXADC_AVG_NUM_SEL_SHARE_MASK                        0x1
#define PMIC_AUXADC_AVG_NUM_SEL_SHARE_SHIFT                       12
#define PMIC_AUXADC_AVG_NUM_SEL_LBAT_ADDR                         MT6353_AUXADC_CON2
#define PMIC_AUXADC_AVG_NUM_SEL_LBAT_MASK                         0x1
#define PMIC_AUXADC_AVG_NUM_SEL_LBAT_SHIFT                        13
#define PMIC_AUXADC_AVG_NUM_SEL_VISMPS_ADDR                       MT6353_AUXADC_CON2
#define PMIC_AUXADC_AVG_NUM_SEL_VISMPS_MASK                       0x1
#define PMIC_AUXADC_AVG_NUM_SEL_VISMPS_SHIFT                      14
#define PMIC_AUXADC_AVG_NUM_SEL_WAKEUP_ADDR                       MT6353_AUXADC_CON2
#define PMIC_AUXADC_AVG_NUM_SEL_WAKEUP_MASK                       0x1
#define PMIC_AUXADC_AVG_NUM_SEL_WAKEUP_SHIFT                      15
#define PMIC_AUXADC_SPL_NUM_LARGE_ADDR                            MT6353_AUXADC_CON3
#define PMIC_AUXADC_SPL_NUM_LARGE_MASK                            0x3FF
#define PMIC_AUXADC_SPL_NUM_LARGE_SHIFT                           0
#define PMIC_AUXADC_SPL_NUM_SLEEP_ADDR                            MT6353_AUXADC_CON4
#define PMIC_AUXADC_SPL_NUM_SLEEP_MASK                            0x3FF
#define PMIC_AUXADC_SPL_NUM_SLEEP_SHIFT                           0
#define PMIC_AUXADC_SPL_NUM_SLEEP_SEL_ADDR                        MT6353_AUXADC_CON4
#define PMIC_AUXADC_SPL_NUM_SLEEP_SEL_MASK                        0x1
#define PMIC_AUXADC_SPL_NUM_SLEEP_SEL_SHIFT                       15
#define PMIC_AUXADC_SPL_NUM_SEL_ADDR                              MT6353_AUXADC_CON5
#define PMIC_AUXADC_SPL_NUM_SEL_MASK                              0xFFF
#define PMIC_AUXADC_SPL_NUM_SEL_SHIFT                             0
#define PMIC_AUXADC_SPL_NUM_SEL_SHARE_ADDR                        MT6353_AUXADC_CON5
#define PMIC_AUXADC_SPL_NUM_SEL_SHARE_MASK                        0x1
#define PMIC_AUXADC_SPL_NUM_SEL_SHARE_SHIFT                       12
#define PMIC_AUXADC_SPL_NUM_SEL_LBAT_ADDR                         MT6353_AUXADC_CON5
#define PMIC_AUXADC_SPL_NUM_SEL_LBAT_MASK                         0x1
#define PMIC_AUXADC_SPL_NUM_SEL_LBAT_SHIFT                        13
#define PMIC_AUXADC_SPL_NUM_SEL_VISMPS_ADDR                       MT6353_AUXADC_CON5
#define PMIC_AUXADC_SPL_NUM_SEL_VISMPS_MASK                       0x1
#define PMIC_AUXADC_SPL_NUM_SEL_VISMPS_SHIFT                      14
#define PMIC_AUXADC_SPL_NUM_SEL_WAKEUP_ADDR                       MT6353_AUXADC_CON5
#define PMIC_AUXADC_SPL_NUM_SEL_WAKEUP_MASK                       0x1
#define PMIC_AUXADC_SPL_NUM_SEL_WAKEUP_SHIFT                      15
#define PMIC_AUXADC_TRIM_CH0_SEL_ADDR                             MT6353_AUXADC_CON6
#define PMIC_AUXADC_TRIM_CH0_SEL_MASK                             0x3
#define PMIC_AUXADC_TRIM_CH0_SEL_SHIFT                            0
#define PMIC_AUXADC_TRIM_CH1_SEL_ADDR                             MT6353_AUXADC_CON6
#define PMIC_AUXADC_TRIM_CH1_SEL_MASK                             0x3
#define PMIC_AUXADC_TRIM_CH1_SEL_SHIFT                            2
#define PMIC_AUXADC_TRIM_CH2_SEL_ADDR                             MT6353_AUXADC_CON6
#define PMIC_AUXADC_TRIM_CH2_SEL_MASK                             0x3
#define PMIC_AUXADC_TRIM_CH2_SEL_SHIFT                            4
#define PMIC_AUXADC_TRIM_CH3_SEL_ADDR                             MT6353_AUXADC_CON6
#define PMIC_AUXADC_TRIM_CH3_SEL_MASK                             0x3
#define PMIC_AUXADC_TRIM_CH3_SEL_SHIFT                            6
#define PMIC_AUXADC_TRIM_CH4_SEL_ADDR                             MT6353_AUXADC_CON6
#define PMIC_AUXADC_TRIM_CH4_SEL_MASK                             0x3
#define PMIC_AUXADC_TRIM_CH4_SEL_SHIFT                            8
#define PMIC_AUXADC_TRIM_CH5_SEL_ADDR                             MT6353_AUXADC_CON6
#define PMIC_AUXADC_TRIM_CH5_SEL_MASK                             0x3
#define PMIC_AUXADC_TRIM_CH5_SEL_SHIFT                            10
#define PMIC_AUXADC_TRIM_CH6_SEL_ADDR                             MT6353_AUXADC_CON6
#define PMIC_AUXADC_TRIM_CH6_SEL_MASK                             0x3
#define PMIC_AUXADC_TRIM_CH6_SEL_SHIFT                            12
#define PMIC_AUXADC_TRIM_CH7_SEL_ADDR                             MT6353_AUXADC_CON6
#define PMIC_AUXADC_TRIM_CH7_SEL_MASK                             0x3
#define PMIC_AUXADC_TRIM_CH7_SEL_SHIFT                            14
#define PMIC_AUXADC_TRIM_CH8_SEL_ADDR                             MT6353_AUXADC_CON7
#define PMIC_AUXADC_TRIM_CH8_SEL_MASK                             0x3
#define PMIC_AUXADC_TRIM_CH8_SEL_SHIFT                            0
#define PMIC_AUXADC_TRIM_CH9_SEL_ADDR                             MT6353_AUXADC_CON7
#define PMIC_AUXADC_TRIM_CH9_SEL_MASK                             0x3
#define PMIC_AUXADC_TRIM_CH9_SEL_SHIFT                            2
#define PMIC_AUXADC_TRIM_CH10_SEL_ADDR                            MT6353_AUXADC_CON7
#define PMIC_AUXADC_TRIM_CH10_SEL_MASK                            0x3
#define PMIC_AUXADC_TRIM_CH10_SEL_SHIFT                           4
#define PMIC_AUXADC_TRIM_CH11_SEL_ADDR                            MT6353_AUXADC_CON7
#define PMIC_AUXADC_TRIM_CH11_SEL_MASK                            0x3
#define PMIC_AUXADC_TRIM_CH11_SEL_SHIFT                           6
#define PMIC_AUXADC_ADC_2S_COMP_ENB_ADDR                          MT6353_AUXADC_CON7
#define PMIC_AUXADC_ADC_2S_COMP_ENB_MASK                          0x1
#define PMIC_AUXADC_ADC_2S_COMP_ENB_SHIFT                         14
#define PMIC_AUXADC_ADC_TRIM_COMP_ADDR                            MT6353_AUXADC_CON7
#define PMIC_AUXADC_ADC_TRIM_COMP_MASK                            0x1
#define PMIC_AUXADC_ADC_TRIM_COMP_SHIFT                           15
#define PMIC_AUXADC_SW_GAIN_TRIM_ADDR                             MT6353_AUXADC_CON8
#define PMIC_AUXADC_SW_GAIN_TRIM_MASK                             0x7FFF
#define PMIC_AUXADC_SW_GAIN_TRIM_SHIFT                            0
#define PMIC_AUXADC_SW_OFFSET_TRIM_ADDR                           MT6353_AUXADC_CON9
#define PMIC_AUXADC_SW_OFFSET_TRIM_MASK                           0x7FFF
#define PMIC_AUXADC_SW_OFFSET_TRIM_SHIFT                          0
#define PMIC_AUXADC_RNG_EN_ADDR                                   MT6353_AUXADC_CON10
#define PMIC_AUXADC_RNG_EN_MASK                                   0x1
#define PMIC_AUXADC_RNG_EN_SHIFT                                  0
#define PMIC_AUXADC_DATA_REUSE_SEL_ADDR                           MT6353_AUXADC_CON10
#define PMIC_AUXADC_DATA_REUSE_SEL_MASK                           0x3
#define PMIC_AUXADC_DATA_REUSE_SEL_SHIFT                          1
#define PMIC_AUXADC_TEST_MODE_ADDR                                MT6353_AUXADC_CON10
#define PMIC_AUXADC_TEST_MODE_MASK                                0x1
#define PMIC_AUXADC_TEST_MODE_SHIFT                               3
#define PMIC_AUXADC_BIT_SEL_ADDR                                  MT6353_AUXADC_CON10
#define PMIC_AUXADC_BIT_SEL_MASK                                  0x1
#define PMIC_AUXADC_BIT_SEL_SHIFT                                 4
#define PMIC_AUXADC_START_SW_ADDR                                 MT6353_AUXADC_CON10
#define PMIC_AUXADC_START_SW_MASK                                 0x1
#define PMIC_AUXADC_START_SW_SHIFT                                5
#define PMIC_AUXADC_START_SWCTRL_ADDR                             MT6353_AUXADC_CON10
#define PMIC_AUXADC_START_SWCTRL_MASK                             0x1
#define PMIC_AUXADC_START_SWCTRL_SHIFT                            6
#define PMIC_AUXADC_TS_VBE_SEL_ADDR                               MT6353_AUXADC_CON10
#define PMIC_AUXADC_TS_VBE_SEL_MASK                               0x1
#define PMIC_AUXADC_TS_VBE_SEL_SHIFT                              7
#define PMIC_AUXADC_TS_VBE_SEL_SWCTRL_ADDR                        MT6353_AUXADC_CON10
#define PMIC_AUXADC_TS_VBE_SEL_SWCTRL_MASK                        0x1
#define PMIC_AUXADC_TS_VBE_SEL_SWCTRL_SHIFT                       8
#define PMIC_AUXADC_VBUF_EN_ADDR                                  MT6353_AUXADC_CON10
#define PMIC_AUXADC_VBUF_EN_MASK                                  0x1
#define PMIC_AUXADC_VBUF_EN_SHIFT                                 9
#define PMIC_AUXADC_VBUF_EN_SWCTRL_ADDR                           MT6353_AUXADC_CON10
#define PMIC_AUXADC_VBUF_EN_SWCTRL_MASK                           0x1
#define PMIC_AUXADC_VBUF_EN_SWCTRL_SHIFT                          10
#define PMIC_AUXADC_OUT_SEL_ADDR                                  MT6353_AUXADC_CON10
#define PMIC_AUXADC_OUT_SEL_MASK                                  0x1
#define PMIC_AUXADC_OUT_SEL_SHIFT                                 11
#define PMIC_AUXADC_DA_DAC_ADDR                                   MT6353_AUXADC_CON11
#define PMIC_AUXADC_DA_DAC_MASK                                   0xFFF
#define PMIC_AUXADC_DA_DAC_SHIFT                                  0
#define PMIC_AUXADC_DA_DAC_SWCTRL_ADDR                            MT6353_AUXADC_CON11
#define PMIC_AUXADC_DA_DAC_SWCTRL_MASK                            0x1
#define PMIC_AUXADC_DA_DAC_SWCTRL_SHIFT                           12
#define PMIC_AD_AUXADC_COMP_ADDR                                  MT6353_AUXADC_CON11
#define PMIC_AD_AUXADC_COMP_MASK                                  0x1
#define PMIC_AD_AUXADC_COMP_SHIFT                                 15
#define PMIC_RG_VBUF_EXTEN_ADDR                                   MT6353_AUXADC_CON12
#define PMIC_RG_VBUF_EXTEN_MASK                                   0x1
#define PMIC_RG_VBUF_EXTEN_SHIFT                                  0
#define PMIC_RG_VBUF_CALEN_ADDR                                   MT6353_AUXADC_CON12
#define PMIC_RG_VBUF_CALEN_MASK                                   0x1
#define PMIC_RG_VBUF_CALEN_SHIFT                                  1
#define PMIC_RG_VBUF_BYP_ADDR                                     MT6353_AUXADC_CON12
#define PMIC_RG_VBUF_BYP_MASK                                     0x1
#define PMIC_RG_VBUF_BYP_SHIFT                                    2
#define PMIC_RG_AUX_RSV_ADDR                                      MT6353_AUXADC_CON12
#define PMIC_RG_AUX_RSV_MASK                                      0xF
#define PMIC_RG_AUX_RSV_SHIFT                                     3
#define PMIC_RG_AUXADC_CALI_ADDR                                  MT6353_AUXADC_CON12
#define PMIC_RG_AUXADC_CALI_MASK                                  0xF
#define PMIC_RG_AUXADC_CALI_SHIFT                                 7
#define PMIC_RG_VBUF_EN_ADDR                                      MT6353_AUXADC_CON12
#define PMIC_RG_VBUF_EN_MASK                                      0x1
#define PMIC_RG_VBUF_EN_SHIFT                                     11
#define PMIC_RG_VBE_SEL_ADDR                                      MT6353_AUXADC_CON12
#define PMIC_RG_VBE_SEL_MASK                                      0x1
#define PMIC_RG_VBE_SEL_SHIFT                                     12
#define PMIC_AUXADC_ADCIN_VSEN_EN_ADDR                            MT6353_AUXADC_CON13
#define PMIC_AUXADC_ADCIN_VSEN_EN_MASK                            0x1
#define PMIC_AUXADC_ADCIN_VSEN_EN_SHIFT                           0
#define PMIC_AUXADC_ADCIN_VBAT_EN_ADDR                            MT6353_AUXADC_CON13
#define PMIC_AUXADC_ADCIN_VBAT_EN_MASK                            0x1
#define PMIC_AUXADC_ADCIN_VBAT_EN_SHIFT                           1
#define PMIC_AUXADC_ADCIN_VSEN_MUX_EN_ADDR                        MT6353_AUXADC_CON13
#define PMIC_AUXADC_ADCIN_VSEN_MUX_EN_MASK                        0x1
#define PMIC_AUXADC_ADCIN_VSEN_MUX_EN_SHIFT                       2
#define PMIC_AUXADC_ADCIN_VSEN_EXT_BATON_EN_ADDR                  MT6353_AUXADC_CON13
#define PMIC_AUXADC_ADCIN_VSEN_EXT_BATON_EN_MASK                  0x1
#define PMIC_AUXADC_ADCIN_VSEN_EXT_BATON_EN_SHIFT                 3
#define PMIC_AUXADC_ADCIN_CHR_EN_ADDR                             MT6353_AUXADC_CON13
#define PMIC_AUXADC_ADCIN_CHR_EN_MASK                             0x1
#define PMIC_AUXADC_ADCIN_CHR_EN_SHIFT                            4
#define PMIC_AUXADC_ADCIN_BATON_TDET_EN_ADDR                      MT6353_AUXADC_CON13
#define PMIC_AUXADC_ADCIN_BATON_TDET_EN_MASK                      0x1
#define PMIC_AUXADC_ADCIN_BATON_TDET_EN_SHIFT                     5
#define PMIC_AUXADC_ACCDET_ANASWCTRL_EN_ADDR                      MT6353_AUXADC_CON13
#define PMIC_AUXADC_ACCDET_ANASWCTRL_EN_MASK                      0x1
#define PMIC_AUXADC_ACCDET_ANASWCTRL_EN_SHIFT                     6
#define PMIC_AUXADC_DIG0_RSV0_ADDR                                MT6353_AUXADC_CON13
#define PMIC_AUXADC_DIG0_RSV0_MASK                                0xF
#define PMIC_AUXADC_DIG0_RSV0_SHIFT                               7
#define PMIC_AUXADC_CHSEL_ADDR                                    MT6353_AUXADC_CON13
#define PMIC_AUXADC_CHSEL_MASK                                    0xF
#define PMIC_AUXADC_CHSEL_SHIFT                                   11
#define PMIC_AUXADC_SWCTRL_EN_ADDR                                MT6353_AUXADC_CON13
#define PMIC_AUXADC_SWCTRL_EN_MASK                                0x1
#define PMIC_AUXADC_SWCTRL_EN_SHIFT                               15
#define PMIC_AUXADC_SOURCE_LBAT_SEL_ADDR                          MT6353_AUXADC_CON14
#define PMIC_AUXADC_SOURCE_LBAT_SEL_MASK                          0x1
#define PMIC_AUXADC_SOURCE_LBAT_SEL_SHIFT                         0
#define PMIC_AUXADC_SOURCE_LBAT2_SEL_ADDR                         MT6353_AUXADC_CON14
#define PMIC_AUXADC_SOURCE_LBAT2_SEL_MASK                         0x1
#define PMIC_AUXADC_SOURCE_LBAT2_SEL_SHIFT                        1
#define PMIC_AUXADC_DIG0_RSV2_ADDR                                MT6353_AUXADC_CON14
#define PMIC_AUXADC_DIG0_RSV2_MASK                                0x7
#define PMIC_AUXADC_DIG0_RSV2_SHIFT                               2
#define PMIC_AUXADC_DIG1_RSV2_ADDR                                MT6353_AUXADC_CON14
#define PMIC_AUXADC_DIG1_RSV2_MASK                                0xF
#define PMIC_AUXADC_DIG1_RSV2_SHIFT                               5
#define PMIC_AUXADC_DAC_EXTD_ADDR                                 MT6353_AUXADC_CON14
#define PMIC_AUXADC_DAC_EXTD_MASK                                 0xF
#define PMIC_AUXADC_DAC_EXTD_SHIFT                                11
#define PMIC_AUXADC_DAC_EXTD_EN_ADDR                              MT6353_AUXADC_CON14
#define PMIC_AUXADC_DAC_EXTD_EN_MASK                              0x1
#define PMIC_AUXADC_DAC_EXTD_EN_SHIFT                             15
#define PMIC_AUXADC_PMU_THR_PDN_SW_ADDR                           MT6353_AUXADC_CON15
#define PMIC_AUXADC_PMU_THR_PDN_SW_MASK                           0x1
#define PMIC_AUXADC_PMU_THR_PDN_SW_SHIFT                          10
#define PMIC_AUXADC_PMU_THR_PDN_SEL_ADDR                          MT6353_AUXADC_CON15
#define PMIC_AUXADC_PMU_THR_PDN_SEL_MASK                          0x1
#define PMIC_AUXADC_PMU_THR_PDN_SEL_SHIFT                         11
#define PMIC_AUXADC_PMU_THR_PDN_STATUS_ADDR                       MT6353_AUXADC_CON15
#define PMIC_AUXADC_PMU_THR_PDN_STATUS_MASK                       0x1
#define PMIC_AUXADC_PMU_THR_PDN_STATUS_SHIFT                      12
#define PMIC_AUXADC_DIG0_RSV1_ADDR                                MT6353_AUXADC_CON15
#define PMIC_AUXADC_DIG0_RSV1_MASK                                0x7
#define PMIC_AUXADC_DIG0_RSV1_SHIFT                               13
#define PMIC_AUXADC_START_SHADE_NUM_ADDR                          MT6353_AUXADC_CON16
#define PMIC_AUXADC_START_SHADE_NUM_MASK                          0x3FF
#define PMIC_AUXADC_START_SHADE_NUM_SHIFT                         0
#define PMIC_AUXADC_START_SHADE_EN_ADDR                           MT6353_AUXADC_CON16
#define PMIC_AUXADC_START_SHADE_EN_MASK                           0x1
#define PMIC_AUXADC_START_SHADE_EN_SHIFT                          14
#define PMIC_AUXADC_START_SHADE_SEL_ADDR                          MT6353_AUXADC_CON16
#define PMIC_AUXADC_START_SHADE_SEL_MASK                          0x1
#define PMIC_AUXADC_START_SHADE_SEL_SHIFT                         15
#define PMIC_AUXADC_AUTORPT_PRD_ADDR                              MT6353_AUXADC_AUTORPT0
#define PMIC_AUXADC_AUTORPT_PRD_MASK                              0x3FF
#define PMIC_AUXADC_AUTORPT_PRD_SHIFT                             0
#define PMIC_AUXADC_AUTORPT_EN_ADDR                               MT6353_AUXADC_AUTORPT0
#define PMIC_AUXADC_AUTORPT_EN_MASK                               0x1
#define PMIC_AUXADC_AUTORPT_EN_SHIFT                              15
#define PMIC_AUXADC_LBAT_DEBT_MAX_ADDR                            MT6353_AUXADC_LBAT0
#define PMIC_AUXADC_LBAT_DEBT_MAX_MASK                            0xFF
#define PMIC_AUXADC_LBAT_DEBT_MAX_SHIFT                           0
#define PMIC_AUXADC_LBAT_DEBT_MIN_ADDR                            MT6353_AUXADC_LBAT0
#define PMIC_AUXADC_LBAT_DEBT_MIN_MASK                            0xFF
#define PMIC_AUXADC_LBAT_DEBT_MIN_SHIFT                           8
#define PMIC_AUXADC_LBAT_DET_PRD_15_0_ADDR                        MT6353_AUXADC_LBAT1
#define PMIC_AUXADC_LBAT_DET_PRD_15_0_MASK                        0xFFFF
#define PMIC_AUXADC_LBAT_DET_PRD_15_0_SHIFT                       0
#define PMIC_AUXADC_LBAT_DET_PRD_19_16_ADDR                       MT6353_AUXADC_LBAT2
#define PMIC_AUXADC_LBAT_DET_PRD_19_16_MASK                       0xF
#define PMIC_AUXADC_LBAT_DET_PRD_19_16_SHIFT                      0
#define PMIC_AUXADC_LBAT_VOLT_MAX_ADDR                            MT6353_AUXADC_LBAT3
#define PMIC_AUXADC_LBAT_VOLT_MAX_MASK                            0xFFF
#define PMIC_AUXADC_LBAT_VOLT_MAX_SHIFT                           0
#define PMIC_AUXADC_LBAT_IRQ_EN_MAX_ADDR                          MT6353_AUXADC_LBAT3
#define PMIC_AUXADC_LBAT_IRQ_EN_MAX_MASK                          0x1
#define PMIC_AUXADC_LBAT_IRQ_EN_MAX_SHIFT                         12
#define PMIC_AUXADC_LBAT_EN_MAX_ADDR                              MT6353_AUXADC_LBAT3
#define PMIC_AUXADC_LBAT_EN_MAX_MASK                              0x1
#define PMIC_AUXADC_LBAT_EN_MAX_SHIFT                             13
#define PMIC_AUXADC_LBAT_MAX_IRQ_B_ADDR                           MT6353_AUXADC_LBAT3
#define PMIC_AUXADC_LBAT_MAX_IRQ_B_MASK                           0x1
#define PMIC_AUXADC_LBAT_MAX_IRQ_B_SHIFT                          15
#define PMIC_AUXADC_LBAT_VOLT_MIN_ADDR                            MT6353_AUXADC_LBAT4
#define PMIC_AUXADC_LBAT_VOLT_MIN_MASK                            0xFFF
#define PMIC_AUXADC_LBAT_VOLT_MIN_SHIFT                           0
#define PMIC_AUXADC_LBAT_IRQ_EN_MIN_ADDR                          MT6353_AUXADC_LBAT4
#define PMIC_AUXADC_LBAT_IRQ_EN_MIN_MASK                          0x1
#define PMIC_AUXADC_LBAT_IRQ_EN_MIN_SHIFT                         12
#define PMIC_AUXADC_LBAT_EN_MIN_ADDR                              MT6353_AUXADC_LBAT4
#define PMIC_AUXADC_LBAT_EN_MIN_MASK                              0x1
#define PMIC_AUXADC_LBAT_EN_MIN_SHIFT                             13
#define PMIC_AUXADC_LBAT_MIN_IRQ_B_ADDR                           MT6353_AUXADC_LBAT4
#define PMIC_AUXADC_LBAT_MIN_IRQ_B_MASK                           0x1
#define PMIC_AUXADC_LBAT_MIN_IRQ_B_SHIFT                          15
#define PMIC_AUXADC_LBAT_DEBOUNCE_COUNT_MAX_ADDR                  MT6353_AUXADC_LBAT5
#define PMIC_AUXADC_LBAT_DEBOUNCE_COUNT_MAX_MASK                  0x1FF
#define PMIC_AUXADC_LBAT_DEBOUNCE_COUNT_MAX_SHIFT                 0
#define PMIC_AUXADC_LBAT_DEBOUNCE_COUNT_MIN_ADDR                  MT6353_AUXADC_LBAT6
#define PMIC_AUXADC_LBAT_DEBOUNCE_COUNT_MIN_MASK                  0x1FF
#define PMIC_AUXADC_LBAT_DEBOUNCE_COUNT_MIN_SHIFT                 0
#define PMIC_AUXADC_ACCDET_AUTO_SPL_ADDR                          MT6353_AUXADC_ACCDET
#define PMIC_AUXADC_ACCDET_AUTO_SPL_MASK                          0x1
#define PMIC_AUXADC_ACCDET_AUTO_SPL_SHIFT                         0
#define PMIC_AUXADC_ACCDET_AUTO_RQST_CLR_ADDR                     MT6353_AUXADC_ACCDET
#define PMIC_AUXADC_ACCDET_AUTO_RQST_CLR_MASK                     0x1
#define PMIC_AUXADC_ACCDET_AUTO_RQST_CLR_SHIFT                    1
#define PMIC_AUXADC_ACCDET_DIG1_RSV0_ADDR                         MT6353_AUXADC_ACCDET
#define PMIC_AUXADC_ACCDET_DIG1_RSV0_MASK                         0x3F
#define PMIC_AUXADC_ACCDET_DIG1_RSV0_SHIFT                        2
#define PMIC_AUXADC_ACCDET_DIG0_RSV0_ADDR                         MT6353_AUXADC_ACCDET
#define PMIC_AUXADC_ACCDET_DIG0_RSV0_MASK                         0xFF
#define PMIC_AUXADC_ACCDET_DIG0_RSV0_SHIFT                        8
#define PMIC_AUXADC_THR_DEBT_MAX_ADDR                             MT6353_AUXADC_THR0
#define PMIC_AUXADC_THR_DEBT_MAX_MASK                             0xFF
#define PMIC_AUXADC_THR_DEBT_MAX_SHIFT                            0
#define PMIC_AUXADC_THR_DEBT_MIN_ADDR                             MT6353_AUXADC_THR0
#define PMIC_AUXADC_THR_DEBT_MIN_MASK                             0xFF
#define PMIC_AUXADC_THR_DEBT_MIN_SHIFT                            8
#define PMIC_AUXADC_THR_DET_PRD_15_0_ADDR                         MT6353_AUXADC_THR1
#define PMIC_AUXADC_THR_DET_PRD_15_0_MASK                         0xFFFF
#define PMIC_AUXADC_THR_DET_PRD_15_0_SHIFT                        0
#define PMIC_AUXADC_THR_DET_PRD_19_16_ADDR                        MT6353_AUXADC_THR2
#define PMIC_AUXADC_THR_DET_PRD_19_16_MASK                        0xF
#define PMIC_AUXADC_THR_DET_PRD_19_16_SHIFT                       0
#define PMIC_AUXADC_THR_VOLT_MAX_ADDR                             MT6353_AUXADC_THR3
#define PMIC_AUXADC_THR_VOLT_MAX_MASK                             0xFFF
#define PMIC_AUXADC_THR_VOLT_MAX_SHIFT                            0
#define PMIC_AUXADC_THR_IRQ_EN_MAX_ADDR                           MT6353_AUXADC_THR3
#define PMIC_AUXADC_THR_IRQ_EN_MAX_MASK                           0x1
#define PMIC_AUXADC_THR_IRQ_EN_MAX_SHIFT                          12
#define PMIC_AUXADC_THR_EN_MAX_ADDR                               MT6353_AUXADC_THR3
#define PMIC_AUXADC_THR_EN_MAX_MASK                               0x1
#define PMIC_AUXADC_THR_EN_MAX_SHIFT                              13
#define PMIC_AUXADC_THR_MAX_IRQ_B_ADDR                            MT6353_AUXADC_THR3
#define PMIC_AUXADC_THR_MAX_IRQ_B_MASK                            0x1
#define PMIC_AUXADC_THR_MAX_IRQ_B_SHIFT                           15
#define PMIC_AUXADC_THR_VOLT_MIN_ADDR                             MT6353_AUXADC_THR4
#define PMIC_AUXADC_THR_VOLT_MIN_MASK                             0xFFF
#define PMIC_AUXADC_THR_VOLT_MIN_SHIFT                            0
#define PMIC_AUXADC_THR_IRQ_EN_MIN_ADDR                           MT6353_AUXADC_THR4
#define PMIC_AUXADC_THR_IRQ_EN_MIN_MASK                           0x1
#define PMIC_AUXADC_THR_IRQ_EN_MIN_SHIFT                          12
#define PMIC_AUXADC_THR_EN_MIN_ADDR                               MT6353_AUXADC_THR4
#define PMIC_AUXADC_THR_EN_MIN_MASK                               0x1
#define PMIC_AUXADC_THR_EN_MIN_SHIFT                              13
#define PMIC_AUXADC_THR_MIN_IRQ_B_ADDR                            MT6353_AUXADC_THR4
#define PMIC_AUXADC_THR_MIN_IRQ_B_MASK                            0x1
#define PMIC_AUXADC_THR_MIN_IRQ_B_SHIFT                           15
#define PMIC_AUXADC_THR_DEBOUNCE_COUNT_MAX_ADDR                   MT6353_AUXADC_THR5
#define PMIC_AUXADC_THR_DEBOUNCE_COUNT_MAX_MASK                   0x1FF
#define PMIC_AUXADC_THR_DEBOUNCE_COUNT_MAX_SHIFT                  0
#define PMIC_AUXADC_THR_DEBOUNCE_COUNT_MIN_ADDR                   MT6353_AUXADC_THR6
#define PMIC_AUXADC_THR_DEBOUNCE_COUNT_MIN_MASK                   0x1FF
#define PMIC_AUXADC_THR_DEBOUNCE_COUNT_MIN_SHIFT                  0
#define PMIC_EFUSE_GAIN_CH4_TRIM_ADDR                             MT6353_AUXADC_EFUSE0
#define PMIC_EFUSE_GAIN_CH4_TRIM_MASK                             0xFFF
#define PMIC_EFUSE_GAIN_CH4_TRIM_SHIFT                            0
#define PMIC_EFUSE_OFFSET_CH4_TRIM_ADDR                           MT6353_AUXADC_EFUSE1
#define PMIC_EFUSE_OFFSET_CH4_TRIM_MASK                           0x7FF
#define PMIC_EFUSE_OFFSET_CH4_TRIM_SHIFT                          0
#define PMIC_EFUSE_GAIN_CH0_TRIM_ADDR                             MT6353_AUXADC_EFUSE2
#define PMIC_EFUSE_GAIN_CH0_TRIM_MASK                             0xFFF
#define PMIC_EFUSE_GAIN_CH0_TRIM_SHIFT                            0
#define PMIC_EFUSE_OFFSET_CH0_TRIM_ADDR                           MT6353_AUXADC_EFUSE3
#define PMIC_EFUSE_OFFSET_CH0_TRIM_MASK                           0x7FF
#define PMIC_EFUSE_OFFSET_CH0_TRIM_SHIFT                          0
#define PMIC_EFUSE_GAIN_CH7_TRIM_ADDR                             MT6353_AUXADC_EFUSE4
#define PMIC_EFUSE_GAIN_CH7_TRIM_MASK                             0xFFF
#define PMIC_EFUSE_GAIN_CH7_TRIM_SHIFT                            0
#define PMIC_EFUSE_OFFSET_CH7_TRIM_ADDR                           MT6353_AUXADC_EFUSE5
#define PMIC_EFUSE_OFFSET_CH7_TRIM_MASK                           0x7FF
#define PMIC_EFUSE_OFFSET_CH7_TRIM_SHIFT                          0
#define PMIC_AUXADC_FGADC_START_SW_ADDR                           MT6353_AUXADC_DBG0
#define PMIC_AUXADC_FGADC_START_SW_MASK                           0x1
#define PMIC_AUXADC_FGADC_START_SW_SHIFT                          0
#define PMIC_AUXADC_FGADC_START_SEL_ADDR                          MT6353_AUXADC_DBG0
#define PMIC_AUXADC_FGADC_START_SEL_MASK                          0x1
#define PMIC_AUXADC_FGADC_START_SEL_SHIFT                         1
#define PMIC_AUXADC_FGADC_R_SW_ADDR                               MT6353_AUXADC_DBG0
#define PMIC_AUXADC_FGADC_R_SW_MASK                               0x1
#define PMIC_AUXADC_FGADC_R_SW_SHIFT                              2
#define PMIC_AUXADC_FGADC_R_SEL_ADDR                              MT6353_AUXADC_DBG0
#define PMIC_AUXADC_FGADC_R_SEL_MASK                              0x1
#define PMIC_AUXADC_FGADC_R_SEL_SHIFT                             3
#define PMIC_AUXADC_DBG_DIG0_RSV2_ADDR                            MT6353_AUXADC_DBG0
#define PMIC_AUXADC_DBG_DIG0_RSV2_MASK                            0x3F
#define PMIC_AUXADC_DBG_DIG0_RSV2_SHIFT                           4
#define PMIC_AUXADC_DBG_DIG1_RSV2_ADDR                            MT6353_AUXADC_DBG0
#define PMIC_AUXADC_DBG_DIG1_RSV2_MASK                            0x3F
#define PMIC_AUXADC_DBG_DIG1_RSV2_SHIFT                           10
#define PMIC_AUXADC_IMPEDANCE_CNT_ADDR                            MT6353_AUXADC_IMP0
#define PMIC_AUXADC_IMPEDANCE_CNT_MASK                            0x3F
#define PMIC_AUXADC_IMPEDANCE_CNT_SHIFT                           0
#define PMIC_AUXADC_IMPEDANCE_CHSEL_ADDR                          MT6353_AUXADC_IMP0
#define PMIC_AUXADC_IMPEDANCE_CHSEL_MASK                          0x1
#define PMIC_AUXADC_IMPEDANCE_CHSEL_SHIFT                         6
#define PMIC_AUXADC_IMPEDANCE_IRQ_CLR_ADDR                        MT6353_AUXADC_IMP0
#define PMIC_AUXADC_IMPEDANCE_IRQ_CLR_MASK                        0x1
#define PMIC_AUXADC_IMPEDANCE_IRQ_CLR_SHIFT                       7
#define PMIC_AUXADC_IMPEDANCE_IRQ_STATUS_ADDR                     MT6353_AUXADC_IMP0
#define PMIC_AUXADC_IMPEDANCE_IRQ_STATUS_MASK                     0x1
#define PMIC_AUXADC_IMPEDANCE_IRQ_STATUS_SHIFT                    8
#define PMIC_AUXADC_CLR_IMP_CNT_STOP_ADDR                         MT6353_AUXADC_IMP0
#define PMIC_AUXADC_CLR_IMP_CNT_STOP_MASK                         0x1
#define PMIC_AUXADC_CLR_IMP_CNT_STOP_SHIFT                        14
#define PMIC_AUXADC_IMPEDANCE_MODE_ADDR                           MT6353_AUXADC_IMP0
#define PMIC_AUXADC_IMPEDANCE_MODE_MASK                           0x1
#define PMIC_AUXADC_IMPEDANCE_MODE_SHIFT                          15
#define PMIC_AUXADC_IMP_AUTORPT_PRD_ADDR                          MT6353_AUXADC_IMP1
#define PMIC_AUXADC_IMP_AUTORPT_PRD_MASK                          0x3FF
#define PMIC_AUXADC_IMP_AUTORPT_PRD_SHIFT                         0
#define PMIC_AUXADC_IMP_AUTORPT_EN_ADDR                           MT6353_AUXADC_IMP1
#define PMIC_AUXADC_IMP_AUTORPT_EN_MASK                           0x1
#define PMIC_AUXADC_IMP_AUTORPT_EN_SHIFT                          15
#define PMIC_AUXADC_VISMPS0_DEBT_MAX_ADDR                         MT6353_AUXADC_VISMPS0_1
#define PMIC_AUXADC_VISMPS0_DEBT_MAX_MASK                         0xFF
#define PMIC_AUXADC_VISMPS0_DEBT_MAX_SHIFT                        0
#define PMIC_AUXADC_VISMPS0_DEBT_MIN_ADDR                         MT6353_AUXADC_VISMPS0_1
#define PMIC_AUXADC_VISMPS0_DEBT_MIN_MASK                         0xFF
#define PMIC_AUXADC_VISMPS0_DEBT_MIN_SHIFT                        8
#define PMIC_AUXADC_VISMPS0_DET_PRD_15_0_ADDR                     MT6353_AUXADC_VISMPS0_2
#define PMIC_AUXADC_VISMPS0_DET_PRD_15_0_MASK                     0xFFFF
#define PMIC_AUXADC_VISMPS0_DET_PRD_15_0_SHIFT                    0
#define PMIC_AUXADC_VISMPS0_DET_PRD_19_16_ADDR                    MT6353_AUXADC_VISMPS0_3
#define PMIC_AUXADC_VISMPS0_DET_PRD_19_16_MASK                    0xF
#define PMIC_AUXADC_VISMPS0_DET_PRD_19_16_SHIFT                   0
#define PMIC_AUXADC_VISMPS0_VOLT_MAX_ADDR                         MT6353_AUXADC_VISMPS0_4
#define PMIC_AUXADC_VISMPS0_VOLT_MAX_MASK                         0xFFF
#define PMIC_AUXADC_VISMPS0_VOLT_MAX_SHIFT                        0
#define PMIC_AUXADC_VISMPS0_IRQ_EN_MAX_ADDR                       MT6353_AUXADC_VISMPS0_4
#define PMIC_AUXADC_VISMPS0_IRQ_EN_MAX_MASK                       0x1
#define PMIC_AUXADC_VISMPS0_IRQ_EN_MAX_SHIFT                      12
#define PMIC_AUXADC_VISMPS0_EN_MAX_ADDR                           MT6353_AUXADC_VISMPS0_4
#define PMIC_AUXADC_VISMPS0_EN_MAX_MASK                           0x1
#define PMIC_AUXADC_VISMPS0_EN_MAX_SHIFT                          13
#define PMIC_AUXADC_VISMPS0_MAX_IRQ_B_ADDR                        MT6353_AUXADC_VISMPS0_4
#define PMIC_AUXADC_VISMPS0_MAX_IRQ_B_MASK                        0x1
#define PMIC_AUXADC_VISMPS0_MAX_IRQ_B_SHIFT                       15
#define PMIC_AUXADC_VISMPS0_VOLT_MIN_ADDR                         MT6353_AUXADC_VISMPS0_5
#define PMIC_AUXADC_VISMPS0_VOLT_MIN_MASK                         0xFFF
#define PMIC_AUXADC_VISMPS0_VOLT_MIN_SHIFT                        0
#define PMIC_AUXADC_VISMPS0_IRQ_EN_MIN_ADDR                       MT6353_AUXADC_VISMPS0_5
#define PMIC_AUXADC_VISMPS0_IRQ_EN_MIN_MASK                       0x1
#define PMIC_AUXADC_VISMPS0_IRQ_EN_MIN_SHIFT                      12
#define PMIC_AUXADC_VISMPS0_EN_MIN_ADDR                           MT6353_AUXADC_VISMPS0_5
#define PMIC_AUXADC_VISMPS0_EN_MIN_MASK                           0x1
#define PMIC_AUXADC_VISMPS0_EN_MIN_SHIFT                          13
#define PMIC_AUXADC_VISMPS0_MIN_IRQ_B_ADDR                        MT6353_AUXADC_VISMPS0_5
#define PMIC_AUXADC_VISMPS0_MIN_IRQ_B_MASK                        0x1
#define PMIC_AUXADC_VISMPS0_MIN_IRQ_B_SHIFT                       15
#define PMIC_AUXADC_VISMPS0_DEBOUNCE_COUNT_MAX_ADDR               MT6353_AUXADC_VISMPS0_6
#define PMIC_AUXADC_VISMPS0_DEBOUNCE_COUNT_MAX_MASK               0x1FF
#define PMIC_AUXADC_VISMPS0_DEBOUNCE_COUNT_MAX_SHIFT              0
#define PMIC_AUXADC_VISMPS0_DEBOUNCE_COUNT_MIN_ADDR               MT6353_AUXADC_VISMPS0_7
#define PMIC_AUXADC_VISMPS0_DEBOUNCE_COUNT_MIN_MASK               0x1FF
#define PMIC_AUXADC_VISMPS0_DEBOUNCE_COUNT_MIN_SHIFT              0
#define PMIC_AUXADC_LBAT2_DEBT_MAX_ADDR                           MT6353_AUXADC_LBAT2_1
#define PMIC_AUXADC_LBAT2_DEBT_MAX_MASK                           0xFF
#define PMIC_AUXADC_LBAT2_DEBT_MAX_SHIFT                          0
#define PMIC_AUXADC_LBAT2_DEBT_MIN_ADDR                           MT6353_AUXADC_LBAT2_1
#define PMIC_AUXADC_LBAT2_DEBT_MIN_MASK                           0xFF
#define PMIC_AUXADC_LBAT2_DEBT_MIN_SHIFT                          8
#define PMIC_AUXADC_LBAT2_DET_PRD_15_0_ADDR                       MT6353_AUXADC_LBAT2_2
#define PMIC_AUXADC_LBAT2_DET_PRD_15_0_MASK                       0xFFFF
#define PMIC_AUXADC_LBAT2_DET_PRD_15_0_SHIFT                      0
#define PMIC_AUXADC_LBAT2_DET_PRD_19_16_ADDR                      MT6353_AUXADC_LBAT2_3
#define PMIC_AUXADC_LBAT2_DET_PRD_19_16_MASK                      0xF
#define PMIC_AUXADC_LBAT2_DET_PRD_19_16_SHIFT                     0
#define PMIC_AUXADC_LBAT2_VOLT_MAX_ADDR                           MT6353_AUXADC_LBAT2_4
#define PMIC_AUXADC_LBAT2_VOLT_MAX_MASK                           0xFFF
#define PMIC_AUXADC_LBAT2_VOLT_MAX_SHIFT                          0
#define PMIC_AUXADC_LBAT2_IRQ_EN_MAX_ADDR                         MT6353_AUXADC_LBAT2_4
#define PMIC_AUXADC_LBAT2_IRQ_EN_MAX_MASK                         0x1
#define PMIC_AUXADC_LBAT2_IRQ_EN_MAX_SHIFT                        12
#define PMIC_AUXADC_LBAT2_EN_MAX_ADDR                             MT6353_AUXADC_LBAT2_4
#define PMIC_AUXADC_LBAT2_EN_MAX_MASK                             0x1
#define PMIC_AUXADC_LBAT2_EN_MAX_SHIFT                            13
#define PMIC_AUXADC_LBAT2_MAX_IRQ_B_ADDR                          MT6353_AUXADC_LBAT2_4
#define PMIC_AUXADC_LBAT2_MAX_IRQ_B_MASK                          0x1
#define PMIC_AUXADC_LBAT2_MAX_IRQ_B_SHIFT                         15
#define PMIC_AUXADC_LBAT2_VOLT_MIN_ADDR                           MT6353_AUXADC_LBAT2_5
#define PMIC_AUXADC_LBAT2_VOLT_MIN_MASK                           0xFFF
#define PMIC_AUXADC_LBAT2_VOLT_MIN_SHIFT                          0
#define PMIC_AUXADC_LBAT2_IRQ_EN_MIN_ADDR                         MT6353_AUXADC_LBAT2_5
#define PMIC_AUXADC_LBAT2_IRQ_EN_MIN_MASK                         0x1
#define PMIC_AUXADC_LBAT2_IRQ_EN_MIN_SHIFT                        12
#define PMIC_AUXADC_LBAT2_EN_MIN_ADDR                             MT6353_AUXADC_LBAT2_5
#define PMIC_AUXADC_LBAT2_EN_MIN_MASK                             0x1
#define PMIC_AUXADC_LBAT2_EN_MIN_SHIFT                            13
#define PMIC_AUXADC_LBAT2_MIN_IRQ_B_ADDR                          MT6353_AUXADC_LBAT2_5
#define PMIC_AUXADC_LBAT2_MIN_IRQ_B_MASK                          0x1
#define PMIC_AUXADC_LBAT2_MIN_IRQ_B_SHIFT                         15
#define PMIC_AUXADC_LBAT2_DEBOUNCE_COUNT_MAX_ADDR                 MT6353_AUXADC_LBAT2_6
#define PMIC_AUXADC_LBAT2_DEBOUNCE_COUNT_MAX_MASK                 0x1FF
#define PMIC_AUXADC_LBAT2_DEBOUNCE_COUNT_MAX_SHIFT                0
#define PMIC_AUXADC_LBAT2_DEBOUNCE_COUNT_MIN_ADDR                 MT6353_AUXADC_LBAT2_7
#define PMIC_AUXADC_LBAT2_DEBOUNCE_COUNT_MIN_MASK                 0x1FF
#define PMIC_AUXADC_LBAT2_DEBOUNCE_COUNT_MIN_SHIFT                0
#define PMIC_AUXADC_MDBG_DET_PRD_ADDR                             MT6353_AUXADC_MDBG_0
#define PMIC_AUXADC_MDBG_DET_PRD_MASK                             0x3FF
#define PMIC_AUXADC_MDBG_DET_PRD_SHIFT                            0
#define PMIC_AUXADC_MDBG_DET_EN_ADDR                              MT6353_AUXADC_MDBG_0
#define PMIC_AUXADC_MDBG_DET_EN_MASK                              0x1
#define PMIC_AUXADC_MDBG_DET_EN_SHIFT                             15
#define PMIC_AUXADC_MDBG_R_PTR_ADDR                               MT6353_AUXADC_MDBG_1
#define PMIC_AUXADC_MDBG_R_PTR_MASK                               0x3F
#define PMIC_AUXADC_MDBG_R_PTR_SHIFT                              0
#define PMIC_AUXADC_MDBG_W_PTR_ADDR                               MT6353_AUXADC_MDBG_1
#define PMIC_AUXADC_MDBG_W_PTR_MASK                               0x3F
#define PMIC_AUXADC_MDBG_W_PTR_SHIFT                              8
#define PMIC_AUXADC_MDBG_BUF_LENGTH_ADDR                          MT6353_AUXADC_MDBG_2
#define PMIC_AUXADC_MDBG_BUF_LENGTH_MASK                          0x3F
#define PMIC_AUXADC_MDBG_BUF_LENGTH_SHIFT                         0
#define PMIC_AUXADC_MDRT_DET_PRD_ADDR                             MT6353_AUXADC_MDRT_0
#define PMIC_AUXADC_MDRT_DET_PRD_MASK                             0x3FF
#define PMIC_AUXADC_MDRT_DET_PRD_SHIFT                            0
#define PMIC_AUXADC_MDRT_DET_EN_ADDR                              MT6353_AUXADC_MDRT_0
#define PMIC_AUXADC_MDRT_DET_EN_MASK                              0x1
#define PMIC_AUXADC_MDRT_DET_EN_SHIFT                             15
#define PMIC_AUXADC_MDRT_DET_WKUP_START_CNT_ADDR                  MT6353_AUXADC_MDRT_1
#define PMIC_AUXADC_MDRT_DET_WKUP_START_CNT_MASK                  0xFFF
#define PMIC_AUXADC_MDRT_DET_WKUP_START_CNT_SHIFT                 0
#define PMIC_AUXADC_MDRT_DET_WKUP_START_CLR_ADDR                  MT6353_AUXADC_MDRT_1
#define PMIC_AUXADC_MDRT_DET_WKUP_START_CLR_MASK                  0x1
#define PMIC_AUXADC_MDRT_DET_WKUP_START_CLR_SHIFT                 15
#define PMIC_AUXADC_MDRT_DET_WKUP_START_ADDR                      MT6353_AUXADC_MDRT_2
#define PMIC_AUXADC_MDRT_DET_WKUP_START_MASK                      0x1
#define PMIC_AUXADC_MDRT_DET_WKUP_START_SHIFT                     0
#define PMIC_AUXADC_MDRT_DET_WKUP_START_SEL_ADDR                  MT6353_AUXADC_MDRT_2
#define PMIC_AUXADC_MDRT_DET_WKUP_START_SEL_MASK                  0x1
#define PMIC_AUXADC_MDRT_DET_WKUP_START_SEL_SHIFT                 1
#define PMIC_AUXADC_MDRT_DET_WKUP_EN_ADDR                         MT6353_AUXADC_MDRT_2
#define PMIC_AUXADC_MDRT_DET_WKUP_EN_MASK                         0x1
#define PMIC_AUXADC_MDRT_DET_WKUP_EN_SHIFT                        2
#define PMIC_AUXADC_MDRT_DET_SRCLKEN_IND_ADDR                     MT6353_AUXADC_MDRT_2
#define PMIC_AUXADC_MDRT_DET_SRCLKEN_IND_MASK                     0x1
#define PMIC_AUXADC_MDRT_DET_SRCLKEN_IND_SHIFT                    3
#define PMIC_AUXADC_DCXO_MDRT_DET_PRD_ADDR                        MT6353_AUXADC_DCXO_MDRT_0
#define PMIC_AUXADC_DCXO_MDRT_DET_PRD_MASK                        0x3FF
#define PMIC_AUXADC_DCXO_MDRT_DET_PRD_SHIFT                       0
#define PMIC_AUXADC_DCXO_MDRT_DET_EN_ADDR                         MT6353_AUXADC_DCXO_MDRT_0
#define PMIC_AUXADC_DCXO_MDRT_DET_EN_MASK                         0x1
#define PMIC_AUXADC_DCXO_MDRT_DET_EN_SHIFT                        15
#define PMIC_AUXADC_DCXO_MDRT_DET_WKUP_START_CNT_ADDR             MT6353_AUXADC_DCXO_MDRT_1
#define PMIC_AUXADC_DCXO_MDRT_DET_WKUP_START_CNT_MASK             0xFFF
#define PMIC_AUXADC_DCXO_MDRT_DET_WKUP_START_CNT_SHIFT            0
#define PMIC_AUXADC_DCXO_MDRT_DET_WKUP_START_CLR_ADDR             MT6353_AUXADC_DCXO_MDRT_1
#define PMIC_AUXADC_DCXO_MDRT_DET_WKUP_START_CLR_MASK             0x1
#define PMIC_AUXADC_DCXO_MDRT_DET_WKUP_START_CLR_SHIFT            15
#define PMIC_AUXADC_DCXO_MDRT_DET_WKUP_EN_ADDR                    MT6353_AUXADC_DCXO_MDRT_2
#define PMIC_AUXADC_DCXO_MDRT_DET_WKUP_EN_MASK                    0x1
#define PMIC_AUXADC_DCXO_MDRT_DET_WKUP_EN_SHIFT                   0
#define PMIC_AUXADC_DCXO_MDRT_DET_WKUP_START_SEL_ADDR             MT6353_AUXADC_DCXO_MDRT_2
#define PMIC_AUXADC_DCXO_MDRT_DET_WKUP_START_SEL_MASK             0x1
#define PMIC_AUXADC_DCXO_MDRT_DET_WKUP_START_SEL_SHIFT            1
#define PMIC_AUXADC_DCXO_MDRT_DET_WKUP_START_ADDR                 MT6353_AUXADC_DCXO_MDRT_2
#define PMIC_AUXADC_DCXO_MDRT_DET_WKUP_START_MASK                 0x1
#define PMIC_AUXADC_DCXO_MDRT_DET_WKUP_START_SHIFT                2
#define PMIC_AUXADC_DCXO_MDRT_DET_SRCLKEN_IND_ADDR                MT6353_AUXADC_DCXO_MDRT_2
#define PMIC_AUXADC_DCXO_MDRT_DET_SRCLKEN_IND_MASK                0x1
#define PMIC_AUXADC_DCXO_MDRT_DET_SRCLKEN_IND_SHIFT               3
#define PMIC_AUXADC_DCXO_CH4_MUX_AP_SEL_ADDR                      MT6353_AUXADC_DCXO_MDRT_2
#define PMIC_AUXADC_DCXO_CH4_MUX_AP_SEL_MASK                      0x1
#define PMIC_AUXADC_DCXO_CH4_MUX_AP_SEL_SHIFT                     4
#define PMIC_AUXADC_NAG_EN_ADDR                                   MT6353_AUXADC_NAG_0
#define PMIC_AUXADC_NAG_EN_MASK                                   0x1
#define PMIC_AUXADC_NAG_EN_SHIFT                                  0
#define PMIC_AUXADC_NAG_CLR_ADDR                                  MT6353_AUXADC_NAG_0
#define PMIC_AUXADC_NAG_CLR_MASK                                  0x1
#define PMIC_AUXADC_NAG_CLR_SHIFT                                 1
#define PMIC_AUXADC_NAG_VBAT1_SEL_ADDR                            MT6353_AUXADC_NAG_0
#define PMIC_AUXADC_NAG_VBAT1_SEL_MASK                            0x1
#define PMIC_AUXADC_NAG_VBAT1_SEL_SHIFT                           2
#define PMIC_AUXADC_NAG_PRD_ADDR                                  MT6353_AUXADC_NAG_0
#define PMIC_AUXADC_NAG_PRD_MASK                                  0x7F
#define PMIC_AUXADC_NAG_PRD_SHIFT                                 3
#define PMIC_AUXADC_NAG_IRQ_EN_ADDR                               MT6353_AUXADC_NAG_0
#define PMIC_AUXADC_NAG_IRQ_EN_MASK                               0x1
#define PMIC_AUXADC_NAG_IRQ_EN_SHIFT                              10
#define PMIC_AUXADC_NAG_C_DLTV_IRQ_ADDR                           MT6353_AUXADC_NAG_0
#define PMIC_AUXADC_NAG_C_DLTV_IRQ_MASK                           0x1
#define PMIC_AUXADC_NAG_C_DLTV_IRQ_SHIFT                          15
#define PMIC_AUXADC_NAG_ZCV_ADDR                                  MT6353_AUXADC_NAG_1
#define PMIC_AUXADC_NAG_ZCV_MASK                                  0x7FFF
#define PMIC_AUXADC_NAG_ZCV_SHIFT                                 0
#define PMIC_AUXADC_NAG_C_DLTV_TH_15_0_ADDR                       MT6353_AUXADC_NAG_2
#define PMIC_AUXADC_NAG_C_DLTV_TH_15_0_MASK                       0xFFFF
#define PMIC_AUXADC_NAG_C_DLTV_TH_15_0_SHIFT                      0
#define PMIC_AUXADC_NAG_C_DLTV_TH_26_16_ADDR                      MT6353_AUXADC_NAG_3
#define PMIC_AUXADC_NAG_C_DLTV_TH_26_16_MASK                      0x7FF
#define PMIC_AUXADC_NAG_C_DLTV_TH_26_16_SHIFT                     0
#define PMIC_AUXADC_NAG_CNT_15_0_ADDR                             MT6353_AUXADC_NAG_4
#define PMIC_AUXADC_NAG_CNT_15_0_MASK                             0xFFFF
#define PMIC_AUXADC_NAG_CNT_15_0_SHIFT                            0
#define PMIC_AUXADC_NAG_CNT_25_16_ADDR                            MT6353_AUXADC_NAG_5
#define PMIC_AUXADC_NAG_CNT_25_16_MASK                            0x3FF
#define PMIC_AUXADC_NAG_CNT_25_16_SHIFT                           0
#define PMIC_AUXADC_NAG_DLTV_ADDR                                 MT6353_AUXADC_NAG_6
#define PMIC_AUXADC_NAG_DLTV_MASK                                 0xFFFF
#define PMIC_AUXADC_NAG_DLTV_SHIFT                                0
#define PMIC_AUXADC_NAG_C_DLTV_15_0_ADDR                          MT6353_AUXADC_NAG_7
#define PMIC_AUXADC_NAG_C_DLTV_15_0_MASK                          0xFFFF
#define PMIC_AUXADC_NAG_C_DLTV_15_0_SHIFT                         0
#define PMIC_AUXADC_NAG_C_DLTV_26_16_ADDR                         MT6353_AUXADC_NAG_8
#define PMIC_AUXADC_NAG_C_DLTV_26_16_MASK                         0x7FF
#define PMIC_AUXADC_NAG_C_DLTV_26_16_SHIFT                        0
#define PMIC_AUXADC_TYPEC_H_DEBT_MAX_ADDR                         MT6353_AUXADC_TYPEC_H_1
#define PMIC_AUXADC_TYPEC_H_DEBT_MAX_MASK                         0xFF
#define PMIC_AUXADC_TYPEC_H_DEBT_MAX_SHIFT                        0
#define PMIC_AUXADC_TYPEC_H_DEBT_MIN_ADDR                         MT6353_AUXADC_TYPEC_H_1
#define PMIC_AUXADC_TYPEC_H_DEBT_MIN_MASK                         0xFF
#define PMIC_AUXADC_TYPEC_H_DEBT_MIN_SHIFT                        8
#define PMIC_AUXADC_TYPEC_H_DET_PRD_15_0_ADDR                     MT6353_AUXADC_TYPEC_H_2
#define PMIC_AUXADC_TYPEC_H_DET_PRD_15_0_MASK                     0xFFFF
#define PMIC_AUXADC_TYPEC_H_DET_PRD_15_0_SHIFT                    0
#define PMIC_AUXADC_TYPEC_H_DET_PRD_19_16_ADDR                    MT6353_AUXADC_TYPEC_H_3
#define PMIC_AUXADC_TYPEC_H_DET_PRD_19_16_MASK                    0xF
#define PMIC_AUXADC_TYPEC_H_DET_PRD_19_16_SHIFT                   0
#define PMIC_AUXADC_TYPEC_H_VOLT_MAX_ADDR                         MT6353_AUXADC_TYPEC_H_4
#define PMIC_AUXADC_TYPEC_H_VOLT_MAX_MASK                         0xFFF
#define PMIC_AUXADC_TYPEC_H_VOLT_MAX_SHIFT                        0
#define PMIC_AUXADC_TYPEC_H_IRQ_EN_MAX_ADDR                       MT6353_AUXADC_TYPEC_H_4
#define PMIC_AUXADC_TYPEC_H_IRQ_EN_MAX_MASK                       0x1
#define PMIC_AUXADC_TYPEC_H_IRQ_EN_MAX_SHIFT                      12
#define PMIC_AUXADC_TYPEC_H_EN_MAX_ADDR                           MT6353_AUXADC_TYPEC_H_4
#define PMIC_AUXADC_TYPEC_H_EN_MAX_MASK                           0x1
#define PMIC_AUXADC_TYPEC_H_EN_MAX_SHIFT                          13
#define PMIC_AUXADC_TYPEC_H_MAX_IRQ_B_ADDR                        MT6353_AUXADC_TYPEC_H_4
#define PMIC_AUXADC_TYPEC_H_MAX_IRQ_B_MASK                        0x1
#define PMIC_AUXADC_TYPEC_H_MAX_IRQ_B_SHIFT                       15
#define PMIC_AUXADC_TYPEC_H_VOLT_MIN_ADDR                         MT6353_AUXADC_TYPEC_H_5
#define PMIC_AUXADC_TYPEC_H_VOLT_MIN_MASK                         0xFFF
#define PMIC_AUXADC_TYPEC_H_VOLT_MIN_SHIFT                        0
#define PMIC_AUXADC_TYPEC_H_IRQ_EN_MIN_ADDR                       MT6353_AUXADC_TYPEC_H_5
#define PMIC_AUXADC_TYPEC_H_IRQ_EN_MIN_MASK                       0x1
#define PMIC_AUXADC_TYPEC_H_IRQ_EN_MIN_SHIFT                      12
#define PMIC_AUXADC_TYPEC_H_EN_MIN_ADDR                           MT6353_AUXADC_TYPEC_H_5
#define PMIC_AUXADC_TYPEC_H_EN_MIN_MASK                           0x1
#define PMIC_AUXADC_TYPEC_H_EN_MIN_SHIFT                          13
#define PMIC_AUXADC_TYPEC_H_MIN_IRQ_B_ADDR                        MT6353_AUXADC_TYPEC_H_5
#define PMIC_AUXADC_TYPEC_H_MIN_IRQ_B_MASK                        0x1
#define PMIC_AUXADC_TYPEC_H_MIN_IRQ_B_SHIFT                       15
#define PMIC_AUXADC_TYPEC_H_DEBOUNCE_COUNT_MAX_ADDR               MT6353_AUXADC_TYPEC_H_6
#define PMIC_AUXADC_TYPEC_H_DEBOUNCE_COUNT_MAX_MASK               0x1FF
#define PMIC_AUXADC_TYPEC_H_DEBOUNCE_COUNT_MAX_SHIFT              0
#define PMIC_AUXADC_TYPEC_H_DEBOUNCE_COUNT_MIN_ADDR               MT6353_AUXADC_TYPEC_H_7
#define PMIC_AUXADC_TYPEC_H_DEBOUNCE_COUNT_MIN_MASK               0x1FF
#define PMIC_AUXADC_TYPEC_H_DEBOUNCE_COUNT_MIN_SHIFT              0
#define PMIC_AUXADC_TYPEC_L_DEBT_MAX_ADDR                         MT6353_AUXADC_TYPEC_L_1
#define PMIC_AUXADC_TYPEC_L_DEBT_MAX_MASK                         0xFF
#define PMIC_AUXADC_TYPEC_L_DEBT_MAX_SHIFT                        0
#define PMIC_AUXADC_TYPEC_L_DEBT_MIN_ADDR                         MT6353_AUXADC_TYPEC_L_1
#define PMIC_AUXADC_TYPEC_L_DEBT_MIN_MASK                         0xFF
#define PMIC_AUXADC_TYPEC_L_DEBT_MIN_SHIFT                        8
#define PMIC_AUXADC_TYPEC_L_DET_PRD_15_0_ADDR                     MT6353_AUXADC_TYPEC_L_2
#define PMIC_AUXADC_TYPEC_L_DET_PRD_15_0_MASK                     0xFFFF
#define PMIC_AUXADC_TYPEC_L_DET_PRD_15_0_SHIFT                    0
#define PMIC_AUXADC_TYPEC_L_DET_PRD_19_16_ADDR                    MT6353_AUXADC_TYPEC_L_3
#define PMIC_AUXADC_TYPEC_L_DET_PRD_19_16_MASK                    0xF
#define PMIC_AUXADC_TYPEC_L_DET_PRD_19_16_SHIFT                   0
#define PMIC_AUXADC_TYPEC_L_VOLT_MAX_ADDR                         MT6353_AUXADC_TYPEC_L_4
#define PMIC_AUXADC_TYPEC_L_VOLT_MAX_MASK                         0xFFF
#define PMIC_AUXADC_TYPEC_L_VOLT_MAX_SHIFT                        0
#define PMIC_AUXADC_TYPEC_L_IRQ_EN_MAX_ADDR                       MT6353_AUXADC_TYPEC_L_4
#define PMIC_AUXADC_TYPEC_L_IRQ_EN_MAX_MASK                       0x1
#define PMIC_AUXADC_TYPEC_L_IRQ_EN_MAX_SHIFT                      12
#define PMIC_AUXADC_TYPEC_L_EN_MAX_ADDR                           MT6353_AUXADC_TYPEC_L_4
#define PMIC_AUXADC_TYPEC_L_EN_MAX_MASK                           0x1
#define PMIC_AUXADC_TYPEC_L_EN_MAX_SHIFT                          13
#define PMIC_AUXADC_TYPEC_L_MAX_IRQ_B_ADDR                        MT6353_AUXADC_TYPEC_L_4
#define PMIC_AUXADC_TYPEC_L_MAX_IRQ_B_MASK                        0x1
#define PMIC_AUXADC_TYPEC_L_MAX_IRQ_B_SHIFT                       15
#define PMIC_AUXADC_TYPEC_L_VOLT_MIN_ADDR                         MT6353_AUXADC_TYPEC_L_5
#define PMIC_AUXADC_TYPEC_L_VOLT_MIN_MASK                         0xFFF
#define PMIC_AUXADC_TYPEC_L_VOLT_MIN_SHIFT                        0
#define PMIC_AUXADC_TYPEC_L_IRQ_EN_MIN_ADDR                       MT6353_AUXADC_TYPEC_L_5
#define PMIC_AUXADC_TYPEC_L_IRQ_EN_MIN_MASK                       0x1
#define PMIC_AUXADC_TYPEC_L_IRQ_EN_MIN_SHIFT                      12
#define PMIC_AUXADC_TYPEC_L_EN_MIN_ADDR                           MT6353_AUXADC_TYPEC_L_5
#define PMIC_AUXADC_TYPEC_L_EN_MIN_MASK                           0x1
#define PMIC_AUXADC_TYPEC_L_EN_MIN_SHIFT                          13
#define PMIC_AUXADC_TYPEC_L_MIN_IRQ_B_ADDR                        MT6353_AUXADC_TYPEC_L_5
#define PMIC_AUXADC_TYPEC_L_MIN_IRQ_B_MASK                        0x1
#define PMIC_AUXADC_TYPEC_L_MIN_IRQ_B_SHIFT                       15
#define PMIC_AUXADC_TYPEC_L_DEBOUNCE_COUNT_MAX_ADDR               MT6353_AUXADC_TYPEC_L_6
#define PMIC_AUXADC_TYPEC_L_DEBOUNCE_COUNT_MAX_MASK               0x1FF
#define PMIC_AUXADC_TYPEC_L_DEBOUNCE_COUNT_MAX_SHIFT              0
#define PMIC_AUXADC_TYPEC_L_DEBOUNCE_COUNT_MIN_ADDR               MT6353_AUXADC_TYPEC_L_7
#define PMIC_AUXADC_TYPEC_L_DEBOUNCE_COUNT_MIN_MASK               0x1FF
#define PMIC_AUXADC_TYPEC_L_DEBOUNCE_COUNT_MIN_SHIFT              0
#define PMIC_RG_AUDACCDETVTHBCAL_ADDR                             MT6353_ACCDET_CON0
#define PMIC_RG_AUDACCDETVTHBCAL_MASK                             0x1
#define PMIC_RG_AUDACCDETVTHBCAL_SHIFT                            0
#define PMIC_RG_AUDACCDETVTHACAL_ADDR                             MT6353_ACCDET_CON0
#define PMIC_RG_AUDACCDETVTHACAL_MASK                             0x1
#define PMIC_RG_AUDACCDETVTHACAL_SHIFT                            1
#define PMIC_RG_AUDACCDETANASWCTRLENB_ADDR                        MT6353_ACCDET_CON0
#define PMIC_RG_AUDACCDETANASWCTRLENB_MASK                        0x1
#define PMIC_RG_AUDACCDETANASWCTRLENB_SHIFT                       2
#define PMIC_RG_ACCDETSEL_ADDR                                    MT6353_ACCDET_CON0
#define PMIC_RG_ACCDETSEL_MASK                                    0x1
#define PMIC_RG_ACCDETSEL_SHIFT                                   3
#define PMIC_RG_AUDACCDETSWCTRL_ADDR                              MT6353_ACCDET_CON0
#define PMIC_RG_AUDACCDETSWCTRL_MASK                              0x7
#define PMIC_RG_AUDACCDETSWCTRL_SHIFT                             4
#define PMIC_RG_AUDACCDETMICBIAS1PULLLOW_ADDR                     MT6353_ACCDET_CON0
#define PMIC_RG_AUDACCDETMICBIAS1PULLLOW_MASK                     0x1
#define PMIC_RG_AUDACCDETMICBIAS1PULLLOW_SHIFT                    7
#define PMIC_RG_AUDACCDETTVDET_ADDR                               MT6353_ACCDET_CON0
#define PMIC_RG_AUDACCDETTVDET_MASK                               0x1
#define PMIC_RG_AUDACCDETTVDET_SHIFT                              8
#define PMIC_RG_AUDACCDETVIN1PULLLOW_ADDR                         MT6353_ACCDET_CON0
#define PMIC_RG_AUDACCDETVIN1PULLLOW_MASK                         0x1
#define PMIC_RG_AUDACCDETVIN1PULLLOW_SHIFT                        9
#define PMIC_AUDACCDETAUXADCSWCTRL_ADDR                           MT6353_ACCDET_CON0
#define PMIC_AUDACCDETAUXADCSWCTRL_MASK                           0x1
#define PMIC_AUDACCDETAUXADCSWCTRL_SHIFT                          10
#define PMIC_AUDACCDETAUXADCSWCTRL_SEL_ADDR                       MT6353_ACCDET_CON0
#define PMIC_AUDACCDETAUXADCSWCTRL_SEL_MASK                       0x1
#define PMIC_AUDACCDETAUXADCSWCTRL_SEL_SHIFT                      11
#define PMIC_RG_AUDACCDETMICBIAS0PULLLOW_ADDR                     MT6353_ACCDET_CON0
#define PMIC_RG_AUDACCDETMICBIAS0PULLLOW_MASK                     0x1
#define PMIC_RG_AUDACCDETMICBIAS0PULLLOW_SHIFT                    12
#define PMIC_RG_AUDACCDETRSV_ADDR                                 MT6353_ACCDET_CON0
#define PMIC_RG_AUDACCDETRSV_MASK                                 0x3
#define PMIC_RG_AUDACCDETRSV_SHIFT                                13
#define PMIC_ACCDET_EN_ADDR                                       MT6353_ACCDET_CON1
#define PMIC_ACCDET_EN_MASK                                       0x1
#define PMIC_ACCDET_EN_SHIFT                                      0
#define PMIC_ACCDET_SEQ_INIT_ADDR                                 MT6353_ACCDET_CON1
#define PMIC_ACCDET_SEQ_INIT_MASK                                 0x1
#define PMIC_ACCDET_SEQ_INIT_SHIFT                                1
#define PMIC_ACCDET_EINTDET_EN_ADDR                               MT6353_ACCDET_CON1
#define PMIC_ACCDET_EINTDET_EN_MASK                               0x1
#define PMIC_ACCDET_EINTDET_EN_SHIFT                              2
#define PMIC_ACCDET_EINT_SEQ_INIT_ADDR                            MT6353_ACCDET_CON1
#define PMIC_ACCDET_EINT_SEQ_INIT_MASK                            0x1
#define PMIC_ACCDET_EINT_SEQ_INIT_SHIFT                           3
#define PMIC_ACCDET_NEGVDET_EN_ADDR                               MT6353_ACCDET_CON1
#define PMIC_ACCDET_NEGVDET_EN_MASK                               0x1
#define PMIC_ACCDET_NEGVDET_EN_SHIFT                              4
#define PMIC_ACCDET_NEGVDET_EN_CTRL_ADDR                          MT6353_ACCDET_CON1
#define PMIC_ACCDET_NEGVDET_EN_CTRL_MASK                          0x1
#define PMIC_ACCDET_NEGVDET_EN_CTRL_SHIFT                         5
#define PMIC_ACCDET_ANASWCTRL_SEL_ADDR                            MT6353_ACCDET_CON1
#define PMIC_ACCDET_ANASWCTRL_SEL_MASK                            0x1
#define PMIC_ACCDET_ANASWCTRL_SEL_SHIFT                           6
#define PMIC_ACCDET_CMP_PWM_EN_ADDR                               MT6353_ACCDET_CON2
#define PMIC_ACCDET_CMP_PWM_EN_MASK                               0x1
#define PMIC_ACCDET_CMP_PWM_EN_SHIFT                              0
#define PMIC_ACCDET_VTH_PWM_EN_ADDR                               MT6353_ACCDET_CON2
#define PMIC_ACCDET_VTH_PWM_EN_MASK                               0x1
#define PMIC_ACCDET_VTH_PWM_EN_SHIFT                              1
#define PMIC_ACCDET_MBIAS_PWM_EN_ADDR                             MT6353_ACCDET_CON2
#define PMIC_ACCDET_MBIAS_PWM_EN_MASK                             0x1
#define PMIC_ACCDET_MBIAS_PWM_EN_SHIFT                            2
#define PMIC_ACCDET_EINT_PWM_EN_ADDR                              MT6353_ACCDET_CON2
#define PMIC_ACCDET_EINT_PWM_EN_MASK                              0x1
#define PMIC_ACCDET_EINT_PWM_EN_SHIFT                             3
#define PMIC_ACCDET_CMP_PWM_IDLE_ADDR                             MT6353_ACCDET_CON2
#define PMIC_ACCDET_CMP_PWM_IDLE_MASK                             0x1
#define PMIC_ACCDET_CMP_PWM_IDLE_SHIFT                            4
#define PMIC_ACCDET_VTH_PWM_IDLE_ADDR                             MT6353_ACCDET_CON2
#define PMIC_ACCDET_VTH_PWM_IDLE_MASK                             0x1
#define PMIC_ACCDET_VTH_PWM_IDLE_SHIFT                            5
#define PMIC_ACCDET_MBIAS_PWM_IDLE_ADDR                           MT6353_ACCDET_CON2
#define PMIC_ACCDET_MBIAS_PWM_IDLE_MASK                           0x1
#define PMIC_ACCDET_MBIAS_PWM_IDLE_SHIFT                          6
#define PMIC_ACCDET_EINT_PWM_IDLE_ADDR                            MT6353_ACCDET_CON2
#define PMIC_ACCDET_EINT_PWM_IDLE_MASK                            0x1
#define PMIC_ACCDET_EINT_PWM_IDLE_SHIFT                           7
#define PMIC_ACCDET_PWM_WIDTH_ADDR                                MT6353_ACCDET_CON3
#define PMIC_ACCDET_PWM_WIDTH_MASK                                0xFFFF
#define PMIC_ACCDET_PWM_WIDTH_SHIFT                               0
#define PMIC_ACCDET_PWM_THRESH_ADDR                               MT6353_ACCDET_CON4
#define PMIC_ACCDET_PWM_THRESH_MASK                               0xFFFF
#define PMIC_ACCDET_PWM_THRESH_SHIFT                              0
#define PMIC_ACCDET_RISE_DELAY_ADDR                               MT6353_ACCDET_CON5
#define PMIC_ACCDET_RISE_DELAY_MASK                               0x7FFF
#define PMIC_ACCDET_RISE_DELAY_SHIFT                              0
#define PMIC_ACCDET_FALL_DELAY_ADDR                               MT6353_ACCDET_CON5
#define PMIC_ACCDET_FALL_DELAY_MASK                               0x1
#define PMIC_ACCDET_FALL_DELAY_SHIFT                              15
#define PMIC_ACCDET_DEBOUNCE0_ADDR                                MT6353_ACCDET_CON6
#define PMIC_ACCDET_DEBOUNCE0_MASK                                0xFFFF
#define PMIC_ACCDET_DEBOUNCE0_SHIFT                               0
#define PMIC_ACCDET_DEBOUNCE1_ADDR                                MT6353_ACCDET_CON7
#define PMIC_ACCDET_DEBOUNCE1_MASK                                0xFFFF
#define PMIC_ACCDET_DEBOUNCE1_SHIFT                               0
#define PMIC_ACCDET_DEBOUNCE2_ADDR                                MT6353_ACCDET_CON8
#define PMIC_ACCDET_DEBOUNCE2_MASK                                0xFFFF
#define PMIC_ACCDET_DEBOUNCE2_SHIFT                               0
#define PMIC_ACCDET_DEBOUNCE3_ADDR                                MT6353_ACCDET_CON9
#define PMIC_ACCDET_DEBOUNCE3_MASK                                0xFFFF
#define PMIC_ACCDET_DEBOUNCE3_SHIFT                               0
#define PMIC_ACCDET_DEBOUNCE4_ADDR                                MT6353_ACCDET_CON10
#define PMIC_ACCDET_DEBOUNCE4_MASK                                0xFFFF
#define PMIC_ACCDET_DEBOUNCE4_SHIFT                               0
#define PMIC_ACCDET_IVAL_CUR_IN_ADDR                              MT6353_ACCDET_CON11
#define PMIC_ACCDET_IVAL_CUR_IN_MASK                              0x3
#define PMIC_ACCDET_IVAL_CUR_IN_SHIFT                             0
#define PMIC_ACCDET_EINT_IVAL_CUR_IN_ADDR                         MT6353_ACCDET_CON11
#define PMIC_ACCDET_EINT_IVAL_CUR_IN_MASK                         0x1
#define PMIC_ACCDET_EINT_IVAL_CUR_IN_SHIFT                        2
#define PMIC_ACCDET_IVAL_SAM_IN_ADDR                              MT6353_ACCDET_CON11
#define PMIC_ACCDET_IVAL_SAM_IN_MASK                              0x3
#define PMIC_ACCDET_IVAL_SAM_IN_SHIFT                             4
#define PMIC_ACCDET_EINT_IVAL_SAM_IN_ADDR                         MT6353_ACCDET_CON11
#define PMIC_ACCDET_EINT_IVAL_SAM_IN_MASK                         0x1
#define PMIC_ACCDET_EINT_IVAL_SAM_IN_SHIFT                        6
#define PMIC_ACCDET_IVAL_MEM_IN_ADDR                              MT6353_ACCDET_CON11
#define PMIC_ACCDET_IVAL_MEM_IN_MASK                              0x3
#define PMIC_ACCDET_IVAL_MEM_IN_SHIFT                             8
#define PMIC_ACCDET_EINT_IVAL_MEM_IN_ADDR                         MT6353_ACCDET_CON11
#define PMIC_ACCDET_EINT_IVAL_MEM_IN_MASK                         0x1
#define PMIC_ACCDET_EINT_IVAL_MEM_IN_SHIFT                        10
#define PMIC_ACCDET_EINT_IVAL_SEL_ADDR                            MT6353_ACCDET_CON11
#define PMIC_ACCDET_EINT_IVAL_SEL_MASK                            0x1
#define PMIC_ACCDET_EINT_IVAL_SEL_SHIFT                           14
#define PMIC_ACCDET_IVAL_SEL_ADDR                                 MT6353_ACCDET_CON11
#define PMIC_ACCDET_IVAL_SEL_MASK                                 0x1
#define PMIC_ACCDET_IVAL_SEL_SHIFT                                15
#define PMIC_ACCDET_IRQ_ADDR                                      MT6353_ACCDET_CON12
#define PMIC_ACCDET_IRQ_MASK                                      0x1
#define PMIC_ACCDET_IRQ_SHIFT                                     0
#define PMIC_ACCDET_NEGV_IRQ_ADDR                                 MT6353_ACCDET_CON12
#define PMIC_ACCDET_NEGV_IRQ_MASK                                 0x1
#define PMIC_ACCDET_NEGV_IRQ_SHIFT                                1
#define PMIC_ACCDET_EINT_IRQ_ADDR                                 MT6353_ACCDET_CON12
#define PMIC_ACCDET_EINT_IRQ_MASK                                 0x1
#define PMIC_ACCDET_EINT_IRQ_SHIFT                                2
#define PMIC_ACCDET_IRQ_CLR_ADDR                                  MT6353_ACCDET_CON12
#define PMIC_ACCDET_IRQ_CLR_MASK                                  0x1
#define PMIC_ACCDET_IRQ_CLR_SHIFT                                 8
#define PMIC_ACCDET_NEGV_IRQ_CLR_ADDR                             MT6353_ACCDET_CON12
#define PMIC_ACCDET_NEGV_IRQ_CLR_MASK                             0x1
#define PMIC_ACCDET_NEGV_IRQ_CLR_SHIFT                            9
#define PMIC_ACCDET_EINT_IRQ_CLR_ADDR                             MT6353_ACCDET_CON12
#define PMIC_ACCDET_EINT_IRQ_CLR_MASK                             0x1
#define PMIC_ACCDET_EINT_IRQ_CLR_SHIFT                            10
#define PMIC_ACCDET_EINT_IRQ_POLARITY_ADDR                        MT6353_ACCDET_CON12
#define PMIC_ACCDET_EINT_IRQ_POLARITY_MASK                        0x1
#define PMIC_ACCDET_EINT_IRQ_POLARITY_SHIFT                       15
#define PMIC_ACCDET_TEST_MODE0_ADDR                               MT6353_ACCDET_CON13
#define PMIC_ACCDET_TEST_MODE0_MASK                               0x1
#define PMIC_ACCDET_TEST_MODE0_SHIFT                              0
#define PMIC_ACCDET_TEST_MODE1_ADDR                               MT6353_ACCDET_CON13
#define PMIC_ACCDET_TEST_MODE1_MASK                               0x1
#define PMIC_ACCDET_TEST_MODE1_SHIFT                              1
#define PMIC_ACCDET_TEST_MODE2_ADDR                               MT6353_ACCDET_CON13
#define PMIC_ACCDET_TEST_MODE2_MASK                               0x1
#define PMIC_ACCDET_TEST_MODE2_SHIFT                              2
#define PMIC_ACCDET_TEST_MODE3_ADDR                               MT6353_ACCDET_CON13
#define PMIC_ACCDET_TEST_MODE3_MASK                               0x1
#define PMIC_ACCDET_TEST_MODE3_SHIFT                              3
#define PMIC_ACCDET_TEST_MODE4_ADDR                               MT6353_ACCDET_CON13
#define PMIC_ACCDET_TEST_MODE4_MASK                               0x1
#define PMIC_ACCDET_TEST_MODE4_SHIFT                              4
#define PMIC_ACCDET_TEST_MODE5_ADDR                               MT6353_ACCDET_CON13
#define PMIC_ACCDET_TEST_MODE5_MASK                               0x1
#define PMIC_ACCDET_TEST_MODE5_SHIFT                              5
#define PMIC_ACCDET_PWM_SEL_ADDR                                  MT6353_ACCDET_CON13
#define PMIC_ACCDET_PWM_SEL_MASK                                  0x3
#define PMIC_ACCDET_PWM_SEL_SHIFT                                 6
#define PMIC_ACCDET_IN_SW_ADDR                                    MT6353_ACCDET_CON13
#define PMIC_ACCDET_IN_SW_MASK                                    0x3
#define PMIC_ACCDET_IN_SW_SHIFT                                   8
#define PMIC_ACCDET_CMP_EN_SW_ADDR                                MT6353_ACCDET_CON13
#define PMIC_ACCDET_CMP_EN_SW_MASK                                0x1
#define PMIC_ACCDET_CMP_EN_SW_SHIFT                               12
#define PMIC_ACCDET_VTH_EN_SW_ADDR                                MT6353_ACCDET_CON13
#define PMIC_ACCDET_VTH_EN_SW_MASK                                0x1
#define PMIC_ACCDET_VTH_EN_SW_SHIFT                               13
#define PMIC_ACCDET_MBIAS_EN_SW_ADDR                              MT6353_ACCDET_CON13
#define PMIC_ACCDET_MBIAS_EN_SW_MASK                              0x1
#define PMIC_ACCDET_MBIAS_EN_SW_SHIFT                             14
#define PMIC_ACCDET_PWM_EN_SW_ADDR                                MT6353_ACCDET_CON13
#define PMIC_ACCDET_PWM_EN_SW_MASK                                0x1
#define PMIC_ACCDET_PWM_EN_SW_SHIFT                               15
#define PMIC_ACCDET_IN_ADDR                                       MT6353_ACCDET_CON14
#define PMIC_ACCDET_IN_MASK                                       0x3
#define PMIC_ACCDET_IN_SHIFT                                      0
#define PMIC_ACCDET_CUR_IN_ADDR                                   MT6353_ACCDET_CON14
#define PMIC_ACCDET_CUR_IN_MASK                                   0x3
#define PMIC_ACCDET_CUR_IN_SHIFT                                  2
#define PMIC_ACCDET_SAM_IN_ADDR                                   MT6353_ACCDET_CON14
#define PMIC_ACCDET_SAM_IN_MASK                                   0x3
#define PMIC_ACCDET_SAM_IN_SHIFT                                  4
#define PMIC_ACCDET_MEM_IN_ADDR                                   MT6353_ACCDET_CON14
#define PMIC_ACCDET_MEM_IN_MASK                                   0x3
#define PMIC_ACCDET_MEM_IN_SHIFT                                  6
#define PMIC_ACCDET_STATE_ADDR                                    MT6353_ACCDET_CON14
#define PMIC_ACCDET_STATE_MASK                                    0x7
#define PMIC_ACCDET_STATE_SHIFT                                   8
#define PMIC_ACCDET_MBIAS_CLK_ADDR                                MT6353_ACCDET_CON14
#define PMIC_ACCDET_MBIAS_CLK_MASK                                0x1
#define PMIC_ACCDET_MBIAS_CLK_SHIFT                               12
#define PMIC_ACCDET_VTH_CLK_ADDR                                  MT6353_ACCDET_CON14
#define PMIC_ACCDET_VTH_CLK_MASK                                  0x1
#define PMIC_ACCDET_VTH_CLK_SHIFT                                 13
#define PMIC_ACCDET_CMP_CLK_ADDR                                  MT6353_ACCDET_CON14
#define PMIC_ACCDET_CMP_CLK_MASK                                  0x1
#define PMIC_ACCDET_CMP_CLK_SHIFT                                 14
#define PMIC_DA_AUDACCDETAUXADCSWCTRL_ADDR                        MT6353_ACCDET_CON14
#define PMIC_DA_AUDACCDETAUXADCSWCTRL_MASK                        0x1
#define PMIC_DA_AUDACCDETAUXADCSWCTRL_SHIFT                       15
#define PMIC_ACCDET_EINT_DEB_SEL_ADDR                             MT6353_ACCDET_CON15
#define PMIC_ACCDET_EINT_DEB_SEL_MASK                             0x1
#define PMIC_ACCDET_EINT_DEB_SEL_SHIFT                            0
#define PMIC_ACCDET_EINT_DEBOUNCE_ADDR                            MT6353_ACCDET_CON15
#define PMIC_ACCDET_EINT_DEBOUNCE_MASK                            0x7
#define PMIC_ACCDET_EINT_DEBOUNCE_SHIFT                           4
#define PMIC_ACCDET_EINT_PWM_THRESH_ADDR                          MT6353_ACCDET_CON15
#define PMIC_ACCDET_EINT_PWM_THRESH_MASK                          0x7
#define PMIC_ACCDET_EINT_PWM_THRESH_SHIFT                         8
#define PMIC_ACCDET_EINT_PWM_WIDTH_ADDR                           MT6353_ACCDET_CON15
#define PMIC_ACCDET_EINT_PWM_WIDTH_MASK                           0x3
#define PMIC_ACCDET_EINT_PWM_WIDTH_SHIFT                          12
#define PMIC_ACCDET_NEGV_THRESH_ADDR                              MT6353_ACCDET_CON16
#define PMIC_ACCDET_NEGV_THRESH_MASK                              0x1F
#define PMIC_ACCDET_NEGV_THRESH_SHIFT                             0
#define PMIC_ACCDET_EINT_PWM_FALL_DELAY_ADDR                      MT6353_ACCDET_CON16
#define PMIC_ACCDET_EINT_PWM_FALL_DELAY_MASK                      0x1
#define PMIC_ACCDET_EINT_PWM_FALL_DELAY_SHIFT                     5
#define PMIC_ACCDET_EINT_PWM_RISE_DELAY_ADDR                      MT6353_ACCDET_CON16
#define PMIC_ACCDET_EINT_PWM_RISE_DELAY_MASK                      0x3FF
#define PMIC_ACCDET_EINT_PWM_RISE_DELAY_SHIFT                     6
#define PMIC_ACCDET_TEST_MODE13_ADDR                              MT6353_ACCDET_CON17
#define PMIC_ACCDET_TEST_MODE13_MASK                              0x1
#define PMIC_ACCDET_TEST_MODE13_SHIFT                             1
#define PMIC_ACCDET_TEST_MODE12_ADDR                              MT6353_ACCDET_CON17
#define PMIC_ACCDET_TEST_MODE12_MASK                              0x1
#define PMIC_ACCDET_TEST_MODE12_SHIFT                             2
#define PMIC_ACCDET_NVDETECTOUT_SW_ADDR                           MT6353_ACCDET_CON17
#define PMIC_ACCDET_NVDETECTOUT_SW_MASK                           0x1
#define PMIC_ACCDET_NVDETECTOUT_SW_SHIFT                          3
#define PMIC_ACCDET_TEST_MODE11_ADDR                              MT6353_ACCDET_CON17
#define PMIC_ACCDET_TEST_MODE11_MASK                              0x1
#define PMIC_ACCDET_TEST_MODE11_SHIFT                             5
#define PMIC_ACCDET_TEST_MODE10_ADDR                              MT6353_ACCDET_CON17
#define PMIC_ACCDET_TEST_MODE10_MASK                              0x1
#define PMIC_ACCDET_TEST_MODE10_SHIFT                             6
#define PMIC_ACCDET_EINTCMPOUT_SW_ADDR                            MT6353_ACCDET_CON17
#define PMIC_ACCDET_EINTCMPOUT_SW_MASK                            0x1
#define PMIC_ACCDET_EINTCMPOUT_SW_SHIFT                           7
#define PMIC_ACCDET_TEST_MODE9_ADDR                               MT6353_ACCDET_CON17
#define PMIC_ACCDET_TEST_MODE9_MASK                               0x1
#define PMIC_ACCDET_TEST_MODE9_SHIFT                              9
#define PMIC_ACCDET_TEST_MODE8_ADDR                               MT6353_ACCDET_CON17
#define PMIC_ACCDET_TEST_MODE8_MASK                               0x1
#define PMIC_ACCDET_TEST_MODE8_SHIFT                              10
#define PMIC_ACCDET_AUXADC_CTRL_SW_ADDR                           MT6353_ACCDET_CON17
#define PMIC_ACCDET_AUXADC_CTRL_SW_MASK                           0x1
#define PMIC_ACCDET_AUXADC_CTRL_SW_SHIFT                          11
#define PMIC_ACCDET_TEST_MODE7_ADDR                               MT6353_ACCDET_CON17
#define PMIC_ACCDET_TEST_MODE7_MASK                               0x1
#define PMIC_ACCDET_TEST_MODE7_SHIFT                              13
#define PMIC_ACCDET_TEST_MODE6_ADDR                               MT6353_ACCDET_CON17
#define PMIC_ACCDET_TEST_MODE6_MASK                               0x1
#define PMIC_ACCDET_TEST_MODE6_SHIFT                              14
#define PMIC_ACCDET_EINTCMP_EN_SW_ADDR                            MT6353_ACCDET_CON17
#define PMIC_ACCDET_EINTCMP_EN_SW_MASK                            0x1
#define PMIC_ACCDET_EINTCMP_EN_SW_SHIFT                           15
#define PMIC_RG_NVCMPSWEN_ADDR                                    MT6353_ACCDET_CON18
#define PMIC_RG_NVCMPSWEN_MASK                                    0x1
#define PMIC_RG_NVCMPSWEN_SHIFT                                   8
#define PMIC_RG_NVMODSEL_ADDR                                     MT6353_ACCDET_CON18
#define PMIC_RG_NVMODSEL_MASK                                     0x1
#define PMIC_RG_NVMODSEL_SHIFT                                    9
#define PMIC_RG_SWBUFSWEN_ADDR                                    MT6353_ACCDET_CON18
#define PMIC_RG_SWBUFSWEN_MASK                                    0x1
#define PMIC_RG_SWBUFSWEN_SHIFT                                   10
#define PMIC_RG_SWBUFMODSEL_ADDR                                  MT6353_ACCDET_CON18
#define PMIC_RG_SWBUFMODSEL_MASK                                  0x1
#define PMIC_RG_SWBUFMODSEL_SHIFT                                 11
#define PMIC_RG_NVDETVTH_ADDR                                     MT6353_ACCDET_CON18
#define PMIC_RG_NVDETVTH_MASK                                     0x1
#define PMIC_RG_NVDETVTH_SHIFT                                    12
#define PMIC_RG_NVDETCMPEN_ADDR                                   MT6353_ACCDET_CON18
#define PMIC_RG_NVDETCMPEN_MASK                                   0x1
#define PMIC_RG_NVDETCMPEN_SHIFT                                  13
#define PMIC_RG_EINTCONFIGACCDET_ADDR                             MT6353_ACCDET_CON18
#define PMIC_RG_EINTCONFIGACCDET_MASK                             0x1
#define PMIC_RG_EINTCONFIGACCDET_SHIFT                            14
#define PMIC_RG_EINTCOMPVTH_ADDR                                  MT6353_ACCDET_CON18
#define PMIC_RG_EINTCOMPVTH_MASK                                  0x1
#define PMIC_RG_EINTCOMPVTH_SHIFT                                 15
#define PMIC_ACCDET_EINT_STATE_ADDR                               MT6353_ACCDET_CON19
#define PMIC_ACCDET_EINT_STATE_MASK                               0x7
#define PMIC_ACCDET_EINT_STATE_SHIFT                              0
#define PMIC_ACCDET_AUXADC_DEBOUNCE_END_ADDR                      MT6353_ACCDET_CON19
#define PMIC_ACCDET_AUXADC_DEBOUNCE_END_MASK                      0x1
#define PMIC_ACCDET_AUXADC_DEBOUNCE_END_SHIFT                     3
#define PMIC_ACCDET_AUXADC_CONNECT_PRE_ADDR                       MT6353_ACCDET_CON19
#define PMIC_ACCDET_AUXADC_CONNECT_PRE_MASK                       0x1
#define PMIC_ACCDET_AUXADC_CONNECT_PRE_SHIFT                      4
#define PMIC_ACCDET_EINT_CUR_IN_ADDR                              MT6353_ACCDET_CON19
#define PMIC_ACCDET_EINT_CUR_IN_MASK                              0x1
#define PMIC_ACCDET_EINT_CUR_IN_SHIFT                             8
#define PMIC_ACCDET_EINT_SAM_IN_ADDR                              MT6353_ACCDET_CON19
#define PMIC_ACCDET_EINT_SAM_IN_MASK                              0x1
#define PMIC_ACCDET_EINT_SAM_IN_SHIFT                             9
#define PMIC_ACCDET_EINT_MEM_IN_ADDR                              MT6353_ACCDET_CON19
#define PMIC_ACCDET_EINT_MEM_IN_MASK                              0x1
#define PMIC_ACCDET_EINT_MEM_IN_SHIFT                             10
#define PMIC_AD_NVDETECTOUT_ADDR                                  MT6353_ACCDET_CON19
#define PMIC_AD_NVDETECTOUT_MASK                                  0x1
#define PMIC_AD_NVDETECTOUT_SHIFT                                 13
#define PMIC_AD_EINTCMPOUT_ADDR                                   MT6353_ACCDET_CON19
#define PMIC_AD_EINTCMPOUT_MASK                                   0x1
#define PMIC_AD_EINTCMPOUT_SHIFT                                  14
#define PMIC_DA_NI_EINTCMPEN_ADDR                                 MT6353_ACCDET_CON19
#define PMIC_DA_NI_EINTCMPEN_MASK                                 0x1
#define PMIC_DA_NI_EINTCMPEN_SHIFT                                15
#define PMIC_ACCDET_NEGV_COUNT_IN_ADDR                            MT6353_ACCDET_CON20
#define PMIC_ACCDET_NEGV_COUNT_IN_MASK                            0x3F
#define PMIC_ACCDET_NEGV_COUNT_IN_SHIFT                           0
#define PMIC_ACCDET_NEGV_EN_FINAL_ADDR                            MT6353_ACCDET_CON20
#define PMIC_ACCDET_NEGV_EN_FINAL_MASK                            0x1
#define PMIC_ACCDET_NEGV_EN_FINAL_SHIFT                           6
#define PMIC_ACCDET_NEGV_COUNT_END_ADDR                           MT6353_ACCDET_CON20
#define PMIC_ACCDET_NEGV_COUNT_END_MASK                           0x1
#define PMIC_ACCDET_NEGV_COUNT_END_SHIFT                          12
#define PMIC_ACCDET_NEGV_MINU_ADDR                                MT6353_ACCDET_CON20
#define PMIC_ACCDET_NEGV_MINU_MASK                                0x1
#define PMIC_ACCDET_NEGV_MINU_SHIFT                               13
#define PMIC_ACCDET_NEGV_ADD_ADDR                                 MT6353_ACCDET_CON20
#define PMIC_ACCDET_NEGV_ADD_MASK                                 0x1
#define PMIC_ACCDET_NEGV_ADD_SHIFT                                14
#define PMIC_ACCDET_NEGV_CMP_ADDR                                 MT6353_ACCDET_CON20
#define PMIC_ACCDET_NEGV_CMP_MASK                                 0x1
#define PMIC_ACCDET_NEGV_CMP_SHIFT                                15
#define PMIC_ACCDET_CUR_DEB_ADDR                                  MT6353_ACCDET_CON21
#define PMIC_ACCDET_CUR_DEB_MASK                                  0xFFFF
#define PMIC_ACCDET_CUR_DEB_SHIFT                                 0
#define PMIC_ACCDET_EINT_CUR_DEB_ADDR                             MT6353_ACCDET_CON22
#define PMIC_ACCDET_EINT_CUR_DEB_MASK                             0x7FFF
#define PMIC_ACCDET_EINT_CUR_DEB_SHIFT                            0
#define PMIC_ACCDET_RSV_CON0_ADDR                                 MT6353_ACCDET_CON23
#define PMIC_ACCDET_RSV_CON0_MASK                                 0xFFFF
#define PMIC_ACCDET_RSV_CON0_SHIFT                                0
#define PMIC_ACCDET_RSV_CON1_ADDR                                 MT6353_ACCDET_CON24
#define PMIC_ACCDET_RSV_CON1_MASK                                 0xFFFF
#define PMIC_ACCDET_RSV_CON1_SHIFT                                0
#define PMIC_ACCDET_AUXADC_CONNECT_TIME_ADDR                      MT6353_ACCDET_CON25
#define PMIC_ACCDET_AUXADC_CONNECT_TIME_MASK                      0xFFFF
#define PMIC_ACCDET_AUXADC_CONNECT_TIME_SHIFT                     0
#define PMIC_RG_VCDT_HV_EN_ADDR                                   MT6353_CHR_CON0
#define PMIC_RG_VCDT_HV_EN_MASK                                   0x1
#define PMIC_RG_VCDT_HV_EN_SHIFT                                  0
#define PMIC_RGS_CHR_LDO_DET_ADDR                                 MT6353_CHR_CON0
#define PMIC_RGS_CHR_LDO_DET_MASK                                 0x1
#define PMIC_RGS_CHR_LDO_DET_SHIFT                                1
#define PMIC_RG_PCHR_AUTOMODE_ADDR                                MT6353_CHR_CON0
#define PMIC_RG_PCHR_AUTOMODE_MASK                                0x1
#define PMIC_RG_PCHR_AUTOMODE_SHIFT                               2
#define PMIC_RG_CSDAC_EN_ADDR                                     MT6353_CHR_CON0
#define PMIC_RG_CSDAC_EN_MASK                                     0x1
#define PMIC_RG_CSDAC_EN_SHIFT                                    3
#define PMIC_RG_CHR_EN_ADDR                                       MT6353_CHR_CON0
#define PMIC_RG_CHR_EN_MASK                                       0x1
#define PMIC_RG_CHR_EN_SHIFT                                      4
#define PMIC_RGS_CHRDET_ADDR                                      MT6353_CHR_CON0
#define PMIC_RGS_CHRDET_MASK                                      0x1
#define PMIC_RGS_CHRDET_SHIFT                                     5
#define PMIC_RGS_VCDT_LV_DET_ADDR                                 MT6353_CHR_CON0
#define PMIC_RGS_VCDT_LV_DET_MASK                                 0x1
#define PMIC_RGS_VCDT_LV_DET_SHIFT                                6
#define PMIC_RGS_VCDT_HV_DET_ADDR                                 MT6353_CHR_CON0
#define PMIC_RGS_VCDT_HV_DET_MASK                                 0x1
#define PMIC_RGS_VCDT_HV_DET_SHIFT                                7
#define PMIC_RG_VCDT_LV_VTH_ADDR                                  MT6353_CHR_CON1
#define PMIC_RG_VCDT_LV_VTH_MASK                                  0xF
#define PMIC_RG_VCDT_LV_VTH_SHIFT                                 0
#define PMIC_RG_VCDT_HV_VTH_ADDR                                  MT6353_CHR_CON1
#define PMIC_RG_VCDT_HV_VTH_MASK                                  0xF
#define PMIC_RG_VCDT_HV_VTH_SHIFT                                 4
#define PMIC_RG_VBAT_CV_EN_ADDR                                   MT6353_CHR_CON2
#define PMIC_RG_VBAT_CV_EN_MASK                                   0x1
#define PMIC_RG_VBAT_CV_EN_SHIFT                                  1
#define PMIC_RG_VBAT_CC_EN_ADDR                                   MT6353_CHR_CON2
#define PMIC_RG_VBAT_CC_EN_MASK                                   0x1
#define PMIC_RG_VBAT_CC_EN_SHIFT                                  2
#define PMIC_RG_CS_EN_ADDR                                        MT6353_CHR_CON2
#define PMIC_RG_CS_EN_MASK                                        0x1
#define PMIC_RG_CS_EN_SHIFT                                       3
#define PMIC_RGS_CS_DET_ADDR                                      MT6353_CHR_CON2
#define PMIC_RGS_CS_DET_MASK                                      0x1
#define PMIC_RGS_CS_DET_SHIFT                                     5
#define PMIC_RGS_VBAT_CV_DET_ADDR                                 MT6353_CHR_CON2
#define PMIC_RGS_VBAT_CV_DET_MASK                                 0x1
#define PMIC_RGS_VBAT_CV_DET_SHIFT                                6
#define PMIC_RGS_VBAT_CC_DET_ADDR                                 MT6353_CHR_CON2
#define PMIC_RGS_VBAT_CC_DET_MASK                                 0x1
#define PMIC_RGS_VBAT_CC_DET_SHIFT                                7
#define PMIC_RG_VBAT_CV_VTH_ADDR                                  MT6353_CHR_CON3
#define PMIC_RG_VBAT_CV_VTH_MASK                                  0x1F
#define PMIC_RG_VBAT_CV_VTH_SHIFT                                 0
#define PMIC_RG_VBAT_CC_VTH_ADDR                                  MT6353_CHR_CON3
#define PMIC_RG_VBAT_CC_VTH_MASK                                  0x3
#define PMIC_RG_VBAT_CC_VTH_SHIFT                                 6
#define PMIC_RG_CS_VTH_ADDR                                       MT6353_CHR_CON4
#define PMIC_RG_CS_VTH_MASK                                       0xF
#define PMIC_RG_CS_VTH_SHIFT                                      0
#define PMIC_RG_PCHR_TOHTC_ADDR                                   MT6353_CHR_CON5
#define PMIC_RG_PCHR_TOHTC_MASK                                   0x7
#define PMIC_RG_PCHR_TOHTC_SHIFT                                  0
#define PMIC_RG_PCHR_TOLTC_ADDR                                   MT6353_CHR_CON5
#define PMIC_RG_PCHR_TOLTC_MASK                                   0x7
#define PMIC_RG_PCHR_TOLTC_SHIFT                                  4
#define PMIC_RG_VBAT_OV_EN_ADDR                                   MT6353_CHR_CON6
#define PMIC_RG_VBAT_OV_EN_MASK                                   0x1
#define PMIC_RG_VBAT_OV_EN_SHIFT                                  0
#define PMIC_RG_VBAT_OV_VTH_ADDR                                  MT6353_CHR_CON6
#define PMIC_RG_VBAT_OV_VTH_MASK                                  0x7
#define PMIC_RG_VBAT_OV_VTH_SHIFT                                 2
#define PMIC_RG_VBAT_OV_DEG_ADDR                                  MT6353_CHR_CON6
#define PMIC_RG_VBAT_OV_DEG_MASK                                  0x1
#define PMIC_RG_VBAT_OV_DEG_SHIFT                                 5
#define PMIC_RGS_VBAT_OV_DET_ADDR                                 MT6353_CHR_CON6
#define PMIC_RGS_VBAT_OV_DET_MASK                                 0x1
#define PMIC_RGS_VBAT_OV_DET_SHIFT                                6
#define PMIC_RG_BATON_EN_ADDR                                     MT6353_CHR_CON7
#define PMIC_RG_BATON_EN_MASK                                     0x1
#define PMIC_RG_BATON_EN_SHIFT                                    0
#define PMIC_RG_BATON_HT_EN_RSV0_ADDR                             MT6353_CHR_CON7
#define PMIC_RG_BATON_HT_EN_RSV0_MASK                             0x1
#define PMIC_RG_BATON_HT_EN_RSV0_SHIFT                            1
#define PMIC_BATON_TDET_EN_ADDR                                   MT6353_CHR_CON7
#define PMIC_BATON_TDET_EN_MASK                                   0x1
#define PMIC_BATON_TDET_EN_SHIFT                                  2
#define PMIC_RG_BATON_HT_TRIM_ADDR                                MT6353_CHR_CON7
#define PMIC_RG_BATON_HT_TRIM_MASK                                0x7
#define PMIC_RG_BATON_HT_TRIM_SHIFT                               4
#define PMIC_RG_BATON_HT_TRIM_SET_ADDR                            MT6353_CHR_CON7
#define PMIC_RG_BATON_HT_TRIM_SET_MASK                            0x1
#define PMIC_RG_BATON_HT_TRIM_SET_SHIFT                           7
#define PMIC_RG_BATON_TDET_EN_ADDR                                MT6353_CHR_CON7
#define PMIC_RG_BATON_TDET_EN_MASK                                0x1
#define PMIC_RG_BATON_TDET_EN_SHIFT                               8
#define PMIC_RG_CSDAC_DATA_ADDR                                   MT6353_CHR_CON8
#define PMIC_RG_CSDAC_DATA_MASK                                   0x3FF
#define PMIC_RG_CSDAC_DATA_SHIFT                                  0
#define PMIC_RG_FRC_CSVTH_USBDL_ADDR                              MT6353_CHR_CON9
#define PMIC_RG_FRC_CSVTH_USBDL_MASK                              0x1
#define PMIC_RG_FRC_CSVTH_USBDL_SHIFT                             0
#define PMIC_RGS_PCHR_FLAG_OUT_ADDR                               MT6353_CHR_CON10
#define PMIC_RGS_PCHR_FLAG_OUT_MASK                               0xF
#define PMIC_RGS_PCHR_FLAG_OUT_SHIFT                              0
#define PMIC_RG_PCHR_FLAG_EN_ADDR                                 MT6353_CHR_CON10
#define PMIC_RG_PCHR_FLAG_EN_MASK                                 0x1
#define PMIC_RG_PCHR_FLAG_EN_SHIFT                                4
#define PMIC_RG_OTG_BVALID_EN_ADDR                                MT6353_CHR_CON10
#define PMIC_RG_OTG_BVALID_EN_MASK                                0x1
#define PMIC_RG_OTG_BVALID_EN_SHIFT                               5
#define PMIC_RGS_OTG_BVALID_DET_ADDR                              MT6353_CHR_CON10
#define PMIC_RGS_OTG_BVALID_DET_MASK                              0x1
#define PMIC_RGS_OTG_BVALID_DET_SHIFT                             6
#define PMIC_RG_PCHR_FLAG_SEL_ADDR                                MT6353_CHR_CON11
#define PMIC_RG_PCHR_FLAG_SEL_MASK                                0x3F
#define PMIC_RG_PCHR_FLAG_SEL_SHIFT                               0
#define PMIC_RG_PCHR_TESTMODE_ADDR                                MT6353_CHR_CON12
#define PMIC_RG_PCHR_TESTMODE_MASK                                0x1
#define PMIC_RG_PCHR_TESTMODE_SHIFT                               0
#define PMIC_RG_CSDAC_TESTMODE_ADDR                               MT6353_CHR_CON12
#define PMIC_RG_CSDAC_TESTMODE_MASK                               0x1
#define PMIC_RG_CSDAC_TESTMODE_SHIFT                              1
#define PMIC_RG_PCHR_RST_ADDR                                     MT6353_CHR_CON12
#define PMIC_RG_PCHR_RST_MASK                                     0x1
#define PMIC_RG_PCHR_RST_SHIFT                                    2
#define PMIC_RG_PCHR_FT_CTRL_ADDR                                 MT6353_CHR_CON12
#define PMIC_RG_PCHR_FT_CTRL_MASK                                 0x7
#define PMIC_RG_PCHR_FT_CTRL_SHIFT                                4
#define PMIC_RG_CHRWDT_TD_ADDR                                    MT6353_CHR_CON13
#define PMIC_RG_CHRWDT_TD_MASK                                    0xF
#define PMIC_RG_CHRWDT_TD_SHIFT                                   0
#define PMIC_RG_CHRWDT_EN_ADDR                                    MT6353_CHR_CON13
#define PMIC_RG_CHRWDT_EN_MASK                                    0x1
#define PMIC_RG_CHRWDT_EN_SHIFT                                   4
#define PMIC_RG_CHRWDT_WR_ADDR                                    MT6353_CHR_CON13
#define PMIC_RG_CHRWDT_WR_MASK                                    0x1
#define PMIC_RG_CHRWDT_WR_SHIFT                                   8
#define PMIC_RG_PCHR_RV_ADDR                                      MT6353_CHR_CON14
#define PMIC_RG_PCHR_RV_MASK                                      0xFF
#define PMIC_RG_PCHR_RV_SHIFT                                     0
#define PMIC_RG_CHRWDT_INT_EN_ADDR                                MT6353_CHR_CON15
#define PMIC_RG_CHRWDT_INT_EN_MASK                                0x1
#define PMIC_RG_CHRWDT_INT_EN_SHIFT                               0
#define PMIC_RG_CHRWDT_FLAG_WR_ADDR                               MT6353_CHR_CON15
#define PMIC_RG_CHRWDT_FLAG_WR_MASK                               0x1
#define PMIC_RG_CHRWDT_FLAG_WR_SHIFT                              1
#define PMIC_RGS_CHRWDT_OUT_ADDR                                  MT6353_CHR_CON15
#define PMIC_RGS_CHRWDT_OUT_MASK                                  0x1
#define PMIC_RGS_CHRWDT_OUT_SHIFT                                 2
#define PMIC_RG_USBDL_RST_ADDR                                    MT6353_CHR_CON16
#define PMIC_RG_USBDL_RST_MASK                                    0x1
#define PMIC_RG_USBDL_RST_SHIFT                                   2
#define PMIC_RG_USBDL_SET_ADDR                                    MT6353_CHR_CON16
#define PMIC_RG_USBDL_SET_MASK                                    0x1
#define PMIC_RG_USBDL_SET_SHIFT                                   3
#define PMIC_RG_ADCIN_VSEN_MUX_EN_ADDR                            MT6353_CHR_CON16
#define PMIC_RG_ADCIN_VSEN_MUX_EN_MASK                            0x1
#define PMIC_RG_ADCIN_VSEN_MUX_EN_SHIFT                           8
#define PMIC_RG_ADCIN_VSEN_EXT_BATON_EN_ADDR                      MT6353_CHR_CON16
#define PMIC_RG_ADCIN_VSEN_EXT_BATON_EN_MASK                      0x1
#define PMIC_RG_ADCIN_VSEN_EXT_BATON_EN_SHIFT                     9
#define PMIC_RG_ADCIN_VBAT_EN_ADDR                                MT6353_CHR_CON16
#define PMIC_RG_ADCIN_VBAT_EN_MASK                                0x1
#define PMIC_RG_ADCIN_VBAT_EN_SHIFT                               10
#define PMIC_RG_ADCIN_VSEN_EN_ADDR                                MT6353_CHR_CON16
#define PMIC_RG_ADCIN_VSEN_EN_MASK                                0x1
#define PMIC_RG_ADCIN_VSEN_EN_SHIFT                               11
#define PMIC_RG_ADCIN_CHR_EN_ADDR                                 MT6353_CHR_CON16
#define PMIC_RG_ADCIN_CHR_EN_MASK                                 0x1
#define PMIC_RG_ADCIN_CHR_EN_SHIFT                                12
#define PMIC_RG_UVLO_VTHL_ADDR                                    MT6353_CHR_CON17
#define PMIC_RG_UVLO_VTHL_MASK                                    0x1F
#define PMIC_RG_UVLO_VTHL_SHIFT                                   0
#define PMIC_RG_UVLO_VH_LAT_ADDR                                  MT6353_CHR_CON17
#define PMIC_RG_UVLO_VH_LAT_MASK                                  0x1
#define PMIC_RG_UVLO_VH_LAT_SHIFT                                 7
#define PMIC_RG_LBAT_INT_VTH_ADDR                                 MT6353_CHR_CON18
#define PMIC_RG_LBAT_INT_VTH_MASK                                 0x1F
#define PMIC_RG_LBAT_INT_VTH_SHIFT                                0
#define PMIC_RG_BGR_RSEL_ADDR                                     MT6353_CHR_CON19
#define PMIC_RG_BGR_RSEL_MASK                                     0x7
#define PMIC_RG_BGR_RSEL_SHIFT                                    0
#define PMIC_RG_BGR_UNCHOP_PH_ADDR                                MT6353_CHR_CON19
#define PMIC_RG_BGR_UNCHOP_PH_MASK                                0x1
#define PMIC_RG_BGR_UNCHOP_PH_SHIFT                               4
#define PMIC_RG_BGR_UNCHOP_ADDR                                   MT6353_CHR_CON19
#define PMIC_RG_BGR_UNCHOP_MASK                                   0x1
#define PMIC_RG_BGR_UNCHOP_SHIFT                                  5
#define PMIC_RG_BC11_BB_CTRL_ADDR                                 MT6353_CHR_CON20
#define PMIC_RG_BC11_BB_CTRL_MASK                                 0x1
#define PMIC_RG_BC11_BB_CTRL_SHIFT                                0
#define PMIC_RG_BC11_RST_ADDR                                     MT6353_CHR_CON20
#define PMIC_RG_BC11_RST_MASK                                     0x1
#define PMIC_RG_BC11_RST_SHIFT                                    1
#define PMIC_RG_BC11_VSRC_EN_ADDR                                 MT6353_CHR_CON20
#define PMIC_RG_BC11_VSRC_EN_MASK                                 0x3
#define PMIC_RG_BC11_VSRC_EN_SHIFT                                2
#define PMIC_RGS_BC11_CMP_OUT_ADDR                                MT6353_CHR_CON20
#define PMIC_RGS_BC11_CMP_OUT_MASK                                0x1
#define PMIC_RGS_BC11_CMP_OUT_SHIFT                               7
#define PMIC_RG_BC11_VREF_VTH_ADDR                                MT6353_CHR_CON21
#define PMIC_RG_BC11_VREF_VTH_MASK                                0x3
#define PMIC_RG_BC11_VREF_VTH_SHIFT                               0
#define PMIC_RG_BC11_CMP_EN_ADDR                                  MT6353_CHR_CON21
#define PMIC_RG_BC11_CMP_EN_MASK                                  0x3
#define PMIC_RG_BC11_CMP_EN_SHIFT                                 2
#define PMIC_RG_BC11_IPD_EN_ADDR                                  MT6353_CHR_CON21
#define PMIC_RG_BC11_IPD_EN_MASK                                  0x3
#define PMIC_RG_BC11_IPD_EN_SHIFT                                 4
#define PMIC_RG_BC11_IPU_EN_ADDR                                  MT6353_CHR_CON21
#define PMIC_RG_BC11_IPU_EN_MASK                                  0x3
#define PMIC_RG_BC11_IPU_EN_SHIFT                                 6
#define PMIC_RG_BC11_BIAS_EN_ADDR                                 MT6353_CHR_CON21
#define PMIC_RG_BC11_BIAS_EN_MASK                                 0x1
#define PMIC_RG_BC11_BIAS_EN_SHIFT                                8
#define PMIC_RG_CSDAC_STP_INC_ADDR                                MT6353_CHR_CON22
#define PMIC_RG_CSDAC_STP_INC_MASK                                0x7
#define PMIC_RG_CSDAC_STP_INC_SHIFT                               0
#define PMIC_RG_CSDAC_STP_DEC_ADDR                                MT6353_CHR_CON22
#define PMIC_RG_CSDAC_STP_DEC_MASK                                0x7
#define PMIC_RG_CSDAC_STP_DEC_SHIFT                               4
#define PMIC_RG_CSDAC_DLY_ADDR                                    MT6353_CHR_CON23
#define PMIC_RG_CSDAC_DLY_MASK                                    0x7
#define PMIC_RG_CSDAC_DLY_SHIFT                                   0
#define PMIC_RG_CSDAC_STP_ADDR                                    MT6353_CHR_CON23
#define PMIC_RG_CSDAC_STP_MASK                                    0x7
#define PMIC_RG_CSDAC_STP_SHIFT                                   4
#define PMIC_RG_LOW_ICH_DB_ADDR                                   MT6353_CHR_CON24
#define PMIC_RG_LOW_ICH_DB_MASK                                   0x3F
#define PMIC_RG_LOW_ICH_DB_SHIFT                                  0
#define PMIC_RG_CHRIND_ON_ADDR                                    MT6353_CHR_CON24
#define PMIC_RG_CHRIND_ON_MASK                                    0x1
#define PMIC_RG_CHRIND_ON_SHIFT                                   6
#define PMIC_RG_CHRIND_DIMMING_ADDR                               MT6353_CHR_CON24
#define PMIC_RG_CHRIND_DIMMING_MASK                               0x1
#define PMIC_RG_CHRIND_DIMMING_SHIFT                              7
#define PMIC_RG_CV_MODE_ADDR                                      MT6353_CHR_CON25
#define PMIC_RG_CV_MODE_MASK                                      0x1
#define PMIC_RG_CV_MODE_SHIFT                                     0
#define PMIC_RG_VCDT_MODE_ADDR                                    MT6353_CHR_CON25
#define PMIC_RG_VCDT_MODE_MASK                                    0x1
#define PMIC_RG_VCDT_MODE_SHIFT                                   1
#define PMIC_RG_CSDAC_MODE_ADDR                                   MT6353_CHR_CON25
#define PMIC_RG_CSDAC_MODE_MASK                                   0x1
#define PMIC_RG_CSDAC_MODE_SHIFT                                  2
#define PMIC_RG_TRACKING_EN_ADDR                                  MT6353_CHR_CON25
#define PMIC_RG_TRACKING_EN_MASK                                  0x1
#define PMIC_RG_TRACKING_EN_SHIFT                                 4
#define PMIC_RG_HWCV_EN_ADDR                                      MT6353_CHR_CON25
#define PMIC_RG_HWCV_EN_MASK                                      0x1
#define PMIC_RG_HWCV_EN_SHIFT                                     6
#define PMIC_RG_ULC_DET_EN_ADDR                                   MT6353_CHR_CON25
#define PMIC_RG_ULC_DET_EN_MASK                                   0x1
#define PMIC_RG_ULC_DET_EN_SHIFT                                  7
#define PMIC_RG_BGR_TRIM_EN_ADDR                                  MT6353_CHR_CON26
#define PMIC_RG_BGR_TRIM_EN_MASK                                  0x1
#define PMIC_RG_BGR_TRIM_EN_SHIFT                                 0
#define PMIC_RG_ICHRG_TRIM_ADDR                                   MT6353_CHR_CON26
#define PMIC_RG_ICHRG_TRIM_MASK                                   0xF
#define PMIC_RG_ICHRG_TRIM_SHIFT                                  4
#define PMIC_RG_BGR_TRIM_ADDR                                     MT6353_CHR_CON27
#define PMIC_RG_BGR_TRIM_MASK                                     0x1F
#define PMIC_RG_BGR_TRIM_SHIFT                                    0
#define PMIC_RG_OVP_TRIM_ADDR                                     MT6353_CHR_CON28
#define PMIC_RG_OVP_TRIM_MASK                                     0xF
#define PMIC_RG_OVP_TRIM_SHIFT                                    0
#define PMIC_RG_CHR_OSC_TRIM_ADDR                                 MT6353_CHR_CON29
#define PMIC_RG_CHR_OSC_TRIM_MASK                                 0x1F
#define PMIC_RG_CHR_OSC_TRIM_SHIFT                                0
#define PMIC_DA_QI_BGR_EXT_BUF_EN_ADDR                            MT6353_CHR_CON29
#define PMIC_DA_QI_BGR_EXT_BUF_EN_MASK                            0x1
#define PMIC_DA_QI_BGR_EXT_BUF_EN_SHIFT                           5
#define PMIC_RG_BGR_TEST_EN_ADDR                                  MT6353_CHR_CON29
#define PMIC_RG_BGR_TEST_EN_MASK                                  0x1
#define PMIC_RG_BGR_TEST_EN_SHIFT                                 6
#define PMIC_RG_BGR_TEST_RSTB_ADDR                                MT6353_CHR_CON29
#define PMIC_RG_BGR_TEST_RSTB_MASK                                0x1
#define PMIC_RG_BGR_TEST_RSTB_SHIFT                               7
#define PMIC_RG_DAC_USBDL_MAX_ADDR                                MT6353_CHR_CON30
#define PMIC_RG_DAC_USBDL_MAX_MASK                                0x3FF
#define PMIC_RG_DAC_USBDL_MAX_SHIFT                               0
#define PMIC_RG_CM_VDEC_TRIG_ADDR                                 MT6353_CHR_CON31
#define PMIC_RG_CM_VDEC_TRIG_MASK                                 0x1
#define PMIC_RG_CM_VDEC_TRIG_SHIFT                                0
#define PMIC_PCHR_CM_VDEC_STATUS_ADDR                             MT6353_CHR_CON31
#define PMIC_PCHR_CM_VDEC_STATUS_MASK                             0x1
#define PMIC_PCHR_CM_VDEC_STATUS_SHIFT                            4
#define PMIC_RG_CM_VINC_TRIG_ADDR                                 MT6353_CHR_CON32
#define PMIC_RG_CM_VINC_TRIG_MASK                                 0x1
#define PMIC_RG_CM_VINC_TRIG_SHIFT                                0
#define PMIC_PCHR_CM_VINC_STATUS_ADDR                             MT6353_CHR_CON32
#define PMIC_PCHR_CM_VINC_STATUS_MASK                             0x1
#define PMIC_PCHR_CM_VINC_STATUS_SHIFT                            4
#define PMIC_RG_CM_VDEC_HPRD1_ADDR                                MT6353_CHR_CON33
#define PMIC_RG_CM_VDEC_HPRD1_MASK                                0x3F
#define PMIC_RG_CM_VDEC_HPRD1_SHIFT                               0
#define PMIC_RG_CM_VDEC_HPRD2_ADDR                                MT6353_CHR_CON33
#define PMIC_RG_CM_VDEC_HPRD2_MASK                                0x3F
#define PMIC_RG_CM_VDEC_HPRD2_SHIFT                               8
#define PMIC_RG_CM_VDEC_HPRD3_ADDR                                MT6353_CHR_CON34
#define PMIC_RG_CM_VDEC_HPRD3_MASK                                0x3F
#define PMIC_RG_CM_VDEC_HPRD3_SHIFT                               0
#define PMIC_RG_CM_VDEC_HPRD4_ADDR                                MT6353_CHR_CON34
#define PMIC_RG_CM_VDEC_HPRD4_MASK                                0x3F
#define PMIC_RG_CM_VDEC_HPRD4_SHIFT                               8
#define PMIC_RG_CM_VDEC_HPRD5_ADDR                                MT6353_CHR_CON35
#define PMIC_RG_CM_VDEC_HPRD5_MASK                                0x3F
#define PMIC_RG_CM_VDEC_HPRD5_SHIFT                               0
#define PMIC_RG_CM_VDEC_HPRD6_ADDR                                MT6353_CHR_CON35
#define PMIC_RG_CM_VDEC_HPRD6_MASK                                0x3F
#define PMIC_RG_CM_VDEC_HPRD6_SHIFT                               8
#define PMIC_RG_CM_VINC_HPRD1_ADDR                                MT6353_CHR_CON36
#define PMIC_RG_CM_VINC_HPRD1_MASK                                0x3F
#define PMIC_RG_CM_VINC_HPRD1_SHIFT                               0
#define PMIC_RG_CM_VINC_HPRD2_ADDR                                MT6353_CHR_CON36
#define PMIC_RG_CM_VINC_HPRD2_MASK                                0x3F
#define PMIC_RG_CM_VINC_HPRD2_SHIFT                               8
#define PMIC_RG_CM_VINC_HPRD3_ADDR                                MT6353_CHR_CON37
#define PMIC_RG_CM_VINC_HPRD3_MASK                                0x3F
#define PMIC_RG_CM_VINC_HPRD3_SHIFT                               0
#define PMIC_RG_CM_VINC_HPRD4_ADDR                                MT6353_CHR_CON37
#define PMIC_RG_CM_VINC_HPRD4_MASK                                0x3F
#define PMIC_RG_CM_VINC_HPRD4_SHIFT                               8
#define PMIC_RG_CM_VINC_HPRD5_ADDR                                MT6353_CHR_CON38
#define PMIC_RG_CM_VINC_HPRD5_MASK                                0x3F
#define PMIC_RG_CM_VINC_HPRD5_SHIFT                               0
#define PMIC_RG_CM_VINC_HPRD6_ADDR                                MT6353_CHR_CON38
#define PMIC_RG_CM_VINC_HPRD6_MASK                                0x3F
#define PMIC_RG_CM_VINC_HPRD6_SHIFT                               8
#define PMIC_RG_CM_LPRD_ADDR                                      MT6353_CHR_CON39
#define PMIC_RG_CM_LPRD_MASK                                      0x3F
#define PMIC_RG_CM_LPRD_SHIFT                                     0
#define PMIC_RG_CM_CS_VTHL_ADDR                                   MT6353_CHR_CON40
#define PMIC_RG_CM_CS_VTHL_MASK                                   0xF
#define PMIC_RG_CM_CS_VTHL_SHIFT                                  0
#define PMIC_RG_CM_CS_VTHH_ADDR                                   MT6353_CHR_CON40
#define PMIC_RG_CM_CS_VTHH_MASK                                   0xF
#define PMIC_RG_CM_CS_VTHH_SHIFT                                  4
#define PMIC_RG_PCHR_RSV_ADDR                                     MT6353_CHR_CON41
#define PMIC_RG_PCHR_RSV_MASK                                     0xFF
#define PMIC_RG_PCHR_RSV_SHIFT                                    0
#define PMIC_RG_ENVTEM_D_ADDR                                     MT6353_CHR_CON42
#define PMIC_RG_ENVTEM_D_MASK                                     0x1
#define PMIC_RG_ENVTEM_D_SHIFT                                    0
#define PMIC_RG_ENVTEM_EN_ADDR                                    MT6353_CHR_CON42
#define PMIC_RG_ENVTEM_EN_MASK                                    0x1
#define PMIC_RG_ENVTEM_EN_SHIFT                                   1
#define PMIC_RGS_BATON_HV_ADDR                                    MT6353_BATON_CON0
#define PMIC_RGS_BATON_HV_MASK                                    0x1
#define PMIC_RGS_BATON_HV_SHIFT                                   6
#define PMIC_RG_HW_VTH_CTRL_ADDR                                  MT6353_BATON_CON0
#define PMIC_RG_HW_VTH_CTRL_MASK                                  0x1
#define PMIC_RG_HW_VTH_CTRL_SHIFT                                 11
#define PMIC_RG_HW_VTH2_ADDR                                      MT6353_BATON_CON0
#define PMIC_RG_HW_VTH2_MASK                                      0x3
#define PMIC_RG_HW_VTH2_SHIFT                                     12
#define PMIC_RG_HW_VTH1_ADDR                                      MT6353_BATON_CON0
#define PMIC_RG_HW_VTH1_MASK                                      0x3
#define PMIC_RG_HW_VTH1_SHIFT                                     14
#define PMIC_RG_CM_VDEC_INT_EN_ADDR                               MT6353_CHR_CON43
#define PMIC_RG_CM_VDEC_INT_EN_MASK                               0x1
#define PMIC_RG_CM_VDEC_INT_EN_SHIFT                              9
#define PMIC_RG_CM_VINC_INT_EN_ADDR                               MT6353_CHR_CON43
#define PMIC_RG_CM_VINC_INT_EN_MASK                               0x1
#define PMIC_RG_CM_VINC_INT_EN_SHIFT                              11
#define PMIC_RG_QI_BATON_LT_EN_ADDR                               MT6353_CHR_CON44
#define PMIC_RG_QI_BATON_LT_EN_MASK                               0x1
#define PMIC_RG_QI_BATON_LT_EN_SHIFT                              0
#define PMIC_RGS_BATON_UNDET_ADDR                                 MT6353_CHR_CON44
#define PMIC_RGS_BATON_UNDET_MASK                                 0x1
#define PMIC_RGS_BATON_UNDET_SHIFT                                1
#define PMIC_RG_VCDT_TRIM_ADDR                                    MT6353_CHR_CON48
#define PMIC_RG_VCDT_TRIM_MASK                                    0xF
#define PMIC_RG_VCDT_TRIM_SHIFT                                   0
#define PMIC_RG_INDICATOR_TRIM_ADDR                               MT6353_CHR_CON48
#define PMIC_RG_INDICATOR_TRIM_MASK                               0x7
#define PMIC_RG_INDICATOR_TRIM_SHIFT                              10
#define PMIC_EOSC_CALI_START_ADDR                                 MT6353_EOSC_CALI_CON0
#define PMIC_EOSC_CALI_START_MASK                                 0x1
#define PMIC_EOSC_CALI_START_SHIFT                                0
#define PMIC_EOSC_CALI_TD_ADDR                                    MT6353_EOSC_CALI_CON0
#define PMIC_EOSC_CALI_TD_MASK                                    0x7
#define PMIC_EOSC_CALI_TD_SHIFT                                   5
#define PMIC_EOSC_CALI_TEST_ADDR                                  MT6353_EOSC_CALI_CON0
#define PMIC_EOSC_CALI_TEST_MASK                                  0xF
#define PMIC_EOSC_CALI_TEST_SHIFT                                 9
#define PMIC_EOSC_CALI_DCXO_RDY_TD_ADDR                           MT6353_EOSC_CALI_CON1
#define PMIC_EOSC_CALI_DCXO_RDY_TD_MASK                           0x7
#define PMIC_EOSC_CALI_DCXO_RDY_TD_SHIFT                          0
#define PMIC_FRC_VTCXO0_ON_ADDR                                   MT6353_EOSC_CALI_CON1
#define PMIC_FRC_VTCXO0_ON_MASK                                   0x1
#define PMIC_FRC_VTCXO0_ON_SHIFT                                  8
#define PMIC_EOSC_CALI_RSV_ADDR                                   MT6353_EOSC_CALI_CON1
#define PMIC_EOSC_CALI_RSV_MASK                                   0xF
#define PMIC_EOSC_CALI_RSV_SHIFT                                  11
#define PMIC_VRTC_PWM_MODE_ADDR                                   MT6353_VRTC_PWM_CON0
#define PMIC_VRTC_PWM_MODE_MASK                                   0x1
#define PMIC_VRTC_PWM_MODE_SHIFT                                  0
#define PMIC_VRTC_PWM_RSV_ADDR                                    MT6353_VRTC_PWM_CON0
#define PMIC_VRTC_PWM_RSV_MASK                                    0x7
#define PMIC_VRTC_PWM_RSV_SHIFT                                   1
#define PMIC_VRTC_PWM_L_DUTY_ADDR                                 MT6353_VRTC_PWM_CON0
#define PMIC_VRTC_PWM_L_DUTY_MASK                                 0xF
#define PMIC_VRTC_PWM_L_DUTY_SHIFT                                4
#define PMIC_VRTC_PWM_H_DUTY_ADDR                                 MT6353_VRTC_PWM_CON0
#define PMIC_VRTC_PWM_H_DUTY_MASK                                 0xF
#define PMIC_VRTC_PWM_H_DUTY_SHIFT                                8
#define PMIC_VRTC_CAP_SEL_ADDR                                    MT6353_VRTC_PWM_CON0
#define PMIC_VRTC_CAP_SEL_MASK                                    0x1
#define PMIC_VRTC_CAP_SEL_SHIFT                                   12

typedef enum {
	PMIC_USBDL,
	PMIC_VTCXO_CONFIG,
	PMIC_RG_THR_DET_DIS,
	PMIC_RG_THR_TEST,
	PMIC_RG_STRUP_THER_DEB_RMAX,
	PMIC_RG_STRUP_THER_DEB_FMAX,
	PMIC_DDUVLO_DEB_EN,
	PMIC_RG_PWRBB_DEB_EN,
	PMIC_RG_STRUP_OSC_EN,
	PMIC_RG_STRUP_OSC_EN_SEL,
	PMIC_RG_STRUP_FT_CTRL,
	PMIC_RG_STRUP_PWRON_FORCE,
	PMIC_RG_BIASGEN_FORCE,
	PMIC_RG_STRUP_PWRON,
	PMIC_RG_STRUP_PWRON_SEL,
	PMIC_RG_BIASGEN,
	PMIC_RG_BIASGEN_SEL,
	PMIC_RG_RTC_XOSC32_ENB,
	PMIC_RG_RTC_XOSC32_ENB_SEL,
	PMIC_STRUP_DIG_IO_PG_FORCE,
	PMIC_CLR_JUST_RST,
	PMIC_UVLO_L2H_DEB_EN,
	PMIC_JUST_PWRKEY_RST,
	PMIC_DA_QI_OSC_EN,
	PMIC_RG_STRUP_EXT_PMIC_EN,
	PMIC_RG_STRUP_EXT_PMIC_SEL,
	PMIC_STRUP_CON8_RSV0,
	PMIC_DA_QI_EXT_PMIC_EN,
	PMIC_RG_STRUP_AUXADC_START_SW,
	PMIC_RG_STRUP_AUXADC_RSTB_SW,
	PMIC_RG_STRUP_AUXADC_START_SEL,
	PMIC_RG_STRUP_AUXADC_RSTB_SEL,
	PMIC_RG_STRUP_AUXADC_RPCNT_MAX,
	PMIC_STRUP_PWROFF_SEQ_EN,
	PMIC_STRUP_PWROFF_PREOFF_EN,
	PMIC_STRUP_DIG0_RSV0,
	PMIC_STRUP_DIG1_RSV0,
	PMIC_RG_RSV_SWREG,
	PMIC_RG_STRUP_UVLO_U1U2_SEL,
	PMIC_RG_STRUP_UVLO_U1U2_SEL_SWCTRL,
	PMIC_RG_STRUP_THR_CLR,
	PMIC_RG_STRUP_LONG_PRESS_EXT_SEL,
	PMIC_RG_STRUP_LONG_PRESS_EXT_TD,
	PMIC_RG_STRUP_LONG_PRESS_EXT_EN,
	PMIC_RG_STRUP_LONG_PRESS_EXT_CHR_CTRL,
	PMIC_RG_STRUP_LONG_PRESS_EXT_PWRKEY_CTRL,
	PMIC_RG_STRUP_LONG_PRESS_EXT_PWRBB_CTRL,
	PMIC_RG_STRUP_ENVTEM,
	PMIC_RG_STRUP_ENVTEM_CTRL,
	PMIC_RG_STRUP_PWRKEY_COUNT_RESET,
	PMIC_RG_STRUP_VCORE2_PG_H2L_EN,
	PMIC_RG_STRUP_VMCH_PG_H2L_EN,
	PMIC_RG_STRUP_VMC_PG_H2L_EN,
	PMIC_RG_STRUP_VUSB33_PG_H2L_EN,
	PMIC_RG_STRUP_VSRAM_PROC_PG_H2L_EN,
	PMIC_RG_STRUP_VPROC_PG_H2L_EN,
	PMIC_RG_STRUP_VDRAM_PG_H2L_EN,
	PMIC_RG_STRUP_VAUD28_PG_H2L_EN,
	PMIC_RG_STRUP_VEMC_PG_H2L_EN,
	PMIC_RG_STRUP_VS1_PG_H2L_EN,
	PMIC_RG_STRUP_VCORE_PG_H2L_EN,
	PMIC_RG_STRUP_VAUX18_PG_H2L_EN,
	PMIC_RG_STRUP_VCORE2_PG_ENB,
	PMIC_RG_STRUP_VMCH_PG_ENB,
	PMIC_RG_STRUP_VMC_PG_ENB,
	PMIC_RG_STRUP_VXO22_PG_ENB,
	PMIC_RG_STRUP_VTCXO_PG_ENB,
	PMIC_RG_STRUP_VUSB33_PG_ENB,
	PMIC_RG_STRUP_VSRAM_PROC_PG_ENB,
	PMIC_RG_STRUP_VPROC_PG_ENB,
	PMIC_RG_STRUP_VDRAM_PG_ENB,
	PMIC_RG_STRUP_VAUD28_PG_ENB,
	PMIC_RG_STRUP_VIO28_PG_ENB,
	PMIC_RG_STRUP_VEMC_PG_ENB,
	PMIC_RG_STRUP_VIO18_PG_ENB,
	PMIC_RG_STRUP_VS1_PG_ENB,
	PMIC_RG_STRUP_VCORE_PG_ENB,
	PMIC_RG_STRUP_VAUX18_PG_ENB,
	PMIC_RG_STRUP_VCORE2_OC_ENB,
	PMIC_RG_STRUP_VPROC_OC_ENB,
	PMIC_RG_STRUP_VS1_OC_ENB,
	PMIC_RG_STRUP_VCORE_OC_ENB,
	PMIC_RG_STRUP_LONG_PRESS_RESET_EXTEND,
	PMIC_RG_THRDET_SEL,
	PMIC_RG_STRUP_THR_SEL,
	PMIC_RG_THR_TMODE,
	PMIC_RG_VREF_BG,
	PMIC_RG_STRUP_IREF_TRIM,
	PMIC_RG_RST_DRVSEL,
	PMIC_RG_EN_DRVSEL,
	PMIC_RG_FCHR_KEYDET_EN,
	PMIC_RG_FCHR_PU_EN,
	PMIC_RG_PMU_RSV,
	PMIC_TYPE_C_PHY_RG_CC_RP_SEL,
	PMIC_REG_TYPE_C_PHY_RG_PD_TX_SLEW_CALEN,
	PMIC_REG_TYPE_C_PHY_RG_PD_TXSLEW_I,
	PMIC_REG_TYPE_C_PHY_RG_CC_MPX_SEL,
	PMIC_REG_TYPE_C_PHY_RG_CC_RESERVE,
	PMIC_REG_TYPE_C_VCMP_CC2_SW_SEL_ST_CNT_VAL,
	PMIC_REG_TYPE_C_VCMP_BIAS_EN_ST_CNT_VAL,
	PMIC_REG_TYPE_C_VCMP_DAC_EN_ST_CNT_VAL,
	PMIC_REG_TYPE_C_PORT_SUPPORT_ROLE,
	PMIC_REG_TYPE_C_ADC_EN,
	PMIC_REG_TYPE_C_ACC_EN,
	PMIC_REG_TYPE_C_AUDIO_ACC_EN,
	PMIC_REG_TYPE_C_DEBUG_ACC_EN,
	PMIC_REG_TYPE_C_TRY_SRC_ST_EN,
	PMIC_REG_TYPE_C_ATTACH_SRC_2_TRY_WAIT_SNK_ST_EN,
	PMIC_REG_TYPE_C_PD2CC_DET_DISABLE_EN,
	PMIC_REG_TYPE_C_ATTACH_SRC_OPEN_PDDEBOUNCE_EN,
	PMIC_REG_TYPE_C_DISABLE_ST_RD_EN,
	PMIC_W1_TYPE_C_SW_ENT_DISABLE_CMD,
	PMIC_W1_TYPE_C_SW_ENT_UNATCH_SRC_CMD,
	PMIC_W1_TYPE_C_SW_ENT_UNATCH_SNK_CMD,
	PMIC_W1_TYPE_C_SW_PR_SWAP_INDICATE_CMD,
	PMIC_W1_TYPE_C_SW_VCONN_SWAP_INDICATE_CMD,
	PMIC_W1_TYPE_C_SW_ADC_RESULT_MET_VRD_DEFAULT_CMD,
	PMIC_W1_TYPE_C_SW_ADC_RESULT_MET_VRD_15_CMD,
	PMIC_W1_TYPE_C_SW_ADC_RESULT_MET_VRD_30_CMD,
	PMIC_TYPE_C_SW_VBUS_PRESENT,
	PMIC_TYPE_C_SW_DA_DRIVE_VCONN_EN,
	PMIC_TYPE_C_SW_VBUS_DET_DIS,
	PMIC_TYPE_C_SW_CC_DET_DIS,
	PMIC_TYPE_C_SW_PD_EN,
	PMIC_W1_TYPE_C_SW_ENT_SNK_PWR_REDETECT_CMD,
	PMIC_REG_TYPE_C_CC_VOL_PERIODIC_MEAS_VAL,
	PMIC_REG_TYPE_C_CC_VOL_CC_DEBOUNCE_CNT_VAL,
	PMIC_REG_TYPE_C_CC_VOL_PD_DEBOUNCE_CNT_VAL,
	PMIC_REG_TYPE_C_DRP_SRC_CNT_VAL_0,
	PMIC_REG_TYPE_C_DRP_SNK_CNT_VAL_0,
	PMIC_REG_TYPE_C_DRP_TRY_CNT_VAL_0,
	PMIC_REG_TYPE_C_DRP_TRY_WAIT_CNT_VAL_0,
	PMIC_REG_TYPE_C_DRP_TRY_WAIT_CNT_VAL_1,
	PMIC_REG_TYPE_C_CC_SRC_VOPEN_DEFAULT_DAC_VAL,
	PMIC_REG_TYPE_C_CC_SRC_VRD_DEFAULT_DAC_VAL,
	PMIC_REG_TYPE_C_CC_SRC_VOPEN_15_DAC_VAL,
	PMIC_REG_TYPE_C_CC_SRC_VRD_15_DAC_VAL,
	PMIC_REG_TYPE_C_CC_SRC_VOPEN_30_DAC_VAL,
	PMIC_REG_TYPE_C_CC_SRC_VRD_30_DAC_VAL,
	PMIC_REG_TYPE_C_CC_SNK_VRP30_DAC_VAL,
	PMIC_REG_TYPE_C_CC_SNK_VRP15_DAC_VAL,
	PMIC_REG_TYPE_C_CC_SNK_VRPUSB_DAC_VAL,
	PMIC_REG_TYPE_C_CC_ENT_ATTACH_SRC_INTR_EN,
	PMIC_REG_TYPE_C_CC_ENT_ATTACH_SNK_INTR_EN,
	PMIC_REG_TYPE_C_CC_ENT_AUDIO_ACC_INTR_EN,
	PMIC_REG_TYPE_C_CC_ENT_DBG_ACC_INTR_EN,
	PMIC_REG_TYPE_C_CC_ENT_DISABLE_INTR_EN,
	PMIC_REG_TYPE_C_CC_ENT_UNATTACH_SRC_INTR_EN,
	PMIC_REG_TYPE_C_CC_ENT_UNATTACH_SNK_INTR_EN,
	PMIC_REG_TYPE_C_CC_ENT_ATTACH_WAIT_SRC_INTR_EN,
	PMIC_REG_TYPE_C_CC_ENT_ATTACH_WAIT_SNK_INTR_EN,
	PMIC_REG_TYPE_C_CC_ENT_TRY_SRC_INTR_EN,
	PMIC_REG_TYPE_C_CC_ENT_TRY_WAIT_SNK_INTR_EN,
	PMIC_REG_TYPE_C_CC_ENT_UNATTACH_ACC_INTR_EN,
	PMIC_REG_TYPE_C_CC_ENT_ATTACH_WAIT_ACC_INTR_EN,
	PMIC_REG_TYPE_C_CC_ENT_SNK_PWR_IDLE_INTR_EN,
	PMIC_REG_TYPE_C_CC_ENT_SNK_PWR_DEFAULT_INTR_EN,
	PMIC_REG_TYPE_C_CC_ENT_SNK_PWR_15_INTR_EN,
	PMIC_REG_TYPE_C_CC_ENT_SNK_PWR_30_INTR_EN,
	PMIC_REG_TYPE_C_CC_ENT_SNK_PWR_REDETECT_INTR_EN,
	PMIC_TYPE_C_CC_ENT_ATTACH_SRC_INTR,
	PMIC_TYPE_C_CC_ENT_ATTACH_SNK_INTR,
	PMIC_TYPE_C_CC_ENT_AUDIO_ACC_INTR,
	PMIC_TYPE_C_CC_ENT_DBG_ACC_INTR,
	PMIC_TYPE_C_CC_ENT_DISABLE_INTR,
	PMIC_TYPE_C_CC_ENT_UNATTACH_SRC_INTR,
	PMIC_TYPE_C_CC_ENT_UNATTACH_SNK_INTR,
	PMIC_TYPE_C_CC_ENT_ATTACH_WAIT_SRC_INTR,
	PMIC_TYPE_C_CC_ENT_ATTACH_WAIT_SNK_INTR,
	PMIC_TYPE_C_CC_ENT_TRY_SRC_INTR,
	PMIC_TYPE_C_CC_ENT_TRY_WAIT_SNK_INTR,
	PMIC_TYPE_C_CC_ENT_UNATTACH_ACC_INTR,
	PMIC_TYPE_C_CC_ENT_ATTACH_WAIT_ACC_INTR,
	PMIC_TYPE_C_CC_ENT_SNK_PWR_IDLE_INTR,
	PMIC_TYPE_C_CC_ENT_SNK_PWR_DEFAULT_INTR,
	PMIC_TYPE_C_CC_ENT_SNK_PWR_15_INTR,
	PMIC_TYPE_C_CC_ENT_SNK_PWR_30_INTR,
	PMIC_TYPE_C_CC_ENT_SNK_PWR_REDETECT_INTR,
	PMIC_RO_TYPE_C_CC_ST,
	PMIC_RO_TYPE_C_ROUTED_CC,
	PMIC_RO_TYPE_C_CC_SNK_PWR_ST,
	PMIC_RO_TYPE_C_CC_PWR_ROLE,
	PMIC_RO_TYPE_C_DRIVE_VCONN_CAPABLE,
	PMIC_RO_TYPE_C_AD_CC_CMP_OUT,
	PMIC_RO_AD_CC_VUSB33_RDY,
	PMIC_REG_TYPE_C_PHY_RG_CC1_RPDE,
	PMIC_REG_TYPE_C_PHY_RG_CC1_RP15,
	PMIC_REG_TYPE_C_PHY_RG_CC1_RP3,
	PMIC_REG_TYPE_C_PHY_RG_CC1_RD,
	PMIC_REG_TYPE_C_PHY_RG_CC2_RPDE,
	PMIC_REG_TYPE_C_PHY_RG_CC2_RP15,
	PMIC_REG_TYPE_C_PHY_RG_CC2_RP3,
	PMIC_REG_TYPE_C_PHY_RG_CC2_RD,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_RCC1,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_RCC2,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_LEV_EN,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_SW_SEL,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_BIAS_EN,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_LPF_EN,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_ADCSW_EN,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_SASW_EN,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_DAC_EN,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_SACLK,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_DAC_IN,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_DAC_CAL,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_EN_DA_CC_DAC_GAIN_CAL,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RPCC1_EN,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RDCC1_EN,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RACC1_EN,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RPCC2_EN,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RDCC2_EN,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_RACC2_EN,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_LEV_EN,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_SW_SEL,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_BIAS_EN,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_LPF_EN,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_ADCSW_EN,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_SASW_EN,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_DAC_EN,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_SACLK,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_DAC_IN,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_DAC_CAL,
	PMIC_REG_TYPE_C_SW_FORCE_MODE_DA_CC_DAC_GAIN_CAL,
	PMIC_TYPE_C_DAC_CAL_START,
	PMIC_REG_TYPE_C_DAC_CAL_STAGE,
	PMIC_RO_TYPE_C_DAC_OK,
	PMIC_RO_TYPE_C_DAC_FAIL,
	PMIC_RO_DA_CC_DAC_CAL,
	PMIC_RO_DA_CC_DAC_GAIN_CAL,
	PMIC_REG_TYPE_C_DBG_PORT_SEL_0,
	PMIC_REG_TYPE_C_DBG_PORT_SEL_1,
	PMIC_REG_TYPE_C_DBG_MOD_SEL,
	PMIC_RO_TYPE_C_DBG_OUT_READ_0,
	PMIC_RO_TYPE_C_DBG_OUT_READ_1,
	PMIC_HWCID,
	PMIC_SWCID,
	PMIC_RG_SRCLKEN_IN0_EN,
	PMIC_RG_SRCLKEN_IN1_EN,
	PMIC_RG_OSC_SEL,
	PMIC_RG_SRCLKEN_IN2_EN,
	PMIC_RG_SRCLKEN_IN0_HW_MODE,
	PMIC_RG_SRCLKEN_IN1_HW_MODE,
	PMIC_RG_OSC_SEL_HW_MODE,
	PMIC_RG_SRCLKEN_IN_SYNC_EN,
	PMIC_RG_OSC_EN_AUTO_OFF,
	PMIC_TEST_OUT,
	PMIC_RG_MON_FLAG_SEL,
	PMIC_RG_MON_GRP_SEL,
	PMIC_RG_NANDTREE_MODE,
	PMIC_RG_TEST_AUXADC,
	PMIC_RG_EFUSE_MODE,
	PMIC_RG_TEST_STRUP,
	PMIC_TESTMODE_SW,
	PMIC_EN_STATUS_VDRAM,
	PMIC_EN_STATUS_VSRAM_PROC,
	PMIC_EN_STATUS_VAUD28,
	PMIC_EN_STATUS_VAUX18,
	PMIC_EN_STATUS_VCAMD,
	PMIC_EN_STATUS_VLDO28,
	PMIC_EN_STATUS_VCAMIO,
	PMIC_EN_STATUS_VCAMA,
	PMIC_EN_STATUS_VCN18,
	PMIC_EN_STATUS_VCN28,
	PMIC_EN_STATUS_VCN33,
	PMIC_EN_STATUS_VRF12,
	PMIC_EN_STATUS_VRF18,
	PMIC_EN_STATUS_VXO22,
	PMIC_EN_STATUS_VTCXO24,
	PMIC_EN_STATUS_VTCXO28,
	PMIC_EN_STATUS_VS1,
	PMIC_EN_STATUS_VCORE,
	PMIC_EN_STATUS_VPROC,
	PMIC_EN_STATUS_VPA,
	PMIC_EN_STATUS_VRTC,
	PMIC_EN_STATUS_TREF,
	PMIC_EN_STATUS_VIBR,
	PMIC_EN_STATUS_VIO18,
	PMIC_EN_STATUS_VEMC33,
	PMIC_EN_STATUS_VUSB33,
	PMIC_EN_STATUS_VMCH,
	PMIC_EN_STATUS_VMC,
	PMIC_EN_STATUS_VIO28,
	PMIC_EN_STATUS_VSIM2,
	PMIC_EN_STATUS_VSIM1,
	PMIC_OC_STATUS_VDRAM,
	PMIC_OC_STATUS_VSRAM_PROC,
	PMIC_OC_STATUS_VAUD28,
	PMIC_OC_STATUS_VAUX18,
	PMIC_OC_STATUS_VCAMD,
	PMIC_OC_STATUS_VLDO28,
	PMIC_OC_STATUS_VCAMIO,
	PMIC_OC_STATUS_VCAMA,
	PMIC_OC_STATUS_VCN18,
	PMIC_OC_STATUS_VCN28,
	PMIC_OC_STATUS_VCN33,
	PMIC_OC_STATUS_VRF12,
	PMIC_OC_STATUS_VRF18,
	PMIC_OC_STATUS_VXO22,
	PMIC_OC_STATUS_VTCXO24,
	PMIC_OC_STATUS_VTCXO28,
	PMIC_OC_STATUS_VS1,
	PMIC_OC_STATUS_VCORE,
	PMIC_OC_STATUS_VPROC,
	PMIC_OC_STATUS_VPA,
	PMIC_OC_STATUS_TREF,
	PMIC_OC_STATUS_VIBR,
	PMIC_OC_STATUS_VIO18,
	PMIC_OC_STATUS_VEMC33,
	PMIC_OC_STATUS_VUSB33,
	PMIC_OC_STATUS_VMCH,
	PMIC_OC_STATUS_VMC,
	PMIC_OC_STATUS_VIO28,
	PMIC_OC_STATUS_VSIM2,
	PMIC_OC_STATUS_VSIM1,
	PMIC_VCORE2_PG_DEB,
	PMIC_VS1_PG_DEB,
	PMIC_VCORE_PG_DEB,
	PMIC_VPROC_PG_DEB,
	PMIC_VIO18_PG_DEB,
	PMIC_VEMC33_PG_DEB,
	PMIC_VUSB33_PG_DEB,
	PMIC_VMCH_PG_DEB,
	PMIC_VMC_PG_DEB,
	PMIC_VIO28_PG_DEB,
	PMIC_VDRAM_PG_DEB,
	PMIC_VSRAM_PROC_PG_DEB,
	PMIC_VAUD28_PG_DEB,
	PMIC_VAUX18_PG_DEB,
	PMIC_VXO22_PG_DEB,
	PMIC_VTCXO24_PG_DEB,
	PMIC_STRUP_VMCH_PG_STATUS_GATED,
	PMIC_STRUP_VMC_PG_STATUS_GATED,
	PMIC_STRUP_VXO22_PG_STATUS_GATED,
	PMIC_STRUP_VTCXO24_PG_STATUS_GATED,
	PMIC_STRUP_VUSB33_PG_STATUS_GATED,
	PMIC_STRUP_VSRAM_PROC_PG_STATUS_GATED,
	PMIC_STRUP_VPROC_PG_STATUS_GATED,
	PMIC_STRUP_VDRAM_PG_STATUS_GATED,
	PMIC_STRUP_VAUD28_PG_STATUS_GATED,
	PMIC_STRUP_VIO28_PG_STATUS_GATED,
	PMIC_STRUP_VEMC33_PG_STATUS_GATED,
	PMIC_STRUP_VIO18_PG_STATUS_GATED,
	PMIC_STRUP_VS1_PG_STATUS_GATED,
	PMIC_STRUP_VCORE_PG_STATUS_GATED,
	PMIC_STRUP_VAUX18_PG_STATUS_GATED,
	PMIC_STRUP_VCORE2_PG_STATUS_GATED,
	PMIC_PMU_THERMAL_DEB,
	PMIC_STRUP_THERMAL_STATUS,
	PMIC_PMU_TEST_MODE_SCAN,
	PMIC_PWRKEY_DEB,
	PMIC_HOMEKEY_DEB,
	PMIC_RTC_XTAL_DET_DONE,
	PMIC_XOSC32_ENB_DET,
	PMIC_RTC_XTAL_DET_RSV,
	PMIC_RG_PMU_TDSEL,
	PMIC_RG_SPI_TDSEL,
	PMIC_RG_AUD_TDSEL,
	PMIC_RG_E32CAL_TDSEL,
	PMIC_RG_PMU_RDSEL,
	PMIC_RG_SPI_RDSEL,
	PMIC_RG_AUD_RDSEL,
	PMIC_RG_E32CAL_RDSEL,
	PMIC_RG_SMT_WDTRSTB_IN,
	PMIC_RG_SMT_HOMEKEY,
	PMIC_RG_SMT_SRCLKEN_IN0,
	PMIC_RG_SMT_SRCLKEN_IN1,
	PMIC_RG_SMT_RTC_32K1V8_0,
	PMIC_RG_SMT_RTC_32K1V8_1,
	PMIC_RG_SMT_SPI_CLK,
	PMIC_RG_SMT_SPI_CSN,
	PMIC_RG_SMT_SPI_MOSI,
	PMIC_RG_SMT_SPI_MISO,
	PMIC_RG_SMT_AUD_CLK,
	PMIC_RG_SMT_AUD_DAT_MOSI,
	PMIC_RG_SMT_AUD_DAT_MISO,
	PMIC_RG_SMT_ANC_DAT_MOSI,
	PMIC_RG_SMT_VOW_CLK_MISO,
	PMIC_RG_SMT_ENBB,
	PMIC_RG_SMT_XOSC_EN,
	PMIC_RG_OCTL_SRCLKEN_IN0,
	PMIC_RG_OCTL_SRCLKEN_IN1,
	PMIC_RG_OCTL_RTC_32K1V8_0,
	PMIC_RG_OCTL_RTC_32K1V8_1,
	PMIC_RG_OCTL_SPI_CLK,
	PMIC_RG_OCTL_SPI_CSN,
	PMIC_RG_OCTL_SPI_MOSI,
	PMIC_RG_OCTL_SPI_MISO,
	PMIC_RG_OCTL_AUD_DAT_MOSI,
	PMIC_RG_OCTL_AUD_DAT_MISO,
	PMIC_RG_OCTL_AUD_CLK,
	PMIC_RG_OCTL_ANC_DAT_MOSI,
	PMIC_RG_OCTL_HOMEKEY,
	PMIC_RG_OCTL_ENBB,
	PMIC_RG_OCTL_XOSC_EN,
	PMIC_RG_OCTL_VOW_CLK_MISO,
	PMIC_TOP_STATUS,
	PMIC_TOP_STATUS_SET,
	PMIC_TOP_STATUS_CLR,
	PMIC_CLK_RSV_CON0_RSV,
	PMIC_RG_DCXO_PWRKEY_RSTB_SEL,
	PMIC_CLK_SRCVOLTEN_SW,
	PMIC_CLK_BUCK_OSC_SEL_SW,
	PMIC_CLK_VOWEN_SW,
	PMIC_CLK_SRCVOLTEN_MODE,
	PMIC_CLK_BUCK_OSC_SEL_MODE,
	PMIC_CLK_VOWEN_MODE,
	PMIC_RG_TOP_CKSEL_CON2_RSV,
	PMIC_CLK_BUCK_CON0_RSV,
	PMIC_CLK_BUCK_CON0_SET,
	PMIC_CLK_BUCK_CON0_CLR,
	PMIC_CLK_BUCK_VCORE_OSC_SEL_SW,
	PMIC_CLK_BUCK_VCORE2_OSC_SEL_SW,
	PMIC_CLK_BUCK_VPROC_OSC_SEL_SW,
	PMIC_CLK_BUCK_VPA_OSC_SEL_SW,
	PMIC_CLK_BUCK_VS1_OSC_SEL_SW,
	PMIC_CLK_LDO_VSRAM_PROC_OSC_SEL_SW,
	PMIC_CLK_BUCK_VCORE_OSC_SEL_MODE,
	PMIC_CLK_BUCK_VCORE2_OSC_SEL_MODE,
	PMIC_CLK_BUCK_VPROC_OSC_SEL_MODE,
	PMIC_CLK_BUCK_VPA_OSC_SEL_MODE,
	PMIC_CLK_BUCK_VS1_OSC_SEL_MODE,
	PMIC_CLK_LDO_VSRAM_PROC_OSC_SEL_MODE,
	PMIC_CLK_BUCK_CON1_RSV,
	PMIC_CLK_BUCK_CON1_SET,
	PMIC_CLK_BUCK_CON1_CLR,
	PMIC_RG_CLKSQ_EN_AUD,
	PMIC_RG_CLKSQ_EN_FQR,
	PMIC_RG_CLKSQ_EN_AUX_AP,
	PMIC_RG_CLKSQ_EN_AUX_MD,
	PMIC_RG_CLKSQ_EN_AUX_GPS,
	PMIC_RG_CLKSQ_EN_AUX_RSV,
	PMIC_RG_CLKSQ_EN_AUX_AP_MODE,
	PMIC_RG_CLKSQ_EN_AUX_MD_MODE,
	PMIC_RG_CLKSQ_IN_SEL_VA18,
	PMIC_RG_CLKSQ_IN_SEL_VA18_SWCTRL,
	PMIC_TOP_CLKSQ_RSV,
	PMIC_DA_CLKSQ_EN_VA28,
	PMIC_TOP_CLKSQ_SET,
	PMIC_TOP_CLKSQ_CLR,
	PMIC_RG_CLKSQ_RTC_EN,
	PMIC_RG_CLKSQ_RTC_EN_HW_MODE,
	PMIC_TOP_CLKSQ_RTC_RSV0,
	PMIC_RG_ENBB_SEL,
	PMIC_RG_XOSC_EN_SEL,
	PMIC_TOP_CLKSQ_RTC_RSV1,
	PMIC_DA_CLKSQ_EN_VDIG18,
	PMIC_TOP_CLKSQ_RTC_SET,
	PMIC_TOP_CLKSQ_RTC_CLR,
	PMIC_OSC_75K_TRIM,
	PMIC_RG_OSC_75K_TRIM_EN,
	PMIC_RG_OSC_75K_TRIM_RATE,
	PMIC_DA_OSC_75K_TRIM,
	PMIC_CLK_PMU75K_CK_TST_DIS,
	PMIC_CLK_SMPS_CK_TST_DIS,
	PMIC_CLK_AUD26M_CK_TST_DIS,
	PMIC_CLK_VOW12M_CK_TST_DIS,
	PMIC_CLK_RTC32K_CK_TST_DIS,
	PMIC_CLK_RTC26M_CK_TST_DIS,
	PMIC_CLK_FG_CK_TST_DIS,
	PMIC_CLK_SPK_CK_TST_DIS,
	PMIC_CLK_CKROOTTST_CON0_RSV,
	PMIC_CLK_BUCK_ANA_AUTO_OFF_DIS,
	PMIC_CLK_PMU75K_CK_TSTSEL,
	PMIC_CLK_SMPS_CK_TSTSEL,
	PMIC_CLK_AUD26M_CK_TSTSEL,
	PMIC_CLK_VOW12M_CK_TSTSEL,
	PMIC_CLK_RTC32K_CK_TSTSEL,
	PMIC_CLK_RTC26M_CK_TSTSEL,
	PMIC_CLK_FG_CK_TSTSEL,
	PMIC_CLK_SPK_CK_TSTSEL,
	PMIC_CLK_CKROOTTST_CON1_RSV,
	PMIC_CLK_RTC32K_1V8_0_O_PDN,
	PMIC_CLK_RTC32K_1V8_1_O_PDN,
	PMIC_CLK_RTC_2SEC_OFF_DET_PDN,
	PMIC_CLK_PDN_CON0_RSV,
	PMIC_CLK_PDN_CON0_SET,
	PMIC_CLK_PDN_CON0_CLR,
	PMIC_CLK_ZCD13M_CK_PDN,
	PMIC_CLK_SMPS_CK_DIV_PDN,
	PMIC_CLK_BUCK_ANA_CK_PDN,
	PMIC_CLK_PDN_CON1_RSV,
	PMIC_CLK_PDN_CON1_SET,
	PMIC_CLK_PDN_CON1_CLR,
	PMIC_CLK_OSC_SEL_HW_SRC_SEL,
	PMIC_CLK_SRCLKEN_SRC_SEL,
	PMIC_CLK_SEL_CON0_RSV,
	PMIC_CLK_SEL_CON0_SET,
	PMIC_CLK_SEL_CON0_CLR,
	PMIC_CLK_AUXADC_CK_CKSEL_HWEN,
	PMIC_CLK_AUXADC_CK_CKSEL,
	PMIC_CLK_75K_32K_SEL,
	PMIC_CLK_SEL_CON1_RSV,
	PMIC_CLK_SEL_CON1_SET,
	PMIC_CLK_SEL_CON1_CLR,
	PMIC_CLK_G_SMPS_PD_CK_PDN,
	PMIC_CLK_G_SMPS_AUD_CK_PDN,
	PMIC_CLK_G_SMPS_TEST_CK_PDN,
	PMIC_CLK_ACCDET_CK_PDN,
	PMIC_CLK_AUDIF_CK_PDN,
	PMIC_CLK_AUD_CK_PDN,
	PMIC_CLK_AUDNCP_CK_PDN,
	PMIC_CLK_AUXADC_1M_CK_PDN,
	PMIC_CLK_AUXADC_SMPS_CK_PDN,
	PMIC_CLK_AUXADC_RNG_CK_PDN,
	PMIC_CLK_AUXADC_26M_CK_PDN,
	PMIC_CLK_AUXADC_CK_PDN,
	PMIC_CLK_DRV_ISINK0_CK_PDN,
	PMIC_CLK_DRV_ISINK1_CK_PDN,
	PMIC_CLK_DRV_ISINK2_CK_PDN,
	PMIC_CLK_DRV_ISINK3_CK_PDN,
	PMIC_CLK_CKPDN_CON0_SET,
	PMIC_CLK_CKPDN_CON0_CLR,
	PMIC_CLK_DRV_CHRIND_CK_PDN,
	PMIC_CLK_DRV_32K_CK_PDN,
	PMIC_CLK_BUCK_1M_CK_PDN,
	PMIC_CLK_BUCK_32K_CK_PDN,
	PMIC_CLK_BUCK_9M_CK_PDN,
	PMIC_CLK_PWMOC_6M_CK_PDN,
	PMIC_CLK_BUCK_VPROC_9M_CK_PDN,
	PMIC_CLK_BUCK_VCORE_9M_CK_PDN,
	PMIC_CLK_BUCK_VCORE2_9M_CK_PDN,
	PMIC_CLK_BUCK_VPA_9M_CK_PDN,
	PMIC_CLK_BUCK_VS1_9M_CK_PDN,
	PMIC_CLK_FQMTR_CK_PDN,
	PMIC_CLK_FQMTR_32K_CK_PDN,
	PMIC_CLK_INTRP_CK_PDN,
	PMIC_CLK_INTRP_PRE_OC_CK_PDN,
	PMIC_CLK_STB_1M_CK_PDN,
	PMIC_CLK_CKPDN_CON1_SET,
	PMIC_CLK_CKPDN_CON1_CLR,
	PMIC_CLK_LDO_CALI_75K_CK_PDN,
	PMIC_CLK_LDO_VSRAM_PROC_9M_CK_PDN,
	PMIC_CLK_RTC_26M_CK_PDN,
	PMIC_CLK_RTC_32K_CK_PDN,
	PMIC_CLK_RTC_MCLK_PDN,
	PMIC_CLK_RTC_75K_CK_PDN,
	PMIC_CLK_RTCDET_CK_PDN,
	PMIC_CLK_RTC_EOSC32_CK_PDN,
	PMIC_CLK_EOSC_CALI_TEST_CK_PDN,
	PMIC_CLK_FGADC_ANA_CK_PDN,
	PMIC_CLK_FGADC_DIG_CK_PDN,
	PMIC_CLK_FGADC_FT_CK_PDN,
	PMIC_CLK_TRIM_75K_CK_PDN,
	PMIC_CLK_EFUSE_CK_PDN,
	PMIC_CLK_STRUP_75K_CK_PDN,
	PMIC_CLK_STRUP_32K_CK_PDN,
	PMIC_CLK_CKPDN_CON2_SET,
	PMIC_CLK_CKPDN_CON2_CLR,
	PMIC_CLK_PCHR_32K_CK_PDN,
	PMIC_CLK_PCHR_TEST_CK_PDN,
	PMIC_CLK_SPI_CK_PDN,
	PMIC_CLK_BGR_TEST_CK_PDN,
	PMIC_CLK_TYPE_C_CC_CK_PDN,
	PMIC_CLK_TYPE_C_CSR_CK_PDN,
	PMIC_CLK_SPK_CK_PDN,
	PMIC_CLK_SPK_PWM_CK_PDN,
	PMIC_CLK_RSV_PDN,
	PMIC_CLK_CKPDN_CON3_SET,
	PMIC_CLK_CKPDN_CON3_CLR,
	PMIC_CLK_G_SMPS_PD_CK_PDN_HWEN,
	PMIC_CLK_G_SMPS_AUD_CK_PDN_HWEN,
	PMIC_CLK_AUXADC_SMPS_CK_PDN_HWEN,
	PMIC_CLK_AUXADC_26M_CK_PDN_HWEN,
	PMIC_CLK_AUXADC_CK_PDN_HWEN,
	PMIC_CLK_BUCK_1M_CK_PDN_HWEN,
	PMIC_CLK_BUCK_VPROC_9M_CK_PDN_HWEN,
	PMIC_CLK_BUCK_VCORE_9M_CK_PDN_HWEN,
	PMIC_CLK_BUCK_VCORE2_9M_CK_PDN_HWEN,
	PMIC_CLK_BUCK_VPA_9M_CK_PDN_HWEN,
	PMIC_CLK_BUCK_VS1_9M_CK_PDN_HWEN,
	PMIC_CLK_STB_1M_CK_PDN_HWEN,
	PMIC_CLK_LDO_VSRAM_PROC_9M_CK_PDN_HWEN,
	PMIC_CLK_RTC_26M_CK_PDN_HWEN,
	PMIC_CLK_RSV_PDN_HWEN,
	PMIC_CLK_CKPDN_HWEN_CON0_SET,
	PMIC_CLK_CKPDN_HWEN_CON0_CLR,
	PMIC_CLK_AUDIF_CK_CKSEL,
	PMIC_CLK_AUD_CK_CKSEL,
	PMIC_CLK_FQMTR_CK_CKSEL,
	PMIC_CLK_FGADC_ANA_CK_CKSEL,
	PMIC_CLK_PCHR_TEST_CK_CKSEL,
	PMIC_CLK_BGR_TEST_CK_CKSEL,
	PMIC_CLK_EFUSE_CK_PDN_HWEN,
	PMIC_CLK_RSV_CKSEL,
	PMIC_CLK_CKSEL_CON0_SET,
	PMIC_CLK_CKSEL_CON0_CLR,
	PMIC_CLK_AUXADC_SMPS_CK_DIVSEL,
	PMIC_CLK_AUXADC_26M_CK_DIVSEL,
	PMIC_CLK_BUCK_9M_CK_DIVSEL,
	PMIC_CLK_REG_CK_DIVSEL,
	PMIC_CLK_SPK_CK_DIVSEL,
	PMIC_CLK_SPK_PWM_CK_DIVSEL,
	PMIC_CLK_RSV_DIVSEL,
	PMIC_CLK_CKDIVSEL_CON0_SET,
	PMIC_CLK_CKDIVSEL_CON0_CLR,
	PMIC_CLK_AUDIF_CK_TSTSEL,
	PMIC_CLK_AUD_CK_TSTSEL,
	PMIC_CLK_AUXADC_SMPS_CK_TSTSEL,
	PMIC_CLK_AUXADC_26M_CK_TSTSEL,
	PMIC_CLK_AUXADC_CK_TSTSEL,
	PMIC_CLK_DRV_CHRIND_CK_TSTSEL,
	PMIC_CLK_FQMTR_CK_TSTSEL,
	PMIC_CLK_RTCDET_CK_TSTSEL,
	PMIC_CLK_RTC_EOSC32_CK_TSTSEL,
	PMIC_CLK_EOSC_CALI_TEST_CK_TSTSEL,
	PMIC_CLK_FGADC_ANA_CK_TSTSEL,
	PMIC_CLK_PCHR_TEST_CK_TSTSEL,
	PMIC_CLK_BGR_TEST_CK_TSTSEL,
	PMIC_CLK_RSV_TSTSEL,
	PMIC_RG_EFUSE_MAN_RST,
	PMIC_RG_AUXADC_RST,
	PMIC_RG_AUXADC_REG_RST,
	PMIC_RG_AUDIO_RST,
	PMIC_RG_ACCDET_RST,
	PMIC_RG_BIF_RST,
	PMIC_RG_DRIVER_RST,
	PMIC_RG_FGADC_RST,
	PMIC_RG_FQMTR_RST,
	PMIC_RG_RTC_RST,
	PMIC_RG_TYPE_C_CC_RST,
	PMIC_RG_CHRWDT_RST,
	PMIC_RG_ZCD_RST,
	PMIC_RG_AUDNCP_RST,
	PMIC_RG_CLK_TRIM_RST,
	PMIC_RG_BUCK_SRCLKEN_RST,
	PMIC_TOP_RST_CON0_SET,
	PMIC_TOP_RST_CON0_CLR,
	PMIC_RG_STRUP_LONG_PRESS_RST,
	PMIC_RG_BUCK_PROT_PMPP_RST,
	PMIC_RG_SPK_RST,
	PMIC_TOP_RST_CON1_RSV,
	PMIC_TOP_RST_CON1_SET,
	PMIC_TOP_RST_CON1_CLR,
	PMIC_RG_CHR_LDO_DET_MODE,
	PMIC_RG_CHR_LDO_DET_SW,
	PMIC_RG_CHRWDT_FLAG_MODE,
	PMIC_RG_CHRWDT_FLAG_SW,
	PMIC_TOP_RST_CON2_RSV,
	PMIC_RG_WDTRSTB_EN,
	PMIC_RG_WDTRSTB_MODE,
	PMIC_WDTRSTB_STATUS,
	PMIC_WDTRSTB_STATUS_CLR,
	PMIC_RG_WDTRSTB_FB_EN,
	PMIC_RG_WDTRSTB_DEB,
	PMIC_RG_HOMEKEY_RST_EN,
	PMIC_RG_PWRKEY_RST_EN,
	PMIC_RG_PWRRST_TMR_DIS,
	PMIC_RG_PWRKEY_RST_TD,
	PMIC_TOP_RST_MISC_RSV,
	PMIC_TOP_RST_MISC_SET,
	PMIC_TOP_RST_MISC_CLR,
	PMIC_VPWRIN_RSTB_STATUS,
	PMIC_DDLO_RSTB_STATUS,
	PMIC_UVLO_RSTB_STATUS,
	PMIC_RTC_DDLO_RSTB_STATUS,
	PMIC_CHRWDT_REG_RSTB_STATUS,
	PMIC_CHRDET_REG_RSTB_STATUS,
	PMIC_TOP_RST_STATUS_RSV,
	PMIC_TOP_RST_STATUS_SET,
	PMIC_TOP_RST_STATUS_CLR,
	PMIC_TOP_RST_RSV_CON0,
	PMIC_TOP_RST_RSV_CON1,
	PMIC_RG_INT_EN_PWRKEY,
	PMIC_RG_INT_EN_HOMEKEY,
	PMIC_RG_INT_EN_PWRKEY_R,
	PMIC_RG_INT_EN_HOMEKEY_R,
	PMIC_RG_INT_EN_THR_H,
	PMIC_RG_INT_EN_THR_L,
	PMIC_RG_INT_EN_BAT_H,
	PMIC_RG_INT_EN_BAT_L,
	PMIC_RG_INT_EN_RTC,
	PMIC_RG_INT_EN_AUDIO,
	PMIC_RG_INT_EN_ACCDET,
	PMIC_RG_INT_EN_ACCDET_EINT,
	PMIC_RG_INT_EN_ACCDET_NEGV,
	PMIC_RG_INT_EN_NI_LBAT_INT,
	PMIC_INT_CON0_SET,
	PMIC_INT_CON0_CLR,
	PMIC_RG_INT_EN_VCORE_OC,
	PMIC_RG_INT_EN_VPROC_OC,
	PMIC_RG_INT_EN_VS1_OC,
	PMIC_RG_INT_EN_VPA_OC,
	PMIC_RG_INT_EN_SPKL_D,
	PMIC_RG_INT_EN_SPKL_AB,
	PMIC_RG_INT_EN_LDO_OC,
	PMIC_INT_CON1_SET,
	PMIC_INT_CON1_CLR,
	PMIC_RG_INT_EN_TYPE_C_L_MIN,
	PMIC_RG_INT_EN_TYPE_C_L_MAX,
	PMIC_RG_INT_EN_TYPE_C_H_MIN,
	PMIC_RG_INT_EN_TYPE_C_H_MAX,
	PMIC_RG_INT_EN_AUXADC_IMP,
	PMIC_RG_INT_EN_NAG_C_DLTV,
	PMIC_RG_INT_EN_TYPE_C_CC_IRQ,
	PMIC_RG_INT_EN_CHRDET_EDGE,
	PMIC_RG_INT_EN_OV,
	PMIC_RG_INT_EN_BVALID_DET,
	PMIC_RG_INT_EN_RGS_BATON_HV,
	PMIC_RG_INT_EN_VBATON_UNDET,
	PMIC_RG_INT_EN_WATCHDOG,
	PMIC_RG_INT_EN_PCHR_CM_VDEC,
	PMIC_RG_INT_EN_CHRDET,
	PMIC_RG_INT_EN_PCHR_CM_VINC,
	PMIC_INT_CON2_SET,
	PMIC_INT_CON2_CLR,
	PMIC_RG_INT_EN_FG_BAT_H,
	PMIC_RG_INT_EN_FG_BAT_L,
	PMIC_RG_INT_EN_FG_CUR_H,
	PMIC_RG_INT_EN_FG_CUR_L,
	PMIC_RG_INT_EN_FG_ZCV,
	PMIC_INT_CON3_SET,
	PMIC_INT_CON3_CLR,
	PMIC_POLARITY,
	PMIC_RG_HOMEKEY_INT_SEL,
	PMIC_RG_PWRKEY_INT_SEL,
	PMIC_RG_CHRDET_INT_SEL,
	PMIC_RG_PCHR_CM_VINC_POLARITY_RSV,
	PMIC_RG_PCHR_CM_VDEC_POLARITY_RSV,
	PMIC_INT_MISC_CON_SET,
	PMIC_INT_MISC_CON_CLR,
	PMIC_RG_INT_STATUS_PWRKEY,
	PMIC_RG_INT_STATUS_HOMEKEY,
	PMIC_RG_INT_STATUS_PWRKEY_R,
	PMIC_RG_INT_STATUS_HOMEKEY_R,
	PMIC_RG_INT_STATUS_THR_H,
	PMIC_RG_INT_STATUS_THR_L,
	PMIC_RG_INT_STATUS_BAT_H,
	PMIC_RG_INT_STATUS_BAT_L,
	PMIC_RG_INT_STATUS_RTC,
	PMIC_RG_INT_STATUS_AUDIO,
	PMIC_RG_INT_STATUS_ACCDET,
	PMIC_RG_INT_STATUS_ACCDET_EINT,
	PMIC_RG_INT_STATUS_ACCDET_NEGV,
	PMIC_RG_INT_STATUS_NI_LBAT_INT,
	PMIC_RG_INT_STATUS_VCORE_OC,
	PMIC_RG_INT_STATUS_VPROC_OC,
	PMIC_RG_INT_STATUS_VS1_OC,
	PMIC_RG_INT_STATUS_VPA_OC,
	PMIC_RG_INT_STATUS_SPKL_D,
	PMIC_RG_INT_STATUS_SPKL_AB,
	PMIC_RG_INT_STATUS_LDO_OC,
	PMIC_RG_INT_STATUS_TYPE_C_L_MIN,
	PMIC_RG_INT_STATUS_TYPE_C_L_MAX,
	PMIC_RG_INT_STATUS_TYPE_C_H_MIN,
	PMIC_RG_INT_STATUS_TYPE_C_H_MAX,
	PMIC_RG_INT_STATUS_AUXADC_IMP,
	PMIC_RG_INT_STATUS_NAG_C_DLTV,
	PMIC_RG_INT_STATUS_TYPE_C_CC_IRQ,
	PMIC_RG_INT_STATUS_CHRDET_EDGE,
	PMIC_RG_INT_STATUS_OV,
	PMIC_RG_INT_STATUS_BVALID_DET,
	PMIC_RG_INT_STATUS_RGS_BATON_HV,
	PMIC_RG_INT_STATUS_VBATON_UNDET,
	PMIC_RG_INT_STATUS_WATCHDOG,
	PMIC_RG_INT_STATUS_PCHR_CM_VDEC,
	PMIC_RG_INT_STATUS_CHRDET,
	PMIC_RG_INT_STATUS_PCHR_CM_VINC,
	PMIC_RG_INT_STATUS_FG_BAT_H,
	PMIC_RG_INT_STATUS_FG_BAT_L,
	PMIC_RG_INT_STATUS_FG_CUR_H,
	PMIC_RG_INT_STATUS_FG_CUR_L,
	PMIC_RG_INT_STATUS_FG_ZCV,
	PMIC_OC_GEAR_LDO,
	PMIC_SPK_EN_L,
	PMIC_SPKMODE_L,
	PMIC_SPK_TRIM_EN_L,
	PMIC_SPK_OC_SHDN_DL,
	PMIC_SPK_THER_SHDN_L_EN,
	PMIC_SPK_OUT_STAGE_SEL,
	PMIC_RG_SPK_GAINL,
	PMIC_DA_SPK_OFFSET_L,
	PMIC_DA_SPK_LEAD_DGLH_L,
	PMIC_AD_NI_SPK_LEAD_L,
	PMIC_SPK_OFFSET_L_OV,
	PMIC_SPK_OFFSET_L_SW,
	PMIC_SPK_LEAD_L_SW,
	PMIC_SPK_OFFSET_L_MODE,
	PMIC_SPK_TRIM_DONE_L,
	PMIC_RG_SPK_INTG_RST_L,
	PMIC_RG_SPK_FORCE_EN_L,
	PMIC_RG_SPK_SLEW_L,
	PMIC_RG_SPKAB_OBIAS_L,
	PMIC_RG_SPKRCV_EN_L,
	PMIC_RG_SPK_DRC_EN_L,
	PMIC_RG_SPK_TEST_EN_L,
	PMIC_RG_SPKAB_OC_EN_L,
	PMIC_RG_SPK_OC_EN_L,
	PMIC_SPK_EN_R,
	PMIC_SPKMODE_R,
	PMIC_SPK_TRIM_EN_R,
	PMIC_SPK_OC_SHDN_DR,
	PMIC_SPK_THER_SHDN_R_EN,
	PMIC_RG_SPK_GAINR,
	PMIC_DA_SPK_OFFSET_R,
	PMIC_DA_SPK_LEAD_DGLH_R,
	PMIC_NI_SPK_LEAD_R,
	PMIC_SPK_OFFSET_R_OV,
	PMIC_SPK_OFFSET_R_SW,
	PMIC_SPK_LEAD_R_SW,
	PMIC_SPK_OFFSET_R_MODE,
	PMIC_SPK_TRIM_DONE_R,
	PMIC_RG_SPK_INTG_RST_R,
	PMIC_RG_SPK_FORCE_EN_R,
	PMIC_RG_SPK_SLEW_R,
	PMIC_RG_SPKAB_OBIAS_R,
	PMIC_RG_SPKRCV_EN_R,
	PMIC_RG_SPK_DRC_EN_R,
	PMIC_RG_SPK_TEST_EN_R,
	PMIC_RG_SPKAB_OC_EN_R,
	PMIC_RG_SPK_OC_EN_R,
	PMIC_RG_SPKPGA_GAINR,
	PMIC_SPK_TRIM_WND,
	PMIC_SPK_TRIM_THD,
	PMIC_SPK_OC_WND,
	PMIC_SPK_OC_THD,
	PMIC_SPK_D_OC_R_DEG,
	PMIC_SPK_AB_OC_R_DEG,
	PMIC_SPK_D_OC_L_DEG,
	PMIC_SPK_AB_OC_L_DEG,
	PMIC_SPK_TD1,
	PMIC_SPK_TD2,
	PMIC_SPK_TD3,
	PMIC_SPK_TRIM_DIV,
	PMIC_RG_BTL_SET,
	PMIC_RG_SPK_IBIAS_SEL,
	PMIC_RG_SPK_CCODE,
	PMIC_RG_SPK_EN_VIEW_VCM,
	PMIC_RG_SPK_EN_VIEW_CLK,
	PMIC_RG_SPK_VCM_SEL,
	PMIC_RG_SPK_VCM_IBSEL,
	PMIC_RG_SPK_FBRC_EN,
	PMIC_RG_SPKAB_OVDRV,
	PMIC_RG_SPK_OCTH_D,
	PMIC_RG_SPKPGA_GAINL,
	PMIC_SPK_RSV0,
	PMIC_SPK_VCM_FAST_EN,
	PMIC_SPK_TEST_MODE0,
	PMIC_SPK_TEST_MODE1,
	PMIC_SPK_TD_WAIT,
	PMIC_SPK_TD_DONE,
	PMIC_SPK_EN_MODE,
	PMIC_SPK_VCM_FAST_SW,
	PMIC_SPK_RST_R_SW,
	PMIC_SPK_RST_L_SW,
	PMIC_SPKMODE_R_SW,
	PMIC_SPKMODE_L_SW,
	PMIC_SPK_DEPOP_EN_R_SW,
	PMIC_SPK_DEPOP_EN_L_SW,
	PMIC_SPK_EN_R_SW,
	PMIC_SPK_EN_L_SW,
	PMIC_SPK_OUTSTG_EN_R_SW,
	PMIC_SPK_OUTSTG_EN_L_SW,
	PMIC_SPK_TRIM_EN_R_SW,
	PMIC_SPK_TRIM_EN_L_SW,
	PMIC_SPK_TRIM_STOP_R_SW,
	PMIC_SPK_TRIM_STOP_L_SW,
	PMIC_RG_SPK_ISENSE_TEST_EN,
	PMIC_RG_SPK_ISENSE_REFSEL,
	PMIC_RG_SPK_ISENSE_GAINSEL,
	PMIC_RG_SPK_ISENSE_PDRESET,
	PMIC_RG_SPK_ISENSE_EN,
	PMIC_RG_SPK_RSV1,
	PMIC_RG_SPK_RSV0,
	PMIC_RG_SPK_ABD_VOLSEN_GAIN,
	PMIC_RG_SPK_ABD_VOLSEN_EN,
	PMIC_RG_SPK_ABD_CURSEN_SEL,
	PMIC_RG_SPK_RSV2,
	PMIC_RG_SPK_TRIM2,
	PMIC_RG_SPK_TRIM1,
	PMIC_RG_SPK_D_CURSEN_RSETSEL,
	PMIC_RG_SPK_D_CURSEN_GAIN,
	PMIC_RG_SPK_D_CURSEN_EN,
	PMIC_RG_SPK_AB_CURSEN_RSETSEL,
	PMIC_RG_SPK_AB_CURSEN_GAIN,
	PMIC_RG_SPK_AB_CURSEN_EN,
	PMIC_RG_SPKPGA_GAIN,
	PMIC_RG_SPK_RSV,
	PMIC_RG_ISENSE_PD_RESET,
	PMIC_RG_AUDIVLPWRUP_VAUDP12,
	PMIC_RG_AUDIVLSTARTUP_VAUDP12,
	PMIC_RG_AUDIVLMUXSEL_VAUDP12,
	PMIC_RG_AUDIVLMUTE_VAUDP12,
	PMIC_FQMTR_TCKSEL,
	PMIC_FQMTR_BUSY,
	PMIC_FQMTR_DCXO26M_EN,
	PMIC_FQMTR_EN,
	PMIC_FQMTR_WINSET,
	PMIC_FQMTR_DATA,
	PMIC_RG_TRIM_EN,
	PMIC_RG_TRIM_SEL,
	PMIC_RG_ISINKS_RSV,
	PMIC_RG_ISINK0_DOUBLE_EN,
	PMIC_RG_ISINK1_DOUBLE_EN,
	PMIC_RG_ISINK2_DOUBLE_EN,
	PMIC_RG_ISINK3_DOUBLE_EN,
	PMIC_ISINK_DIM0_FSEL,
	PMIC_ISINK0_RSV1,
	PMIC_ISINK0_RSV0,
	PMIC_ISINK_DIM0_DUTY,
	PMIC_ISINK_CH0_STEP,
	PMIC_ISINK_BREATH0_TF2_SEL,
	PMIC_ISINK_BREATH0_TF1_SEL,
	PMIC_ISINK_BREATH0_TR2_SEL,
	PMIC_ISINK_BREATH0_TR1_SEL,
	PMIC_ISINK_BREATH0_TOFF_SEL,
	PMIC_ISINK_BREATH0_TON_SEL,
	PMIC_ISINK_DIM1_FSEL,
	PMIC_ISINK1_RSV1,
	PMIC_ISINK1_RSV0,
	PMIC_ISINK_DIM1_DUTY,
	PMIC_ISINK_CH1_STEP,
	PMIC_ISINK_BREATH1_TF2_SEL,
	PMIC_ISINK_BREATH1_TF1_SEL,
	PMIC_ISINK_BREATH1_TR2_SEL,
	PMIC_ISINK_BREATH1_TR1_SEL,
	PMIC_ISINK_BREATH1_TOFF_SEL,
	PMIC_ISINK_BREATH1_TON_SEL,
	PMIC_AD_NI_ISINK3_STATUS,
	PMIC_AD_NI_ISINK2_STATUS,
	PMIC_AD_NI_ISINK1_STATUS,
	PMIC_AD_NI_ISINK0_STATUS,
	PMIC_ISINK_PHASE0_DLY_EN,
	PMIC_ISINK_PHASE1_DLY_EN,
	PMIC_ISINK_PHASE_DLY_TC,
	PMIC_ISINK_CHOP0_SW,
	PMIC_ISINK_CHOP1_SW,
	PMIC_ISINK_SFSTR1_EN,
	PMIC_ISINK_SFSTR1_TC,
	PMIC_ISINK_SFSTR0_EN,
	PMIC_ISINK_SFSTR0_TC,
	PMIC_ISINK_CH0_EN,
	PMIC_ISINK_CH1_EN,
	PMIC_ISINK_CHOP0_EN,
	PMIC_ISINK_CHOP1_EN,
	PMIC_ISINK_CH0_BIAS_EN,
	PMIC_ISINK_CH1_BIAS_EN,
	PMIC_ISINK_RSV,
	PMIC_ISINK_CH1_MODE,
	PMIC_ISINK_CH0_MODE,
	PMIC_DA_QI_ISINKS_CH3_STEP,
	PMIC_DA_QI_ISINKS_CH2_STEP,
	PMIC_DA_QI_ISINKS_CH1_STEP,
	PMIC_DA_QI_ISINKS_CH0_STEP,
	PMIC_ISINK2_RSV1,
	PMIC_ISINK2_RSV0,
	PMIC_ISINK_CH2_STEP,
	PMIC_ISINK3_RSV1,
	PMIC_ISINK3_RSV0,
	PMIC_ISINK_CH3_STEP,
	PMIC_ISINK_CHOP3_SW,
	PMIC_ISINK_CHOP2_SW,
	PMIC_ISINK_CH3_EN,
	PMIC_ISINK_CH2_EN,
	PMIC_ISINK_CHOP3_EN,
	PMIC_ISINK_CHOP2_EN,
	PMIC_ISINK_CH3_BIAS_EN,
	PMIC_ISINK_CH2_BIAS_EN,
	PMIC_CHRIND_DIM_FSEL,
	PMIC_CHRIND_RSV1,
	PMIC_CHRIND_RSV0,
	PMIC_CHRIND_DIM_DUTY,
	PMIC_CHRIND_STEP,
	PMIC_CHRIND_BREATH_TF2_SEL,
	PMIC_CHRIND_BREATH_TF1_SEL,
	PMIC_CHRIND_BREATH_TR2_SEL,
	PMIC_CHRIND_BREATH_TR1_SEL,
	PMIC_CHRIND_BREATH_TOFF_SEL,
	PMIC_CHRIND_BREATH_TON_SEL,
	PMIC_CHRIND_SFSTR_EN,
	PMIC_CHRIND_SFSTR_TC,
	PMIC_CHRIND_EN_SEL,
	PMIC_CHRIND_EN,
	PMIC_CHRIND_CHOP_EN,
	PMIC_CHRIND_MODE,
	PMIC_CHRIND_CHOP_SW,
	PMIC_CHRIND_BIAS_EN,
	PMIC_RG_SLP_RW_EN,
	PMIC_RG_SPI_RSV,
	PMIC_DEW_DIO_EN,
	PMIC_DEW_READ_TEST,
	PMIC_DEW_WRITE_TEST,
	PMIC_DEW_CRC_SWRST,
	PMIC_DEW_CRC_EN,
	PMIC_DEW_CRC_VAL,
	PMIC_DEW_DBG_MON_SEL,
	PMIC_DEW_CIPHER_KEY_SEL,
	PMIC_DEW_CIPHER_IV_SEL,
	PMIC_DEW_CIPHER_EN,
	PMIC_DEW_CIPHER_RDY,
	PMIC_DEW_CIPHER_MODE,
	PMIC_DEW_CIPHER_SWRST,
	PMIC_DEW_RDDMY_NO,
	PMIC_INT_TYPE_CON0,
	PMIC_INT_TYPE_CON0_SET,
	PMIC_INT_TYPE_CON0_CLR,
	PMIC_INT_TYPE_CON1,
	PMIC_INT_TYPE_CON1_SET,
	PMIC_INT_TYPE_CON1_CLR,
	PMIC_INT_TYPE_CON2,
	PMIC_INT_TYPE_CON2_SET,
	PMIC_INT_TYPE_CON2_CLR,
	PMIC_INT_TYPE_CON3,
	PMIC_INT_TYPE_CON3_SET,
	PMIC_INT_TYPE_CON3_CLR,
	PMIC_CPU_INT_STA,
	PMIC_MD32_INT_STA,
	PMIC_BUCK_LDO_FT_TESTMODE_EN,
	PMIC_EFUSE_VOSEL_LIMIT,
	PMIC_EFUSE_OC_EN_SEL,
	PMIC_EFUSE_OC_SDN_EN,
	PMIC_BUCK_FOR_LDO_OSC_SEL,
	PMIC_BUCK_ALL_CON0_RSV0,
	PMIC_BUCK_BUCK_RSV,
	PMIC_BUCK_VCORE_VOSEL_CTRL_PROT,
	PMIC_BUCK_VCORE_VOSEL_CTRL_PROT_EN,
	PMIC_BUCK_ALL_CON2_RSV0,
	PMIC_BUCK_VSLEEP_SRC0,
	PMIC_BUCK_VSLEEP_SRC1,
	PMIC_BUCK_R2R_SRC0,
	PMIC_BUCK_R2R_SRC1,
	PMIC_BUCK_BUCK_OSC_SEL_SRC0,
	PMIC_BUCK_SRCLKEN_DLY_SRC1,
	PMIC_BUCK_VPA_VOSEL_DLC011,
	PMIC_BUCK_VPA_VOSEL_DLC111,
	PMIC_BUCK_VPA_DLC_MAP_EN,
	PMIC_BUCK_VPA_VOSEL_DLC001,
	PMIC_BUCK_VPA_DLC,
	PMIC_DA_NI_VPA_DLC,
	PMIC_BUCK_VSRAM_PROC_TRACK_SLEEP_CTRL,
	PMIC_BUCK_VSRAM_PROC_TRACK_ON_CTRL,
	PMIC_BUCK_VPROC_TRACK_ON_CTRL,
	PMIC_BUCK_VSRAM_PROC_VOSEL_DELTA,
	PMIC_BUCK_VSRAM_PROC_VOSEL_OFFSET,
	PMIC_BUCK_VSRAM_PROC_VOSEL_ON_LB,
	PMIC_BUCK_VSRAM_PROC_VOSEL_ON_HB,
	PMIC_BUCK_VSRAM_PROC_VOSEL_SLEEP_LB,
	PMIC_AD_QI_VCORE_OC_STATUS,
	PMIC_AD_QI_VPROC_OC_STATUS,
	PMIC_AD_QI_VS1_OC_STATUS,
	PMIC_AD_QI_VPA_OC_STATUS,
	PMIC_BUCK_ANA_STATUS,
	PMIC_AD_QI_VCORE_DIG_MON,
	PMIC_AD_QI_VPROC_DIG_MON,
	PMIC_AD_QI_VS1_DIG_MON,
	PMIC_BUCK_VCORE_OC_STATUS,
	PMIC_BUCK_VCORE2_OC_STATUS,
	PMIC_BUCK_VPROC_OC_STATUS,
	PMIC_BUCK_VS1_OC_STATUS,
	PMIC_BUCK_VPA_OC_STATUS,
	PMIC_BUCK_VCORE_OC_INT_EN,
	PMIC_BUCK_VCORE2_OC_INT_EN,
	PMIC_BUCK_VPROC_OC_INT_EN,
	PMIC_BUCK_VS1_OC_INT_EN,
	PMIC_BUCK_VPA_OC_INT_EN,
	PMIC_BUCK_VCORE_EN_OC_SDN_SEL,
	PMIC_BUCK_VCORE2_EN_OC_SDN_SEL,
	PMIC_BUCK_VPROC_EN_OC_SDN_SEL,
	PMIC_BUCK_VS1_EN_OC_SDN_SEL,
	PMIC_BUCK_VPA_EN_OC_SDN_SEL,
	PMIC_BUCK_VCORE_OC_FLAG_CLR,
	PMIC_BUCK_VCORE2_OC_FLAG_CLR,
	PMIC_BUCK_VPROC_OC_FLAG_CLR,
	PMIC_BUCK_VS1_OC_FLAG_CLR,
	PMIC_BUCK_VPA_OC_FLAG_CLR,
	PMIC_BUCK_VCORE_OC_FLAG_CLR_SEL,
	PMIC_BUCK_VCORE2_OC_FLAG_CLR_SEL,
	PMIC_BUCK_VPROC_OC_FLAG_CLR_SEL,
	PMIC_BUCK_VS1_OC_FLAG_CLR_SEL,
	PMIC_BUCK_VPA_OC_FLAG_CLR_SEL,
	PMIC_BUCK_VCORE_OC_DEG_EN,
	PMIC_BUCK_VCORE_OC_WND,
	PMIC_BUCK_VCORE_OC_THD,
	PMIC_BUCK_VCORE2_OC_DEG_EN,
	PMIC_BUCK_VCORE2_OC_WND,
	PMIC_BUCK_VCORE2_OC_THD,
	PMIC_BUCK_VPROC_OC_DEG_EN,
	PMIC_BUCK_VPROC_OC_WND,
	PMIC_BUCK_VPROC_OC_THD,
	PMIC_BUCK_VS1_OC_DEG_EN,
	PMIC_BUCK_VS1_OC_WND,
	PMIC_BUCK_VS1_OC_THD,
	PMIC_BUCK_VPA_OC_DEG_EN,
	PMIC_BUCK_VPA_OC_WND,
	PMIC_BUCK_VPA_OC_THD,
	PMIC_RG_SMPS_TESTMODE_B,
	PMIC_RG_VSRAM_PROC_TRIMH,
	PMIC_RG_VSRAM_PROC_TRIML,
	PMIC_RG_VPROC_TRIMH,
	PMIC_RG_VPROC_TRIML,
	PMIC_RG_VCORE_TRIMH,
	PMIC_RG_VCORE_TRIML,
	PMIC_RG_VCORE2_TRIMH,
	PMIC_RG_VCORE2_TRIML,
	PMIC_RG_VS1_TRIMH,
	PMIC_RG_VS1_TRIML,
	PMIC_RG_VPA_TRIMH,
	PMIC_RG_VPA_TRIML,
	PMIC_RG_VPA_TRIM_REF,
	PMIC_RG_VSRAM_PROC_VSLEEP,
	PMIC_RG_VPROC_VSLEEP,
	PMIC_RG_VCORE_VSLEEP,
	PMIC_RG_VCORE2_VSLEEP,
	PMIC_RG_VPA_BURSTH,
	PMIC_RG_VPA_BURSTL,
	PMIC_RG_DMY100MA_EN,
	PMIC_RG_DMY100MA_SEL,
	PMIC_RG_VSRAM_PROC_VSLEEP_VOLTAGE,
	PMIC_RG_VPROC_VSLEEP_VOLTAGE,
	PMIC_RG_VCORE_VSLEEP_VOLTAGE,
	PMIC_RG_VCORE2_VSLEEP_VOLTAGE,
	PMIC_RG_VPA_RZSEL,
	PMIC_RG_VPA_CC,
	PMIC_RG_VPA_CSR,
	PMIC_RG_VPA_CSMIR,
	PMIC_RG_VPA_CSL,
	PMIC_RG_VPA_SLP,
	PMIC_RG_VPA_ZX_OS_TRIM,
	PMIC_RG_VPA_ZX_OS,
	PMIC_RG_VPA_HZP,
	PMIC_RG_VPA_BWEX_GAT,
	PMIC_RG_VPA_MODESET,
	PMIC_RG_VPA_SLEW,
	PMIC_RG_VPA_SLEW_NMOS,
	PMIC_RG_VPA_NDIS_EN,
	PMIC_RG_VPA_MIN_ON,
	PMIC_RG_VPA_VBAT_DEL,
	PMIC_RG_VPA_RSV1,
	PMIC_RG_VPA_RSV2,
	PMIC_RGS_VPA_ZX_STATUS,
	PMIC_RG_VCORE_MIN_OFF,
	PMIC_RG_VCORE_VRF18_SSTART_EN,
	PMIC_RG_VCORE_1P35UP_SEL_EN,
	PMIC_RG_VCORE_RZSEL,
	PMIC_RG_VCORE_CSR,
	PMIC_RG_VCORE_CSL,
	PMIC_RG_VCORE_SLP,
	PMIC_RG_VCORE_ZX_OS,
	PMIC_RG_VCORE_ZXOS_TRIM,
	PMIC_RG_VCORE_MODESET,
	PMIC_RG_VCORE_NDIS_EN,
	PMIC_RG_VCORE_CSM_N,
	PMIC_RG_VCORE_CSM_P,
	PMIC_RG_VCORE_RSV,
	PMIC_RG_VCORE_PFM_RIP,
	PMIC_RG_VCORE_DTS_ENB,
	PMIC_RG_VCORE_AUTO_MODE,
	PMIC_RG_VCORE_PWM_TRIG,
	PMIC_RG_VCORE_TRAN_BST,
	PMIC_RGS_VCORE_ENPWM_STATUS,
	PMIC_RG_VPROC_MIN_OFF,
	PMIC_RG_VPROC_VRF18_SSTART_EN,
	PMIC_RG_VPROC_1P35UP_SEL_EN,
	PMIC_RG_VPROC_RZSEL,
	PMIC_RG_VPROC_CSR,
	PMIC_RG_VPROC_CSL,
	PMIC_RG_VPROC_SLP,
	PMIC_RG_VPROC_ZX_OS,
	PMIC_RG_VPROC_ZXOS_TRIM,
	PMIC_RG_VPROC_MODESET,
	PMIC_RG_VPROC_NDIS_EN,
	PMIC_RG_VPROC_CSM_N,
	PMIC_RG_VPROC_CSM_P,
	PMIC_RG_VPROC_RSV,
	PMIC_RG_VPROC_PFM_RIP,
	PMIC_RG_VPROC_DTS_ENB,
	PMIC_RG_VPROC_BW_EXTEND,
	PMIC_RG_VPROC_PWM_TRIG,
	PMIC_RG_VPROC_TRAN_BST,
	PMIC_RGS_VPROC_ENPWM_STATUS,
	PMIC_RG_VS1_MIN_OFF,
	PMIC_RG_VS1_NVT_BUFF_OFF_EN,
	PMIC_RG_VS1_VRF18_SSTART_EN,
	PMIC_RG_VS1_1P35UP_SEL_EN,
	PMIC_RG_VS1_RZSEL,
	PMIC_RG_VS1_CSR,
	PMIC_RG_VS1_CSL,
	PMIC_RG_VS1_SLP,
	PMIC_RG_VS1_ZX_OS,
	PMIC_RG_VS1_NDIS_EN,
	PMIC_RG_VS1_CSM_N,
	PMIC_RG_VS1_CSM_P,
	PMIC_RG_VS1_RSV,
	PMIC_RG_VS1_ZXOS_TRIM,
	PMIC_RG_VS1_MODESET,
	PMIC_RG_VS1_PFM_RIP,
	PMIC_RG_VS1_TRAN_BST,
	PMIC_RG_VS1_DTS_ENB,
	PMIC_RG_VS1_AUTO_MODE,
	PMIC_RG_VS1_PWM_TRIG,
	PMIC_RGS_VS1_ENPWM_STATUS,
	PMIC_DA_QI_VCORE_BURST,
	PMIC_DA_QI_VCORE2_BURST,
	PMIC_DA_QI_VS1_BURST,
	PMIC_DA_QI_VPROC_BURST,
	PMIC_DA_QI_VCORE_DLC,
	PMIC_DA_QI_VCORE_DLC_N,
	PMIC_DA_QI_VCORE2_DLC,
	PMIC_DA_QI_VCORE2_DLC_N,
	PMIC_DA_QI_VS1_DLC,
	PMIC_DA_QI_VS1_DLC_N,
	PMIC_DA_QI_VPROC_DLC,
	PMIC_DA_QI_VPROC_DLC_N,
	PMIC_RG_VCORE2_MIN_OFF,
	PMIC_RG_VCORE2_VRF18_SSTART_EN,
	PMIC_RG_VCORE2_1P35UP_SEL_EN,
	PMIC_RG_VCORE2_RZSEL,
	PMIC_RG_VCORE2_CSR,
	PMIC_RG_VCORE2_CSL,
	PMIC_RG_VCORE2_SLP,
	PMIC_RG_VCORE2_ZX_OS,
	PMIC_RG_VCORE2_ZXOS_TRIM,
	PMIC_RG_VCORE2_MODESET,
	PMIC_RG_VCORE2_NDIS_EN,
	PMIC_RG_VCORE2_CSM_N,
	PMIC_RG_VCORE2_CSM_P,
	PMIC_RG_VCORE2_RSV,
	PMIC_RG_VCORE2_PFM_RIP,
	PMIC_RG_VCORE2_DTS_ENB,
	PMIC_RG_VCORE2_AUTO_MODE,
	PMIC_RG_VCORE2_PWM_TRIG,
	PMIC_RG_VCORE2_TRAN_BST,
	PMIC_RGS_VCORE2_ENPWM_STATUS,
	PMIC_BUCK_VPROC_EN_CTRL,
	PMIC_BUCK_VPROC_VOSEL_CTRL,
	PMIC_BUCK_VPROC_EN_SEL,
	PMIC_BUCK_VPROC_VOSEL_SEL,
	PMIC_BUCK_VPROC_EN,
	PMIC_BUCK_VPROC_STBTD,
	PMIC_DA_VPROC_STB,
	PMIC_DA_QI_VPROC_EN,
	PMIC_BUCK_VPROC_SFCHG_FRATE,
	PMIC_BUCK_VPROC_SFCHG_FEN,
	PMIC_BUCK_VPROC_SFCHG_RRATE,
	PMIC_BUCK_VPROC_SFCHG_REN,
	PMIC_DA_NI_VPROC_VOSEL,
	PMIC_BUCK_VPROC_VOSEL,
	PMIC_BUCK_VPROC_VOSEL_ON,
	PMIC_BUCK_VPROC_VOSEL_SLEEP,
	PMIC_BUCK_VPROC_TRANS_TD,
	PMIC_BUCK_VPROC_TRANS_CTRL,
	PMIC_BUCK_VPROC_TRANS_ONCE,
	PMIC_DA_QI_VPROC_DVS_EN,
	PMIC_BUCK_VPROC_VSLEEP_EN,
	PMIC_BUCK_VPROC_R2R_PDN,
	PMIC_BUCK_VPROC_VSLEEP_SEL,
	PMIC_DA_NI_VPROC_R2R_PDN,
	PMIC_DA_NI_VPROC_VSLEEP_SEL,
	PMIC_BUCK_VS1_EN_CTRL,
	PMIC_BUCK_VS1_VOSEL_CTRL,
	PMIC_BUCK_VS1_VOSEL_CTRL_0,
	PMIC_BUCK_VS1_VOSEL_CTRL_1,
	PMIC_BUCK_VS1_VOSEL_CTRL_2,
	PMIC_BUCK_VS1_MODESET_0,
	PMIC_BUCK_VS1_MODESET_1,
	PMIC_BUCK_VS1_MODESET_2,
	PMIC_DA_VS1_MODESET,
	PMIC_BUCK_VS1_VOSEL_CTRL_INV,
	PMIC_BUCK_VS1_HWM_CON0_SET,
	PMIC_BUCK_VS1_HWM_CON0_CLR,
	PMIC_BUCK_VS1_EN_SEL,
	PMIC_BUCK_VS1_VOSEL_SEL,
	PMIC_BUCK_VS1_EN,
	PMIC_BUCK_VS1_STBTD,
	PMIC_DA_VS1_STB,
	PMIC_DA_QI_VS1_EN,
	PMIC_BUCK_VS1_SFCHG_FRATE,
	PMIC_BUCK_VS1_SFCHG_FEN,
	PMIC_BUCK_VS1_SFCHG_RRATE,
	PMIC_BUCK_VS1_SFCHG_REN,
	PMIC_DA_NI_VS1_VOSEL,
	PMIC_BUCK_VS1_VOSEL,
	PMIC_BUCK_VS1_VOSEL_ON,
	PMIC_BUCK_VS1_VOSEL_SLEEP,
	PMIC_BUCK_VS1_TRANS_TD,
	PMIC_BUCK_VS1_TRANS_CTRL,
	PMIC_BUCK_VS1_TRANS_ONCE,
	PMIC_DA_QI_VS1_DVS_EN,
	PMIC_BUCK_VS1_VSLEEP_EN,
	PMIC_BUCK_VS1_R2R_PDN,
	PMIC_BUCK_VS1_VSLEEP_SEL,
	PMIC_DA_NI_VS1_R2R_PDN,
	PMIC_DA_NI_VS1_VSLEEP_SEL,
	PMIC_BUCK_VCORE_EN_CTRL,
	PMIC_BUCK_VCORE_VOSEL_CTRL,
	PMIC_BUCK_VCORE_EN_SEL,
	PMIC_BUCK_VCORE_VOSEL_SEL,
	PMIC_BUCK_VCORE_EN,
	PMIC_BUCK_VCORE_STBTD,
	PMIC_DA_VCORE_STB,
	PMIC_DA_QI_VCORE_EN,
	PMIC_BUCK_VCORE_SFCHG_FRATE,
	PMIC_BUCK_VCORE_SFCHG_FEN,
	PMIC_BUCK_VCORE_SFCHG_RRATE,
	PMIC_BUCK_VCORE_SFCHG_REN,
	PMIC_DA_NI_VCORE_VOSEL,
	PMIC_BUCK_VCORE_VOSEL,
	PMIC_BUCK_VCORE_VOSEL_ON,
	PMIC_BUCK_VCORE_VOSEL_SLEEP,
	PMIC_BUCK_VCORE_TRANS_TD,
	PMIC_BUCK_VCORE_TRANS_CTRL,
	PMIC_BUCK_VCORE_TRANS_ONCE,
	PMIC_DA_QI_VCORE_DVS_EN,
	PMIC_BUCK_VCORE_VSLEEP_EN,
	PMIC_BUCK_VCORE_R2R_PDN,
	PMIC_BUCK_VCORE_VSLEEP_SEL,
	PMIC_DA_NI_VCORE_R2R_PDN,
	PMIC_DA_NI_VCORE_VSLEEP_SEL,
	PMIC_BUCK_VCORE2_EN_CTRL,
	PMIC_BUCK_VCORE2_VOSEL_CTRL,
	PMIC_BUCK_VCORE2_EN_SEL,
	PMIC_BUCK_VCORE2_VOSEL_SEL,
	PMIC_BUCK_VCORE2_EN,
	PMIC_BUCK_VCORE2_STBTD,
	PMIC_DA_VCORE2_STB,
	PMIC_DA_QI_VCORE2_EN,
	PMIC_BUCK_VCORE2_SFCHG_FRATE,
	PMIC_BUCK_VCORE2_SFCHG_FEN,
	PMIC_BUCK_VCORE2_SFCHG_RRATE,
	PMIC_BUCK_VCORE2_SFCHG_REN,
	PMIC_DA_NI_VCORE2_VOSEL,
	PMIC_BUCK_VCORE2_VOSEL,
	PMIC_BUCK_VCORE2_VOSEL_ON,
	PMIC_BUCK_VCORE2_VOSEL_SLEEP,
	PMIC_BUCK_VCORE2_TRANS_TD,
	PMIC_BUCK_VCORE2_TRANS_CTRL,
	PMIC_BUCK_VCORE2_TRANS_ONCE,
	PMIC_DA_QI_VCORE2_DVS_EN,
	PMIC_BUCK_VCORE2_VSLEEP_EN,
	PMIC_BUCK_VCORE2_R2R_PDN,
	PMIC_BUCK_VCORE2_VSLEEP_SEL,
	PMIC_DA_NI_VCORE2_R2R_PDN,
	PMIC_DA_NI_VCORE2_VSLEEP_SEL,
	PMIC_BUCK_VPA_EN,
	PMIC_BUCK_VPA_STBTD,
	PMIC_DA_VPA_STB,
	PMIC_DA_QI_VPA_EN,
	PMIC_BUCK_VPA_SFCHG_FRATE,
	PMIC_BUCK_VPA_SFCHG_FEN,
	PMIC_BUCK_VPA_SFCHG_RRATE,
	PMIC_BUCK_VPA_SFCHG_REN,
	PMIC_DA_NI_VPA_VOSEL,
	PMIC_BUCK_VPA_VOSEL,
	PMIC_BUCK_VPA_TRANS_TD,
	PMIC_BUCK_VPA_TRANS_CTRL,
	PMIC_BUCK_VPA_TRANS_ONCE,
	PMIC_DA_NI_VPA_DVS_TRANST,
	PMIC_BUCK_VPA_DVS_BW_TD,
	PMIC_BUCK_VPA_DVS_BW_CTRL,
	PMIC_BUCK_VPA_DVS_BW_ONCE,
	PMIC_DA_NI_VPA_DVS_BW,
	PMIC_BUCK_K_RST_DONE,
	PMIC_BUCK_K_MAP_SEL,
	PMIC_BUCK_K_ONCE_EN,
	PMIC_BUCK_K_ONCE,
	PMIC_BUCK_K_START_MANUAL,
	PMIC_BUCK_K_SRC_SEL,
	PMIC_BUCK_K_AUTO_EN,
	PMIC_BUCK_K_INV,
	PMIC_BUCK_K_CONTROL_SMPS,
	PMIC_K_RESULT,
	PMIC_K_DONE,
	PMIC_K_CONTROL,
	PMIC_DA_QI_SMPS_OSC_CAL,
	PMIC_BUCK_K_BUCK_CK_CNT,
	PMIC_WDTDBG_CLR,
	PMIC_WDTDBG_CON0_RSV0,
	PMIC_VPROC_VOSEL_WDTDBG,
	PMIC_VCORE_VOSEL_WDTDBG,
	PMIC_VPA_VOSEL_WDTDBG,
	PMIC_VS1_VOSEL_WDTDBG,
	PMIC_VSRAM_PROC_VOSEL_WDTDBG,
	PMIC_RG_AUDZCDENABLE,
	PMIC_RG_AUDZCDGAINSTEPTIME,
	PMIC_RG_AUDZCDGAINSTEPSIZE,
	PMIC_RG_AUDZCDTIMEOUTMODESEL,
	PMIC_RG_AUDZCDCLKSEL_VAUDP15,
	PMIC_RG_AUDZCDMUXSEL_VAUDP15,
	PMIC_RG_AUDLOLGAIN,
	PMIC_RG_AUDLORGAIN,
	PMIC_RG_AUDHPLGAIN,
	PMIC_RG_AUDHPRGAIN,
	PMIC_RG_AUDHSGAIN,
	PMIC_RG_AUDIVLGAIN,
	PMIC_RG_AUDIVRGAIN,
	PMIC_RG_AUDINTGAIN1,
	PMIC_RG_AUDINTGAIN2,
	PMIC_LDO_LDO_RSV1,
	PMIC_LDO_LDO_RSV0,
	PMIC_LDO_LDO_RSV1_RO,
	PMIC_LDO_LDO_RSV2,
	PMIC_LDO_VAUX18_AUXADC_PWDB_EN,
	PMIC_LDO_VIBR_THER_SDN_EN,
	PMIC_LDO_TOP_CON0_RSV,
	PMIC_LDO_VTCXO24_SWITCH,
	PMIC_LDO_VXO22_SWITCH,
	PMIC_LDO_DEGTD_SEL,
	PMIC_LDO_VRTC_EN,
	PMIC_DA_QI_VRTC_EN,
	PMIC_LDO_VCN33_EN_BT,
	PMIC_LDO_VCN33_EN_CTRL_BT,
	PMIC_LDO_VCN33_EN_SEL_BT,
	PMIC_LDO_VCN33_EN_WIFI,
	PMIC_LDO_VCN33_EN_CTRL_WIFI,
	PMIC_LDO_VCN33_EN_SEL_WIFI,
	PMIC_LDO_VCN33_LP_MODE,
	PMIC_LDO_VCN33_LP_CTRL,
	PMIC_LDO_VCN33_LP_SEL,
	PMIC_DA_QI_VCN33_MODE,
	PMIC_LDO_VCN33_STBTD,
	PMIC_DA_QI_VCN33_STB,
	PMIC_DA_QI_VCN33_EN,
	PMIC_LDO_VCN33_OCFB_EN,
	PMIC_DA_QI_VCN33_OCFB_EN,
	PMIC_LDO_VLDO28_EN_0,
	PMIC_LDO_VLDO28_EN_1,
	PMIC_LDO_VLDO28_LP_MODE,
	PMIC_LDO_VLDO28_LP_CTRL,
	PMIC_LDO_VLDO28_LP_SEL,
	PMIC_DA_QI_VLDO28_MODE,
	PMIC_LDO_VLDO28_STBTD,
	PMIC_DA_QI_VLDO28_STB,
	PMIC_DA_QI_VLDO28_EN,
	PMIC_LDO_VLDO28_OCFB_EN,
	PMIC_DA_QI_VLDO28_OCFB_EN,
	PMIC_LDO_VLDO28_FAST_TRAN_DL_EN,
	PMIC_LDO_VLDO28_FAST_TRAN_DL_CTRL,
	PMIC_LDO_VLDO28_FAST_TRAN_DL_SRCLKEN_SEL,
	PMIC_DA_QI_VLDO28_FAST_TRAN_DL_EN,
	PMIC_LDO_VLDO28_FAST_TRAN_CL_EN,
	PMIC_LDO_VLDO28_FAST_TRAN_CL_CTRL,
	PMIC_LDO_VLDO28_FAST_TRAN_CL_SRCLKEN_SEL,
	PMIC_DA_QI_VLDO28_FAST_TRAN_CL_EN,
	PMIC_LDO_VSRAM_PROC_VOSEL_CTRL,
	PMIC_LDO_VSRAM_PROC_VOSEL_SEL,
	PMIC_LDO_VSRAM_PROC_SFCHG_FRATE,
	PMIC_LDO_VSRAM_PROC_SFCHG_FEN,
	PMIC_LDO_VSRAM_PROC_SFCHG_RRATE,
	PMIC_LDO_VSRAM_PROC_SFCHG_REN,
	PMIC_DA_NI_VSRAM_PROC_VOSEL,
	PMIC_LDO_VSRAM_PROC_VOSEL,
	PMIC_LDO_VSRAM_PROC_VOSEL_ON,
	PMIC_LDO_VSRAM_PROC_VOSEL_SLEEP,
	PMIC_LDO_VSRAM_PROC_TRANS_TD,
	PMIC_LDO_VSRAM_PROC_TRANS_CTRL,
	PMIC_LDO_VSRAM_PROC_TRANS_ONCE,
	PMIC_DA_QI_VSRAM_PROC_DVS_EN,
	PMIC_LDO_VSRAM_PROC_VSLEEP_EN,
	PMIC_LDO_VSRAM_PROC_R2R_PDN,
	PMIC_LDO_VSRAM_PROC_VSLEEP_SEL,
	PMIC_DA_NI_VSRAM_PROC_R2R_PDN,
	PMIC_DA_NI_VSRAM_PROC_VSLEEP_SEL,
	PMIC_LDO_BATON_HT_EN,
	PMIC_LDO_BATON_HT_EN_DLY_TIME,
	PMIC_DA_QI_BATON_HT_EN,
	PMIC_LDO_TREF_EN,
	PMIC_LDO_TREF_EN_CTRL,
	PMIC_LDO_TREF_EN_SEL,
	PMIC_DA_QI_TREF_EN,
	PMIC_LDO_VTCXO28_LP_MODE,
	PMIC_LDO_VTCXO28_EN,
	PMIC_LDO_VTCXO28_LP_CTRL,
	PMIC_LDO_VTCXO28_EN_CTRL,
	PMIC_LDO_VTCXO28_LP_SEL,
	PMIC_DA_QI_VTCXO28_MODE,
	PMIC_LDO_VTCXO28_STBTD,
	PMIC_LDO_VTCXO28_EN_SEL,
	PMIC_DA_QI_VTCXO28_STB,
	PMIC_DA_QI_VTCXO28_EN,
	PMIC_LDO_VTCXO28_OCFB_EN,
	PMIC_DA_QI_VTCXO28_OCFB_EN,
	PMIC_LDO_VTCXO24_LP_MODE,
	PMIC_LDO_VTCXO24_EN,
	PMIC_LDO_VTCXO24_LP_CTRL,
	PMIC_LDO_VTCXO24_EN_CTRL,
	PMIC_LDO_VTCXO24_LP_SEL,
	PMIC_DA_QI_VTCXO24_MODE,
	PMIC_LDO_VTCXO24_STBTD,
	PMIC_LDO_VTCXO24_EN_SEL,
	PMIC_DA_QI_VTCXO24_STB,
	PMIC_DA_QI_VTCXO24_EN,
	PMIC_LDO_VTCXO24_OCFB_EN,
	PMIC_DA_QI_VTCXO24_OCFB_EN,
	PMIC_LDO_VXO22_LP_MODE,
	PMIC_LDO_VXO22_EN,
	PMIC_LDO_VXO22_LP_CTRL,
	PMIC_LDO_VXO22_EN_CTRL,
	PMIC_LDO_VXO22_LP_SEL,
	PMIC_DA_QI_VXO22_MODE,
	PMIC_LDO_VXO22_STBTD,
	PMIC_LDO_VXO22_EN_SEL,
	PMIC_DA_QI_VXO22_STB,
	PMIC_DA_QI_VXO22_EN,
	PMIC_LDO_VXO22_OCFB_EN,
	PMIC_DA_QI_VXO22_OCFB_EN,
	PMIC_LDO_VRF18_LP_MODE,
	PMIC_LDO_VRF18_EN,
	PMIC_LDO_VRF18_LP_CTRL,
	PMIC_LDO_VRF18_EN_CTRL,
	PMIC_LDO_VRF18_LP_SEL,
	PMIC_DA_QI_VRF18_MODE,
	PMIC_LDO_VRF18_STBTD,
	PMIC_LDO_VRF18_EN_SEL,
	PMIC_DA_QI_VRF18_STB,
	PMIC_DA_QI_VRF18_EN,
	PMIC_LDO_VRF18_OCFB_EN,
	PMIC_DA_QI_VRF18_OCFB_EN,
	PMIC_LDO_VRF12_LP_MODE,
	PMIC_LDO_VRF12_EN,
	PMIC_LDO_VRF12_LP_CTRL,
	PMIC_LDO_VRF12_EN_CTRL,
	PMIC_LDO_VRF12_LP_SEL,
	PMIC_DA_QI_VRF12_MODE,
	PMIC_LDO_VRF12_STBTD,
	PMIC_LDO_VRF12_EN_SEL,
	PMIC_DA_QI_VRF12_STB,
	PMIC_DA_QI_VRF12_EN,
	PMIC_LDO_VRF12_OCFB_EN,
	PMIC_DA_QI_VRF12_OCFB_EN,
	PMIC_LDO_VRF12_FAST_TRAN_DL_EN,
	PMIC_LDO_VRF12_FAST_TRAN_DL_CTRL,
	PMIC_LDO_VRF12_FAST_TRAN_DL_SRCLKEN_SEL,
	PMIC_DA_QI_VRF12_FAST_TRAN_DL_EN,
	PMIC_LDO_VRF12_FAST_TRAN_CL_EN,
	PMIC_LDO_VRF12_FAST_TRAN_CL_CTRL,
	PMIC_LDO_VRF12_FAST_TRAN_CL_SRCLKEN_SEL,
	PMIC_DA_QI_VRF12_FAST_TRAN_CL_EN,
	PMIC_LDO_VCN28_LP_MODE,
	PMIC_LDO_VCN28_EN,
	PMIC_LDO_VCN28_LP_CTRL,
	PMIC_LDO_VCN28_EN_CTRL,
	PMIC_LDO_VCN28_LP_SEL,
	PMIC_DA_QI_VCN28_MODE,
	PMIC_LDO_VCN28_STBTD,
	PMIC_LDO_VCN28_EN_SEL,
	PMIC_DA_QI_VCN28_STB,
	PMIC_DA_QI_VCN28_EN,
	PMIC_LDO_VCN28_OCFB_EN,
	PMIC_DA_QI_VCN28_OCFB_EN,
	PMIC_LDO_VCN18_LP_MODE,
	PMIC_LDO_VCN18_EN,
	PMIC_LDO_VCN18_LP_CTRL,
	PMIC_LDO_VCN18_EN_CTRL,
	PMIC_LDO_VCN18_LP_SEL,
	PMIC_DA_QI_VCN18_MODE,
	PMIC_LDO_VCN18_STBTD,
	PMIC_LDO_VCN18_EN_SEL,
	PMIC_DA_QI_VCN18_STB,
	PMIC_DA_QI_VCN18_EN,
	PMIC_LDO_VCN18_OCFB_EN,
	PMIC_DA_QI_VCN18_OCFB_EN,
	PMIC_LDO_VCAMA_LP_MODE,
	PMIC_LDO_VCAMA_EN,
	PMIC_DA_QI_VCAMA_MODE,
	PMIC_LDO_VCAMA_STBTD,
	PMIC_DA_QI_VCAMA_STB,
	PMIC_DA_QI_VCAMA_EN,
	PMIC_LDO_VCAMA_OCFB_EN,
	PMIC_DA_QI_VCAMA_OCFB_EN,
	PMIC_LDO_VCAMIO_LP_MODE,
	PMIC_LDO_VCAMIO_EN,
	PMIC_DA_QI_VCAMIO_MODE,
	PMIC_LDO_VCAMIO_STBTD,
	PMIC_DA_QI_VCAMIO_STB,
	PMIC_DA_QI_VCAMIO_EN,
	PMIC_LDO_VCAMIO_OCFB_EN,
	PMIC_DA_QI_VCAMIO_OCFB_EN,
	PMIC_LDO_VCAMD_LP_MODE,
	PMIC_LDO_VCAMD_EN,
	PMIC_DA_QI_VCAMD_MODE,
	PMIC_LDO_VCAMD_STBTD,
	PMIC_DA_QI_VCAMD_STB,
	PMIC_DA_QI_VCAMD_EN,
	PMIC_LDO_VCAMD_OCFB_EN,
	PMIC_DA_QI_VCAMD_OCFB_EN,
	PMIC_LDO_VAUX18_LP_MODE,
	PMIC_LDO_VAUX18_EN,
	PMIC_LDO_VAUX18_LP_CTRL,
	PMIC_LDO_VAUX18_EN_CTRL,
	PMIC_LDO_VAUX18_LP_SEL,
	PMIC_DA_QI_VAUX18_MODE,
	PMIC_LDO_VAUX18_STBTD,
	PMIC_LDO_VAUX18_EN_SEL,
	PMIC_DA_QI_VAUX18_STB,
	PMIC_DA_QI_VAUX18_EN,
	PMIC_LDO_VAUX18_OCFB_EN,
	PMIC_DA_QI_VAUX18_OCFB_EN,
	PMIC_LDO_VAUD28_LP_MODE,
	PMIC_LDO_VAUD28_EN,
	PMIC_LDO_VAUD28_LP_CTRL,
	PMIC_LDO_VAUD28_EN_CTRL,
	PMIC_LDO_VAUD28_LP_SEL,
	PMIC_DA_QI_VAUD28_MODE,
	PMIC_LDO_VAUD28_STBTD,
	PMIC_LDO_VAUD28_EN_SEL,
	PMIC_DA_QI_VAUD28_STB,
	PMIC_DA_QI_VAUD28_EN,
	PMIC_LDO_VAUD28_OCFB_EN,
	PMIC_DA_QI_VAUD28_OCFB_EN,
	PMIC_LDO_VSRAM_PROC_LP_MODE,
	PMIC_LDO_VSRAM_PROC_EN,
	PMIC_LDO_VSRAM_PROC_LP_CTRL,
	PMIC_LDO_VSRAM_PROC_EN_CTRL,
	PMIC_LDO_VSRAM_PROC_LP_SEL,
	PMIC_DA_QI_VSRAM_PROC_MODE,
	PMIC_LDO_VSRAM_PROC_STBTD,
	PMIC_LDO_VSRAM_PROC_EN_SEL,
	PMIC_DA_QI_VSRAM_PROC_STB,
	PMIC_DA_QI_VSRAM_PROC_EN,
	PMIC_LDO_VSRAM_PROC_OCFB_EN,
	PMIC_DA_QI_VSRAM_PROC_OCFB_EN,
	PMIC_LDO_VDRAM_LP_MODE,
	PMIC_LDO_VDRAM_EN,
	PMIC_LDO_VDRAM_LP_CTRL,
	PMIC_LDO_VDRAM_LP_SEL,
	PMIC_DA_QI_VDRAM_MODE,
	PMIC_LDO_VDRAM_STBTD,
	PMIC_DA_QI_VDRAM_STB,
	PMIC_DA_QI_VDRAM_EN,
	PMIC_LDO_VDRAM_OCFB_EN,
	PMIC_DA_QI_VDRAM_OCFB_EN,
	PMIC_LDO_VDRAM_FAST_TRAN_DL_EN,
	PMIC_LDO_VDRAM_FAST_TRAN_DL_CTRL,
	PMIC_LDO_VDRAM_FAST_TRAN_DL_SRCLKEN_SEL,
	PMIC_DA_QI_VDRAM_FAST_TRAN_DL_EN,
	PMIC_LDO_VDRAM_FAST_TRAN_CL_EN,
	PMIC_LDO_VDRAM_FAST_TRAN_CL_CTRL,
	PMIC_LDO_VDRAM_FAST_TRAN_CL_SRCLKEN_SEL,
	PMIC_DA_QI_VDRAM_FAST_TRAN_CL_EN,
	PMIC_LDO_VSIM1_LP_MODE,
	PMIC_LDO_VSIM1_EN,
	PMIC_LDO_VSIM1_LP_CTRL,
	PMIC_LDO_VSIM1_LP_SEL,
	PMIC_DA_QI_VSIM1_MODE,
	PMIC_LDO_VSIM1_STBTD,
	PMIC_DA_QI_VSIM1_STB,
	PMIC_DA_QI_VSIM1_EN,
	PMIC_LDO_VSIM1_OCFB_EN,
	PMIC_DA_QI_VSIM1_OCFB_EN,
	PMIC_LDO_VSIM2_LP_MODE,
	PMIC_LDO_VSIM2_EN,
	PMIC_LDO_VSIM2_LP_CTRL,
	PMIC_LDO_VSIM2_LP_SEL,
	PMIC_DA_QI_VSIM2_MODE,
	PMIC_LDO_VSIM2_STBTD,
	PMIC_DA_QI_VSIM2_STB,
	PMIC_DA_QI_VSIM2_EN,
	PMIC_LDO_VSIM2_OCFB_EN,
	PMIC_DA_QI_VSIM2_OCFB_EN,
	PMIC_LDO_VIO28_LP_MODE,
	PMIC_LDO_VIO28_EN,
	PMIC_LDO_VIO28_LP_CTRL,
	PMIC_LDO_VIO28_LP_SEL,
	PMIC_DA_QI_VIO28_MODE,
	PMIC_LDO_VIO28_STBTD,
	PMIC_DA_QI_VIO28_STB,
	PMIC_DA_QI_VIO28_EN,
	PMIC_LDO_VIO28_OCFB_EN,
	PMIC_DA_QI_VIO28_OCFB_EN,
	PMIC_LDO_VMC_LP_MODE,
	PMIC_LDO_VMC_EN,
	PMIC_LDO_VMC_LP_CTRL,
	PMIC_LDO_VMC_LP_SEL,
	PMIC_DA_QI_VMC_MODE,
	PMIC_LDO_VMC_STBTD,
	PMIC_DA_QI_VMC_STB,
	PMIC_DA_QI_VMC_EN,
	PMIC_LDO_VMC_OCFB_EN,
	PMIC_DA_QI_VMC_OCFB_EN,
	PMIC_LDO_VMC_FAST_TRAN_DL_EN,
	PMIC_LDO_VMC_FAST_TRAN_DL_CTRL,
	PMIC_LDO_VMC_FAST_TRAN_DL_SRCLKEN_SEL,
	PMIC_DA_QI_VMC_FAST_TRAN_DL_EN,
	PMIC_LDO_VMC_FAST_TRAN_CL_EN,
	PMIC_LDO_VMC_FAST_TRAN_CL_CTRL,
	PMIC_LDO_VMC_FAST_TRAN_CL_SRCLKEN_SEL,
	PMIC_DA_QI_VMC_FAST_TRAN_CL_EN,
	PMIC_LDO_VMCH_LP_MODE,
	PMIC_LDO_VMCH_EN,
	PMIC_LDO_VMCH_LP_CTRL,
	PMIC_LDO_VMCH_LP_SEL,
	PMIC_DA_QI_VMCH_MODE,
	PMIC_LDO_VMCH_STBTD,
	PMIC_DA_QI_VMCH_STB,
	PMIC_DA_QI_VMCH_EN,
	PMIC_LDO_VMCH_OCFB_EN,
	PMIC_DA_QI_VMCH_OCFB_EN,
	PMIC_LDO_VMCH_FAST_TRAN_DL_EN,
	PMIC_LDO_VMCH_FAST_TRAN_DL_CTRL,
	PMIC_LDO_VMCH_FAST_TRAN_DL_SRCLKEN_SEL,
	PMIC_DA_QI_VMCH_FAST_TRAN_DL_EN,
	PMIC_LDO_VMCH_FAST_TRAN_CL_EN,
	PMIC_LDO_VMCH_FAST_TRAN_CL_CTRL,
	PMIC_LDO_VMCH_FAST_TRAN_CL_SRCLKEN_SEL,
	PMIC_DA_QI_VMCH_FAST_TRAN_CL_EN,
	PMIC_LDO_VUSB33_LP_MODE,
	PMIC_LDO_VUSB33_EN,
	PMIC_LDO_VUSB33_LP_CTRL,
	PMIC_LDO_VUSB33_LP_SEL,
	PMIC_DA_QI_VUSB33_MODE,
	PMIC_LDO_VUSB33_STBTD,
	PMIC_DA_QI_VUSB33_STB,
	PMIC_DA_QI_VUSB33_EN,
	PMIC_LDO_VUSB33_OCFB_EN,
	PMIC_DA_QI_VUSB33_OCFB_EN,
	PMIC_LDO_VEMC33_LP_MODE,
	PMIC_LDO_VEMC33_EN,
	PMIC_LDO_VEMC33_LP_CTRL,
	PMIC_LDO_VEMC33_LP_SEL,
	PMIC_DA_QI_VEMC33_MODE,
	PMIC_LDO_VEMC33_STBTD,
	PMIC_DA_QI_VEMC33_STB,
	PMIC_DA_QI_VEMC33_EN,
	PMIC_LDO_VEMC33_OCFB_EN,
	PMIC_DA_QI_VEMC33_OCFB_EN,
	PMIC_LDO_VEMC33_FAST_TRAN_DL_EN,
	PMIC_LDO_VEMC33_FAST_TRAN_DL_CTRL,
	PMIC_LDO_VEMC33_FAST_TRAN_DL_SRCLKEN_SEL,
	PMIC_DA_QI_VEMC33_FAST_TRAN_DL_EN,
	PMIC_LDO_VEMC33_FAST_TRAN_CL_EN,
	PMIC_LDO_VEMC33_FAST_TRAN_CL_CTRL,
	PMIC_LDO_VEMC33_FAST_TRAN_CL_SRCLKEN_SEL,
	PMIC_DA_QI_VEMC33_FAST_TRAN_CL_EN,
	PMIC_LDO_VIO18_LP_MODE,
	PMIC_LDO_VIO18_EN,
	PMIC_LDO_VIO18_LP_CTRL,
	PMIC_LDO_VIO18_LP_SEL,
	PMIC_DA_QI_VIO18_MODE,
	PMIC_LDO_VIO18_STBTD,
	PMIC_DA_QI_VIO18_STB,
	PMIC_DA_QI_VIO18_EN,
	PMIC_LDO_VIO18_OCFB_EN,
	PMIC_DA_QI_VIO18_OCFB_EN,
	PMIC_LDO_VIBR_LP_MODE,
	PMIC_LDO_VIBR_EN,
	PMIC_DA_QI_VIBR_MODE,
	PMIC_LDO_VIBR_STBTD,
	PMIC_DA_QI_VIBR_STB,
	PMIC_DA_QI_VIBR_EN,
	PMIC_LDO_VIBR_OCFB_EN,
	PMIC_DA_QI_VIBR_OCFB_EN,
	PMIC_RG_VTCXO28_CAL,
	PMIC_RG_VTCXO28_VOSEL,
	PMIC_RG_VTCXO28_NDIS_EN,
	PMIC_RG_VAUD28_CAL,
	PMIC_RG_VAUD28_VOSEL,
	PMIC_RG_VAUD28_NDIS_EN,
	PMIC_RG_VCN28_CAL,
	PMIC_RG_VCN28_VOSEL,
	PMIC_RG_VCN28_NDIS_EN,
	PMIC_RG_VAUX18_CAL,
	PMIC_RG_VAUX18_VOSEL,
	PMIC_RG_VAUX18_NDIS_EN,
	PMIC_RG_VCAMA_CAL,
	PMIC_RG_VCAMA_VOSEL,
	PMIC_RG_VCAMA_NDIS_EN,
	PMIC_RG_ALDO_RSV_H,
	PMIC_RG_ALDO_RSV_L,
	PMIC_RG_VXO22_CAL,
	PMIC_RG_VXO22_VOSEL,
	PMIC_RG_VXO22_NDIS_EN,
	PMIC_RG_VTCXO24_CAL,
	PMIC_RG_VTCXO24_VOSEL,
	PMIC_RG_VTCXO24_NDIS_EN,
	PMIC_RG_VSIM1_CAL,
	PMIC_RG_VSIM1_VOSEL,
	PMIC_RG_VSIM1_NDIS_EN,
	PMIC_RG_VSIM2_CAL,
	PMIC_RG_VSIM2_VOSEL,
	PMIC_RG_VSIM2_NDIS_EN,
	PMIC_RG_VCN33_CAL,
	PMIC_RG_VCN33_VOSEL,
	PMIC_RG_VCN33_NDIS_EN,
	PMIC_RG_VUSB33_CAL,
	PMIC_RG_VUSB33_NDIS_EN,
	PMIC_RG_VMCH_CAL,
	PMIC_RG_VMCH_VOSEL,
	PMIC_RG_VMCH_NDIS_EN,
	PMIC_RG_VMCH_OC_TRIM,
	PMIC_RG_VMCH_DUMMY_LOAD,
	PMIC_RG_VMCH_DL_EN,
	PMIC_RG_VMCH_CL_EN,
	PMIC_RG_VMCH_STB_SEL,
	PMIC_RG_VEMC33_CAL,
	PMIC_RG_VEMC33_VOSEL,
	PMIC_RG_VEMC33_NDIS_EN,
	PMIC_RG_VEMC33_OC_TRIM,
	PMIC_RG_VEMC33_DUMMY_LOAD,
	PMIC_RG_VEMC33_DL_EN,
	PMIC_RG_VEMC33_CL_EN,
	PMIC_RG_VEMC33_STB_SEL,
	PMIC_RG_VIO28_CAL,
	PMIC_RG_VIO28_NDIS_EN,
	PMIC_RG_VIBR_CAL,
	PMIC_RG_VIBR_VOSEL,
	PMIC_RG_VIBR_NDIS_EN,
	PMIC_RG_VLDO28_CAL,
	PMIC_RG_VLDO28_VOSEL,
	PMIC_RG_VLDO28_NDIS_EN,
	PMIC_RG_VLDO28_DUMMY_LOAD,
	PMIC_RG_VLDO28_DL_EN,
	PMIC_RG_VLDO28_CL_EN,
	PMIC_RG_VMC_CAL,
	PMIC_RG_VMC_VOSEL,
	PMIC_RG_VMC_NDIS_EN,
	PMIC_RG_VMC_DUMMY_LOAD,
	PMIC_RG_VMC_DL_EN,
	PMIC_RG_VMC_CL_EN,
	PMIC_RG_DLDO_RSV_H,
	PMIC_RG_DLDO_RSV_L,
	PMIC_RG_VCAMD_CAL,
	PMIC_RG_VCAMD_VOSEL,
	PMIC_RG_VCAMD_NDIS_EN,
	PMIC_RG_VRF18_CAL,
	PMIC_RG_VRF18_NDIS_EN,
	PMIC_RG_VRF12_CAL,
	PMIC_RG_VRF12_VOSEL,
	PMIC_RG_VRF12_NDIS_EN,
	PMIC_RG_VRF12_STB_SEL,
	PMIC_RG_VRF12_DUMMY_LOAD,
	PMIC_RG_VRF12_DL_EN,
	PMIC_RG_VRF12_CL_EN,
	PMIC_RG_VIO18_CAL,
	PMIC_RG_VIO18_NDIS_EN,
	PMIC_RG_VDRAM_CAL,
	PMIC_RG_VDRAM_VOSEL,
	PMIC_RG_VDRAM_NDIS_EN,
	PMIC_RG_VDRAM_PCUR_CAL,
	PMIC_RG_VDRAM_DL_EN,
	PMIC_RG_VDRAM_CL_EN,
	PMIC_RG_VCAMIO_CAL,
	PMIC_RG_VCAMIO_VOSEL,
	PMIC_RG_VCAMIO_NDIS_EN,
	PMIC_RG_VCN18_CAL,
	PMIC_RG_VCN18_NDIS_EN,
	PMIC_RG_VSRAM_PROC_VOSEL,
	PMIC_RG_VSRAM_PROC_NDIS_PLCUR,
	PMIC_RG_VSRAM_PROC_NDIS_EN,
	PMIC_RG_VSRAM_PROC_PLCUR_EN,
	PMIC_RG_SLDO_RSV_H,
	PMIC_RG_SLDO_RSV_L,
	PMIC_RG_OTP_PA,
	PMIC_RG_OTP_PDIN,
	PMIC_RG_OTP_PTM,
	PMIC_RG_OTP_PWE,
	PMIC_RG_OTP_PPROG,
	PMIC_RG_OTP_PWE_SRC,
	PMIC_RG_OTP_PROG_PKEY,
	PMIC_RG_OTP_RD_PKEY,
	PMIC_RG_OTP_RD_TRIG,
	PMIC_RG_RD_RDY_BYPASS,
	PMIC_RG_SKIP_OTP_OUT,
	PMIC_RG_OTP_RD_SW,
	PMIC_RG_OTP_DOUT_SW,
	PMIC_RG_OTP_RD_BUSY,
	PMIC_RG_OTP_RD_ACK,
	PMIC_RG_OTP_PA_SW,
	PMIC_RG_OTP_DOUT_0_15,
	PMIC_RG_OTP_DOUT_16_31,
	PMIC_RG_OTP_DOUT_32_47,
	PMIC_RG_OTP_DOUT_48_63,
	PMIC_RG_OTP_DOUT_64_79,
	PMIC_RG_OTP_DOUT_80_95,
	PMIC_RG_OTP_DOUT_96_111,
	PMIC_RG_OTP_DOUT_112_127,
	PMIC_RG_OTP_DOUT_128_143,
	PMIC_RG_OTP_DOUT_144_159,
	PMIC_RG_OTP_DOUT_160_175,
	PMIC_RG_OTP_DOUT_176_191,
	PMIC_RG_OTP_DOUT_192_207,
	PMIC_RG_OTP_DOUT_208_223,
	PMIC_RG_OTP_DOUT_224_239,
	PMIC_RG_OTP_DOUT_240_255,
	PMIC_RG_OTP_DOUT_256_271,
	PMIC_RG_OTP_DOUT_272_287,
	PMIC_RG_OTP_DOUT_288_303,
	PMIC_RG_OTP_DOUT_304_319,
	PMIC_RG_OTP_DOUT_320_335,
	PMIC_RG_OTP_DOUT_336_351,
	PMIC_RG_OTP_DOUT_352_367,
	PMIC_RG_OTP_DOUT_368_383,
	PMIC_RG_OTP_DOUT_384_399,
	PMIC_RG_OTP_DOUT_400_415,
	PMIC_RG_OTP_DOUT_416_431,
	PMIC_RG_OTP_DOUT_432_447,
	PMIC_RG_OTP_DOUT_448_463,
	PMIC_RG_OTP_DOUT_464_479,
	PMIC_RG_OTP_DOUT_480_495,
	PMIC_RG_OTP_DOUT_496_511,
	PMIC_RG_OTP_VAL_0_15,
	PMIC_RG_OTP_VAL_16_31,
	PMIC_RG_OTP_VAL_32_47,
	PMIC_RG_OTP_VAL_48_63,
	PMIC_RG_OTP_VAL_64_79,
	PMIC_RG_OTP_VAL_80_95,
	PMIC_RG_OTP_VAL_96_111,
	PMIC_RG_OTP_VAL_112_127,
	PMIC_RG_OTP_VAL_128_143,
	PMIC_RG_OTP_VAL_144_159,
	PMIC_RG_OTP_VAL_160_175,
	PMIC_RG_OTP_VAL_176_191,
	PMIC_RG_OTP_VAL_192_207,
	PMIC_RG_OTP_VAL_208_223,
	PMIC_RG_OTP_VAL_224_239,
	PMIC_RG_OTP_VAL_240_255,
	PMIC_RG_OTP_VAL_256_271,
	PMIC_RG_OTP_VAL_272_287,
	PMIC_RG_OTP_VAL_288_303,
	PMIC_RG_OTP_VAL_304_319,
	PMIC_RG_OTP_VAL_320_335,
	PMIC_RG_OTP_VAL_336_351,
	PMIC_RG_OTP_VAL_352_367,
	PMIC_RG_OTP_VAL_368_383,
	PMIC_RG_OTP_VAL_384_399,
	PMIC_RG_OTP_VAL_400_415,
	PMIC_RG_OTP_VAL_416_431,
	PMIC_RG_OTP_VAL_432_447,
	PMIC_RG_OTP_VAL_448_463,
	PMIC_RG_OTP_VAL_464_479,
	PMIC_RG_OTP_VAL_480_495,
	PMIC_RG_OTP_VAL_496_511,
	PMIC_MIX_EOSC32_STP_LPDTB,
	PMIC_MIX_EOSC32_STP_LPDEN,
	PMIC_MIX_XOSC32_STP_PWDB,
	PMIC_MIX_XOSC32_STP_LPDTB,
	PMIC_MIX_XOSC32_STP_LPDEN,
	PMIC_MIX_XOSC32_STP_LPDRST,
	PMIC_MIX_XOSC32_STP_CALI,
	PMIC_STMP_MODE,
	PMIC_MIX_EOSC32_STP_CHOP_EN,
	PMIC_MIX_DCXO_STP_LVSH_EN,
	PMIC_MIX_PMU_STP_DDLO_VRTC,
	PMIC_MIX_PMU_STP_DDLO_VRTC_EN,
	PMIC_MIX_RTC_STP_XOSC32_ENB,
	PMIC_MIX_DCXO_STP_TEST_DEGLITCH_MODE,
	PMIC_MIX_EOSC32_STP_RSV,
	PMIC_MIX_EOSC32_VCT_EN,
	PMIC_MIX_EOSC32_OPT,
	PMIC_MIX_DCXO_STP_LVSH_EN_INT,
	PMIC_MIX_RTC_GPIO_COREDETB,
	PMIC_MIX_RTC_GPIO_F32KOB,
	PMIC_MIX_RTC_GPIO_GPO,
	PMIC_MIX_RTC_GPIO_OE,
	PMIC_MIX_RTC_STP_DEBUG_OUT,
	PMIC_MIX_RTC_STP_DEBUG_SEL,
	PMIC_MIX_RTC_STP_K_EOSC32_EN,
	PMIC_MIX_RTC_STP_EMBCK_SEL,
	PMIC_MIX_STP_BBWAKEUP,
	PMIC_MIX_STP_RTC_DDLO,
	PMIC_MIX_RTC_XOSC32_ENB,
	PMIC_MIX_EFUSE_XOSC32_ENB_OPT,
	PMIC_FG_ON,
	PMIC_FG_CAL,
	PMIC_FG_AUTOCALRATE,
	PMIC_FG_SW_CR,
	PMIC_FG_SW_READ_PRE,
	PMIC_FG_LATCHDATA_ST,
	PMIC_FG_SW_CLEAR,
	PMIC_FG_OFFSET_RST,
	PMIC_FG_TIME_RST,
	PMIC_FG_CHARGE_RST,
	PMIC_FG_SW_RSTCLR,
	PMIC_FG_CAR_34_19,
	PMIC_FG_CAR_18_03,
	PMIC_FG_CAR_02_00,
	PMIC_FG_NTER_32_17,
	PMIC_FG_NTER_16_01,
	PMIC_FG_NTER_00,
	PMIC_FG_BLTR_31_16,
	PMIC_FG_BLTR_15_00,
	PMIC_FG_BFTR_31_16,
	PMIC_FG_BFTR_15_00,
	PMIC_FG_CURRENT_OUT,
	PMIC_FG_ADJUST_OFFSET_VALUE,
	PMIC_FG_OFFSET,
	PMIC_RG_FGANALOGTEST,
	PMIC_RG_FGINTMODE,
	PMIC_RG_SPARE,
	PMIC_FG_OSR,
	PMIC_FG_ADJ_OFFSET_EN,
	PMIC_FG_ADC_AUTORST,
	PMIC_FG_FIR1BYPASS,
	PMIC_FG_FIR2BYPASS,
	PMIC_FG_L_CUR_INT_STS,
	PMIC_FG_H_CUR_INT_STS,
	PMIC_FG_L_INT_STS,
	PMIC_FG_H_INT_STS,
	PMIC_FG_ADC_RSTDETECT,
	PMIC_FG_SLP_EN,
	PMIC_FG_ZCV_DET_EN,
	PMIC_RG_FG_AUXADC_R,
	PMIC_DA_FGADC_EN,
	PMIC_DA_FGCAL_EN,
	PMIC_DA_FG_RST,
	PMIC_FG_CIC2,
	PMIC_FG_SLP_CUR_TH,
	PMIC_FG_SLP_TIME,
	PMIC_FG_SRCVOLTEN_FTIME,
	PMIC_FG_DET_TIME,
	PMIC_FG_ZCV_CAR_34_19,
	PMIC_FG_ZCV_CAR_18_03,
	PMIC_FG_ZCV_CAR_02_00,
	PMIC_FG_ZCV_CURR,
	PMIC_FG_R_CURR,
	PMIC_FG_MODE,
	PMIC_FG_RST_SW,
	PMIC_FG_FGCAL_EN_SW,
	PMIC_FG_FGADC_EN_SW,
	PMIC_FG_RSV1,
	PMIC_FG_TEST_MODE0,
	PMIC_FG_TEST_MODE1,
	PMIC_FG_GAIN,
	PMIC_FG_CUR_HTH,
	PMIC_FG_CUR_LTH,
	PMIC_FG_ZCV_DET_TIME,
	PMIC_FG_ZCV_CAR_TH_33_19,
	PMIC_FG_ZCV_CAR_TH_18_03,
	PMIC_FG_ZCV_CAR_TH_02_00,
	PMIC_SYSTEM_INFO_CON0,
	PMIC_SYSTEM_INFO_CON1,
	PMIC_SYSTEM_INFO_CON2,
	PMIC_SYSTEM_INFO_CON3,
	PMIC_SYSTEM_INFO_CON4,
	PMIC_RG_AUDDACLPWRUP_VAUDP15,
	PMIC_RG_AUDDACRPWRUP_VAUDP15,
	PMIC_RG_AUD_DAC_PWR_UP_VA28,
	PMIC_RG_AUD_DAC_PWL_UP_VA28,
	PMIC_RG_AUDHSPWRUP_VAUDP15,
	PMIC_RG_AUDHPLPWRUP_VAUDP15,
	PMIC_RG_AUDHPRPWRUP_VAUDP15,
	PMIC_RG_AUDHSMUXINPUTSEL_VAUDP15,
	PMIC_RG_AUDHPLMUXINPUTSEL_VAUDP15,
	PMIC_RG_AUDHPRMUXINPUTSEL_VAUDP15,
	PMIC_RG_AUDHSSCDISABLE_VAUDP15,
	PMIC_RG_AUDHPLSCDISABLE_VAUDP15,
	PMIC_RG_AUDHPRSCDISABLE_VAUDP15,
	PMIC_RG_AUDHPLBSCCURRENT_VAUDP15,
	PMIC_RG_AUDHPRBSCCURRENT_VAUDP15,
	PMIC_RG_AUDHSBSCCURRENT_VAUDP15,
	PMIC_RG_AUDHPSTARTUP_VAUDP15,
	PMIC_RG_AUDHSSTARTUP_VAUDP15,
	PMIC_RG_PRECHARGEBUF_EN_VAUDP15,
	PMIC_RG_HPINPUTSTBENH_VAUDP15,
	PMIC_RG_HPOUTPUTSTBENH_VAUDP15,
	PMIC_RG_HPINPUTRESET0_VAUDP15,
	PMIC_RG_HPOUTPUTRESET0_VAUDP15,
	PMIC_RG_HPOUT_SHORTVCM_VAUDP15,
	PMIC_RG_HSINPUTSTBENH_VAUDP15,
	PMIC_RG_HSOUTPUTSTBENH_VAUDP15,
	PMIC_RG_HSINPUTRESET0_VAUDP15,
	PMIC_RG_HSOUTPUTRESET0_VAUDP15,
	PMIC_RG_HPOUTSTB_RSEL_VAUDP15,
	PMIC_RG_HSOUT_SHORTVCM_VAUDP15,
	PMIC_RG_AUDHPLTRIM_VAUDP15,
	PMIC_RG_AUDHPRTRIM_VAUDP15,
	PMIC_RG_AUDHPTRIM_EN_VAUDP15,
	PMIC_RG_AUDHPLFINETRIM_VAUDP15,
	PMIC_RG_AUDHPRFINETRIM_VAUDP15,
	PMIC_RG_AUDTRIMBUF_EN_VAUDP15,
	PMIC_RG_AUDTRIMBUF_INPUTMUXSEL_VAUDP15,
	PMIC_RG_AUDTRIMBUF_GAINSEL_VAUDP15,
	PMIC_RG_AUDHPSPKDET_EN_VAUDP15,
	PMIC_RG_AUDHPSPKDET_INPUTMUXSEL_VAUDP15,
	PMIC_RG_AUDHPSPKDET_OUTPUTMUXSEL_VAUDP15,
	PMIC_RG_ABIDEC_RESERVED_VA28,
	PMIC_RG_ABIDEC_RESERVED_VAUDP15,
	PMIC_RG_AUDBIASADJ_0_VAUDP15,
	PMIC_RG_AUDBIASADJ_1_VAUDP15,
	PMIC_RG_AUDIBIASPWRDN_VAUDP15,
	PMIC_RG_RSTB_DECODER_VA28,
	PMIC_RG_RSTB_ENCODER_VA28,
	PMIC_RG_SEL_DECODER_96K_VA28,
	PMIC_RG_SEL_ENCODER_96K_VA28,
	PMIC_RG_SEL_DELAY_VCORE,
	PMIC_RG_HCLDO_EN_VA18,
	PMIC_RG_LCLDO_EN_VA18,
	PMIC_RG_LCLDO_ENC_EN_VA28,
	PMIC_RG_VA33REFGEN_EN_VA18,
	PMIC_RG_HCLDO_PDDIS_EN_VA18,
	PMIC_RG_HCLDO_REMOTE_SENSE_VA18,
	PMIC_RG_LCLDO_PDDIS_EN_VA18,
	PMIC_RG_LCLDO_REMOTE_SENSE_VA18,
	PMIC_RG_LCLDO_VOSEL_VA18,
	PMIC_RG_HCLDO_VOSEL_VA18,
	PMIC_RG_LCLDO_ENC_PDDIS_EN_VA28,
	PMIC_RG_LCLDO_ENC_REMOTE_SENSE_VA28,
	PMIC_RG_VA28REFGEN_EN_VA28,
	PMIC_RG_AUDPMU_RESERVED_VA28,
	PMIC_RG_AUDPMU_RESERVED_VA18,
	PMIC_RG_AUDPMU_RESERVED_VAUDP15,
	PMIC_RG_NVREG_EN_VAUDP15,
	PMIC_RG_NVREG_PULL0V_VAUDP15,
	PMIC_RG_AUDGLB_PWRDN_VA28,
	PMIC_RG_AUDPREAMPLON,
	PMIC_RG_AUDPREAMPLDCCEN,
	PMIC_RG_AUDPREAMPLDCRPECHARGE,
	PMIC_RG_AUDPREAMPLPGATEST,
	PMIC_RG_AUDPREAMPLVSCALE,
	PMIC_RG_AUDPREAMPLINPUTSEL,
	PMIC_RG_AUDADCLPWRUP,
	PMIC_RG_AUDADCLINPUTSEL,
	PMIC_RG_AUDPREAMPRON,
	PMIC_RG_AUDPREAMPRDCCEN,
	PMIC_RG_AUDPREAMPRDCRPECHARGE,
	PMIC_RG_AUDPREAMPRPGATEST,
	PMIC_RG_AUDPREAMPRVSCALE,
	PMIC_RG_AUDPREAMPRINPUTSEL,
	PMIC_RG_AUDADCRPWRUP,
	PMIC_RG_AUDADCRINPUTSEL,
	PMIC_RG_AUDULHALFBIAS,
	PMIC_RG_AUDGLBMADLPWEN,
	PMIC_RG_AUDPREAMPLPEN,
	PMIC_RG_AUDADC1STSTAGELPEN,
	PMIC_RG_AUDADC2NDSTAGELPEN,
	PMIC_RG_AUDADCFLASHLPEN,
	PMIC_RG_AUDPREAMPIDDTEST,
	PMIC_RG_AUDADC1STSTAGEIDDTEST,
	PMIC_RG_AUDADC2NDSTAGEIDDTEST,
	PMIC_RG_AUDADCREFBUFIDDTEST,
	PMIC_RG_AUDADCFLASHIDDTEST,
	PMIC_RG_AUDADCDAC0P25FS,
	PMIC_RG_AUDADCCLKSEL,
	PMIC_RG_AUDADCCLKSOURCE,
	PMIC_RG_AUDADCCLKGENMODE,
	PMIC_RG_AUDPREAMPAAFEN,
	PMIC_RG_DCCVCMBUFLPMODSEL,
	PMIC_RG_DCCVCMBUFLPSWEN,
	PMIC_RG_AUDSPAREPGA,
	PMIC_RG_AUDADC1STSTAGESDENB,
	PMIC_RG_AUDADC2NDSTAGERESET,
	PMIC_RG_AUDADC3RDSTAGERESET,
	PMIC_RG_AUDADCFSRESET,
	PMIC_RG_AUDADCWIDECM,
	PMIC_RG_AUDADCNOPATEST,
	PMIC_RG_AUDADCBYPASS,
	PMIC_RG_AUDADCFFBYPASS,
	PMIC_RG_AUDADCDACFBCURRENT,
	PMIC_RG_AUDADCDACIDDTEST,
	PMIC_RG_AUDADCDACNRZ,
	PMIC_RG_AUDADCNODEM,
	PMIC_RG_AUDADCDACTEST,
	PMIC_RG_AUDADCTESTDATA,
	PMIC_RG_AUDRCTUNEL,
	PMIC_RG_AUDRCTUNELSEL,
	PMIC_RG_AUDRCTUNER,
	PMIC_RG_AUDRCTUNERSEL,
	PMIC_RG_AUDSPAREVA28,
	PMIC_RG_AUDSPAREVA18,
	PMIC_RG_AUDENCSPAREVA28,
	PMIC_RG_AUDENCSPAREVA18,
	PMIC_RG_AUDDIGMICEN,
	PMIC_RG_AUDDIGMICBIAS,
	PMIC_RG_DMICHPCLKEN,
	PMIC_RG_AUDDIGMICPDUTY,
	PMIC_RG_AUDDIGMICNDUTY,
	PMIC_RG_DMICMONEN,
	PMIC_RG_DMICMONSEL,
	PMIC_RG_AUDSPAREVMIC,
	PMIC_RG_AUDPWDBMICBIAS0,
	PMIC_RG_AUDMICBIAS0DCSWPEN,
	PMIC_RG_AUDMICBIAS0DCSWNEN,
	PMIC_RG_AUDMICBIAS0BYPASSEN,
	PMIC_RG_AUDPWDBMICBIAS1,
	PMIC_RG_AUDMICBIAS1DCSWPEN,
	PMIC_RG_AUDMICBIAS1DCSWNEN,
	PMIC_RG_AUDMICBIAS1BYPASSEN,
	PMIC_RG_AUDMICBIASVREF,
	PMIC_RG_AUDMICBIASLOWPEN,
	PMIC_RG_BANDGAPGEN,
	PMIC_RG_AUDPREAMPLGAIN,
	PMIC_RG_AUDPREAMPRGAIN,
	PMIC_RG_DIVCKS_CHG,
	PMIC_RG_DIVCKS_ON,
	PMIC_RG_DIVCKS_PRG,
	PMIC_RG_DIVCKS_PWD_NCP,
	PMIC_RG_DIVCKS_PWD_NCP_ST_SEL,
	PMIC_XO_EXTBUF1_MODE,
	PMIC_XO_EXTBUF1_EN_M,
	PMIC_XO_EXTBUF2_MODE,
	PMIC_XO_EXTBUF2_EN_M,
	PMIC_XO_EXTBUF3_MODE,
	PMIC_XO_EXTBUF3_EN_M,
	PMIC_XO_EXTBUF4_MODE,
	PMIC_XO_EXTBUF4_EN_M,
	PMIC_XO_BB_LPM_EN,
	PMIC_XO_ENBB_MAN,
	PMIC_XO_ENBB_EN_M,
	PMIC_XO_CLKSEL_MAN,
	PMIC_DCXO_CW00_SET,
	PMIC_DCXO_CW00_CLR,
	PMIC_XO_CLKSEL_EN_M,
	PMIC_XO_EXTBUF1_CKG_MAN,
	PMIC_XO_EXTBUF1_CKG_EN_M,
	PMIC_XO_EXTBUF2_CKG_MAN,
	PMIC_XO_EXTBUF2_CKG_EN_M,
	PMIC_XO_EXTBUF3_CKG_MAN,
	PMIC_XO_EXTBUF3_CKG_EN_M,
	PMIC_XO_EXTBUF4_CKG_MAN,
	PMIC_XO_EXTBUF4_CKG_EN_M,
	PMIC_XO_INTBUF_MAN,
	PMIC_XO_PBUF_EN_M,
	PMIC_XO_IBUF_EN_M,
	PMIC_XO_LPMBUF_MAN,
	PMIC_XO_LPM_PREBUF_EN_M,
	PMIC_XO_LPBUF_EN_M,
	PMIC_XO_AUDIO_EN_M,
	PMIC_XO_EN32K_MAN,
	PMIC_XO_EN32K_M,
	PMIC_XO_XMODE_MAN,
	PMIC_XO_XMODE_M,
	PMIC_XO_STRUP_MODE,
	PMIC_XO_AAC_FPM_TIME,
	PMIC_XO_AAC_MODE_LPM,
	PMIC_XO_AAC_MODE_FPM,
	PMIC_XO_EN26M_OFFSQ_EN,
	PMIC_XO_LDOCAL_EN,
	PMIC_XO_CBANK_SYNC_DYN,
	PMIC_XO_26MLP_MAN_EN,
	PMIC_XO_OPMODE_MAN,
	PMIC_XO_BUFLDO13_VSET_M,
	PMIC_XO_RESERVED0,
	PMIC_XO_LPM_ISEL_MAN,
	PMIC_XO_FPM_ISEL_MAN,
	PMIC_XO_CDAC_FPM,
	PMIC_XO_CDAC_LPM,
	PMIC_XO_32KDIV_NFRAC_FPM,
	PMIC_XO_COFST_FPM,
	PMIC_XO_32KDIV_NFRAC_LPM,
	PMIC_XO_COFST_LPM,
	PMIC_XO_CORE_MAN,
	PMIC_XO_CORE_EN_M,
	PMIC_XO_CORE_TURBO_EN_M,
	PMIC_XO_CORE_AAC_EN_M,
	PMIC_XO_STARTUP_EN_M,
	PMIC_XO_CORE_VBFPM_EN_M,
	PMIC_XO_CORE_VBLPM_EN_M,
	PMIC_XO_LPMBIAS_EN_M,
	PMIC_XO_VTCGEN_EN_M,
	PMIC_XO_IAAC_COMP_EN_M,
	PMIC_XO_IFPM_COMP_EN_M,
	PMIC_XO_ILPM_COMP_EN_M,
	PMIC_XO_CORE_BYPCAS_FPM,
	PMIC_XO_CORE_GMX2_FPM,
	PMIC_XO_CORE_IDAC_FPM,
	PMIC_XO_AAC_COMP_MAN,
	PMIC_XO_AAC_EN_M,
	PMIC_XO_AAC_MONEN_M,
	PMIC_XO_COMP_EN_M,
	PMIC_XO_COMP_TSTEN_M,
	PMIC_XO_AAC_HV_FPM,
	PMIC_XO_AAC_IBIAS_FPM,
	PMIC_XO_AAC_VOFST_FPM,
	PMIC_XO_AAC_COMP_HV_FPM,
	PMIC_XO_AAC_VSEL_FPM,
	PMIC_XO_AAC_COMP_POL,
	PMIC_XO_CORE_BYPCAS_LPM,
	PMIC_XO_CORE_GMX2_LPM,
	PMIC_XO_CORE_IDAC_LPM,
	PMIC_XO_AAC_COMP_HV_LPM,
	PMIC_XO_AAC_VSEL_LPM,
	PMIC_XO_AAC_HV_LPM,
	PMIC_XO_AAC_IBIAS_LPM,
	PMIC_XO_AAC_VOFST_LPM,
	PMIC_XO_AAC_FPM_SWEN,
	PMIC_XO_SWRST,
	PMIC_XO_32KDIV_SWRST,
	PMIC_XO_32KDIV_RATIO_MAN,
	PMIC_XO_32KDIV_TEST_EN,
	PMIC_XO_CBANK_SYNC_MAN,
	PMIC_XO_CBANK_SYNC_EN_M,
	PMIC_XO_CTL_SYNC_MAN,
	PMIC_XO_CTL_SYNC_EN_M,
	PMIC_XO_LDO_MAN,
	PMIC_XO_LDOPBUF_EN_M,
	PMIC_XO_LDOPBUF_VSET_M,
	PMIC_XO_LDOVTST_EN_M,
	PMIC_XO_TEST_VCAL_EN_M,
	PMIC_XO_VBIST_EN_M,
	PMIC_XO_VTEST_SEL_MUX,
	PMIC_RG_XO_CORE_OSCTD,
	PMIC_RG_XO_THADC_EN,
	PMIC_RG_XO_SYNC_CKPOL,
	PMIC_RG_XO_CBANK_POL,
	PMIC_RG_XO_CBANK_SYNC_BYP,
	PMIC_RG_XO_CTL_POL,
	PMIC_RG_XO_CTL_SYNC_BYP,
	PMIC_RG_XO_LPBUF_INV,
	PMIC_RG_XO_LDOPBUF_BYP,
	PMIC_RG_XO_LDOPBUF_ENCL,
	PMIC_RG_XO_VGBIAS_VSET,
	PMIC_RG_XO_PBUF_ISET,
	PMIC_RG_XO_IBUF_ISET,
	PMIC_RG_XO_BUFLDO13_ENCL,
	PMIC_RG_XO_BUFLDO13_IBX2,
	PMIC_RG_XO_BUFLDO13_IX2,
	PMIC_XO_RESERVED1,
	PMIC_RG_XO_BUFLDO24_ENCL,
	PMIC_RG_XO_BUFLDO24_IBX2,
	PMIC_XO_BUFLDO24_VSET_M,
	PMIC_RG_XO_EXTBUF1_HD,
	PMIC_RG_XO_EXTBUF1_ISET,
	PMIC_RG_XO_EXTBUF2_HD,
	PMIC_RG_XO_EXTBUF2_ISET,
	PMIC_RG_XO_EXTBUF3_HD,
	PMIC_RG_XO_EXTBUF3_ISET,
	PMIC_RG_XO_EXTBUF4_HD,
	PMIC_RG_XO_EXTBUF4_ISET,
	PMIC_RG_XO_LPM_PREBUF_ISET,
	PMIC_RG_XO_AUDIO_ATTEN,
	PMIC_RG_XO_AUDIO_ISET,
	PMIC_XO_EXTBUF4_CLKSEL_MAN,
	PMIC_RG_XO_RESERVED0,
	PMIC_XO_VIO18PG_BUFEN,
	PMIC_RG_XO_RESERVED1,
	PMIC_XO_CAL_EN_MAN,
	PMIC_XO_CAL_EN_M,
	PMIC_XO_STATIC_AUXOUT_SEL,
	PMIC_XO_AUXOUT_SEL,
	PMIC_XO_STATIC_AUXOUT,
	PMIC_AUXADC_ADC_OUT_CH0,
	PMIC_AUXADC_ADC_RDY_CH0,
	PMIC_AUXADC_ADC_OUT_CH1,
	PMIC_AUXADC_ADC_RDY_CH1,
	PMIC_AUXADC_ADC_OUT_CH2,
	PMIC_AUXADC_ADC_RDY_CH2,
	PMIC_AUXADC_ADC_OUT_CH3,
	PMIC_AUXADC_ADC_RDY_CH3,
	PMIC_AUXADC_ADC_OUT_CH4,
	PMIC_AUXADC_ADC_RDY_CH4,
	PMIC_AUXADC_ADC_OUT_CH5,
	PMIC_AUXADC_ADC_RDY_CH5,
	PMIC_AUXADC_ADC_OUT_CH6,
	PMIC_AUXADC_ADC_RDY_CH6,
	PMIC_AUXADC_ADC_OUT_CH7,
	PMIC_AUXADC_ADC_RDY_CH7,
	PMIC_AUXADC_ADC_OUT_CH8,
	PMIC_AUXADC_ADC_RDY_CH8,
	PMIC_AUXADC_ADC_OUT_CH9,
	PMIC_AUXADC_ADC_RDY_CH9,
	PMIC_AUXADC_ADC_OUT_CH10,
	PMIC_AUXADC_ADC_RDY_CH10,
	PMIC_AUXADC_ADC_OUT_CH11,
	PMIC_AUXADC_ADC_RDY_CH11,
	PMIC_AUXADC_ADC_OUT_CH12_15,
	PMIC_AUXADC_ADC_RDY_CH12_15,
	PMIC_AUXADC_ADC_OUT_THR_HW,
	PMIC_AUXADC_ADC_RDY_THR_HW,
	PMIC_AUXADC_ADC_OUT_LBAT,
	PMIC_AUXADC_ADC_RDY_LBAT,
	PMIC_AUXADC_ADC_OUT_LBAT2,
	PMIC_AUXADC_ADC_RDY_LBAT2,
	PMIC_AUXADC_ADC_OUT_CH7_BY_GPS,
	PMIC_AUXADC_ADC_RDY_CH7_BY_GPS,
	PMIC_AUXADC_ADC_OUT_CH7_BY_MD,
	PMIC_AUXADC_ADC_RDY_CH7_BY_MD,
	PMIC_AUXADC_ADC_OUT_CH7_BY_AP,
	PMIC_AUXADC_ADC_RDY_CH7_BY_AP,
	PMIC_AUXADC_ADC_OUT_CH4_BY_MD,
	PMIC_AUXADC_ADC_RDY_CH4_BY_MD,
	PMIC_AUXADC_ADC_OUT_WAKEUP_PCHR,
	PMIC_AUXADC_ADC_RDY_WAKEUP_PCHR,
	PMIC_AUXADC_ADC_OUT_WAKEUP_SWCHR,
	PMIC_AUXADC_ADC_RDY_WAKEUP_SWCHR,
	PMIC_AUXADC_ADC_OUT_CH0_BY_MD,
	PMIC_AUXADC_ADC_RDY_CH0_BY_MD,
	PMIC_AUXADC_ADC_OUT_CH0_BY_AP,
	PMIC_AUXADC_ADC_RDY_CH0_BY_AP,
	PMIC_AUXADC_ADC_OUT_CH1_BY_MD,
	PMIC_AUXADC_ADC_RDY_CH1_BY_MD,
	PMIC_AUXADC_ADC_OUT_CH1_BY_AP,
	PMIC_AUXADC_ADC_RDY_CH1_BY_AP,
	PMIC_AUXADC_ADC_OUT_VISMPS0,
	PMIC_AUXADC_ADC_RDY_VISMPS0,
	PMIC_AUXADC_ADC_OUT_FGADC1,
	PMIC_AUXADC_ADC_RDY_FGADC1,
	PMIC_AUXADC_ADC_OUT_FGADC2,
	PMIC_AUXADC_ADC_RDY_FGADC2,
	PMIC_AUXADC_ADC_OUT_IMP,
	PMIC_AUXADC_ADC_RDY_IMP,
	PMIC_AUXADC_ADC_OUT_IMP_AVG,
	PMIC_AUXADC_ADC_RDY_IMP_AVG,
	PMIC_AUXADC_ADC_OUT_RAW,
	PMIC_AUXADC_ADC_OUT_MDRT,
	PMIC_AUXADC_ADC_RDY_MDRT,
	PMIC_AUXADC_ADC_OUT_MDBG,
	PMIC_AUXADC_ADC_RDY_MDBG,
	PMIC_AUXADC_ADC_OUT_DCXO_BY_GPS,
	PMIC_AUXADC_ADC_RDY_DCXO_BY_GPS,
	PMIC_AUXADC_ADC_OUT_DCXO_BY_MD,
	PMIC_AUXADC_ADC_RDY_DCXO_BY_MD,
	PMIC_AUXADC_ADC_OUT_DCXO_BY_AP,
	PMIC_AUXADC_ADC_RDY_DCXO_BY_AP,
	PMIC_AUXADC_ADC_OUT_DCXO_MDRT,
	PMIC_AUXADC_ADC_RDY_DCXO_MDRT,
	PMIC_AUXADC_ADC_OUT_NAG,
	PMIC_AUXADC_ADC_RDY_NAG,
	PMIC_AUXADC_ADC_OUT_TYPEC_H,
	PMIC_AUXADC_ADC_RDY_TYPEC_H,
	PMIC_AUXADC_ADC_OUT_TYPEC_L,
	PMIC_AUXADC_ADC_RDY_TYPEC_L,
	PMIC_AUXADC_BUF_OUT_00,
	PMIC_AUXADC_BUF_RDY_00,
	PMIC_AUXADC_BUF_OUT_01,
	PMIC_AUXADC_BUF_RDY_01,
	PMIC_AUXADC_BUF_OUT_02,
	PMIC_AUXADC_BUF_RDY_02,
	PMIC_AUXADC_BUF_OUT_03,
	PMIC_AUXADC_BUF_RDY_03,
	PMIC_AUXADC_BUF_OUT_04,
	PMIC_AUXADC_BUF_RDY_04,
	PMIC_AUXADC_BUF_OUT_05,
	PMIC_AUXADC_BUF_RDY_05,
	PMIC_AUXADC_BUF_OUT_06,
	PMIC_AUXADC_BUF_RDY_06,
	PMIC_AUXADC_BUF_OUT_07,
	PMIC_AUXADC_BUF_RDY_07,
	PMIC_AUXADC_BUF_OUT_08,
	PMIC_AUXADC_BUF_RDY_08,
	PMIC_AUXADC_BUF_OUT_09,
	PMIC_AUXADC_BUF_RDY_09,
	PMIC_AUXADC_BUF_OUT_10,
	PMIC_AUXADC_BUF_RDY_10,
	PMIC_AUXADC_BUF_OUT_11,
	PMIC_AUXADC_BUF_RDY_11,
	PMIC_AUXADC_BUF_OUT_12,
	PMIC_AUXADC_BUF_RDY_12,
	PMIC_AUXADC_BUF_OUT_13,
	PMIC_AUXADC_BUF_RDY_13,
	PMIC_AUXADC_BUF_OUT_14,
	PMIC_AUXADC_BUF_RDY_14,
	PMIC_AUXADC_BUF_OUT_15,
	PMIC_AUXADC_BUF_RDY_15,
	PMIC_AUXADC_BUF_OUT_16,
	PMIC_AUXADC_BUF_RDY_16,
	PMIC_AUXADC_BUF_OUT_17,
	PMIC_AUXADC_BUF_RDY_17,
	PMIC_AUXADC_BUF_OUT_18,
	PMIC_AUXADC_BUF_RDY_18,
	PMIC_AUXADC_BUF_OUT_19,
	PMIC_AUXADC_BUF_RDY_19,
	PMIC_AUXADC_BUF_OUT_20,
	PMIC_AUXADC_BUF_RDY_20,
	PMIC_AUXADC_BUF_OUT_21,
	PMIC_AUXADC_BUF_RDY_21,
	PMIC_AUXADC_BUF_OUT_22,
	PMIC_AUXADC_BUF_RDY_22,
	PMIC_AUXADC_BUF_OUT_23,
	PMIC_AUXADC_BUF_RDY_23,
	PMIC_AUXADC_BUF_OUT_24,
	PMIC_AUXADC_BUF_RDY_24,
	PMIC_AUXADC_BUF_OUT_25,
	PMIC_AUXADC_BUF_RDY_25,
	PMIC_AUXADC_BUF_OUT_26,
	PMIC_AUXADC_BUF_RDY_26,
	PMIC_AUXADC_BUF_OUT_27,
	PMIC_AUXADC_BUF_RDY_27,
	PMIC_AUXADC_BUF_OUT_28,
	PMIC_AUXADC_BUF_RDY_28,
	PMIC_AUXADC_BUF_OUT_29,
	PMIC_AUXADC_BUF_RDY_29,
	PMIC_AUXADC_BUF_OUT_30,
	PMIC_AUXADC_BUF_RDY_30,
	PMIC_AUXADC_BUF_OUT_31,
	PMIC_AUXADC_BUF_RDY_31,
	PMIC_AUXADC_ADC_BUSY_IN,
	PMIC_AUXADC_ADC_BUSY_IN_LBAT,
	PMIC_AUXADC_ADC_BUSY_IN_LBAT2,
	PMIC_AUXADC_ADC_BUSY_IN_VISMPS0,
	PMIC_AUXADC_ADC_BUSY_IN_WAKEUP,
	PMIC_AUXADC_ADC_BUSY_IN_DCXO_MDRT,
	PMIC_AUXADC_ADC_BUSY_IN_DCXO_GPS_AP,
	PMIC_AUXADC_ADC_BUSY_IN_DCXO_GPS_MD,
	PMIC_AUXADC_ADC_BUSY_IN_DCXO_GPS,
	PMIC_AUXADC_ADC_BUSY_IN_MDRT,
	PMIC_AUXADC_ADC_BUSY_IN_MDBG,
	PMIC_AUXADC_ADC_BUSY_IN_SHARE,
	PMIC_AUXADC_ADC_BUSY_IN_IMP,
	PMIC_AUXADC_ADC_BUSY_IN_FGADC1,
	PMIC_AUXADC_ADC_BUSY_IN_FGADC2,
	PMIC_AUXADC_ADC_BUSY_IN_GPS_AP,
	PMIC_AUXADC_ADC_BUSY_IN_GPS_MD,
	PMIC_AUXADC_ADC_BUSY_IN_GPS,
	PMIC_AUXADC_ADC_BUSY_IN_THR_HW,
	PMIC_AUXADC_ADC_BUSY_IN_THR_MD,
	PMIC_AUXADC_ADC_BUSY_IN_TYPEC_H,
	PMIC_AUXADC_ADC_BUSY_IN_TYPEC_L,
	PMIC_AUXADC_ADC_BUSY_IN_NAG,
	PMIC_AUXADC_RQST_CH0,
	PMIC_AUXADC_RQST_CH1,
	PMIC_AUXADC_RQST_CH2,
	PMIC_AUXADC_RQST_CH3,
	PMIC_AUXADC_RQST_CH4,
	PMIC_AUXADC_RQST_CH5,
	PMIC_AUXADC_RQST_CH6,
	PMIC_AUXADC_RQST_CH7,
	PMIC_AUXADC_RQST_CH8,
	PMIC_AUXADC_RQST_CH9,
	PMIC_AUXADC_RQST_CH10,
	PMIC_AUXADC_RQST_CH11,
	PMIC_AUXADC_RQST_CH12,
	PMIC_AUXADC_RQST_CH13,
	PMIC_AUXADC_RQST_CH14,
	PMIC_AUXADC_RQST_CH15,
	PMIC_AUXADC_RQST0_SET,
	PMIC_AUXADC_RQST0_CLR,
	PMIC_AUXADC_RQST_CH0_BY_MD,
	PMIC_AUXADC_RQST_CH1_BY_MD,
	PMIC_AUXADC_RQST_RSV0,
	PMIC_AUXADC_RQST_CH4_BY_MD,
	PMIC_AUXADC_RQST_CH7_BY_MD,
	PMIC_AUXADC_RQST_CH7_BY_GPS,
	PMIC_AUXADC_RQST_DCXO_BY_MD,
	PMIC_AUXADC_RQST_DCXO_BY_GPS,
	PMIC_AUXADC_RQST_RSV1,
	PMIC_AUXADC_RQST1_SET,
	PMIC_AUXADC_RQST1_CLR,
	PMIC_AUXADC_CK_ON_EXTD,
	PMIC_AUXADC_SRCLKEN_SRC_SEL,
	PMIC_AUXADC_ADC_PWDB,
	PMIC_AUXADC_ADC_PWDB_SWCTRL,
	PMIC_AUXADC_STRUP_CK_ON_ENB,
	PMIC_AUXADC_ADC_RDY_WAKEUP_CLR,
	PMIC_AUXADC_SRCLKEN_CK_EN,
	PMIC_AUXADC_CK_AON_GPS,
	PMIC_AUXADC_CK_AON_MD,
	PMIC_AUXADC_CK_AON,
	PMIC_AUXADC_CON0_SET,
	PMIC_AUXADC_CON0_CLR,
	PMIC_AUXADC_AVG_NUM_SMALL,
	PMIC_AUXADC_AVG_NUM_LARGE,
	PMIC_AUXADC_SPL_NUM,
	PMIC_AUXADC_AVG_NUM_SEL,
	PMIC_AUXADC_AVG_NUM_SEL_SHARE,
	PMIC_AUXADC_AVG_NUM_SEL_LBAT,
	PMIC_AUXADC_AVG_NUM_SEL_VISMPS,
	PMIC_AUXADC_AVG_NUM_SEL_WAKEUP,
	PMIC_AUXADC_SPL_NUM_LARGE,
	PMIC_AUXADC_SPL_NUM_SLEEP,
	PMIC_AUXADC_SPL_NUM_SLEEP_SEL,
	PMIC_AUXADC_SPL_NUM_SEL,
	PMIC_AUXADC_SPL_NUM_SEL_SHARE,
	PMIC_AUXADC_SPL_NUM_SEL_LBAT,
	PMIC_AUXADC_SPL_NUM_SEL_VISMPS,
	PMIC_AUXADC_SPL_NUM_SEL_WAKEUP,
	PMIC_AUXADC_TRIM_CH0_SEL,
	PMIC_AUXADC_TRIM_CH1_SEL,
	PMIC_AUXADC_TRIM_CH2_SEL,
	PMIC_AUXADC_TRIM_CH3_SEL,
	PMIC_AUXADC_TRIM_CH4_SEL,
	PMIC_AUXADC_TRIM_CH5_SEL,
	PMIC_AUXADC_TRIM_CH6_SEL,
	PMIC_AUXADC_TRIM_CH7_SEL,
	PMIC_AUXADC_TRIM_CH8_SEL,
	PMIC_AUXADC_TRIM_CH9_SEL,
	PMIC_AUXADC_TRIM_CH10_SEL,
	PMIC_AUXADC_TRIM_CH11_SEL,
	PMIC_AUXADC_ADC_2S_COMP_ENB,
	PMIC_AUXADC_ADC_TRIM_COMP,
	PMIC_AUXADC_SW_GAIN_TRIM,
	PMIC_AUXADC_SW_OFFSET_TRIM,
	PMIC_AUXADC_RNG_EN,
	PMIC_AUXADC_DATA_REUSE_SEL,
	PMIC_AUXADC_TEST_MODE,
	PMIC_AUXADC_BIT_SEL,
	PMIC_AUXADC_START_SW,
	PMIC_AUXADC_START_SWCTRL,
	PMIC_AUXADC_TS_VBE_SEL,
	PMIC_AUXADC_TS_VBE_SEL_SWCTRL,
	PMIC_AUXADC_VBUF_EN,
	PMIC_AUXADC_VBUF_EN_SWCTRL,
	PMIC_AUXADC_OUT_SEL,
	PMIC_AUXADC_DA_DAC,
	PMIC_AUXADC_DA_DAC_SWCTRL,
	PMIC_AD_AUXADC_COMP,
	PMIC_RG_VBUF_EXTEN,
	PMIC_RG_VBUF_CALEN,
	PMIC_RG_VBUF_BYP,
	PMIC_RG_AUX_RSV,
	PMIC_RG_AUXADC_CALI,
	PMIC_RG_VBUF_EN,
	PMIC_RG_VBE_SEL,
	PMIC_AUXADC_ADCIN_VSEN_EN,
	PMIC_AUXADC_ADCIN_VBAT_EN,
	PMIC_AUXADC_ADCIN_VSEN_MUX_EN,
	PMIC_AUXADC_ADCIN_VSEN_EXT_BATON_EN,
	PMIC_AUXADC_ADCIN_CHR_EN,
	PMIC_AUXADC_ADCIN_BATON_TDET_EN,
	PMIC_AUXADC_ACCDET_ANASWCTRL_EN,
	PMIC_AUXADC_DIG0_RSV0,
	PMIC_AUXADC_CHSEL,
	PMIC_AUXADC_SWCTRL_EN,
	PMIC_AUXADC_SOURCE_LBAT_SEL,
	PMIC_AUXADC_SOURCE_LBAT2_SEL,
	PMIC_AUXADC_DIG0_RSV2,
	PMIC_AUXADC_DIG1_RSV2,
	PMIC_AUXADC_DAC_EXTD,
	PMIC_AUXADC_DAC_EXTD_EN,
	PMIC_AUXADC_PMU_THR_PDN_SW,
	PMIC_AUXADC_PMU_THR_PDN_SEL,
	PMIC_AUXADC_PMU_THR_PDN_STATUS,
	PMIC_AUXADC_DIG0_RSV1,
	PMIC_AUXADC_START_SHADE_NUM,
	PMIC_AUXADC_START_SHADE_EN,
	PMIC_AUXADC_START_SHADE_SEL,
	PMIC_AUXADC_AUTORPT_PRD,
	PMIC_AUXADC_AUTORPT_EN,
	PMIC_AUXADC_LBAT_DEBT_MAX,
	PMIC_AUXADC_LBAT_DEBT_MIN,
	PMIC_AUXADC_LBAT_DET_PRD_15_0,
	PMIC_AUXADC_LBAT_DET_PRD_19_16,
	PMIC_AUXADC_LBAT_VOLT_MAX,
	PMIC_AUXADC_LBAT_IRQ_EN_MAX,
	PMIC_AUXADC_LBAT_EN_MAX,
	PMIC_AUXADC_LBAT_MAX_IRQ_B,
	PMIC_AUXADC_LBAT_VOLT_MIN,
	PMIC_AUXADC_LBAT_IRQ_EN_MIN,
	PMIC_AUXADC_LBAT_EN_MIN,
	PMIC_AUXADC_LBAT_MIN_IRQ_B,
	PMIC_AUXADC_LBAT_DEBOUNCE_COUNT_MAX,
	PMIC_AUXADC_LBAT_DEBOUNCE_COUNT_MIN,
	PMIC_AUXADC_ACCDET_AUTO_SPL,
	PMIC_AUXADC_ACCDET_AUTO_RQST_CLR,
	PMIC_AUXADC_ACCDET_DIG1_RSV0,
	PMIC_AUXADC_ACCDET_DIG0_RSV0,
	PMIC_AUXADC_THR_DEBT_MAX,
	PMIC_AUXADC_THR_DEBT_MIN,
	PMIC_AUXADC_THR_DET_PRD_15_0,
	PMIC_AUXADC_THR_DET_PRD_19_16,
	PMIC_AUXADC_THR_VOLT_MAX,
	PMIC_AUXADC_THR_IRQ_EN_MAX,
	PMIC_AUXADC_THR_EN_MAX,
	PMIC_AUXADC_THR_MAX_IRQ_B,
	PMIC_AUXADC_THR_VOLT_MIN,
	PMIC_AUXADC_THR_IRQ_EN_MIN,
	PMIC_AUXADC_THR_EN_MIN,
	PMIC_AUXADC_THR_MIN_IRQ_B,
	PMIC_AUXADC_THR_DEBOUNCE_COUNT_MAX,
	PMIC_AUXADC_THR_DEBOUNCE_COUNT_MIN,
	PMIC_EFUSE_GAIN_CH4_TRIM,
	PMIC_EFUSE_OFFSET_CH4_TRIM,
	PMIC_EFUSE_GAIN_CH0_TRIM,
	PMIC_EFUSE_OFFSET_CH0_TRIM,
	PMIC_EFUSE_GAIN_CH7_TRIM,
	PMIC_EFUSE_OFFSET_CH7_TRIM,
	PMIC_AUXADC_FGADC_START_SW,
	PMIC_AUXADC_FGADC_START_SEL,
	PMIC_AUXADC_FGADC_R_SW,
	PMIC_AUXADC_FGADC_R_SEL,
	PMIC_AUXADC_DBG_DIG0_RSV2,
	PMIC_AUXADC_DBG_DIG1_RSV2,
	PMIC_AUXADC_IMPEDANCE_CNT,
	PMIC_AUXADC_IMPEDANCE_CHSEL,
	PMIC_AUXADC_IMPEDANCE_IRQ_CLR,
	PMIC_AUXADC_IMPEDANCE_IRQ_STATUS,
	PMIC_AUXADC_CLR_IMP_CNT_STOP,
	PMIC_AUXADC_IMPEDANCE_MODE,
	PMIC_AUXADC_IMP_AUTORPT_PRD,
	PMIC_AUXADC_IMP_AUTORPT_EN,
	PMIC_AUXADC_VISMPS0_DEBT_MAX,
	PMIC_AUXADC_VISMPS0_DEBT_MIN,
	PMIC_AUXADC_VISMPS0_DET_PRD_15_0,
	PMIC_AUXADC_VISMPS0_DET_PRD_19_16,
	PMIC_AUXADC_VISMPS0_VOLT_MAX,
	PMIC_AUXADC_VISMPS0_IRQ_EN_MAX,
	PMIC_AUXADC_VISMPS0_EN_MAX,
	PMIC_AUXADC_VISMPS0_MAX_IRQ_B,
	PMIC_AUXADC_VISMPS0_VOLT_MIN,
	PMIC_AUXADC_VISMPS0_IRQ_EN_MIN,
	PMIC_AUXADC_VISMPS0_EN_MIN,
	PMIC_AUXADC_VISMPS0_MIN_IRQ_B,
	PMIC_AUXADC_VISMPS0_DEBOUNCE_COUNT_MAX,
	PMIC_AUXADC_VISMPS0_DEBOUNCE_COUNT_MIN,
	PMIC_AUXADC_LBAT2_DEBT_MAX,
	PMIC_AUXADC_LBAT2_DEBT_MIN,
	PMIC_AUXADC_LBAT2_DET_PRD_15_0,
	PMIC_AUXADC_LBAT2_DET_PRD_19_16,
	PMIC_AUXADC_LBAT2_VOLT_MAX,
	PMIC_AUXADC_LBAT2_IRQ_EN_MAX,
	PMIC_AUXADC_LBAT2_EN_MAX,
	PMIC_AUXADC_LBAT2_MAX_IRQ_B,
	PMIC_AUXADC_LBAT2_VOLT_MIN,
	PMIC_AUXADC_LBAT2_IRQ_EN_MIN,
	PMIC_AUXADC_LBAT2_EN_MIN,
	PMIC_AUXADC_LBAT2_MIN_IRQ_B,
	PMIC_AUXADC_LBAT2_DEBOUNCE_COUNT_MAX,
	PMIC_AUXADC_LBAT2_DEBOUNCE_COUNT_MIN,
	PMIC_AUXADC_MDBG_DET_PRD,
	PMIC_AUXADC_MDBG_DET_EN,
	PMIC_AUXADC_MDBG_R_PTR,
	PMIC_AUXADC_MDBG_W_PTR,
	PMIC_AUXADC_MDBG_BUF_LENGTH,
	PMIC_AUXADC_MDRT_DET_PRD,
	PMIC_AUXADC_MDRT_DET_EN,
	PMIC_AUXADC_MDRT_DET_WKUP_START_CNT,
	PMIC_AUXADC_MDRT_DET_WKUP_START_CLR,
	PMIC_AUXADC_MDRT_DET_WKUP_START,
	PMIC_AUXADC_MDRT_DET_WKUP_START_SEL,
	PMIC_AUXADC_MDRT_DET_WKUP_EN,
	PMIC_AUXADC_MDRT_DET_SRCLKEN_IND,
	PMIC_AUXADC_DCXO_MDRT_DET_PRD,
	PMIC_AUXADC_DCXO_MDRT_DET_EN,
	PMIC_AUXADC_DCXO_MDRT_DET_WKUP_START_CNT,
	PMIC_AUXADC_DCXO_MDRT_DET_WKUP_START_CLR,
	PMIC_AUXADC_DCXO_MDRT_DET_WKUP_EN,
	PMIC_AUXADC_DCXO_MDRT_DET_WKUP_START_SEL,
	PMIC_AUXADC_DCXO_MDRT_DET_WKUP_START,
	PMIC_AUXADC_DCXO_MDRT_DET_SRCLKEN_IND,
	PMIC_AUXADC_DCXO_CH4_MUX_AP_SEL,
	PMIC_AUXADC_NAG_EN,
	PMIC_AUXADC_NAG_CLR,
	PMIC_AUXADC_NAG_VBAT1_SEL,
	PMIC_AUXADC_NAG_PRD,
	PMIC_AUXADC_NAG_IRQ_EN,
	PMIC_AUXADC_NAG_C_DLTV_IRQ,
	PMIC_AUXADC_NAG_ZCV,
	PMIC_AUXADC_NAG_C_DLTV_TH_15_0,
	PMIC_AUXADC_NAG_C_DLTV_TH_26_16,
	PMIC_AUXADC_NAG_CNT_15_0,
	PMIC_AUXADC_NAG_CNT_25_16,
	PMIC_AUXADC_NAG_DLTV,
	PMIC_AUXADC_NAG_C_DLTV_15_0,
	PMIC_AUXADC_NAG_C_DLTV_26_16,
	PMIC_AUXADC_TYPEC_H_DEBT_MAX,
	PMIC_AUXADC_TYPEC_H_DEBT_MIN,
	PMIC_AUXADC_TYPEC_H_DET_PRD_15_0,
	PMIC_AUXADC_TYPEC_H_DET_PRD_19_16,
	PMIC_AUXADC_TYPEC_H_VOLT_MAX,
	PMIC_AUXADC_TYPEC_H_IRQ_EN_MAX,
	PMIC_AUXADC_TYPEC_H_EN_MAX,
	PMIC_AUXADC_TYPEC_H_MAX_IRQ_B,
	PMIC_AUXADC_TYPEC_H_VOLT_MIN,
	PMIC_AUXADC_TYPEC_H_IRQ_EN_MIN,
	PMIC_AUXADC_TYPEC_H_EN_MIN,
	PMIC_AUXADC_TYPEC_H_MIN_IRQ_B,
	PMIC_AUXADC_TYPEC_H_DEBOUNCE_COUNT_MAX,
	PMIC_AUXADC_TYPEC_H_DEBOUNCE_COUNT_MIN,
	PMIC_AUXADC_TYPEC_L_DEBT_MAX,
	PMIC_AUXADC_TYPEC_L_DEBT_MIN,
	PMIC_AUXADC_TYPEC_L_DET_PRD_15_0,
	PMIC_AUXADC_TYPEC_L_DET_PRD_19_16,
	PMIC_AUXADC_TYPEC_L_VOLT_MAX,
	PMIC_AUXADC_TYPEC_L_IRQ_EN_MAX,
	PMIC_AUXADC_TYPEC_L_EN_MAX,
	PMIC_AUXADC_TYPEC_L_MAX_IRQ_B,
	PMIC_AUXADC_TYPEC_L_VOLT_MIN,
	PMIC_AUXADC_TYPEC_L_IRQ_EN_MIN,
	PMIC_AUXADC_TYPEC_L_EN_MIN,
	PMIC_AUXADC_TYPEC_L_MIN_IRQ_B,
	PMIC_AUXADC_TYPEC_L_DEBOUNCE_COUNT_MAX,
	PMIC_AUXADC_TYPEC_L_DEBOUNCE_COUNT_MIN,
	PMIC_RG_AUDACCDETVTHBCAL,
	PMIC_RG_AUDACCDETVTHACAL,
	PMIC_RG_AUDACCDETANASWCTRLENB,
	PMIC_RG_ACCDETSEL,
	PMIC_RG_AUDACCDETSWCTRL,
	PMIC_RG_AUDACCDETMICBIAS1PULLLOW,
	PMIC_RG_AUDACCDETTVDET,
	PMIC_RG_AUDACCDETVIN1PULLLOW,
	PMIC_AUDACCDETAUXADCSWCTRL,
	PMIC_AUDACCDETAUXADCSWCTRL_SEL,
	PMIC_RG_AUDACCDETMICBIAS0PULLLOW,
	PMIC_RG_AUDACCDETRSV,
	PMIC_ACCDET_EN,
	PMIC_ACCDET_SEQ_INIT,
	PMIC_ACCDET_EINTDET_EN,
	PMIC_ACCDET_EINT_SEQ_INIT,
	PMIC_ACCDET_NEGVDET_EN,
	PMIC_ACCDET_NEGVDET_EN_CTRL,
	PMIC_ACCDET_ANASWCTRL_SEL,
	PMIC_ACCDET_CMP_PWM_EN,
	PMIC_ACCDET_VTH_PWM_EN,
	PMIC_ACCDET_MBIAS_PWM_EN,
	PMIC_ACCDET_EINT_PWM_EN,
	PMIC_ACCDET_CMP_PWM_IDLE,
	PMIC_ACCDET_VTH_PWM_IDLE,
	PMIC_ACCDET_MBIAS_PWM_IDLE,
	PMIC_ACCDET_EINT_PWM_IDLE,
	PMIC_ACCDET_PWM_WIDTH,
	PMIC_ACCDET_PWM_THRESH,
	PMIC_ACCDET_RISE_DELAY,
	PMIC_ACCDET_FALL_DELAY,
	PMIC_ACCDET_DEBOUNCE0,
	PMIC_ACCDET_DEBOUNCE1,
	PMIC_ACCDET_DEBOUNCE2,
	PMIC_ACCDET_DEBOUNCE3,
	PMIC_ACCDET_DEBOUNCE4,
	PMIC_ACCDET_IVAL_CUR_IN,
	PMIC_ACCDET_EINT_IVAL_CUR_IN,
	PMIC_ACCDET_IVAL_SAM_IN,
	PMIC_ACCDET_EINT_IVAL_SAM_IN,
	PMIC_ACCDET_IVAL_MEM_IN,
	PMIC_ACCDET_EINT_IVAL_MEM_IN,
	PMIC_ACCDET_EINT_IVAL_SEL,
	PMIC_ACCDET_IVAL_SEL,
	PMIC_ACCDET_IRQ,
	PMIC_ACCDET_NEGV_IRQ,
	PMIC_ACCDET_EINT_IRQ,
	PMIC_ACCDET_IRQ_CLR,
	PMIC_ACCDET_NEGV_IRQ_CLR,
	PMIC_ACCDET_EINT_IRQ_CLR,
	PMIC_ACCDET_EINT_IRQ_POLARITY,
	PMIC_ACCDET_TEST_MODE0,
	PMIC_ACCDET_TEST_MODE1,
	PMIC_ACCDET_TEST_MODE2,
	PMIC_ACCDET_TEST_MODE3,
	PMIC_ACCDET_TEST_MODE4,
	PMIC_ACCDET_TEST_MODE5,
	PMIC_ACCDET_PWM_SEL,
	PMIC_ACCDET_IN_SW,
	PMIC_ACCDET_CMP_EN_SW,
	PMIC_ACCDET_VTH_EN_SW,
	PMIC_ACCDET_MBIAS_EN_SW,
	PMIC_ACCDET_PWM_EN_SW,
	PMIC_ACCDET_IN,
	PMIC_ACCDET_CUR_IN,
	PMIC_ACCDET_SAM_IN,
	PMIC_ACCDET_MEM_IN,
	PMIC_ACCDET_STATE,
	PMIC_ACCDET_MBIAS_CLK,
	PMIC_ACCDET_VTH_CLK,
	PMIC_ACCDET_CMP_CLK,
	PMIC_DA_AUDACCDETAUXADCSWCTRL,
	PMIC_ACCDET_EINT_DEB_SEL,
	PMIC_ACCDET_EINT_DEBOUNCE,
	PMIC_ACCDET_EINT_PWM_THRESH,
	PMIC_ACCDET_EINT_PWM_WIDTH,
	PMIC_ACCDET_NEGV_THRESH,
	PMIC_ACCDET_EINT_PWM_FALL_DELAY,
	PMIC_ACCDET_EINT_PWM_RISE_DELAY,
	PMIC_ACCDET_TEST_MODE13,
	PMIC_ACCDET_TEST_MODE12,
	PMIC_ACCDET_NVDETECTOUT_SW,
	PMIC_ACCDET_TEST_MODE11,
	PMIC_ACCDET_TEST_MODE10,
	PMIC_ACCDET_EINTCMPOUT_SW,
	PMIC_ACCDET_TEST_MODE9,
	PMIC_ACCDET_TEST_MODE8,
	PMIC_ACCDET_AUXADC_CTRL_SW,
	PMIC_ACCDET_TEST_MODE7,
	PMIC_ACCDET_TEST_MODE6,
	PMIC_ACCDET_EINTCMP_EN_SW,
	PMIC_RG_NVCMPSWEN,
	PMIC_RG_NVMODSEL,
	PMIC_RG_SWBUFSWEN,
	PMIC_RG_SWBUFMODSEL,
	PMIC_RG_NVDETVTH,
	PMIC_RG_NVDETCMPEN,
	PMIC_RG_EINTCONFIGACCDET,
	PMIC_RG_EINTCOMPVTH,
	PMIC_ACCDET_EINT_STATE,
	PMIC_ACCDET_AUXADC_DEBOUNCE_END,
	PMIC_ACCDET_AUXADC_CONNECT_PRE,
	PMIC_ACCDET_EINT_CUR_IN,
	PMIC_ACCDET_EINT_SAM_IN,
	PMIC_ACCDET_EINT_MEM_IN,
	PMIC_AD_NVDETECTOUT,
	PMIC_AD_EINTCMPOUT,
	PMIC_DA_NI_EINTCMPEN,
	PMIC_ACCDET_NEGV_COUNT_IN,
	PMIC_ACCDET_NEGV_EN_FINAL,
	PMIC_ACCDET_NEGV_COUNT_END,
	PMIC_ACCDET_NEGV_MINU,
	PMIC_ACCDET_NEGV_ADD,
	PMIC_ACCDET_NEGV_CMP,
	PMIC_ACCDET_CUR_DEB,
	PMIC_ACCDET_EINT_CUR_DEB,
	PMIC_ACCDET_RSV_CON0,
	PMIC_ACCDET_RSV_CON1,
	PMIC_ACCDET_AUXADC_CONNECT_TIME,
	PMIC_RG_VCDT_HV_EN,
	PMIC_RGS_CHR_LDO_DET,
	PMIC_RG_PCHR_AUTOMODE,
	PMIC_RG_CSDAC_EN,
	PMIC_RG_CHR_EN,
	PMIC_RGS_CHRDET,
	PMIC_RGS_VCDT_LV_DET,
	PMIC_RGS_VCDT_HV_DET,
	PMIC_RG_VCDT_LV_VTH,
	PMIC_RG_VCDT_HV_VTH,
	PMIC_RG_VBAT_CV_EN,
	PMIC_RG_VBAT_CC_EN,
	PMIC_RG_CS_EN,
	PMIC_RGS_CS_DET,
	PMIC_RGS_VBAT_CV_DET,
	PMIC_RGS_VBAT_CC_DET,
	PMIC_RG_VBAT_CV_VTH,
	PMIC_RG_VBAT_CC_VTH,
	PMIC_RG_CS_VTH,
	PMIC_RG_PCHR_TOHTC,
	PMIC_RG_PCHR_TOLTC,
	PMIC_RG_VBAT_OV_EN,
	PMIC_RG_VBAT_OV_VTH,
	PMIC_RG_VBAT_OV_DEG,
	PMIC_RGS_VBAT_OV_DET,
	PMIC_RG_BATON_EN,
	PMIC_RG_BATON_HT_EN_RSV0,
	PMIC_BATON_TDET_EN,
	PMIC_RG_BATON_HT_TRIM,
	PMIC_RG_BATON_HT_TRIM_SET,
	PMIC_RG_BATON_TDET_EN,
	PMIC_RG_CSDAC_DATA,
	PMIC_RG_FRC_CSVTH_USBDL,
	PMIC_RGS_PCHR_FLAG_OUT,
	PMIC_RG_PCHR_FLAG_EN,
	PMIC_RG_OTG_BVALID_EN,
	PMIC_RGS_OTG_BVALID_DET,
	PMIC_RG_PCHR_FLAG_SEL,
	PMIC_RG_PCHR_TESTMODE,
	PMIC_RG_CSDAC_TESTMODE,
	PMIC_RG_PCHR_RST,
	PMIC_RG_PCHR_FT_CTRL,
	PMIC_RG_CHRWDT_TD,
	PMIC_RG_CHRWDT_EN,
	PMIC_RG_CHRWDT_WR,
	PMIC_RG_PCHR_RV,
	PMIC_RG_CHRWDT_INT_EN,
	PMIC_RG_CHRWDT_FLAG_WR,
	PMIC_RGS_CHRWDT_OUT,
	PMIC_RG_USBDL_RST,
	PMIC_RG_USBDL_SET,
	PMIC_RG_ADCIN_VSEN_MUX_EN,
	PMIC_RG_ADCIN_VSEN_EXT_BATON_EN,
	PMIC_RG_ADCIN_VBAT_EN,
	PMIC_RG_ADCIN_VSEN_EN,
	PMIC_RG_ADCIN_CHR_EN,
	PMIC_RG_UVLO_VTHL,
	PMIC_RG_UVLO_VH_LAT,
	PMIC_RG_LBAT_INT_VTH,
	PMIC_RG_BGR_RSEL,
	PMIC_RG_BGR_UNCHOP_PH,
	PMIC_RG_BGR_UNCHOP,
	PMIC_RG_BC11_BB_CTRL,
	PMIC_RG_BC11_RST,
	PMIC_RG_BC11_VSRC_EN,
	PMIC_RGS_BC11_CMP_OUT,
	PMIC_RG_BC11_VREF_VTH,
	PMIC_RG_BC11_CMP_EN,
	PMIC_RG_BC11_IPD_EN,
	PMIC_RG_BC11_IPU_EN,
	PMIC_RG_BC11_BIAS_EN,
	PMIC_RG_CSDAC_STP_INC,
	PMIC_RG_CSDAC_STP_DEC,
	PMIC_RG_CSDAC_DLY,
	PMIC_RG_CSDAC_STP,
	PMIC_RG_LOW_ICH_DB,
	PMIC_RG_CHRIND_ON,
	PMIC_RG_CHRIND_DIMMING,
	PMIC_RG_CV_MODE,
	PMIC_RG_VCDT_MODE,
	PMIC_RG_CSDAC_MODE,
	PMIC_RG_TRACKING_EN,
	PMIC_RG_HWCV_EN,
	PMIC_RG_ULC_DET_EN,
	PMIC_RG_BGR_TRIM_EN,
	PMIC_RG_ICHRG_TRIM,
	PMIC_RG_BGR_TRIM,
	PMIC_RG_OVP_TRIM,
	PMIC_RG_CHR_OSC_TRIM,
	PMIC_DA_QI_BGR_EXT_BUF_EN,
	PMIC_RG_BGR_TEST_EN,
	PMIC_RG_BGR_TEST_RSTB,
	PMIC_RG_DAC_USBDL_MAX,
	PMIC_RG_CM_VDEC_TRIG,
	PMIC_PCHR_CM_VDEC_STATUS,
	PMIC_RG_CM_VINC_TRIG,
	PMIC_PCHR_CM_VINC_STATUS,
	PMIC_RG_CM_VDEC_HPRD1,
	PMIC_RG_CM_VDEC_HPRD2,
	PMIC_RG_CM_VDEC_HPRD3,
	PMIC_RG_CM_VDEC_HPRD4,
	PMIC_RG_CM_VDEC_HPRD5,
	PMIC_RG_CM_VDEC_HPRD6,
	PMIC_RG_CM_VINC_HPRD1,
	PMIC_RG_CM_VINC_HPRD2,
	PMIC_RG_CM_VINC_HPRD3,
	PMIC_RG_CM_VINC_HPRD4,
	PMIC_RG_CM_VINC_HPRD5,
	PMIC_RG_CM_VINC_HPRD6,
	PMIC_RG_CM_LPRD,
	PMIC_RG_CM_CS_VTHL,
	PMIC_RG_CM_CS_VTHH,
	PMIC_RG_PCHR_RSV,
	PMIC_RG_ENVTEM_D,
	PMIC_RG_ENVTEM_EN,
	PMIC_RGS_BATON_HV,
	PMIC_RG_HW_VTH_CTRL,
	PMIC_RG_HW_VTH2,
	PMIC_RG_HW_VTH1,
	PMIC_RG_CM_VDEC_INT_EN,
	PMIC_RG_CM_VINC_INT_EN,
	PMIC_RG_QI_BATON_LT_EN,
	PMIC_RGS_BATON_UNDET,
	PMIC_RG_VCDT_TRIM,
	PMIC_RG_INDICATOR_TRIM,
	PMIC_EOSC_CALI_START,
	PMIC_EOSC_CALI_TD,
	PMIC_EOSC_CALI_TEST,
	PMIC_EOSC_CALI_DCXO_RDY_TD,
	PMIC_FRC_VTCXO0_ON,
	PMIC_EOSC_CALI_RSV,
	PMIC_VRTC_PWM_MODE,
	PMIC_VRTC_PWM_RSV,
	PMIC_VRTC_PWM_L_DUTY,
	PMIC_VRTC_PWM_H_DUTY,
	PMIC_VRTC_CAP_SEL,
	PMU_COMMAND_MAX
} PMU_FLAGS_LIST_ENUM;

typedef struct {
	PMU_FLAGS_LIST_ENUM flagname;
	unsigned short offset;
	unsigned short mask;
	unsigned char shift;
} PMU_FLAG_TABLE_ENTRY;

#endif /* _MT_PMIC_UPMU_HW_MT6353_H_ */
