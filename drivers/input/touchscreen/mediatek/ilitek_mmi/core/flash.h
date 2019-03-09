/*
 * ILITEK Touch IC driver
 *
 * Copyright (C) 2011 ILI Technology Corporation.
 *
 * Author: Dicky Chiang <dicky_chiang@ilitek.com>
 * Based on TDD v7.0 implemented by Mstar & ILITEK
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef __FLASH_H
#define __FLASH_H


#define NUM_OF_4_MULTIPLE(x)  ((((x) + 3) / 4) * 4)


/* DMA Control Registers */
#define DMA_BASED_ADDR 	0x72000
#define DMA48_ADDR  (DMA_BASED_ADDR+0xC0)
#define DMA48_reg_dma_ch0_busy_flag         DMA48_ADDR
#define DMA48_reserved_0                    0xFFFE
#define DMA48_reg_dma_ch0_trigger_sel       (BIT(16)|BIT(17)|BIT(18)|BIT(19)) // [RESET] h0
#define DMA48_reserved_1                    (BIT(20)|BIT(21)|BIT(22)|BIT(23))
#define DMA48_reg_dma_ch0_start_set         BIT(24) // [RESET] h0
#define DMA48_reg_dma_ch0_start_clear       BIT(25) // [RESET] h0
#define DMA48_reg_dma_ch0_trigger_src_mask  BIT(26) // [RESET] h0
#define DMA48_reserved_2                    BIT(27)

#define DMA49_ADDR  (DMA_BASED_ADDR+0xC4)
#define DMA49_reg_dma_ch0_src1_addr         DMA49_ADDR // [RESET] h0
#define DMA49_reserved_0                    BIT(20)

#define DMA50_ADDR  (DMA_BASED_ADDR+0xC8)
#define DMA50_reg_dma_ch0_src1_step_inc     DMA50_ADDR // [RESET] h0
#define DMA50_reserved_0                    (DMA50_ADDR+0x01)
#define DMA50_reg_dma_ch0_src1_format       (BIT(24)|BIT(25))// [RESET] h0
#define DMA50_reserved_1                    (BIT(26)|BIT(27)|BIT(28)|BIT(29)|BIT(30))
#define DMA50_reg_dma_ch0_src1_en           BIT(31) // [RESET] h1

#define DMA52_ADDR  (DMA_BASED_ADDR+0xD0)
#define DMA52_reg_dma_ch0_src2_step_inc     DMA52_ADDR // [RESET] h0
#define DMA52_reserved_0                    (DMA52_ADDR+0x01)
#define DMA52_reg_dma_ch0_src2_format       (BIT(24)|BIT(25)) // [RESET] h0
#define DMA52_reserved_1                    (BIT(26)|BIT(27)|BIT(28)|BIT(29)|BIT(30))
#define DMA52_reg_dma_ch0_src2_en           BIT(31) // [RESET] h0

#define DMA53_ADDR  (DMA_BASED_ADDR+0xD4)
#define DMA53_reg_dma_ch0_dest_addr         DMA53_ADDR // [RESET] h0
#define DMA53_reserved_0                    BIT(20)

#define DMA54_ADDR  (DMA_BASED_ADDR+0xD8)
#define DMA54_reg_dma_ch0_dest_step_inc     DMA54_ADDR // [RESET] h0
#define DMA54_reserved_0                    (DMA54_ADDR+0x01)
#define DMA54_reg_dma_ch0_dest_format       (BIT(24)|BIT(25)) // [RESET] h0
#define DMA54_reserved_1                    (BIT(26)|BIT(27)|BIT(28)|BIT(29)|BIT(30))
#define DMA54_reg_dma_ch0_dest_en           BIT(31) // [RESET] h1

#define DMA55_ADDR  (DMA_BASED_ADDR+0xDC)
#define DMA55_reg_dma_ch0_trafer_counts     DMA55_ADDR // [RESET] h0
#define DMA55_reserved_0                    (BIT(17)|BIT(18)|BIT(19)|BIT(20)|BIT(21)|BIT(22)|BIT(23))
#define DMA55_reg_dma_ch0_trafer_mode       (BIT(24)|BIT(25)|BIT(26)|BIT(27)) // [RESET] h0
#define DMA55_reserved_1                    (BIT(28)|BIT(29)|BIT(30)|BIT(31))

