#include "mtk_kbase_spm.h"
static const u32 dvfs_gpu_binary[] = {
};
struct pcm_desc dvfs_gpu_pcm = {
	.version	= "SPM_PCMASM_JADE_20150421",
	.base		= dvfs_gpu_binary,
	.size		= 0,
	.sess		= 1,
	.replace	= 0,
};
