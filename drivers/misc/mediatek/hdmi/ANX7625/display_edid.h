/*
*
SiI8348 Linux Driver

Copyright (C) 2013 Silicon Image, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation version 2.
This program is distributed AS-IS WITHOUT ANY WARRANTY of any
kind, whether express or implied; INCLUDING without the implied warranty
of MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE or NON-INFRINGEMENT.  See
the GNU General Public License for more details at http://www.gnu.org/licenses/gpl-2.0.html.

*/

#if !defined(SI_EDID_H)
#define SI_EDID_H

/******************************************/
/*must align to extd_drv.h*/
enum {
	HDMI_CHANNEL_2 = 0x2,
	HDMI_CHANNEL_3 = 0x3,
	HDMI_CHANNEL_4 = 0x4,
	HDMI_CHANNEL_5 = 0x5,
	HDMI_CHANNEL_6 = 0x6,
	HDMI_CHANNEL_7 = 0x7,
	HDMI_CHANNEL_8 = 0x8,
};

enum {
	HDMI_SAMPLERATE_32  = 0x1,
	HDMI_SAMPLERATE_44  = 0x2,
	HDMI_SAMPLERATE_48  = 0x3,
	HDMI_SAMPLERATE_96  = 0x4,
	HDMI_SAMPLERATE_192 = 0x5,
};

enum {
	HDMI_BITWIDTH_16  = 0x1,
	HDMI_BITWIDTH_24  = 0x2,
	HDMI_BITWIDTH_20  = 0x3,
};

enum {
	AUDIO_32K_2CH		= 0x01,
	AUDIO_44K_2CH		= 0x02,
	AUDIO_48K_2CH		= 0x03,
	AUDIO_96K_2CH		= 0x05,
	AUDIO_192K_2CH	= 0x07,
	AUDIO_32K_8CH		= 0x81,
	AUDIO_44K_8CH		= 0x82,
	AUDIO_48K_8CH		= 0x83,
	AUDIO_96K_8CH		= 0x85,
	AUDIO_192K_8CH	= 0x87,
	AUDIO_INITIAL		= 0xFF
};

/*
*
enum
{
	VIDEO_3D_NONE		= 0x00,
	VIDEO_3D_FS		= 0x01,
	VIDEO_3D_TB		= 0x02,
	VIDEO_3D_SS		= 0x03,
	VIDEO_3D_INITIAL	= 0xFF
};
*/

/* Video mode define ( = VIC code, please see CEA-861 spec)*/
#define HDMI_640X480P		1
#define HDMI_480I60_4X3	6
#define HDMI_480I60_16X9	7
#define HDMI_576I50_4X3	21
#define HDMI_576I50_16X9	22
#define HDMI_480P60_4X3	2
#define HDMI_480P60_16X9	3
#define HDMI_576P50_4X3	17
#define HDMI_576P50_16X9	18
#define HDMI_720P60			4
#define HDMI_720P50			19
#define HDMI_1080I60		5
#define HDMI_1080I50		20
#define HDMI_1080P24		32
#define HDMI_1080P25		33
#define HDMI_1080P30		34
#define HDMI_1080P60		16 /*MHL doesn't supported*/
#define HDMI_1080P50		31 /*MHL doesn't supported*/
#define HDMI_4k30_DSC		95 /*MHL doesn't supported*/
#define HDMI_4k25_DSC		94 /*MHL doesn't supported*/
#define HDMI_4k24_DSC		93 /*MHL doesn't supported*/

#define HDMI_SPECAIL_RES	190 /*the value is Forbidden resolution*/
#define HDMI_2k70_DSC		191 /*the value is Forbidden resolution*/


/*****************macros********************/
#define PLACE_IN_CODE_SEG

#define SI_PUSH_STRUCT_PACKING
#define SI_POP_STRUCT_PACKING
#define SI_PACK_THIS_STRUCT	__attribute__((__packed__))
#define SII_OFFSETOF offsetof

#define SII_ASSERT(cond, ...)	\
do {							\
	if (!(cond)) {				\
		printk(__VA_ARGS__);	\
		BUG();					\
	}							\
} while (0)

enum __3D_structure_e {
	tdsFramePacking = 0x00
	, tdsTopAndBottom = 0x06
	, tdsSideBySide   = 0x08
};
#define _3D_structure_e enum __3D_structure_e
struct _MHL2_high_low_t {
	uint8_t high;
	uint8_t low;
};
#define MHL2_high_low_t struct _MHL2_high_low_t
#define PMHL2_high_low_t struct _MHL2_high_low_t *

/* see MHL2.0 spec section 5.9.1.2*/
struct _MHL2_video_descriptor_t {
	uint8_t reserved_high;
	unsigned char frame_sequential:1;	/*FB_SUPP*/
	unsigned char top_bottom:1;				/*TB_SUPP*/
	unsigned char left_right:1;				/*LR_SUPP*/
	unsigned char reserved_low:5;
};
#define MHL2_video_descriptor_t struct _MHL2_video_descriptor_t
#define PMHL2_video_descriptor_t struct _MHL2_video_descriptor_t *

struct _MHL2_video_format_data_t {
	MHL2_high_low_t burst_id;
	uint8_t checksum;
	uint8_t total_entries;
	uint8_t sequence_index;
	uint8_t num_entries_this_burst;
	MHL2_video_descriptor_t video_descriptors[5];
};
#define MHL2_video_format_data_t struct _MHL2_video_format_data_t
#define PMHL2_video_format_data_t struct _MHL2_video_format_data_t *

struct SI_PACK_THIS_STRUCT _TwoBytes_t {
	unsigned char low;
	unsigned char high;
};
#define TwoBytes_t struct SI_PACK_THIS_STRUCT _TwoBytes_t
#define PTwoBytes_t struct SI_PACK_THIS_STRUCT _TwoBytes_t *


#define EDID_EXTENSION_TAG  0x02
#define EDID_EXTENSION_BLOCK_MAP 0xF0
#define EDID_REV_THREE      0x03
/*#define LONG_DESCR_LEN                  18*/
#define EDID_BLOCK_0        0x00
#define EDID_BLOCK_2_3      0x01
/*#define EDID_BLOCK_0_OFFSET 0x0000*/

