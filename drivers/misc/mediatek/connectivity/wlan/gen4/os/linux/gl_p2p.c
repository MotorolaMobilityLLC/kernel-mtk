/*
** Id: @(#) gl_p2p.c@@
*/

/*! \file   gl_p2p.c
*    \brief  Main routines of Linux driver interface for Wi-Fi Direct
*
*    This file contains the main routines of Linux driver for MediaTek Inc. 802.11
*    Wireless LAN Adapters.
*/

/*
** Log: gl_p2p.c
**
** 04 14 2014 eason.tsai
** [ALPS01510349] [6595][KK][HotKnot][Reboot][KE][p2pDevFsmRunEventScanDone]Sender
** reboot automatically with KE about p2pDevFsmRunEventScanDone.
** fix KE
**
** 01 23 2014 eason.tsai
** [ALPS01070904] [Need Patch] [Volunteer Patch][MT6630][Driver]MT6630 Wi-Fi Patch
** fix ap mode
**
** 07 31 2013 yuche.tsai
** [BORA00002398] [MT6630][Volunteer Patch] P2P Driver Re-Design for Multiple BSS support
** Change private data of net device.
**
** 07 30 2013 yuche.tsai
** [BORA00002398] [MT6630][Volunteer Patch] P2P Driver Re-Design for Multiple BSS support
** Temp fix Hot-spot data path issue.
**
** 07 30 2013 yuche.tsai
** [BORA00002398] [MT6630][Volunteer Patch] P2P Driver Re-Design for Multiple BSS support
** MT6630 Driver Update for Hot-Spot.
**
** 07 29 2013 cp.wu
** [BORA00002725] [MT6630][Wi-Fi] Add MGMT TX/RX support for Linux port
** Preparation for porting remain_on_channel support
**
** 07 26 2013 terry.wu
** [BORA00002207] [MT6630 Wi-Fi] TXM & MQM Implementation
** 1. Reduce extra Tx frame header parsing
** 2. Add TX port control
** 3. Add net interface to BSS binding
**
** 07 19 2013 yuche.tsai
** [BORA00002398] [MT6630][Volunteer Patch] P2P Driver Re-Design for Multiple BSS support
** Code update for P2P.
**
** 07 17 2013 yuche.tsai
** [BORA00002398] [MT6630][Volunteer Patch] P2P Driver Re-Design for Multiple BSS support
** MT6630 P2P first connection check point 1.
**
** 02 27 2013 yuche.tsai
** [BORA00002398] [MT6630][Volunteer Patch] P2P Driver Re-Design for Multiple BSS support
** Add new code, fix compile warning.
**
** 01 23 2013 terry.wu
** [BORA00002207] [MT6630 Wi-Fi] TXM & MQM Implementation
** Refine net dev implementation
**
** 01 21 2013 terry.wu
** [BORA00002207] [MT6630 Wi-Fi] TXM & MQM Implementation
** Update TX path based on new ucBssIndex modifications.
**
** 01 17 2013 cm.chang
** [BORA00002149] [MT6630 Wi-Fi] Initial software development
** Use ucBssIndex to replace eNetworkTypeIndex
**
** 09 17 2012 cm.chang
** [BORA00002149] [MT6630 Wi-Fi] Initial software development
** Duplicate source from MT6620 v2.3 driver branch
** (Davinci label: MT6620_WIFI_Driver_V2_3_120913_1942_As_MT6630_Base)
**
** 08 24 2012 yuche.tsai
** NULL
** Fix bug of invitation request.
**
** 08 17 2012 yuche.tsai
** NULL
** Fix compile warning.
**
** 08 16 2012 yuche.tsai
** NULL
** Fix compile warning.
**
** 08 15 2012 yuche.tsai
** NULL
** Fix compile warning.
**
** 07 31 2012 yuche.tsai
** NULL
** Update Active/Deactive network policy for P2P network.
** Highly related to power saving.
**
** 07 24 2012 yuche.tsai
** NULL
** Bug fix for JB.
**
** 07 20 2012 yuche.tsai
** [WCXRP00001119] [Volunteer Patch][WiFi Direct][Driver] Connection Policy Set for WFD SIGMA test
** Support remain on channel.
**
** 07 19 2012 yuche.tsai
** NULL
** Update code for net device register.
**
** 07 19 2012 yuche.tsai
** NULL
** Code update for JB.
 *
 * 07 17 2012 yuche.tsai
 * NULL
 * Fix compile error for JB.
 *
 * 07 17 2012 yuche.tsai
 * NULL
 * Let netdev bring up.
 *
 * 07 17 2012 yuche.tsai
 * NULL
 * Compile no error before trial run.
 *
 * 01 09 2012 terry.wu
 * [WCXRP00001166] [Wi-Fi] [Driver] cfg80211 integration for p2p newtork
 * cfg80211 integration for p2p network.
 *
 * 12 19 2011 terry.wu
 * [WCXRP00001142] [Wi-Fi] [P2P Driver] XOR local admin bit to generate p2p net device MAC
 * XOR local administrated bit to generate net device MAC of p2p network.
 *
 * 12 02 2011 yuche.tsai
 * NULL
 * Fix possible KE when unload p2p.
 *
 * 11 24 2011 yuche.tsai
 * NULL
 * Fix P2P IOCTL of multicast address bug, add low power driver stop control.
 *
 * 11 22 2011 yuche.tsai
 * NULL
 * Update RSSI link quality of P2P Network query method. (Bug fix)
 *
 * 11 19 2011 yuche.tsai
 * NULL
 * Add RSSI support for P2P network.
 *
 * 11 16 2011 yuche.tsai
 * [WCXRP00001107] [Volunteer Patch][Driver] Large Network Type index assert in FW issue.
 * Avoid using work thread in set p2p multicast address callback.
 *
 * 11 11 2011 yuche.tsai
 * NULL
 * Fix work thread cancel issue.
 *
 * 11 11 2011 yuche.tsai
 * NULL
 * Fix default device name issue.
 *
 * 11 08 2011 yuche.tsai
 * [WCXRP00001094] [Volunteer Patch][Driver] Driver version & supplicant version query & set support for service
 * discovery version check.
 * Add support for driver version query & p2p supplicant verseion set.
 * For new service discovery mechanism sync.
 *
 * 11 07 2011 yuche.tsai
 * NULL
 * [ALPS 00087243] KE in worker thread.
 * The multicast address list is scheduled in worker thread.
 * Before the worker thread is excuted, if P2P is unloaded, a KE may occur.
 *
 * 10 26 2011 terry.wu
 * [WCXRP00001066] [MT6620 Wi-Fi] [P2P Driver] Fix P2P Oid Issue
 * Fix some P2P OID functions didn't raise its flag "fgIsP2pOid" issue.
 *
 * 10 25 2011 cm.chang
 * [WCXRP00001058] [All Wi-Fi][Driver] Fix sta_rec's phyTypeSet and OBSS scan in AP mode
 * .
 *
 * 10 18 2011 yuche.tsai
 * [WCXRP00001045] [WiFi Direct][Driver] Check 2.1 branch.
 * Support Channel Query.
 *
 * 10 18 2011 yuche.tsai
 * [WCXRP00001045] [WiFi Direct][Driver] Check 2.1 branch.
 * New 2.1 branch

 *
 * 08 26 2011 yuche.tsai
 * NULL
 * Fix bug of parsing secondary device list type issue.
 *
 * 08 24 2011 yuche.tsai
 * [WCXRP00000919] [Volunteer Patch][WiFi Direct][Driver] Invitation New Feature.
 * Invitation Abort.
 *
 * 08 23 2011 yuche.tsai
 * NULL
 * Fix multicast address list issue of P2P.
 *
 * 08 22 2011 chinglan.wang
 * NULL
 * Fix invitation indication bug..
 *
 * 08 16 2011 cp.wu
 * [WCXRP00000934] [MT6620 Wi-Fi][Driver][P2P] Wi-Fi hot spot with auto sparse channel residence
 * auto channel decision for 2.4GHz hot spot mode
 *
 * 08 16 2011 chinglan.wang
 * NULL
 * Add the group id information in the invitation indication.
 *
 * 08 09 2011 yuche.tsai
 * [WCXRP00000919] [Volunteer Patch][WiFi Direct][Driver] Invitation New Feature.
 * Invitation Feature add on.
 *
 * 08 05 2011 yuche.tsai
 * [WCXRP00000856] [Volunteer Patch][WiFi Direct][Driver] MT6620 WiFi Direct IOT Issue with BCM solution.
 * Add Password ID check for quick connection.
 * Also modify some connection policy.
 *
 * 07 18 2011 chinglan.wang
 * NULL
 * Add IOC_P2P_GO_WSC_IE (p2p capability).
 *
 * 06 14 2011 yuche.tsai
 * NULL
 * Add compile flag to disable persistent group support.
 *
 * 05 04 2011 chinglan.wang
 * [WCXRP00000698] [MT6620 Wi-Fi][P2P][Driver] Add p2p invitation command for the p2p driver
 * .
 *
 * 05 02 2011 yuche.tsai
 * [WCXRP00000693] [Volunteer Patch][MT6620][Driver] Clear Formation Flag after TX lifetime timeout.
 * Clear formation flag after formation timeout.
 *
 * 04 22 2011 george.huang
 * [WCXRP00000621] [MT6620 Wi-Fi][Driver] Support P2P supplicant to set power mode
 * .
 *
 * 04 21 2011 george.huang
 * [WCXRP00000621] [MT6620 Wi-Fi][Driver] Support P2P supplicant to set power mode
 * 1. Revise P2P power mode setting.
 * 2. Revise fast-PS for concurrent
 *
 * 04 19 2011 wh.su
 * NULL
 * Adding length check before doing WPA RSN IE parsing for scan results indicate.
 *
 * 04 14 2011 yuche.tsai
 * [WCXRP00000646] [Volunteer Patch][MT6620][FW/Driver] Sigma Test Modification for some test case.
 * Connection flow refine for Sigma test.
 *
 * 04 08 2011 yuche.tsai
 * [WCXRP00000624] [Volunteer Patch][MT6620][Driver] Add device discoverability support for GO.
 * Add device discoverability support.
 *
 * 04 08 2011 george.huang
 * [WCXRP00000621] [MT6620 Wi-Fi][Driver] Support P2P supplicant to set power mode
 * separate settings of P2P and AIS
 *
 * 04 07 2011 terry.wu
 * [WCXRP00000619] [MT6620 Wi-Fi][Driver] fix kernel panic may occur when removing wlan
 * Fix kernel panic may occur when removing wlan driver.
 *
 * 03 31 2011 wh.su
 * [WCXRP00000614] [MT6620 Wi-Fi][Driver] P2P: Update beacon content while setting WSC IE
 * Update the wsc ie to beacon content.
 *
 * 03 25 2011 wh.su
 * NULL
 * add the sample code for set power mode and get power mode.
 *
 * 03 25 2011 yuche.tsai
 * NULL
 * Improve some error handleing.
 *
 * 03 22 2011 george.huang
 * [WCXRP00000504] [MT6620 Wi-Fi][FW] Support Sigma CAPI for power saving related command
 * link with supplicant commands
 *
 * 03 22 2011 yuche.tsai
 * [WCXRP00000584] [Volunteer Patch][MT6620][Driver] Add beacon timeout support for WiFi Direct.
 * Modify formation policy.
 *
 * 03 22 2011 yuche.tsai
 * NULL
 * Modify formation policy setting.
 *
 * 03 18 2011 yuche.tsai
 * [WCXRP00000574] [Volunteer Patch][MT6620][Driver] Modify P2P FSM Connection Flow
 * Modify connection flow after Group Formation Complete, or device connect to a GO.
 * Instead of request channel & connect directly, we use scan to allocate channel bandwidth & connect after RX BCN.
 *
 * 03 15 2011 wh.su
 * [WCXRP00000563] [MT6620 Wi-Fi] [P2P] Set local config method while set password Id ready
 * set lccal config method method while set password Id ready.
 *
 * 03 15 2011 yuche.tsai
 * [WCXRP00000560] [Volunteer Patch][MT6620][Driver] P2P Connection from UI using KEY/DISPLAY issue
 * Fix some configure method issue.
 *
 * 03 15 2011 jeffrey.chang
 * [WCXRP00000558] [MT6620 Wi-Fi][MT6620 Wi-Fi][Driver] refine the queue selection algorithm for WMM
 * refine queue_select function
 *
 * 03 13 2011 wh.su
 * [WCXRP00000530] [MT6620 Wi-Fi] [Driver] skip doing p2pRunEventAAAComplete after send assoc response Tx Done
 * add code for avoid compiling warning.
 *
 * 03 10 2011 yuche.tsai
 * NULL
 * Add P2P API.
 *
 * 03 10 2011 terry.wu
 * [WCXRP00000505] [MT6620 Wi-Fi][Driver/FW] WiFi Direct Integration
 * Remove unnecessary assert and message.
 *
 * 03 08 2011 wh.su
 * [WCXRP00000488] [MT6620 Wi-Fi][Driver] Support the SIGMA set p2p parameter to driver
 * support the power save related p2p setting.
 *
 * 03 07 2011 wh.su
 * [WCXRP00000506] [MT6620 Wi-Fi][Driver][FW] Add Security check related code
 * rename the define to anti_pviracy.
 *
 * 03 05 2011 wh.su
 * [WCXRP00000506] [MT6620 Wi-Fi][Driver][FW] Add Security check related code
 * add the code to get the check rsponse and indicate to app.
 *
 * 03 03 2011 jeffrey.chang
 * [WCXRP00000512] [MT6620 Wi-Fi][Driver] modify the net device relative functions to support the H/W multiple queue
 * support concurrent network
 *
 * 03 03 2011 jeffrey.chang
 * [WCXRP00000512] [MT6620 Wi-Fi][Driver] modify the net device relative functions to support the H/W multiple queue
 * modify P2P's netdevice  functions to support multiple H/W queues
 *
 * 03 03 2011 cp.wu
 * [WCXRP00000283] [MT6620 Wi-Fi][Driver][Wi-Fi Direct] Implementation of interface for supporting Wi-Fi Direct Service
 * Discovery
 * for get request, the buffer length to be copied is header + payload.
 *
 * 03 02 2011 wh.su
 * [WCXRP00000506] [MT6620 Wi-Fi][Driver][FW] Add Security check related code
 * add code to let the beacon and probe response for Auto GO WSC .
 *
 * 03 02 2011 cp.wu
 * [WCXRP00000283] [MT6620 Wi-Fi][Driver][Wi-Fi Direct] Implementation of interface for supporting Wi-Fi Direct Service
 * Discovery
 * add a missed break.
 *
 * 03 01 2011 yuche.tsai
 * [WCXRP00000501] [Volunteer Patch][MT6620][Driver] No common channel issue when doing GO formation
 * Update channel issue when doing GO formation..
 *
 * 02 25 2011 wh.su
 * [WCXRP00000488] [MT6620 Wi-Fi][Driver] Support the SIGMA set p2p parameter to driver
 * add the Operation channel setting.
 *
 * 02 23 2011 wh.su
 * [WCXRP00000488] [MT6620 Wi-Fi][Driver] Support the SIGMA set p2p parameter to driver
 * fixed the set int ioctl set index and value map to driver issue.
 *
 * 02 22 2011 wh.su
 * [WCXRP00000488] [MT6620 Wi-Fi][Driver] Support the SIGMA set p2p parameter to driver
 * adding the ioctl set int from supplicant, and can used to set the p2p parameters
 *
 * 02 21 2011 terry.wu
 * [WCXRP00000476] [MT6620 Wi-Fi][Driver] Clean P2P scan list while removing P2P
 * Clean P2P scan list while removing P2P.
 *
 * 02 18 2011 wh.su
 * [WCXRP00000471] [MT6620 Wi-Fi][Driver] Add P2P Provison discovery append Config Method attribute at WSC IE
 * fixed the ioctl setting that index not map to spec defined config method.
 *
 * 02 17 2011 wh.su
 * [WCXRP00000471] [MT6620 Wi-Fi][Driver] Add P2P Provison discovery append Config Method attribute at WSC IE
 * append the WSC IE config method attribute at provision discovery request.
 *
 * 02 17 2011 wh.su
 * [WCXRP00000448] [MT6620 Wi-Fi][Driver] Fixed WSC IE not send out at probe request
 * modify the structure pointer for set WSC IE.
 *
 * 02 16 2011 wh.su
 * [WCXRP00000448] [MT6620 Wi-Fi][Driver] Fixed WSC IE not send out at probe request
 * fixed the probe request send out without WSC IE issue (at P2P).
 *
 * 02 09 2011 cp.wu
 * [WCXRP00000283] [MT6620 Wi-Fi][Driver][Wi-Fi Direct] Implementation of interface for supporting Wi-Fi Direct Service
 * Discovery
 * fix typo
 *
 * 02 09 2011 yuche.tsai
 * [WCXRP00000431] [Volunteer Patch][MT6620][Driver] Add MLME support for deauthentication under AP(Hot-Spot) mode.
 * Add Support for MLME deauthentication for Hot-Spot.
 *
 * 01 25 2011 terry.wu
 * [WCXRP00000393] [MT6620 Wi-Fi][Driver] Add new module insert parameter
 * Add a new module parameter to indicate current runnig mode, P2P or AP.
 *
 * 01 12 2011 yuche.tsai
 * [WCXRP00000352] [Volunteer Patch][MT6620][Driver] P2P Statsion Record Client List Issue
 * 1. Modify Channel Acquire Time of AP mode from 5s to 1s.
 * 2. Call cnmP2pIsPermit() before active P2P network.
 * 3. Add channel selection support for AP mode.
 *
 * 01 05 2011 cp.wu
 * [WCXRP00000283] [MT6620 Wi-Fi][Driver][Wi-Fi Direct] Implementation of interface for supporting Wi-Fi Direct Service
 * Discovery
 * ioctl implementations for P2P Service Discovery
 *
 * 01 04 2011 cp.wu
 * [WCXRP00000338] [MT6620 Wi-Fi][Driver] Separate kalMemAlloc into kmalloc and vmalloc implementations to ease
 * physically continuous memory demands
 * separate kalMemAlloc() into virtually-continuous and physically-continuous type to ease slab system pressure
 *
 * 12 22 2010 cp.wu
 * [WCXRP00000283] [MT6620 Wi-Fi][Driver][Wi-Fi Direct] Implementation of interface for supporting Wi-Fi Direct Service
 * Discovery
 * 1. header file restructure for more clear module isolation
 * 2. add function interface definition for implementing Service Discovery callbacks
 *
 * 12 15 2010 cp.wu
 * NULL
 * invoke nicEnableInterrupt() before leaving from wlanAdapterStart()
 *
 * 12 08 2010 yuche.tsai
 * [WCXRP00000245] [MT6620][Driver] Invitation & Provision Discovery Feature Check-in
 * [WCXRP000000245][MT6620][Driver] Invitation Request Feature Add
 *
 * 11 30 2010 yuche.tsai
 * NULL
 * Invitation & Provision Discovery Indication.
 *
 * 11 17 2010 wh.su
 * [WCXRP00000164] [MT6620 Wi-Fi][Driver] Support the p2p random SSID[WCXRP00000179] [MT6620 Wi-Fi][FW] Set the Tx
 * lowest rate at wlan table for normal operation
 * fixed some ASSERT check.
 *
 * 11 04 2010 wh.su
 * [WCXRP00000164] [MT6620 Wi-Fi][Driver] Support the p2p random SSID
 * adding the p2p random ssid support.
 *
 * 10 20 2010 wh.su
 * [WCXRP00000124] [MT6620 Wi-Fi] [Driver] Support the dissolve P2P Group
 * Add the code to support disconnect p2p group
 *
 * 10 04 2010 wh.su
 * [WCXRP00000081] [MT6620][Driver] Fix the compiling error at WinXP while enable P2P
 * add a kal function for set cipher.
 *
 * 10 04 2010 wh.su
 * [WCXRP00000081] [MT6620][Driver] Fix the compiling error at WinXP while enable P2P
 * fixed compiling error while enable p2p.
 *
 * 09 28 2010 wh.su
 * NULL
 * [WCXRP00000069][MT6620 Wi-Fi][Driver] Fix some code for phase 1 P2P Demo.
 *
 * 09 21 2010 kevin.huang
 * [WCXRP00000054] [MT6620 Wi-Fi][Driver] Restructure driver for second Interface
 * Isolate P2P related function for Hardware Software Bundle
 *
 * 09 21 2010 kevin.huang
 * [WCXRP00000052] [MT6620 Wi-Fi][Driver] Eliminate Linux Compile Warning
 * Eliminate Linux Compile Warning
 *
 * 09 10 2010 george.huang
 * NULL
 * update iwpriv LP related
 *
 * 09 10 2010 wh.su
 * NULL
 * fixed the compiling error at win XP.
 *
 * 09 09 2010 cp.wu
 * NULL
 * add WPS/WPA/RSN IE for Wi-Fi Direct scanning result.
 *
 * 09 07 2010 wh.su
 * NULL
 * adding the code for beacon/probe req/ probe rsp wsc ie at p2p.
 *
 * 09 06 2010 wh.su
 * NULL
 * let the p2p can set the privacy bit at beacon and rsn ie at assoc req at key handshake state.
 *
 * 08 25 2010 cp.wu
 * NULL
 * add netdev_ops(NDO) for linux kernel 2.6.31 or greater
 *
 * 08 23 2010 cp.wu
 * NULL
 * revise constant definitions to be matched with implementation (original cmd-event definition is deprecated)
 *
 * 08 20 2010 cp.wu
 * NULL
 * correct typo.
 *
 * 08 20 2010 yuche.tsai
 * NULL
 * Invert Connection request provision status parameter.
 *
 * 08 19 2010 cp.wu
 * NULL
 * add set mac address interface for further possibilities of wpa_supplicant overriding interface address.
 *
 * 08 18 2010 cp.wu
 * NULL
 * modify pwp ioctls attribution by removing FIXED_SIZE.
 *
 * 08 18 2010 jeffrey.chang
 * NULL
 * support multi-function sdio
 *
 * 08 17 2010 cp.wu
 * NULL
 * correct p2p net device registration with NULL pointer access issue.
 *
 * 08 16 2010 cp.wu
 * NULL
 * P2P packets are now marked when being queued into driver, and identified later without checking MAC address
 *
 * 08 16 2010 cp.wu
 * NULL
 * add subroutines for P2P to set multicast list.
 *
 * 08 16 2010 george.huang
 * NULL
 * add wext handlers to link P2P set PS profile/ network address function (TBD)
 *
 * 08 16 2010 cp.wu
 * NULL
 * revised implementation of Wi-Fi Direct io controls.
 *
 * 08 12 2010 cp.wu
 * NULL
 * follow-up with ioctl interface update for Wi-Fi Direct application
 *
 * 08 06 2010 cp.wu
 * NULL
 * driver hook modifications corresponding to ioctl interface change.
 *
 * 08 03 2010 cp.wu
 * NULL
 * add basic support for ioctl of getting scan result. (only address and SSID are reporterd though)
 *
 * 08 03 2010 cp.wu
 * NULL
 * [Wi-Fi Direct Driver Hook] change event indication API to be consistent with supplicant
 *
 * 08 03 2010 cp.wu
 * NULL
 * surpress compilation warning.
 *
 * 08 03 2010 cp.wu
 * NULL
 * [Wi-Fi Direct] add framework for driver hooks
 *
 * 07 08 2010 cp.wu
 *
 * [WPD00003833] [MT6620 and MT5931] Driver migration - move to new repository.
 *
 * 06 23 2010 cp.wu
 * [WPD00003833][MT6620 and MT5931] Driver migration
 * p2p interface revised to be sync. with HAL
 *
 * 06 06 2010 kevin.huang
 * [WPD00003832][MT6620 5931] Create driver base
 * [MT6620 5931] Create driver base
 *
 * 06 01 2010 cp.wu
 * [WPD00003831][MT6620 Wi-Fi] Add framework for Wi-Fi Direct support
 * add ioctl to configure scan mode for p2p connection
 *
 * 05 31 2010 cp.wu
 * [WPD00003831][MT6620 Wi-Fi] Add framework for Wi-Fi Direct support
 * add cfg80211 interface, which is to replace WE, for further extension
 *
 * 05 17 2010 cp.wu
 * [WPD00003831][MT6620 Wi-Fi] Add framework for Wi-Fi Direct support
 * implement private io controls for Wi-Fi Direct
 *
 * 05 17 2010 cp.wu
 * [WPD00003831][MT6620 Wi-Fi] Add framework for Wi-Fi Direct support
 * implement get scan result.
 *
 * 05 17 2010 cp.wu
 * [WPD00003831][MT6620 Wi-Fi] Add framework for Wi-Fi Direct support
 * add basic handling framework for wireless extension ioctls.
 *
 * 05 17 2010 cp.wu
 * [WPD00003831][MT6620 Wi-Fi] Add framework for Wi-Fi Direct support
 * 1) add timeout handler mechanism for pending command packets
 * 2) add p2p add/removal key
 *
 * 05 14 2010 cp.wu
 * [WPD00003831][MT6620 Wi-Fi] Add framework for Wi-Fi Direct support
 * implement wireless extension ioctls in iw_handler form.
 *
 * 05 14 2010 cp.wu
 * [WPD00003831][MT6620 Wi-Fi] Add framework for Wi-Fi Direct support
 * add ioctl framework for Wi-Fi Direct by reusing wireless extension ioctls as well
 *
 * 05 11 2010 cp.wu
 * [WPD00003831][MT6620 Wi-Fi] Add framework for Wi-Fi Direct support
 * p2p ioctls revised.
 *
 * 05 11 2010 cp.wu
 * [WPD00003831][MT6620 Wi-Fi] Add framework for Wi-Fi Direct support
 * add ioctl for controlling p2p scan phase parameters
 *
 * 05 10 2010 cp.wu
 * [WPD00003831][MT6620 Wi-Fi] Add framework for Wi-Fi Direct support
 * implement basic wi-fi direct framework
 *
 * 05 07 2010 cp.wu
 * [WPD00003831][MT6620 Wi-Fi] Add framework for Wi-Fi Direct support
 * add basic framework for implementating P2P driver hook.
 *
**
*/

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

