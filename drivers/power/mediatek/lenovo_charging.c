/************************************************
         Lenovo-sw yexh1 add for Lenovo charging spec 
         Rule:
         (1) plase keep the lenovo charging code as nice as possible
         (2) please change the mediatek's code as less as possible
         (3) please add some comments in this head 
         Attention:
         (1) please include lenovo_charging.h to use the following functions
         (2) please use the macro LENOVO_CHARGING_STANDARD to control the code in MTK native code
         (3) add the lenovo_charging.o in makefile
         (4) add MACRO in cust_charging.h
              #define AC_CHARGER_CURRENT_LIMIT	CHARGE_CURRENT_600_00_MA   //lenovo standard 0.3C
              #define LENOVO_CHARGING_STANDARD
         (5)    
*************************************************/
#include "lenovo_charging.h"
//lenovo-sw mahj2 modify for battery charging temp notify at 20141212 Begin
#include <linux/power_supply.h>
//lenovo-sw mahj2 modify for battery charging temp notify at 20141212 End
#include <mach/mt_charging.h>
/* lenovo-sw zhangrc2 use pmic to control charging led 2014-12-08 */
#include "../../misc/mediatek/leds/mt6797/leds_sw.h"
/* lenovo-sw zhangrc2 use pmic to control charging led 2014-12-08 */
/*lenovo sw zhangrc2 update code for getting charging current 2014-08-14*/ 	
#include <linux/delay.h>
/*lenovo sw zhangrc2 update code for getting charging current 2014-08-14*/ 	
///////////Lenovo charging global variables//////////
static unsigned int g_battery_notify = 0;
static int g_battery_chg_current = 0;
static int g_battery_vol = 0;
static int g_battery_wake_count = 0;
static kal_bool  g_charger_1st_in_flag = KAL_TRUE;
static kal_bool  g_charger_in_flag = KAL_FALSE;
LENOVO_CHARGING_STATE lenovo_chg_state = CHARGING_STATE_PROTECT;
LENOVO_TEMP_STATE lenovo_temp_state = LENOVO_TEMP_POS_10_TO_POS_45;

static int g_temp_value; 
static int g_battery_charging_time = 0;

static int g_temp_protect_cnt =0;
static int g_temp_err_report_cnt = 0;
static int g_temp_err_resume_cnt= 0;
static int g_temp_to_cc_cnt= 0;
static int g_temp_to_limit_c_cnt= 0;


static int g_battery_debug_value = 0;
static int g_battery_debug_temp = 0;

/*lenovo_sw liaohj add for charging led diff call led 2013-11-14 ---begin*/
#ifdef MTK_FAN5405_SUPPORT
int g_temp_charging_blue_flag = 0;
#endif
/*lenovo_sw liaohj add for charging led diff call led 2013-11-14 ---end*/

	
/*lenovo sw zhangrc2 update code for getting charging current 2014-08-14*/ 	
/////////////////////////////////////////////////////////////////
                ///MTK dependency charging functions///// 
////////////////////////////////////////////////////////////////
static unsigned int g_bat_charging_state;
void lenovo_battery_backup_charging_state(void)
{
       if (BMT_status.bat_charging_state != CHR_ERROR)
	   	g_bat_charging_state = BMT_status.bat_charging_state;
}
void lenovo_battery_resume_charging_state(void)
{
        BMT_status.bat_charging_state = g_bat_charging_state;
	  
}
extern signed int gFG_DOD0;    //mtk dependency
kal_bool lenovo_battery_is_deep_charging(void)
{
       if (gFG_DOD0 > 85)
	   	return KAL_TRUE;
       return KAL_FALSE;
}

void lenovo_battery_get_data(kal_bool in)
{
	g_battery_chg_current = (in ==KAL_TRUE) ? battery_meter_get_charging_current() : 0; 
	g_temp_value = BMT_status.temperature;
	g_battery_vol =BMT_status.bat_vol;

	//DEBUG
	if (g_battery_debug_temp != 0)
		g_temp_value = g_battery_debug_temp;
}

void lenovo_battery_disable_charging(void)
{
    signed int disable = KAL_FALSE;
    battery_charging_control(CHARGING_CMD_ENABLE, &disable);
}

void lenovo_battery_enable_charging(void)
{
    signed int flag = KAL_TRUE;
    battery_charging_control(CHARGING_CMD_ENABLE, &flag);
}

//in lenovo factory mode(####1111#) test, in factory we need to display Ichg ASAP
void lenvovo_battery_wake_bat_thread(void)
{
     if (g_battery_wake_count++ < 5){
        	   printk("[BATTERY] wake up again. Need to display Icharging ASAP in lenovo fac. mode.\n"); 
              // msleep(50);  
        	   wake_up_bat ();
      }else
		       g_battery_wake_count = 5;
}