enum _data_block_tag_code_e {
	DBTC_TERMINATOR												= 0
	, DBTC_AUDIO_DATA_BLOCK								= 1
	, DBTC_VIDEO_DATA_BLOCK								= 2
	, DBTC_VENDOR_SPECIFIC_DATA_BLOCK			= 3
	, DBTC_SPEAKER_ALLOCATION_DATA_BLOCK	= 4
	, DBTC_VESA_DTC_DATA_BLOCK						= 5
		/*reserved					= 6*/
	, DBTC_USE_EXTENDED_TAG								= 7
};
#define data_block_tag_code_e enum _data_block_tag_code_e
struct SI_PACK_THIS_STRUCT _data_block_header_fields_t {
	uint8_t length_following_header:5;

	data_block_tag_code_e tag_code:3;
};
#define data_block_header_fields_t struct SI_PACK_THIS_STRUCT _data_block_header_fields_t
#define Pdata_block_header_fields_t struct SI_PACK_THIS_STRUCT _data_block_header_fields_t *

union SI_PACK_THIS_STRUCT _data_block_header_byte_t {
	data_block_header_fields_t	fields;
	uint8_t						as_byte;
};
#define data_block_header_byte_t union SI_PACK_THIS_STRUCT _data_block_header_byte_t
#define Pdata_block_header_byte_t union SI_PACK_THIS_STRUCT _data_block_header_byte_t *

enum _extended_tag_code_e {
	ETC_VIDEO_CAPABILITY_DATA_BLOCK                    =  0
	, ETC_VENDOR_SPECIFIC_VIDEO_DATA_BLOCK                =  1
	, ETC_VESA_VIDEO_DISPLAY_DEVICE_INFORMATION_DATA_BLOCK  =  2
	, ETC_VESA_VIDEO_DATA_BLOCK                          =  3
	, ETC_HDMI_VIDEO_DATA_BLOCK                          =  4
	, ETC_COLORIMETRY_DATA_BLOCK                        =  5
	, ETC_VIDEO_RELATED                                =  6

	, ETC_CEA_MISC_AUDIO_FIELDS                          = 16
	, ETC_VENDOR_SPECIFIC_AUDIO_DATA_BLOCK                = 17
	, ETC_HDMI_AUDIO_DATA_BLOCK                          = 18
	, ETC_AUDIO_RELATED                               = 19

	, ETC_GENERAL                                     = 32
};
#define extended_tag_code_e enum _extended_tag_code_e

struct SI_PACK_THIS_STRUCT _extended_tag_code_t {
	extended_tag_code_e etc:8;
};
#define extended_tag_code_t struct SI_PACK_THIS_STRUCT _extended_tag_code_t
#define Pextended_tag_code_t struct SI_PACK_THIS_STRUCT _extended_tag_code_t *

struct SI_PACK_THIS_STRUCT _cea_short_descriptor_t {
	unsigned char VIC:7;
	unsigned char native:1;

};
#define cea_short_descriptor_t struct SI_PACK_THIS_STRUCT _cea_short_descriptor_t
#define Pcea_short_descriptor_t struct SI_PACK_THIS_STRUCT _cea_short_descriptor_t *

struct SI_PACK_THIS_STRUCT  _MHL_short_desc_t {
	cea_short_descriptor_t cea_short_desc;
	MHL2_video_descriptor_t mhl_vid_desc;
};
#define MHL_short_desc_t struct SI_PACK_THIS_STRUCT  _MHL_short_desc_t
#define PMHL_short_desc_t struct SI_PACK_THIS_STRUCT  _MHL_short_desc_t *

struct SI_PACK_THIS_STRUCT _video_data_block_t {
	data_block_header_byte_t header;
	cea_short_descriptor_t short_descriptors[1];/*open ended*/
};
#define video_data_block_t struct SI_PACK_THIS_STRUCT _video_data_block_t
#define p_video_data_block_t struct SI_PACK_THIS_STRUCT _video_data_block_t *

enum _AudioFormatCodes_e {
	/* reserved             =  0*/
	afd_linear_PCM_IEC60958 =  1
	, afd_AC3                =  2
	, afd_MPEG1_layers_1_2   =  3
	, afd_MPEG1_layer_3      =  4
	, afdMPEG2_MultiChannel  =  5
	, afd_AAC                =  6
	, afd_DTS                =  7
	, afd_ATRAC              =  8
	, afd_one_bit_audio      =  9
	, afd_dolby_digital      = 10
	, afd_DTS_HD             = 11
	, afd_MAT_MLP            = 12
	, afd_DST                = 13
	, afd_WMA_Pro            = 14
	/*reserved				= 15*/
};
#define AudioFormatCodes_e enum _AudioFormatCodes_e

struct SI_PACK_THIS_STRUCT _CEA_short_audio_descriptor_t {
	unsigned char       max_channels_minus_one:3;

	AudioFormatCodes_e  audio_format_code:4;
	unsigned char       F17:1;

	unsigned char freq_32_Khz:1;
	unsigned char freq_44_1_KHz:1;
	unsigned char freq_48_KHz:1;
	unsigned char freq_88_2_KHz:1;
	unsigned char freq_96_KHz:1;
	unsigned char freq_176_4_KHz:1;
	unsigned char freq_192_KHz:1;
	unsigned char F27:1;

	union {
		struct SI_PACK_THIS_STRUCT {
			unsigned res_16_bit:1;
			unsigned res_20_bit:1;
			unsigned res_24_bit:1;
			unsigned F33_37:5;
		} audio_code_1_LPCM;
		struct SI_PACK_THIS_STRUCT {
			uint8_t max_bit_rate_div_by_8_KHz;
		} audio_codes_2_8;
		struct SI_PACK_THIS_STRUCT {
			uint8_t default_zero;
		} audio_codes_9_15;
	} byte3;
};
#define CEA_short_audio_descriptor_t struct SI_PACK_THIS_STRUCT _CEA_short_audio_descriptor_t
#define PCEA_short_audio_descriptor_t struct SI_PACK_THIS_STRUCT _CEA_short_audio_descriptor_t *

struct SI_PACK_THIS_STRUCT _audio_data_block_t {
	data_block_header_byte_t header;
	CEA_short_audio_descriptor_t  short_audio_descriptors[1]; /* open ended*/
};
#define audio_data_block_t struct SI_PACK_THIS_STRUCT _audio_data_block_t
#define Paudio_data_block_t struct SI_PACK_THIS_STRUCT _audio_data_block_t *

