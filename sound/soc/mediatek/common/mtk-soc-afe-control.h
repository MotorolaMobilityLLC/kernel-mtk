/*
* Copyright (C) 2015 MediaTek Inc.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/


/*******************************************************************************
 *
 * Filename:
 * ---------
 *  mt_sco_afe_control.h
 *
 * Project:
 * --------
 *   MT6583  Audio Driver Kernel Function
 *
 * Description:
 * ------------
 *   Audio register
 *
 * Author:
 * -------
 * Chipeng Chang
 *
 *------------------------------------------------------------------------------
 *
 *
 *******************************************************************************/


/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/

#ifndef _AUDIO_AFE_CONTROL_H
#define _AUDIO_AFE_CONTROL_H

#include "mtk-auddrv-common.h"
#include "mtk-auddrv-def.h"
#include "mtk-auddrv-afe.h"
#include "mtk-auddrv-clk.h"

#include <sound/memalloc.h>
#include <sound/pcm.h>

#define GIC_PRIVATE_SIGNALS          (32)
#define AUDDRV_DL1_MAX_BUFFER_LENGTH (0x6000)
#define MT6735_AFE_MCU_IRQ_LINE (GIC_PRIVATE_SIGNALS + 144)
#define MASK_ALL          (0xFFFFFFFF)
#define AFE_MASK_ALL  (0xffffffff)

bool InitAfeControl(struct device *pdev);
bool ResetAfeControl(void);
bool Register_Aud_Irq(void *dev, uint32 afe_irq_number);

bool SetConnectionFormat(uint32 ConnectionFormat, uint32 Aud_block);
bool SetConnection(uint32 ConnectionState, uint32 Input, uint32 Output);
bool SetIntfConnection(uint32 ConnectionState, uint32 Aud_block_In, uint32 Aud_block_Out);
bool SetMemoryPathEnable(uint32 Aud_block, bool bEnable);
bool GetMemoryPathEnable(uint32 Aud_block);
bool SetI2SDacEnable(bool bEnable);
bool GetI2SDacEnable(void);
void EnableAfe(bool bEnable);
bool set_chip_afe_enable(bool enable);

bool Set2ndI2SOutAttribute(uint32_t sampleRate);
bool Set2ndI2SOut(AudioDigtalI2S *DigtalI2S);
bool Set2ndI2SOutEnable(bool benable);
bool set_adc_in(unsigned int rate);
bool set_adc2_in(unsigned int rate);
bool setDmicPath(bool _enable);

void set_ul_src_enable(bool enable);
void set_ul2_src_enable(bool enable);
void SetDLSrcEnable(bool bEnable);
void SetADDAEnable(bool bEnable);
bool set_chip_adda_enable(bool bEnable);
bool set_chip_ul_src_enable(bool enable);
bool set_chip_ul2_src_enable(bool enable);
bool set_chip_dl_src_enable(bool enable);

bool SetExtI2SAdcIn(AudioDigtalI2S *DigtalI2S);
bool SetExtI2SAdcInEnable(bool bEnable);

bool Set2ndI2SAdcIn(AudioDigtalI2S *DigtalI2S);

int setConnsysI2SIn(AudioDigtalI2S *DigtalI2S);
int setConnsysI2SInEnable(bool enable);
int setConnsysI2SAsrc(bool bIsUseASRC, unsigned int dToSampleRate);
int setConnsysI2SEnable(bool enable);

bool GetMrgI2SEnable(void);
bool SetMrgI2SEnable(bool bEnable, unsigned int sampleRate);
bool SetDaiBt(AudioDigitalDAIBT *mAudioDaiBt);
bool SetDaiBtEnable(bool bEanble);
bool set_chip_dai_bt_enable(bool enable, AudioDigitalDAIBT *dai_bt, AudioMrgIf *mrg);