//MTK dependency
void lenovo_get_charging_curret(void *data)  // kernel panic, if you pass CHR_CURRENT_ENUM*
{    
       if(data == NULL) {
	   	return ;
       }	
	 if ( *(CHR_CURRENT_ENUM*)(data) == CHARGE_CURRENT_0_00_MA){
              return;     
	 }
	 if (lenovo_chg_state == CHARGING_STATE_PROTECT || lenovo_chg_state == CHARGING_STATE_ERROR)
	 	    *(CHR_CURRENT_ENUM*)(data) = CHARGE_CURRENT_0_00_MA;
	 
       if (BMT_status.charger_type == STANDARD_CHARGER)  //call or temp abnormal, we will limit the currenct
       	{     if (g_call_state == CALL_ACTIVE || lenovo_chg_state == CHARGING_STATE_LIMIT_CHARGING){
       	       #ifdef AC_CHARGER_CURRENT_LIMIT
			    /*Begin, lenovo-sw chailu1 add for 45-50  0.5c   20150318 */
			   #ifdef LENOVO_AC_CHARGER_CURRENT_LIMIT_HTEMP_SUPPORT
                             if (lenovo_temp_state == LENOVO_TEMP_POS_45_TO_POS_50)
				     *(CHR_CURRENT_ENUM*)(data)  =  AC_CHARGER_CURRENT_LIMIT_HTEMP; 
				else
				      *(CHR_CURRENT_ENUM*)(data)  =  AC_CHARGER_CURRENT_LIMIT;  //0.2C  	
			   #else
			     *(CHR_CURRENT_ENUM*)(data)  =  AC_CHARGER_CURRENT_LIMIT;  //0.2C 
			    #endif
			  /*End, lenovo-sw chailu1 add for 45-50  0.5c   20150318 */
			 #else  
		            *(CHR_CURRENT_ENUM*)(data)  =  AC_CHARGER_CURRENT;  
		       #endif  
       	      }
       	}	  
}

kal_bool lenovo_battery_is_battery_exist( int R, int Rdown)
{    //if the paralleling resistors almost equal to RBAT_PULL_DOWN_R, we asume the bat not exists
	if(R >= Rdown-2000){	
		lenovo_bat_printk(LENOVO_BAT_LOG_CRTI,  "Battery no exist, TRes_temp=%d, Rdown:%d \n", R,Rdown);
		return KAL_FALSE;
		}
	return KAL_TRUE;
}

#if 0
/*added for led-soc sync*/
#ifdef MTK_FAN5405_SUPPORT
extern void fan5405_set_en_stat(kal_uint32 val);
void charging_led_enable(kal_uint32 val)
{
      fan5405_set_en_stat(val);
}
#endif

/*lenovo_sw liaohj modify for charging led diff call led 2013-11-14 ---begin*/
#ifdef MTK_FAN5405_SUPPORT
void lenovo_battery_charging_set_led_state(void)
{
	if(BMT_status.UI_SOC==100)
	{
		charging_led_enable(0);
	}
	else
	{	
		if(g_temp_charging_blue_flag == 0)
		{
			charging_led_enable(1);
		}
		else
		{
			charging_led_enable(0);
		}
	}
}
#else
void lenovo_battery_charging_set_led_state(void)
{
}	
#endif
/*lenovo_sw liaohj modify for charging led diff call led 2013-11-14 ---end*/
/*added for led-soc sync end*/
#endif

////////////////////////////////////////////////////
           //////Lenovo charging debug functions//////////
///////////////////////////////////////////////////// 
#define LENOVO_START_CHARGING_VOLT  3500   //3.5V
kal_bool g_battery_discharging_flag = KAL_FALSE;
static struct task_struct *p_discharging_task = NULL;

int lenovo_battery_discharging_kthread(void *data)
{
      int life_and_everything; 
      // busy looper //
	do{
            life_and_everything = 99 * 99;
	}while(!kthread_should_stop());
	return 0;
}
void  lenovo_battery_start_discharging(void)
{      
      p_discharging_task = kthread_run(lenovo_battery_discharging_kthread, NULL, "lenovo_battery_discharging_kthread"); 
}
void lenovo_battery_stop_discharging(void)
{      if (p_discharging_task != NULL){
             kthread_stop(p_discharging_task);
		p_discharging_task = NULL;	 
		lenovo_bat_printk(LENOVO_BAT_LOG_CRTI, "stop debug discharging, bat vol:%d \n ",  g_battery_vol);

       }
}
kal_bool lenovo_battery_is_debug_discharging(void)
{
        if (g_battery_debug_value == 1)
        	{  if (g_battery_vol > LENOVO_START_CHARGING_VOLT){
        	          lenovo_bat_printk(LENOVO_BAT_LOG_CRTI, "is debug discharging, bat vol:%d \n ",  g_battery_vol);
       	          if(g_battery_discharging_flag == KAL_FALSE){
                                    lenovo_battery_start_discharging();
					    g_battery_discharging_flag = KAL_TRUE;
					  }
		           return KAL_TRUE;

		   }else{
		            g_battery_debug_value = 0;
		           if(g_battery_discharging_flag == KAL_TRUE){
		                   lenovo_battery_stop_discharging();
			            g_battery_discharging_flag = KAL_FALSE;  
		           	}                      
		   }
		}
	 return KAL_FALSE;
}


////////////////////////////////////////////////////
           //////Lenovo charging functions//////////
