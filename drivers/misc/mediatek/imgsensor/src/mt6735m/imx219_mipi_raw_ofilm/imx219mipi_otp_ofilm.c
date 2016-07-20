#include "imx219mipi_otp_ofilm.h"

extern UINT32 temp_CalGain;
extern UINT32 temp_FacGain; 
extern UINT16 temp_AFInf;
extern UINT16 temp_AFMacro;
extern UINT16 temp_ModuleID;

/****************add OTP Ofilm*********************/
static kal_uint16 read_cmos_sensor(kal_uint32 addr)
{
	kal_uint16 get_byte=0;

	char pu_send_cmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
	iReadRegI2C(pu_send_cmd, 2, (u8*)&get_byte, 1, SLAVE_ID_OFILM);

	return get_byte;
}

static void write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
	char pu_send_cmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};
	iWriteRegI2C(pu_send_cmd, 3, SLAVE_ID_OFILM);
}

static void IMX219_readpage(kal_uint16 page, UBYTE *buf)
{
   //kal_uint16 cnt = 0;
   USHORT i = 0x3204;

   if(page<0 || page>11)
   		return;
   if(buf==NULL)
   		return;

   write_cmos_sensor(0x0100,0x00);//stream on

   //12MHZ target 25us
   write_cmos_sensor(0x3302,0x01);
   write_cmos_sensor(0x3303,0x2c);

   write_cmos_sensor(0x012A,0x0c);
   write_cmos_sensor(0x012B,0x00);
   /*
   //24MHZ,target 25us
   write_cmos_sensor(0x3302,0x02);
   write_cmos_sensor(0x3303,0x58);

   write_cmos_sensor(0x012A,0x18);
   write_cmos_sensor(0x012B,0x00);
   */
   //set ECC here OFF
  // if(page == 1||page == 0)
   write_cmos_sensor(0x3300,0x08);
   
   //write_cmos_sensor(0x3300,0x00);
   //set Read mode
   write_cmos_sensor(0x3200,0x01);
   //check read status OK?
#if 0
   cnt = 0;
   while((read_cmos_sensor(0x3201)&0x01)==0)
   	{
   	cnt++;
	msleep(10);
	if(cnt==100) 
		break;
   	}
#endif
   
   //set page
   write_cmos_sensor(0x3202,page);

   //read OTP buffer to sensor
   for(i=0x3204;i<=0x3243;i++)
   {
   		buf[i-0x3204] = (UBYTE)read_cmos_sensor(i);
   }

   //check read status OK?
#if 0
   cnt = 0;
    while((read_cmos_sensor(0x3201)&0x01)==0)
   	{
   	cnt++;
	msleep(10);
	if(cnt==100) 
		break;
   	}
#endif
   write_cmos_sensor(0x0100,0x01);//stream off   

}

static void IMX219_readAFpage(kal_uint16 page, UBYTE *buf)
{

   USHORT i = 0x321D;

   if(page<0 || page>11)
   		return;
   if(buf==NULL)
   		return;

   write_cmos_sensor(0x0100,0x00);

   //12MHZ target 25us
   write_cmos_sensor(0x3302,0x01);
   write_cmos_sensor(0x3303,0x2c);

   write_cmos_sensor(0x012A,0x0c);
   write_cmos_sensor(0x012B,0x00);
   /*
   //24MHZ,target 25us
   write_cmos_sensor(0x3302,0x02);
   write_cmos_sensor(0x3303,0x58);

   write_cmos_sensor(0x012A,0x18);
   write_cmos_sensor(0x012B,0x00);
   */
   //set ECC here OFF
  // if(page == 1||page == 0)
   write_cmos_sensor(0x3300,0x08);
   
   //write_cmos_sensor(0x3300,0x00);
   //set Read mode
   write_cmos_sensor(0x3200,0x01);

   //set page
   write_cmos_sensor(0x3202,page);

   //read OTP buffer to sensor
   for(i=0x321D;i<=0x322B;i++)
   {
   		buf[i-0x321D] = (UBYTE)read_cmos_sensor(i);
   }
   write_cmos_sensor(0x0100,0x01);//stream off    

}
static void	IMX219_readpageLsc(kal_uint16 page, UBYTE *buf)
{
   if(page != 0)
   		return;
   if(buf == NULL)
   		return;

   write_cmos_sensor(0x0100,0x00);

   //12MHZ target 25us
   write_cmos_sensor(0x3302,0x01);
   write_cmos_sensor(0x3303,0x2c);

   write_cmos_sensor(0x012A,0x0c);
   write_cmos_sensor(0x012B,0x00);
   /*
   //24MHZ,target 25us
   write_cmos_sensor(0x3302,0x02);
   write_cmos_sensor(0x3303,0x58);

   write_cmos_sensor(0x012A,0x18);
   write_cmos_sensor(0x012B,0x00);
   */
   //set ECC here OFF
  // if(page == 1||page == 0)
   write_cmos_sensor(0x3300,0x08);
   
   //write_cmos_sensor(0x3300,0x00);
   //set Read mode
   write_cmos_sensor(0x3200,0x01);

   //set page
   write_cmos_sensor(0x3202,page);

   //read OTP buffer to sensor
	buf[0] = (UBYTE)read_cmos_sensor(0x3243);
   write_cmos_sensor(0x0100,0x01);//stream off 
   
}

