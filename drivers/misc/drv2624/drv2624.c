/*
** =============================================================================
** Copyright (c) 2016  Texas Instruments Inc.
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**
** File:
**     drv2624.c
**
** Description:
**     DRV2624 chip driver
**
** =============================================================================
*/
#define DEBUG
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/semaphore.h>
#include <linux/device.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/spinlock_types.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/jiffies.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/miscdevice.h>
#include "drv2624.h"
static struct drv2624_data *g_DRV2624data = NULL;
static bool g_logEnable = false;
static int drv2624_reg_read(struct drv2624_data *pDRV2624,
	unsigned char reg)
{
	unsigned int val;
	int nResult;
	mutex_lock(&pDRV2624->dev_lock);
	nResult = regmap_read(pDRV2624->mpRegmap, reg, &val);
    mutex_unlock(&pDRV2624->dev_lock);
	if (nResult < 0) {
		dev_err(pDRV2624->dev, "%s I2C error %d\n", __func__, nResult);
		return nResult;
	} else {
		if (g_logEnable)
			dev_dbg(pDRV2624->dev, "%s, Reg[0x%x]=0x%x\n", __func__, reg, val);
		return val;
	}
}
static int drv2624_reg_write(struct drv2624_data *pDRV2624,
	unsigned char reg, unsigned char val)
{
	int nResult;
	mutex_lock(&pDRV2624->dev_lock);
	nResult = regmap_write(pDRV2624->mpRegmap, reg, val);
	mutex_unlock(&pDRV2624->dev_lock);
	if (nResult < 0)
		dev_err(pDRV2624->dev, "%s reg=0x%x, value=0%x error %d\n",
			__func__, reg, val, nResult);
	else if (g_logEnable)
		dev_dbg(pDRV2624->dev, "%s, Reg[0x%x]=0x%x\n", __func__, reg, val);
	return nResult;
}
static int drv2624_bulk_read(struct drv2624_data *pDRV2624,
	unsigned char reg, unsigned char *buf, unsigned int count)
{
	int nResult;
	mutex_lock(&pDRV2624->dev_lock);
	nResult = regmap_bulk_read(pDRV2624->mpRegmap, reg, buf, count);
	mutex_unlock(&pDRV2624->dev_lock);
	if (nResult < 0)
		dev_err(pDRV2624->dev, "%s reg=0%x, count=%d error %d\n",
			__func__, reg, count, nResult);
	return nResult;
}
static int drv2624_bulk_write(struct drv2624_data *pDRV2624,
	unsigned char reg, const u8 *buf, unsigned int count)
{
	int nResult, i;
	mutex_lock(&pDRV2624->dev_lock);
	nResult = regmap_bulk_write(pDRV2624->mpRegmap, reg, buf, count);
	mutex_unlock(&pDRV2624->dev_lock);
	if (nResult < 0)
		dev_err(pDRV2624->dev, "%s reg=0%x, count=%d error %d\n",
			__func__, reg, count, nResult);
	else if (g_logEnable)
		for(i = 0; i < count; i++)
			dev_dbg(pDRV2624->dev, "%s, Reg[0x%x]=0x%x\n",
				__func__, reg+i, buf[i]);
	return nResult;
}
static int drv2624_set_bits(struct drv2624_data *pDRV2624,
	unsigned char reg, unsigned char mask, unsigned char val)
{
	int nResult;
	mutex_lock(&pDRV2624->dev_lock);
	nResult = regmap_update_bits(pDRV2624->mpRegmap, reg, mask, val);
	mutex_unlock(&pDRV2624->dev_lock);
	if (nResult < 0)
		dev_err(pDRV2624->dev, "%s reg=%x, mask=0x%x, value=0x%x error %d\n",
			__func__, reg, mask, val, nResult);
	else if (g_logEnable)
		dev_dbg(pDRV2624->dev, "%s, Reg[0x%x]:M=0x%x, V=0x%x\n",
			__func__, reg, mask, val);
	return nResult;
}
static int drv2624_enableIRQ(struct drv2624_data *pDRV2624, unsigned char bRTP)
{
	int nResult = 0;
	unsigned char mask = INT_ENABLE_CRITICAL;
	if (!pDRV2624->mbIRQUsed)
		goto end;
	if (pDRV2624->mbIRQEnabled)
		goto end;
	if (bRTP == 0)
		mask = INT_ENABLE_ALL;
	nResult = drv2624_reg_read(pDRV2624,DRV2624_REG_STATUS);
	if (nResult < 0)
		goto end;
	nResult = drv2624_reg_write(pDRV2624, DRV2624_REG_INT_ENABLE, mask);
	if (nResult < 0)
		goto end;
//	enable_irq(pDRV2624->mnIRQ);
	pDRV2624->mbIRQEnabled = true;
end:
	return nResult;
}
static void drv2624_disableIRQ(struct drv2624_data *pDRV2624)
{
	if (pDRV2624->mbIRQUsed) {
		if (pDRV2624->mbIRQEnabled) {
//			disable_irq_nosync(pDRV2624->mnIRQ);
			drv2624_reg_write(pDRV2624, DRV2624_REG_INT_ENABLE, INT_MASK_ALL);
			pDRV2624->mbIRQEnabled = false;
		}
	}
}
static int drv2624_set_go_bit(struct drv2624_data *pDRV2624, unsigned char val)
{
	int nResult = 0, value =0;
	int retry = POLL_GO_BIT_RETRY;
	val &= 0x01;
	nResult = drv2624_reg_write(pDRV2624, DRV2624_REG_GO, val);
	if (nResult < 0)
		goto end;
	mdelay(POLL_GO_BIT_INTERVAL);
	value = drv2624_reg_read(pDRV2624, DRV2624_REG_GO);
	if (value < 0) {
		nResult = value;
		goto end;
	}
	if (val) {
		if (value != GO) {
			nResult = -EIO;
			dev_warn(pDRV2624->dev, "%s, GO fail, stop action\n", __func__);
		}
	} else {
		while (retry > 0) {
			value = drv2624_reg_read(pDRV2624, DRV2624_REG_GO);
			if (value < 0) {
				nResult = value;
				break;
			}
			if(value==0)
				break;
			mdelay(POLL_GO_BIT_INTERVAL);
			retry--;
		}
		if (retry == 0)
			dev_err(pDRV2624->dev,
				"%s, ERROR: clear GO fail\n", __func__);
		else if (g_logEnable)
			dev_dbg(pDRV2624->dev,
				"%s, clear GO, remain=%d\n", __func__, retry);
	}
end:
	return nResult;
}
static int drv2624_change_mode(struct drv2624_data *pDRV2624,
	unsigned char work_mode)
{
	return drv2624_set_bits(pDRV2624,
			DRV2624_REG_MODE, WORKMODE_MASK, work_mode);
}
static int vibrator_get_time(struct timed_output_dev *dev)
{
	struct drv2624_data *pDRV2624 = container_of(dev, struct drv2624_data, to_dev);
	if (hrtimer_active(&pDRV2624->timer)) {
		ktime_t r = hrtimer_get_remaining(&pDRV2624->timer);
		return ktime_to_ms(r);
	}
	return 0;
}
static void drv2624_set_stopflag(struct drv2624_data *pDRV2624)
{
	pDRV2624->mnVibratorPlaying = NO;
	wake_unlock(&pDRV2624->wklock);
	if (g_logEnable)
		dev_dbg(pDRV2624->dev, "wklock unlock");
}
static int drv2624_get_diag_result(struct drv2624_data *pDRV2624, unsigned char nStatus)
{
	int nResult = 0;
	pDRV2624->mDiagResult.mnResult = nStatus;
	if((nStatus&DIAG_MASK) != DIAG_SUCCESS)
		dev_err(pDRV2624->dev, "Diagnostic fail\n");
	else {
		nResult = drv2624_reg_read(pDRV2624, DRV2624_REG_DIAG_Z);
		if (nResult < 0)
			goto end;
		pDRV2624->mDiagResult.mnDiagZ = nResult;
		nResult = drv2624_reg_read(pDRV2624, DRV2624_REG_DIAG_K);
		if (nResult < 0)
			goto end;
		pDRV2624->mDiagResult.mnDiagK = nResult;
		dev_dbg(pDRV2624->dev, "Diag : ZResult=0x%x, CurrentK=0x%x\n",
			pDRV2624->mDiagResult.mnDiagZ,
			pDRV2624->mDiagResult.mnDiagK);
	}
end:
	return nResult;
}
static int drv2624_get_calibration_result(struct drv2624_data *pDRV2624, unsigned char nStatus)
{
	int nResult = 0;
	pDRV2624->mAutoCalResult.mnResult = nStatus;
	if ((nStatus&DIAG_MASK) != DIAG_SUCCESS)
		dev_err(pDRV2624->dev, "Calibration fail\n");
	else {
		nResult = drv2624_reg_read(pDRV2624, DRV2624_REG_CAL_COMP);
		if (nResult < 0)
			goto end;
		pDRV2624->mAutoCalResult.mnCalComp = nResult;
		nResult = drv2624_reg_read(pDRV2624, DRV2624_REG_CAL_BEMF);
		if (nResult < 0)
			goto end;
		pDRV2624->mAutoCalResult.mnCalBemf = nResult;
		nResult = drv2624_reg_read(pDRV2624, DRV2624_REG_LOOP_CONTROL) & BEMFGAIN_MASK;
		if (nResult < 0)
			goto end;
		pDRV2624->mAutoCalResult.mnCalGain = nResult;
		dev_dbg(pDRV2624->dev, "AutoCal : Comp=0x%x, Bemf=0x%x, Gain=0x%x\n",
			pDRV2624->mAutoCalResult.mnCalComp,
			pDRV2624->mAutoCalResult.mnCalBemf,
			pDRV2624->mAutoCalResult.mnCalGain);
	}
end:
	return nResult;
}
static int drv2624_stop(struct drv2624_data *pDRV2624)
{
	int nResult = 0, mode = 0;
	nResult = drv2624_reg_read(pDRV2624, DRV2624_REG_MODE);
	if (nResult < 0)
		return nResult;
	mode =  nResult & WORKMODE_MASK;
	if (mode == MODE_WAVEFORM_SEQUENCER)
	{
		dev_dbg(pDRV2624->dev, "In sequence play, ignore stop\n");
		return 0;
	}
	if (pDRV2624->mnVibratorPlaying == YES) {
		dev_dbg(pDRV2624->dev, "%s\n", __func__);
		if (pDRV2624->mbIRQUsed)
			drv2624_disableIRQ(pDRV2624);
		if (hrtimer_active(&pDRV2624->timer))
			hrtimer_cancel(&pDRV2624->timer);
		nResult = drv2624_set_go_bit(pDRV2624, STOP);
		drv2624_set_stopflag(pDRV2624);
	}
	return nResult;
}
static void vibrator_enable( struct timed_output_dev *dev, int value)
{
	int nResult = 0;
	struct drv2624_data *pDRV2624 =
		container_of(dev, struct drv2624_data, to_dev);
	dev_dbg(pDRV2624->dev, "%s, value=%d\n", __func__, value);
	mutex_lock(&pDRV2624->lock);
	pDRV2624->mnWorkMode = WORK_IDLE;
	if (g_logEnable)
		dev_dbg(pDRV2624->dev, "%s, afer mnWorkMode=0x%x\n",
			__func__, pDRV2624->mnWorkMode);
	drv2624_stop(pDRV2624);
	if (value > 0) {
		wake_lock(&pDRV2624->wklock);
		if (g_logEnable)
			dev_dbg(pDRV2624->dev, "wklock lock");
		nResult = drv2624_change_mode(pDRV2624, MODE_RTP);
		if (nResult < 0)
			goto end;
		nResult = drv2624_set_go_bit(pDRV2624, GO);
		if (nResult >= 0) {
			value = (value > MAX_TIMEOUT) ? MAX_TIMEOUT : value;
			pDRV2624->mnWorkMode |= WORK_VIBRATOR;
			hrtimer_start(&pDRV2624->timer,
				ns_to_ktime((u64)value * NSEC_PER_MSEC), HRTIMER_MODE_REL);
			pDRV2624->mnVibratorPlaying = YES;
			if (pDRV2624->mbIRQUsed)
				nResult = drv2624_enableIRQ(pDRV2624, YES);
		}
	}
end:
	if (nResult < 0) {
		wake_unlock(&pDRV2624->wklock);
		if (g_logEnable)
			dev_dbg(pDRV2624->dev, "wklock unlock");
	}
	mutex_unlock(&pDRV2624->lock);
}
static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	struct drv2624_data *pDRV2624 =
		container_of(timer, struct drv2624_data, timer);
	dev_dbg(pDRV2624->dev, "%s\n", __func__);
	schedule_work(&pDRV2624->vibrator_work);
	return HRTIMER_NORESTART;
}
static void vibrator_work_routine(struct work_struct *work)
{
	struct drv2624_data *pDRV2624 =
		container_of(work, struct drv2624_data, vibrator_work);
	unsigned char mode = MODE_RTP;
	unsigned char status;
	int nResult = 0;
	mutex_lock(&pDRV2624->lock);
	if (g_logEnable)
		dev_dbg(pDRV2624->dev, "%s, afer mnWorkMode=0x%x\n",
			__func__, pDRV2624->mnWorkMode);
	if ((pDRV2624->mnWorkMode & WORK_IRQ) && pDRV2624->mbIRQUsed) {
		nResult = drv2624_reg_read(pDRV2624,DRV2624_REG_STATUS);
		if (nResult >= 0)
			pDRV2624->mnIntStatus = nResult;
		drv2624_disableIRQ(pDRV2624);
		if (nResult < 0)
			goto err;
		status = pDRV2624->mnIntStatus;
		dev_dbg(pDRV2624->dev, "%s, status=0x%x\n",
			__func__, pDRV2624->mnIntStatus);
		if (status & OVERCURRENT_MASK)
			dev_err(pDRV2624->dev, "ERROR, Over Current detected!!\n");
		if (status & OVERTEMPRATURE_MASK)
			dev_err(pDRV2624->dev, "ERROR, Over Temperature detected!!\n");
		if (status & ULVO_MASK)
			dev_err(pDRV2624->dev, "ERROR, VDD drop observed!!\n");
		if (status & PRG_ERR_MASK)
			dev_err(pDRV2624->dev, "ERROR, PRG error!!\n");
		if (status & PROCESS_DONE_MASK) {
			nResult = drv2624_reg_read(pDRV2624, DRV2624_REG_MODE);
			if (nResult < 0)
				goto err;
			mode =  nResult & WORKMODE_MASK;
			if (mode == MODE_CALIBRATION)
				nResult = drv2624_get_calibration_result(pDRV2624, status);
			else if (mode == MODE_DIAGNOSTIC)
				nResult = drv2624_get_diag_result(pDRV2624, status);
			else if (mode == MODE_WAVEFORM_SEQUENCER)
				dev_dbg(pDRV2624->dev,
					"Waveform Sequencer Playback finished\n");
			else if(mode == MODE_RTP)
				dev_dbg(pDRV2624->dev, "RTP IRQ\n");
		}
		if ((mode != MODE_RTP) && (pDRV2624->mnVibratorPlaying == YES))
			drv2624_set_stopflag(pDRV2624);
		pDRV2624->mnWorkMode &= ~WORK_IRQ;
	}
	if (pDRV2624->mnWorkMode & WORK_VIBRATOR) {
		drv2624_stop(pDRV2624);
		pDRV2624->mnWorkMode &= ~WORK_VIBRATOR;
	}
	if (pDRV2624->mnWorkMode & WORK_EFFECTSEQUENCER) {
		status = drv2624_reg_read(pDRV2624, DRV2624_REG_GO);
		if ((status < 0) || (status == STOP) || !pDRV2624->mnVibratorPlaying) {
			drv2624_stop(pDRV2624);
			pDRV2624->mnWorkMode &= ~WORK_EFFECTSEQUENCER;
		} else {
			if (!hrtimer_active(&pDRV2624->timer)) {
				if (g_logEnable)
					dev_dbg(pDRV2624->dev, "will check GO bit after %d ms\n", POLL_GO_BIT_INTERVAL);
				hrtimer_start(&pDRV2624->timer,
					ns_to_ktime((u64)POLL_GO_BIT_INTERVAL * NSEC_PER_MSEC), HRTIMER_MODE_REL);
			}
		}
	}
	if (pDRV2624->mnWorkMode & WORK_CALIBRATION) {
		status = drv2624_reg_read(pDRV2624, DRV2624_REG_GO);
		if ((status < 0) || (status == STOP) || !pDRV2624->mnVibratorPlaying) {
			drv2624_stop(pDRV2624);
			pDRV2624->mnWorkMode &= ~WORK_CALIBRATION;
			if (status < 0)
				goto err;
			nResult = drv2624_reg_read(pDRV2624, DRV2624_REG_STATUS);
			if (nResult >= 0)
				nResult = drv2624_get_calibration_result(pDRV2624, nResult);
		} else {
			if (!hrtimer_active(&pDRV2624->timer)) {
				if (g_logEnable)
					dev_dbg(pDRV2624->dev, "will check GO bit after %d ms\n", POLL_GO_BIT_INTERVAL);
				hrtimer_start(&pDRV2624->timer,
					ns_to_ktime((u64)POLL_GO_BIT_INTERVAL * NSEC_PER_MSEC), HRTIMER_MODE_REL);
			}
		}
	}
	if (pDRV2624->mnWorkMode & WORK_DIAGNOSTIC) {
		status = drv2624_reg_read(pDRV2624, DRV2624_REG_GO);
		if ((status < 0) || (status == STOP) || !pDRV2624->mnVibratorPlaying) {
			drv2624_stop(pDRV2624);
			pDRV2624->mnWorkMode &= ~WORK_DIAGNOSTIC;
			if (status < 0)
				goto err;
			nResult = drv2624_reg_read(pDRV2624, DRV2624_REG_STATUS);
			if (nResult >= 0)
				nResult = drv2624_get_diag_result(pDRV2624, nResult);
		} else {
			if (!hrtimer_active(&pDRV2624->timer)) {
				if (g_logEnable)
					dev_dbg(pDRV2624->dev, "will check GO bit after %d ms\n", POLL_GO_BIT_INTERVAL);
				hrtimer_start(&pDRV2624->timer,
					ns_to_ktime((u64)POLL_GO_BIT_INTERVAL * NSEC_PER_MSEC), HRTIMER_MODE_REL);
			}
		}
	}
err:
	mutex_unlock(&pDRV2624->lock);
}
static int dev_auto_calibrate(struct drv2624_data *pDRV2624)
{
	int nResult = 0;
	dev_info(pDRV2624->dev, "%s\n", __func__);
	wake_lock(&pDRV2624->wklock);
	if (g_logEnable)
		dev_dbg(pDRV2624->dev, "wklock lock");
	nResult = drv2624_change_mode(pDRV2624, MODE_CALIBRATION);
	if (nResult < 0)
		goto end;
	nResult = drv2624_set_go_bit(pDRV2624, GO);
	if (nResult < 0)
		goto end;
	dev_dbg(pDRV2624->dev, "calibration start\n");
	pDRV2624->mnVibratorPlaying = YES;
	if (!pDRV2624->mbIRQUsed) {
		pDRV2624->mnWorkMode |= WORK_CALIBRATION;
		schedule_work(&pDRV2624->vibrator_work);
	}
end:
	if (nResult < 0) {
		wake_unlock(&pDRV2624->wklock);
		if (g_logEnable)
			dev_dbg(pDRV2624->dev, "wklock unlock");
	}
	return nResult;
}
static int dev_run_diagnostics(struct drv2624_data *pDRV2624)
{
	int nResult = 0;
	dev_info(pDRV2624->dev, "%s\n", __func__);
	wake_lock(&pDRV2624->wklock);
	if (g_logEnable)
		dev_dbg(pDRV2624->dev, "wklock lock");
	nResult = drv2624_change_mode(pDRV2624, MODE_DIAGNOSTIC);
	if (nResult < 0)
		goto end;
	nResult = drv2624_set_go_bit(pDRV2624, GO);
	if (nResult < 0)
		goto end;
	dev_dbg(pDRV2624->dev, "Diag start\n");
	pDRV2624->mnVibratorPlaying = YES;
	if (!pDRV2624->mbIRQUsed) {
		pDRV2624->mnWorkMode |= WORK_DIAGNOSTIC;
		schedule_work(&pDRV2624->vibrator_work);
	}
end:
	if (nResult < 0) {
		wake_unlock(&pDRV2624->wklock);
		if (g_logEnable)
			dev_dbg(pDRV2624->dev, "wklock unlock");
	}
	return nResult;
}
static int drv2624_playEffect(struct drv2624_data *pDRV2624)
{
	int nResult = 0;
	dev_info(pDRV2624->dev, "%s\n", __func__);
	wake_lock(&pDRV2624->wklock);
	if (g_logEnable)
		dev_dbg(pDRV2624->dev, "wklock lock");
	nResult = drv2624_change_mode(pDRV2624, MODE_WAVEFORM_SEQUENCER);
	if (nResult < 0)
		goto end;
	nResult = drv2624_set_go_bit(pDRV2624, GO);
	if (nResult < 0)
		goto end;
	dev_dbg(pDRV2624->dev, "effects start\n");
	pDRV2624->mnVibratorPlaying = YES;
	if (!pDRV2624->mbIRQUsed) {
		pDRV2624->mnWorkMode |= WORK_EFFECTSEQUENCER;
		schedule_work(&pDRV2624->vibrator_work);
	}
end:
	if (nResult < 0) {
		wake_unlock(&pDRV2624->wklock);
		dev_dbg(pDRV2624->dev, "wklock unlock");
	}
	return nResult;
}

