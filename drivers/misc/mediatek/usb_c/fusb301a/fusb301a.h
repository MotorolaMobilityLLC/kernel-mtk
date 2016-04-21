#ifndef FUSB301A_H
#define	FUSB301A_H

/* Debug Log Level */
#define K_EMERG	(1<<7)
#define K_QMU	(1<<7)
#define K_ALET		(1<<6)
#define K_CRIT		(1<<5)
#define K_ERR		(1<<4)
#define K_WARNIN	(1<<3)
#define K_NOTICE	(1<<2)
#define K_INFO		(1<<1)
#define K_DEBUG	(1<<0)

/* Register Map */
#define FUSB301_REG_DEVICEID            0x01
#define FUSB301_REG_MODES               0x02
#define FUSB301_REG_CONTROL             0x03
#define FUSB301_REG_MANUAL              0x04
#define FUSB301_REG_RESET               0x05
#define FUSB301_REG_MASK                0x10
#define FUSB301_REG_STATUS              0x11
#define FUSB301_REG_TYPE                0x12
#define FUSB301_REG_INT                 0x13
/* Register Vaules */
#define FUSB301_DRP_ACC                 BIT(5)
#define FUSB301_DRP                     BIT(4)
#define FUSB301_SNK_ACC                 BIT(3)
#define FUSB301_SNK                     BIT(2)
#define FUSB301_SRC_ACC                 BIT(1)
#define FUSB301_SRC                     BIT(0)
#define FUSB301_TGL_35MS                0
#define FUSB301_TGL_30MS                1
#define FUSB301_TGL_25MS                2
#define FUSB301_TGL_20MS                3
#define FUSB301_HOST_0MA                0
#define FUSB301_HOST_DEFAULT            1
#define FUSB301_HOST_1500MA             2
#define FUSB301_HOST_3000MA             3
#define FUSB301_INT_ENABLE              0x00
#define FUSB301_INT_DISABLE             0x01
#define FUSB301_UNATT_SNK               BIT(3)
#define FUSB301_UNATT_SRC               BIT(2)
#define FUSB301_DISABLED                BIT(1)
#define FUSB301_ERR_REC                 BIT(0)
#define FUSB301_DISABLED_CLEAR          0x00
#define FUSB301_SW_RESET                BIT(0)
#define FUSB301_M_ACC_CH                BIT(3)
#define FUSB301_M_BCLVL                 BIT(2)
#define FUSB301_M_DETACH                BIT(1)
#define FUSB301_M_ATTACH                BIT(0)
#define FUSB301_FAULT_CC                0x30
#define FUSB301_CC2                     0x20
#define FUSB301_CC1                     0x10
#define FUSB301_NO_CONN                 0x00
#define FUSB301_VBUS_OK                 0x08
#define FUSB301_SNK_0MA                 0x00
#define FUSB301_SNK_DEFAULT             0x02
#define FUSB301_SNK_1500MA              0x04
#define FUSB301_SNK_3000MA              0x06
#define FUSB301_ATTACH                  0x01
#define FUSB301_TYPE_SNK                BIT(4)
#define FUSB301_TYPE_SRC                BIT(3)
#define FUSB301_TYPE_PWR_ACC            BIT(2)
#define FUSB301_TYPE_DBG_ACC            BIT(1)
#define FUSB301_TYPE_AUD_ACC            BIT(0)
#define FUSB301_TYPE_PWR_DBG_ACC       (FUSB301_TYPE_PWR_ACC|\
                                        FUSB301_TYPE_DBG_ACC)
#define FUSB301_TYPE_PWR_AUD_ACC       (FUSB301_TYPE_PWR_ACC|\
                                        FUSB301_TYPE_AUD_ACC)
#define FUSB301_TYPE_INVALID            0x00
#define FUSB301_INT_ACC_CH              BIT(3)
#define FUSB301_INT_BCLVL               BIT(2)
#define FUSB301_INT_DETACH              BIT(1)
#define FUSB301_INT_ATTACH              BIT(0)
#define FUSB301_REV10                   0x10
#define FUSB301_REV11                   0x11
#define FUSB301_REV12                   0x12
/* Mask */
#define FUSB301_TGL_MASK                0x30
#define FUSB301_HOST_CUR_MASK           0x06
#define FUSB301_INT_MASK                0x01
#define FUSB301_BCLVL_MASK              0x06
#define FUSB301_TYPE_MASK               0x1F
#define FUSB301_MODE_MASK               0x3F
#define FUSB301_INT_STS_MASK            0x0F
#define FUSB301_MAX_TRY_COUNT           10
/* FUSB STATES */
#define FUSB_STATE_DISABLED             0x00
#define FUSB_STATE_ERROR_RECOVERY       0x01
#define FUSB_STATE_UNATTACHED_SNK       0x02
#define FUSB_STATE_UNATTACHED_SRC       0x03
#define FUSB_STATE_ATTACHWAIT_SNK       0x04
#define FUSB_STATE_ATTACHWAIT_SRC       0x05
#define FUSB_STATE_ATTACHED_SNK         0x06
#define FUSB_STATE_ATTACHED_SRC         0x07
#define FUSB_STATE_AUDIO_ACCESSORY      0x08
#define FUSB_STATE_DEBUG_ACCESSORY      0x09
#define FUSB_STATE_TRY_SNK              0x0A
#define FUSB_STATE_TRYWAIT_SRC          0x0B
#define FUSB_STATE_TRY_SRC              0x0C
#define FUSB_STATE_TRYWAIT_SNK          0x0D
/* wake lock timeout in ms */
#define FUSB301_WAKE_LOCK_TIMEOUT       1000
#define ROLE_SWITCH_TIMEOUT		1500

#endif	/* FUSB301A_H */
