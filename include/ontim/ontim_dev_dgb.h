
#define NAME_LEN 64
//+add by hzb
enum ontim_dev_attr_type {
    ONTIM_DEV_ARTTR_TYPE_STR,
    ONTIM_DEV_ARTTR_TYPE_VAL_RO,
    ONTIM_DEV_ARTTR_TYPE_VAL_8BIT,
    ONTIM_DEV_ARTTR_TYPE_VAL_16BIT,
    ONTIM_DEV_ARTTR_TYPE_VAL_32BIT,
    ONTIM_DEV_ARTTR_TYPE_EXEC,
    ONTIM_DEV_ARTTR_TYPE_EXEC_PARAM
};
//-add by hzb
struct dev_arrt
{
    const char *dev_attr_name;
//+modify by hzb
    void *dev_attr_context;
    enum ontim_dev_attr_type dev_attr_type;
//+modify by hzb
};
struct ontim_debug
{
     const unsigned char *dev_name;
     struct dev_arrt *dev_attr_array;
     u8     dev_attr_array_len;
     unsigned int dbg_trace_level;     
};


//+modify by hzb
#define DEV_ATTR_DEFINE(ATTR,CONTEXT)\
     {ATTR,CONTEXT,ONTIM_DEV_ARTTR_TYPE_STR},

#define DEV_ATTR_VAL_DEFINE(ATTR,CONTEXT,TYPE)\
     {ATTR,CONTEXT,TYPE},

#define DEV_ATTR_EXEC_DEFINE(ATTR,CONTEXT)\
     {ATTR,CONTEXT,ONTIM_DEV_ARTTR_TYPE_EXEC},

#define DEV_ATTR_EXEC_DEFINE_PARAM(ATTR,CONTEXT)\
     {ATTR,CONTEXT,ONTIM_DEV_ARTTR_TYPE_EXEC_PARAM},
//-modify by hzb

#define DEV_ATTR_DECLARE(NAME) \
    static struct dev_arrt attr_##NAME[]={ 

#define DEV_ATTR_DECLARE_END \
    }



#define ONTIM_DEBUG_DECLARE_AND_INIT(NAME,ATTR_ARRAY,DBG_LEVEL)\
     static struct ontim_debug NAME={\
     .dev_name=#NAME,\
     .dbg_trace_level=DBG_LEVEL,\
     .dev_attr_array=attr_##ATTR_ARRAY,\
     .dev_attr_array_len=sizeof(attr_##ATTR_ARRAY)/sizeof(struct dev_arrt),\
   };\
   static struct ontim_debug *ontim_debug_for_everyone=&NAME;


#define ontim_dev_printk(level, format, arg...)	\
    if(level>(ontim_debug_for_everyone->dbg_trace_level)){\
	printk(KERN_ERR "%s : " format ,  \
	       ontim_debug_for_everyone->dev_name , ## arg);\
    }

#define ontim_dev_dbg(level,format, arg...)		\
	ontim_dev_printk(level , format , ## arg)


extern int ontim_dev_debug_file_exist(struct ontim_debug *ontim_debug);
extern int regester_and_init_ontim_debug(struct ontim_debug *ontim_debug);
#define CHECK_THIS_DEV_DEBUG_AREADY_EXIT() \
         ontim_dev_debug_file_exist(ontim_debug_for_everyone)
#define REGISTER_AND_INIT_ONTIM_DEBUG_FOR_THIS_DEV()\
         regester_and_init_ontim_debug(ontim_debug_for_everyone)

#if 0
struct dev_arrt dev_attr_array1[]={{.dev_attr_name="mytest_1",.dev_attr_context="i am mytest1_1"}
                    ,{.dev_attr_name="mytest1_2",.dev_attr_context="i am mytest1_2"}};
struct ontim_debug ontim_dbg1=
{
    .dev_name="mytest1",
    .dev_attr_array=dev_attr_array1,
    .dev_attr_array_len=2,
};
struct dev_arrt dev_attr_array2[]={{.dev_attr_name="mytest_2",.dev_attr_context="i am mytest_2"}
                    ,{.dev_attr_name="mytest2_2",.dev_attr_context="i am mytest2_2"}};
struct ontim_debug ontim_dbg2=
{
    .dev_name="mytest2",
    .dev_attr_array=dev_attr_array2,
    .dev_attr_array_len=2,
};
    DEV_ATTR_DECLARE(ontim_dbg1)
    DEV_ATTR_DEFINE("version","20111212")
    DEV_ATTR_DEFINE("vendor","ontim")
    DEV_ATTR_DECLARE_END;
    ONTIM_DEBUG_DECLARE_AND_INIT(ontim_dbg1,ontim_dbg1,1);



    DEV_ATTR_DECLARE(ontim_dbg2)
    DEV_ATTR_DEFINE("version","20111212")
    DEV_ATTR_DEFINE("vendor","ontim")
    DEV_ATTR_DECLARE_END;
    ONTIM_DEBUG_DECLARE_AND_INIT(ontim_dbg2,ontim_dbg2,2);
    regester_and_init_ontim_debug(&ontim_dbg1);
    regester_and_init_ontim_debug(&ontim_dbg2);

#endif

enum touch_factory_name {
    TOUCH_TRULY,
    TOUCH_TIANMA,
    TOUCH_BOYI,
    TOUCH_DIJING,
    TOUCH_TCL,
    TOUCH_NUMBER_TOTAL
};