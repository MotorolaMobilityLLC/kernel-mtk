#ifndef __MTK_SPM_SODI_CMDQ_H
#define __MTK_SPM_SODI_CMDQ_H

#include <cmdq_def.h>
#include <cmdq_record.h>
#include <cmdq_reg.h>
#include <cmdq_core.h>

void exit_pd_by_cmdq(struct cmdqRecStruct *handler);
void enter_pd_by_cmdq(struct cmdqRecStruct *handler);

#endif /* __MTK_SPM_SODI_CMDQ_H */
