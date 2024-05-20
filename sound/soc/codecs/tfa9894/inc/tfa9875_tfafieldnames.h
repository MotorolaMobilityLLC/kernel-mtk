/* 
 * Copyright 2021 GOODIX 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef _TFA9875_TFAFIELDNAMES_H
#define _TFA9875_TFAFIELDNAMES_H


#define TFA9875_I2CVERSION    26

typedef enum Tfa9875BfEnumList {
    TFA9875_BF_PWDN  = 0x0000,    /*!< Powerdown selection                                */
    TFA9875_BF_I2CR  = 0x0010,    /*!< I2C reset - auto clear                             */
    TFA9875_BF_AMPE  = 0x0030,    /*!< Activate amplifier                                 */
    TFA9875_BF_DCA   = 0x0040,    /*!< Activate DC-to-DC converter                        */
    TFA9875_BF_INTP  = 0x0071,    /*!< Interrupt config                                   */
    TFA9875_BF_BYPUVP= 0x00a0,    /*!< Bypass UVP                                         */
    TFA9875_BF_BYPOCP= 0x00b0,    /*!< Bypass OCP                                         */
    TFA9875_BF_TSTOCP= 0x00c0,    /*!< OCP testing control                                */
    TFA9875_BF_MANWTIME= 0x00d0,    /*!< Manager wait time selection control                */
    TFA9875_BF_ENPLLSYNC= 0x00e0,    /*!< Manager control for enabling synchronisation with PLL FS */
    TFA9875_BF_COORHYS= 0x00f0,    /*!< Select hysteresis for clock range detector         */
    TFA9875_BF_MANSCONF= 0x0120,    /*!< Configuration setting if I2C settings are uploaded by the host */
    TFA9875_BF_MUTETO= 0x0160,    /*!< Time out mute sequence                             */
    TFA9875_BF_MANROBOD= 0x0170,    /*!< Controls the reaction of the device on BOD         */
    TFA9875_BF_BODHYS= 0x0180,    /*!< Enable Hysteresis of BOD                           */
    TFA9875_BF_BODFILT= 0x0191,    /*!< BOD filter                                         */
    TFA9875_BF_BODTHLVL= 0x01b1,    /*!< BOD threshold level                                */
    TFA9875_BF_MANEDCTH= 0x01d0,    /*!< Device response to too high DC level flag (DCTH = 1)  */
    TFA9875_BF_OPENMTP= 0x01e0,    /*!< Control for MTP protection                         */
    TFA9875_BF_AUDFS = 0x0203,    /*!< Sample rate (Fs)                                   */
    TFA9875_BF_ENLBW = 0x0240,    /*!< CS/VS decimator low bandwidth mode control         */
    TFA9875_BF_FRACTDEL= 0x0255,    /*!< V/I Fractional delay                               */
    TFA9875_BF_REV   = 0x030f,    /*!< Device revision information                        */
    TFA9875_BF_REFCKEXT= 0x0400,    /*!< PLL external ref clock                             */
    TFA9875_BF_MANAOOSC= 0x0460,    /*!< Internal oscillator control during power down mode */
    TFA9875_BF_FSSYNCEN= 0x0480,    /*!< Enable FS synchronisation for clock divider        */
    TFA9875_BF_CLKREFSYNCEN= 0x0490,    /*!< Enable PLL reference clock synchronisation for clock divider */
    TFA9875_BF_CGUSYNCDCG= 0x0500,    /*!< Clock gating control for CGU synchronisation module */
    TFA9875_BF_FRCCLKSPKR= 0x0510,    /*!< Force active the speaker sub-system clock when in idle power */
    TFA9875_BF_BSTCLKLP= 0x0520,    /*!< Boost clock control in low power mode1             */
    TFA9875_BF_SSFAIME= 0x05c0,    /*!< Sub-system FAIM (MTP)                              */
    TFA9875_BF_VDDS  = 0x1000,    /*!< POR (sticky flag, clear on write a '1')            */
    TFA9875_BF_OTDS  = 0x1010,    /*!< OTP alarm (sticky flag,  clear on write a '1')     */
    TFA9875_BF_UVDS  = 0x1020,    /*!< UVP alarm (sticky flag,  clear on write a '1')     */
    TFA9875_BF_OVDS  = 0x1030,    /*!< OVP alarm (sticky flag,  clear on write a '1')     */
    TFA9875_BF_OCDS  = 0x1040,    /*!< OCP amplifier (sticky flag,  clear on write a '1') */
    TFA9875_BF_NOCLK = 0x1050,    /*!< Lost clock (sticky flag,  clear on write a '1')    */
    TFA9875_BF_CLKOOR= 0x1060,    /*!< External clock status (sticky flag,  clear on write a '1') */
    TFA9875_BF_DCOCPOK= 0x1070,    /*!< DCDC OCP nmos (sticky flag,  clear on write a '1') */
    TFA9875_BF_DCIL  = 0x1080,    /*!< DCDC current limiting (sticky flag,  clear on write a '1') */
    TFA9875_BF_DCDCA = 0x1090,    /*!< DCDC active (sticky flag,  clear on write a '1')   */
    TFA9875_BF_ADCCR = 0x10a0,    /*!< ADC ready flag (sticky flag,  clear on write a '1') */
    TFA9875_BF_OCPOAP= 0x10b0,    /*!< OCPOK pmos A (sticky flag,  clear on write a '1')  */
    TFA9875_BF_OCPOAN= 0x10c0,    /*!< OCPOK nmos A (sticky flag,  clear on write a '1')  */
    TFA9875_BF_OCPOBP= 0x10d0,    /*!< OCPOK pmos B (sticky flag,  clear on write a '1')  */
    TFA9875_BF_OCPOBN= 0x10e0,    /*!< OCPOK nmos B (sticky flag,  clear on write a '1')  */
    TFA9875_BF_DCTH  = 0x10f0,    /*!< DC level on audio input stream too high (sticky flag,  clear on write a '1') */
    TFA9875_BF_CLKS  = 0x1100,    /*!< Clocks stable                                      */
    TFA9875_BF_MTPB  = 0x1110,    /*!< MTP busy                                           */
    TFA9875_BF_TDMERR= 0x1120,    /*!< TDM error                                          */
    TFA9875_BF_DCDCPC= 0x1130,    /*!< Indicates current is max in DC-to-DC converter     */
    TFA9875_BF_DCHVBAT= 0x1140,    /*!< DCDC level 1x                                      */
    TFA9875_BF_DCH114= 0x1150,    /*!< DCDC level 1.14x                                   */
    TFA9875_BF_DCH107= 0x1160,    /*!< DCDC level 1.07x                                   */
    TFA9875_BF_PLLS  = 0x1170,    /*!< PLL lock                                           */
    TFA9875_BF_TDMLUTER= 0x1180,    /*!< TDM LUT error                                      */
    TFA9875_BF_SWS   = 0x1190,    /*!< Amplifier engage                                   */
    TFA9875_BF_AMPS  = 0x11a0,    /*!< Amplifier enable                                   */
    TFA9875_BF_AREFS = 0x11b0,    /*!< References enable                                  */
    TFA9875_BF_CLIPS = 0x11c0,    /*!< Amplifier clipping                                 */
    TFA9875_BF_MANSTATE= 0x1203,    /*!< Device manager status                              */
    TFA9875_BF_AMPSTE= 0x1243,    /*!< Amplifier control status                           */
    TFA9875_BF_TDMSTAT= 0x1282,    /*!< TDM status bits                                    */
    TFA9875_BF_DCMODE= 0x12b1,    /*!< DCDC mode status bits                              */
    TFA9875_BF_WAITSYNC= 0x12d0,    /*!< CGU and PLL synchronisation status flag from CGU   */
    TFA9875_BF_BODNOK= 0x1300,    /*!< BOD Flag VDD NOT OK (sticky flag,  clear on write a '1') */
    TFA9875_BF_DCLD  = 0x140c,    /*!< DC level detected by DC protection module (2s complement) */
    TFA9875_BF_BATS  = 0x1509,    /*!< Battery voltage (V)                                */
    TFA9875_BF_TEMPS = 0x1608,    /*!< IC Temperature (C)                                 */
    TFA9875_BF_VDDPS = 0x1709,    /*!< IC VDDP voltage ( 1023*VDDP/13 V)                  */
    TFA9875_BF_TDME  = 0x2000,    /*!< Enable interface                                   */
    TFA9875_BF_AMPINSEL= 0x2011,    /*!< Amplifier input selection                          */
    TFA9875_BF_INPLEV= 0x2030,    /*!< TDM output attenuation                             */
    TFA9875_BF_TDMCLINV= 0x2040,    /*!< Reception data to BCK clock                        */
    TFA9875_BF_TDMFSPOL= 0x2050,    /*!< FS polarity                                        */
    TFA9875_BF_TDMSLOTS= 0x2061,    /*!< N-slots in Frame                                   */
    TFA9875_BF_TDMSLLN= 0x2081,    /*!< N-bits in slot                                     */
    TFA9875_BF_TDMSSIZE= 0x20a1,    /*!< Sample size per slot                               */
    TFA9875_BF_TDMNBCK= 0x20c3,    /*!< N-BCK's in FS                                      */
    TFA9875_BF_TDMDEL= 0x2100,    /*!< Data delay to FS                                   */
    TFA9875_BF_TDMADJ= 0x2110,    /*!< Data adjustment                                    */
    TFA9875_BF_TDMSPKE= 0x2120,    /*!< Control audio TDM channel in 0                     */
    TFA9875_BF_TDMDCE= 0x2130,    /*!< Control audio TDM channel in 1                     */
    TFA9875_BF_TDMSRC0E= 0x2140,    /*!< Enable TDM source0 data channel                    */
    TFA9875_BF_TDMSRC1E= 0x2150,    /*!< Enable TDM source1 data channel                    */
    TFA9875_BF_TDMSRC2E= 0x2160,    /*!< Enable TDM source2 data channel                    */
    TFA9875_BF_TDMSRC3E= 0x2170,    /*!< Enable TDM source3 data channel                    */
    TFA9875_BF_TDMSPKS= 0x2183,    /*!< TDM slot for sink 0                                */
    TFA9875_BF_TDMDCS= 0x21c3,    /*!< TDM slot for sink 1                                */
    TFA9875_BF_TDMSRC0S= 0x2203,    /*!< Slot Position of TDM source0 channel data          */
    TFA9875_BF_TDMSRC1S= 0x2243,    /*!< Slot Position of TDM source1 channel data          */
    TFA9875_BF_TDMSRC2S= 0x2283,    /*!< Slot Position of TDM source2 channel data          */
    TFA9875_BF_TDMSRC3S= 0x22c3,    /*!< Slot Position of TDM source3 channel data          */
    TFA9875_BF_ISTVDDS= 0x4000,    /*!< Interrupt status POR                               */
    TFA9875_BF_ISTBSTOC= 0x4010,    /*!< Interrupt status DCDC OCP alarm                    */
    TFA9875_BF_ISTOTDS= 0x4020,    /*!< Interrupt status OTP alarm                         */
    TFA9875_BF_ISTOCPR= 0x4030,    /*!< Interrupt status OCP alarm                         */
    TFA9875_BF_ISTUVDS= 0x4040,    /*!< Interrupt status UVP alarm                         */
    TFA9875_BF_ISTTDMER= 0x4050,    /*!< Interrupt status TDM error                         */
    TFA9875_BF_ISTNOCLK= 0x4060,    /*!< Interrupt status lost clock                        */
    TFA9875_BF_ISTDCTH= 0x4070,    /*!< Interrupt status dc too high                       */
    TFA9875_BF_ISTBODNOK= 0x4080,    /*!< Interrupt status brown out detected                */
    TFA9875_BF_ISTCOOR= 0x4090,    /*!< Interrupt status clock out of range                */
    TFA9875_BF_ICLVDDS= 0x4400,    /*!< Clear interrupt status POR                         */
    TFA9875_BF_ICLBSTOC= 0x4410,    /*!< Clear interrupt status DCDC OCP                    */
    TFA9875_BF_ICLOTDS= 0x4420,    /*!< Clear interrupt status OTP alarm                   */
    TFA9875_BF_ICLOCPR= 0x4430,    /*!< Clear interrupt status OCP alarm                   */
    TFA9875_BF_ICLUVDS= 0x4440,    /*!< Clear interrupt status UVP alarm                   */
    TFA9875_BF_ICLTDMER= 0x4450,    /*!< Clear interrupt status TDM error                   */
    TFA9875_BF_ICLNOCLK= 0x4460,    /*!< Clear interrupt status lost clk                    */
    TFA9875_BF_ICLDCTH= 0x4470,    /*!< Clear interrupt status dc too high                 */
    TFA9875_BF_ICLBODNOK= 0x4480,    /*!< Clear interrupt status brown out detected          */
    TFA9875_BF_ICLCOOR= 0x4490,    /*!< Clear interrupt status clock out of range          */
    TFA9875_BF_IEVDDS= 0x4800,    /*!< Enable interrupt POR                               */
    TFA9875_BF_IEBSTOC= 0x4810,    /*!< Enable interrupt DCDC OCP                          */
    TFA9875_BF_IEOTDS= 0x4820,    /*!< Enable interrupt OTP alarm                         */
    TFA9875_BF_IEOCPR= 0x4830,    /*!< Enable interrupt OCP alarm                         */
    TFA9875_BF_IEUVDS= 0x4840,    /*!< Enable interrupt UVP alarm                         */
    TFA9875_BF_IETDMER= 0x4850,    /*!< Enable interrupt TDM error                         */
    TFA9875_BF_IENOCLK= 0x4860,    /*!< Enable interrupt lost clk                          */
    TFA9875_BF_IEDCTH= 0x4870,    /*!< Enable interrupt dc too high                       */
    TFA9875_BF_IEBODNOK= 0x4880,    /*!< Enable interrupt brown out detect                  */
    TFA9875_BF_IECOOR= 0x4890,    /*!< Enable interrupt clock out of range                */
    TFA9875_BF_IPOVDDS= 0x4c00,    /*!< Interrupt polarity POR                             */
    TFA9875_BF_IPOBSTOC= 0x4c10,    /*!< Interrupt polarity DCDC OCP                        */
    TFA9875_BF_IPOOTDS= 0x4c20,    /*!< Interrupt polarity OTP alarm                       */
    TFA9875_BF_IPOOCPR= 0x4c30,    /*!< Interrupt polarity OCP alarm                       */
    TFA9875_BF_IPOUVDS= 0x4c40,    /*!< Interrupt polarity UVP alarm                       */
    TFA9875_BF_IPOTDMER= 0x4c50,    /*!< Interrupt polarity TDM error                       */
    TFA9875_BF_IPONOCLK= 0x4c60,    /*!< Interrupt polarity lost clk                        */
    TFA9875_BF_IPODCTH= 0x4c70,    /*!< Interrupt polarity dc too high                     */
    TFA9875_BF_IPOBODNOK= 0x4c80,    /*!< Interrupt polarity brown out detect                */
    TFA9875_BF_IPOCOOR= 0x4c90,    /*!< Interrupt polarity clock out of range              */
    TFA9875_BF_BSSCR = 0x5001,    /*!< Battery safeguard attack time                      */
    TFA9875_BF_BSST  = 0x5023,    /*!< Battery safeguard threshold voltage level          */
    TFA9875_BF_BSSRL = 0x5061,    /*!< Battery safeguard maximum reduction                */
    TFA9875_BF_BSSCLRST= 0x50d0,    /*!< Reset clipper - auto clear                         */
    TFA9875_BF_BSSR  = 0x50e0,    /*!< Battery voltage read out                           */
    TFA9875_BF_BSSBY = 0x50f0,    /*!< Bypass battery safeguard                           */
    TFA9875_BF_BSSS  = 0x5100,    /*!< Vbat prot steepness                                */
    TFA9875_BF_DCPTC = 0x5111,    /*!< Duration of DC level detection                     */
    TFA9875_BF_DCPL  = 0x5131,    /*!< DC-level detection                                 */
    TFA9875_BF_HPFBYP= 0x5150,    /*!< Bypass HPF                                         */
    TFA9875_BF_BYHWCLIP= 0x5240,    /*!< Bypass hardware clipper                            */
    TFA9875_BF_AMPGAIN= 0x5257,    /*!< Amplifier gain                                     */
    TFA9875_BF_BYPDLYLINE= 0x52f0,    /*!< Bypass the interpolator delay line                 */
    TFA9875_BF_SLOPEE= 0x54a0,    /*!< Enables slope control                              */
    TFA9875_BF_SLOPESET= 0x54b0,    /*!< Slope speed setting                                */
    TFA9875_BF_TDMSPKG= 0x5f63,    /*!< Total gain depending on INPLEV setting (channel 0) */
    TFA9875_BF_IPM   = 0x60e1,    /*!< Idle power mode control                            */
    TFA9875_BF_LNMODE= 0x62e1,    /*!< Ctrl select mode                                   */
    TFA9875_BF_LPM1MODE= 0x64e1,    /*!< Low power mode control                             */
    TFA9875_BF_TDMSRCMAP= 0x6802,    /*!< TDM source mapping                                 */
    TFA9875_BF_TDMSRCAS= 0x6831,    /*!< Sensed value A                                     */
    TFA9875_BF_TDMSRCBS= 0x6851,    /*!< Sensed value B                                     */
    TFA9875_BF_TDMSRCACLIP= 0x6871,    /*!< Clip flag information for TDM Compressed2a/b/c modes */
    TFA9875_BF_LP0   = 0x6e00,    /*!< Idle power mode                                    */
    TFA9875_BF_LP1   = 0x6e10,    /*!< Low power mode 1 detection                         */
    TFA9875_BF_LA    = 0x6e20,    /*!< Low amplitude detection                            */
    TFA9875_BF_VDDPH = 0x6e30,    /*!< Vddp greater than Vbat                             */
    TFA9875_BF_DELCURCOMP= 0x6f02,    /*!< Delay to allign compensation signal with current sense signal */
    TFA9875_BF_SIGCURCOMP= 0x6f40,    /*!< Polarity of compensation for current sense         */
    TFA9875_BF_ENCURCOMP= 0x6f50,    /*!< Enable current sense compensation                  */
    TFA9875_BF_LVLCLPPWM= 0x6f72,    /*!< Set the amount of pwm pulse that may be skipped before clip-flag is triggered */
    TFA9875_BF_DCMCC = 0x7003,    /*!< Max coil current                                   */
    TFA9875_BF_DCCV  = 0x7041,    /*!< Slope compensation current, represents LxF (inductance x frequency)  */
    TFA9875_BF_DCIE  = 0x7060,    /*!< Adaptive boost mode                                */
    TFA9875_BF_DCSR  = 0x7070,    /*!< Soft ramp up/down                                  */
    TFA9875_BF_DCOVL = 0x7087,    /*!< Threshold level to activate active overshoot control */
    TFA9875_BF_DCNS  = 0x7400,    /*!< Disable control of noise shaper in DCDC control    */
    TFA9875_BF_DCNSRST= 0x7410,    /*!< Disable control of reset of noise shaper when 8 bit value for dcdc control occurs */
    TFA9875_BF_DCDYFSW= 0x7420,    /*!< Disables the dynamic frequency switching due to flag_voutcomp86/93 */
    TFA9875_BF_DCTRACK= 0x7430,    /*!< Boost algorithm selection, effective only when DCIE is set to 1 */
    TFA9875_BF_DCTRIP= 0x7444,    /*!< Headroom for 1st Adaptive boost trip level, effective only when DCIE is set to 1 and DCTRACK is 0 */
    TFA9875_BF_DCHOLD= 0x7494,    /*!< Hold time for DCDC booster, effective only when DCIE is set to 1 */
    TFA9875_BF_DCINT = 0x74e0,    /*!< Selection of data for adaptive boost algorithm, effective only when DCIE is set to 1 */
    TFA9875_BF_DCDIS = 0x7500,    /*!< DCDC on/off                                        */
    TFA9875_BF_DCPWM = 0x7510,    /*!< DCDC PWM only mode                                 */
    TFA9875_BF_DCTRIP2= 0x7534,    /*!< Headroom for 2nd Adaptive boost trip level, effective only when DCIE is 1 and DCTRACK is 0 */
    TFA9875_BF_DCTRIPT= 0x7584,    /*!< Headroom for Tracking Adaptive boost trip level, effective only when DCIE is 1 and DCTRACK is 1 */
    TFA9875_BF_BYPDCLPF= 0x75d0,    /*!< Bypass control of DCDC control low pass filter for quantization noise suppression */
    TFA9875_BF_ENBSTFLT= 0x75e0,    /*!< Enable the boost filter                            */
    TFA9875_BF_DCTRIPHYSTE= 0x75f0,    /*!< Enable hysteresis on booster trip levels           */
    TFA9875_BF_DCVOF = 0x7607,    /*!< First boost voltage level                          */
    TFA9875_BF_DCVOS = 0x7687,    /*!< Second boost voltage level                         */
    TFA9875_BF_MTPK  = 0xa107,    /*!< MTP KEY2 register                                  */
    TFA9875_BF_KEY1LOCKED= 0xa200,    /*!< Indicates KEY1 is locked                           */
    TFA9875_BF_KEY2LOCKED= 0xa210,    /*!< Indicates KEY2 is locked                           */
    TFA9875_BF_MTPADDR= 0xa302,    /*!< MTP address from I2C register for read/writing mtp in manual single word mode */
    TFA9875_BF_MTPRDMSB= 0xa50f,    /*!< MSB word of MTP manual read data                   */
    TFA9875_BF_MTPRDLSB= 0xa60f,    /*!< LSB word of MTP manual read data                   */
    TFA9875_BF_MTPWRMSB= 0xa70f,    /*!< MSB word of write data for MTP manual write        */
    TFA9875_BF_MTPWRLSB= 0xa80f,    /*!< LSB word of write data for MTP manual write        */
    TFA9875_BF_EXTTS = 0xb108,    /*!< External temperature (C)                           */
    TFA9875_BF_TROS  = 0xb190,    /*!< Select temp Speaker calibration                    */
    TFA9875_BF_PLLSELI= 0xcd05,    /*!< PLL seli bandwidth control in PLL direct control mode only, use_direct_pll_ctrl set to 1 */
    TFA9875_BF_PLLSELP= 0xcd64,    /*!< PLL selp bandwidth control in PLL direct control mode only, use_direct_pll_ctrl set to 1 */
    TFA9875_BF_PLLSELR= 0xcdb3,    /*!< PLL selr bandwidth control in PLL direct control mode only, use_direct_pll_ctrl set to 1 */
    TFA9875_BF_PLLBDSEL= 0xcdf0,    /*!< PLL bandwidth selection direct control, USE WITH CAUTION */
    TFA9875_BF_PLLFRM= 0xce00,    /*!< PLL free running mode control in functional mode   */
    TFA9875_BF_PLLDI = 0xce10,    /*!< PLL directi control in PLL direct control mode only, use_direct_pll_ctrl set to 1 */
    TFA9875_BF_PLLDO = 0xce20,    /*!< PLL directo control in PLL direct control mode only, use_direct_pll_ctrl set to 1 */
    TFA9875_BF_PLLPDIV= 0xce34,    /*!< PLL PDIV in PLL direct control mode only, use_direct_pll_ctrl set to 1 */
    TFA9875_BF_PLLNDIV= 0xce87,    /*!< PLL NDIV in PLL direct control mode only, use_direct_pll_ctrl set to 1 */
    TFA9875_BF_PLLMDIV= 0xcf0f,    /*!< PLL MDIV in PLL direct control mode only, use_direct_pll_ctrl set to 1 */
    TFA9875_BF_PLLDCTRL= 0xd000,    /*!< Enable PLL direct control mode, overrules the PLL LUT with I2C register values */
    TFA9875_BF_PLLENBL= 0xd010,    /*!< Enables PLL in PLL direct control mode, use_direct_pll_ctrl set to 1 */
    TFA9875_BF_PLLLIMOFF= 0xd020,    /*!< PLL up limiter control in PLL direct bandwidth control mode */
    TFA9875_BF_PLLCLKSTB= 0xd030,    /*!< PLL FRM clock stable in pll direct control mode, use_direct_pll_ctrl set to 1 */
    TFA9875_BF_PLLSTRTM= 0xd042,    /*!< PLL startup time selection control                 */
    TFA9875_BF_SWPROFIL= 0xe00f,    /*!< Software profile data                              */
    TFA9875_BF_SWVSTEP= 0xe10f,    /*!< Software vstep information                         */
    TFA9875_BF_MTPOTC= 0xf000,    /*!< Calibration schedule                               */
    TFA9875_BF_MTPEX = 0xf010,    /*!< Calibration Ron executed                           */
    TFA9875_BF_DCMCCAPI= 0xf020,    /*!< Calibration current limit DCDC                     */
    TFA9875_BF_DCMCCSB= 0xf030,    /*!< Sign bit for delta calibration current limit DCDC  */
    TFA9875_BF_USERDEF= 0xf042,    /*!< Calibration delta current limit DCDC               */
    TFA9875_BF_CUSTINFO= 0xf078,    /*!< Reserved space for allowing customer to store speaker information */
    TFA9875_BF_R25C  = 0xf50f,    /*!< Ron resistance of speaker coil                     */
} Tfa9875BfEnumList_t;
#define TFA9875_NAMETABLE static tfaBfName_t Tfa9875DatasheetNames[]= {\
   { 0x0, "PWDN"},    /* Powerdown selection                               , */\
   { 0x10, "I2CR"},    /* I2C reset - auto clear                            , */\
   { 0x30, "AMPE"},    /* Activate amplifier                                , */\
   { 0x40, "DCA"},    /* Activate DC-to-DC converter                       , */\
   { 0x71, "INTP"},    /* Interrupt config                                  , */\
   { 0xa0, "BYPUVP"},    /* Bypass UVP                                        , */\
   { 0xb0, "BYPOCP"},    /* Bypass OCP                                        , */\
   { 0xc0, "TSTOCP"},    /* OCP testing control                               , */\
   { 0xd0, "MANWTIME"},    /* Manager wait time selection control               , */\
   { 0xe0, "ENPLLSYNC"},    /* Manager control for enabling synchronisation with PLL FS, */\
   { 0xf0, "COORHYS"},    /* Select hysteresis for clock range detector        , */\
   { 0x120, "MANSCONF"},    /* Configuration setting if I2C settings are uploaded by the host, */\
   { 0x160, "MUTETO"},    /* Time out mute sequence                            , */\
   { 0x170, "MANROBOD"},    /* Controls the reaction of the device on BOD        , */\
   { 0x180, "BODHYS"},    /* Enable Hysteresis of BOD                          , */\
   { 0x191, "BODFILT"},    /* BOD filter                                        , */\
   { 0x1b1, "BODTHLVL"},    /* BOD threshold level                               , */\
   { 0x1d0, "MANEDCTH"},    /* Device response to too high DC level flag (DCTH = 1) , */\
   { 0x1e0, "OPENMTP"},    /* Control for MTP protection                        , */\
   { 0x203, "AUDFS"},    /* Sample rate (Fs)                                  , */\
   { 0x240, "ENLBW"},    /* CS/VS decimator low bandwidth mode control        , */\
   { 0x255, "FRACTDEL"},    /* V/I Fractional delay                              , */\
   { 0x30f, "REV"},    /* Device revision information                       , */\
   { 0x400, "REFCKEXT"},    /* PLL external ref clock                            , */\
   { 0x460, "MANAOOSC"},    /* Internal oscillator control during power down mode, */\
   { 0x480, "FSSYNCEN"},    /* Enable FS synchronisation for clock divider       , */\
   { 0x490, "CLKREFSYNCEN"},    /* Enable PLL reference clock synchronisation for clock divider, */\
   { 0x500, "CGUSYNCDCG"},    /* Clock gating control for CGU synchronisation module, */\
   { 0x510, "FRCCLKSPKR"},    /* Force active the speaker sub-system clock when in idle power, */\
   { 0x520, "BSTCLKLP"},    /* Boost clock control in low power mode1            , */\
   { 0x5c0, "SSFAIME"},    /* Sub-system FAIM (MTP)                             , */\
   { 0x1000, "VDDS"},    /* POR (sticky flag, clear on write a '1')           , */\
   { 0x1010, "OTDS"},    /* OTP alarm (sticky flag,  clear on write a '1')    , */\
   { 0x1020, "UVDS"},    /* UVP alarm (sticky flag,  clear on write a '1')    , */\
   { 0x1030, "OVDS"},    /* OVP alarm (sticky flag,  clear on write a '1')    , */\
   { 0x1040, "OCDS"},    /* OCP amplifier (sticky flag,  clear on write a '1'), */\
   { 0x1050, "NOCLK"},    /* Lost clock (sticky flag,  clear on write a '1')   , */\
   { 0x1060, "CLKOOR"},    /* External clock status (sticky flag,  clear on write a '1'), */\
   { 0x1070, "DCOCPOK"},    /* DCDC OCP nmos (sticky flag,  clear on write a '1'), */\
   { 0x1080, "DCIL"},    /* DCDC current limiting (sticky flag,  clear on write a '1'), */\
   { 0x1090, "DCDCA"},    /* DCDC active (sticky flag,  clear on write a '1')  , */\
   { 0x10a0, "ADCCR"},    /* ADC ready flag (sticky flag,  clear on write a '1'), */\
   { 0x10b0, "OCPOAP"},    /* OCPOK pmos A (sticky flag,  clear on write a '1') , */\
   { 0x10c0, "OCPOAN"},    /* OCPOK nmos A (sticky flag,  clear on write a '1') , */\
   { 0x10d0, "OCPOBP"},    /* OCPOK pmos B (sticky flag,  clear on write a '1') , */\
   { 0x10e0, "OCPOBN"},    /* OCPOK nmos B (sticky flag,  clear on write a '1') , */\
   { 0x10f0, "DCTH"},    /* DC level on audio input stream too high (sticky flag,  clear on write a '1'), */\
   { 0x1100, "CLKS"},    /* Clocks stable                                     , */\
   { 0x1110, "MTPB"},    /* MTP busy                                          , */\
   { 0x1120, "TDMERR"},    /* TDM error                                         , */\
   { 0x1130, "DCDCPC"},    /* Indicates current is max in DC-to-DC converter    , */\
   { 0x1140, "DCHVBAT"},    /* DCDC level 1x                                     , */\
   { 0x1150, "DCH114"},    /* DCDC level 1.14x                                  , */\
   { 0x1160, "DCH107"},    /* DCDC level 1.07x                                  , */\
   { 0x1170, "PLLS"},    /* PLL lock                                          , */\
   { 0x1180, "TDMLUTER"},    /* TDM LUT error                                     , */\
   { 0x1190, "SWS"},    /* Amplifier engage                                  , */\
   { 0x11a0, "AMPS"},    /* Amplifier enable                                  , */\
   { 0x11b0, "AREFS"},    /* References enable                                 , */\
   { 0x11c0, "CLIPS"},    /* Amplifier clipping                                , */\
   { 0x1203, "MANSTATE"},    /* Device manager status                             , */\
   { 0x1243, "AMPSTE"},    /* Amplifier control status                          , */\
   { 0x1282, "TDMSTAT"},    /* TDM status bits                                   , */\
   { 0x12b1, "DCMODE"},    /* DCDC mode status bits                             , */\
   { 0x12d0, "WAITSYNC"},    /* CGU and PLL synchronisation status flag from CGU  , */\
   { 0x1300, "BODNOK"},    /* BOD Flag VDD NOT OK (sticky flag,  clear on write a '1'), */\
   { 0x140c, "DCLD"},    /* DC level detected by DC protection module (2s complement), */\
   { 0x1509, "BATS"},    /* Battery voltage (V)                               , */\
   { 0x1608, "TEMPS"},    /* IC Temperature (C)                                , */\
   { 0x1709, "VDDPS"},    /* IC VDDP voltage ( 1023*VDDP/13 V)                 , */\
   { 0x2000, "TDME"},    /* Enable interface                                  , */\
   { 0x2011, "AMPINSEL"},    /* Amplifier input selection                         , */\
   { 0x2030, "INPLEV"},    /* TDM output attenuation                            , */\
   { 0x2040, "TDMCLINV"},    /* Reception data to BCK clock                       , */\
   { 0x2050, "TDMFSPOL"},    /* FS polarity                                       , */\
   { 0x2061, "TDMSLOTS"},    /* N-slots in Frame                                  , */\
   { 0x2081, "TDMSLLN"},    /* N-bits in slot                                    , */\
   { 0x20a1, "TDMSSIZE"},    /* Sample size per slot                              , */\
   { 0x20c3, "TDMNBCK"},    /* N-BCK's in FS                                     , */\
   { 0x2100, "TDMDEL"},    /* Data delay to FS                                  , */\
   { 0x2110, "TDMADJ"},    /* Data adjustment                                   , */\
   { 0x2120, "TDMSPKE"},    /* Control audio TDM channel in 0                    , */\
   { 0x2130, "TDMDCE"},    /* Control audio TDM channel in 1                    , */\
   { 0x2140, "TDMSRC0E"},    /* Enable TDM source0 data channel                   , */\
   { 0x2150, "TDMSRC1E"},    /* Enable TDM source1 data channel                   , */\
   { 0x2160, "TDMSRC2E"},    /* Enable TDM source2 data channel                   , */\
   { 0x2170, "TDMSRC3E"},    /* Enable TDM source3 data channel                   , */\
   { 0x2183, "TDMSPKS"},    /* TDM slot for sink 0                               , */\
   { 0x21c3, "TDMDCS"},    /* TDM slot for sink 1                               , */\
   { 0x2203, "TDMSRC0S"},    /* Slot Position of TDM source0 channel data         , */\
   { 0x2243, "TDMSRC1S"},    /* Slot Position of TDM source1 channel data         , */\
   { 0x2283, "TDMSRC2S"},    /* Slot Position of TDM source2 channel data         , */\
   { 0x22c3, "TDMSRC3S"},    /* Slot Position of TDM source3 channel data         , */\
   { 0x4000, "ISTVDDS"},    /* Interrupt status POR                              , */\
   { 0x4010, "ISTBSTOC"},    /* Interrupt status DCDC OCP alarm                   , */\
   { 0x4020, "ISTOTDS"},    /* Interrupt status OTP alarm                        , */\
   { 0x4030, "ISTOCPR"},    /* Interrupt status OCP alarm                        , */\
   { 0x4040, "ISTUVDS"},    /* Interrupt status UVP alarm                        , */\
   { 0x4050, "ISTTDMER"},    /* Interrupt status TDM error                        , */\
   { 0x4060, "ISTNOCLK"},    /* Interrupt status lost clock                       , */\
   { 0x4070, "ISTDCTH"},    /* Interrupt status dc too high                      , */\
   { 0x4080, "ISTBODNOK"},    /* Interrupt status brown out detected               , */\
   { 0x4090, "ISTCOOR"},    /* Interrupt status clock out of range               , */\
   { 0x4400, "ICLVDDS"},    /* Clear interrupt status POR                        , */\
   { 0x4410, "ICLBSTOC"},    /* Clear interrupt status DCDC OCP                   , */\
   { 0x4420, "ICLOTDS"},    /* Clear interrupt status OTP alarm                  , */\
   { 0x4430, "ICLOCPR"},    /* Clear interrupt status OCP alarm                  , */\
   { 0x4440, "ICLUVDS"},    /* Clear interrupt status UVP alarm                  , */\
   { 0x4450, "ICLTDMER"},    /* Clear interrupt status TDM error                  , */\
   { 0x4460, "ICLNOCLK"},    /* Clear interrupt status lost clk                   , */\
   { 0x4470, "ICLDCTH"},    /* Clear interrupt status dc too high                , */\
   { 0x4480, "ICLBODNOK"},    /* Clear interrupt status brown out detected         , */\
   { 0x4490, "ICLCOOR"},    /* Clear interrupt status clock out of range         , */\
   { 0x4800, "IEVDDS"},    /* Enable interrupt POR                              , */\
   { 0x4810, "IEBSTOC"},    /* Enable interrupt DCDC OCP                         , */\
   { 0x4820, "IEOTDS"},    /* Enable interrupt OTP alarm                        , */\
   { 0x4830, "IEOCPR"},    /* Enable interrupt OCP alarm                        , */\
   { 0x4840, "IEUVDS"},    /* Enable interrupt UVP alarm                        , */\
   { 0x4850, "IETDMER"},    /* Enable interrupt TDM error                        , */\
   { 0x4860, "IENOCLK"},    /* Enable interrupt lost clk                         , */\
   { 0x4870, "IEDCTH"},    /* Enable interrupt dc too high                      , */\
   { 0x4880, "IEBODNOK"},    /* Enable interrupt brown out detect                 , */\
   { 0x4890, "IECOOR"},    /* Enable interrupt clock out of range               , */\
   { 0x4c00, "IPOVDDS"},    /* Interrupt polarity POR                            , */\
   { 0x4c10, "IPOBSTOC"},    /* Interrupt polarity DCDC OCP                       , */\
   { 0x4c20, "IPOOTDS"},    /* Interrupt polarity OTP alarm                      , */\
   { 0x4c30, "IPOOCPR"},    /* Interrupt polarity OCP alarm                      , */\
   { 0x4c40, "IPOUVDS"},    /* Interrupt polarity UVP alarm                      , */\
   { 0x4c50, "IPOTDMER"},    /* Interrupt polarity TDM error                      , */\
   { 0x4c60, "IPONOCLK"},    /* Interrupt polarity lost clk                       , */\
   { 0x4c70, "IPODCTH"},    /* Interrupt polarity dc too high                    , */\
   { 0x4c80, "IPOBODNOK"},    /* Interrupt polarity brown out detect               , */\
   { 0x4c90, "IPOCOOR"},    /* Interrupt polarity clock out of range             , */\
   { 0x5001, "BSSCR"},    /* Battery safeguard attack time                     , */\
   { 0x5023, "BSST"},    /* Battery safeguard threshold voltage level         , */\
   { 0x5061, "BSSRL"},    /* Battery safeguard maximum reduction               , */\
   { 0x50d0, "BSSCLRST"},    /* Reset clipper - auto clear                        , */\
   { 0x50e0, "BSSR"},    /* Battery voltage read out                          , */\
   { 0x50f0, "BSSBY"},    /* Bypass battery safeguard                          , */\
   { 0x5100, "BSSS"},    /* Vbat prot steepness                               , */\
   { 0x5111, "DCPTC"},    /* Duration of DC level detection                    , */\
   { 0x5131, "DCPL"},    /* DC-level detection                                , */\
   { 0x5150, "HPFBYP"},    /* Bypass HPF                                        , */\
   { 0x5240, "BYHWCLIP"},    /* Bypass hardware clipper                           , */\
   { 0x5257, "AMPGAIN"},    /* Amplifier gain                                    , */\
   { 0x52f0, "BYPDLYLINE"},    /* Bypass the interpolator delay line                , */\
   { 0x54a0, "SLOPEE"},    /* Enables slope control                             , */\
   { 0x54b0, "SLOPESET"},    /* Slope speed setting                               , */\
   { 0x5f63, "TDMSPKG"},    /* Total gain depending on INPLEV setting (channel 0), */\
   { 0x60e1, "IPM"},    /* Idle power mode control                           , */\
   { 0x62e1, "LNMODE"},    /* Ctrl select mode                                  , */\
   { 0x64e1, "LPM1MODE"},    /* Low power mode control                            , */\
   { 0x6802, "TDMSRCMAP"},    /* TDM source mapping                                , */\
   { 0x6831, "TDMSRCAS"},    /* Sensed value A                                    , */\
   { 0x6851, "TDMSRCBS"},    /* Sensed value B                                    , */\
   { 0x6871, "TDMSRCACLIP"},    /* Clip flag information for TDM Compressed2a/b/c modes, */\
   { 0x6e00, "LP0"},    /* Idle power mode                                   , */\
   { 0x6e10, "LP1"},    /* Low power mode 1 detection                        , */\
   { 0x6e20, "LA"},    /* Low amplitude detection                           , */\
   { 0x6e30, "VDDPH"},    /* Vddp greater than Vbat                            , */\
   { 0x6f02, "DELCURCOMP"},    /* Delay to allign compensation signal with current sense signal, */\
   { 0x6f40, "SIGCURCOMP"},    /* Polarity of compensation for current sense        , */\
   { 0x6f50, "ENCURCOMP"},    /* Enable current sense compensation                 , */\
   { 0x6f72, "LVLCLPPWM"},    /* Set the amount of pwm pulse that may be skipped before clip-flag is triggered, */\
   { 0x7003, "DCMCC"},    /* Max coil current                                  , */\
   { 0x7041, "DCCV"},    /* Slope compensation current, represents LxF (inductance x frequency) , */\
   { 0x7060, "DCIE"},    /* Adaptive boost mode                               , */\
   { 0x7070, "DCSR"},    /* Soft ramp up/down                                 , */\
   { 0x7087, "DCOVL"},    /* Threshold level to activate active overshoot control, */\
   { 0x7400, "DCNS"},    /* Disable control of noise shaper in DCDC control   , */\
   { 0x7410, "DCNSRST"},    /* Disable control of reset of noise shaper when 8 bit value for dcdc control occurs, */\
   { 0x7420, "DCDYFSW"},    /* Disables the dynamic frequency switching due to flag_voutcomp86/93, */\
   { 0x7430, "DCTRACK"},    /* Boost algorithm selection, effective only when DCIE is set to 1, */\
   { 0x7444, "DCTRIP"},    /* Headroom for 1st Adaptive boost trip level, effective only when DCIE is set to 1 and DCTRACK is 0, */\
   { 0x7494, "DCHOLD"},    /* Hold time for DCDC booster, effective only when DCIE is set to 1, */\
   { 0x74e0, "DCINT"},    /* Selection of data for adaptive boost algorithm, effective only when DCIE is set to 1, */\
   { 0x7500, "DCDIS"},    /* DCDC on/off                                       , */\
   { 0x7510, "DCPWM"},    /* DCDC PWM only mode                                , */\
   { 0x7534, "DCTRIP2"},    /* Headroom for 2nd Adaptive boost trip level, effective only when DCIE is 1 and DCTRACK is 0, */\
   { 0x7584, "DCTRIPT"},    /* Headroom for Tracking Adaptive boost trip level, effective only when DCIE is 1 and DCTRACK is 1, */\
   { 0x75d0, "BYPDCLPF"},    /* Bypass control of DCDC control low pass filter for quantization noise suppression, */\
   { 0x75e0, "ENBSTFLT"},    /* Enable the boost filter                           , */\
   { 0x75f0, "DCTRIPHYSTE"},    /* Enable hysteresis on booster trip levels          , */\
   { 0x7607, "DCVOF"},    /* First boost voltage level                         , */\
   { 0x7687, "DCVOS"},    /* Second boost voltage level                        , */\
   { 0xa107, "MTPK"},    /* MTP KEY2 register                                 , */\
   { 0xa200, "KEY1LOCKED"},    /* Indicates KEY1 is locked                          , */\
   { 0xa210, "KEY2LOCKED"},    /* Indicates KEY2 is locked                          , */\
   { 0xa302, "MTPADDR"},    /* MTP address from I2C register for read/writing mtp in manual single word mode, */\
   { 0xa50f, "MTPRDMSB"},    /* MSB word of MTP manual read data                  , */\
   { 0xa60f, "MTPRDLSB"},    /* LSB word of MTP manual read data                  , */\
   { 0xa70f, "MTPWRMSB"},    /* MSB word of write data for MTP manual write       , */\
   { 0xa80f, "MTPWRLSB"},    /* LSB word of write data for MTP manual write       , */\
   { 0xb108, "EXTTS"},    /* External temperature (C)                          , */\
   { 0xb190, "TROS"},    /* Select temp Speaker calibration                   , */\
   { 0xcd05, "PLLSELI"},    /* PLL seli bandwidth control in PLL direct control mode only, use_direct_pll_ctrl set to 1, */\
   { 0xcd64, "PLLSELP"},    /* PLL selp bandwidth control in PLL direct control mode only, use_direct_pll_ctrl set to 1, */\
   { 0xcdb3, "PLLSELR"},    /* PLL selr bandwidth control in PLL direct control mode only, use_direct_pll_ctrl set to 1, */\
   { 0xcdf0, "PLLBDSEL"},    /* PLL bandwidth selection direct control, USE WITH CAUTION, */\
   { 0xce00, "PLLFRM"},    /* PLL free running mode control in functional mode  , */\
   { 0xce10, "PLLDI"},    /* PLL directi control in PLL direct control mode only, use_direct_pll_ctrl set to 1, */\
   { 0xce20, "PLLDO"},    /* PLL directo control in PLL direct control mode only, use_direct_pll_ctrl set to 1, */\
   { 0xce34, "PLLPDIV"},    /* PLL PDIV in PLL direct control mode only, use_direct_pll_ctrl set to 1, */\
   { 0xce87, "PLLNDIV"},    /* PLL NDIV in PLL direct control mode only, use_direct_pll_ctrl set to 1, */\
   { 0xcf0f, "PLLMDIV"},    /* PLL MDIV in PLL direct control mode only, use_direct_pll_ctrl set to 1, */\
   { 0xd000, "PLLDCTRL"},    /* Enable PLL direct control mode, overrules the PLL LUT with I2C register values, */\
   { 0xd010, "PLLENBL"},    /* Enables PLL in PLL direct control mode, use_direct_pll_ctrl set to 1, */\
   { 0xd020, "PLLLIMOFF"},    /* PLL up limiter control in PLL direct bandwidth control mode, */\
   { 0xd030, "PLLCLKSTB"},    /* PLL FRM clock stable in pll direct control mode, use_direct_pll_ctrl set to 1, */\
   { 0xd042, "PLLSTRTM"},    /* PLL startup time selection control                , */\
   { 0xe00f, "SWPROFIL"},    /* Software profile data                             , */\
   { 0xe10f, "SWVSTEP"},    /* Software vstep information                        , */\
   { 0xf000, "MTPOTC"},    /* Calibration schedule                              , */\
   { 0xf010, "MTPEX"},    /* Calibration Ron executed                          , */\
   { 0xf020, "DCMCCAPI"},    /* Calibration current limit DCDC                    , */\
   { 0xf030, "DCMCCSB"},    /* Sign bit for delta calibration current limit DCDC , */\
   { 0xf042, "USERDEF"},    /* Calibration delta current limit DCDC              , */\
   { 0xf078, "CUSTINFO"},    /* Reserved space for allowing customer to store speaker information, */\
   { 0xf50f, "R25C"},    /* Ron resistance of speaker coil                    , */\
   { 0xffff,"Unknown bitfield enum" }   /* not found */\
};