static int drv2624_config_waveform(struct drv2624_data *pDRV2624,
	struct drv2624_wave_setting *psetting)
{
	int nResult = 0;
	int value = 0;
	nResult = drv2624_reg_write(pDRV2624,
			DRV2624_REG_MAIN_LOOP, psetting->mnLoop & 0x07);
	if (nResult >= 0) {
		value |= ((psetting->mnInterval & 0x01) << INTERVAL_SHIFT);
		value |= (psetting->mnScale & 0x03);
		nResult = drv2624_set_bits(pDRV2624, DRV2624_REG_CONTROL2,
					INTERVAL_MASK | SCALE_MASK, value);
	}
	return nResult;
}
static int drv2624_set_waveform(struct drv2624_data *pDRV2624,
	struct drv2624_waveform_sequencer *pSequencer)
{
	int nResult = 0;
	int i = 0;
	unsigned char loop[2] = {0};
	unsigned char effects[DRV2624_SEQUENCER_SIZE] = {0};
	unsigned char len = 0;
	for (i = 0; i < DRV2624_SEQUENCER_SIZE; i++) {
		len++;
		if (pSequencer->msWaveform[i].mnEffect != 0) {
			if (i < 4)
				loop[0] |= (pSequencer->msWaveform[i].mnLoop << (2*i));
			else
				loop[1] |= (pSequencer->msWaveform[i].mnLoop << (2*(i-4)));
			effects[i] = pSequencer->msWaveform[i].mnEffect;
		} else
			break;
	}
	if (len == 1)
		nResult = drv2624_reg_write(pDRV2624, DRV2624_REG_SEQUENCER_1, 0);
	else
		nResult = drv2624_bulk_write(pDRV2624, DRV2624_REG_SEQUENCER_1, effects, len);
	if (nResult < 0) {
		dev_err(pDRV2624->dev, "sequence error\n");
		goto end;
	}
	if (len > 1) {
		if ((len-1) <= 4)
			drv2624_reg_write(pDRV2624, DRV2624_REG_SEQ_LOOP_1, loop[0]);
		else
			drv2624_bulk_write(pDRV2624, DRV2624_REG_SEQ_LOOP_1, loop, 2);
	}
end:
	return nResult;
}
static int fw_chksum(const struct firmware *fw)
{
	int sum = 0;
	int i=0;
	int size = fw->size;
	const unsigned char *pBuf = fw->data;
	for (i=0; i < size; i++) {
		if ((i <= 11) || (i >= 16))
			sum += pBuf[i];
	}
	return sum;
}
/* drv2624_firmware_load:
* This function is called by the
* request_firmware_nowait function as soon
* as the firmware has been loaded from the file.
* The firmware structure contains the data and$
* the size of the firmware loaded.
*
* @fw: pointer to firmware file to be dowloaded
* @context: pointer variable to drv2624 data
*/
static void drv2624_firmware_load(const struct firmware *fw, void *context)
{
	struct drv2624_data *pDRV2624 = context;
	int size = 0, fwsize = 0, i=0;
	const unsigned char *pBuf = NULL;
	mutex_lock(&pDRV2624->lock);
	if (fw != NULL) {
		pBuf = fw->data;
		size = fw->size;
		if (size > 1024)
			dev_err(pDRV2624->dev,"%s, ERROR!! firmware size %d too big\n",
				__func__, size);
		else {
			memcpy(&(pDRV2624->fw_header), pBuf, sizeof(struct drv2624_fw_header));
			if ((pDRV2624->fw_header.fw_magic != DRV2624_MAGIC)
				|| (pDRV2624->fw_header.fw_size != size)
				|| (pDRV2624->fw_header.fw_chksum != fw_chksum(fw)))
				dev_err(pDRV2624->dev,
					"%s, ERROR!! firmware not right:Magic=0x%x, Size=%d, chksum=0x%x\n",
					__func__, pDRV2624->fw_header.fw_magic,
					pDRV2624->fw_header.fw_size, pDRV2624->fw_header.fw_chksum);
			else {
				dev_info(pDRV2624->dev,"%s, firmware good\n", __func__);
				pBuf += sizeof(struct drv2624_fw_header);
				drv2624_reg_write(pDRV2624, DRV2624_REG_RAM_ADDR_UPPER, 0);
				drv2624_reg_write(pDRV2624, DRV2624_REG_RAM_ADDR_LOWER, 0);
				fwsize = size - sizeof(struct drv2624_fw_header);
				for(i = 0; i < fwsize; i++)
					drv2624_reg_write(pDRV2624, DRV2624_REG_RAM_DATA, pBuf[i]);
			}
		}
	} else
		dev_err(pDRV2624->dev, "%s, ERROR!! firmware not found\n", __func__);
	release_firmware(fw);
    mutex_unlock(&pDRV2624->lock);
}
static int drv2624_file_open(struct inode *inode, struct file *file)
{
	if (!try_module_get(THIS_MODULE))
		return -ENODEV;
	file->private_data = (void*)g_DRV2624data;
	return 0;
}
static int drv2624_file_release(struct inode *inode, struct file *file)
{
	file->private_data = (void*)NULL;
	module_put(THIS_MODULE);
	return 0;
}
static long drv2624_file_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct drv2624_data *pDRV2624 = file->private_data;
	//void __user *user_arg = (void __user *)arg;
	int nResult = 0;
	mutex_lock(&pDRV2624->lock);
	dev_dbg(pDRV2624->dev, "ioctl 0x%x\n", cmd);
	switch (cmd) {
	}
	mutex_unlock(&pDRV2624->lock);
	return nResult;
}
static ssize_t drv2624_file_read(struct file* filp, char* buff, size_t length, loff_t* offset)
{
	struct drv2624_data *pDRV2624 = (struct drv2624_data *)filp->private_data;
	int nResult = 0;
	unsigned char value = 0;
	unsigned char *p_kBuf = NULL;
	mutex_lock(&pDRV2624->lock);
	switch (pDRV2624->mnFileCmd) {
	case HAPTIC_CMDID_REG_READ:
		if (length == 1) {
			nResult = drv2624_reg_read(pDRV2624, pDRV2624->mnCurrentReg);
			if (nResult >= 0) {
				value = nResult;
				nResult = copy_to_user(buff, &value, 1);
				if (0 != nResult) {
					/* Failed to copy all the data, exit */
					dev_err(pDRV2624->dev, "copy to user fail %d\n", nResult);
				}
			}
		} else if (length > 1) {
			p_kBuf = (unsigned char *)kzalloc(length, GFP_KERNEL);
			if (p_kBuf != NULL) {
				nResult = drv2624_bulk_read(pDRV2624,
					pDRV2624->mnCurrentReg, p_kBuf, length);
				if (nResult >= 0) {
					nResult = copy_to_user(buff, p_kBuf, length);
					if (0 != nResult) {
						/* Failed to copy all the data, exit */
						dev_err(pDRV2624->dev, "copy to user fail %d\n", nResult);
					}
				}
				kfree(p_kBuf);
			} else {
				dev_err(pDRV2624->dev, "read no mem\n");
				nResult = -ENOMEM;
			}
		}
		break;
	case HAPTIC_CMDID_RUN_DIAG:
		if (pDRV2624->mnVibratorPlaying)
			length = 0;
		else {
			unsigned char buf[3];
			buf[0] = pDRV2624->mDiagResult.mnResult;
			buf[1] = pDRV2624->mDiagResult.mnDiagZ;
			buf[2] = pDRV2624->mDiagResult.mnDiagK;
			nResult = copy_to_user(buff, buf, 3);
			if (0 != nResult) {
				/* Failed to copy all the data, exit */
				dev_err(pDRV2624->dev, "copy to user fail %d\n", nResult);
			}
		}
		break;
	case HAPTIC_CMDID_RUN_CALIBRATION:
		if (pDRV2624->mnVibratorPlaying)
			length = 0;
		else {
			unsigned char buf[4];
			buf[0] = pDRV2624->mAutoCalResult.mnResult;
			buf[1] = pDRV2624->mAutoCalResult.mnCalComp;
			buf[2] = pDRV2624->mAutoCalResult.mnCalBemf;
			buf[3] = pDRV2624->mAutoCalResult.mnCalGain;
			nResult = copy_to_user(buff, buf, 4);
			if (0 != nResult) {
				/* Failed to copy all the data, exit */
				dev_err(pDRV2624->dev, "copy to user fail %d\n", nResult);
			}
		}
		break;
	case HAPTIC_CMDID_CONFIG_WAVEFORM:
		if (length == sizeof(struct drv2624_wave_setting)) {
			struct drv2624_wave_setting wavesetting;
			value = drv2624_reg_read(pDRV2624, DRV2624_REG_CONTROL2);
			wavesetting.mnLoop = drv2624_reg_read(pDRV2624, DRV2624_REG_MAIN_LOOP)&0x07;
			wavesetting.mnInterval = ((value&INTERVAL_MASK)>>INTERVAL_SHIFT);
			wavesetting.mnScale = (value&SCALE_MASK);
			nResult = copy_to_user(buff, &wavesetting, length);
			if (0 != nResult) {
				/* Failed to copy all the data, exit */
				dev_err(pDRV2624->dev, "copy to user fail %d\n", nResult);
			}
		}
		break;
	case HAPTIC_CMDID_SET_SEQUENCER:
		if (length == sizeof(struct drv2624_waveform_sequencer)) {
			struct drv2624_waveform_sequencer sequencer;
			unsigned char effects[DRV2624_SEQUENCER_SIZE] = {0};
			unsigned char loop[2] = {0};
			int i = 0;
			nResult = drv2624_bulk_read(pDRV2624, DRV2624_REG_SEQUENCER_1,
						effects, DRV2624_SEQUENCER_SIZE);
			if (nResult < 0)
				break;
			nResult = drv2624_bulk_read(pDRV2624, DRV2624_REG_SEQ_LOOP_1, loop, 2);
			if (nResult < 0)
				break;
			for(i = 0; i < DRV2624_SEQUENCER_SIZE; i++){
				sequencer.msWaveform[i].mnEffect = effects[i];
				if(i < 4)
					sequencer.msWaveform[i].mnLoop = ((loop[0]>>(2*i))&0x03);
				else
					sequencer.msWaveform[i].mnLoop = ((loop[1]>>(2*(i-4)))&0x03);
			}
			nResult = copy_to_user(buff, &sequencer, length);
			if (0 != nResult) {
				/* Failed to copy all the data, exit */
				dev_err(pDRV2624->dev, "copy to user fail %d\n", nResult);
			}
		}
		break;
	case HAPTIC_CMDID_READ_FIRMWARE:
		if (length > 0) {
			int i = 0;
			p_kBuf = (unsigned char *)kzalloc(length, GFP_KERNEL);
			if(p_kBuf != NULL){
				nResult = drv2624_reg_write(pDRV2624, DRV2624_REG_RAM_ADDR_UPPER, pDRV2624->mRAMMSB);
				if (nResult < 0)
					break;
				nResult = drv2624_reg_write(pDRV2624, DRV2624_REG_RAM_ADDR_LOWER, pDRV2624->mRAMLSB);
				if (nResult < 0)
					break;
				for (i = 0; i < length; i++) {
					nResult = drv2624_reg_read(pDRV2624,DRV2624_REG_RAM_DATA);
					if (nResult < 0)
						break;
					p_kBuf[i] = nResult;
				}
				if(0 > nResult)
					break;
				nResult = copy_to_user(buff, p_kBuf, length);
				if (0 != nResult) {
					/* Failed to copy all the data, exit */
					dev_err(pDRV2624->dev, "copy to user fail %d\n", nResult);
				}
				kfree(p_kBuf);
			} else {
				dev_err(pDRV2624->dev, "read no mem\n");
				nResult = -ENOMEM;
			}
		}
		break;
	case HAPTIC_CMDID_REGLOG_ENABLE:
		if (length == 1) {
			nResult = copy_to_user(buff, &g_logEnable, 1);
			if (0 != nResult) {
				/* Failed to copy all the data, exit */
				dev_err(pDRV2624->dev, "copy to user fail %d\n", nResult);
			}
		}
		break;
	default:
		pDRV2624->mnFileCmd = 0;
		break;
	}
	mutex_unlock(&pDRV2624->lock);
    return length;
}
static ssize_t drv2624_file_write(struct file* filp, const char* buff, size_t len, loff_t* off)
{
	struct drv2624_data *pDRV2624 = (struct drv2624_data *)filp->private_data;
	unsigned char *p_kBuf = NULL;
	int nResult = 0;
	mutex_lock(&pDRV2624->lock);
	p_kBuf = (unsigned char *)kzalloc(len, GFP_KERNEL);
	if (p_kBuf == NULL) {
		dev_err(pDRV2624->dev, "write no mem\n");
		goto err;
	}
	nResult = copy_from_user(p_kBuf, buff, len);
	if (0 != nResult) {
		dev_err(pDRV2624->dev,"copy_from_user failed.\n");
		goto err;
	}
	pDRV2624->mnFileCmd = p_kBuf[0];
	switch(pDRV2624->mnFileCmd) {
	case HAPTIC_CMDID_REG_READ:
		if (len == 2)
			pDRV2624->mnCurrentReg = p_kBuf[1];
		else
			dev_err(pDRV2624->dev, " read cmd len %d err\n", len);
		break;
	case HAPTIC_CMDID_REG_WRITE:
		if ((len - 1) == 2)
			nResult = drv2624_reg_write(pDRV2624, p_kBuf[1], p_kBuf[2]);
		else if ((len - 1) > 2)
			nResult = drv2624_bulk_write(pDRV2624, p_kBuf[1], &p_kBuf[2], len-2);
		else
			dev_err(pDRV2624->dev, "%s, reg_write len %d error\n", __func__, len);
		break;
	case HAPTIC_CMDID_REG_SETBIT:
		if (len == 4)
			nResult = drv2624_set_bits(pDRV2624, p_kBuf[1], p_kBuf[2], p_kBuf[3]);
		else
			dev_err(pDRV2624->dev, "setbit len %d error\n", len);
		break;
	case HAPTIC_CMDID_RUN_DIAG:
		nResult = drv2624_stop(pDRV2624);
		if (nResult < 0)
			break;
		nResult = dev_run_diagnostics(pDRV2624);
		if ((nResult >= 0) && pDRV2624->mbIRQUsed)
			drv2624_enableIRQ(pDRV2624, NO);
		break;
	case HAPTIC_CMDID_UPDATE_FIRMWARE:
		nResult = drv2624_stop(pDRV2624);
		if (nResult < 0)
			break;
		nResult = request_firmware_nowait(THIS_MODULE,
							FW_ACTION_HOTPLUG,
							"drv2624.bin",
							pDRV2624->dev,
							GFP_KERNEL,
							pDRV2624,
							drv2624_firmware_load);
		break;
	case HAPTIC_CMDID_READ_FIRMWARE:
		if (len == 3) {
			pDRV2624->mRAMMSB = buff[1];
			pDRV2624->mRAMLSB = buff[2];
		} else
			dev_err(pDRV2624->dev, "%s, read fw len error\n", __func__);
		break;
	case HAPTIC_CMDID_RUN_CALIBRATION:
		nResult = drv2624_stop(pDRV2624);
		if (nResult < 0)
			break;
		nResult = dev_auto_calibrate(pDRV2624);
		if ((nResult >= 0) && pDRV2624->mbIRQUsed)
			drv2624_enableIRQ(pDRV2624, NO);
		break;
	case HAPTIC_CMDID_CONFIG_WAVEFORM:
		if (len == (1 + sizeof(struct drv2624_wave_setting))) {
			struct drv2624_wave_setting wavesetting;
			memcpy(&wavesetting, &p_kBuf[1], sizeof(struct drv2624_wave_setting));
			nResult = drv2624_config_waveform(pDRV2624, &wavesetting);
		} else
			dev_dbg(pDRV2624->dev, "pass cmd, prepare for read\n");
		break;
	case HAPTIC_CMDID_SET_SEQUENCER:
		if (len == (1 + sizeof(struct drv2624_waveform_sequencer))) {
			struct drv2624_waveform_sequencer sequencer;
			memcpy(&sequencer, &p_kBuf[1], sizeof(struct drv2624_waveform_sequencer));
			nResult = drv2624_set_waveform(pDRV2624, &sequencer);
		} else
			dev_dbg(pDRV2624->dev, "pass cmd, prepare for read\n");
		break;
	case HAPTIC_CMDID_PLAY_EFFECT_SEQUENCE:
		nResult = drv2624_stop(pDRV2624);
		if (nResult < 0)
			break;
		nResult = drv2624_playEffect(pDRV2624);
		if ((nResult >= 0) && pDRV2624->mbIRQUsed)
			drv2624_enableIRQ(pDRV2624, NO);
		break;
	case HAPTIC_CMDID_STOP:
		nResult = drv2624_stop(pDRV2624);
		break;
	case HAPTIC_CMDID_REGLOG_ENABLE:
		if (len == 2)
			g_logEnable = p_kBuf[1];
		break;
	default:
		dev_err(pDRV2624->dev, "%s, unknown cmd\n", __func__);
		break;
	}
err:
	if (p_kBuf != NULL)
		kfree(p_kBuf);
    mutex_unlock(&pDRV2624->lock);
    return len;
}
static struct file_operations fops =
{
	.owner = THIS_MODULE,
	.read = drv2624_file_read,
	.write = drv2624_file_write,
	.unlocked_ioctl = drv2624_file_unlocked_ioctl,
	.open = drv2624_file_open,
	.release = drv2624_file_release,
};
static struct miscdevice drv2624_misc =
{
	.minor = MISC_DYNAMIC_MINOR,
	.name = HAPTICS_DEVICE_NAME,
	.fops = &fops,
};

