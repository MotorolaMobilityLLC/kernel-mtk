#ifndef __SH_SVP_H__
#define __SH_SVP_H__

#if defined(CONFIG_CMA) && defined(CONFIG_MTK_SVP)
int svp_region_offline(phys_addr_t *pa, unsigned long *size);
int svp_region_online(void);

#ifdef CONFIG_TRUSTONIC_TEE_SUPPORT
extern int secmem_api_enable(u32 start, u32 size);
extern int secmem_api_disable(void);
extern int secmem_api_query(u32 *allocate_size);
#else
static inline int secmem_api_enable(u32 start, u32 size) {return 0; }
static inline int secmem_api_disable(void) {return 0; }
static inline int secmem_api_query(u32 *allocate_size) {return 0; }
#endif

#else
static inline int svp_region_offline(void) { return -ENOSYS; }
static inline int svp_region_online(void) { return -ENOSYS; }
#endif

extern struct cma *svp_contiguous_default_area;

#if defined(CONFIG_CMA) && defined(CONFIG_MTK_SVP)
void svp_contiguous_reserve(phys_addr_t addr_limit);

unsigned long get_svp_cma_basepfn(void);
unsigned long get_svp_cma_count(void);

int svp_migrate_range(unsigned long pfn);
int svp_is_in_range(unsigned long pfn);

#else
static inline void svp_contiguous_reserve(phys_addr_t limit) { }
static inline unsigned long get_svp_cma_basepfn(void) { return 0; }
static inline long get_svp_cma_count(void) { return 0; }
static inline int svp_migrate_range(unsigned long pfn) { return 0; }
static inline int svp_is_in_range(unsigned long pfn) { return 0; }
#endif

#define SVP_REGION_IOC_MAGIC		'S'

#define SVP_REGION_IOC_ONLINE		_IOR(SVP_REGION_IOC_MAGIC, 2, int)
#define SVP_REGION_IOC_OFFLINE		_IOR(SVP_REGION_IOC_MAGIC, 4, int)

#endif