/////////////////////////////////////////////////////           
void lenovo_battery_reset_debounce(void)
{
}

void lenovo_battery_reset_vars(void)
{      g_battery_charging_time = 0;
        g_battery_wake_count = 0;
	 g_temp_protect_cnt = 0;
	 g_charger_1st_in_flag = KAL_TRUE;
	 lenovo_temp_state = LENOVO_TEMP_POS_10_TO_POS_45; 
	 lenovo_chg_state = CHARGING_STATE_PROTECT;
	 g_battery_notify &= BATTERY_TEMP_NORMAL_MASK;
	 g_battery_chg_current = 0;
}
unsigned int lenvovo_battery_notify_check(void)
{     unsigned int notify_code = 0x0000;
	switch (lenovo_temp_state){
             case LENOVO_TEMP_BELOW_0:
			notify_code |=  BATTERY_TEMP_LOW_STOP_CHARGING;
			break;
		case LENOVO_TEMP_POS_0_TO_POS_10:
			//notify_code |= BATTERY_TEMP_LOW_SLOW_CHARGING;
			break;			
		case LENOVO_TEMP_POS_10_TO_POS_45:
			notify_code = 0x0000;
			break;
		case LENOVO_TEMP_POS_45_TO_POS_50:
			//notify_code |=  BATTERY_TEMP_HIGH_SLOW_CHARGING;
			break;
		case LENOVO_TEMP_ABOVE_POS_50:
		      notify_code |=  BATTERY_TEMP_HIGH_STOP_CHARGING;
		      break;
		default:	
			break;
			}    
	return notify_code;
}

kal_bool lenovo_battery_is_valid_cc(int temp)
{    // (15,45]
      if ((temp > LENOVO_TEMP_POS_15_THRESHOLD) && (temp <= LENOVO_TEMP_POS_45_THRESHOLD))
	  	return KAL_TRUE;
	return KAL_FALSE;
}

kal_bool lenovo_battery_limit_to_cc(int temp)
{    // [16, 43]
      if ((temp >= LENOVO_TEMP_POS_15_UP) && (temp <= LENOVO_TEMP_POS_45_DOWN))
	  	return KAL_TRUE;
	return KAL_FALSE;
}

kal_bool lenovo_battery_is_valid_limit(int temp)
{
      if ((temp > LENOVO_TEMP_POS_0_THRESHOLD) && (temp <= LENOVO_TEMP_POS_15_THRESHOLD))
	  	return KAL_TRUE;  // (0, 15 ]
      if ((temp > LENOVO_TEMP_POS_45_THRESHOLD) && (temp <= LENOVO_TEMP_POS_50_THRESHOLD))
	  	return KAL_TRUE;  // (45, 50 ]  	
	return KAL_FALSE;
}

kal_bool lenovo_battery_cc_to_limit(int temp)
{
      if ((temp > LENOVO_TEMP_POS_0_THRESHOLD) && (temp <= LENOVO_TEMP_POS_15_DOWN))
	  	return KAL_TRUE;  // (0, 13]
      if ((temp >= LENOVO_TEMP_POS_45_UP) && (temp <= LENOVO_TEMP_POS_50_THRESHOLD))
	  	return KAL_TRUE;  // [46, 50 ]  	
	return KAL_FALSE;
}

kal_bool lenovo_battery_error_to_limit(int temp)
{    
      if ((temp >= LENOVO_TEMP_POS_0_UP) && (temp <= LENOVO_TEMP_POS_50_DOWN))
	  	return KAL_TRUE;   //  [2, 48]
	return KAL_FALSE;
}

kal_bool lenovo_battery_is_neg10_low_temp(int temp)
{    
      if (temp <= LENOVO_TEMP_NEG_10_THRESHOLD)
	  	return KAL_TRUE;   //  <= -10
	return KAL_FALSE;
}
kal_bool lenovo_battery_is_0_low_temp(int temp)
{    
      if (temp <= LENOVO_TEMP_POS_0_THRESHOLD)
	  	return KAL_TRUE;   //  <= 0
	return KAL_FALSE;
}

kal_bool lenovo_battery_is_50_high_temp(int temp)
{    
      if (temp > LENOVO_TEMP_POS_50_THRESHOLD)
	  	return KAL_TRUE;   //  >50
	return KAL_FALSE;
}

kal_bool lenovo_battery_is_0_50_normal_temp(int temp)
{    
      if ((temp > LENOVO_TEMP_POS_0_THRESHOLD) && (temp <= LENOVO_TEMP_POS_50_THRESHOLD))
	  	return KAL_TRUE;   //  (0,50]
	return KAL_FALSE;
}

kal_bool lenovo_battery_is_2_48_normal_temp(int temp)
{    
      if ((temp > LENOVO_TEMP_POS_0_UP) && (temp <= LENOVO_TEMP_POS_50_DOWN))
	  	return KAL_TRUE;   //  [2,48]
	return KAL_FALSE;
}