bool set_adc_enable(bool enable);
bool set_adc2_enable(bool enable);
bool Set2ndI2SAdcEnable(bool bEnable);
bool SetI2SDacOut(uint32 SampleRate, bool Lowgitter, bool I2SWLen);
bool Set2ndI2SEnable(bool bEnable);
bool set_i2s_dac_out_source(uint32 aud_block);

bool SetHwDigitalGainMode(uint32 GainType, uint32 SampleRate, uint32 SamplePerStep);
bool SetHwDigitalGainEnable(int GainType, bool Enable);
bool SetHwDigitalGain(uint32 Gain, int GainType);
bool set_chip_hw_digital_gain_mode(uint32 gain_type, uint32 sample_rate, uint32 sample_per_step);
bool set_chip_hw_digital_gain_enable(int gain_type, bool enable);
bool set_chip_hw_digital_gain(uint32 gain, int gain_type);

bool EnableSineGen(uint32 connection, bool direction, bool Enable);
bool SetSineGenSampleRate(uint32 SampleRate);
bool SetSineGenAmplitude(uint32 ampDivide);
bool set_chip_sine_gen_sample_rate(uint32 sample_rate);
bool set_chip_sine_gen_amplitude(uint32 amp_divide);

bool SetModemPcmEnable(int modem_index, bool modem_pcm_on);
bool SetModemPcmConfig(int modem_index, AudioDigitalPCM p_modem_pcm_attribute);

bool Set2ndI2SIn(AudioDigtalI2S *mDigitalI2S);
bool Set2ndI2SInConfig(unsigned int sampleRate, bool bIsSlaveMode);
bool Set2ndI2SInEnable(bool bEnable);

bool checkDllinkMEMIfStatus(void);
bool checkUplinkMEMIfStatus(void);
bool SetMemIfFetchFormatPerSample(uint32 InterfaceType, uint32 eFetchFormat);
bool SetMemIfFormatReg(uint32 InterfaceType, uint32 eFetchFormat);
bool SetoutputConnectionFormat(uint32 ConnectionFormat, uint32 Output);

int set_memif_pbuf_size(int aud_blk, enum memif_pbuf_size pbuf_size);

bool set_chip_adc_in(unsigned int rate);
bool set_chip_adc2_in(unsigned int rate);
bool setChipDmicPath(bool _enable, uint32 sample_rate);

/* Sample Rate Transform */
uint32 SampleRateTransform(uint32 sampleRate, Soc_Aud_Digital_Block audBlock);

void EnableAPLLTunerbySampleRate(uint32 SampleRate);
void DisableAPLLTunerbySampleRate(uint32 SampleRate);

int AudDrv_Allocate_mem_Buffer(struct device *pDev, Soc_Aud_Digital_Block MemBlock,
			       uint32 Buffer_length);
AFE_MEM_CONTROL_T *Get_Mem_ControlT(Soc_Aud_Digital_Block MemBlock);
bool SetMemifSubStream(Soc_Aud_Digital_Block MemBlock, struct snd_pcm_substream *substream);
bool RemoveMemifSubStream(Soc_Aud_Digital_Block MemBlock, struct snd_pcm_substream *substream);
bool ClearMemBlock(Soc_Aud_Digital_Block MemBlock);

/* interrupt handler */
void Auddrv_Dl1_Spinlock_lock(void);
void Auddrv_Dl1_Spinlock_unlock(void);
void Auddrv_Dl2_Spinlock_lock(void);
void Auddrv_Dl2_Spinlock_unlock(void);
void Auddrv_Dl3_Spinlock_lock(void);
void Auddrv_Dl3_Spinlock_unlock(void);