#include <linux/poll.h>

#include <linux/kmod.h>


#include "gl_os.h"
#include "debug.h"
#include "wlan_lib.h"
#include "gl_wext.h"

/* #include <net/cfg80211.h> */
#include "gl_p2p_ioctl.h"

#include "precomp.h"
#include "gl_vendor.h"

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
#define ARGV_MAX_NUM        (4)

/*For CFG80211 - wiphy parameters*/
#define MAX_SCAN_LIST_NUM   (1)
#define MAX_SCAN_IE_LEN     (512)

#if 0
#define RUNNING_P2P_MODE 0
#define RUNNING_AP_MODE 1
#define RUNNING_DUAL_AP_MODE 2
#endif
/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/


/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

struct net_device *g_P2pPrDev;
struct wireless_dev *gprP2pWdev;
struct wireless_dev *gprP2pRoleWdev[KAL_P2P_NUM];
struct net_device *gPrP2pDev[KAL_P2P_NUM];

#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
static struct cfg80211_ops mtk_p2p_ops = {
#if (CFG_ENABLE_WIFI_DIRECT_CFG_80211 != 0)
	/* Froyo */
	.add_virtual_intf = mtk_p2p_cfg80211_add_iface,
	.change_virtual_intf = mtk_p2p_cfg80211_change_iface,	/* 1 st */
	.del_virtual_intf = mtk_p2p_cfg80211_del_iface,
	.change_bss = mtk_p2p_cfg80211_change_bss,
	.scan = mtk_p2p_cfg80211_scan,
	.remain_on_channel = mtk_p2p_cfg80211_remain_on_channel,
	.cancel_remain_on_channel = mtk_p2p_cfg80211_cancel_remain_on_channel,
	.mgmt_tx = mtk_p2p_cfg80211_mgmt_tx,
	.mgmt_tx_cancel_wait = mtk_p2p_cfg80211_mgmt_tx_cancel_wait,
	.connect = mtk_p2p_cfg80211_connect,
	.disconnect = mtk_p2p_cfg80211_disconnect,
	.deauth = mtk_p2p_cfg80211_deauth,
	.disassoc = mtk_p2p_cfg80211_disassoc,
	.start_ap = mtk_p2p_cfg80211_start_ap,
	.change_beacon = mtk_p2p_cfg80211_change_beacon,
	.stop_ap = mtk_p2p_cfg80211_stop_ap,
	.set_wiphy_params = mtk_p2p_cfg80211_set_wiphy_params,
	.del_station = mtk_p2p_cfg80211_del_station,
	.set_bitrate_mask = mtk_p2p_cfg80211_set_bitrate_mask,
	.mgmt_frame_register = mtk_p2p_cfg80211_mgmt_frame_register,
	.get_station = mtk_p2p_cfg80211_get_station,
	.add_key = mtk_p2p_cfg80211_add_key,
	.get_key = mtk_p2p_cfg80211_get_key,
	.del_key = mtk_p2p_cfg80211_del_key,
	.set_default_key = mtk_p2p_cfg80211_set_default_key,
	.join_ibss = mtk_p2p_cfg80211_join_ibss,
	.leave_ibss = mtk_p2p_cfg80211_leave_ibss,
	.set_tx_power = mtk_p2p_cfg80211_set_txpower,
	.get_tx_power = mtk_p2p_cfg80211_get_txpower,
	.set_power_mgmt = mtk_p2p_cfg80211_set_power_mgmt,
#ifdef CONFIG_NL80211_TESTMODE
	.testmode_cmd = mtk_p2p_cfg80211_testmode_cmd,
#endif
#endif
};

