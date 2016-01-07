#include <linux/platform_device.h>

int mtk_platform_init(struct platform_device *pdev, struct kbase_device *kbdev);


#ifdef ENABLE_MTK_MEMINFO
#define MTK_MEMINFO_SIZE 150

typedef struct
{
	int pid;
	int used_pages;
} mtk_gpu_meminfo_type;
#endif /* ENABLE_MTK_MEMINFO */

#ifdef ENABLE_MTK_MEMINFO
void mtk_kbase_gpu_memory_debug_init(void);
void mtk_kbase_gpu_memory_debug_remove(void);
void mtk_kbase_reset_gpu_meminfo(void);
void mtk_kbase_set_gpu_meminfo(ssize_t index, int pid, int used_pages);
bool mtk_kbase_dump_gpu_memory_usage(void);
unsigned int mtk_kbase_report_gpu_memory_usage(void);
int mtk_kbase_report_gpu_memory_peak(void);
void mtk_kbase_set_gpu_memory_peak(void);
#endif /* ENABLE_MTK_MEMINFO */

#ifdef CONFIG_PROC_FS
void proc_mali_register(void);
void proc_mali_unregister(void);
#endif /* CONFIG_PROC_FS */