struct SI_PACK_THIS_STRUCT _speaker_allocation_flags_t {
	unsigned char   spk_front_left_front_right:1;
	unsigned char   spk_LFE:1;
	unsigned char   spk_front_center:1;
	unsigned char   spk_rear_left_rear_right:1;
	unsigned char   spk_rear_center:1;
	unsigned char   spk_front_left_center_front_right_center:1;
	unsigned char   spk_rear_left_center_rear_right_center:1;
	unsigned char   spk_reserved:1;
};
#define speaker_allocation_flags_t struct SI_PACK_THIS_STRUCT _speaker_allocation_flags_t
#define Pspeaker_allocation_flags_t struct SI_PACK_THIS_STRUCT _speaker_allocation_flags_t *

struct SI_PACK_THIS_STRUCT _speaker_allocation_data_block_payload_t {
	speaker_allocation_flags_t speaker_alloc_flags;

	uint8_t         reserved[2];
};
#define speaker_allocation_data_block_payload_t struct SI_PACK_THIS_STRUCT _speaker_allocation_data_block_payload_t
#define Pspeaker_allocation_data_block_payload_t struct SI_PACK_THIS_STRUCT _speaker_allocation_data_block_payload_t *

struct SI_PACK_THIS_STRUCT _speaker_allocation_data_block_t {
	data_block_header_byte_t header;
	speaker_allocation_data_block_payload_t payload;

};
#define speaker_allocation_data_block_t struct SI_PACK_THIS_STRUCT _speaker_allocation_data_block_t
#define Pspeaker_allocation_data_block_t struct SI_PACK_THIS_STRUCT _speaker_allocation_data_block_t *

struct SI_PACK_THIS_STRUCT _HDMI_LLC_BA_t {
	unsigned char B:4;
	unsigned char A:4;
};
#define HDMI_LLC_BA_t struct SI_PACK_THIS_STRUCT _HDMI_LLC_BA_t
#define PHDMI_LLC_BA_t struct SI_PACK_THIS_STRUCT _HDMI_LLC_BA_t *

struct SI_PACK_THIS_STRUCT _HDMI_LLC_DC_t {
	unsigned char D:4;
	unsigned char C:4;
};
#define HDMI_LLC_DC_t struct SI_PACK_THIS_STRUCT _HDMI_LLC_DC_t
#define PHDMI_LLC_DC_t struct SI_PACK_THIS_STRUCT _HDMI_LLC_DC_t *

struct SI_PACK_THIS_STRUCT _HDMI_LLC_Byte6_t {
	unsigned char DVI_dual:1;
	unsigned char reserved:2;
	unsigned char DC_Y444:1;
	unsigned char DC_30bit:1;
	unsigned char DC_36bit:1;
	unsigned char DC_48bit:1;
	unsigned char supports_AI:1;
};
#define HDMI_LLC_Byte6_t struct SI_PACK_THIS_STRUCT _HDMI_LLC_Byte6_t
#define PHDMI_LLC_Byte6_t struct SI_PACK_THIS_STRUCT _HDMI_LLC_Byte6_t *

struct SI_PACK_THIS_STRUCT _HDMI_LLC_byte8_t {
	unsigned char CNC0_adjacent_pixels_independent:1;
	unsigned char CNC1_specific_processing_still_pictures:1;
	unsigned char CNC2_specific_processing_cinema_content:1;
	unsigned char CNC3_specific_processing_low_AV_latency:1;
	unsigned char reserved:1;
	unsigned char HDMI_video_present:1;
	unsigned char I_latency_fields_present:1;
	unsigned char latency_fields_present:1;
};
#define HDMI_LLC_byte8_t struct SI_PACK_THIS_STRUCT _HDMI_LLC_byte8_t
#define PHDMI_LLC_byte8_t struct SI_PACK_THIS_STRUCT _HDMI_LLC_byte8_t *

enum _image_size_e {
	imsz_NO_ADDITIONAL						= 0
	, imsz_ASPECT_RATIO_CORRECT_BUT_NO_GUARRANTEE_OF_CORRECT_SIZE	= 1
	, imsz_CORRECT_SIZES_ROUNDED_TO_NEAREST_1_CM		= 2
	, imsz_CORRECT_SIZES_DIVIDED_BY_5_ROUNDED_TO_NEAREST_5_CM		= 3
};
#define image_size_e enum _image_size_e

struct SI_PACK_THIS_STRUCT _HDMI_LLC_Byte13_t {
	unsigned char reserved:3;

	image_size_e image_size:2;
	unsigned char _3D_multi_present:2;
	unsigned char _3D_present:1;
};
#define HDMI_LLC_Byte13_t struct SI_PACK_THIS_STRUCT _HDMI_LLC_Byte13_t
#define PHDMI_LLC_Byte13_t struct SI_PACK_THIS_STRUCT _HDMI_LLC_Byte13_t *

struct SI_PACK_THIS_STRUCT _HDMI_LLC_Byte14_t {
	unsigned char HDMI_3D_len:5;
	unsigned char HDMI_VIC_len:3;
};
#define HDMI_LLC_Byte14_t struct SI_PACK_THIS_STRUCT _HDMI_LLC_Byte14_t
#define PHDMI_LLC_Byte14_t struct SI_PACK_THIS_STRUCT _HDMI_LLC_Byte14_t *

struct SI_PACK_THIS_STRUCT _VSDB_byte_13_through_byte_15_t {
	HDMI_LLC_Byte13_t    byte13;
	HDMI_LLC_Byte14_t    byte14;
	uint8_t vicList[1]; /* variable length list base on HDMI_VIC_len*/
};
#define VSDB_byte_13_through_byte_15_t struct SI_PACK_THIS_STRUCT _VSDB_byte_13_through_byte_15_t
#define PVSDB_byte_13_through_byte_15_t struct SI_PACK_THIS_STRUCT _VSDB_byte_13_through_byte_15_t *

struct SI_PACK_THIS_STRUCT _VSDB_all_fields_byte_9_through_byte15_t {
	uint8_t video_latency;
	uint8_t audio_latency;
	uint8_t interlaced_video_latency;
	uint8_t interlaced_audio_latency;
	VSDB_byte_13_through_byte_15_t   byte_13_through_byte_15;
	/* There must be no fields after here*/
};
#define VSDB_all_fields_byte_9_through_byte15_t struct SI_PACK_THIS_STRUCT _VSDB_all_fields_byte_9_through_byte15_t
#define PVSDB_all_fields_byte_9_through_byte15_t struct SI_PACK_THIS_STRUCT _VSDB_all_fields_byte_9_through_byte15_t *