static const struct wiphy_vendor_command mtk_p2p_vendor_ops[] = {
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_SUBCMD_GET_CHANNEL_LIST
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_get_channel_list
	},
	{
		{
			.vendor_id = GOOGLE_OUI,
			.subcmd = WIFI_SUBCMD_SET_COUNTRY_CODE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = mtk_cfg80211_vendor_set_country_code
	},
};

/* There isn't a lot of sense in it, but you can transmit anything you like */
static const struct ieee80211_txrx_stypes
mtk_cfg80211_default_mgmt_stypes[NUM_NL80211_IFTYPES] = {
	[NL80211_IFTYPE_ADHOC] = {
				  .tx = 0xffff,
				  .rx = BIT(IEEE80211_STYPE_ACTION >> 4)
				  },
	[NL80211_IFTYPE_STATION] = {
				    .tx = 0xffff,
				    .rx = BIT(IEEE80211_STYPE_ACTION >> 4) | BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
				    },
	[NL80211_IFTYPE_AP] = {
			       .tx = 0xffff,
			       .rx = BIT(IEEE80211_STYPE_PROBE_REQ >> 4) | BIT(IEEE80211_STYPE_ACTION >> 4)
			       },
	[NL80211_IFTYPE_AP_VLAN] = {
				    /* copy AP */
				    .tx = 0xffff,
				    .rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
				    BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
				    BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
				    BIT(IEEE80211_STYPE_DISASSOC >> 4) |
				    BIT(IEEE80211_STYPE_AUTH >> 4) |
				    BIT(IEEE80211_STYPE_DEAUTH >> 4) | BIT(IEEE80211_STYPE_ACTION >> 4)
				    },
	[NL80211_IFTYPE_P2P_CLIENT] = {
				       .tx = 0xffff,
				       .rx = BIT(IEEE80211_STYPE_ACTION >> 4) | BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
				       },
	[NL80211_IFTYPE_P2P_GO] = {
				   .tx = 0xffff,
				   .rx = BIT(IEEE80211_STYPE_PROBE_REQ >> 4) | BIT(IEEE80211_STYPE_ACTION >> 4)
				   }
};

#endif


static const struct iw_priv_args rP2PIwPrivTable[] = {
	{
	 .cmd = IOC_P2P_CFG_DEVICE,
	 .set_args = IW_PRIV_TYPE_BYTE | (__u16) sizeof(IW_P2P_CFG_DEVICE_TYPE),
	 .get_args = IW_PRIV_TYPE_NONE,
	 .name = "P2P_CFG_DEVICE"}
	,
	{
	 .cmd = IOC_P2P_START_STOP_DISCOVERY,
	 .set_args = IW_PRIV_TYPE_BYTE | (__u16) sizeof(IW_P2P_REQ_DEVICE_TYPE),
	 .get_args = IW_PRIV_TYPE_NONE,
	 .name = "P2P_DISCOVERY"}
	,
	{
	 .cmd = IOC_P2P_DISCOVERY_RESULTS,
	 .set_args = IW_PRIV_TYPE_NONE,
	 .get_args = IW_PRIV_TYPE_NONE,
	 .name = "P2P_RESULT"}
	,
	{
	 .cmd = IOC_P2P_WSC_BEACON_PROBE_RSP_IE,
	 .set_args = IW_PRIV_TYPE_BYTE | (__u16) sizeof(IW_P2P_HOSTAPD_PARAM),
	 .get_args = IW_PRIV_TYPE_NONE,
	 .name = "P2P_WSC_IE"}
	,
	{
	 .cmd = IOC_P2P_CONNECT_DISCONNECT,
	 .set_args = IW_PRIV_TYPE_BYTE | (__u16) sizeof(IW_P2P_CONNECT_DEVICE),
	 .get_args = IW_PRIV_TYPE_NONE,
	 .name = "P2P_CONNECT"}
	,
	{
	 .cmd = IOC_P2P_PASSWORD_READY,
	 .set_args = IW_PRIV_TYPE_BYTE | (__u16) sizeof(IW_P2P_PASSWORD_READY),
	 .get_args = IW_PRIV_TYPE_NONE,
	 .name = "P2P_PASSWD_RDY"}
	,
	{
	 .cmd = IOC_P2P_GET_STRUCT,
	 .set_args = IW_PRIV_TYPE_NONE,
	 .get_args = 256,
	 .name = "P2P_GET_STRUCT"}
	,
	{
	 .cmd = IOC_P2P_SET_STRUCT,
	 .set_args = 256,
	 .get_args = IW_PRIV_TYPE_NONE,
	 .name = "P2P_SET_STRUCT"}
	,
	{
	 .cmd = IOC_P2P_GET_REQ_DEVICE_INFO,
	 .set_args = IW_PRIV_TYPE_NONE,
	 .get_args = IW_PRIV_TYPE_BYTE | (__u16) sizeof(IW_P2P_DEVICE_REQ),
	 .name = "P2P_GET_REQDEV"}
	,
	{
	 /* SET STRUCT sub-ioctls commands */
	 .cmd = PRIV_CMD_OID,
	 .set_args = 256,
	 .get_args = IW_PRIV_TYPE_NONE,
	 .name = "set_oid"}
	,
	{
	 /* GET STRUCT sub-ioctls commands */
	 .cmd = PRIV_CMD_OID,
	 .set_args = IW_PRIV_TYPE_NONE,
	 .get_args = 256,
	 .name = "get_oid"}
};