kal_uint16 get_Ofilm_otp_module_id(void)
{	
	kal_uint16 module_id = 0;
	kal_uint16 calibration_version = 0;
	kal_uint16 production_year = 0;
	kal_uint16 production_month = 0;
	kal_uint16 production_day = 0;
	kal_uint16 lens_id = 0;
	kal_uint16 vcm_id = 0;
	kal_uint16 driveric_id = 0;
	//BasicInfo Flag check
    kal_uint16 Flag = 0;
	kal_uint16 useGroupIndex = 0;
    UBYTE pBasicInfoData[64] = {0};

	IMX219_readpage(0,pBasicInfoData);

	Flag = pBasicInfoData[0];
	if((Flag & 0x30) ==0x10){
		useGroupIndex = 2;
	}else if((Flag & 0xc0) == 0x40)	{
		useGroupIndex = 1;
	}else{
		OTP_LOG2("Ofilm IM219_OTP data Invalid or empty\n");
		return -1;
	}

	OTP_LOG2("IMX219_otp:Module info useGroup=%d",useGroupIndex);
	module_id = pBasicInfoData[1+12*(useGroupIndex-1)];
	OTP_LOG2("Ofilm IM219_OTP module_id = 0x%x\n",module_id);
	calibration_version = pBasicInfoData[2+12*(useGroupIndex-1)];
	OTP_LOG2("Ofilm IM219_OTP calibration_version = 0x%x\n",calibration_version);

	production_year = pBasicInfoData[3+12*(useGroupIndex-1)];
	OTP_LOG2("Ofilm production_year = 0x%x\n",production_year);

	production_month = pBasicInfoData[4+12*(useGroupIndex-1)];
	OTP_LOG2("Ofilm production_month = 0x%x\n",production_month);

	production_day = pBasicInfoData[5+12*(useGroupIndex-1)];
	OTP_LOG2("Ofilm production_day = 0x%x\n",production_day);

	lens_id = pBasicInfoData[6+12*(useGroupIndex-1)];
	OTP_LOG2("Ofilm lens_id = 0x%x\n",lens_id);

	vcm_id = pBasicInfoData[7+12*(useGroupIndex-1)];
	OTP_LOG2("Ofilm vcm_id = 0x%x\n",vcm_id);

	driveric_id = pBasicInfoData[8+12*(useGroupIndex-1)];
	OTP_LOG2("Ofilm IM219_OTP driveric_id = 0x%x\n",driveric_id);
	//add for Ofilm OTP AWB&AF
	temp_ModuleID = module_id;

    return module_id;

}