#define TFA9875_BITNAMETABLE static tfaBfName_t Tfa9875BitNames[]= {\
   { 0x0, "powerdown"},    /* Powerdown selection                               , */\
   { 0x10, "reset"},    /* I2C reset - auto clear                            , */\
   { 0x30, "enbl_amplifier"},    /* Activate amplifier                                , */\
   { 0x40, "enbl_boost"},    /* Activate DC-to-DC converter                       , */\
   { 0x71, "int_pad_io"},    /* Interrupt config                                  , */\
   { 0xa0, "bypass_uvp"},    /* Bypass UVP                                        , */\
   { 0xb0, "bypass_ocp"},    /* Bypass OCP                                        , */\
   { 0xc0, "test_ocp"},    /* OCP testing control                               , */\
   { 0xd0, "sel_man_wait_time"},    /* Manager wait time selection control               , */\
   { 0xe0, "enbl_pll_synchronisation"},    /* Manager control for enabling synchronisation with PLL FS, */\
   { 0xf0, "sel_hysteresis"},    /* Select hysteresis for clock range detector        , */\
   { 0x120, "src_set_configured"},    /* Configuration setting if I2C settings are uploaded by the host, */\
   { 0x160, "disable_mute_time_out"},    /* Time out mute sequence                            , */\
   { 0x170, "man_enbl_brown"},    /* Controls the reaction of the device on BOD        , */\
   { 0x180, "bod_hyst_enbl"},    /* Enable Hysteresis of BOD                          , */\
   { 0x191, "bod_delay_set"},    /* BOD filter                                        , */\
   { 0x1b1, "bod_lvl_set"},    /* BOD threshold level                               , */\
   { 0x1d0, "man_enbl_dc_too_high"},    /* Device response to too high DC level flag (DCTH = 1) , */\
   { 0x1e0, "unprotect_faim"},    /* Control for MTP protection                        , */\
   { 0x203, "audio_fs"},    /* Sample rate (Fs)                                  , */\
   { 0x240, "enbl_low_bandwidth"},    /* CS/VS decimator low bandwidth mode control        , */\
   { 0x255, "cs_frac_delay"},    /* V/I Fractional delay                              , */\
   { 0x30f, "device_rev"},    /* Device revision information                       , */\
   { 0x400, "pll_clkin_sel"},    /* PLL external ref clock                            , */\
   { 0x460, "enbl_osc_auto_off"},    /* Internal oscillator control during power down mode, */\
   { 0x480, "enbl_fs_sync"},    /* Enable FS synchronisation for clock divider       , */\
   { 0x490, "enbl_clkref_sync"},    /* Enable PLL reference clock synchronisation for clock divider, */\
   { 0x500, "disable_cgu_sync_cgate"},    /* Clock gating control for CGU synchronisation module, */\
   { 0x510, "force_spkr_clk"},    /* Force active the speaker sub-system clock when in idle power, */\
   { 0x520, "ctrl_bst_clk_lp1"},    /* Boost clock control in low power mode1            , */\
   { 0x5c0, "enbl_faim_ss"},    /* Sub-system FAIM (MTP)                             , */\
   { 0x802, "ctrl_on2off_criterion"},    /* Amplifier on-off criteria for shutdown            , */\
   { 0xe07, "ctrl_digtoana"},    /* Spare control from digital to analog              , */\
   { 0xf0f, "hidden_code"},    /* Hidden code to enable access to key registers     , */\
   { 0x1000, "flag_por"},    /* POR (sticky flag, clear on write a '1')           , */\
   { 0x1010, "flag_otpok"},    /* OTP alarm (sticky flag,  clear on write a '1')    , */\
   { 0x1020, "flag_uvpok"},    /* UVP alarm (sticky flag,  clear on write a '1')    , */\
   { 0x1030, "flag_ovpok"},    /* OVP alarm (sticky flag,  clear on write a '1')    , */\
   { 0x1040, "flag_ocp_alarm"},    /* OCP amplifier (sticky flag,  clear on write a '1'), */\
   { 0x1050, "flag_lost_clk"},    /* Lost clock (sticky flag,  clear on write a '1')   , */\
   { 0x1060, "flag_clk_out_of_range"},    /* External clock status (sticky flag,  clear on write a '1'), */\
   { 0x1070, "flag_bst_ocpok"},    /* DCDC OCP nmos (sticky flag,  clear on write a '1'), */\
   { 0x1080, "flag_bst_bstcur"},    /* DCDC current limiting (sticky flag,  clear on write a '1'), */\
   { 0x1090, "flag_bst_hiz"},    /* DCDC active (sticky flag,  clear on write a '1')  , */\
   { 0x10a0, "flag_adc10_ready"},    /* ADC ready flag (sticky flag,  clear on write a '1'), */\
   { 0x10b0, "flag_ocpokap"},    /* OCPOK pmos A (sticky flag,  clear on write a '1') , */\
   { 0x10c0, "flag_ocpokan"},    /* OCPOK nmos A (sticky flag,  clear on write a '1') , */\
   { 0x10d0, "flag_ocpokbp"},    /* OCPOK pmos B (sticky flag,  clear on write a '1') , */\
   { 0x10e0, "flag_ocpokbn"},    /* OCPOK nmos B (sticky flag,  clear on write a '1') , */\
   { 0x10f0, "flag_dc_too_high"},    /* DC level on audio input stream too high (sticky flag,  clear on write a '1'), */\
   { 0x1100, "flag_clocks_stable"},    /* Clocks stable                                     , */\
   { 0x1110, "flag_mtp_busy"},    /* MTP busy                                          , */\
   { 0x1120, "flag_tdm_error"},    /* TDM error                                         , */\
   { 0x1130, "flag_bst_peakcur"},    /* Indicates current is max in DC-to-DC converter    , */\
   { 0x1140, "flag_bst_voutcomp"},    /* DCDC level 1x                                     , */\
   { 0x1150, "flag_bst_voutcomp86"},    /* DCDC level 1.14x                                  , */\
   { 0x1160, "flag_bst_voutcomp93"},    /* DCDC level 1.07x                                  , */\
   { 0x1170, "flag_pll_lock"},    /* PLL lock                                          , */\
   { 0x1180, "flag_tdm_lut_error"},    /* TDM LUT error                                     , */\
   { 0x1190, "flag_engage"},    /* Amplifier engage                                  , */\
   { 0x11a0, "flag_enbl_amp"},    /* Amplifier enable                                  , */\
   { 0x11b0, "flag_enbl_ref"},    /* References enable                                 , */\
   { 0x11c0, "flag_clip"},    /* Amplifier clipping                                , */\
   { 0x1203, "man_state"},    /* Device manager status                             , */\
   { 0x1243, "amp_ctrl_state"},    /* Amplifier control status                          , */\
   { 0x1282, "flag_tdm_status"},    /* TDM status bits                                   , */\
   { 0x12b1, "status_bst_mode"},    /* DCDC mode status bits                             , */\
   { 0x12d0, "flag_waiting_for_sync"},    /* CGU and PLL synchronisation status flag from CGU  , */\
   { 0x1300, "flag_bod_vddd_nok"},    /* BOD Flag VDD NOT OK (sticky flag,  clear on write a '1'), */\
   { 0x140c, "dc_level_detect"},    /* DC level detected by DC protection module (2s complement), */\
   { 0x1509, "bat_adc"},    /* Battery voltage (V)                               , */\
   { 0x1608, "temp_adc"},    /* IC Temperature (C)                                , */\
   { 0x1709, "vddp_adc"},    /* IC VDDP voltage ( 1023*VDDP/13 V)                 , */\
   { 0x2000, "tdm_enable"},    /* Enable interface                                  , */\
   { 0x2011, "tdm_vamp_sel"},    /* Amplifier input selection                         , */\
   { 0x2030, "tdm_input_level"},    /* TDM output attenuation                            , */\
   { 0x2040, "tdm_clk_inversion"},    /* Reception data to BCK clock                       , */\
   { 0x2050, "tdm_fs_ws_polarity"},    /* FS polarity                                       , */\
   { 0x2061, "tdm_nb_of_slots"},    /* N-slots in Frame                                  , */\
   { 0x2081, "tdm_slot_length"},    /* N-bits in slot                                    , */\
   { 0x20a1, "tdm_sample_size"},    /* Sample size per slot                              , */\
   { 0x20c3, "tdm_nbck"},    /* N-BCK's in FS                                     , */\
   { 0x2100, "tdm_data_delay"},    /* Data delay to FS                                  , */\
   { 0x2110, "tdm_data_adjustment"},    /* Data adjustment                                   , */\
   { 0x2120, "tdm_sink0_enable"},    /* Control audio TDM channel in 0                    , */\
   { 0x2130, "tdm_sink1_enable"},    /* Control audio TDM channel in 1                    , */\
   { 0x2140, "tdm_source0_enable"},    /* Enable TDM source0 data channel                   , */\
   { 0x2150, "tdm_source1_enable"},    /* Enable TDM source1 data channel                   , */\
   { 0x2160, "tdm_source2_enable"},    /* Enable TDM source2 data channel                   , */\
   { 0x2170, "tdm_source3_enable"},    /* Enable TDM source3 data channel                   , */\
   { 0x2183, "tdm_sink0_slot"},    /* TDM slot for sink 0                               , */\
   { 0x21c3, "tdm_sink1_slot"},    /* TDM slot for sink 1                               , */\
   { 0x2203, "tdm_source0_slot"},    /* Slot Position of TDM source0 channel data         , */\
   { 0x2243, "tdm_source1_slot"},    /* Slot Position of TDM source1 channel data         , */\
   { 0x2283, "tdm_source2_slot"},    /* Slot Position of TDM source2 channel data         , */\
   { 0x22c3, "tdm_source3_slot"},    /* Slot Position of TDM source3 channel data         , */\
   { 0x4000, "int_out_flag_por"},    /* Interrupt status POR                              , */\
   { 0x4010, "int_out_flag_bst_ocpok"},    /* Interrupt status DCDC OCP alarm                   , */\
   { 0x4020, "int_out_flag_otpok"},    /* Interrupt status OTP alarm                        , */\
   { 0x4030, "int_out_flag_ocp_alarm"},    /* Interrupt status OCP alarm                        , */\
   { 0x4040, "int_out_flag_uvpok"},    /* Interrupt status UVP alarm                        , */\
   { 0x4050, "int_out_flag_tdm_error"},    /* Interrupt status TDM error                        , */\
   { 0x4060, "int_out_flag_lost_clk"},    /* Interrupt status lost clock                       , */\
   { 0x4070, "int_out_flag_dc_too_high"},    /* Interrupt status dc too high                      , */\
   { 0x4080, "int_out_flag_bod_vddd_nok"},    /* Interrupt status brown out detected               , */\
   { 0x4090, "int_out_flag_clk_out_of_range"},    /* Interrupt status clock out of range               , */\
   { 0x4400, "int_in_flag_por"},    /* Clear interrupt status POR                        , */\
   { 0x4410, "int_in_flag_bst_ocpok"},    /* Clear interrupt status DCDC OCP                   , */\
   { 0x4420, "int_in_flag_otpok"},    /* Clear interrupt status OTP alarm                  , */\
   { 0x4430, "int_in_flag_ocp_alarm"},    /* Clear interrupt status OCP alarm                  , */\
   { 0x4440, "int_in_flag_uvpok"},    /* Clear interrupt status UVP alarm                  , */\
   { 0x4450, "int_in_flag_tdm_error"},    /* Clear interrupt status TDM error                  , */\
   { 0x4460, "int_in_flag_lost_clk"},    /* Clear interrupt status lost clk                   , */\
   { 0x4470, "int_in_flag_dc_too_high"},    /* Clear interrupt status dc too high                , */\
   { 0x4480, "int_in_flag_bod_vddd_nok"},    /* Clear interrupt status brown out detected         , */\
   { 0x4490, "int_in_flag_clk_out_of_range"},    /* Clear interrupt status clock out of range         , */\
   { 0x4800, "int_enable_flag_por"},    /* Enable interrupt POR                              , */\
   { 0x4810, "int_enable_flag_bst_ocpok"},    /* Enable interrupt DCDC OCP                         , */\
   { 0x4820, "int_enable_flag_otpok"},    /* Enable interrupt OTP alarm                        , */\
   { 0x4830, "int_enable_flag_ocp_alarm"},    /* Enable interrupt OCP alarm                        , */\
   { 0x4840, "int_enable_flag_uvpok"},    /* Enable interrupt UVP alarm                        , */\
   { 0x4850, "int_enable_flag_tdm_error"},    /* Enable interrupt TDM error                        , */\
   { 0x4860, "int_enable_flag_lost_clk"},    /* Enable interrupt lost clk                         , */\
   { 0x4870, "int_enable_flag_dc_too_high"},    /* Enable interrupt dc too high                      , */\
   { 0x4880, "int_enable_flag_bod_vddd_nok"},    /* Enable interrupt brown out detect                 , */\
   { 0x4890, "int_enable_flag_clk_out_of_range"},    /* Enable interrupt clock out of range               , */\
   { 0x4c00, "int_polarity_flag_por"},    /* Interrupt polarity POR                            , */\
   { 0x4c10, "int_polarity_flag_bst_ocpok"},    /* Interrupt polarity DCDC OCP                       , */\
   { 0x4c20, "int_polarity_flag_otpok"},    /* Interrupt polarity OTP alarm                      , */\
   { 0x4c30, "int_polarity_flag_ocp_alarm"},    /* Interrupt polarity OCP alarm                      , */\
   { 0x4c40, "int_polarity_flag_uvpok"},    /* Interrupt polarity UVP alarm                      , */\
   { 0x4c50, "int_polarity_flag_tdm_error"},    /* Interrupt polarity TDM error                      , */\
   { 0x4c60, "int_polarity_flag_lost_clk"},    /* Interrupt polarity lost clk                       , */\
   { 0x4c70, "int_polarity_flag_dc_too_high"},    /* Interrupt polarity dc too high                    , */\
   { 0x4c80, "int_polarity_flag_bod_vddd_nok"},    /* Interrupt polarity brown out detect               , */\
   { 0x4c90, "int_polarity_flag_clk_out_of_range"},    /* Interrupt polarity clock out of range             , */\
   { 0x5001, "vbat_prot_attack_time"},    /* Battery safeguard attack time                     , */\
   { 0x5023, "vbat_prot_thlevel"},    /* Battery safeguard threshold voltage level         , */\
   { 0x5061, "vbat_prot_max_reduct"},    /* Battery safeguard maximum reduction               , */\
   { 0x50d0, "rst_min_vbat"},    /* Reset clipper - auto clear                        , */\
   { 0x50e0, "sel_vbat"},    /* Battery voltage read out                          , */\
   { 0x50f0, "bypass_clipper"},    /* Bypass battery safeguard                          , */\
   { 0x5100, "batsense_steepness"},    /* Vbat prot steepness                               , */\
   { 0x5111, "dc_prot_time_constant"},    /* Duration of DC level detection                    , */\
   { 0x5131, "dc_prot_level"},    /* DC-level detection                                , */\
   { 0x5150, "bypass_hp"},    /* Bypass HPF                                        , */\
   { 0x5240, "bypasshwclip"},    /* Bypass hardware clipper                           , */\
   { 0x5257, "gain"},    /* Amplifier gain                                    , */\
   { 0x52f0, "bypass_dly_line"},    /* Bypass the interpolator delay line                , */\
   { 0x5300, "bypass_lp"},    /* Bypass the low pass filter inside temperature sensor, */\
   { 0x5310, "icomp_engage"},    /* Engage of icomp                                   , */\
   { 0x5320, "ctrl_kickback"},    /* Prevent double pulses of output stage             , */\
   { 0x5330, "icomp_engage_overrule"},    /* To overrule the functional icomp_engage signal during validation, */\
   { 0x5343, "ctrl_dem"},    /* Enable DEM icomp and DEM one bit DAC              , */\
   { 0x5381, "ref_amp_irefdist_set_ctrl"},    /* Scaling of reference current for amplifier OCP    , */\
   { 0x5400, "bypass_ctrlloop"},    /* Switch amplifier into open loop configuration     , */\
   { 0x5413, "ctrl_dem_mismatch"},    /* Enable DEM icomp mismatch for testing             , */\
   { 0x5452, "amp_pst_drive_ctrl"},    /* Amplifier powerstage drive control in direct control mode only, */\
   { 0x5481, "ctrlloop_vstress_select"},    /* GO2 capacitor stress selector for control loop    , */\
   { 0x54a0, "ctrl_slopectrl"},    /* Enables slope control                             , */\
   { 0x54b0, "ctrl_slope"},    /* Slope speed setting                               , */\
   { 0x54c0, "sel_icomp_dem_clk"},    /* Clock selection for icomp_dem                     , */\
   { 0x5600, "ref_iref_enbl"},    /* Enable of reference current for OCP               , */\
   { 0x5610, "ref_iref_test_enbl"},    /* Enable of test function of reference current      , */\
   { 0x5652, "ref_irefdist_test_enbl"},    /* Enable of test-function of distribution of reference current, used for OCP. When enabled, the current will to to anamux iso powerstages. Using e.g. 011 it will add the current of powerstage P and N., */\
   { 0x5705, "enbl_amp"},    /* Switch on the class-D power sections, each part of the analog sections can be switched on/off individually in I2C direct control mode, */\
   { 0x57b0, "enbl_engage"},    /* Enables/engage the control stage in I2C direct control mode, */\
   { 0x57c0, "enbl_engage_pst"},    /* Enables/engage the power stage in I2C direct control mode, */\
   { 0x5810, "hard_mute"},    /* Hard mute - PWM                                   , */\
   { 0x5844, "pwm_delay"},    /* PWM delay bits to set the delay, clock PWM is 1/(K*4096*fs), */\
   { 0x5890, "reclock_pwm"},    /* Reclock the PWM signal inside analog              , */\
   { 0x58c0, "enbl_pwm_phase_shift"},    /* Control for PWM phase shift                       , */\
   { 0x5910, "sel_pwm_delay_src"},    /* Control for selection for PWM delay line source   , */\
   { 0x5f63, "ctrl_attr"},    /* Total gain depending on INPLEV setting (channel 0), */\
   { 0x6005, "idle_power_cal_offset"},    /* Idle power mode detector ctrl cal_offset from gain module , */\
   { 0x6065, "idle_power_zero_lvl"},    /* IIdle power mode zero crossing detection level    , */\
   { 0x60e1, "idle_power_mode"},    /* Idle power mode control                           , */\
   { 0x6105, "idle_power_threshold_lvl"},    /* Idle power mode amplitude trigger level           , */\
   { 0x6165, "idle_power_hold_time"},    /* Idle power mode detector ctrl hold time before low audio is reckoned to be low audio, */\
   { 0x61c0, "disable_idle_power_mode"},    /* Idle power mode detector control                  , */\
   { 0x6265, "zero_lvl"},    /* Low noise gain switch zero trigger level          , */\
   { 0x62c1, "ctrl_fb_resistor"},    /* Select amplifier feedback resistor connection     , */\
   { 0x62e1, "lownoisegain_mode"},    /* Ctrl select mode                                  , */\
   { 0x6305, "threshold_lvl"},    /* Low noise gain switch trigger level               , */\
   { 0x6365, "hold_time"},    /* Low noise gain switch ctrl hold time before low audio is reckoned to be low audio, */\
   { 0x6405, "lpm1_cal_offset"},    /* Low power mode1 detector ctrl cal_offset from gain module , */\
   { 0x6465, "lpm1_zero_lvl"},    /* Low power mode1 zero crossing detection level     , */\
   { 0x64e1, "lpm1_mode"},    /* Low power mode control                            , */\
   { 0x6505, "lpm1_threshold_lvl"},    /* Low power mode1 amplitude trigger level           , */\
   { 0x6565, "lpm1_hold_time"},    /* Low power mode1 detector ctrl hold time before low audio is reckoned to be low audio, */\
   { 0x65c0, "disable_low_power_mode"},    /* Low power mode1 detector control                  , */\
   { 0x6611, "dcdc_ctrl_maxzercnt"},    /* DCDC Number of zero current flags to count before going to PFM mode, */\
   { 0x6656, "dcdc_vbat_delta_detect"},    /* Threshold before booster is reacting on a delta Vbat (in PFM mode) by temporarily switching to PWM mode, */\
   { 0x66c0, "dcdc_ignore_vbat"},    /* Ignore an increase on Vbat                        , */\
   { 0x66d2, "pfmfreq_limit"},    /* Lowest PFM frequency limit                        , */\
   { 0x6700, "enbl_minion"},    /* Enables minion (small) power stage in I2C direct control mode, */\
   { 0x6713, "vth_vddpvbat"},    /* Select vddp-vbat threshold signal                 , */\
   { 0x6750, "lpen_vddpvbat"},    /* Select vddp-vbat filtred vs unfiltered compare    , */\
   { 0x6761, "ctrl_rfb"},    /* Feedback resistor selection - I2C direct mode     , */\
   { 0x6802, "tdm_source_mapping"},    /* TDM source mapping                                , */\
   { 0x6831, "tdm_sourcea_frame_sel"},    /* Sensed value A                                    , */\
   { 0x6851, "tdm_sourceb_frame_sel"},    /* Sensed value B                                    , */\
   { 0x6871, "tdm_source0_clip_sel"},    /* Clip flag information for TDM Compressed2a/b/c modes, */\
   { 0x6a02, "rst_min_vbat_delay"},    /* rst_min_vbat delay (nb fs)                        , */\
   { 0x6b00, "disable_auto_engage"},    /* Disable auto engage                               , */\
   { 0x6b10, "disable_engage"},    /* Disable engage                                    , */\
   { 0x6c02, "ns_hp2ln_criterion"},    /* 0..7 zeroes at ns as threshold to swap from high_power to low_noise, */\
   { 0x6c32, "ns_ln2hp_criterion"},    /* 0..7 zeroes at ns as threshold to swap from low_noise to high_power, */\
   { 0x6c69, "spare_out"},    /* Spare control bits for future use                 , */\
   { 0x6d09, "spare_in"},    /* Spare control bit - read only                     , */\
   { 0x6e00, "flag_idle_power_mode"},    /* Idle power mode                                   , */\
   { 0x6e10, "flag_lp_detect_mode1"},    /* Low power mode 1 detection                        , */\
   { 0x6e20, "flag_low_amplitude"},    /* Low amplitude detection                           , */\
   { 0x6e30, "flag_vddp_gt_vbat"},    /* Vddp greater than Vbat                            , */\
   { 0x6f02, "cursense_comp_delay"},    /* Delay to allign compensation signal with current sense signal, */\
   { 0x6f40, "cursense_comp_sign"},    /* Polarity of compensation for current sense        , */\
   { 0x6f50, "enbl_cursense_comp"},    /* Enable current sense compensation                 , */\
   { 0x6f72, "pwms_clip_lvl"},    /* Set the amount of pwm pulse that may be skipped before clip-flag is triggered, */\
   { 0x7003, "boost_cur"},    /* Max coil current                                  , */\
   { 0x7041, "bst_slpcmplvl"},    /* Slope compensation current, represents LxF (inductance x frequency) , */\
   { 0x7060, "boost_intel"},    /* Adaptive boost mode                               , */\
   { 0x7070, "boost_speed"},    /* Soft ramp up/down                                 , */\
   { 0x7087, "overshoot_correction_lvl"},    /* Threshold level to activate active overshoot control, */\
   { 0x7104, "bst_drive"},    /* Binary coded drive setting for boost converter power stage, */\
   { 0x7151, "bst_scalecur"},    /* Scale factor for peak current measurement - ratio between reference current and power transistor current, */\
   { 0x7174, "bst_slopecur"},    /* For testing direct control slope current          , */\
   { 0x71c1, "bst_slope"},    /* Boost slope speed                                 , */\
   { 0x71e0, "bst_bypass_bstcur"},    /* Bypass control for boost current settings         , */\
   { 0x71f0, "bst_bypass_bstfoldback"},    /* Bypass control for boost foldback                 , */\
   { 0x7200, "enbl_bst_engage"},    /* Enable power stage dcdc controller in I2C direct control mode, */\
   { 0x7210, "enbl_bst_hizcom"},    /* Enable hiz comparator in I2C direct control mode  , */\
   { 0x7220, "enbl_bst_peak2avg"},    /* Enable boost peak2avg functionality               , */\
   { 0x7230, "enbl_bst_peakcur"},    /* Enable peak current in I2C direct control mode    , */\
   { 0x7240, "enbl_bst_power"},    /* Enable line of the powerstage in I2C direct control mode, */\
   { 0x7250, "enbl_bst_slopecur"},    /* Enable bit of max-current DAC in I2C direct control mode, */\
   { 0x7260, "enbl_bst_voutcomp"},    /* Enable vout comparators in I2C direct control mode, */\
   { 0x7270, "enbl_bst_voutcomp86"},    /* Enable vout-86 comparators in I2C direct control mode, */\
   { 0x7280, "enbl_bst_voutcomp93"},    /* Enable vout-93 comparators in I2C direct control mode, */\
   { 0x7290, "enbl_bst_windac"},    /* Enable window DAC in I2C direct control mode      , */\
   { 0x7300, "boost_alg"},    /* Control for boost adaptive loop gain              , */\
   { 0x7311, "boost_loopgain"},    /* DCDC boost loopgain setting                       , */\
   { 0x7331, "bst_freq"},    /* DCDC boost frequency control                      , */\
   { 0x7350, "disable_artf654484_fix"},    /* Disables the fix for artf654484 (loss of efficiency when Vbst is close to Vbat), */\
   { 0x7360, "disable_artf676996_fix"},    /* Disables the fix for artf676996 (OCP booster triggered when Vtrgt is just above Vbat), */\
   { 0x7371, "ref_bst_irefdist_set_ctrl"},    /* Scaling of reference current for booster OCP      , */\
   { 0x7400, "dcdc_disable_ns"},    /* Disable control of noise shaper in DCDC control   , */\
   { 0x7410, "dcdc_disable_mod8bit"},    /* Disable control of reset of noise shaper when 8 bit value for dcdc control occurs, */\
   { 0x7420, "disable_dynamic_freq"},    /* Disables the dynamic frequency switching due to flag_voutcomp86/93, */\
   { 0x7430, "boost_track"},    /* Boost algorithm selection, effective only when DCIE is set to 1, */\
   { 0x7444, "boost_trip_lvl_1st"},    /* Headroom for 1st Adaptive boost trip level, effective only when DCIE is set to 1 and DCTRACK is 0, */\
   { 0x7494, "boost_hold_time"},    /* Hold time for DCDC booster, effective only when DCIE is set to 1, */\
   { 0x74e0, "sel_dcdc_envelope_8fs"},    /* Selection of data for adaptive boost algorithm, effective only when DCIE is set to 1, */\
   { 0x74f0, "ignore_flag_voutcomp86"},    /* Determines the maximum PWM frequency be the most efficient in relation to the Booster inductor value, */\
   { 0x7500, "dcdcoff_mode"},    /* DCDC on/off                                       , */\
   { 0x7510, "dcdc_pwmonly"},    /* DCDC PWM only mode                                , */\
   { 0x7534, "boost_trip_lvl_2nd"},    /* Headroom for 2nd Adaptive boost trip level, effective only when DCIE is 1 and DCTRACK is 0, */\
   { 0x7584, "boost_trip_lvl_track"},    /* Headroom for Tracking Adaptive boost trip level, effective only when DCIE is 1 and DCTRACK is 1, */\
   { 0x75d0, "bypass_dcdc_lpf"},    /* Bypass control of DCDC control low pass filter for quantization noise suppression, */\
   { 0x75e0, "enbl_bst_filter"},    /* Enable the boost filter                           , */\
   { 0x75f0, "enbl_trip_hyst"},    /* Enable hysteresis on booster trip levels          , */\
   { 0x7607, "frst_boost_voltage"},    /* First boost voltage level                         , */\
   { 0x7687, "scnd_boost_voltage"},    /* Second boost voltage level                        , */\
   { 0x7707, "bst_windac"},    /* For testing direct control windac                 , */\
   { 0x8050, "cs_gain_control"},    /* Current sense gain control                        , */\
   { 0x8060, "cs_bypass_gc"},    /* Bypasses the CS gain correction                   , */\
   { 0x8087, "cs_gain"},    /* Current sense gain                                , */\
   { 0x8210, "invertpwm"},    /* Current sense common mode feedback pwm invert control, */\
   { 0x8305, "cs_ktemp"},    /* Current sense temperature compensation trimming (1 - VALUE*TEMP)*signal, */\
   { 0x8364, "cs_ktemp2"},    /* Second order temperature compensation coefficient , */\
   { 0x8400, "cs_adc_bsoinv"},    /* Bitstream inversion for current sense ADC         , */\
   { 0x8440, "cs_adc_nortz"},    /* Return to zero for current sense ADC              , */\
   { 0x8490, "cs_adc_slowdel"},    /* Select delay for current sense ADC (internal decision circuitry), */\
   { 0x8510, "cs_classd_tran_skip"},    /* Skip current sense connection during a classD amplifier transition, */\
   { 0x8530, "cs_inn_short"},    /* Short current sense negative to common mode       , */\
   { 0x8540, "cs_inp_short"},    /* Short current sense positive to common mode       , */\
   { 0x8550, "cs_ldo_bypass"},    /* Bypass current sense LDO                          , */\
   { 0x8560, "cs_ldo_pulldown"},    /* Pull down current sense LDO, only valid if left_enbl_cs_ldo is high, */\
   { 0x8574, "cs_ldo_voset"},    /* Current sense LDO voltage level setting (two's complement), */\
   { 0x8700, "enbl_cs_adc"},    /* Enable current sense ADC                          , */\
   { 0x8710, "enbl_cs_inn1"},    /* Enable connection of current sense negative1      , */\
   { 0x8720, "enbl_cs_inn2"},    /* Enable connection of current sense negative2      , */\
   { 0x8730, "enbl_cs_inp1"},    /* Enable connection of current sense positive1      , */\
   { 0x8740, "enbl_cs_inp2"},    /* Enable connection of current sense positive2      , */\
   { 0x8750, "enbl_cs_ldo"},    /* Enable current sense LDO                          , */\
   { 0x8780, "enbl_cs_vbatldo"},    /* Enable of current sense LDO                       , */\
   { 0x8790, "enbl_dc_filter"},    /* Control for enabling the DC blocking filter for voltage and current sense, */\
   { 0x87a0, "enbl_ana_pre"},    /* Control for enabling the pre-empasis filter for voltage and current sense decimator, */\
   { 0x8800, "vs1_adc_enbl"},    /* Enable voltage sense VS1 ADC (direct control mode only), */\
   { 0x8810, "vs1_inn_short_ctrl"},    /* Short voltage sense VS1 negative to common mode   , */\
   { 0x8820, "vs1_inp_short_ctrl"},    /* Short voltage sense VS1 positive to common mode   , */\
   { 0x8830, "vs1_ldo_enbl"},    /* Enable voltage sense VS1 LDO (direct control mode only), */\
   { 0x8840, "vs1_vbatldo_enbl"},    /* Enable voltage sense VS1 VBAT LDO (direct control mode only), */\
   { 0x8850, "vs1_gain_control"},    /* Voltage sense VS1 gain control                    , */\
   { 0x8860, "vs1_bypass_gc"},    /* Bypasses the VS1 gain correction                  , */\
   { 0x8870, "vs1_adc_bsoinv_ctrl"},    /* Bitstream inversion for voltage sense VS1 ADC     , */\
   { 0x8887, "vs1_gain"},    /* Voltage sense VS1 gain                            , */\
   { 0x8900, "vs2_adc_enbl"},    /* Enable voltage sense VS2 ADC (direct control mode only), */\
   { 0x8910, "vs2_inn_short_ctrl"},    /* Short voltage sense VS2 negative to common mode   , */\
   { 0x8920, "vs2_inp_short_ctrl"},    /* Short voltage sense VS2 positive to common mode   , */\
   { 0x8930, "vs2_ldo_enbl"},    /* Enable voltage sense VS2 LDO (direct control mode only), */\
   { 0x8940, "vs2_vbatldo_enbl"},    /* Enable voltage sense VS2 VBAT LDO (direct control mode only), */\
   { 0x8950, "vs2_gain_control"},    /* Voltage sense VS2 gain control                    , */\
   { 0x8960, "vs2_bypass_gc"},    /* Bypasses the VS2 gain correction                  , */\
   { 0x8970, "vs2_adc_bsoinv_ctrl"},    /* Bitstream inversion for voltage sense VS1 ADC     , */\
   { 0x8987, "vs2_gain"},    /* Voltage sense VS2 gain                            , */\
   { 0x8a00, "vs_adc_delay_ctrl"},    /* Select delay for voltage sense ADC (internal decision circuitry), */\
   { 0x8a10, "vs_adc_nortz_ctrl"},    /* Return to zero for voltage sense ADC              , */\
   { 0x8a20, "vs_ldo_bypass_ctrl"},    /* Bypass voltage sense LDO                          , */\
   { 0x8a30, "vs_ldo_pulldown_ctrl"},    /* Pull down voltage sense LDO, only valid if left_enbl_cs_ldo is high, */\
   { 0x8a44, "vs_ldo_voset_ctrl"},    /* Voltage sense LDO voltage level setting (two's complement), */\
   { 0x8a90, "vs_anamux_overrule"},    /* Overrule the disconnection of VS pins             , */\
   { 0xa007, "mtpkey1"},    /* 5Ah, 90d To access KEY1_protected registers (default for engineering), */\
   { 0xa107, "mtpkey2"},    /* MTP KEY2 register                                 , */\
   { 0xa200, "key01_locked"},    /* Indicates KEY1 is locked                          , */\
   { 0xa210, "key02_locked"},    /* Indicates KEY2 is locked                          , */\
   { 0xa302, "mtp_man_address_in"},    /* MTP address from I2C register for read/writing mtp in manual single word mode, */\
   { 0xa330, "man_copy_mtp_to_iic"},    /* Start copying single word from mtp to I2C mtp register, */\
   { 0xa340, "man_copy_iic_to_mtp"},    /* Start copying single word from I2C mtp register to mtp, */\
   { 0xa350, "auto_copy_mtp_to_iic"},    /* Start copying all the data from mtp to I2C mtp registers, */\
   { 0xa360, "auto_copy_iic_to_mtp"},    /* Start copying data from I2C mtp registers to mtp  , */\
   { 0xa400, "faim_set_clkws"},    /* Sets the faim controller clock wait state register, */\
   { 0xa410, "faim_sel_evenrows"},    /* All even rows of the faim are selected, active high, */\
   { 0xa420, "faim_sel_oddrows"},    /* All odd rows of the faim are selected, all rows in combination with sel_evenrows, */\
   { 0xa430, "faim_program_only"},    /* Skip the erase access at wr_faim command (write-program-marginread), */\
   { 0xa440, "faim_erase_only"},    /* Skip the program access at wr_faim command (write-erase-marginread), */\
   { 0xa50f, "mtp_man_data_out_msb"},    /* MSB word of MTP manual read data                  , */\
   { 0xa60f, "mtp_man_data_out_lsb"},    /* LSB word of MTP manual read data                  , */\
   { 0xa70f, "mtp_man_data_in_msb"},    /* MSB word of write data for MTP manual write       , */\
   { 0xa80f, "mtp_man_data_in_lsb"},    /* LSB word of write data for MTP manual write       , */\
   { 0xb010, "bypass_ocpcounter"},    /* Bypass OCP Counter                                , */\
   { 0xb020, "bypass_glitchfilter"},    /* Bypass glitch filter                              , */\
   { 0xb030, "bypass_ovp"},    /* Bypass OVP                                        , */\
   { 0xb050, "bypass_otp"},    /* Bypass OTP                                        , */\
   { 0xb060, "bypass_lost_clk"},    /* Bypass lost clock detector                        , */\
   { 0xb070, "ctrl_vpalarm"},    /* vpalarm (uvp ovp handling)                        , */\
   { 0xb087, "ocp_threshold"},    /* OCP threshold level                               , */\
   { 0xb108, "ext_temp"},    /* External temperature (C)                          , */\
   { 0xb190, "ext_temp_sel"},    /* Select temp Speaker calibration                   , */\
   { 0xc000, "use_direct_ctrls"},    /* Direct control to overrule several functions for testing, */\
   { 0xc010, "rst_datapath"},    /* Direct control for datapath reset                 , */\
   { 0xc020, "rst_cgu"},    /* Direct control for cgu reset                      , */\
   { 0xc038, "enbl_ref"},    /* Switch on the analog references, each part of the references can be switched on/off individually, */\
   { 0xc0d0, "enbl_ringo"},    /* Enable the ring oscillator for test purpose       , */\
   { 0xc0e0, "enbl_fro"},    /* Enables FRO8M in I2C direct control mode only     , */\
   { 0xc0f0, "bod_enbl"},    /* Enable BOD (only in direct control mode)          , */\
   { 0xc100, "enbl_tsense"},    /* Temperature sensor enable control - I2C direct mode, */\
   { 0xc110, "tsense_hibias"},    /* Bit to set the biasing in temp sensor to high     , */\
   { 0xc120, "enbl_flag_vbg"},    /* Enable flagging of bandgap out of control         , */\
   { 0xc20f, "abist_offset"},    /* Offset control for ABIST testing (two's complement), */\
   { 0xc300, "bypasslatch"},    /* Bypass latch                                      , */\
   { 0xc311, "sourcea"},    /* Set OUTA to                                       , */\
   { 0xc331, "sourceb"},    /* Set OUTB to                                       , */\
   { 0xc350, "inverta"},    /* Invert pwma test signal                           , */\
   { 0xc360, "invertb"},    /* Invert pwmb test signal                           , */\
   { 0xc376, "pulselength"},    /* Pulse length setting test input for amplifier (PWM clock 2048/4096 Fs), */\
   { 0xc3e0, "tdm_enable_loopback"},    /* TDM loopback test                                 , */\
   { 0xc400, "bst_bypasslatch"},    /* Bypass latch in boost converter                   , */\
   { 0xc411, "bst_source"},    /* Sets the source of the pwmbst output to boost converter input for testing, */\
   { 0xc430, "bst_invertb"},    /* Invert PWMbst test signal                         , */\
   { 0xc444, "bst_pulselength"},    /* Pulse length setting test input for boost converter , */\
   { 0xc490, "test_bst_ctrlsthv"},    /* Test mode for boost control stage                 , */\
   { 0xc4a0, "test_bst_iddq"},    /* IDDQ testing in power stage of boost converter    , */\
   { 0xc4b0, "test_bst_rdson"},    /* RDSON testing - boost power stage                 , */\
   { 0xc4c0, "test_bst_cvi"},    /* CVI testing - boost power stage                   , */\
   { 0xc4d0, "test_bst_ocp"},    /* Boost OCP. For old OCP (ctrl_reversebst is 0), For new OCP (ctrl_reversebst is 1), */\
   { 0xc4e0, "test_bst_sense"},    /* Test option for the sense NMOS in booster for current mode control., */\
   { 0xc500, "test_cvi"},    /* Analog BIST, switch choose which transistor will be used as current source (also cross coupled sources possible), */\
   { 0xc510, "test_discrete"},    /* Test function noise measurement                   , */\
   { 0xc520, "test_iddq"},    /* Set the power stages in iddq mode for gate stress., */\
   { 0xc540, "test_rdson"},    /* Analog BIST, switch to enable Rdson measurement   , */\
   { 0xc550, "test_sdelta"},    /* Analog BIST, noise test                           , */\
   { 0xc570, "cs_test_enbl"},    /* Enable for digimux mode of current sense          , */\
   { 0xc580, "vs1_test_enbl"},    /* Enable for digimux mode of voltage sense          , */\
   { 0xc590, "vs2_test_enbl"},    /* Enable for digimux mode of dual sense             , */\
   { 0xc600, "enbl_pwm_dcc"},    /* Enables direct control of pwm duty cycle for DCDC power stage, */\
   { 0xc613, "pwm_dcc_cnt"},    /* Control pwm duty cycle when enbl_pwm_dcc is 1     , */\
   { 0xc650, "enbl_ldo_stress"},    /* Enable stress of internal supply voltages powerstages, */\
   { 0xc707, "digimuxa_sel"},    /* DigimuxA input selection control routed to DATAO (see Digimux list for details), */\
   { 0xc787, "digimuxb_sel"},    /* DigimuxB input selection control routed to INT (see Digimux list for details), */\
   { 0xc807, "digimuxc_sel"},    /* DigimuxC input selection control routed to ADS1 (see Digimux list for details), */\
   { 0xc981, "int_ehs"},    /* Speed/load setting for INT IO cell, clk or data mode range (see SLIMMF IO cell datasheet), */\
   { 0xc9c0, "hs_mode"},    /* I2C high speed mode control                       , */\
   { 0xca00, "enbl_anamux1"},    /* Enable anamux1                                    , */\
   { 0xca10, "enbl_anamux2"},    /* Enable anamux2                                    , */\
   { 0xca20, "enbl_anamux3"},    /* Enable anamux3                                    , */\
   { 0xca30, "enbl_anamux4"},    /* Enable anamux4                                    , */\
   { 0xca74, "anamux1"},    /* Anamux selection control - anamux on TEST1        , */\
   { 0xcb04, "anamux2"},    /* Anamux selection control - anamux on TEST2        , */\
   { 0xcb53, "anamux3"},    /* Anamux selection control - anamux on VSN/TEST3    , */\
   { 0xcba3, "anamux4"},    /* Anamux selection control - anamux on VSP/TEST4    , */\
   { 0xcd05, "pll_seli"},    /* PLL seli bandwidth control in PLL direct control mode only, use_direct_pll_ctrl set to 1, */\
   { 0xcd64, "pll_selp"},    /* PLL selp bandwidth control in PLL direct control mode only, use_direct_pll_ctrl set to 1, */\
   { 0xcdb3, "pll_selr"},    /* PLL selr bandwidth control in PLL direct control mode only, use_direct_pll_ctrl set to 1, */\
   { 0xcdf0, "pll_band_direct"},    /* PLL bandwidth selection direct control, USE WITH CAUTION, */\
   { 0xce00, "pll_frm"},    /* PLL free running mode control in functional mode  , */\
   { 0xce10, "pll_directi"},    /* PLL directi control in PLL direct control mode only, use_direct_pll_ctrl set to 1, */\
   { 0xce20, "pll_directo"},    /* PLL directo control in PLL direct control mode only, use_direct_pll_ctrl set to 1, */\
   { 0xce34, "pll_pdiv"},    /* PLL PDIV in PLL direct control mode only, use_direct_pll_ctrl set to 1, */\
   { 0xce87, "pll_ndiv"},    /* PLL NDIV in PLL direct control mode only, use_direct_pll_ctrl set to 1, */\
   { 0xcf0f, "pll_mdiv"},    /* PLL MDIV in PLL direct control mode only, use_direct_pll_ctrl set to 1, */\
   { 0xd000, "use_direct_pll_ctrl"},    /* Enable PLL direct control mode, overrules the PLL LUT with I2C register values, */\
   { 0xd010, "enbl_pll"},    /* Enables PLL in PLL direct control mode, use_direct_pll_ctrl set to 1, */\
   { 0xd020, "pll_limup_off"},    /* PLL up limiter control in PLL direct bandwidth control mode, */\
   { 0xd030, "pll_frm_clockstable"},    /* PLL FRM clock stable in pll direct control mode, use_direct_pll_ctrl set to 1, */\
   { 0xd042, "sel_pll_startup_time"},    /* PLL startup time selection control                , */\
   { 0xd10f, "tsig_freq_lsb"},    /* Internal sinus test generator frequency control   , */\
   { 0xd202, "tsig_freq_msb"},    /* Internal sinus test generator, frequency control msb bits, */\
   { 0xd230, "inject_tsig"},    /* Control bit to switch to internal sinus test generator, */\
   { 0xd283, "tsig_gain"},    /* Test signal gain                                  , */\
   { 0xd300, "adc10_reset"},    /* Reset for ADC10 - I2C direct control mode         , */\
   { 0xd311, "adc10_test"},    /* Test mode selection signal for ADC10 - I2C direct control mode, */\
   { 0xd332, "adc10_sel"},    /* Select the input to convert for ADC10 - I2C direct control mode, */\
   { 0xd364, "adc10_prog_sample"},    /* ADC10 program sample setting - I2C direct control mode, */\
   { 0xd3b0, "adc10_enbl"},    /* Enable ADC10 - I2C direct control mode            , */\
   { 0xd3c0, "bypass_lp_vbat"},    /* Bypass control for Low pass filter in batt sensor , */\
   { 0xd409, "data_adc10_tempbat"},    /* ADC 10 data output data for testing               , */\
   { 0xd507, "ctrl_digtoana_hidden"},    /* Spare digital to analog control bits - Hidden     , */\
   { 0xd580, "enbl_clk_out_of_range"},    /* Clock out of range checker                        , */\
   { 0xd721, "datao_ehs"},    /* Speed/load setting for DATAO IO cell, clk or data mode range (see SLIMMF IO cell datasheet), */\
   { 0xd740, "bck_ehs"},    /* High-speed and standard/fast mode selection for BCK IO cell (see IIC3V3 IO cell datasheet), */\
   { 0xd750, "datai_ehs"},    /* High-speed and standard/fast mode selection for DATAI IO cell (see IIC3V3 IO cell datasheet), */\
   { 0xd800, "source_in_testmode"},    /* TDM source in test mode (return only current and voltage sense), */\
   { 0xd822, "test_parametric_io"},    /* Test io parametric                                , */\
   { 0xd861, "test_spare_out1"},    /* Test spare out 1                                  , */\
   { 0xd880, "bst_dcmbst"},    /* DCM boost                                         , */\
   { 0xd8c3, "test_spare_out2"},    /* Test spare out 1                                  , */\
   { 0xd900, "enbl_frocal"},    /* Enable FRO calibration                            , */\
   { 0xd910, "start_fro_calibration"},    /* Start FRO8 calibration                            , */\
   { 0xda00, "fro_calibration_done"},    /* FRO8 calibration done - Read Only                 , */\
   { 0xda15, "fro_auto_trim_val"},    /* Calibration value from auto calibration, to be written into MTP - Read Only, */\
   { 0xe00f, "sw_profile"},    /* Software profile data                             , */\
   { 0xe10f, "sw_vstep"},    /* Software vstep information                        , */\
   { 0xf000, "calibration_onetime"},    /* Calibration schedule                              , */\
   { 0xf010, "calibr_ron_done"},    /* Calibration Ron executed                          , */\
   { 0xf020, "calibr_dcdc_api_calibrate"},    /* Calibration current limit DCDC                    , */\
   { 0xf030, "calibr_dcdc_delta_sign"},    /* Sign bit for delta calibration current limit DCDC , */\
   { 0xf042, "calibr_dcdc_delta"},    /* Calibration delta current limit DCDC              , */\
   { 0xf078, "calibr_speaker_info"},    /* Reserved space for allowing customer to store speaker information, */\
   { 0xf107, "calibr_vout_offset"},    /* DCDC offset calibration 2's complement (key1 protected), */\
   { 0xf203, "calibr_gain"},    /* HW gain module (2's complement)                   , */\
   { 0xf245, "calibr_offset"},    /* Offset for amplifier, HW gain module (2's complement), */\
   { 0xf307, "calibr_vs1_gain"},    /* Voltage sense VS1 gain calibration                , */\
   { 0xf387, "calibr_vs2_gain"},    /* Voltage sense VS2 gain calibration                , */\
   { 0xf407, "calibr_vs1_trim"},    /* VS1 ADC trimming                                  , */\
   { 0xf487, "calibr_vs2_trim"},    /* VS2 ADC Trimming                                  , */\
   { 0xf50f, "calibr_R25C_R"},    /* Ron resistance of speaker coil                    , */\
   { 0xf607, "calibr_gain_cs"},    /* Current sense gain (signed two's complement format), */\
   { 0xf706, "ctrl_offset_a"},    /* Offset of level shifter A                         , */\
   { 0xf786, "ctrl_offset_b"},    /* Offset of amplifier level shifter B               , */\
   { 0xf806, "htol_iic_addr"},    /* 7-bit I2C address to be used during HTOL testing  , */\
   { 0xf870, "htol_iic_addr_en"},    /* HTOL I2C address enable control                   , */\
   { 0xf884, "calibr_temp_offset"},    /* Temperature offset 2's compliment (key1 protected), */\
   { 0xf8d2, "calibr_temp_gain"},    /* Temperature gain 2's compliment (key1 protected)  , */\
   { 0xf900, "mtp_lock_dcdcoff_mode"},    /* Disable function dcdcoff_mode                     , */\
   { 0xf920, "mtp_lock_bypass_clipper"},    /* Disable function bypass_clipper                   , */\
   { 0xf930, "mtp_lock_max_dcdc_voltage"},    /* Force Boost in follower mode                      , */\
   { 0xf943, "calibr_vbg_trim"},    /* Bandgap trimming control                          , */\
   { 0xfa0f, "mtpdataA"},    /* MTPdataA (key1 protected)                         , */\
   { 0xfb0f, "mtpdataB"},    /* MTPdataB (key1 protected)                         , */\
   { 0xfc0f, "mtpdataC"},    /* MTPdataC (key1 protected)                         , */\
   { 0xfd0f, "mtpdataD"},    /* MTPdataD (key1 protected)                         , */\
   { 0xfe0f, "mtpdataE"},    /* MTPdataE (key1 protected)                         , */\
   { 0xff05, "fro_trim"},    /* 8 MHz oscillator trim code                        , */\
   { 0xff61, "fro_shortnwell"},    /* Short 4 or 6 n-well resistors                     , */\
   { 0xff81, "fro_boost"},    /* Self bias current selection                       , */\
   { 0xffa4, "calibr_iref_trim"},    /* Trimming control of reference current for OCP     , */\
   { 0xffff,"Unknown bitfield enum" }    /* not found */\
};

enum tfa9875_irq {
	tfa9875_irq_max = -1,
	tfa9875_irq_all = -1 /* all irqs */};

#define TFA9875_IRQ_NAMETABLE static tfaIrqName_t Tfa9875IrqNames[]= {\
};
#endif /* _TFA9875_TFAFIELDNAMES_H */