#if 0
const struct iw_handler_def mtk_p2p_wext_handler_def = {
	.num_standard = (__u16) sizeof(rP2PIwStandardHandler) / sizeof(iw_handler),
/* .num_private        = (__u16)sizeof(rP2PIwPrivHandler)/sizeof(iw_handler), */
	.num_private_args = (__u16) sizeof(rP2PIwPrivTable) / sizeof(struct iw_priv_args),
	.standard = rP2PIwStandardHandler,
/* .private            = rP2PIwPrivHandler, */
	.private_args = rP2PIwPrivTable,
#if CFG_SUPPORT_P2P_RSSI_QUERY
	.get_wireless_stats = mtk_p2p_wext_get_wireless_stats,
#else
	.get_wireless_stats = NULL,
#endif
};
#endif

#ifdef CONFIG_PM
static const struct wiphy_wowlan_support mtk_p2p_wowlan_support = {
	.flags = WIPHY_WOWLAN_DISCONNECT | WIPHY_WOWLAN_ANY,
};
#endif

static const struct ieee80211_iface_limit mtk_p2p_sta_go_limits[] = {
	{
		.max = 3,
		.types = BIT(NL80211_IFTYPE_STATION),
	},

	{
		.max = 1,
		.types = BIT(NL80211_IFTYPE_P2P_GO) | BIT(NL80211_IFTYPE_P2P_CLIENT),
	},
};

static const struct ieee80211_iface_combination
mtk_iface_combinations_sta[] = {
	{
#ifdef CFG_NUM_DIFFERENT_CHANNELS_STA
		.num_different_channels = CFG_NUM_DIFFERENT_CHANNELS_STA,
#else
		.num_different_channels = 2,
#endif /* CFG_NUM_DIFFERENT_CHANNELS_STA */
		.max_interfaces = 3,
		/*.beacon_int_infra_match = true,*/
		.limits = mtk_p2p_sta_go_limits,
		.n_limits = 1, /* include p2p */
	},
};

static const struct ieee80211_iface_combination
mtk_iface_combinations_p2p[] = {
	{
#ifdef CFG_NUM_DIFFERENT_CHANNELS_P2P
		.num_different_channels = CFG_NUM_DIFFERENT_CHANNELS_P2P,
#else
		.num_different_channels = 2,
#endif /* CFG_NUM_DIFFERENT_CHANNELS_P2P */
		.max_interfaces = 3,
		/*.beacon_int_infra_match = true,*/
		.limits = mtk_p2p_sta_go_limits,
		.n_limits = ARRAY_SIZE(mtk_p2p_sta_go_limits), /* include p2p */
	},
};
const struct ieee80211_iface_combination *p_mtk_iface_combinations_sta = mtk_iface_combinations_sta;
const INT_32 mtk_iface_combinations_sta_num = ARRAY_SIZE(mtk_iface_combinations_sta);

const struct ieee80211_iface_combination *p_mtk_iface_combinations_p2p = mtk_iface_combinations_p2p;
const INT_32 mtk_iface_combinations_p2p_num = ARRAY_SIZE(mtk_iface_combinations_p2p);

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/* Net Device Hooks */
static int p2pOpen(IN struct net_device *prDev);

static int p2pStop(IN struct net_device *prDev);

static struct net_device_stats *p2pGetStats(IN struct net_device *prDev);

static void p2pSetMulticastList(IN struct net_device *prDev);

static int p2pHardStartXmit(IN struct sk_buff *prSkb, IN struct net_device *prDev);

static int p2pSetMACAddress(IN struct net_device *prDev, void *addr);

static int p2pDoIOCTL(struct net_device *prDev, struct ifreq *prIFReq, int i4Cmd);


/*----------------------------------------------------------------------------*/
/*!
* \brief A function for prDev->init
*
* \param[in] prDev      Pointer to struct net_device.
*
* \retval 0         The execution of wlanInit succeeds.
* \retval -ENXIO    No such device.
*/
/*----------------------------------------------------------------------------*/
static int p2pInit(struct net_device *prDev)
{
	if (!prDev)
		return -ENXIO;

	return 0;		/* success */
}				/* end of p2pInit() */

/*----------------------------------------------------------------------------*/
/*!
* \brief A function for prDev->uninit
*
* \param[in] prDev      Pointer to struct net_device.
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
static void p2pUninit(IN struct net_device *prDev)
{
}				/* end of p2pUninit() */

const struct net_device_ops p2p_netdev_ops = {
	.ndo_open = p2pOpen,
	.ndo_stop = p2pStop,
	.ndo_set_mac_address = p2pSetMACAddress,
	.ndo_set_rx_mode = p2pSetMulticastList,
	.ndo_get_stats = p2pGetStats,
	.ndo_do_ioctl = p2pDoIOCTL,
	.ndo_start_xmit = p2pHardStartXmit,
	/* .ndo_select_queue       = p2pSelectQueue, */
	.ndo_select_queue = wlanSelectQueue,
	.ndo_init = p2pInit,
	.ndo_uninit = p2pUninit,
};

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

/*----------------------------------------------------------------------------*/
/*!
* \brief Allocate memory for P2P_INFO, GL_P2P_INFO, P2P_CONNECTION_SETTINGS
*                                          P2P_SPECIFIC_BSS_INFO, P2P_FSM_INFO
*
* \param[in] prGlueInfo      Pointer to glue info
*
* \return   TRUE
*           FALSE
*/
/*----------------------------------------------------------------------------*/
BOOLEAN p2PAllocInfo(IN P_GLUE_INFO_T prGlueInfo, IN UINT_8 ucIdex)
{
	P_ADAPTER_T prAdapter = NULL;
	P_WIFI_VAR_T prWifiVar = NULL;
	/* UINT_32 u4Idx = 0; */

	ASSERT(prGlueInfo);

	prAdapter = prGlueInfo->prAdapter;
	prWifiVar = &(prAdapter->rWifiVar);

	ASSERT(prAdapter);
	ASSERT(prWifiVar);

	do {
		if (prGlueInfo == NULL)
			break;

		if (prGlueInfo->prP2PInfo[ucIdex] == NULL) {
			/*alloc memory for p2p info */
			prGlueInfo->prP2PDevInfo = kalMemAlloc(sizeof(GL_P2P_DEV_INFO_T), VIR_MEM_TYPE);
			prGlueInfo->prP2PInfo[ucIdex] = kalMemAlloc(sizeof(GL_P2P_INFO_T), VIR_MEM_TYPE);

			if (ucIdex == 0) {
				/*printk("[CHECK!]p2PAllocInfo : Alloc Common part only first interface\n");*/
				prAdapter->prP2pInfo = kalMemAlloc(sizeof(P2P_INFO_T), VIR_MEM_TYPE);
				prWifiVar->prP2pDevFsmInfo = kalMemAlloc(sizeof(P2P_DEV_FSM_INFO_T), VIR_MEM_TYPE);
			}
			prWifiVar->prP2PConnSettings[ucIdex] =
				kalMemAlloc(sizeof(P2P_CONNECTION_SETTINGS_T), VIR_MEM_TYPE);
			prWifiVar->prP2pSpecificBssInfo[ucIdex] =
				kalMemAlloc(sizeof(P2P_SPECIFIC_BSS_INFO_T), VIR_MEM_TYPE);

			/* TODO: It can be moved to the interface been created. */
#if 0
			    for (u4Idx = 0; u4Idx < BSS_P2P_NUM; u4Idx++) {
				prWifiVar->aprP2pRoleFsmInfo[u4Idx] =
				kalMemAlloc(sizeof(P2P_ROLE_FSM_INFO_T), VIR_MEM_TYPE);
			    }
#endif


		} else {
			ASSERT(prAdapter->prP2pInfo != NULL);
			ASSERT(prWifiVar->prP2PConnSettings[ucIdex] != NULL);
			/* ASSERT(prWifiVar->prP2pFsmInfo != NULL); */
			ASSERT(prWifiVar->prP2pSpecificBssInfo[ucIdex] != NULL);
		}
		/*MUST set memory to 0 */
		kalMemZero(prGlueInfo->prP2PInfo[ucIdex], sizeof(GL_P2P_INFO_T));
		kalMemZero(prGlueInfo->prP2PDevInfo, sizeof(GL_P2P_DEV_INFO_T));
		kalMemZero(prAdapter->prP2pInfo, sizeof(P2P_INFO_T));
		kalMemZero(prWifiVar->prP2PConnSettings[ucIdex], sizeof(P2P_CONNECTION_SETTINGS_T));
/* kalMemZero(prWifiVar->prP2pFsmInfo, sizeof(P2P_FSM_INFO_T)); */
		kalMemZero(prWifiVar->prP2pSpecificBssInfo[ucIdex], sizeof(P2P_SPECIFIC_BSS_INFO_T));

	} while (FALSE);

	if (!prGlueInfo->prP2PDevInfo)
		DBGLOG(P2P, ERROR, "prP2PDevInfo error\n");
	else
		DBGLOG(P2P, INFO, "prP2PDevInfo ok\n");

	if (!prGlueInfo->prP2PInfo[ucIdex])
		DBGLOG(P2P, ERROR, "prP2PInfo error\n");
	else
		DBGLOG(P2P, INFO, "prP2PInfo ok\n");



	/* chk if alloc successful or not */
	if (prGlueInfo->prP2PInfo[ucIdex] && prGlueInfo->prP2PDevInfo && prAdapter->prP2pInfo &&
		prWifiVar->prP2PConnSettings[ucIdex] &&
/* prWifiVar->prP2pFsmInfo && */
	    prWifiVar->prP2pSpecificBssInfo[ucIdex])
		return TRUE;


	DBGLOG(P2P, ERROR, "[fail!]p2PAllocInfo :fail\n");

	if (prWifiVar->prP2pSpecificBssInfo[ucIdex]) {
		kalMemFree(prWifiVar->prP2pSpecificBssInfo[ucIdex], VIR_MEM_TYPE, sizeof(P2P_SPECIFIC_BSS_INFO_T));

		prWifiVar->prP2pSpecificBssInfo[ucIdex] = NULL;
	}
/* if (prWifiVar->prP2pFsmInfo) { */
/* kalMemFree(prWifiVar->prP2pFsmInfo, VIR_MEM_TYPE, sizeof(P2P_FSM_INFO_T)); */

/* prWifiVar->prP2pFsmInfo = NULL; */
/* } */
	if (prWifiVar->prP2PConnSettings[ucIdex]) {
		kalMemFree(prWifiVar->prP2PConnSettings[ucIdex], VIR_MEM_TYPE, sizeof(P2P_CONNECTION_SETTINGS_T));

		prWifiVar->prP2PConnSettings[ucIdex] = NULL;
	}
	if (prGlueInfo->prP2PDevInfo) {
		kalMemFree(prGlueInfo->prP2PDevInfo, VIR_MEM_TYPE, sizeof(GL_P2P_DEV_INFO_T));

		prGlueInfo->prP2PDevInfo = NULL;
	}
	if (prGlueInfo->prP2PInfo[ucIdex]) {
		kalMemFree(prGlueInfo->prP2PInfo[ucIdex], VIR_MEM_TYPE, sizeof(GL_P2P_INFO_T));

		prGlueInfo->prP2PInfo[ucIdex] = NULL;
	}
	if (prAdapter->prP2pInfo) {
		kalMemFree(prAdapter->prP2pInfo, VIR_MEM_TYPE, sizeof(P2P_INFO_T));

		prAdapter->prP2pInfo = NULL;
	}
	return FALSE;

}

