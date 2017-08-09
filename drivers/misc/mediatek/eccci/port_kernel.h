#ifndef __PORT_KERNEL_H__
#define __PORT_KERNEL_H__

#include <ccci_core.h>

#define CCCI_AED_DUMP_EX_MEM		(1<<0)
#define CCCI_AED_DUMP_MD_IMG_MEM	(1<<1)
#define CCCI_AED_DUMP_CCIF_REG		(1<<2)
#define CCCI_AED_DUMP_EX_PKT		(1<<3)

#ifdef MD_UMOLY_EE_SUPPORT
#define EE_BUF_LEN_UMOLY		(0x700)
#define AED_STR_LEN		(2048)
#define EE_BUF_LEN		(256)
#else
#define EE_BUF_LEN		(256)
#define AED_STR_LEN		(512)
#endif

#define CCCI_EXREC_OFFSET_OFFENDER 288

enum {
	MD_EX_TYPE_INVALID = 0,
	MD_EX_TYPE_UNDEF = 1,
	MD_EX_TYPE_SWI = 2,
	MD_EX_TYPE_PREF_ABT = 3,
	MD_EX_TYPE_DATA_ABT = 4,
	MD_EX_TYPE_ASSERT = 5,
	MD_EX_TYPE_FATALERR_TASK = 6,
	MD_EX_TYPE_FATALERR_BUF = 7,
	MD_EX_TYPE_LOCKUP = 8,
	MD_EX_TYPE_ASSERT_DUMP = 9,
	MD_EX_TYPE_ASSERT_FAIL = 10,
	DSP_EX_TYPE_ASSERT = 11,
	DSP_EX_TYPE_EXCEPTION = 12,
	DSP_EX_FATAL_ERROR = 13,

	/*cross core trigger exception, only md3 will trigger this exception*/
	CC_MD1_EXCEPTION = 15,

	NUM_EXCEPTION,
	MD_EX_TYPE_C2K_ERROR = 0x25,
	MD_EX_TYPE_EMI_CHECK = 99,
	MD_EX_C2K_FATAL_ERROR = 0x3000,
};

#ifdef MD_UMOLY_EE_SUPPORT
enum {
	MD_EX_DUMP_INVALID = 0,
	MD_EX_DUMP_ASSERT = 1,
	MD_EX_DUMP_3P_EX = 2,
	MD_EX_DUMP_2P_EX = 3,

	MD_EX_DUMP_EMI_CHECK = MD_EX_TYPE_EMI_CHECK,

	/*MD_EX_C2K_FATAL_ERROR = 0x3000,*/
	MD_EX_DUMP_UNKNOWN,
};
#endif

enum {
	MD_EE_FLOW_START = 0,
	MD_EE_DUMP_ON_GOING,
	MD_STATE_UPDATE,
	MD_EE_MSG_GET,
	MD_EE_TIME_OUT_SET,
	MD_EE_OK_MSG_GET,
	MD_EE_FOUND_BY_ISR,	/* not using */
	MD_EE_FOUND_BY_TX,	/* not using */
	MD_EE_PENDING_TOO_LONG,
	MD_EE_SWINT_GET,
	MD_EE_WDT_GET,

	MD_EE_INFO_OFFSET = 20,	/* not using */
	MD_EE_EXCP_OCCUR = 20,	/* not using */
	MD_EE_AP_MASK_I_BIT_TOO_LONG = 21,	/* not using */
	MD_EE_TIMER1_DUMP_ON_GOING,
	MD_EE_TIMER2_DUMP_ON_GOING,
};

enum {
	MD_EE_CASE_NORMAL = 0,
	MD_EE_CASE_ONLY_EX,
	MD_EE_CASE_ONLY_EX_OK,
	MD_EE_CASE_TX_TRG,	/* not using */
	MD_EE_CASE_ISR_TRG,	/* not using */
	MD_EE_CASE_NO_RESPONSE,
	MD_EE_CASE_AP_MASK_I_BIT_TOO_LONG,	/* not using */
	MD_EE_CASE_ONLY_SWINT,
	MD_EE_CASE_SWINT_MISSING,
	MD_EE_CASE_WDT,
};