static int Haptics_init(struct drv2624_data *pDRV2624)
{
	int nResult = 0;
	pDRV2624->to_dev.name = "vibrator";
	pDRV2624->to_dev.get_time = vibrator_get_time;
	pDRV2624->to_dev.enable = vibrator_enable;
	nResult = timed_output_dev_register(&(pDRV2624->to_dev));
	if (nResult < 0) {
		dev_err(pDRV2624->dev, "drv2624: fail to create timed output dev\n");
		return nResult;
	}
	nResult = misc_register(&drv2624_misc);
	if (nResult) {
		dev_err(pDRV2624->dev, "drv2624 misc fail: %d\n", nResult);
		return nResult;
	}
	hrtimer_init(&pDRV2624->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	pDRV2624->timer.function = vibrator_timer_func;
	INIT_WORK(&pDRV2624->vibrator_work, vibrator_work_routine);
	wake_lock_init(&pDRV2624->wklock, WAKE_LOCK_SUSPEND, "vibrator");
	mutex_init(&pDRV2624->lock);
	return 0;
}
static void dev_init_platform_data(struct drv2624_data *pDRV2624)
{
	struct drv2624_platform_data *pDrv2624Platdata = &pDRV2624->msPlatData;
	struct actuator_data actuator = pDrv2624Platdata->msActuator;
	unsigned char value_temp = 0;
	unsigned char mask_temp = 0;
	drv2624_set_bits(pDRV2624,
		DRV2624_REG_MODE, PINFUNC_MASK, (PINFUNC_INT<<PINFUNC_SHIFT));
	if ((actuator.mnActuatorType == ERM)
		|| (actuator.mnActuatorType == LRA)) {
		mask_temp |= ACTUATOR_MASK;
		value_temp |= (actuator.mnActuatorType << ACTUATOR_SHIFT);
	}
	if ((pDrv2624Platdata->mnLoop == CLOSE_LOOP)
		|| (pDrv2624Platdata->mnLoop == OPEN_LOOP)) {
		mask_temp |= LOOP_MASK;
		value_temp |= (pDrv2624Platdata->mnLoop << LOOP_SHIFT);
	}
	if (value_temp != 0)
		drv2624_set_bits(pDRV2624, DRV2624_REG_CONTROL1,
			mask_temp|AUTOBRK_OK_MASK, value_temp|AUTOBRK_OK_ENABLE);
	value_temp = 0;
	if (actuator.mnActuatorType == ERM)
		value_temp = LIB_ERM;
	else if (actuator.mnActuatorType == LRA)
		value_temp = LIB_LRA;
	if (value_temp != 0)
		drv2624_set_bits(pDRV2624, DRV2624_REG_CONTROL2, LIB_MASK, value_temp<<LIB_SHIFT);
	if (actuator.mnRatedVoltage != 0)
		drv2624_reg_write(pDRV2624, DRV2624_REG_RATED_VOLTAGE, actuator.mnRatedVoltage);
	else
		dev_err(pDRV2624->dev, "%s, ERROR Rated ZERO\n", __func__);
	if (actuator.mnOverDriveClampVoltage != 0)
		drv2624_reg_write(pDRV2624, DRV2624_REG_OVERDRIVE_CLAMP, actuator.mnOverDriveClampVoltage);
	else
		dev_err(pDRV2624->dev,"%s, ERROR OverDriveVol ZERO\n", __func__);
	if (actuator.mnActuatorType == LRA) {
		unsigned char DriveTime = 5*(1000 - actuator.mnLRAFreq)/actuator.mnLRAFreq;
		unsigned short openLoopPeriod =
			(unsigned short)((unsigned int)1000000000 / (24619 * actuator.mnLRAFreq));
		if (actuator.mnLRAFreq < 125)
			DriveTime |= (MINFREQ_SEL_45HZ << MINFREQ_SEL_SHIFT);
		drv2624_set_bits(pDRV2624, DRV2624_REG_DRIVE_TIME,
			DRIVE_TIME_MASK | MINFREQ_SEL_MASK, DriveTime);
		drv2624_set_bits(pDRV2624, DRV2624_REG_OL_PERIOD_H, 0x03, (openLoopPeriod&0x0300)>>8);
		drv2624_reg_write(pDRV2624, DRV2624_REG_OL_PERIOD_L, (openLoopPeriod&0x00ff));
		dev_info(pDRV2624->dev, "%s, LRA = %d, DriveTime=0x%x\n",
			__func__, actuator.mnLRAFreq, DriveTime);
	}
}
#if 0
static irqreturn_t drv2624_irq_handler(int irq, void *dev_id)
{
	struct drv2624_data *pDRV2624 = (struct drv2624_data *)dev_id;
	pDRV2624->mnWorkMode |= WORK_IRQ;
	schedule_work(&pDRV2624->vibrator_work);
	return IRQ_HANDLED;
}
#endif
static int drv2624_parse_dt(struct device *dev, struct drv2624_data *pDRV2624)
{
	struct device_node *np = dev->of_node;
	struct drv2624_platform_data *pPlatData = &pDRV2624->msPlatData;
	int rc= 0, nResult = 0;
	unsigned int value;
	pPlatData->mnGpioNRST = of_get_named_gpio(np, "ti,reset-gpio", 0);
	if (pPlatData->mnGpioNRST < 0) {
		dev_err(pDRV2624->dev, "Looking up %s property in node %s failed %d\n",
			"ti,reset-gpio", np->full_name, pPlatData->mnGpioNRST);
		nResult = -EINVAL;
	} else
		dev_dbg(pDRV2624->dev, "ti,reset-gpio=%d\n", pPlatData->mnGpioNRST);
	if (nResult >=0) {
		rc = of_property_read_u32(np, "ti,smart-loop", &value);
		if (rc) {
			dev_err(pDRV2624->dev, "Looking up %s property in node %s failed %d\n",
				"ti,smart-loop", np->full_name, rc);
			nResult = -EINVAL;
		} else {
			pPlatData->mnLoop = value & 0x01;
			dev_dbg(pDRV2624->dev, "ti,smart-loop=%d\n", pPlatData->mnLoop);
		}
	}
	if (nResult >=0) {
		rc = of_property_read_u32(np, "ti,actuator", &value);
		if (rc) {
			dev_err(pDRV2624->dev, "Looking up %s property in node %s failed %d\n",
				"ti,actuator", np->full_name, rc);
			nResult = -EINVAL;
		} else {
			pPlatData->msActuator.mnActuatorType = value & 0x01;
			dev_dbg(pDRV2624->dev, "ti,actuator=%d\n",
				pPlatData->msActuator.mnActuatorType);
		}
	}
	if (nResult >=0) {
		rc = of_property_read_u32(np, "ti,rated-voltage", &value);
		if (rc) {
			dev_err(pDRV2624->dev, "Looking up %s property in node %s failed %d\n",
				"ti,rated-voltage", np->full_name, rc);
			nResult = -EINVAL;
		}else{
			pPlatData->msActuator.mnRatedVoltage = value;
			dev_dbg(pDRV2624->dev, "ti,rated-voltage=0x%x\n",
				pPlatData->msActuator.mnRatedVoltage);
		}
	}
	if (nResult >=0) {
		rc = of_property_read_u32(np, "ti,odclamp-voltage", &value);
		if (rc) {
			dev_err(pDRV2624->dev, "Looking up %s property in node %s failed %d\n",
				"ti,odclamp-voltage", np->full_name, rc);
			nResult = -EINVAL;
		} else {
			pPlatData->msActuator.mnOverDriveClampVoltage = value;
			dev_dbg(pDRV2624->dev, "ti,odclamp-voltage=0x%x\n",
				pPlatData->msActuator.mnOverDriveClampVoltage);
		}
	}
	if ((nResult >= 0) && (pPlatData->msActuator.mnActuatorType == LRA)) {
		rc = of_property_read_u32(np, "ti,lra-frequency", &value);
		if (rc) {
			dev_err(pDRV2624->dev, "Looking up %s property in node %s failed %d\n",
				"ti,lra-frequency", np->full_name, rc);
			nResult = -EINVAL;
		} else {
			if ((value >= 45) && (value <= 300)) {
				pPlatData->msActuator.mnLRAFreq = value;
				dev_dbg(pDRV2624->dev, "ti,lra-frequency=%d\n",
					pPlatData->msActuator.mnLRAFreq);
			} else {
				nResult = -EINVAL;
				dev_err(pDRV2624->dev, "ERROR, ti,lra-frequency=%d, out of range\n",
					pPlatData->msActuator.mnLRAFreq);
			}
		}
	}
#if 0
	if (nResult >= 0) {
		pPlatData->mnGpioINT = of_get_named_gpio(np, "ti,irq-gpio", 0);
		if (pPlatData->mnGpioINT < 0) {
			dev_err(pDRV2624->dev, "Looking up %s property in node %s failed %d\n",
				"ti,irq-gpio", np->full_name, pPlatData->mnGpioINT);
		} else
			dev_dbg(pDRV2624->dev, "ti,irq-gpio=%d\n", pPlatData->mnGpioINT);
	}
#endif
	return nResult;
}
#ifdef CONFIG_PM_SLEEP
static int drv2624_suspend(struct device *dev)
{
	struct drv2624_data *pDRV2624 = dev_get_drvdata(dev);
	dev_dbg(pDRV2624->dev, "%s\n", __func__);
	mutex_lock(&pDRV2624->lock);
	if (hrtimer_active(&pDRV2624->timer) || pDRV2624->mnVibratorPlaying)
		drv2624_stop(pDRV2624);
	mutex_unlock(&pDRV2624->lock);
	return 0;
}
static int drv2624_resume(struct device *dev)
{
	return 0;
}
#endif
static struct regmap_config drv2624_i2c_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0xff,
	.cache_type = REGCACHE_NONE,
};
static int drv2624_i2c_probe(struct i2c_client* client, const struct i2c_device_id* id)
{
	struct drv2624_data *pDRV2624;
	int nResult = 0;
	dev_info(&client->dev, "%s enter\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "%s:I2C check failed\n", __func__);
		return -ENODEV;
	}
	pDRV2624 = devm_kzalloc(&client->dev, sizeof(struct drv2624_data), GFP_KERNEL);
	if (pDRV2624 == NULL) {
		dev_err(&client->dev, "%s:no memory\n", __func__);
		return -ENOMEM;
	}
	dev_info(&client->dev, "addres: 0x%x \n", client->addr);
	client->addr=0x5a;
	dev_info(&client->dev, "addres: 0x%x \n", client->addr);

	pDRV2624->dev = &client->dev;
	i2c_set_clientdata(client,pDRV2624);
	dev_set_drvdata(&client->dev, pDRV2624);
	pDRV2624->mpRegmap = devm_regmap_init_i2c(client, &drv2624_i2c_regmap);
	if (IS_ERR(pDRV2624->mpRegmap)) {
		nResult = PTR_ERR(pDRV2624->mpRegmap);
		dev_err(pDRV2624->dev, "%s:Failed to allocate register map: %d\n",
			__func__, nResult);
		return nResult;
	}
	if (client->dev.of_node) {
		dev_dbg(pDRV2624->dev, "of node parse\n");
		nResult = drv2624_parse_dt(&client->dev, pDRV2624);
	} else if (client->dev.platform_data) {
		dev_dbg(pDRV2624->dev, "platform data parse\n");
		memcpy(&pDRV2624->msPlatData, client->dev.platform_data,
			sizeof(struct drv2624_platform_data));
	} else {
		dev_err(pDRV2624->dev, "%s: ERROR no platform data\n",__func__);
		return -EINVAL;
	}
	if ((nResult < 0) || !gpio_is_valid(pDRV2624->msPlatData.mnGpioNRST)) {
		dev_err(pDRV2624->dev, "%s: platform data error\n",__func__);
		return -EINVAL;
	}
	if (gpio_is_valid(pDRV2624->msPlatData.mnGpioNRST)) {
		nResult = gpio_request(pDRV2624->msPlatData.mnGpioNRST,"DRV2624-NRST");
		if (nResult < 0) {
			dev_err(pDRV2624->dev, "%s: GPIO %d request NRST error\n",
				__func__, pDRV2624->msPlatData.mnGpioNRST);
			return nResult;
		}
		gpio_direction_output(pDRV2624->msPlatData.mnGpioNRST, 0);
		mdelay(5);
		gpio_direction_output(pDRV2624->msPlatData.mnGpioNRST, 1);
		mdelay(2);
	}
	mutex_init(&pDRV2624->dev_lock);
	nResult = drv2624_reg_read(pDRV2624, DRV2624_REG_ID);
	if (nResult < 0)
		goto exit_gpio_request_failed1;
	else {
		dev_info(pDRV2624->dev, "%s, ID status (0x%x)\n", __func__, nResult);
		pDRV2624->mnDeviceID = nResult;
	}
	if ((pDRV2624->mnDeviceID & 0xf0) != DRV2624_ID) {
		dev_err(pDRV2624->dev, "%s, device_id(0x%x) fail\n",
			__func__, pDRV2624->mnDeviceID);
		goto exit_gpio_request_failed1;
	}
	dev_init_platform_data(pDRV2624);

#if 0
	if (gpio_is_valid(pDRV2624->msPlatData.mnGpioINT)) {
		nResult = gpio_request(pDRV2624->msPlatData.mnGpioINT, "DRV2624-IRQ");
		if (nResult < 0) {
			dev_err(pDRV2624->dev, "%s: GPIO %d request INT error\n",
				__func__, pDRV2624->msPlatData.mnGpioINT);
			goto exit_gpio_request_failed1;
		}
		gpio_direction_input(pDRV2624->msPlatData.mnGpioINT);
		pDRV2624->mnIRQ = gpio_to_irq(pDRV2624->msPlatData.mnGpioINT);
		dev_dbg(pDRV2624->dev, "irq = %d \n", pDRV2624->mnIRQ);
		nResult = request_threaded_irq(pDRV2624->mnIRQ, drv2624_irq_handler,
				NULL, IRQF_TRIGGER_FALLING | IRQF_ONESHOT, client->name, pDRV2624);
		if (nResult < 0) {
			dev_err(pDRV2624->dev, "request_irq failed, %d\n", nResult);
			goto exit_gpio_request_failed2;
		}
		disable_irq_nosync(pDRV2624->mnIRQ);
		pDRV2624->mbIRQEnabled = false;
		pDRV2624->mbIRQUsed = true;
	} else
#endif
		pDRV2624->mbIRQUsed = false;
	g_DRV2624data = pDRV2624;
	Haptics_init(pDRV2624);
	request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG, "drv2624.bin",
		&(client->dev), GFP_KERNEL, pDRV2624, drv2624_firmware_load);
	dev_info(pDRV2624->dev, "drv2624 probe succeeded\n");
	return 0;