/* INT Function Registers */
#define INTR_BASED_ADDR     0x48000
#define INTR1_ADDR   (INTR_BASED_ADDR+0x4)
#define INTR1_reg_uart_tx_int_flag            INTR1_ADDR // [RESET] h0
#define INTR1_reserved_0                      (BIT(1)|BIT(2)|BIT(3)|BIT(4)|BIT(5)|BIT(6)|BIT(7))
#define INTR1_reg_wdt_alarm_int_flag          BIT(8) // [RESET] h0
#define INTR1_reserved_1                      (BIT(9)|BIT(10)|BIT(11)|BIT(12)|BIT(13)|BIT(14)|BIT(15))
#define INTR1_reg_dma_ch1_int_flag            BIT(16) // [RESET] h0
#define INTR1_reg_dma_ch0_int_flag            BIT(17) // [RESET] h0
#define INTR1_reg_dma_frame_done_int_flag     BIT(18) // [RESET] h0
#define INTR1_reg_dma_tdi_done_int_flag       BIT(19) // [RESET] h0
#define INTR1_reserved_2                      (BIT(20)|BIT(21)|BIT(22)|BIT(23))
#define INTR1_reg_flash_error_flag            BIT(24) // [RESET] h0
#define INTR1_reg_flash_int_flag              BIT(25) // [RESET] h0
#define INTR1_reserved_3                      BIT(26)

#define INTR2_ADDR  (INTR_BASED_ADDR+0x8)
#define INTR2_td_int_flag_clear                    INTR2_ADDR
#define INTR2_td_timeout_int_flag_clear            BIT(1)
#define INTR2_td_debug_frame_done_int_flag_clear   BIT(2)
#define INTR2_td_frame_start_scan_int_flag_clear   BIT(3)
#define INTR2_log_int_flag_clear                   BIT(4)
#define INTR2_d2t_crc_err_int_flag_clear           BIT(8)
#define INTR2_d2t_flash_req_int_flag_clear         BIT(9)
#define INTR2_d2t_ddi_int_flag_clear               BIT(10)
#define INTR2_wr_done_int_flag_clear               BIT(16)
#define INTR2_rd_done_int_flag_clear               BIT(17)
#define INTR2_tdi_err_int_flag_clear               BIT(18)
#define INTR2_d2t_slpout_rise_flag_clear           BIT(24)
#define INTR2_d2t_slpout_fall_flag_clear           BIT(25)
#define INTR2_d2t_dstby_flag_clear                 BIT(26)
#define INTR2_ddi_pwr_rdy_flag_clear               BIT(27)

#define INTR32_ADDR  (INTR_BASED_ADDR+0x80)
#define INTR32_reg_ice_sw_int_en 			  INTR32_ADDR // [RESET] h0
#define INTR32_reg_ice_apb_conflict_int_en	  BIT(1) // [RESET] h0
#define INTR32_reg_ice_ilm_conflict_int_en	  BIT(2) // [RESET] h0
#define INTR32_reg_ice_dlm_conflict_int_en	  BIT(3) // [RESET] h0
#define INTR32_reserved_0 	  				  (BIT(4)|BIT(5)|BIT(6)|BIT(7))
#define INTR32_reg_spi_sr_int_en			  BIT(8) // [RESET] h0
#define INTR32_reg_spi_sp_int_en			  BIT(9) // [RESET] h0
#define INTR32_reg_spi_trx_int_en			  BIT(10) // [RESET] h0
#define INTR32_reg_spi_cmd_int_en			  BIT(11) // [RESET] h0
#define INTR32_reg_spi_rw_int_en			  BIT(12) // [RESET] h0
#define INTR32_reserved_1					  (BIT(13)|BIT(14)|BIT(15))
#define INTR32_reg_i2c_start_int_en			  BIT(16) // [RESET] h0
#define INTR32_reg_i2c_addr_match_int_en	  BIT(17) // [RESET] h0
#define INTR32_reg_i2c_cmd_int_en			  BIT(18) // [RESET] h0
#define INTR32_reg_i2c_sr_int_en			  BIT(19) // [RESET] h0
#define INTR32_reg_i2c_trx_int_en			  BIT(20) // [RESET] h0
#define INTR32_reg_i2c_rx_stop_int_en		  BIT(21) // [RESET] h0
#define INTR32_reg_i2c_tx_stop_int_en		  BIT(22) // [RESET] h0
#define INTR32_reserved_2					  BIT(23);
#define INTR32_reg_t0_int_en				  BIT(24) // [RESET] h0
#define INTR32_reg_t1_int_en				  BIT(25) // [RESET] h0
#define INTR32_reserved_3					  (BIT(26)|BIT(27)|BIT(28)|BIT(29)|BIT(30)|BIT(31))

#define INTR33_ADDR  (INTR_BASED_ADDR+0x84)
#define INTR33_reg_uart_tx_int_en             INTR33_ADDR // [RESET] h0
#define INTR33_reserved_0                     (BIT(1)|BIT(2)|BIT(3)|BIT(4)|BIT(5)|BIT(6)|BIT(7))
#define INTR33_reg_wdt_alarm_int_en           BIT(8) // [RESET] h0
#define INTR33_reserved_1                     (BIT(9)|BIT(10)|BIT(11)|BIT(12)|BIT(13)|BIT(14)|BIT(15))
#define INTR33_reg_dma_ch1_int_en             BIT(16) // [RESET] h0
#define INTR33_reg_dma_ch0_int_en             BIT(17) // [RESET] h0
#define INTR33_reg_dma_frame_done_int_en      BIT(18) // [RESET] h0
#define INTR33_reg_dma_tdi_done_int_en        BIT(19) // [RESET] h0
#define INTR33_reserved_2                     (BIT(20)|BIT(21)|BIT(22)|BIT(23))
#define INTR33_reg_flash_error_en             BIT(24) // [RESET] h0
#define INTR33_reg_flash_int_en               BIT(25) // [RESET] h0
#define INTR33_reserved_3                     (BIT(26)|BIT(27)|BIT(28)|BIT(29)|BIT(30)|BIT(31))