typedef enum {
	IPC_RPC_CPSVC_SECURE_ALGO_OP = 0x2001,
	IPC_RPC_GET_SECRO_OP = 0x2002,
#ifdef CONFIG_MTK_TC1_FEATURE
	/* LGE specific OP ID */
	RPC_CCCI_LGE_FAC_READ_SIM_LOCK_TYPE = 0x3001,
	RPC_CCCI_LGE_FAC_READ_FUSG_FLAG,
	RPC_CCCI_LGE_FAC_CHECK_UNLOCK_CODE_VALIDNESS,
	RPC_CCCI_LGE_FAC_CHECK_NETWORK_CODE_VALIDNESS,
	RPC_CCCI_LGE_FAC_WRITE_SIM_LOCK_TYPE,
	RPC_CCCI_LGE_FAC_READ_IMEI,
	RPC_CCCI_LGE_FAC_WRITE_IMEI,
	RPC_CCCI_LGE_FAC_READ_NETWORK_CODE_LIST_NUM,
	RPC_CCCI_LGE_FAC_READ_NETWORK_CODE,
	/* ............. */
	RPC_CCCI_LGE_FAC_WRITE_NETWORK_CODE_LIST_NUM,
	RPC_CCCI_LGE_FAC_WRITE_UNLOCK_CODE_VERIFY_FAIL_COUNT,
	RPC_CCCI_LGE_FAC_READ_UNLOCK_CODE_VERIFY_FAIL_COUNT,
	RPC_CCCI_LGE_FAC_WRITE_UNLOCK_FAIL_COUNT,
	RPC_CCCI_LGE_FAC_READ_UNLOCK_FAIL_COUNT,
	RPC_CCCI_LGE_FAC_WRITE_UNLOCK_CODE,
	RPC_CCCI_LGE_FAC_VERIFY_UNLOCK_CODE,
	RPC_CCCI_LGE_FAC_WRITE_NETWORK_CODE,
	RPC_CCCI_LGE_FAC_INIT_SIM_LOCK_DATA,
#endif
	IPC_RPC_GET_TDD_EINT_NUM_OP = 0x4001,
	IPC_RPC_GET_GPIO_NUM_OP = 0x4002,
	IPC_RPC_GET_ADC_NUM_OP = 0x4003,
	IPC_RPC_GET_EMI_CLK_TYPE_OP = 0x4004,
	IPC_RPC_GET_EINT_ATTR_OP = 0x4005,
	IPC_RPC_GET_GPIO_VAL_OP = 0x4006,
	IPC_RPC_GET_ADC_VAL_OP = 0x4007,
	IPC_RPC_GET_RF_CLK_BUF_OP = 0x4008,
	IPC_RPC_GET_GPIO_ADC_OP = 0x4009,
	IPC_RPC_USIM2NFC_OP = 0x400A,
	IPC_RPC_DSP_EMI_MPU_SETTING = 0x400B,

	IPC_RPC_IT_OP = 0x4321,
} RPC_OP_ID;

struct rpc_pkt {
	unsigned int len;
	void *buf;
} __packed;

struct rpc_buffer {
	struct ccci_header header;
	u32 op_id;
	u32 para_num;
	u8 buffer[0];
} __packed;

/* hardcode, becarefull with data size, should not exceed tmp_data[] in ccci_rpc_work_helper() */
#define CLKBUF_MAX_COUNT 4
struct ccci_rpc_clkbuf_result {
	u16 CLKBuf_Count;
	u8 CLKBuf_Status[CLKBUF_MAX_COUNT];
	u8 CLKBuf_SWCtrl_Status[CLKBUF_MAX_COUNT];
} __packed;	/* the total size should sync with tmp_data[] using in ccci_rpc_work_helper() */

#ifdef CONFIG_MTK_TC1_FEATURE
/* hardcode, becarefull with data size, should not exceed tmp_data[] in ccci_rpc_work_helper() */
#define GPIO_MAX_COUNT 6
#else
/* hardcode, becarefull with data size, should not exceed tmp_data[] in ccci_rpc_work_helper() */
#define GPIO_MAX_COUNT 3
#endif
#define GPIO_MAX_COUNT_V2 10
#define GPIO_PIN_NAME_STR_MAX_LEN 34
#define ADC_CH_NAME_STR_MAX_LEN 33

enum {
	RPC_REQ_GPIO_PIN = (1 << 0),
	RPC_REQ_GPIO_VALUE = (1 << 1),
	RPC_REQ_ADC_PIN = (1 << 4),
	RPC_REQ_ADC_VALUE = (1 << 5),
};
struct ccci_rpc_gpio_adc_intput {
	u8 reqMask;
	u8 gpioValidPinMask;
	char gpioPinName[GPIO_MAX_COUNT][GPIO_PIN_NAME_STR_MAX_LEN];
	u32 gpioPinNum[GPIO_MAX_COUNT];
	char adcChName[ADC_CH_NAME_STR_MAX_LEN];
	u32 adcChNum;
	u32 adcChMeasCount;
} __packed;

struct ccci_rpc_gpio_adc_output {
	u32 gpioPinNum[GPIO_MAX_COUNT];
	u32 gpioPinValue[GPIO_MAX_COUNT];
	u32 adcChNum;
	u32 adcChMeasSum;
} __packed;	/* the total size should sync with tmp_data[] using in ccci_rpc_work_helper() */