/*----------------------------------------------------------------------------*/
/*!
* \brief Free memory for P2P_INFO, GL_P2P_INFO, P2P_CONNECTION_SETTINGS
*                                          P2P_SPECIFIC_BSS_INFO, P2P_FSM_INFO
*
* \param[in] prGlueInfo      Pointer to glue info
*
* \return   TRUE
*           FALSE
*/
/*----------------------------------------------------------------------------*/
BOOLEAN p2PFreeInfo(P_GLUE_INFO_T prGlueInfo)
{

	UINT_8	i;

	ASSERT(prGlueInfo);
	ASSERT(prGlueInfo->prAdapter);

	/* free memory after p2p module is ALREADY unregistered */
	if (prGlueInfo->prAdapter->fgIsP2PRegistered == FALSE) {

		kalMemFree(prGlueInfo->prAdapter->prP2pInfo, VIR_MEM_TYPE, sizeof(P2P_INFO_T));

		for (i = 0; i < KAL_P2P_NUM; i++)
			if (prGlueInfo->prP2PInfo[i] != NULL) {
				kalMemFree(prGlueInfo->prAdapter->rWifiVar.prP2PConnSettings[i], VIR_MEM_TYPE,
					   sizeof(P2P_CONNECTION_SETTINGS_T));

				prGlueInfo->prAdapter->rWifiVar.prP2PConnSettings[i] = NULL;
			}

		kalMemFree(prGlueInfo->prP2PDevInfo, VIR_MEM_TYPE, sizeof(GL_P2P_DEV_INFO_T));

		for (i = 0; i < KAL_P2P_NUM; i++) {

			if (prGlueInfo->prP2PInfo[i] != NULL) {
				kalMemFree(prGlueInfo->prP2PInfo[i], VIR_MEM_TYPE, sizeof(GL_P2P_INFO_T));
				prGlueInfo->prP2PInfo[i] = NULL;
			}
		}

/* kalMemFree(prGlueInfo->prAdapter->rWifiVar.prP2pFsmInfo, VIR_MEM_TYPE, sizeof(P2P_FSM_INFO_T)); */
		for (i = 0; i < KAL_P2P_NUM; i++)
			if (prGlueInfo->prP2PInfo[i] != NULL) {
				kalMemFree(prGlueInfo->prAdapter->rWifiVar.prP2pSpecificBssInfo[i], VIR_MEM_TYPE,
					   sizeof(P2P_SPECIFIC_BSS_INFO_T));

				prGlueInfo->prAdapter->rWifiVar.prP2pSpecificBssInfo[i] = NULL;
			}

		kalMemFree(prGlueInfo->prAdapter->rWifiVar.prP2pDevFsmInfo, VIR_MEM_TYPE, sizeof(P2P_DEV_FSM_INFO_T));

		/*Reomve p2p bss scan list */
		scanRemoveAllP2pBssDesc(prGlueInfo->prAdapter);

		/*reset all pointer to NULL */
		/*prGlueInfo->prP2PInfo[0] = NULL;*/
		prGlueInfo->prAdapter->prP2pInfo = NULL;
/* prGlueInfo->prAdapter->rWifiVar.prP2pFsmInfo = NULL; */
		prGlueInfo->prAdapter->rWifiVar.prP2pDevFsmInfo = NULL;

		return TRUE;
	} else {
		return FALSE;
	}

}



BOOLEAN p2pNetRegister(P_GLUE_INFO_T prGlueInfo, BOOLEAN fgIsRtnlLockAcquired)
{
	BOOLEAN fgDoRegister = FALSE;
	BOOLEAN fgRollbackRtnlLock = FALSE;
	BOOLEAN ret;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prGlueInfo);
	ASSERT(prGlueInfo->prAdapter);

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	if (prGlueInfo->prAdapter->rP2PNetRegState == ENUM_NET_REG_STATE_UNREGISTERED) {
		prGlueInfo->prAdapter->rP2PNetRegState = ENUM_NET_REG_STATE_REGISTERING;
		fgDoRegister = TRUE;
	}
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	if (!fgDoRegister)
		return TRUE;

	if (fgIsRtnlLockAcquired && rtnl_is_locked()) {
		fgRollbackRtnlLock = TRUE;
		rtnl_unlock();
	}

	/* net device initialize */
	netif_carrier_off(prGlueInfo->prP2PInfo[0]->prDevHandler);
	netif_tx_stop_all_queues(prGlueInfo->prP2PInfo[0]->prDevHandler);

	/* register for net device */
	if (register_netdev(prGlueInfo->prP2PInfo[0]->prDevHandler) < 0) {
		DBGLOG(INIT, WARN, "unable to register netdevice for p2p\n");

		free_netdev(prGlueInfo->prP2PInfo[0]->prDevHandler);

		ret = FALSE;
	} else {
		prGlueInfo->prAdapter->rP2PNetRegState = ENUM_NET_REG_STATE_REGISTERED;
		gPrP2pDev[0] = prGlueInfo->prP2PInfo[0]->prDevHandler;
		ret = TRUE;
	}

	if (prGlueInfo->prAdapter->prP2pInfo->u4DeviceNum == RUNNING_DUAL_AP_MODE) {
		/* net device initialize */
		netif_carrier_off(prGlueInfo->prP2PInfo[1]->prDevHandler);
		netif_tx_stop_all_queues(prGlueInfo->prP2PInfo[1]->prDevHandler);

		/* register for net device */
		if (register_netdev(prGlueInfo->prP2PInfo[1]->prDevHandler) < 0) {
			DBGLOG(INIT, WARN, "unable to register netdevice for p2p\n");

			free_netdev(prGlueInfo->prP2PInfo[1]->prDevHandler);

			ret = FALSE;
		} else {
			prGlueInfo->prAdapter->rP2PNetRegState = ENUM_NET_REG_STATE_REGISTERED;
			gPrP2pDev[1] = prGlueInfo->prP2PInfo[1]->prDevHandler;
			ret = TRUE;
		}


		DBGLOG(P2P, INFO, "P2P 2nd interface work\n");
	}
	if (fgRollbackRtnlLock)
		rtnl_lock();

	return ret;
}