int AudDrv_DSP_IRQ_handler(void *PrivateData);
void Auddrv_DSP_DL1_Interrupt_Handler(void *PrivateData);
void Auddrv_DL1_Interrupt_Handler(void);
void Auddrv_DL2_Interrupt_Handler(void);
void Auddrv_UL1_Interrupt_Handler(void);
void Auddrv_AWB_Interrupt_Handler(void);
void Auddrv_DAI_Interrupt_Handler(void);
void Auddrv_HDMI_Interrupt_Handler(void);
void Auddrv_UL2_Interrupt_Handler(void);
void Auddrv_MOD_DAI_Interrupt_Handler(void);
kal_uint32 Get_Mem_CopySizeByStream(Soc_Aud_Digital_Block MemBlock,
				    struct snd_pcm_substream *substream);
void Set_Mem_CopySizeByStream(Soc_Aud_Digital_Block MemBlock, struct snd_pcm_substream *substream,
			      uint32 size);

struct snd_dma_buffer *Get_Mem_Buffer(Soc_Aud_Digital_Block MemBlock);
int AudDrv_Allocate_DL1_Buffer(struct device *pDev, kal_uint32 Afe_Buf_Length,
	dma_addr_t dma_addr, unsigned char *dma_area);


bool Restore_Audio_Register(void);

void AfeControlMutexLock(void);
void AfeControlMutexUnLock(void);

/* Sram management  function */
void AfeControlSramLock(void);
void AfeControlSramUnLock(void);
size_t GetCaptureSramSize(void);
unsigned int GetSramState(void);
void ClearSramState(unsigned int State);
void SetSramState(unsigned int State);
unsigned int GetPLaybackSramFullSize(void);
unsigned int GetPLaybackSramPartial(void);
unsigned int GetPLaybackDramSize(void);
size_t GetCaptureDramSize(void);

/* offsetTrimming */
void OpenAfeDigitaldl1(bool bEnable);
void SetExternalModemStatus(const bool bEnable);

/* set VOW status for AFE GPIO control */
void SetVOWStatus(bool bEnable);
bool ConditionEnterSuspend(void);
void SetFMEnableFlag(bool bEnable);
void SetOffloadEnableFlag(bool bEnable);
bool GetOffloadEnableFlag(void);

unsigned int word_size_align(unsigned int in_size);

void AudDrv_checkDLISRStatus(void);

/* sram mamager */
bool InitSramManager(struct device *pDev, unsigned int sramblocksize);
bool CheckSramAvail(unsigned int mSramLength, unsigned int *mSramBlockidx, unsigned int *mSramBlocknum);
int AllocateAudioSram(dma_addr_t *sram_phys_addr, unsigned char **msram_virt_addr,
	unsigned int mSramLength, void *user);
int freeAudioSram(void *user);

/* IRQ Manager */
int init_irq_manager(void);
int irq_add_user(const void *_user,
		 enum Soc_Aud_IRQ_MCU_MODE _irq,
		 unsigned int _rate,
		 unsigned int _count);
int irq_remove_user(const void *_user,
		    enum Soc_Aud_IRQ_MCU_MODE _irq);
int irq_update_user(const void *_user,
		    enum Soc_Aud_IRQ_MCU_MODE _irq,
		    unsigned int _rate,
		    unsigned int _count);
int irq_get_total_user(enum Soc_Aud_IRQ_MCU_MODE _irq);

/* IRQ Register Control Table and Handler Function Table*/
void RunIRQHandler(enum Soc_Aud_IRQ_MCU_MODE irqIndex);
const struct Aud_IRQ_CTRL_REG *GetIRQCtrlReg(enum Soc_Aud_IRQ_MCU_MODE irqIndex);
const struct Aud_RegBitsInfo *GetIRQPurposeReg(enum Soc_Aud_IRQ_PURPOSE sIrqPurpose);

bool SetHighAddr(Soc_Aud_Digital_Block MemBlock, bool usingdram, dma_addr_t addr);

int memif_lpbk_enable(struct memif_lpbk *memif_lpbk);
int memif_lpbk_disable(struct memif_lpbk *memif_lbpk);