struct ccci_rpc_gpio_adc_intput_v2 { /* 10 pin GPIO support */
	u16 reqMask;
	u16 gpioValidPinMask;
	char gpioPinName[GPIO_MAX_COUNT_V2][GPIO_PIN_NAME_STR_MAX_LEN];
	u32 gpioPinNum[GPIO_MAX_COUNT_V2];
	char adcChName[ADC_CH_NAME_STR_MAX_LEN];
	u32 adcChNum;
	u32 adcChMeasCount;
} __packed;

struct ccci_rpc_gpio_adc_output_v2 { /* 10 pin GPIO support */
	u32 gpioPinNum[GPIO_MAX_COUNT_V2];
	u32 gpioPinValue[GPIO_MAX_COUNT_V2];
	u32 adcChNum;
	u32 adcChMeasSum;
} __packed;	/* the total size should sync with tmp_data[] using in ccci_rpc_work_helper() */

struct ccci_rpc_dsp_emi_mpu_input {
	u32 request;
} __packed;

struct ccci_rpc_usim2nfs {
	u8 lock_vsim1;
} __packed;

#define RPC_REQ_BUFFER_NUM       2	/* support 2 concurrently request */
#define RPC_MAX_ARG_NUM          6	/* parameter number */
#define RPC_MAX_BUF_SIZE         2048
#define RPC_API_RESP_ID          0xFFFF0000

#define FS_NO_ERROR										 0
#define FS_NO_OP										-1
#define	FS_PARAM_ERROR									-2
#define FS_NO_FEATURE									-3
#define FS_NO_MATCH									    -4
#define FS_FUNC_FAIL								    -5
#define FS_ERROR_RESERVED								-6
#define FS_MEM_OVERFLOW									-7

#define CCCI_SED_LEN_BYTES   16
typedef struct {
	unsigned char sed[CCCI_SED_LEN_BYTES];
} sed_t;
#define SED_INITIALIZER { {[0 ... CCCI_SED_LEN_BYTES-1] = 0} }
#ifdef FEATURE_GET_MD_EINT_ATTR_DTS
#define MD_SIM_MAX (16)		/*(MD number * SIM number EACH MD) */

typedef enum {
	SIM_HOT_PLUG_EINT_NUMBER,
	SIM_HOT_PLUG_EINT_DEBOUNCETIME,
	SIM_HOT_PLUG_EINT_POLARITY,
	SIM_HOT_PLUG_EINT_SENSITIVITY,
	SIM_HOT_PLUG_EINT_SOCKETTYPE,
	SIM_HOT_PLUG_EINT_DEDICATEDEN,
	SIM_HOT_PLUG_EINT_SRCPIN,

	SIM_HOT_PLUG_EINT_MAX,
} sim_hot_plug_eint_queryType;

typedef enum {
	ERR_SIM_HOT_PLUG_NULL_POINTER = -13,
	ERR_SIM_HOT_PLUG_QUERY_TYPE,
	ERR_SIM_HOT_PLUG_QUERY_STRING,
} sim_hot_plug_eint_queryErr;

struct eint_struct {
	int type;		/* sync with MD: value type of MD want to get */
	char *property;		/* property name in the node of dtsi */
	int index;		/* cell index in property */
	int value_sim[MD_SIM_MAX];	/* value of each node of current type from property */
};
struct eint_node_name {
	char *node_name;	/*node name in dtsi */
	int md_id;		/* md_id in node_name, no use currently */
	int sim_id;		/* sim_id in node_name, no use currently */
};
struct eint_node_struct {
	unsigned int ExistFlag;	/* if node exist */
	struct eint_node_name *name;
	struct eint_struct *eint_value;
};

#endif

int port_kernel_init(struct ccci_port *port);
int process_rpc_kernel_msg(struct ccci_port *port, struct ccci_request *req);

#if defined(FEATURE_GET_MD_ADC_NUM)
extern int IMM_get_adc_channel_num(char *channel_name, int len);
#endif
#if defined(FEATURE_GET_MD_ADC_VAL)
extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int *rawdata);
#endif

#if defined(FEATURE_GET_MD_PMIC_ADC_VAL)
extern int PMIC_IMM_GetOneChannelValue(int dwChannel, int deCount, int trimd);
#endif

#if defined(FEATURE_GET_MD_PMIC_ADC_NUM)
extern int PMIC_IMM_get_adc_channel_num(char *adc_name, int len);
#endif
#ifdef FEATURE_GET_MD_GPIO_VAL
extern int mt_get_md_gpio(char *gpio_name, int len);
#endif
#if defined(FEATURE_GET_DRAM_TYPE_CLK)
extern int get_dram_info(int *clk, int *type);
#endif
#ifndef FEATURE_GET_MD_EINT_ATTR_DTS
#if defined(FEATURE_GET_MD_EINT_ATTR)
extern int get_eint_attribute(char *name, unsigned int name_len, unsigned int type, char *result, unsigned int *len);
#endif
#endif
extern void ccci_set_dsp_region_protection(struct ccci_modem *md, int loaded);
#endif				/* __PORT_KERNEL_H__ */