void lenovo_battery_limit_c_temp_state(int temp)
{
     if ((temp > LENOVO_TEMP_POS_0_THRESHOLD) && (temp <= LENOVO_TEMP_POS_15_THRESHOLD))
	  	lenovo_temp_state = LENOVO_TEMP_POS_0_TO_POS_10;  // (0, 15 ]
      if ((temp > LENOVO_TEMP_POS_45_THRESHOLD) && (temp <= LENOVO_TEMP_POS_50_THRESHOLD))
	  	lenovo_temp_state = LENOVO_TEMP_POS_45_TO_POS_50;  // (45, 50 ] 	 
}

void lenovo_battery_charing_protect(int temp)
{     static kal_bool has_been_neg10 = KAL_FALSE;

       g_temp_protect_cnt++;
       if (KAL_TRUE == lenovo_battery_is_0_50_normal_temp(temp)){
       	    if (KAL_TRUE == lenovo_battery_is_valid_cc(temp)){
				lenovo_chg_state = CHARGING_STATE_CHARGING;
				lenovo_temp_state = LENOVO_TEMP_POS_10_TO_POS_45;
       	    	} else{   
				lenovo_chg_state = CHARGING_STATE_LIMIT_CHARGING;
				 lenovo_battery_limit_c_temp_state(temp);
		    	}				
	       }
	 else if  (KAL_TRUE == lenovo_battery_is_neg10_low_temp(temp)){
	 	has_been_neg10 = KAL_TRUE;
	 	if (g_temp_protect_cnt > LENOVO_TEMP_NEG_10_COUNT){
                   lenovo_chg_state = CHARGING_STATE_ERROR;
	             lenovo_temp_state = LENOVO_TEMP_BELOW_0;
		}
	 } else if  (KAL_TRUE == lenovo_battery_is_0_low_temp(temp)){
	      if (has_been_neg10 == KAL_TRUE){
			has_been_neg10 = KAL_FALSE;
			g_temp_protect_cnt = 0;
		  }
             if (g_temp_protect_cnt > LENOVO_TEMP_NEG_0_COUNT){
                   lenovo_chg_state = CHARGING_STATE_ERROR;
	             lenovo_temp_state = LENOVO_TEMP_BELOW_0;
		}
	 }else if  (KAL_TRUE == lenovo_battery_is_50_high_temp(temp)){
             if (g_temp_protect_cnt > LENOVO_TEMP_POS_50_COUNT){
                   lenovo_chg_state = CHARGING_STATE_ERROR;
	             lenovo_temp_state = LENOVO_TEMP_ABOVE_POS_50;
		}
	 }

	 if (lenovo_chg_state != CHARGING_STATE_PROTECT)
	 	g_temp_protect_cnt = 0;
	 
}
void lenovo_battery_charging(int temp)
{
      if (KAL_TRUE == lenovo_battery_is_0_50_normal_temp(temp)){
	  	  if (lenovo_chg_state == CHARGING_STATE_CHARGING){
		  	if (KAL_TRUE == lenovo_battery_cc_to_limit(temp))
				  g_temp_to_limit_c_cnt++;
			else  g_temp_to_limit_c_cnt = 0;
			if (g_temp_to_limit_c_cnt > LENOVO_TEMP_LIMIT_C_DEBOUNCE_COUNT){
                            lenovo_chg_state = CHARGING_STATE_LIMIT_CHARGING;
				  lenovo_battery_limit_c_temp_state(temp);			
				  g_temp_to_limit_c_cnt = 0; 			
				}
		  }else{
		  	if (KAL_TRUE == lenovo_battery_limit_to_cc(temp))
				   g_temp_to_cc_cnt++;
			else    g_temp_to_cc_cnt = 0;
			if (g_temp_to_cc_cnt > LENOVO_TEMP_CC_DEBOUNCE_COUNT){
                            lenovo_chg_state = CHARGING_STATE_CHARGING;
				  lenovo_temp_state = LENOVO_TEMP_POS_10_TO_POS_45; 			
				  g_temp_to_cc_cnt = 0; 			
				}
		  }
		  g_temp_err_report_cnt = 0;	 

	  } else{  //temp error
	         g_temp_err_report_cnt++;
               if (g_temp_err_report_cnt > LENOVO_TEMP_ERROR_REPORT_COUNT){
			     g_temp_err_report_cnt = 0;
			     lenovo_chg_state = CHARGING_STATE_ERROR; 	 
                        if (KAL_TRUE == lenovo_battery_is_0_low_temp(temp)){                             
	                        lenovo_temp_state = LENOVO_TEMP_BELOW_0;			
				}else{
	                       lenovo_temp_state = LENOVO_TEMP_ABOVE_POS_50;
				}
				lenovo_bat_printk(LENOVO_BAT_LOG_CRTI, "error happens, temp:%d \n ", temp);				
			  }
                g_temp_to_cc_cnt= 0;
                g_temp_to_limit_c_cnt= 0;		   	   
	  }
}
void lenovo_battery_charging_error(int temp)
{
      if (KAL_TRUE == lenovo_battery_is_2_48_normal_temp(temp))
	  	   g_temp_err_resume_cnt++;
	else   g_temp_err_resume_cnt = 0;
	
	if (g_temp_err_resume_cnt > LENOVO_TEMP_ERROR_RESUME_COUNT){
                g_temp_err_resume_cnt = 0;
       	    if (KAL_TRUE == lenovo_battery_is_valid_cc(temp)){
				lenovo_chg_state = CHARGING_STATE_CHARGING;
				lenovo_temp_state = LENOVO_TEMP_POS_10_TO_POS_45;
       	    	}
		    else{   
				lenovo_chg_state = CHARGING_STATE_LIMIT_CHARGING;
				 lenovo_battery_limit_c_temp_state(temp);
		    	}
		   //reset mtk charging state
		   lenovo_battery_resume_charging_state();
		   lenovo_battery_enable_charging();
	       }
}


