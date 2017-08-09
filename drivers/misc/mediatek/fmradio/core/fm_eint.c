#if (defined(MT6620_FM) || defined(MT6628_FM) || defined(MT6627_FM) || defined(MT6580_FM) || defined(MT6630_FM))
#include "osal_typedef.h"
#include "stp_exp.h"
#include "wmt_exp.h"

#include "fm_typedef.h"
#include "fm_dbg.h"
#include "fm_err.h"
#include "fm_eint.h"

fm_s32 fm_enable_eint(void)
{
	return 0;
}

fm_s32 fm_disable_eint(void)
{
	return 0;
}

fm_s32 fm_request_eint(void (*parser) (void))
{
	mtk_wcn_stp_register_event_cb(FM_TASK_INDX, parser);

	return 0;
}

fm_s32 fm_eint_pin_cfg(fm_s32 mode)
{
	return 0;
}
#endif