BOOLEAN p2pNetUnregister(P_GLUE_INFO_T prGlueInfo, BOOLEAN fgIsRtnlLockAcquired)
{
	BOOLEAN fgDoUnregister = FALSE;
	BOOLEAN fgRollbackRtnlLock = FALSE;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prGlueInfo);
	ASSERT(prGlueInfo->prAdapter);

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	if (prGlueInfo->prAdapter->rP2PNetRegState == ENUM_NET_REG_STATE_REGISTERED) {
		prGlueInfo->prAdapter->rP2PNetRegState = ENUM_NET_REG_STATE_UNREGISTERING;
		fgDoUnregister = TRUE;
	}
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	if (!fgDoUnregister)
		return TRUE;

	/* prepare for removal */
	if (prGlueInfo->prP2PInfo[0]->prDevHandler != prGlueInfo->prP2PInfo[0]->aprRoleHandler) {
		if (netif_carrier_ok(prGlueInfo->prP2PInfo[0]->aprRoleHandler))
			netif_carrier_off(prGlueInfo->prP2PInfo[0]->aprRoleHandler);

		netif_tx_stop_all_queues(prGlueInfo->prP2PInfo[0]->aprRoleHandler);
	}

	if (netif_carrier_ok(prGlueInfo->prP2PInfo[0]->prDevHandler))
		netif_carrier_off(prGlueInfo->prP2PInfo[0]->prDevHandler);

	netif_tx_stop_all_queues(prGlueInfo->prP2PInfo[0]->prDevHandler);

	if (fgIsRtnlLockAcquired && rtnl_is_locked()) {
		fgRollbackRtnlLock = TRUE;
		rtnl_unlock();
	}
	/* Here are functions which need rtnl_lock */
	if (prGlueInfo->prP2PInfo[0]->prDevHandler != prGlueInfo->prP2PInfo[0]->aprRoleHandler) {
		DBGLOG(INIT, INFO, "unregister p2p[0]\n");
		unregister_netdev(prGlueInfo->prP2PInfo[0]->aprRoleHandler);
	}
	DBGLOG(INIT, INFO, "unregister p2pdev\n");
	unregister_netdev(prGlueInfo->prP2PInfo[0]->prDevHandler);

	if (prGlueInfo->prAdapter->prP2pInfo->u4DeviceNum == RUNNING_DUAL_AP_MODE) {
		/* prepare for removal */
		if (netif_carrier_ok(prGlueInfo->prP2PInfo[1]->prDevHandler))
			netif_carrier_off(prGlueInfo->prP2PInfo[1]->prDevHandler);

		netif_tx_stop_all_queues(prGlueInfo->prP2PInfo[1]->prDevHandler);

		if (fgIsRtnlLockAcquired && rtnl_is_locked()) {
			fgRollbackRtnlLock = TRUE;
			rtnl_unlock();
		}
		/* Here are functions which need rtnl_lock */

		unregister_netdev(prGlueInfo->prP2PInfo[1]->prDevHandler);
	}

	if (fgRollbackRtnlLock)
		rtnl_lock();

	prGlueInfo->prAdapter->rP2PNetRegState = ENUM_NET_REG_STATE_UNREGISTERED;

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Register for cfg80211 for Wi-Fi Direct
*
* \param[in] prGlueInfo      Pointer to glue info
*
* \return   TRUE
*           FALSE
*/
/*----------------------------------------------------------------------------*/
BOOLEAN glRegisterP2P(P_GLUE_INFO_T prGlueInfo, const char *prDevName, const char *prDevName2, UINT_8 ucApMode)
{
	P_ADAPTER_T prAdapter = NULL;
	P_GL_HIF_INFO_T prHif = NULL;
	PARAM_MAC_ADDRESS rMacAddr;
	P_NETDEV_PRIVATE_GLUE_INFO prNetDevPriv = (P_NETDEV_PRIVATE_GLUE_INFO) NULL;
	BOOLEAN fgIsApMode = FALSE;
	UINT_8  ucRegisterNum = 1, i = 0;
	struct wireless_dev *prP2pWdev;
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
	struct device *prDev;
#endif
	const char *prSetDevName;

	ASSERT(prGlueInfo);

	prAdapter = prGlueInfo->prAdapter;
	ASSERT(prAdapter);

	prHif = &prGlueInfo->rHifInfo;
	ASSERT(prHif);

	if ((ucApMode == RUNNING_AP_MODE) || (ucApMode == RUNNING_DUAL_AP_MODE || (ucApMode == RUNNING_P2P_AP_MODE))) {
		fgIsApMode = TRUE;
		if ((ucApMode == RUNNING_DUAL_AP_MODE) || (ucApMode == RUNNING_P2P_AP_MODE)) {
			ucRegisterNum = 2;
			glP2pCreateWirelessDevice(prGlueInfo);
		}
	}
	prSetDevName = prDevName;

	do {
		if ((ucApMode == RUNNING_P2P_AP_MODE) && (i == 0)) {
			prSetDevName = prDevName;
			fgIsApMode = FALSE;
		} else if ((ucApMode == RUNNING_P2P_AP_MODE) && (i == 1)) {
			prSetDevName = prDevName2;
			fgIsApMode = TRUE;
		}
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
		if (!gprP2pRoleWdev[i]) {
			DBGLOG(P2P, ERROR, "gl_p2p, wireless device is not exist\n");
			return FALSE;
		}
#endif
		prP2pWdev = gprP2pRoleWdev[i];
		DBGLOG(INIT, INFO, "glRegisterP2P(%d)\n", i);
		/*0. allocate p2pinfo */
		if (!p2PAllocInfo(prGlueInfo, i)) {
			DBGLOG(INIT, WARN, "Allocate memory for p2p FAILED\n");
			ASSERT(0);
			return FALSE;
		}
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211

		/* 1.1 fill wiphy parameters */
		glGetHifDev(prHif, &prDev);
		if (!prDev)
			DBGLOG(INIT, INFO, "unable to get struct dev for p2p\n");

		prGlueInfo->prP2PInfo[i]->prWdev = prP2pWdev;
		/*prGlueInfo->prP2PInfo[i]->prRoleWdev[0] = prP2pWdev;*//* TH3 multiple P2P */

		ASSERT(prP2pWdev);
		ASSERT(prP2pWdev->wiphy);
		ASSERT(prDev);
		ASSERT(prGlueInfo->prAdapter);

		set_wiphy_dev(prP2pWdev->wiphy, prDev);

		if (!prGlueInfo->prAdapter->fgEnable5GBand)
			prP2pWdev->wiphy->bands[BAND_5G] = NULL;




		/* 2.2 wdev initialization */
		if (fgIsApMode)
			prP2pWdev->iftype = NL80211_IFTYPE_AP;
		else
			prP2pWdev->iftype = NL80211_IFTYPE_P2P_CLIENT;

#endif /* CFG_ENABLE_WIFI_DIRECT_CFG_80211 */


		/* 3. allocate netdev */
		prGlueInfo->prP2PInfo[i]->prDevHandler =
			alloc_netdev_mq(sizeof(NETDEV_PRIVATE_GLUE_INFO), prSetDevName,
					NET_NAME_PREDICTABLE, ether_setup, CFG_MAX_TXQ_NUM);
		if (!prGlueInfo->prP2PInfo[i]->prDevHandler) {
			DBGLOG(INIT, WARN, "unable to allocate netdevice for p2p\n");

			goto err_alloc_netdev;
		}

		/* 4. setup netdev */
		/* 4.1 Point to shared glue structure */
	/* *((P_GLUE_INFO_T *) netdev_priv(prGlueInfo->prP2PInfo->prDevHandler)) = prGlueInfo; */
		prNetDevPriv = (P_NETDEV_PRIVATE_GLUE_INFO) netdev_priv(prGlueInfo->prP2PInfo[i]->prDevHandler);
		prNetDevPriv->prGlueInfo = prGlueInfo;

		/* 4.2 fill hardware address */
		COPY_MAC_ADDR(rMacAddr, prAdapter->rMyMacAddr);
		rMacAddr[0] |= 0x2;
		rMacAddr[0] ^= i << 2;	/* change to local administrated address */
		kalMemCopy(prGlueInfo->prP2PInfo[i]->prDevHandler->dev_addr, rMacAddr, ETH_ALEN);
		kalMemCopy(prGlueInfo->prP2PInfo[i]->prDevHandler->perm_addr,
			prGlueInfo->prP2PInfo[i]->prDevHandler->dev_addr,
			   ETH_ALEN);

		/* 4.3 register callback functions */
		prGlueInfo->prP2PInfo[i]->prDevHandler->needed_headroom += NIC_TX_HEAD_ROOM;
		prGlueInfo->prP2PInfo[i]->prDevHandler->netdev_ops = &p2p_netdev_ops;
	/* prGlueInfo->prP2PInfo->prDevHandler->wireless_handlers    = &mtk_p2p_wext_handler_def; */

#if defined(_HIF_SDIO)
#if (MTK_WCN_HIF_SDIO == 0)
		SET_NETDEV_DEV(prGlueInfo->prP2PInfo[i]->prDevHandler, &(prHif->func->dev));
#endif
#endif

#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
		prGlueInfo->prP2PInfo[i]->prDevHandler->ieee80211_ptr = prP2pWdev;
		prP2pWdev->netdev = prGlueInfo->prP2PInfo[i]->prDevHandler;
#endif

#if CFG_TCP_IP_CHKSUM_OFFLOAD
		prGlueInfo->prP2PInfo[i]->prDevHandler->features = NETIF_F_IP_CSUM;
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

		kalResetStats(prGlueInfo->prP2PInfo[i]->prDevHandler);


#if 0
		/* 7. net device initialize */
		netif_carrier_off(prGlueInfo->prP2PInfo->prDevHandler);
		netif_tx_stop_all_queues(prGlueInfo->prP2PInfo->prDevHandler);

		/* 8. register for net device */
		if (register_netdev(prGlueInfo->prP2PInfo->prDevHandler) < 0) {
			DBGLOG(INIT, WARN, "unable to register netdevice for p2p\n");

			goto err_reg_netdev;
		}
#endif

		/* 8. set p2p net device register state */
		prGlueInfo->prAdapter->rP2PNetRegState = ENUM_NET_REG_STATE_UNREGISTERED;

		/* 9. setup running mode */
		/* prGlueInfo->prAdapter->rWifiVar.prP2pFsmInfo->fgIsApMode = fgIsApMode; */

		/* 10. finish */
		/* 13. bind netdev pointer to netdev index */
		wlanBindBssIdxToNetInterface(prGlueInfo, p2pDevFsmInit(prAdapter),
			(PVOID) prGlueInfo->prP2PInfo[i]->prDevHandler);

		prGlueInfo->prP2PInfo[i]->aprRoleHandler = prGlueInfo->prP2PInfo[i]->prDevHandler;


		DBGLOG(P2P, INFO, "check prDevHandler = %p\n", prGlueInfo->prP2PInfo[i]->prDevHandler);
		DBGLOG(P2P, INFO, "aprRoleHandler = %p\n", prGlueInfo->prP2PInfo[i]->aprRoleHandler);

		prNetDevPriv->ucBssIdx = p2pRoleFsmInit(prAdapter, i);
		/* 11. Currently wpasupplicant can't support create interface. */
		/* so initial the corresponding data structure here. */
		wlanBindBssIdxToNetInterface(prGlueInfo, prNetDevPriv->ucBssIdx,
					     (PVOID) prGlueInfo->prP2PInfo[i]->aprRoleHandler);

		/* 13. bind netdev pointer to netdev index */
		/* wlanBindNetInterface(prGlueInfo, NET_DEV_P2P_IDX, (PVOID)prGlueInfo->prP2PInfo->prDevHandler); */

		/* 12. setup running mode */
		p2pFuncInitConnectionSettings(prAdapter, prAdapter->rWifiVar.prP2PConnSettings[i], fgIsApMode);

		/* Active network too early would cause HW not able to sleep.
		 * Defer the network active time.
		 */
	/* nicActivateNetwork(prAdapter, NETWORK_TYPE_P2P_INDEX); */

		i++;
	} while (i < ucRegisterNum);

	if ((ucApMode == RUNNING_DUAL_AP_MODE) || (ucApMode == RUNNING_P2P_AP_MODE))
		prGlueInfo->prAdapter->prP2pInfo->u4DeviceNum = 2;
	else
		prGlueInfo->prAdapter->prP2pInfo->u4DeviceNum = 1;


	return TRUE;
#if 0
err_reg_netdev:
	free_netdev(prGlueInfo->prP2PInfo->prDevHandler);
#endif
err_alloc_netdev:
	return FALSE;
}				/* end of glRegisterP2P() */


BOOLEAN glP2pCreateWirelessDevice(P_GLUE_INFO_T prGlueInfo)
{
	struct wiphy *prWiphy = NULL;
	struct wireless_dev *prWdev = NULL;
	UINT_8	i = 0;
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
	prWdev = kzalloc(sizeof(struct wireless_dev), GFP_KERNEL);
	if (!prWdev) {
		DBGLOG(P2P, ERROR, "allocate p2p wireless device fail, no memory\n");
		return FALSE;
	}
	/* 1. allocate WIPHY */
	prWiphy = wiphy_new(&mtk_p2p_ops, sizeof(P_GLUE_INFO_T));
	if (!prWiphy) {
		DBGLOG(P2P, ERROR, "unable to allocate wiphy for p2p\n");
		goto free_wdev;
	}

	prWiphy->interface_modes = BIT(NL80211_IFTYPE_AP)
	    | BIT(NL80211_IFTYPE_P2P_CLIENT)
	    | BIT(NL80211_IFTYPE_P2P_GO)
	    | BIT(NL80211_IFTYPE_STATION);

	prWiphy->software_iftypes |= BIT(NL80211_IFTYPE_P2P_DEVICE);

	prWiphy->iface_combinations = p_mtk_iface_combinations_p2p;
	prWiphy->n_iface_combinations = mtk_iface_combinations_p2p_num;

	prWiphy->bands[IEEE80211_BAND_2GHZ] = &mtk_band_2ghz;
	prWiphy->bands[IEEE80211_BAND_5GHZ] = &mtk_band_5ghz;

	prWiphy->mgmt_stypes = mtk_cfg80211_default_mgmt_stypes;
	prWiphy->max_remain_on_channel_duration = 5000;
	prWiphy->n_cipher_suites = 5;
	prWiphy->cipher_suites = mtk_cipher_suites;
	prWiphy->flags = WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL | WIPHY_FLAG_HAVE_AP_SME;
	prWiphy->regulatory_flags = REGULATORY_CUSTOM_REG;
	prWiphy->ap_sme_capa = 1;

	prWiphy->max_scan_ssids = MAX_SCAN_LIST_NUM;
	prWiphy->max_scan_ie_len = MAX_SCAN_IE_LEN;
	prWiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;
	prWiphy->vendor_commands = mtk_p2p_vendor_ops;
	prWiphy->n_vendor_commands = sizeof(mtk_p2p_vendor_ops) / sizeof(struct wiphy_vendor_command);

#ifdef CONFIG_PM
	prWiphy->wowlan = &mtk_p2p_wowlan_support;
#endif

	/* 2.1 set priv as pointer to glue structure */
	*((P_GLUE_INFO_T *) wiphy_priv(prWiphy)) = prGlueInfo;
	/* Here are functions which need rtnl_lock */
	if (wiphy_register(prWiphy) < 0) {
		DBGLOG(INIT, WARN, "fail to register wiphy for p2p\n");
		goto free_wiphy;
	}
	prWdev->wiphy = prWiphy;

	for (i = 0 ; i < KAL_P2P_NUM; i++) {
		if (!gprP2pRoleWdev[i]) {
			gprP2pRoleWdev[i] = prWdev;
			DBGLOG(INIT, INFO, "glP2pCreateWirelessDevice (%x)\n", gprP2pRoleWdev[i]->wiphy);
			break;
		}
	}

	if (i == KAL_P2P_NUM)
		DBGLOG(INIT, WARN, "fail to register wiphy to driver\n");

	return TRUE;

free_wiphy:
	wiphy_free(prWiphy);
free_wdev:
	kfree(prWdev);
#endif
	return FALSE;
}