struct SI_PACK_THIS_STRUCT _VSDB_all_fields_byte_9_through_byte_15_sans_progressive_latency_t {
	uint8_t interlaced_video_latency;
	uint8_t interlaced_audio_latency;
	VSDB_byte_13_through_byte_15_t   byte_13_through_byte_15;
	/* There must be no fields after here*/
};
#define VSDB_all_fields_byte_9_through_byte_15_sans_progressive_latency_t \
	struct SI_PACK_THIS_STRUCT _VSDB_all_fields_byte_9_through_byte_15_sans_progressive_latency_t
#define PVSDB_all_fields_byte_9_through_byte_15_sans_progressive_latency_t \
	struct SI_PACK_THIS_STRUCT _VSDB_all_fields_byte_9_through_byte_15_sans_progressive_latency_t *

struct SI_PACK_THIS_STRUCT _VSDB_all_fields_byte_9_through_byte_15_sans_interlaced_latency_t {
	uint8_t video_latency;
	uint8_t audio_latency;
	VSDB_byte_13_through_byte_15_t   byte_13_through_byte_15;
	/* There must be no fields after here*/
};
#define VSDB_all_fields_byte_9_through_byte_15_sans_interlaced_latency_t \
	struct SI_PACK_THIS_STRUCT _VSDB_all_fields_byte_9_through_byte_15_sans_interlaced_latency_t
#define PVSDB_all_fields_byte_9_through_byte_15_sans_interlaced_latency_t \
	struct SI_PACK_THIS_STRUCT _VSDB_all_fields_byte_9_through_byte_15_sans_interlaced_latency_t *

struct SI_PACK_THIS_STRUCT _VSDB_all_fields_byte_9_through_byte_15_sans_all_latency_t {
	VSDB_byte_13_through_byte_15_t   byte_13_through_byte_15;
	/* There must be no fields after here*/
};
#define VSDB_all_fields_byte_9_through_byte_15_sans_all_latency_t \
	struct SI_PACK_THIS_STRUCT _VSDB_all_fields_byte_9_through_byte_15_sans_all_latency_t
#define PVSDB_all_fields_byte_9_through_byte_15_sans_all_latency_t \
	struct SI_PACK_THIS_STRUCT _VSDB_all_fields_byte_9_through_byte_15_sans_all_latency_t *

struct SI_PACK_THIS_STRUCT _HDMI_LLC_vsdb_payload_t {
	HDMI_LLC_BA_t B_A;
	HDMI_LLC_DC_t D_C;
	HDMI_LLC_Byte6_t byte6;
	uint8_t maxTMDSclock;
	HDMI_LLC_byte8_t byte8;
	union {
		VSDB_all_fields_byte_9_through_byte_15_sans_all_latency_t
			vsdb_all_fields_byte_9_through_byte_15_sans_all_latency;
		VSDB_all_fields_byte_9_through_byte_15_sans_progressive_latency_t
			vsdb_all_fields_byte_9_through_byte_15_sans_progressive_latency;
		VSDB_all_fields_byte_9_through_byte_15_sans_interlaced_latency_t
			vsdb_all_fields_byte_9_through_byte_15_sans_interlaced_latency;
		VSDB_all_fields_byte_9_through_byte15_t
			vsdb_all_fields_byte_9_through_byte_15;
	} vsdb_fields_byte_9_through_byte_15;
	/* There must be no fields after here*/
};
#define HDMI_LLC_vsdb_payload_t struct SI_PACK_THIS_STRUCT _HDMI_LLC_vsdb_payload_t
#define PHDMI_LLC_vsdb_payload_t struct SI_PACK_THIS_STRUCT _HDMI_LLC_vsdb_payload_t *

struct SI_PACK_THIS_STRUCT st_3D_structure_all_15_8_t {
	uint8_t frame_packing:1;
	uint8_t reserved1:5;
	uint8_t top_bottom:1;
	uint8_t reserved2:1;
};
#define _3D_structure_all_15_8_t struct SI_PACK_THIS_STRUCT st_3D_structure_all_15_8_t
#define P_3D_structure_all_15_8_t struct SI_PACK_THIS_STRUCT st_3D_structure_all_15_8_t *

struct SI_PACK_THIS_STRUCT st_3D_structure_all_7_0_t {
	uint8_t side_by_side:1;
	uint8_t reserved:7;
};
#define _3D_structure_all_7_0_t struct SI_PACK_THIS_STRUCT st_3D_structure_all_7_0_t
#define P_3D_structure_all_7_0_t struct SI_PACK_THIS_STRUCT st_3D_structure_all_7_0_t *


struct SI_PACK_THIS_STRUCT tag_3D_structure_all_t {
	_3D_structure_all_15_8_t _3D_structure_all_15_8;
	_3D_structure_all_7_0_t _3D_structure_all_7_0;
};
#define _3D_structure_all_t struct SI_PACK_THIS_STRUCT tag_3D_structure_all_t
#define P_3D_structure_all_t struct SI_PACK_THIS_STRUCT tag_3D_structure_all_t *


struct SI_PACK_THIS_STRUCT tag_3D_mask_t {
	uint8_t _3D_mask_15_8;
	uint8_t _3D_mask_7_0;
};
#define _3D_mask_t struct SI_PACK_THIS_STRUCT tag_3D_mask_t
#define P_3D_mask_t struct SI_PACK_THIS_STRUCT tag_3D_mask_t *



struct SI_PACK_THIS_STRUCT tag_2D_VIC_order_3D_structure_t {
	_3D_structure_e _3D_structure:4;    /* definition from info frame*/
	unsigned    _2D_VIC_order:4;
};
#define _2D_VIC_order_3D_structure_t struct SI_PACK_THIS_STRUCT tag_2D_VIC_order_3D_structure_t
#define P_2D_VIC_order_3D_structure_t struct SI_PACK_THIS_STRUCT tag_2D_VIC_order_3D_structure_t *

struct SI_PACK_THIS_STRUCT tag_3D_detail_t {
	unsigned char reserved:4;
	unsigned char _3D_detail:4;
};
#define _3D_detail_t struct SI_PACK_THIS_STRUCT tag_3D_detail_t
#define P_3D_detail_t struct SI_PACK_THIS_STRUCT tag_3D_detail_t *