void lenovo_battery_charger_in(void)
{    int temp = g_temp_value;
      lenvovo_battery_wake_bat_thread();
	lenovo_battery_backup_charging_state();  

	if (g_charger_1st_in_flag == KAL_TRUE){
          g_charger_1st_in_flag = KAL_FALSE;
	    lenovo_chg_state = CHARGING_STATE_PROTECT;
	    lenovo_battery_set_Qmax_cali_status(1);	
	}

	switch (lenovo_chg_state){
             case CHARGING_STATE_PROTECT:
			lenovo_battery_charing_protect(temp); 	
			break;
		case CHARGING_STATE_LIMIT_CHARGING:
		case CHARGING_STATE_CHARGING:
			lenovo_battery_charging(temp);
			break;
		case CHARGING_STATE_ERROR:
			lenovo_battery_charging_error(temp);
			break;
		default:	
			break;
			}

}

/*lenovo-sw weiweij added for led-soc sync*/
//#ifdef LENOVO_CHARGING_STANDARD
#if 1
extern PMU_ChargerStruct BMT_status;

/* lenovo-sw mahj2 use pmic to control charging led Begin*/
#ifdef  LENOVO_USE_PMIC_CONTROL_LED
extern void mt_pmic_set_charging_led(int on);
static  int  charging_led_flag = 0;
static int   charging_led_state = 0;
static int   charging_led_in_factory_mode = 0;
#endif
/* lenovo-sw mahj2 use pmic to control charging led End */
   
/*Begin, lenovo-sw chailu1 add for 45-50  CV limit  20150318 */
#ifdef LENOVO_TEMP_POS_45_TO_POS_50_CV_LiMIT_SUPPORT
kal_bool lenovo_battery_is_temp_45_to_pos_50(void)
{   
	if(lenovo_temp_state == LENOVO_TEMP_POS_45_TO_POS_50)
	    return KAL_TRUE;
	else
		return KAL_FALSE;
}
#endif
/*End, lenovo-sw chailu1 add for 45-50  CV limit   20150318 */

/* lenovo-sw zhangrc2 use pmic control charging led 2014-12-08 */
#if defined (LENOVO_USE_PMIC_CONTROL_LED )
 void lenovo_battery_charging_set_led_state(void) 
{
	if(charging_led_in_factory_mode != 1 )
	{
		if(BMT_status.UI_SOC2==100)
		{
			mt_pmic_set_charging_led(0);   
		}
		else
		{	
			 if(( BMT_status.charger_exist == KAL_TRUE ) && (charging_led_flag == 0) 
			 	&& (BMT_status.bat_charging_state != CHR_ERROR) 
			 	&&   (lenovo_chg_state != CHARGING_STATE_ERROR))
			{
				mt_pmic_set_charging_led(1);
			}
			else
			{
				mt_pmic_set_charging_led(0);
			}
		}
	}
}  
/* lenovo-sw zhangrc2 use pmic control charging led 2014-12-08 */
#else
void lenovo_battery_charging_set_led_state(void)
{

}
#endif
#endif
/*lenovo-sw weiweij added for led-soc sync end*/

void lenovo_battery_charger_out(void)
{
      lenovo_battery_reset_vars();
	lenovo_battery_set_Qmax_cali_status(0);  
}

void lenovo_battery_debug_print(void)
{     g_battery_charging_time += LENOVO_CHARGING_THREAD_PERIOD; 
       lenovo_bat_printk(LENOVO_BAT_LOG_CRTI, "I:%d, Vbat:%d, Temp:%d, chg_state:%d, temp_state;%d \n ", 
	   	g_battery_chg_current, g_battery_vol, g_temp_value, lenovo_chg_state, lenovo_temp_state);
	 if (g_bat_charging_state == CHR_CC)
	 	lenovo_bat_printk(LENOVO_BAT_LOG_CRTI, "time:%d mins, state: CHR_CC \n ", g_battery_charging_time/60);
	 if (g_bat_charging_state == CHR_BATFULL)
	 	lenovo_bat_printk(LENOVO_BAT_LOG_CRTI, "time:%d mins, state: CHR_BATFULL \n ", g_battery_charging_time/60);
}