/* FLASH Control Function Registers */
#define FLASH_BASED_ADDR   0x41000
#define FLASH0_ADDR  (FLASH_BASED_ADDR+0x0)
#define FLASH0_reg_flash_csb                  FLASH0_ADDR//1 ; // [RESET] h1
#define FLASH0_reserved_0                     (BIT(8)|BIT(9)|BIT(10)|BIT(11)|BIT(12)|BIT(13)|BIT(14)|BIT(15))
#define FLASH0_reg_preclk_sel		          (BIT(16)|BIT(17)|BIT(18)|BIT(19)|BIT(20)|BIT(21)|BIT(22)|BIT(23)) //FLASH0_ADDR+16 //4 ; // [RESET] h2 // struct member reg write is not permitted, please use byte, half-word, word operation instead.
#define FLASH0_reg_rx_dual                    BIT(24) // [RESET] h0
#define FLASH0_reg_tx_dual                    BIT(25) // [RESET] h0
#define FLASH0_reserved_26                    (BIT(26)|BIT(27)|BIT(28)|BIT(29)|BIT(30)|BIT(31))

#define FLASH1_ADDR  (FLASH_BASED_ADDR+0x4)
#define FLASH1_reg_flash_key1                 FLASH1_ADDR // [RESET] h0 // struct member reg write is not permitted, please use byte, half-word, word operation instead.
#define FLASH1_reg_flash_key2                 (FLASH1_ADDR+0x01) // [RESET] h0 // struct member reg write is not permitted, please use byte, half-word, word operation instead.
#define FLASH1_reg_flash_key3                 (FLASH1_ADDR+0x02) // [RESET] h0 // struct member reg write is not permitted, please use byte, half-word, word operation instead.
#define FLASH1_reserved_0                     (FLASH1_ADDR+0x03)

#define FLASH2_ADDR  (FLASH_BASED_ADDR+0x8)
#define FLASH2_reg_tx_data                    FLASH2_ADDR //8 ; // [RESET] h0 // struct member reg write is not permitted, please use byte, half-word, word operation instead.

#define FLASH3_ADDR  (FLASH_BASED_ADDR+0xC)
#define FLASH3_reg_rcv_cnt                    FLASH3_ADDR //20 ; // [RESET] h0

#define FLASH4_ADDR  (FLASH_BASED_ADDR+0x10)
#define FLASH4_reg_rcv_data 				  FLASH4_ADDR // [RESET] h0 // struct member reg write is not permitted, please use byte, half-word, word operation instead.
#define FLASH4_reg_rcv_dly					  BIT(8)// [RESET] h0
#define FLASH4_reg_sutrg_en					  BIT(9) // [RESET] h0
#define FLASH4_reserved_1					  (BIT(10)|BIT(11)|BIT(12)|BIT(13)|BIT(14)|BIT(15))
#define FLASH4_reg_rcv_data_valid_state       BIT(16)// [RESET] h0 // struct member reg write is not permitted, please use byte, half-word, word operation instead.
#define FLASH4_reg_flash_rd_finish_state	  BIT(17);
#define FLASH4_reserved_2					  (BIT(18)|BIT(19)|BIT(20)|BIT(21)|BIT(22)|BIT(23))
#define FLASH4_reg_flash_dma_trigger_en		  (BIT(24)|BIT(25)|BIT(26)|BIT(27)|BIT(28)|BIT(29)|BIT(30)|BIT(31))//1 ; // [RESET] h0

/* SYSTEM Function Register */
#define SYS_BASED_ADDR             0x40000
#define SYS53_ADDR  (SYS_BASED_ADDR+0xD4)
#define SYS53_reg_dummy_0 					   SYS53_ADDR	// [RESET] h0

struct flash_table {
	uint16_t mid;
	uint16_t dev_id;
	int mem_size;
	int program_page;
	int sector;
	int block;
};

extern struct flash_table *flashtab;

extern void core_flash_dma_clear(void);
extern void core_flash_dma_write(uint32_t start, uint32_t end, uint32_t len);
extern int core_flash_poll_busy(int timer);
extern int core_flash_write_enable(void);
extern void core_flash_enable_protect(bool status);
extern void core_flash_init(uint16_t mid, uint16_t did);

#endif /* __FLASH_H */