struct SI_PACK_THIS_STRUCT tag_3D_structure_and_detail_entry_sans_byte1_t {
	_2D_VIC_order_3D_structure_t    byte0;
	/*see HDMI 1.4 spec w.r.t. contents of 3D_structure_X */
};
#define _3D_structure_and_detail_entry_sans_byte1_t \
	struct SI_PACK_THIS_STRUCT tag_3D_structure_and_detail_entry_sans_byte1_t
#define P_3D_structure_and_detail_entry_sans_byte1_t \
	struct SI_PACK_THIS_STRUCT tag_3D_structure_and_detail_entry_sans_byte1_t *

struct SI_PACK_THIS_STRUCT tag_3D_structure_and_detail_entry_with_byte1_t {
	_2D_VIC_order_3D_structure_t    byte0;
	_3D_detail_t                    byte1;
};
#define _3D_structure_and_detail_entry_with_byte1_t \
	struct SI_PACK_THIS_STRUCT tag_3D_structure_and_detail_entry_with_byte1_t
#define P_3D_structure_and_detail_entry_with_byte1_t \
	struct SI_PACK_THIS_STRUCT tag_3D_structure_and_detail_entry_with_byte1_t *

union tag_3D_structure_and_detail_entry_u {
	_3D_structure_and_detail_entry_sans_byte1_t   sans_byte1;
	_3D_structure_and_detail_entry_with_byte1_t   with_byte1;
};
#define _3D_structure_and_detail_entry_u union tag_3D_structure_and_detail_entry_u
#define P_3D_structure_and_detail_entry_u union tag_3D_structure_and_detail_entry_u *

struct SI_PACK_THIS_STRUCT _HDMI_3D_sub_block_sans_all_AND_mask_t {
	_3D_structure_and_detail_entry_u    _3D_structure_and_detail_list[1];
};
#define HDMI_3D_sub_block_sans_all_AND_mask_t \
	struct SI_PACK_THIS_STRUCT _HDMI_3D_sub_block_sans_all_AND_mask_t
#define PHDMI_3D_sub_block_sans_all_AND_mask_t \
	struct SI_PACK_THIS_STRUCT _HDMI_3D_sub_block_sans_all_AND_mask_t *

struct SI_PACK_THIS_STRUCT _HDMI_3D_sub_block_sans_mask_t {
	_3D_structure_all_t  _3D_structure_all;
	_3D_structure_and_detail_entry_u    _3D_structure_and_detail_list[1];
};
#define HDMI_3D_sub_block_sans_mask_t struct SI_PACK_THIS_STRUCT _HDMI_3D_sub_block_sans_mask_t
#define PHDMI_3D_sub_block_sans_mask_t struct SI_PACK_THIS_STRUCT _HDMI_3D_sub_block_sans_mask_t *

struct SI_PACK_THIS_STRUCT _HDMI_3D_sub_block_with_all_AND_mask_t {
	_3D_structure_all_t  _3D_structure_all;
	_3D_mask_t          _3D_mask;
	_3D_structure_and_detail_entry_u    _3D_structure_and_detail_list[1];
};
#define HDMI_3D_sub_block_with_all_AND_mask_t \
	struct SI_PACK_THIS_STRUCT _HDMI_3D_sub_block_with_all_AND_mask_t
#define PHDMI_3D_sub_block_with_all_AND_mask_t \
	struct SI_PACK_THIS_STRUCT _HDMI_3D_sub_block_with_all_AND_mask_t *

union tag_HDMI_3D_sub_block_t {
	HDMI_3D_sub_block_sans_all_AND_mask_t   hDMI_3D_sub_block_sans_all_AND_mask;
	HDMI_3D_sub_block_sans_mask_t         HDMI_3D_sub_block_sans_mask;
	HDMI_3D_sub_block_with_all_AND_mask_t   HDMI_3D_sub_block_with_all_AND_mask;
};
#define HDMI_3D_sub_block_t union tag_HDMI_3D_sub_block_t
#define PHDMI_3D_sub_block_t union tag_HDMI_3D_sub_block_t *

struct SI_PACK_THIS_STRUCT _vsdb_t {
	data_block_header_byte_t header;
	uint8_t IEEE_OUI[3];
	union {
		HDMI_LLC_vsdb_payload_t HDMI_LLC;
		uint8_t payload[1]; /* open ended*/
	} payload_u;
};
#define vsdb_t struct SI_PACK_THIS_STRUCT _vsdb_t
#define P_vsdb_t struct SI_PACK_THIS_STRUCT _vsdb_t *

enum _colorimetry_xvYCC_e {
	xvYCC_601   = 1
	, xvYCC_709   = 2
};
#define colorimetry_xvYCC_e enum _colorimetry_xvYCC_e

struct SI_PACK_THIS_STRUCT _colorimetry_xvYCC_t {
	colorimetry_xvYCC_e     xvYCC:2;
	unsigned char           reserved1:6;
};
#define colorimetry_xvYCC_t struct SI_PACK_THIS_STRUCT _colorimetry_xvYCC_t
#define Pcolorimetry_xvYCC_t struct SI_PACK_THIS_STRUCT _colorimetry_xvYCC_t *

struct SI_PACK_THIS_STRUCT _colorimetry_meta_data_t {
	unsigned char           meta_data:3;
	unsigned char           reserved2:5;
};
#define colorimetry_meta_data_t struct SI_PACK_THIS_STRUCT _colorimetry_meta_data_t
#define Pcolorimetry_meta_data_t struct SI_PACK_THIS_STRUCT _colorimetry_meta_data_t *

struct SI_PACK_THIS_STRUCT _colorimetry_data_payload_t {
	colorimetry_xvYCC_t     ci_data;
	colorimetry_meta_data_t  cm_meta_data;
};
#define colorimetry_data_payload_t struct SI_PACK_THIS_STRUCT _colorimetry_data_payload_t
#define Pcolorimetry_data_payload_t struct SI_PACK_THIS_STRUCT _colorimetry_data_payload_t *
struct SI_PACK_THIS_STRUCT _colorimetry_data_block_t {
	data_block_header_byte_t   header;
	extended_tag_code_t       extended_tag;
	colorimetry_data_payload_t payload;

};
#define colorimetry_data_block_t struct SI_PACK_THIS_STRUCT _colorimetry_data_block_t
#define Pcolorimetry_data_block_t struct SI_PACK_THIS_STRUCT _colorimetry_data_block_t *