//ret:0 is OK,-1 is Fail
static unsigned char get_otp_AFData(void)
{
	UBYTE pBasicAFInfoData[15] = {0};
	//Basic AF Info Flag check
    kal_uint16 AF_Flag = 0;
	kal_uint16 afGroupInx = 0;
	kal_uint16 macroH = 0;
	kal_uint16 macroL = 0;
	kal_uint16 infiH = 0;
	kal_uint16 infiL = 0;
	kal_uint16 af_macro,af_infi;
	int ret = 0;

	IMX219_readAFpage(0,pBasicAFInfoData); 
	AF_Flag = pBasicAFInfoData[0];
	printk("IM219_OTP:AF_Flag=0x%x\n",AF_Flag);
	if((AF_Flag&0x30) ==0x10){
		afGroupInx = 2;
	}else if((AF_Flag & 0xc0) == 0x40){
		afGroupInx = 1;
	}else{
		OTP_LOG2("IMX219_otp:AF Info is Invalid or empty\n");
		return -1;
	}

	printk("IMX219_otp:AF useGroup=%d\n",afGroupInx);
	infiH = pBasicAFInfoData[2+7*(afGroupInx-1)];
	infiL = pBasicAFInfoData[3+7*(afGroupInx-1)];
	macroH = pBasicAFInfoData[4+7*(afGroupInx-1)];
	macroL = pBasicAFInfoData[5+7*(afGroupInx-1)];
	af_macro = ((macroH & 0xff)<<2 | (macroL & 0xc0)>>6);
	af_infi = ((infiH & 0xff)<<2 | (infiL & 0xc0)>>6);
	printk("Ofilm IM219_OTP: macroH=0x%x,macroL=0x%x,infiH=0x%x,infiL=0x%x\n",macroH,macroL,infiH,infiL);
	printk("Ofilm IM219_OTP: af_macro=0x%x,af_infi=0x%x\n",af_macro,af_infi);

	temp_AFInf = af_infi-10; //modify by lvxiaoliang for add infi range
	temp_AFMacro = af_macro + ((af_macro-af_infi)/5)+200; //modify by lvxiaoliang for macro 7cm
	printk("Qtech IM219_OTP: temp_AFMacro=0x%x,af_infi=0x%x\n",temp_AFMacro,af_infi);

	return ret;
}

static void IMX219_ApplyLSCAutoLoad(kal_uint16 table)
{
    //enable it
	write_cmos_sensor(0x0190,0x01);
	write_cmos_sensor(0x0192,table);
	write_cmos_sensor(0x0191,0x00);
	write_cmos_sensor(0x0193,0x00);
	write_cmos_sensor(0x01a4,0x03);
}

static unsigned char get_otp_lsc(void)
{
	//LSC flag check
	UBYTE pBasicInfoData[10] = {0};
	kal_uint16 LscFlag = 0;
	kal_uint16 lscTable = 0;

	kal_uint16 nLSCGroup = 0;

#if 0
	UBYTE pLSCInfoData[256] = {0};
	kal_uint16 i;
	kal_uint16 nLSCCheckSum;
	kal_uint16 sum;

#endif  	  
 
	IMX219_readpageLsc(0,pBasicInfoData); 

	LscFlag = pBasicInfoData[0];
	OTP_LOG2("IM219_OTP:LscFlag=0x%x\n",LscFlag);
	if((LscFlag & 0xc0) == 0x40)
	{
		nLSCGroup = 1;
		lscTable = 0;
	#if 0 
		for (i = 2; i < 5; i++)  
		{
			IMX219_readpage(i, pLSCInfoData+ (i-2)*64);
		}
	#endif
	}
	else if((LscFlag & 0x30) == 0x10)
	{
		nLSCGroup = 2;
		lscTable = 2;
	#if 0
		for (i = 7; i < 11; i++)  
		{
			IMX219_readpage(i, pLSCInfoData+ (i-7)*64);
		}
	#endif
	}
	else
	{
		OTP_LOG2("Ofilm IM219_OTP:Lsc data Invalid or empty\n");
		return -1;
	}

	printk("IM219_OTP:nLSCGroup = %d\n",nLSCGroup);
#if 0
	// LSC CheckSum
	sum = 0;
	//kal_uint16 i;
	for(i=0+30*(nLSCGroup-1);i<175+30*(nLSCGroup-1);i++)
	{
		 sum += pLSCInfoData[i];
		 printk("pLSCInfoData[%d] = %d,i = %d\n",i,pLSCInfoData[i],i);
	}
	nLSCCheckSum = sum % 0xff + 1;

	printk("pBasicInfoData[61+(nLSCGroup-1)] = %d\n",pBasicInfoData[61+(nLSCGroup-1)]);
	printk("nLSCCheckSum = %d\n",nLSCCheckSum);
	if(pBasicInfoData[61+(nLSCGroup-1)] != nLSCCheckSum)
	{
		    OTP_LOG2("LSC CheckSum is error");
	}
#endif
	IMX219_ApplyLSCAutoLoad(lscTable);

	return 0;
      
}

