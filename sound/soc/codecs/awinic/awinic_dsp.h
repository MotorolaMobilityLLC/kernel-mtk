#ifndef __AWINIC_DSP_H__
#define __AWINIC_DSP_H__



#define AW_MTK_PLATFORM
/*#define AW_QCOM_PLATFORM*/

#define AWINIC_DSP_MSG_HDR_VER (1)
#define AW_DSP_TRY_TIME (3)
#define AW_DSP_SLEEP_TIME (10)
#define AFE_PORT_ID_AWDSP_RX (0x1006)     /*AFE_PORT_ID_QUATERNARY_MI2S_RX*/
#define AW_COPP_MODULE_ID (0X10013D02)			/*SKT module id*/
#define AW_COPP_MODULE_PARAMS_ID_EN (0X10013D14)	/*SKT enable param id*/

#define INLINE_PARAM_ID_NULL				(0x00000000)
#define INLINE_PARAM_ID_ENABLE_CALI			(0x00000001)
#define INLINE_PARAM_ID_ENABLE_HMUTE			(0x00000002)
#define INLINE_PARAM_ID_F0_Q				(0x00000003)
#define INLINE_PARAM_ID_ACTIVE_FLAG			(0x00000004)
#define INLINE_PARAM_ID_REAL_DATA			(0x00000005)
#define INLINE_PARAM_ID_DIRECT_CURRENT_FLAG		(0x00000006)
#define INLINE_PARAMS_ID_SPK_STATUS			(0x00000007)
#define INLINE_PARAMS_ID_VERSION			(0x00000008)


/*dsp params id*/
#define AFE_PARAM_ID_AWDSP_RX_SET_ENABLE		(0x10013D11)
#define AFE_PARAM_ID_AWDSP_RX_PARAMS			(0x10013D12)
#define AFE_PARAM_ID_AWDSP_TX_SET_ENABLE		(0x10013D13)
#define AFE_PARAM_ID_AWDSP_RX_VMAX_L			(0X10013D17)
#define AFE_PARAM_ID_AWDSP_RX_VMAX_R			(0X10013D18)
#define AFE_PARAM_ID_AWDSP_RX_CALI_CFG_L		(0X10013D19)
#define AFE_PARAM_ID_AWDSP_RX_CALI_CFG_R		(0x10013d1A)
#define AFE_PARAM_ID_AWDSP_RX_RE_L			(0x10013d1B)
#define AFE_PARAM_ID_AWDSP_RX_RE_R			(0X10013D1C)
#define AFE_PARAM_ID_AWDSP_RX_NOISE_L			(0X10013D1D)
#define AFE_PARAM_ID_AWDSP_RX_NOISE_R			(0X10013D1E)
#define AFE_PARAM_ID_AWDSP_RX_F0_L			(0X10013D1F)
#define AFE_PARAM_ID_AWDSP_RX_F0_R			(0X10013D20)
#define AFE_PARAM_ID_AWDSP_RX_REAL_DATA_L		(0X10013D21)
#define AFE_PARAM_ID_AWDSP_RX_REAL_DATA_R		(0X10013D22)
#define AFE_PARAM_ID_AWDSP_RX_MSG			(0X10013D2A)

enum aw882xx_dsp_msg_type {
	DSP_MSG_TYPE_DATA = 0,
	DSP_MSG_TYPE_CMD = 1,
};

enum aw882xx_dsp_hmute_st {
	DSP_UNHMUTE = 0,
	DSP_HMUTE = 1,
};

enum aw882xx_dsp_cali_st {
	DSP_BACK_CALI_CFG = 0,
	DSP_SET_CALI_CFG = 1,
};

enum aef_module_type {
	AW_RX_MODULE = 0,
	AW_TX_MODULE = 1,
};

enum afe_param_id_adsp {
	INDEX_PARAMS_ID_RX_PARAMS = 0,
	INDEX_PARAMS_ID_RX_ENBALE,
	INDEX_PARAMS_ID_TX_ENABLE,
	INDEX_PARAMS_ID_RX_VMAX,
	INDEX_PARAMS_ID_RX_CALI_CFG,
	INDEX_PARAMS_ID_RX_RE,
	INDEX_PARAMS_ID_RX_NOISE,
	INDEX_PARAMS_ID_RX_F0,
	INDEX_PARAMS_ID_RX_REAL_DATA,
	INDEX_PARAMS_ID_AWDSP_RX_MSG,
	INDEX_PARAMS_ID_MAX
};

struct aw_dsp_msg_hdr {
	int32_t type;
	int32_t opcode_id;
	int32_t version;
	int32_t reserver[3];
};

struct f0_q_data {
	int32_t left_f0;
	int32_t left_q;
	int32_t right_f0;
	int32_t right_q;
};


int aw_write_data_to_dsp(int index, void *data, int data_size, int channel);
int aw_read_data_from_dsp(int index, void *data, int data_size, int channel);
int aw_write_msg_to_dsp(int inline_id, void *data, int data_size, int channel);
int aw_read_msg_from_dsp(int inline_id, void *data, int data_size, int channel);
int aw_send_module_enable(void *buf, uint8_t type);
int aw_get_module_enable(void *buf, uint8_t type);
int aw_get_f0_q(struct f0_q_data *data, int data_size, int channel);
int aw_get_algo_version(unsigned int *data);
int aw_dsp_copp_module_en(bool enable);

#endif