enum _CE_overscan_underscan_behavior_e {
	ceou_NEITHER                    = 0
	, ceou_ALWAYS_OVERSCANNED         = 1
	, ceou_ALWAYS_UNDERSCANNED        = 2
	, ceou_BOTH                       = 3
};
#define CE_overscan_underscan_behavior_e enum _CE_overscan_underscan_behavior_e

enum _IT_overscan_underscan_behavior_e {
	itou_NEITHER                   = 0
	, itou_ALWAYS_OVERSCANNED        = 1
	, itou_ALWAYS_UNDERSCANNED       = 2
	, itou_BOTH                      = 3
};
#define IT_overscan_underscan_behavior_e enum _IT_overscan_underscan_behavior_e

enum _PT_overscan_underscan_behavior_e {
	ptou_NEITHER                    = 0
	, ptou_ALWAYS_OVERSCANNED          = 1
	, ptou_ALWAYS_UNDERSCANNED         = 2
	, ptou_BOTH                       = 3
};
#define PT_overscan_underscan_behavior_e enum _PT_overscan_underscan_behavior_e

struct SI_PACK_THIS_STRUCT _video_capability_data_payload_t {
	CE_overscan_underscan_behavior_e S_CE:2;
	IT_overscan_underscan_behavior_e S_IT:2;
	PT_overscan_underscan_behavior_e S_PT:2;
	unsigned QS:1;
	unsigned quantization_range_selectable:1;
};
#define video_capability_data_payload_t struct SI_PACK_THIS_STRUCT _video_capability_data_payload_t
#define Pvideo_capability_data_payload_t struct SI_PACK_THIS_STRUCT _video_capability_data_payload_t *

struct SI_PACK_THIS_STRUCT _video_capability_data_block_t {
	data_block_header_byte_t   header;
	extended_tag_code_t       extended_tag;
	video_capability_data_payload_t payload;

};
#define video_capability_data_block_t struct SI_PACK_THIS_STRUCT _video_capability_data_block_t
#define Pvideo_capability_data_block_t struct SI_PACK_THIS_STRUCT _video_capability_data_block_t *

struct SI_PACK_THIS_STRUCT _CEA_data_block_collection_t {
	data_block_header_byte_t header;

	union {
		extended_tag_code_t extended_tag;
		cea_short_descriptor_t short_descriptor;
	} payload_u;
	/* open ended array of cea_short_descriptor_t starts here*/
};
#define CEA_data_block_collection_t struct SI_PACK_THIS_STRUCT _CEA_data_block_collection_t
#define PCEA_data_block_collection_t struct SI_PACK_THIS_STRUCT _CEA_data_block_collection_t *

struct SI_PACK_THIS_STRUCT _CEA_extension_version_1_t {
	uint8_t reservedMustBeZero;
	uint8_t reserved[123];
};
#define CEA_extension_version_1_t struct SI_PACK_THIS_STRUCT _CEA_extension_version_1_t
#define PCEA_extension_version_1_t struct SI_PACK_THIS_STRUCT _CEA_extension_version_1_t *

struct SI_PACK_THIS_STRUCT _CEA_extension_2_3_misc_support_t {
	uint8_t total_number_detailed_timing_descriptors_in_entire_EDID:4;
	uint8_t YCrCb422_support:1;
	uint8_t YCrCb444_support:1;
	uint8_t basic_audio_support:1;
	uint8_t underscan_IT_formats_by_default:1;
};
#define CEA_extension_2_3_misc_support_t struct SI_PACK_THIS_STRUCT _CEA_extension_2_3_misc_support_t
#define PCEA_extension_2_3_misc_support_t struct SI_PACK_THIS_STRUCT _CEA_extension_2_3_misc_support_t *
struct SI_PACK_THIS_STRUCT _CEA_extension_version_2_t {
	CEA_extension_2_3_misc_support_t misc_support;

	uint8_t reserved[123];
};
#define CEA_extension_version_2_t struct SI_PACK_THIS_STRUCT _CEA_extension_version_2_t
#define PCEA_extension_version_2_t struct SI_PACK_THIS_STRUCT _CEA_extension_version_2_t *

struct SI_PACK_THIS_STRUCT _CEA_extension_version_3_t {
	CEA_extension_2_3_misc_support_t misc_support;
	union {
		uint8_t data_block_collection[123];
		uint8_t reserved[123];
	} Offset4_u;
};
#define CEA_extension_version_3_t struct SI_PACK_THIS_STRUCT _CEA_extension_version_3_t
#define PCEA_extension_version_3_t struct SI_PACK_THIS_STRUCT _CEA_extension_version_3_t *

struct SI_PACK_THIS_STRUCT _block_map_t {
	uint8_t tag;
	uint8_t block_tags[126];
	uint8_t checksum;
};
#define block_map_t struct SI_PACK_THIS_STRUCT _block_map_t
#define Pblock_map_t struct SI_PACK_THIS_STRUCT _block_map_t *

struct SI_PACK_THIS_STRUCT _CEA_extension_t {
	uint8_t tag;
	uint8_t revision;
	uint8_t byte_offset_to_18_byte_descriptors;
	union {
		CEA_extension_version_1_t      version1;
		CEA_extension_version_2_t      version2;
		CEA_extension_version_3_t      version3;
	} version_u;
	uint8_t checksum;
};
#define CEA_extension_t struct SI_PACK_THIS_STRUCT _CEA_extension_t
#define PCEA_extension_t struct SI_PACK_THIS_STRUCT _CEA_extension_t *