//lenovo-sw mahj2 modify for battery charging temp notify at 20141212 Begin
void lenovo_battery_notify_to_health(unsigned int notify, unsigned int *BAT_HEALTH)
{
	unsigned int lenovo_battery_temp_notify = notify & 0xF000;
	if(lenovo_battery_temp_notify == BATTERY_TEMP_LOW_STOP_CHARGING)
	{
		*BAT_HEALTH = POWER_SUPPLY_HEALTH_COLD;
	}
	else if(lenovo_battery_temp_notify == BATTERY_TEMP_HIGH_STOP_CHARGING)
	{
		*BAT_HEALTH = POWER_SUPPLY_HEALTH_OVERHEAT;
	}
	else if(notify&0x0001)
	{
		*BAT_HEALTH = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
	}
}
//lenovo-sw mahj2 modify for battery charging temp notify at 20141212 End

//mtk dependency : BMT_status.charger_exis
kal_bool lenovo_battery_charging_thread(unsigned int *notify, unsigned int g_BN_TestMode)
{ 
    lenovo_battery_get_data(BMT_status.charger_exist);
	
    if( BMT_status.charger_exist == KAL_TRUE ){
	    if (lenovo_battery_is_debug_discharging() == KAL_TRUE) {	
			lenovo_battery_disable_charging();
			return KAL_FALSE;
	    	}
          lenovo_battery_charger_in();  

	   *notify &= BATTERY_TEMP_NORMAL_MASK;	    	
	   if (g_BN_TestMode == 0x0000) {	/* for normal case */
        	 *notify |= lenvovo_battery_notify_check();
	   }
	   else /* for UI test */
   	   {
   	   	if(g_BN_TestMode == 0x0006)
   	   	{
   	   		*notify |= BATTERY_TEMP_LOW_STOP_CHARGING;
   	   	}
		else if(g_BN_TestMode == 0x0007)
		{
			*notify |= BATTERY_TEMP_HIGH_STOP_CHARGING;
		}
   	   }
	  
	    g_battery_notify = *notify; //update the notify value	 
#if 0
	   lenovo_battery_charging_set_led_state();   //added for led-soc sync
#endif
          lenovo_battery_debug_print(); 		   
    }else{
           if (g_charger_in_flag == KAL_TRUE)
                      lenovo_battery_charger_out();
    	}  
    g_charger_in_flag = BMT_status.charger_exist;

/*lenovo-sw weiweij added for led-soc sync*/
//#ifdef LENOVO_CHARGING_STANDARD	
#if 0
	lenovo_battery_charging_set_led_state();
#endif
/*lenovo-sw weiweij added for led-soc sync*/

    return KAL_TRUE;
}


///////////////////////////////////////////////////////////////////////
                   /////Lenovo charging sys nodes//// 
//////////////////////////////////////////////////////////////////////

/// battery calibration status (start)
int battery_cali_start_status = 0;  //0: no Qmax cali; 1: on going Qmax cali (gFG_DOD0 > 85); 2: Qmax cali done
void lenovo_battery_set_Qmax_cali_status(int status)
{   
/*Begin,Lenovo-sw chailu1 modify 2014-5-16, support TI's FG fuction*/	
#if defined(SOC_BY_3RD_FG)
#else
          if (status == 1){
		  if (lenovo_battery_is_deep_charging())		  	
		  	battery_cali_start_status = status; 
          	}
	    else	  
                    battery_cali_start_status = status; 

           lenovo_bat_printk(LENOVO_BAT_LOG_CRTI, "batt_calistatus:%d \n ", battery_cali_start_status);		
#endif	
/*End,Lenovo-sw chailu1 modify 2014-5-16, support TI's FG fuction*/	
}

static ssize_t batt_show_calistatus(struct device* dev,
				struct device_attribute *attr, char* buf)
{
    return sprintf(buf, "%d\n", battery_cali_start_status);
}
static DEVICE_ATTR(batt_calistatus, S_IRUGO, batt_show_calistatus, NULL);
/// battery calibration status (end)


/// battery show charging current (start)
 static ssize_t chg_show_i_current(struct device* dev,
				struct device_attribute *attr, char* buf)
{
			     	
if( BMT_status.charger_exist == KAL_TRUE )	
  g_battery_chg_current = battery_meter_get_charging_current(); 	
else	
  g_battery_chg_current = 0; 	
	

   // xlog_printk(ANDROID_LOG_DEBUG, "Power/Battery", " lenovo_chg_show_i_current : %d\n", g_battery_chg_current);
    return sprintf(buf, "%d\n", g_battery_chg_current);
}
static DEVICE_ATTR(chg_current, S_IRUGO, chg_show_i_current, NULL);
/// battery show charging current (start)

//start
 static ssize_t chg_show_debug_value(struct device* dev,
				struct device_attribute *attr, char* buf)
{
    return sprintf(buf, "%d\n", g_battery_debug_value);
}

static ssize_t chg_store_debug_value(struct device *pdev, struct device_attribute *attr,
			    const char *buf, size_t size)
{
        sscanf(buf, "%d", &g_battery_debug_value);	
        return size; 
} 
static DEVICE_ATTR(debug_value, S_IRUGO|S_IWUSR, chg_show_debug_value, chg_store_debug_value);
//end

//start
 static ssize_t chg_show_debug_temp(struct device* dev,
				struct device_attribute *attr, char* buf)
{
    return sprintf(buf, "%d\n", g_battery_debug_temp);
}

