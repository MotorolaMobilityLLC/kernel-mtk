#ifndef __MT_SPM_SODI_CMDQ_H
#define __MT_SPM_SODI_CMDQ_H

#include "cmdq_def.h"
#include "cmdq_record.h"
#include "cmdq_reg.h"
#include "cmdq_core.h"

void exit_pd_by_cmdq(cmdqRecHandle handler);
void enter_pd_by_cmdq(cmdqRecHandle handler);

#endif /* __MT_SPM_SODI_CMDQ_H */