struct SI_PACK_THIS_STRUCT _detailed_timing_descriptor_t {
	uint8_t pixel_clock_low;
	uint8_t pixel_clock_high;
	uint8_t horz_active_7_0;
	uint8_t horz_blanking_7_0;
	struct SI_PACK_THIS_STRUCT {
		unsigned char horz_blanking_11_8:4;
		unsigned char horz_active_11_8:4;
	} horz_active_blanking_high;
	uint8_t vert_active_7_0;
	uint8_t vert_blanking_7_0;
	struct SI_PACK_THIS_STRUCT {
		unsigned char vert_blanking_11_8:4;
		unsigned char vert_active_11_8:4;
	} vert_active_blanking_high;
	uint8_t horz_sync_offset_7_0;
	uint8_t horz_sync_pulse_width7_0;
	struct SI_PACK_THIS_STRUCT {
		unsigned char vert_sync_pulse_width_3_0:4;
		unsigned char vert_sync_offset_3_0:4;
	} vert_sync_offset_width;
	struct SI_PACK_THIS_STRUCT {
		unsigned char vert_sync_pulse_width_5_4:2;
		unsigned char vert_sync_offset_5_4:2;
		unsigned char horz_sync_pulse_width_9_8:2;
		unsigned char horzSyncOffset9_8:2;
	} hs_offset_hs_pulse_width_vs_offset_vs_pulse_width;
	uint8_t horz_image_size_in_mm_7_0;
	uint8_t vert_image_size_in_mm_7_0;
	struct SI_PACK_THIS_STRUCT {
		unsigned char vert_image_size_in_mm_11_8:4;
		unsigned char horz_image_size_in_mm_11_8:4;
	} image_size_high;
	uint8_t horz_border_in_lines;
	uint8_t vert_border_in_pixels;
	struct SI_PACK_THIS_STRUCT {
		unsigned char stereo_bit_0:1;
		unsigned char sync_signal_options:2;
		unsigned char sync_signal_type:2;
		unsigned char stereo_bits_2_1:2;
		unsigned char interlaced:1;
	} flags;
};
#define detailed_timing_descriptor_t struct SI_PACK_THIS_STRUCT _detailed_timing_descriptor_t
#define Pdetailed_timing_descriptor_t struct SI_PACK_THIS_STRUCT _detailed_timing_descriptor_t *

struct SI_PACK_THIS_STRUCT _red_green_bits_1_0_t {
	unsigned char   green_y:2;
	unsigned char   green_x:2;
	unsigned char   red_y:2;
	unsigned char   red_x:2;
};
#define red_green_bits_1_0_t struct SI_PACK_THIS_STRUCT _red_green_bits_1_0_t
#define Pred_green_bits_1_0_t struct SI_PACK_THIS_STRUCT _red_green_bits_1_0_t *

struct SI_PACK_THIS_STRUCT _blue_white_bits_1_0_t {
	unsigned char   white_y:2;
	unsigned char   white_x:2;
	unsigned char   blue_y:2;
	unsigned char   blue_x:2;

};
#define blue_white_bits_1_0_t struct SI_PACK_THIS_STRUCT _blue_white_bits_1_0_t
#define Pblue_white_bits_1_0_t struct SI_PACK_THIS_STRUCT _blue_white_bits_1_0_t *


struct SI_PACK_THIS_STRUCT _established_timings_I_t {
	unsigned char et800x600_60p:1;
	unsigned char et800x600_56p:1;
	unsigned char et640x480_75p:1;
	unsigned char et640x480_72p:1;
	unsigned char et640x480_67p:1;
	unsigned char et640x480_60p:1;
	unsigned char et720x400_88p:1;
	unsigned char et720x400_70p:1;
};
#define established_timings_I_t struct SI_PACK_THIS_STRUCT _established_timings_I_t
#define Pestablished_timings_I_t struct SI_PACK_THIS_STRUCT _established_timings_I_t *

struct SI_PACK_THIS_STRUCT _established_timings_II_t {
	unsigned char et1280x1024_75p:1;
	unsigned char et1024x768_75p:1;
	unsigned char et1024x768_70p:1;
	unsigned char et1024x768_60p:1;
	unsigned char et1024x768_87i:1;
	unsigned char et832x624_75p:1;
	unsigned char et800x600_75p:1;
	unsigned char et800x600_72p:1;
};
#define established_timings_II_t struct SI_PACK_THIS_STRUCT _established_timings_II_t
#define Pestablished_timings_II_t struct SI_PACK_THIS_STRUCT _established_timings_II_t *

struct SI_PACK_THIS_STRUCT _manufacturers_timings_t {
	unsigned char   reserved:7;
	unsigned char   et1152x870_75p:1;
};
#define manufacturers_timings_t struct SI_PACK_THIS_STRUCT _manufacturers_timings_t
#define Pmanufacturers_timings_t struct SI_PACK_THIS_STRUCT _manufacturers_timings_t *

enum _image_aspect_ratio_e {
	iar_16_to_10   = 0
	, iar_4_to_3    = 1
	, iar_5_to_4    = 2
	, iar_16_to_9   = 3
};
#define image_aspect_ratio_e enum _image_aspect_ratio_e

struct SI_PACK_THIS_STRUCT _standard_timing_t {
	unsigned char   horz_pix_div_8_minus_31;

	unsigned char   field_refresh_rate_minus_60:6;

	image_aspect_ratio_e image_aspect_ratio:2;
};
#define standard_timing_t struct SI_PACK_THIS_STRUCT _standard_timing_t
#define Pstandard_timing_t struct SI_PACK_THIS_STRUCT _standard_timing_t *

struct SI_PACK_THIS_STRUCT _EDID_block0_t {
	unsigned char               header_data[8];
	TwoBytes_t                  id_manufacturer_name;
	TwoBytes_t                  id_product_code;
	unsigned char               serial_number[4];
	unsigned char               week_of_manufacture;
	unsigned char               year_of_manufacture;
	unsigned char               EDID_version;
	unsigned char               EDID_revision;
	unsigned char               video_input_definition;
	unsigned char               horz_screen_size_or_aspect_ratio;
	unsigned char               vert_screen_size_or_aspect_ratio;
	unsigned char               display_transfer_characteristic;
	unsigned char               feature_support;
	red_green_bits_1_0_t           red_green_bits_1_0;
	blue_white_bits_1_0_t          blue_white_bits_1_0;
	unsigned char               red_x;
	unsigned char               red_y;
	unsigned char               green_x;
	unsigned char               green_y;
	unsigned char               blue_x;
	unsigned char               blue_y;
	unsigned char               white_x;
	unsigned char               white_y;
	established_timings_I_t       established_timings_I;
	established_timings_II_t      established_timings_II;
	manufacturers_timings_t      manufacturers_timings;
	standard_timing_t            standard_timings[8];
	detailed_timing_descriptor_t  detailed_timing_descriptors[4];
	unsigned char               extension_flag;
	unsigned char               checksum;

};
#define EDID_block0_t struct SI_PACK_THIS_STRUCT _EDID_block0_t
#define PEDID_block0_t struct SI_PACK_THIS_STRUCT _EDID_block0_t *