/*wangliangfeng 20141118 moidify for read module awb parameter start*/
static unsigned char get_otp_wb(void)
{
	int ret = 0;
	UBYTE pWBInfoData[64] = {0};
	unsigned char temph = 0;
	unsigned char templ = 0;
	unsigned char current_R = 0;
	unsigned char current_B = 0;
	unsigned char current_Gr = 0;
	unsigned char current_Gb = 0;
	unsigned char golden_R = 0;
	unsigned char golden_B = 0;
	unsigned char golden_Gr = 0;
	unsigned char golden_Gb = 0;
	kal_uint16 nWBFlag = 0;		//WB flag check
	kal_uint16 nWBGroup = 0;

	IMX219_readpage(1,pWBInfoData);
	nWBFlag = pWBInfoData[0];
	OTP_LOG2("IM219_OTP:nWBFlag=0x%x\n",nWBFlag);
	if((nWBFlag & 0x30) == 0x10)//group2
	{
		nWBGroup = 2; 
		temph = pWBInfoData[22];
		templ = pWBInfoData[23];
		current_r_Gr = (unsigned short)templ + (((unsigned short)temph)  << 8);
		current_r_Gr = current_r_Gr >> 6;
		OTP_LOG2("IM219_OTP:group2 get_otp_wb() current_r_Gr=0x%x\n",current_r_Gr);

		temph = pWBInfoData[24];
		templ = pWBInfoData[25];
		current_b_Gr = (unsigned short)templ + (((unsigned short)temph)  << 8);
		current_b_Gr = current_b_Gr >> 6;
		OTP_LOG2("---Ofilm group2  current_b_Gr=0x%x\n",current_b_Gr);

		temph = pWBInfoData[26];
		templ = pWBInfoData[27];
		current_Gb_Gr = (unsigned short)templ + (((unsigned short)temph)  << 8);
		current_Gb_Gr = current_Gb_Gr >> 6;
		OTP_LOG2("---Ofilm group2  current_Gb_Gr=0x%x\n",current_Gb_Gr);

		temph = pWBInfoData[28];
		templ = pWBInfoData[29];
		golden_r_Gr= (unsigned short)templ + (((unsigned short)temph)  << 8);
		golden_r_Gr = golden_r_Gr >> 6;
		OTP_LOG2("---Ofilm group2  golden_r_Gr=0x%x\n",golden_r_Gr);

		temph = pWBInfoData[30];
		templ = pWBInfoData[31];
		golden_b_Gr= (unsigned short)templ + (((unsigned short)temph)  << 8);
		golden_b_Gr = golden_b_Gr >> 6;
		OTP_LOG2("---Ofilm group2  golden_b_Gr=0x%x\n",golden_b_Gr);

		temph = pWBInfoData[32];
		templ = pWBInfoData[33];
		golden_Gb_Gr= (unsigned short)templ + (((unsigned short)temph)  << 8);
		golden_Gb_Gr = golden_Gb_Gr >> 6;
		OTP_LOG2("---Ofilm group2  golden_Gb_Gr=0x%x\n",golden_Gb_Gr);
		current_R = pWBInfoData[34];
		current_B = pWBInfoData[35];
		current_Gr = pWBInfoData[36];
		current_Gb = pWBInfoData[37];
		OTP_LOG2("IMX219_OTP:Group2 get_otp_wb:R=0x%x,B=0x%x,Gr=0x%x,Gb=0x%x\n",current_R,current_B,current_Gr,current_Gb);

		golden_R = pWBInfoData[38];
		golden_B = pWBInfoData[39];
		golden_Gr = pWBInfoData[40];
		golden_Gb = pWBInfoData[41];
		OTP_LOG2("IMX219_OTP:Group2 get_otp_wb:Golden_R=0x%x,Golden_B=0x%x,Golden_Gr=0x%x,Golden_Gb=0x%x\n",golden_R,golden_B,golden_Gr,golden_Gb);

	}else if((nWBFlag & 0xC0) == 0x40){	//group1
		nWBGroup = 1;

		temph = pWBInfoData[1];
		templ = pWBInfoData[2];
		current_r_Gr = (unsigned short)templ + (((unsigned short)temph)  << 8);
		current_r_Gr = current_r_Gr >> 6;
		OTP_LOG2("IM219_OTP: group1 current_r_Gr=0x%x\n",current_r_Gr);

		temph = pWBInfoData[3];
		templ = pWBInfoData[4];
		current_b_Gr = (unsigned short)templ + (((unsigned short)temph)  << 8);
		current_b_Gr = current_b_Gr >> 6;
		OTP_LOG2("---Ofilm group1 current_b_Gr=0x%x\n",current_b_Gr);

		temph = pWBInfoData[5];
		templ = pWBInfoData[6];
		current_Gb_Gr = (unsigned short)templ + (((unsigned short)temph)  << 8);
		current_Gb_Gr = current_Gb_Gr >> 6;
		OTP_LOG2("---Ofilm group1 current_Gb_Gr=0x%x\n",current_Gb_Gr);

		temph = pWBInfoData[7];
		templ = pWBInfoData[8];
		golden_r_Gr= (unsigned short)templ + (((unsigned short)temph)  << 8);
		golden_r_Gr = golden_r_Gr >> 6;
		OTP_LOG2("---Ofilm group1 golden_r_Gr=0x%x\n",golden_r_Gr);

		temph = pWBInfoData[9];
		templ = pWBInfoData[10];
		golden_b_Gr= (unsigned short)templ + (((unsigned short)temph)  << 8);
		golden_b_Gr = golden_b_Gr >> 6;
		OTP_LOG2("---Ofilm group1 golden_b_Gr=0x%x\n",golden_b_Gr);

		temph = pWBInfoData[11];
		templ = pWBInfoData[12];
		golden_Gb_Gr= (unsigned short)templ + (((unsigned short)temph)  << 8);
		golden_Gb_Gr = golden_Gb_Gr >> 6;
		OTP_LOG2("---Ofilm group1 golden_Gb_Gr=0x%x\n",golden_Gb_Gr);
		current_R = pWBInfoData[13];
		current_B = pWBInfoData[14];
		current_Gr = pWBInfoData[15];
		current_Gb = pWBInfoData[16];
		OTP_LOG2("IMX219_OTP:Group1 get_otp_wb:R=0x%x,B=0x%x,Gr=0x%x,Gb=0x%x\n",current_R,current_B,current_Gr,current_Gb);

		golden_R = pWBInfoData[17];
		golden_B = pWBInfoData[18];
		golden_Gr = pWBInfoData[19];
		golden_Gb = pWBInfoData[20];
		OTP_LOG2("IMX219_OTP:Group1 get_otp_wb:Golden_R=0x%x,Golden_B=0x%x,Golden_Gr=0x%x,Golden_Gb=0x%x\n",golden_R,golden_B,golden_Gr,golden_Gb);

	}else{
		OTP_LOG2("Ofilm IM219_OTP:wb data Invalid or empty\n");
		return -1;
	}
	printk("IM219_OTP:nWBGroup = %d\n",nWBGroup);

	temp_CalGain = (current_R&0x000000ff)|((current_B<<8)&0x0000ff00)|((current_Gr<<16)&0x00ff0000)|((current_Gb<<24)&0xff000000);

	//add by liuzhen to custom Golden awb data
	golden_R = 0x8e;
	golden_B = 0x84;
	golden_Gr = 0xc8;
	golden_Gb = 0xca;
	//add end 

	temp_FacGain = (golden_R&0x000000ff)|((golden_B<<8)&0x0000ff00)|((golden_Gr<<16)&0x00ff0000)|((golden_Gb<<24)&0xff000000);
#if 0
	//WB Data CheckSum
	sum = 0;

	for(i=1+21*(nWBGroup-1);i<21+21*(nWBGroup-1);i++)
	{
	     sum+=pWBInfoData[i];
	}
	nWBCheckSum = (sum % 0xff + 1);
	if(pWBInfoData[21+21*(nWBGroup-1)] != nWBCheckSum)
	{
         OTP_LOG2("WB Data CheckSum is error");
	}
#endif
	return ret;
}

int read_otp_info_ofilm(void)
{
	int ret = 0;

	if(get_otp_AFData() != 0)
	{
		printk("Ofilm:[%s]:IMX219_OTP:Get AF data Err\n",__func__);
		ret = -1;
	}

	if(get_otp_wb() != 0)
	{
		printk("Ofilm:[%s]:IMX219_OTP:Get WB data Err\n",__func__);
		ret = -1;
	}

	return ret;
}

/*wangliangfeng 20141118 moidify for read module awb parameter end.*/
int otp_update_ofilm(void)	//only open Lsc autoload
{
	int ret = 0;

    if(get_otp_lsc() != 0)
    {
		printk("IMX219_OTP:Update LSC Fail\n");
		return -1;
    }
	printk("IMX219_OTP:Update LSC success\n");

	return ret;
}

/****************OTP end Ofilm************************/