//exit_gpio_request_failed2:
//	if (gpio_is_valid(pDRV2624->msPlatData.mnGpioINT))
//		gpio_free(pDRV2624->msPlatData.mnGpioINT);
exit_gpio_request_failed1:
	if (gpio_is_valid(pDRV2624->msPlatData.mnGpioNRST))
		gpio_free(pDRV2624->msPlatData.mnGpioNRST);
	mutex_destroy(&pDRV2624->dev_lock);
	dev_err(pDRV2624->dev, "%s failed, err=%d\n", __func__, nResult);
	return nResult;
}
static int drv2624_i2c_remove(struct i2c_client* client)
{
	struct drv2624_data *pDRV2624 = i2c_get_clientdata(client);
	if(pDRV2624->msPlatData.mnGpioNRST)
		gpio_free(pDRV2624->msPlatData.mnGpioNRST);
	if(pDRV2624->msPlatData.mnGpioINT)
		gpio_free(pDRV2624->msPlatData.mnGpioINT);
	misc_deregister(&drv2624_misc);
	mutex_destroy(&pDRV2624->lock);
	mutex_destroy(&pDRV2624->dev_lock);
	return 0;
}
static const struct i2c_device_id drv2624_i2c_id[] = {
	{"drv2624", 0},
	{}
};
#ifdef CONFIG_PM_SLEEP
static const struct dev_pm_ops drv2624_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(drv2624_suspend, drv2624_resume)
};
#endif
MODULE_DEVICE_TABLE(i2c, drv2624_i2c_id);
#if defined(CONFIG_OF)
static const struct of_device_id drv2624_of_match[] = {
	{.compatible = "ti,drv2624"},
	{},
};
MODULE_DEVICE_TABLE(of, drv2624_of_match);
#endif
static struct i2c_driver drv2624_i2c_driver = {
	.driver = {
			.name = "drv2624",
			.owner = THIS_MODULE,
#ifdef CONFIG_PM_SLEEP
			.pm = &drv2624_pm_ops,
#endif
#if defined(CONFIG_OF)
			.of_match_table = of_match_ptr(drv2624_of_match),
#endif
		},
	.probe = drv2624_i2c_probe,
	.remove = drv2624_i2c_remove,
	.id_table = drv2624_i2c_id,
};
module_i2c_driver(drv2624_i2c_driver);
MODULE_AUTHOR("Texas Instruments Inc.");
MODULE_DESCRIPTION("DRV2624 I2C Smart Haptics driver");
MODULE_LICENSE("GPLv2");