void glP2pDestroyWirelessDevice(void)
{
#if 0
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
	set_wiphy_dev(gprP2pWdev->wiphy, NULL);

	wiphy_unregister(gprP2pWdev->wiphy);
	wiphy_free(gprP2pWdev->wiphy);
	kfree(gprP2pWdev);
	gprP2pWdev = NULL;
#endif
#else
	int i = 0;

	set_wiphy_dev(gprP2pWdev->wiphy, NULL);

	wiphy_unregister(gprP2pWdev->wiphy);
	wiphy_free(gprP2pWdev->wiphy);
	kfree(gprP2pWdev);

	for (i = 0; i < KAL_P2P_NUM; i++) {
		if (gprP2pRoleWdev[i] && (gprP2pWdev != gprP2pRoleWdev[i])) {
			set_wiphy_dev(gprP2pRoleWdev[i]->wiphy, NULL);

			DBGLOG(INIT, INFO, "glP2pDestroyWirelessDevice (%x)\n", gprP2pRoleWdev[i]->wiphy);
			wiphy_unregister(gprP2pRoleWdev[i]->wiphy);
			wiphy_free(gprP2pRoleWdev[i]->wiphy);
			kfree(gprP2pRoleWdev[i]);
			gprP2pRoleWdev[i] = NULL;
		}
	}

	gprP2pWdev = NULL;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
* \brief Unregister Net Device for Wi-Fi Direct
*
* \param[in] prGlueInfo      Pointer to glue info
*
* \return   TRUE
*           FALSE
*/
/*----------------------------------------------------------------------------*/
BOOLEAN glUnregisterP2P(P_GLUE_INFO_T prGlueInfo)
{
	UINT_8 ucRoleIdx;
	P_ADAPTER_T prAdapter;

	ASSERT(prGlueInfo);

	prAdapter = prGlueInfo->prAdapter;

	/* 4 <1> Uninit P2P dev FSM */
	/* Uninit P2P device FSM */
	p2pDevFsmUninit(prAdapter);

	/* 4 <2> Uninit P2P role FSM */
	for (ucRoleIdx = 0; ucRoleIdx < BSS_P2P_NUM; ucRoleIdx++) {
		if (P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, ucRoleIdx))
			p2pRoleFsmUninit(prGlueInfo->prAdapter, ucRoleIdx);
	}

	/* 4 <3> Free Wiphy & netdev */
	if (prGlueInfo->prP2PInfo[0]->prDevHandler != prGlueInfo->prP2PInfo[0]->aprRoleHandler) {
		free_netdev(prGlueInfo->prP2PInfo[0]->aprRoleHandler);
		prGlueInfo->prP2PInfo[0]->aprRoleHandler = NULL;
	}

	free_netdev(prGlueInfo->prP2PInfo[0]->prDevHandler);
	prGlueInfo->prP2PInfo[0]->prDevHandler = NULL;

	/* 4 <4> Free P2P internal memory */
	if (!p2PFreeInfo(prGlueInfo)) {
		DBGLOG(INIT, WARN, "Free memory for p2p FAILED\n");
		ASSERT(0);
		return FALSE;
	}

	return TRUE;

#if 0
	p2pFsmUninit(prGlueInfo->prAdapter);

	nicDeactivateNetwork(prGlueInfo->prAdapter, NETWORK_TYPE_P2P_INDEX);

#if 0
	/* Release command, mgmt and security frame belong to P2P network in
	 *   prGlueInfo->prCmdQue
	 *   prAdapter->rPendingCmdQueue
	 *   prAdapter->rTxCtrl.rTxMgmtTxingQueue
	 *  To ensure there is no pending CmdDone/TxDone handler to be executed after p2p module is removed.
	 */

	/* Clear CmdQue */
	kalClearMgmtFramesByBssIdx(prGlueInfo, NETWORK_TYPE_P2P_INDEX);
	kalClearSecurityFramesByBssIdx(prGlueInfo, NETWORK_TYPE_P2P_INDEX);
	/* Clear PendingCmdQue */
	wlanReleasePendingCMDbyBssIdx(prGlueInfo->prAdapter, NETWORK_TYPE_P2P_INDEX);
	/* Clear PendingTxMsdu */
	nicFreePendingTxMsduInfoByBssIdx(prGlueInfo->prAdapter, NETWORK_TYPE_P2P_INDEX);
#endif

#if 0
	/* prepare for removal */
	if (netif_carrier_ok(prGlueInfo->prP2PInfo[0]->prDevHandler))
		netif_carrier_off(prGlueInfo->prP2PInfo[0]->prDevHandler);

	netif_tx_stop_all_queues(prGlueInfo->prP2PInfo[0]->prDevHandler);

	/* netdevice unregistration & free */
	unregister_netdev(prGlueInfo->prP2PInfo[0]->prDevHandler);
#endif
	free_netdev(prGlueInfo->prP2PInfo[0]->prDevHandler);
	prGlueInfo->prP2PInfo[0]->prDevHandler = NULL;

#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
#if 0
	wiphy_unregister(prGlueInfo->prP2PInfo[0]->wdev.wiphy);
#endif
	wiphy_free(prGlueInfo->prP2PInfo[0]->wdev.wiphy);
	prGlueInfo->prP2PInfo[0]->wdev.wiphy = NULL;
#endif

	/* Free p2p memory */
#if 1
	if (!p2PFreeInfo(prGlueInfo)) {
		DBGLOG(INIT, WARN, "Free memory for p2p FAILED\n");
		ASSERT(0);
		return FALSE;
	}
#endif
	return TRUE;

#else
	return 0;
#endif
}				/* end of glUnregisterP2P() */

/* Net Device Hooks */
/*----------------------------------------------------------------------------*/
/*!
 * \brief A function for net_device open (ifup)
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \retval 0     The execution succeeds.
 * \retval < 0   The execution failed.
 */
/*----------------------------------------------------------------------------*/
static int p2pOpen(IN struct net_device *prDev)
{
/* P_GLUE_INFO_T prGlueInfo = NULL; */
/* P_ADAPTER_T prAdapter = NULL; */
/* P_MSG_P2P_FUNCTION_SWITCH_T prFuncSwitch; */

	ASSERT(prDev);

#if 0				/* Move after device name set. (mtk_p2p_set_local_dev_info) */
	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));
	ASSERT(prGlueInfo);

	prAdapter = prGlueInfo->prAdapter;
	ASSERT(prAdapter);

	/* 1. switch P2P-FSM on */
	/* 1.1 allocate for message */
	prFuncSwitch = (P_MSG_P2P_FUNCTION_SWITCH_T) cnmMemAlloc(prAdapter,
								 RAM_TYPE_MSG, sizeof(MSG_P2P_FUNCTION_SWITCH_T));

	if (!prFuncSwitch) {
		ASSERT(0);	/* Can't trigger P2P FSM */
		return -ENOMEM;
	}

	/* 1.2 fill message */
	prFuncSwitch->rMsgHdr.eMsgId = MID_MNY_P2P_FUN_SWITCH;
	prFuncSwitch->fgIsFuncOn = TRUE;

	/* 1.3 send message */
	mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T) prFuncSwitch, MSG_SEND_METHOD_BUF);
#endif

	/* 2. carrier on & start TX queue */
	netif_carrier_on(prDev);
	netif_tx_start_all_queues(prDev);

	return 0;		/* success */
}				/* end of p2pOpen() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief A function for net_device stop (ifdown)
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \retval 0     The execution succeeds.
 * \retval < 0   The execution failed.
 */
/*----------------------------------------------------------------------------*/
static int p2pStop(IN struct net_device *prDev)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	P_ADAPTER_T prAdapter = NULL;
/* P_MSG_P2P_FUNCTION_SWITCH_T prFuncSwitch; */

	ASSERT(prDev);

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));
	ASSERT(prGlueInfo);

	prAdapter = prGlueInfo->prAdapter;
	ASSERT(prAdapter);

	/* 1. stop TX queue */
	netif_tx_stop_all_queues(prDev);
#if 0
	/* 2. switch P2P-FSM off */
	/* 2.1 allocate for message */
	prFuncSwitch = (P_MSG_P2P_FUNCTION_SWITCH_T) cnmMemAlloc(prAdapter,
								 RAM_TYPE_MSG, sizeof(MSG_P2P_FUNCTION_SWITCH_T));

	if (!prFuncSwitch) {
		ASSERT(0);	/* Can't trigger P2P FSM */
		return -ENOMEM;
	}

	/* 2.2 fill message */
	prFuncSwitch->rMsgHdr.eMsgId = MID_MNY_P2P_FUN_SWITCH;
	prFuncSwitch->fgIsFuncOn = FALSE;

	/* 2.3 send message */
	mboxSendMsg(prAdapter, MBOX_ID_0, (P_MSG_HDR_T) prFuncSwitch, MSG_SEND_METHOD_BUF);
#endif
	/* 3. stop queue and turn off carrier */
	/*prGlueInfo->prP2PInfo[0]->eState = PARAM_MEDIA_STATE_DISCONNECTED;*//* TH3 multiple P2P */

	netif_tx_stop_all_queues(prDev);
	if (netif_carrier_ok(prDev))
		netif_carrier_off(prDev);

	return 0;
}				/* end of p2pStop() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief A method of struct net_device, to get the network interface statistical
 *        information.
 *
 * Whenever an application needs to get statistics for the interface, this method
 * is called. This happens, for example, when ifconfig or netstat -i is run.
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \return net_device_stats buffer pointer.
 */
/*----------------------------------------------------------------------------*/
struct net_device_stats *p2pGetStats(IN struct net_device *prDev)
{
	return (struct net_device_stats *)kalGetStats(prDev);
}				/* end of p2pGetStats() */