struct SI_PACK_THIS_STRUCT _monitor_name_t {
	uint8_t flag_required[2];
	uint8_t flag_reserved;
	uint8_t data_type_tag;
	uint8_t flag;
	uint8_t ascii_name[13];


};
#define monitor_name_t struct SI_PACK_THIS_STRUCT _monitor_name_t
#define Pmonitor_name_t struct SI_PACK_THIS_STRUCT _monitor_name_t *

struct SI_PACK_THIS_STRUCT _monitor_range_limits_t {

	uint8_t flag_required[2];
	uint8_t flag_reserved;
	uint8_t data_type_tag;
	uint8_t flag;
	uint8_t min_vertical_rate_in_Hz;
	uint8_t max_vertical_rate_in_Hz;
	uint8_t min_horizontal_rate_in_KHz;
	uint8_t max_horizontal_rate_in_KHz;
	uint8_t max_pixel_clock_in_MHz_div_10;
	uint8_t tag_secondary_formula;
	uint8_t filler[7];
};
#define monitor_range_limits_t struct SI_PACK_THIS_STRUCT _monitor_range_limits_t
#define Pmonitor_range_limits_t struct SI_PACK_THIS_STRUCT _monitor_range_limits_t *


union tag_18_byte_descriptor_u {
	detailed_timing_descriptor_t  dtd;
	monitor_name_t               name;
	monitor_range_limits_t        range_limits;
};
#define _18_byte_descriptor_u union tag_18_byte_descriptor_u
#define P_18_byte_descriptor_u union tag_18_byte_descriptor_u *


struct SI_PACK_THIS_STRUCT _display_mode_3D_info_t {
	unsigned char dmi_3D_supported:1;
	unsigned char dmi_sufficient_bandwidth:1;
};
#define display_mode_3D_info_t struct SI_PACK_THIS_STRUCT _display_mode_3D_info_t
#define Pdisplay_mode_3D_info_t struct SI_PACK_THIS_STRUCT _display_mode_3D_info_t *

enum _VIC_info_flags_e {
	vif_single_frame_rate = 0x00
	, vif_dual_frame_rate   = 0x01

};
#define VIC_info_flags_e enum _VIC_info_flags_e

enum _VIC_scan_mode_e {
	vsm_progressive  = 0
	, vsm_interlaced   = 1
};
#define VIC_scan_mode_e enum _VIC_scan_mode_e

enum _pixel_aspect_ratio_e {
	par_1_to_1
	, par_16_to_15
	, par_16_to_27
	, par_16_to_45
	, par_16_to_45_160_to_45
	, par_1_to_15_10_to_15
	, par_1_to_9_10_to_9
	, par_2_to_15_20_to_15
	, par_2_to_9
	, par_2_to_9_20_to_9
	, par_32_to_27
	, par_32_to_45
	, par_4_to_27_40_to_27
	, par_4_to_9
	, par_4_to_15
	, par_64_to_45
	, par_8_to_15
	, par_8_to_27
	, par_8_to_27_80_to_27
	, par_8_to_45_80_to_45
	, par_8_to_9
};
#define pixel_aspect_ratio_e enum _pixel_aspect_ratio_e



struct SI_PACK_THIS_STRUCT _VIC_info_fields_t {
	image_aspect_ratio_e  image_aspect_ratio:2;
	VIC_scan_mode_e			interlaced:1;
	pixel_aspect_ratio_e	pixel_aspect_ratio:5;

	VIC_info_flags_e  frame_rate_info:1;
	uint8_t         clocks_per_pixel_shift_count:2;
	uint8_t         field2_v_blank:2;
	uint8_t         reserved:3;
};
#define VIC_info_fields_t struct SI_PACK_THIS_STRUCT _VIC_info_fields_t
#define PVIC_info_fields_t struct SI_PACK_THIS_STRUCT _VIC_info_fields_t *



struct SI_PACK_THIS_STRUCT _VIC_info_t {
	uint16_t columns;
	uint16_t rows;
	uint16_t h_blank_in_pixels;
	uint16_t v_blank_in_pixels;
	uint32_t field_rate_in_milliHz;
	VIC_info_fields_t fields;
/*    uint16_t pixClockDiv10000;*/
};
#define VIC_info_t struct SI_PACK_THIS_STRUCT _VIC_info_t
#define PVIC_info_t struct SI_PACK_THIS_STRUCT _VIC_info_t *

struct SI_PACK_THIS_STRUCT _HDMI_VIC_info_t {
	uint16_t columns;
	uint16_t rows;
	uint32_t field_rate_0_in_milliHz, field_rate_1_in_milliHz;
	uint32_t pixel_clock_0, pixel_clock_1;
};
#define HDMI_VIC_info_t struct SI_PACK_THIS_STRUCT _HDMI_VIC_info_t
#define PHDMI_VIC_info_t struct SI_PACK_THIS_STRUCT _HDMI_VIC_info_t *



void dump_EDID_block_impl(const char *pszFunction, int iLineNum, uint8_t override, uint8_t *pData, uint16_t length);
/*void clear_EDID_block_impl(uint8_t *pData);*/
#define DUMP_EDID_BLOCK(override, pData, length)	\
	dump_EDID_block_impl(__func__, __LINE__, override, (uint8_t *)pData, length)
/*#define CLEAR_EDID_BLOCK(pData) clear_EDID_block_impl(pData);*/

extern bool SlimPort_3D_Support;

enum EDID_error_codes {
	EDID_OK,
	EDID_INCORRECT_HEADER,
	EDID_CHECKSUM_ERROR,
	EDID_NO_861_EXTENSIONS,
	EDID_SHORT_DESCRIPTORS_OK,
	EDID_LONG_DESCRIPTORS_OK,
	EDID_EXT_TAG_ERROR,
	EDID_REV_ADDR_ERROR,
	EDID_V_DESCR_OVERFLOW,
	EDID_UNKNOWN_TAG_CODE,
	EDID_NO_DETAILED_DESCRIPTORS,
	EDID_DDC_BUS_REQ_FAILURE,
	EDID_DDC_BUS_RELEASE_FAILURE,
	EDID_READ_TIMEOUT
};

#endif /* #if !defined(SI_EDID_H) */