static ssize_t chg_store_debug_temp(struct device *pdev, struct device_attribute *attr,
			    const char *buf, size_t size)
{
        sscanf(buf, "%d", &g_battery_debug_temp);	
        return size; 
} 
static DEVICE_ATTR(debug_temp, S_IRUGO|S_IWUSR, chg_show_debug_temp, chg_store_debug_temp);
//end

//start
 static ssize_t chg_show_notify_value(struct device* dev,
				struct device_attribute *attr, char* buf)
{
    return sprintf(buf, "%u\n", g_battery_notify);
}
static DEVICE_ATTR(notify_value, S_IRUGO, chg_show_notify_value, NULL);
//end
/*lenovo-sw zhangrc2 add function for reboot machine by tp gesture  2014-05-01 begin*/
#ifdef CONFIG_LENOVO_POWEROFF_CHARGING_UI	
int g_tp_poweron = 0;
int ipo_flag = 0;
extern int tp_button_flag;//chailu1 add 20140317 for show ui in bat overdischager status
extern void arch_reset(char mode, const char *cmd);
//start

 static ssize_t chg_show_tp_poweron(struct device* dev,
				struct device_attribute *attr, char* buf)
{
    return sprintf(buf, "%d\n", g_tp_poweron);
}

/*Begin lenovo-sw chailu1 add 20140317 for show ui in bat overdischager status */
#if defined(LENOVO_CHARGING_IMMEDIATELY_POWERON_SUPPORT) 
static ssize_t chg_store_tp_poweron(struct device *pdev, struct device_attribute *attr,
			    const char *buf, size_t size)
{     	
    int cmd;

     if(buf != NULL && size != 0)
    {	
        if (1 == sscanf(buf, "%d", &cmd))
        {
            if(1 == cmd)
            {
                 printk( "chg_store_tp_poweron: cmd = 1 \n");  
	          tp_button_flag = 0;		 
	     }
	     else if(2 == cmd)	
	    {
                 printk( "chg_store_tp_poweron: cmd =2 \n");  
                 g_tp_poweron = 0;
	          tp_button_flag = 0;		 
	    }
			
        }
        else
        {
            printk( "chg_store_tp_poweron error !\n");    
        }
    }
	 
    return size;
} 

static DEVICE_ATTR(tp_poweron, S_IRUGO|S_IWUSR, chg_show_tp_poweron, chg_store_tp_poweron);
#else
static DEVICE_ATTR(tp_poweron, S_IRUGO|S_IWUSR, chg_show_tp_poweron, NULL);
#endif
/*End lenovo-sw chailu1 add 20140317 for show ui in bat overdischager status */


 static ssize_t chg_show_ipo_is_enable(struct device* dev,
				struct device_attribute *attr, char* buf)
{
    return sprintf(buf, "%d\n", ipo_flag);
}

static ssize_t chg_store_ipo_is_enable(struct device *pdev, struct device_attribute *attr,
			    const char *buf, size_t size)
{     	
        sscanf(buf, "%d", &ipo_flag);	
        printk("zrc now the ipo_flag is %d\n",ipo_flag);
	 if (ipo_flag == 0)  {
	 	g_tp_poweron = 0;
	 }	
        return size; 
} 
static DEVICE_ATTR(ipo_is_enable, S_IRUGO|S_IWUSR, chg_show_ipo_is_enable, chg_store_ipo_is_enable);

 static ssize_t chg_store_tp_poweron_action(struct device* pdev,
			struct device_attribute *attr, char* buf,size_t size)
{
       printk("shone\n");
	 arch_reset(0,NULL);
//	 return size; 
}
static DEVICE_ATTR(tp_poweron_action, S_IRUGO|S_IWUSR, NULL, chg_store_tp_poweron_action);
#endif
/*lenovo-sw zhangrc2 add function for reboot machine by tp gesture  2014-05-01 end*/

/* lenovo-sw zhangrc2 use pmic to control charging led 2014-12-08 */
#ifdef  LENOVO_USE_PMIC_CONTROL_LED
static ssize_t chg_show_charging_led_factory_mode(struct device* dev,
				struct device_attribute *attr, char* buf)
{
    return sprintf(buf, "%d\n", charging_led_in_factory_mode);
}

static ssize_t chg_store_charging_led_factory_mode(struct device *pdev, struct device_attribute *attr,
			    const char *buf, size_t size)
{
	sscanf(buf, "%d", &charging_led_in_factory_mode);      //1: factory mode  0: not factory mode
	lenovo_bat_printk(LENOVO_BAT_LOG_CRTI, "show charging_led_in_factory_mode is :%d \n ",  charging_led_in_factory_mode);
	return size;
}
static DEVICE_ATTR(charging_led_in_factory_mode, S_IRUGO|S_IWUSR, chg_show_charging_led_factory_mode, chg_store_charging_led_factory_mode);