/* GetEnableAudioBlockRegOffset */
enum MEM_BLOCK_ENABLE_REG_INDEX {
	MEM_BLOCK_ENABLE_REG_INDEX_AUDIO_BLOCK = 0,
	MEM_BLOCK_ENABLE_REG_INDEX_REG,
	MEM_BLOCK_ENABLE_REG_INDEX_OFFSET,
	MEM_BLOCK_ENABLE_REG_INDEX_NUM
};
uint32 GetEnableAudioBlockRegOffset(uint32 Aud_block);
uint32 GetEnableAudioBlockRegAddr(uint32 Aud_block);

/* FM AP Dependent */
bool SetFmI2sConnection(uint32 ConnectionState);
bool SetFmAwbConnection(uint32 ConnectionState);
int SetFmI2sInEnable(bool enable);
int SetFmI2sIn(AudioDigtalI2S *mDigitalI2S);
bool GetFmI2sInPathEnable(void);
bool SetFmI2sInPathEnable(bool bEnable);
int SetFmI2sAsrcEnable(bool bEnable);
int SetFmI2sAsrcConfig(bool bIsUseASRC, unsigned int dToSampleRate);

/* ANC AP Dependent */
bool SetAncRecordReg(uint32 value, uint32 mask);

/*Auxadc Interface*/
int audio_get_auxadc_value(void);

/* irq from other module */
bool is_irq_from_ext_module(void);
int start_ext_sync_signal(void);
int stop_ext_sync_signal(void);
int ext_sync_signal(void);
void ext_sync_signal_lock(void);
void ext_sync_signal_unlock(void);

/* api for other modules to allocate audio sram */
int mtk_audio_request_sram(dma_addr_t *phys_addr,
			   unsigned char **virt_addr,
			   unsigned int length,
			   void *user);
void mtk_audio_free_sram(void *user);

struct mtk_mem_blk_ops {
	int (*set_chip_memif_addr)(int mem_blk, dma_addr_t addr, size_t size);
};

struct mtk_afe_platform_ops {
	bool (*set_sinegen)(uint32 connection, bool direction, bool Enable);
	void (*init_platform)(void);
	bool (*set_smartpa_i2s)(int sidegen_control, int hdoutput_control, int extcodec_echoref_control,
				int mtk_soc_always_hd);
	bool (*set_smartpa_echo_ref)(int sample_rate, int extcodec_echoref_control, int enable);
	bool (*set_dpd_module)(bool enable, int impedance);
};


void set_mem_blk_ops(struct mtk_mem_blk_ops *ops);

/* sould return current frame index  with memblock*/
snd_pcm_uframes_t get_mem_frame_index(struct snd_pcm_substream *substream,
	AFE_MEM_CONTROL_T *afe_mem_control, Soc_Aud_Digital_Block mem_block);
void mem_blk_spinlock(Soc_Aud_Digital_Block mem_blk);
void mem_blk_spinunlock(Soc_Aud_Digital_Block mem_blk);
int mtk_memblk_copy(struct snd_pcm_substream *substream,
				int channel, snd_pcm_uframes_t pos, void __user *dst, snd_pcm_uframes_t count,
				AFE_MEM_CONTROL_T *pMemControl, Soc_Aud_Digital_Block mem_blk);

int set_memif_addr(int mem_blk, dma_addr_t addr, size_t size);
int set_mem_block(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *hw_params,
	AFE_MEM_CONTROL_T *pMemControl, Soc_Aud_Digital_Block mem_blk);
void init_afe_ops(void);
void set_afe_platform_ops(struct mtk_afe_platform_ops *ops);
struct mtk_afe_platform_ops *get_afe_platform_ops(void);

/* for vcore dvfs */
int vcore_dvfs(bool *enable, bool reset);
void set_screen_state(bool state);

/* low latency debug */
int get_LowLatencyDebug(void);
void set_LowLatencyDebug(bool bFlag);

#endif