static void p2pSetMulticastList(IN struct net_device *prDev)
{
	P_GLUE_INFO_T prGlueInfo = (P_GLUE_INFO_T) NULL;

	prGlueInfo = (prDev != NULL) ? *((P_GLUE_INFO_T *) netdev_priv(prDev)) : NULL;

	ASSERT(prDev);
	ASSERT(prGlueInfo);
	if (!prDev || !prGlueInfo) {
		DBGLOG(INIT, WARN, " abnormal dev or skb: prDev(0x%p), prGlueInfo(0x%p)\n", prDev, prGlueInfo);
		return;
	}

	g_P2pPrDev = prDev;

	/* 4  Mark HALT, notify main thread to finish current job */
	set_bit(GLUE_FLAG_SUB_MOD_MULTICAST_BIT, &prGlueInfo->ulFlag);
	/* wake up main thread */
	wake_up_interruptible(&prGlueInfo->waitq);

}				/* p2pSetMulticastList */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is to set multicast list and set rx mode.
 *
 * \param[in] prDev  Pointer to struct net_device
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void mtk_p2p_wext_set_Multicastlist(P_GLUE_INFO_T prGlueInfo)
{
	UINT_32 u4SetInfoLen = 0;
	UINT_32 u4McCount;
	struct net_device *prDev = g_P2pPrDev;

	prGlueInfo = (prDev != NULL) ? *((P_GLUE_INFO_T *) netdev_priv(prDev)) : NULL;

	ASSERT(prDev);
	ASSERT(prGlueInfo);
	if (!prDev || !prGlueInfo) {
		DBGLOG(INIT, WARN, " abnormal dev or skb: prDev(0x%p), prGlueInfo(0x%p)\n", prDev, prGlueInfo);
		return;
	}

	if (prDev->flags & IFF_PROMISC)
		prGlueInfo->prP2PDevInfo->u4PacketFilter |= PARAM_PACKET_FILTER_PROMISCUOUS;

	if (prDev->flags & IFF_BROADCAST)
		prGlueInfo->prP2PDevInfo->u4PacketFilter |= PARAM_PACKET_FILTER_BROADCAST;
	u4McCount = netdev_mc_count(prDev);

	if (prDev->flags & IFF_MULTICAST) {
		if ((prDev->flags & IFF_ALLMULTI) || (u4McCount > MAX_NUM_GROUP_ADDR))
			prGlueInfo->prP2PDevInfo->u4PacketFilter |= PARAM_PACKET_FILTER_ALL_MULTICAST;
		else
			prGlueInfo->prP2PDevInfo->u4PacketFilter |= PARAM_PACKET_FILTER_MULTICAST;
	}

	if (prGlueInfo->prP2PDevInfo->u4PacketFilter & PARAM_PACKET_FILTER_MULTICAST) {
		/* Prepare multicast address list */
		struct netdev_hw_addr *ha;
		UINT_32 i = 0;

		netdev_for_each_mc_addr(ha, prDev) {
			if (i < MAX_NUM_GROUP_ADDR) {
				COPY_MAC_ADDR(&(prGlueInfo->prP2PDevInfo->aucMCAddrList[i]), ha->addr);
				i++;
			}
		}

		DBGLOG(P2P, TRACE, "SEt Multicast Address List\n");

		if (i >= MAX_NUM_GROUP_ADDR)
			return;
		wlanoidSetP2PMulticastList(prGlueInfo->prAdapter,
					   &(prGlueInfo->prP2PDevInfo->aucMCAddrList[0]),
					   (i * ETH_ALEN), &u4SetInfoLen);

	}

}				/* end of p2pSetMulticastList() */

/*----------------------------------------------------------------------------*/
/*!
 * * \brief This function is TX entry point of NET DEVICE.
 * *
 * * \param[in] prSkb  Pointer of the sk_buff to be sent
 * * \param[in] prDev  Pointer to struct net_device
 * *
 * * \retval NETDEV_TX_OK - on success.
 * * \retval NETDEV_TX_BUSY - on failure, packet will be discarded by upper layer.
 */
/*----------------------------------------------------------------------------*/
int p2pHardStartXmit(IN struct sk_buff *prSkb, IN struct net_device *prDev)
{
	P_NETDEV_PRIVATE_GLUE_INFO prNetDevPrivate = (P_NETDEV_PRIVATE_GLUE_INFO) NULL;
	P_GLUE_INFO_T prGlueInfo = NULL;
	UINT_8 ucBssIndex;

	ASSERT(prSkb);
	ASSERT(prDev);

	prNetDevPrivate = (P_NETDEV_PRIVATE_GLUE_INFO) netdev_priv(prDev);
	prGlueInfo = prNetDevPrivate->prGlueInfo;
	ucBssIndex = prNetDevPrivate->ucBssIdx;

	kalResetPacket(prGlueInfo, (P_NATIVE_PACKET) prSkb);

	kalHardStartXmit(prSkb, prDev, prGlueInfo, ucBssIndex);

	return NETDEV_TX_OK;
}				/* end of p2pHardStartXmit() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief A method of struct net_device, a primary SOCKET interface to configure
 *        the interface lively. Handle an ioctl call on one of our devices.
 *        Everything Linux ioctl specific is done here. Then we pass the contents
 *        of the ifr->data to the request message handler.
 *
 * \param[in] prDev      Linux kernel netdevice
 *
 * \param[in] prIFReq    Our private ioctl request structure, typed for the generic
 *                       struct ifreq so we can use ptr to function
 *
 * \param[in] cmd        Command ID
 *
 * \retval WLAN_STATUS_SUCCESS The IOCTL command is executed successfully.
 * \retval OTHER The execution of IOCTL command is failed.
 */
/*----------------------------------------------------------------------------*/

int p2pDoIOCTL(struct net_device *prDev, struct ifreq *prIfReq, int i4Cmd)
{
	P_GLUE_INFO_T prGlueInfo = NULL;
	int ret = 0;
	/* char *prExtraBuf = NULL; */
	/* UINT_32 u4ExtraSize = 0; */
	/* struct iwreq *prIwReq = (struct iwreq *)prIfReq; */
	/* struct iw_request_info rIwReqInfo; */

	ASSERT(prDev && prIfReq);

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));
	if (!prGlueInfo) {
		DBGLOG(P2P, ERROR, "prGlueInfo is NULL\n");
		return -EFAULT;
	}

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(P2P, ERROR, "Adapter is not ready\n");
		return -EINVAL;
	}

	if (i4Cmd == SIOCDEVPRIVATE + 1) {
		ret = priv_support_driver_cmd(prDev, prIfReq, i4Cmd);

	} else {
		DBGLOG(INIT, WARN, "Unexpected ioctl command: 0x%04x\n", i4Cmd);
		ret = -1;
	}

#if 0
	/* fill rIwReqInfo */
	rIwReqInfo.cmd = (__u16) i4Cmd;
	rIwReqInfo.flags = 0;

	switch (i4Cmd) {
	case SIOCSIWENCODEEXT:
		/* Set Encryption Material after 4-way handshaking is done */
		if (prIwReq->u.encoding.pointer) {
			u4ExtraSize = prIwReq->u.encoding.length;
			prExtraBuf = kalMemAlloc(u4ExtraSize, VIR_MEM_TYPE);

			if (!prExtraBuf) {
				ret = -ENOMEM;
				break;
			}

			if (copy_from_user(prExtraBuf, prIwReq->u.encoding.pointer, prIwReq->u.encoding.length))
				ret = -EFAULT;
		} else if (prIwReq->u.encoding.length != 0) {
			ret = -EINVAL;
			break;
		}

		if (ret == 0)
			ret = mtk_p2p_wext_set_key(prDev, &rIwReqInfo, &(prIwReq->u), prExtraBuf);

		kalMemFree(prExtraBuf, VIR_MEM_TYPE, u4ExtraSize);
		prExtraBuf = NULL;
		break;

	case SIOCSIWMLME:
		/* IW_MLME_DISASSOC used for disconnection */
		if (prIwReq->u.data.length != sizeof(struct iw_mlme)) {
			DBGLOG(INIT, INFO, "MLME buffer strange:%d\n", prIwReq->u.data.length);
			ret = -EINVAL;
			break;
		}

		if (!prIwReq->u.data.pointer) {
			ret = -EINVAL;
			break;
		}

		prExtraBuf = kalMemAlloc(sizeof(struct iw_mlme), VIR_MEM_TYPE);
		if (!prExtraBuf) {
			ret = -ENOMEM;
			break;
		}

		if (copy_from_user(prExtraBuf, prIwReq->u.data.pointer, sizeof(struct iw_mlme)))
			ret = -EFAULT;
		else
			ret = mtk_p2p_wext_mlme_handler(prDev, &rIwReqInfo, &(prIwReq->u), prExtraBuf);

		kalMemFree(prExtraBuf, VIR_MEM_TYPE, sizeof(struct iw_mlme));
		prExtraBuf = NULL;
		break;

	case SIOCGIWPRIV:
		/* This ioctl is used to list all IW privilege ioctls */
		ret = mtk_p2p_wext_get_priv(prDev, &rIwReqInfo, &(prIwReq->u), NULL);
		break;

	case SIOCGIWSCAN:
		ret = mtk_p2p_wext_discovery_results(prDev, &rIwReqInfo, &(prIwReq->u), NULL);
		break;

	case SIOCSIWAUTH:
		ret = mtk_p2p_wext_set_auth(prDev, &rIwReqInfo, &(prIwReq->u), NULL);
		break;

	case IOC_P2P_CFG_DEVICE:
	case IOC_P2P_PROVISION_COMPLETE:
	case IOC_P2P_START_STOP_DISCOVERY:
	case IOC_P2P_DISCOVERY_RESULTS:
	case IOC_P2P_WSC_BEACON_PROBE_RSP_IE:
	case IOC_P2P_CONNECT_DISCONNECT:
	case IOC_P2P_PASSWORD_READY:
	case IOC_P2P_GET_STRUCT:
	case IOC_P2P_SET_STRUCT:
	case IOC_P2P_GET_REQ_DEVICE_INFO:
#if 0
		ret = rP2PIwPrivHandler[i4Cmd - SIOCIWFIRSTPRIV](prDev,
									&rIwReqInfo,
									&(prIwReq->u),
									(char *)&(prIwReq->u));
#endif
		break;
#if CFG_SUPPORT_P2P_RSSI_QUERY
	case SIOCGIWSTATS:
		ret = mtk_p2p_wext_get_rssi(prDev, &rIwReqInfo, &(prIwReq->u), NULL);
		break;
#endif
	default:
		ret = -ENOTTY;
	}
#endif /* 0 */

	return ret;
}				/* end of p2pDoIOCTL() */


/*----------------------------------------------------------------------------*/
/*!
 * \brief To override p2p interface address
 *
 * \param[in] prDev Net device requested.
 * \param[in] addr  Pointer to address
 *
 * \retval 0 For success.
 * \retval -E2BIG For user's buffer size is too small.
 * \retval -EFAULT For fail.
 *
 */
/*----------------------------------------------------------------------------*/
int p2pSetMACAddress(IN struct net_device *prDev, void *addr)
{
	P_ADAPTER_T prAdapter = NULL;
	P_GLUE_INFO_T prGlueInfo = NULL;

	ASSERT(prDev);

	prGlueInfo = *((P_GLUE_INFO_T *) netdev_priv(prDev));
	ASSERT(prGlueInfo);

	prAdapter = prGlueInfo->prAdapter;
	ASSERT(prAdapter);

	/* @FIXME */
	return eth_mac_addr(prDev, addr);
}