static ssize_t chg_store_charging_led_state(struct device *pdev, struct device_attribute *attr,
			    const char *buf, size_t size)
{
	sscanf(buf, "%d", &charging_led_state);	      //1:on  0:off
	lenovo_bat_printk(LENOVO_BAT_LOG_CRTI, "show charging_led_state is :%d \n ",  charging_led_state);	
	if(charging_led_in_factory_mode == 1)
	{
		if(charging_led_state == 1)
		{
			mt_pmic_set_charging_led(1);
		}
		else
		{
			mt_pmic_set_charging_led(0);
		}
	}
	return size;
}
static DEVICE_ATTR(charging_led_state, S_IWUSR, NULL, chg_store_charging_led_state);

 static ssize_t chg_show_charging_led_flag(struct device* dev,
				struct device_attribute *attr, char* buf)
{
    return sprintf(buf, "%d\n", charging_led_flag);
}

static ssize_t chg_store_charging_led_flag(struct device *pdev, struct device_attribute *attr,
			    const char *buf, size_t size)
{     	
        sscanf(buf, "%d", &charging_led_flag);	      
	lenovo_bat_printk(LENOVO_BAT_LOG_CRTI, "show charging_led_flag is :%d \n ",  charging_led_flag);	
	if(charging_led_in_factory_mode != 1)
	{
		if (charging_led_flag == 1) {
			mt_pmic_set_charging_led(0);
		 } else  if ((charging_led_flag == 0) && (BMT_status.charger_exist == KAL_TRUE ) 
		  	     && (BMT_status.UI_SOC2 !=100)  && (BMT_status.bat_charging_state != CHR_ERROR)
		  	    &&  (lenovo_chg_state != CHARGING_STATE_ERROR) ) {
		            mt_pmic_set_charging_led(1);
		 } else {
			mt_pmic_set_charging_led(0);
		 }
	}
        return size; 
} 
static DEVICE_ATTR(charging_led_flag, S_IRUGO|S_IWUSR, chg_show_charging_led_flag, chg_store_charging_led_flag);
#endif
/* lenovo-sw zhangrc2 use pmic to control charging led 2014-12-08 */

//lenovo-sw mahj2 add for show charging ic id Begin
#ifdef LENOVO_SHOW_CHARGING_IC_ID
extern int is_bq24296_exist(void);
static ssize_t chg_show_charging_ic_id(struct device* dev,
				struct device_attribute *attr, char* buf)
{
	int charging_ic_id = 0;
	if(is_bq24296_exist())
	{
		charging_ic_id = 0x6b;
	}
	else
	{
		charging_ic_id = 0;
	}
    return sprintf(buf, "0x%x\n", charging_ic_id);
}
static DEVICE_ATTR(charging_ic_id, S_IRUGO, chg_show_charging_ic_id, NULL);
#endif
//lenovo-sw mahj2 add for show charging ic id End
int lenovo_battery_create_sys_file(struct device *dev)
{     
	int ret;
	
	ret = device_create_file(dev, &dev_attr_chg_current);
	if (ret)
	{
		return ret;
	}
	ret = device_create_file(dev, &dev_attr_batt_calistatus);
	if (ret)
	{
		return ret;
	}
	ret = device_create_file(dev, &dev_attr_debug_value);
	if (ret)
	{
		return ret;
	}
	ret = device_create_file(dev, &dev_attr_debug_temp);
	if (ret)
	{
		return ret;
	} 
	ret = device_create_file(dev, &dev_attr_notify_value);
	if (ret)
	{
		return ret;
	}
/*lenovo-sw zhangrc2 add function for reboot machine by tp gesture  2014-05-01 begin*/
#ifdef CONFIG_LENOVO_POWEROFF_CHARGING_UI

	ret = device_create_file(dev, &dev_attr_ipo_is_enable);
	if (ret)
	{
		return ret;
	}	   
	ret = device_create_file(dev, &dev_attr_tp_poweron);
	if (ret)
	{
		return ret;
	}

	ret = device_create_file(dev, &dev_attr_tp_poweron_action);
   	if (ret)
	{
		return ret;
	}
#endif
/*lenovo-sw zhangrc2 add function for reboot machine by tp gesture  2014-05-01 begin*/
/* lenovo-sw zhangrc2 use pmic to control charging led 2014-12-08 */
#ifdef  LENOVO_USE_PMIC_CONTROL_LED  
	ret = device_create_file(dev, &dev_attr_charging_led_in_factory_mode);
	if (ret)
	{
		return ret;
	}
	
	ret = device_create_file(dev, &dev_attr_charging_led_state);
	if (ret)
	{
		return ret;
	}
	
	ret = device_create_file(dev, &dev_attr_charging_led_flag);
   	if (ret)
	{
		return ret;
	}
#endif
/* lenovo-sw zhangrc2 use pmic to control charging led 2014-12-08 */
//lenovo-sw mahj2 add for show charging ic id Begin
#ifdef LENOVO_SHOW_CHARGING_IC_ID
	ret = device_create_file(dev, &dev_attr_charging_ic_id);
	if (ret)
	{
		return ret;
	}
#endif
//lenovo-sw mahj2 add for show charging ic id End
	return 0;
}

/////////////////////////////////////////////////////////
                 // Lenovo misc fuctions
/////////////////////////////////////////////////////////
