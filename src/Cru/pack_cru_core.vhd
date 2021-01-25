-- 20180412 JB  Add adress definiion for add_gbt_link_mask_hi  =X"0000_0020"
--                                       add_gbt_link_mask_med =X"0000_0024"
--                                       add_gbt_link_mask_lo  =X"0000_0028"
-- 20180713 JB  Change adress definition for GBT register after change the Avalon 
--              component type
--              Add adress definition for add_gbt_wrapper_test_control = x"0000_0008"

library ieee;
use ieee.std_logic_1164.ALL;
use ieee.numeric_std.all;

package pack_cru_core is

-- type declaration
type Array2bit is array(natural range <>) of std_logic_vector(1 downto 0);
type Array3bit is array(natural range <>) of std_logic_vector(2 downto 0);
type Array4bit is array(natural range <>) of std_logic_vector(3 downto 0);
type Array8bit is array(natural range <>) of std_logic_vector(7 downto 0);
type Array16bit is array(natural range <>) of std_logic_vector(15 downto 0);
type Array24bit is array(natural range <>) of std_logic_vector(23 downto 0);
type Array32bit is array(natural range <>) of std_logic_vector(31 downto 0);
type Array33bit is array(natural range <>) of std_logic_vector(32 downto 0);
type Array64bit is array(natural range <>) of std_logic_vector(63 downto 0);
type Array80bit is array(natural range <>) of std_logic_vector(79 downto 0);
type Array84bit is array(natural range <>) of std_logic_vector(83 downto 0);
type Array120bit is array(natural range <>) of std_logic_vector(119 downto 0);
type Array128bit is array(natural range <>) of std_logic_vector(127 downto 0);
type Array256bit is array(natural range <>) of std_logic_vector(255 downto 0);


type t_cru_gbt is record
	data_valid : std_logic;     -- valid one tick out of 6
	is_data_sel    : std_logic; -- equivalent to current bit 119
	icec    : std_logic_vector(3 downto 0); -- bit 115 downto 112
	data  : std_logic_vector(111 downto 0); -- in GBT mode bit 79 to 0 are valid, others are 0, in widebus bit 111 to 0 are valids
end record t_cru_gbt;

type t_cru_gbt_array is array (natural range <>) of t_cru_gbt; 

-------------------------------------------------------------------------------
----             transcodage matrix for the GBT enumerate                  ----
-------------------------------------------------------------------------------

-- constant swap_table_fid : t_swap_table := 
--  ( 36 + 11, 36 + 9, 36 + 7, 36 + 5, 36 + 3, 36 + 1,
--    36 + 10, 36 + 8, 36 + 6, 36 + 4, 36 + 2, 36 + 0,
--    24 + 11, 24 + 9, 24 + 7, 24 + 5, 24 + 3, 24 + 1,
--    24 + 0, 24 + 2, 24 + 4, 24 + 6, 24 + 8, 24 + 10,
--    12 + 11, 12 + 9, 12 + 7, 12 + 3, 12 + 5, 12 + 1,
--    12 + 0, 12 + 2, 12 + 4, 12 + 6, 12 + 8, 12 + 10,
--    00 + 11, 00 + 9, 00 + 7, 00 + 5, 00 + 3, 00 + 1,
--    00 + 0, 00 + 2, 00 + 4, 00 + 8, 00 + 10, 00 + 6  );

 type t_swap_table is array (47 downto 0) of integer;
    
 constant swap_table : t_swap_table := 
  ( 36 + 11, 36 +  9, 36 + 7, 36 + 5, 36 + 3, 36 + 1,
    36 + 10, 36 +  8, 36 + 6, 36 + 4, 36 + 2, 36 + 0,
    24 + 11, 24 +  9, 24 + 7, 24 + 5, 24 + 3, 24 + 1,
    24 + 10, 24 +  8, 24 + 6, 24 + 4, 24 + 2, 24 + 0,
    12 + 11, 12 +  9, 12 + 7, 12 + 5, 12 + 3, 12 + 1,
    12 + 10, 12 +  8, 12 + 6, 12 + 4, 12 + 2, 12 + 0,
    00 + 11, 00 +  9, 00 + 7, 00 + 5, 00 + 3, 00 + 1,
    00 + 10, 00 +  8, 00 + 6, 00 + 4, 00 + 2, 00 + 0  );

-------------------------------------------------------------------------------
----                       Constant definition                             ----
-------------------------------------------------------------------------------
 constant c_GBT_FRAME   : integer := 0;  --! GBT-FRAME encoding (constant definition)
 constant c_WIDE_BUS    : integer := 1;  --! WideBus encoding (constant definition)
 constant c_GBT_DYNAMIC : integer := 2;  --! GBT-FRAME or WideBus encoding can be changed dynamically (constant definition)
 
-- g_GBT_user_type definition : each word correspond to the summ of the 2 4 bit word of the ASCII character

 constant c_GBT         : std_logic_vector(11 downto 0) := x"B69"; -- x"47_42_54"
 constant c_TRD         : std_logic_vector(11 downto 0) := x"978"; -- x"54_52_44"

--------------------------------------------------------------------------------
-- CRU BASE ADD
--------------------------------------------------------------------------------
constant add_bsp                                : unsigned(31 downto 0):=X"0000_0000";
constant add_ro_protocol_base		        : unsigned(31 downto 0):=X"0010_0000";
constant add_ttc_pon			        : unsigned(31 downto 0):=X"0020_0000";
constant add_gbt_wrapper0			: unsigned(31 downto 0):=X"0040_0000";
constant add_gbt_wrapper1			: unsigned(31 downto 0):=X"0050_0000";
constant add_base_datapathwrapper0              : unsigned(31 downto 0):=X"0060_0000";
constant add_base_datapathwrapper1              : unsigned(31 downto 0):=X"0070_0000";
constant add_serial_flash_csr                   : unsigned(31 downto 0):=X"00A0_0000";
constant add_serial_flash_wr_rst                : unsigned(31 downto 0):=X"00B0_0000";
constant add_userlogic    		        : unsigned(31 downto 0):=X"00C0_0000";
constant add_ddg	                        : unsigned(31 downto 0):=X"00D0_0000";
constant add_gbt_sc	        		: unsigned(31 downto 0):=X"00F0_0000";
-------------------------------------------------------------------------------
-- Redaout protocol address tables
-------------------------------------------------------------------------------
constant add_ro_prot_conf_reg 		        : unsigned(31 downto 0):=X"0000_0000"+add_ro_protocol_base;
constant add_ro_prot_check_mask 		: unsigned(31 downto 0):=X"0000_0004"+add_ro_protocol_base;
constant add_ro_prot_alloc_fail 		: unsigned(31 downto 0):=X"0000_0008"+add_ro_protocol_base;
constant add_ro_prot_ttc_linkerr 	        : unsigned(31 downto 0):=X"0000_000C"+add_ro_protocol_base;
constant add_ro_prot_nack_dly_reg 	        : unsigned(31 downto 0):=X"0000_0010"+add_ro_protocol_base;
constant add_ro_prot_system_id   	        : unsigned(31 downto 0):=add_ro_protocol_base+X"0000_0014";

-------------------------------------------------------------------------------
-- GBT address tables
-------------------------------------------------------------------------------
-- GBT wrapper pages
constant add_gbt_wrapper_gregs		: unsigned(31 downto 0):=X"0000_0000";
constant add_gbt_wrapper_bank_offset: unsigned(31 downto 0):=X"0002_0000"; -- multiply by 1 to 6 (for 6 banks)
constant add_gbt_wrapper_atx_pll	: unsigned(31 downto 0):=X"000E_0000"; -- alt_a10_gx_240mhz_atx_pll

-- GBT wrapper reg offsets
constant add_gbt_wrapper_conf0        : unsigned(31 downto 0) := x"0000_0000"; -- RO
constant add_gbt_wrapper_conf1        : unsigned(31 downto 0) := x"0000_0004"; -- RO
constant add_gbt_wrapper_test_control : unsigned(31 downto 0) := x"0000_0008"; -- RW
constant add_gbt_wrapper_clk_cnt      : unsigned(31 downto 0) := x"0000_000C"; -- RO
constant add_gbt_wrapper_refclk0_freq : unsigned(31 downto 0) := x"0000_0010"; -- RO
constant add_gbt_wrapper_refclk1_freq : unsigned(31 downto 0) := x"0000_0014"; -- RO
constant add_gbt_wrapper_refclk2_freq : unsigned(31 downto 0) := x"0000_0018"; -- RO
constant add_gbt_wrapper_refclk3_freq : unsigned(31 downto 0) := x"0000_001C"; -- RO

-- GBT bank pages
constant add_gbt_bank_link_offset     : unsigned(31 downto 0) := x"0000_2000"; -- multiply by 1 to 6 (for 6 links)
constant add_gbt_bank_fpll            : unsigned(31 downto 0) := x"0000_E000"; -- alt_a10_gx_240mhz_fpll

-- GBT link
constant add_gbt_link_regs_offset     : unsigned(31 downto 0) := x"0000_0000";
constant add_gbt_link_xcvr_offset     : unsigned(31 downto 0) := x"0000_1000";-- alt_a10_gx_240mhz_latopt_x1

-- gbt link regs offsets
constant add_gbt_link_status		: unsigned(31 downto 0):=X"0000_0000"; -- RO
constant add_gbt_link_txclk_cnt		: unsigned(31 downto 0):=X"0000_0004"; -- RO
constant add_gbt_link_rxclk_cnt		: unsigned(31 downto 0):=X"0000_0008"; -- RO
constant add_gbt_link_rxframe_32lsb	: unsigned(31 downto 0):=X"0000_000C"; -- RO
constant add_gbt_link_rx_err_cnt	: unsigned(31 downto 0) := x"0000_0010"; -- RO
constant add_gbt_link_FEC_monitoring : unsigned(31 downto 0) := x"0000_001C";

constant add_gbt_link_mask_hi        : unsigned(31 downto 0) := x"0000_0020"; -- W with loopBack
constant add_gbt_link_mask_med	     : unsigned(31 downto 0) := x"0000_0024"; -- W with loopBack
constant add_gbt_link_mask_lo	     : unsigned(31 downto 0) := x"0000_0028"; -- W with loopBack
constant add_gbt_link_tx_ctrl_offset : unsigned(31 downto 0) := x"0000_002c";
constant add_gbt_link_source_sel	 : unsigned(31 downto 0) := x"0000_0030"; -- W with loopBack
constant add_gbt_link_clr_errcnt     : unsigned(31 downto 0) := x"0000_0038"; -- Wr only
constant add_gbt_link_rx_ctrl_offset : unsigned(31 downto 0) := x"0000_003C";

-------------------------------------------------------------------------------
-- GBTSC address tables
-------------------------------------------------------------------------------
-- GBTSCA wrapper pages
-- SCA WR
constant add_gbt_sca_wr_data        		: unsigned(31 downto 0):=add_gbt_sc+X"0000_0000";
constant add_gbt_sca_wr_cmd        		: unsigned(31 downto 0):=add_gbt_sc+X"0000_0004";
constant add_gbt_sca_wr_ctr        		: unsigned(31 downto 0):=add_gbt_sc+X"0000_0008";
-- SCA RD
constant add_gbt_sca_rd_data        		: unsigned(31 downto 0):=add_gbt_sc+X"0000_0010";
constant add_gbt_sca_rd_cmd        		: unsigned(31 downto 0):=add_gbt_sc+X"0000_0014";
constant add_gbt_sca_rd_ctr        		: unsigned(31 downto 0):=add_gbt_sc+X"0000_0018";
constant add_gbt_sca_rd_mon        		: unsigned(31 downto 0):=add_gbt_sc+X"0000_001c";
-- IC
constant add_gbt_ic_wr_data        		: unsigned(31 downto 0):=add_gbt_sc+X"0000_0020";
constant add_gbt_ic_wr_cfg        		: unsigned(31 downto 0):=add_gbt_sc+X"0000_0024";
constant add_gbt_ic_wr_cmd        		: unsigned(31 downto 0):=add_gbt_sc+X"0000_0028";
--
constant add_gbt_ic_rd_data        		: unsigned(31 downto 0):=add_gbt_sc+X"0000_0030";
-- SWT
constant add_gbt_swt_wr_l                       : unsigned(31 downto 0):=add_gbt_sc+X"0000_0040";
constant add_gbt_swt_wr_m                       : unsigned(31 downto 0):=add_gbt_sc+X"0000_0044";
constant add_gbt_swt_wr_h                       : unsigned(31 downto 0):=add_gbt_sc+X"0000_0048";
constant add_gbt_swt_cmd                        : unsigned(31 downto 0):=add_gbt_sc+X"0000_004c";
constant add_gbt_swt_rd_l                       : unsigned(31 downto 0):=add_gbt_sc+X"0000_0050";
constant add_gbt_swt_rd_m                       : unsigned(31 downto 0):=add_gbt_sc+X"0000_0054";
constant add_gbt_swt_rd_h                       : unsigned(31 downto 0):=add_gbt_sc+X"0000_0058";
constant add_gbt_swt_mon                        : unsigned(31 downto 0):=add_gbt_sc+X"0000_005c";
constant add_gbt_swt_word_mon                   : unsigned(31 downto 0):=add_gbt_sc+X"0000_0060";
--
constant add_gbt_sc_link                        : unsigned(31 downto 0):=add_gbt_sc+X"0000_0078";
constant add_gbt_sc_rst                         : unsigned(31 downto 0):=add_gbt_sc+X"0000_007c";

-------------------------------------------------------------------------------
-- TTC PON address tables
-------------------------------------------------------------------------------
constant add_ttc_regs			: unsigned(31 downto 0):=add_ttc_pon+X"0000_0000";
constant add_ttc_onu			: unsigned(31 downto 0):=add_ttc_pon+X"0002_0000";
constant add_ttc_clkgen			: unsigned(31 downto 0):=add_ttc_pon+X"0004_0000";
constant add_ttc_patplayer       	: unsigned(31 downto 0):=add_ttc_pon+X"0006_0000";
constant add_ctp_emu			: unsigned(31 downto 0):=add_ttc_pon+X"0008_0000";

-- reg zone
constant add_ttc_data_ctrl		: unsigned(31 downto 0):=add_ttc_regs+X"0000_0000";
constant add_ttc_hbtrig_ltu	    : unsigned(31 downto 0):=add_ttc_regs+X"0000_0004";
constant add_ttc_phystrig_ltu	: unsigned(31 downto 0):=add_ttc_regs+X"0000_0008";
constant add_ttc_eox_sox_ltu	: unsigned(31 downto 0):=add_ttc_regs+X"0000_000C";
constant add_ttc_ttcok	                : unsigned(31 downto 0):=add_ttc_regs+X"0000_0010";
constant add_ttc_onuerror_sticky        : unsigned(31 downto 0):=add_ttc_regs+X"0000_0014";

constant add_ttc_clkgen_ttc240freq	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_0000";
constant add_ttc_clkgen_glb240freq	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_0004";
constant add_ttc_clkgen_rxref240freq	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_0008";
constant add_ttc_clkgen_txref240freq  	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_000C";
constant add_ttc_clkgen_clkctrl 	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_0010";
constant add_ttc_clkgen_clkstat 	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_0014";
constant add_ttc_clkgen_pllctrlonu 	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_0018";
constant add_ttc_clkgen_pllstatonu 	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_001C";
constant add_ttc_clkgen_phasecnt 	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_0020";
constant add_ttc_clkgen_phasestat 	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_0024";
constant add_ttc_clkgen_phasehist 	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_002C";
constant add_ttc_clkgen_clknotokcnt 	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_0028";


constant add_ttc_clkgen_onufpll 	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_8000";

-- ONU zone
constant add_ttc_onu_ctrl               : unsigned(31 downto 0):=add_ttc_onu+X"0000_0000";

constant add_pon_verinfo	        : unsigned(31 downto 0):=add_ttc_onu+X"0000_0000";
constant add_pon_wrapper_reg	        : unsigned(31 downto 0):=add_ttc_onu+X"0000_2000";
constant add_pon_wrapper_pll	        : unsigned(31 downto 0):=add_ttc_onu+X"0000_4000";
constant add_pon_wrapper_tx		: unsigned(31 downto 0):=add_ttc_onu+X"0000_6000";
constant add_onu_user_logic	        : unsigned(31 downto 0):=add_ttc_onu+X"0000_A000";
constant add_onu_freq_meas		: unsigned(31 downto 0):=add_ttc_onu+X"0000_E000";


-- Pattern player
constant add_patplayer_cfg		: unsigned(31 downto 0):=add_ttc_patplayer+X"0000_0000";
constant add_patplayer_idlepat0		: unsigned(31 downto 0):=add_ttc_patplayer+X"0000_0004";
constant add_patplayer_idlepat1		: unsigned(31 downto 0):=add_ttc_patplayer+X"0000_0008";
constant add_patplayer_idlepat2		: unsigned(31 downto 0):=add_ttc_patplayer+X"0000_000C";
constant add_patplayer_syncpat0		: unsigned(31 downto 0):=add_ttc_patplayer+X"0000_0010";
constant add_patplayer_syncpat1		: unsigned(31 downto 0):=add_ttc_patplayer+X"0000_0014";
constant add_patplayer_syncpat2		: unsigned(31 downto 0):=add_ttc_patplayer+X"0000_0018";
constant add_patplayer_rstpat0		: unsigned(31 downto 0):=add_ttc_patplayer+X"0000_001C";
constant add_patplayer_rstpat1		: unsigned(31 downto 0):=add_ttc_patplayer+X"0000_0020";
constant add_patplayer_rstpat2		: unsigned(31 downto 0):=add_ttc_patplayer+X"0000_0024";
constant add_patplayer_synccnt		: unsigned(31 downto 0):=add_ttc_patplayer+X"0000_0028";
constant add_patplayer_delaycnt  	: unsigned(31 downto 0):=add_ttc_patplayer+X"0000_002C";
constant add_patplayer_rstcnt		: unsigned(31 downto 0):=add_ttc_patplayer+X"0000_0030";
constant add_patplayer_trigsel		: unsigned(31 downto 0):=add_ttc_patplayer+X"0000_0034";
constant add_patplayer_debug		: unsigned(31 downto 0):=add_ttc_patplayer+X"0000_0038";


-- CTP
constant add_ctp_emu_core		: unsigned(31 downto 0):=add_ctp_emu+X"0000_0000";

--ctpemu core
constant add_ctp_emu_ctrl		: unsigned(31 downto 0):=add_ctp_emu_core+X"0000_0000";
constant add_ctp_emu_bc_max		: unsigned(31 downto 0):=add_ctp_emu_core+X"0000_0004";
constant add_ctp_emu_hb_max		: unsigned(31 downto 0):=add_ctp_emu_core+X"0000_0008";
constant add_ctp_emu_prescaler  : unsigned(31 downto 0):=add_ctp_emu_core+X"0000_000C";

constant add_ctp_emu_runmode	        : unsigned(31 downto 0):=add_ctp_emu_core+X"0000_0010";
constant add_ctp_emu_physdiv	        : unsigned(31 downto 0):=add_ctp_emu_core+X"0000_0014";
constant add_ctp_emu_hcdiv	            : unsigned(31 downto 0):=add_ctp_emu_core+X"0000_0018";
constant add_ctp_emu_userbits	        : unsigned(31 downto 0):=add_ctp_emu_core+X"0000_001C";
constant add_ctp_emu_caldiv	            : unsigned(31 downto 0):=add_ctp_emu_core+X"0000_0020";
constant add_ctp_emu_fbct	            : unsigned(31 downto 0):=add_ctp_emu_core+X"0000_0024";


-------------------------------------------------------------------------------
-- DDG address tables
-------------------------------------------------------------------------------
constant add_ddg_ctrl	     : unsigned(31 downto 0):=add_ddg+X"0000_0000";
constant add_ddg_ctrl2	     : unsigned(31 downto 0):=add_ddg+X"0000_0004";
constant add_ddg_ctrl3	     : unsigned(31 downto 0):=add_ddg+X"0000_0008";
constant add_ddg_trgmask     : unsigned(31 downto 0):=add_ddg+X"0000_000C";
constant add_ddg_pkt_cnt     : unsigned(31 downto 0):=add_ddg+X"0000_0010";
constant add_ddg_trgmiss_cnt : unsigned(31 downto 0):=add_ddg+X"0000_0014";

-------------------------------------------------------------------------------
-- datapath wrapper address tables
-------------------------------------------------------------------------------
constant add_dwrapper_gregs		  : unsigned(31 downto 0):=X"0000_0000";
constant add_datapathlink_offset  : unsigned(31 downto 0):=X"0004_0000"; -- add link offset to access it
constant add_flowctrl_offset	  : unsigned(31 downto 0):=X"000C_0000";

-- datapath link page access
constant add_datalink_offset		: unsigned(31 downto 0):=X"0000_2000"; -- to multiply by 0 to 15

-- datapath wrapper global registers
constant add_dwrapper_enreg 	   : unsigned(31 downto 0):=X"0000_0000"; -- WO
constant add_dwrapper_datagenctrl  : unsigned(31 downto 0):=X"0000_0004"; -- WO (cdc)

constant add_dwrapper_datagenstatus: unsigned(31 downto 0):=X"0000_0008"; -- RO
constant add_dwrapper_bigfifo_lvl  : unsigned(31 downto 0):=X"0000_000C"; -- RO
constant add_dwrapper_tot_words	   : unsigned(31 downto 0):=X"0000_0010"; -- RO
constant add_dwrapper_drop_words   : unsigned(31 downto 0):=X"0000_0014"; -- RO
constant add_dwrapper_tot_pkts	   : unsigned(31 downto 0):=X"0000_0018"; -- RO
constant add_dwrapper_drop_pkts    : unsigned(31 downto 0):=X"0000_001C"; -- RO
constant add_dwrapper_lastHBID     : unsigned(31 downto 0):=X"0000_0020"; -- RO
constant add_dwrapper_clockcore    : unsigned(31 downto 0):=X"0000_0024"; -- RO
constant add_dwrapper_clockcore_free: unsigned(31 downto 0):=X"0000_0028"; -- RO
constant add_dwrapper_tot_per_sec  : unsigned(31 downto 0):=X"0000_002C"; -- RO
constant add_dwrapper_drop_per_sec : unsigned(31 downto 0):=X"0000_0030"; -- RO
constant add_dwrapper_trigsize     : unsigned(31 downto 0):=X"0000_0034"; -- WO

--datapath link registers
constant add_datalink_ctrl	      : unsigned(31 downto 0):=X"0000_0000";
constant add_datalink_feeid	      : unsigned(31 downto 0):=X"0000_0004";
constant add_datalink_rej_pkt     : unsigned(31 downto 0):=X"0000_0008";
constant add_datalink_acc_pkt     : unsigned(31 downto 0):=X"0000_000C";
constant add_datalink_forced_pkt  : unsigned(31 downto 0):=X"0000_0010";

-- flow control registers
constant add_flowctrl_ctrlreg		: unsigned(31 downto 0):=X"0000_0000";
constant add_flowctrl_pkt_rej		: unsigned(31 downto 0):=X"0000_0004";
constant add_flowctrl_pkt_tot		: unsigned(31 downto 0):=X"0000_0008";


-------------------------------------------------------------------------------
-- Serial Flash address tables
-------------------------------------------------------------------------------

constant add_serial_flash_wr_data : unsigned(31 downto 0):=X"00B0_0004";


-------------------------------------------------------------------------------
-- BSP address tables
-------------------------------------------------------------------------------
constant add_bsp_info	       : unsigned(31 downto 0):=add_bsp+X"0000_0000";
constant add_bsp_hkeeping      : unsigned(31 downto 0):=add_bsp+X"0001_0000";
constant add_bsp_rsu           : unsigned(31 downto 0):=add_bsp+X"0002_0000";
constant add_bsp_i2c	       : unsigned(31 downto 0):=add_bsp+X"0003_0000";

constant add_bsp_info_dirtystatus  : unsigned(31 downto 0)   :=add_bsp_info+X"0000_0000";
constant add_bsp_info_shorthash    : unsigned(31 downto 0)   :=add_bsp_info+X"0000_0004";
constant add_bsp_info_builddate    : unsigned(31 downto 0)   :=add_bsp_info+X"0000_0008";
constant add_bsp_info_buildtime    : unsigned(31 downto 0)   :=add_bsp_info+X"0000_000C";
constant add_bsp_info_boardtype    : unsigned(31 downto 0)   :=add_bsp_info+X"0000_0010";
constant add_bsp_info_userctrl     : unsigned(31 downto 0)   :=add_bsp_info+X"0000_0018";
constant add_bsp_info_usertxsel    : unsigned(31 downto 0)   :=add_bsp_info+X"0000_001C";

constant add_bsp_hkeeping_gpi      : unsigned(31 downto 0)   :=add_bsp_hkeeping+X"0000_0000";
constant add_bsp_hkeeping_gpo      : unsigned(31 downto 0)   :=add_bsp_hkeeping+X"0000_0004";
constant add_bsp_hkeeping_tempctrl  : unsigned(31 downto 0)  :=add_bsp_hkeeping+X"0000_0008";
constant add_bsp_hkeeping_tempstat : unsigned(31 downto 0)   :=add_bsp_hkeeping+X"0000_0008";
constant add_bsp_hkeeping_swlimit  : unsigned(31 downto 0)   :=add_bsp_hkeeping+X"0000_000c";
constant add_bsp_hkeeping_hwlimit  : unsigned(31 downto 0)   :=add_bsp_hkeeping+X"0000_0010";
constant add_bsp_hkeeping_chipid_high : unsigned(31 downto 0):=add_bsp_hkeeping+X"0000_0014";
constant add_bsp_hkeeping_chipid_low  : unsigned(31 downto 0):=add_bsp_hkeeping+X"0000_0018";
constant add_bsp_hkeeping_spare_in    : unsigned(31 downto 0):=add_bsp_hkeeping+X"0000_001C";
                                                                                   
constant add_A10_meas_vcc             : unsigned(31 downto 0):= add_bsp_hkeeping + x"0000_0020";
constant add_A10_meas_1v8_all         : unsigned(31 downto 0):= add_bsp_hkeeping + x"0000_0024";
constant add_A10_meas_VCCR            : unsigned(31 downto 0):= add_bsp_hkeeping + x"0000_0028";
constant add_A10_meas_VCCT            : unsigned(31 downto 0):= add_bsp_hkeeping + x"0000_002C";
constant add_A10_meas_VCCPT           : unsigned(31 downto 0):= add_bsp_hkeeping + x"0000_0030";
constant add_A10_meas_1v8             : unsigned(31 downto 0):= add_bsp_hkeeping + x"0000_0034";
constant add_A10_meas_3v3             : unsigned(31 downto 0):= add_bsp_hkeeping + x"0000_0038";
constant add_A10_meas_2v5             : unsigned(31 downto 0):= add_bsp_hkeeping + x"0000_003C";
constant add_A10_meas_12v             : unsigned(31 downto 0):= add_bsp_hkeeping + x"0000_0040";
constant add_A10_meas_12v_ATX         : unsigned(31 downto 0):= add_bsp_hkeeping + x"0000_0044";
constant add_A10_meas_ext_ADC         : unsigned(31 downto 0):= add_bsp_hkeeping + x"0000_0048";
constant add_A10_meas_constant        : unsigned(31 downto 0):= add_bsp_hkeeping + x"0000_0058";

constant add_bsp_rsu_reconf_cond      : unsigned(31 downto 0)   :=add_bsp_rsu+X"0000_0000";
constant add_bsp_rsu_watchdog_timeout : unsigned(31 downto 0)   :=add_bsp_rsu+X"0000_0008";
constant add_bsp_rsu_watchdog_enable  : unsigned(31 downto 0)   :=add_bsp_rsu+X"0000_000C";
constant add_bsp_rsu_pagesel          : unsigned(31 downto 0)   :=add_bsp_rsu+X"0000_0010";
constant add_bsp_rsu_conf_mode        : unsigned(31 downto 0)   :=add_bsp_rsu+X"0000_0014";
constant add_bsp_rsu_ctrl             : unsigned(31 downto 0)   :=add_bsp_rsu+X"0000_0018";

constant add_bsp_i2c_tsensor      : unsigned(31 downto 0):=add_bsp_i2c+X"0000_0000";
constant add_bsp_i2c_sfp1         : unsigned(31 downto 0):=add_bsp_i2c+X"0000_0200";
constant add_bsp_i2c_minipods     : unsigned(31 downto 0):=add_bsp_i2c+X"0000_0300";
constant add_bsp_i2c_si5344       : unsigned(31 downto 0):=add_bsp_i2c+X"0000_0400";
constant add_bsp_i2c_si5345_1     : unsigned(31 downto 0):=add_bsp_i2c+X"0000_0500";
constant add_bsp_i2c_si5345_2     : unsigned(31 downto 0):=add_bsp_i2c+X"0000_0600";
constant add_bsp_i2c_sfp2         : unsigned(31 downto 0):=add_bsp_i2c+X"0000_0700";
constant add_bsp_i2c_eeprom       : unsigned(31 downto 0):=add_bsp_i2c+X"0000_0800";

-------------------------------------------------------------------------------
-- User logic
-------------------------------------------------------------------------------
constant add_userlogic_info     	: unsigned(31 downto 0):=add_userlogic+X"0000_0000"; 
constant add_userlogic_ctrl     	: unsigned(31 downto 0):=add_userlogic+X"0008_0000"; 

constant add_user_logic_dirty_idcode  : unsigned(31 downto 0)  :=add_userlogic_info+X"0000_0000";
constant add_user_logic_shorthash    : unsigned(31 downto 0)   :=add_userlogic_info+X"0000_0004";
constant add_user_logic_builddate    : unsigned(31 downto 0)   :=add_userlogic_info+X"0000_0008";
constant add_user_logic_buildtime    : unsigned(31 downto 0)   :=add_userlogic_info+X"0000_000C";

constant add_user_logic_reset     : unsigned(31 downto 0) := add_userlogic_ctrl+X"0000_0000";
constant add_user_logic_eventsize : unsigned(31 downto 0) := add_userlogic_ctrl+X"0000_0004";
constant add_user_logic_rand_eventsize_toggle : unsigned(31 downto 0) := add_userlogic_ctrl+X"0000_0008";

--------------------------------------------------------------------------------
-- BAR 0 REGISTERs (DMA)
--------------------------------------------------------------------------------
constant add_pcie_dma_ctrl         : unsigned(31 downto 0)   :=X"0000_0200";
-- DESCRIPTOR SW -> CRU
constant add_pcie_dma_desc_h       : unsigned(31 downto 0)   :=X"0000_0204";
constant add_pcie_dma_desc_l       : unsigned(31 downto 0)   :=X"0000_0208";
constant add_pcie_dma_desc_sz      : unsigned(31 downto 0)   :=X"0000_020c";
--
constant add_pcie_dma_rst          : unsigned(31 downto 0)   :=X"0000_0400";
constant add_pcie_dma_ep_id        : unsigned(31 downto 0)   :=X"0000_0500";
-- DDG
constant add_pcie_dma_ddg_cfg0     : unsigned(31 downto 0)   :=X"0000_0600";
constant add_pcie_dma_ddg_cfg1     : unsigned(31 downto 0)   :=X"0000_0604";
constant add_pcie_dma_ddg_cfg2     : unsigned(31 downto 0)   :=X"0000_0608";
constant add_pcie_dma_ddg_cfg3     : unsigned(31 downto 0)   :=X"0000_060c";
--
constant add_pcie_dma_data_sel     : unsigned(31 downto 0)   :=X"0000_0700";
-- SUPERPAGE REPORT CRU > SW
constant add_pcie_dma_spg0_ack     : unsigned(31 downto 0)   :=X"0000_0800";
--
constant add_pcie_dma_dbg          : unsigned(31 downto 0)   :=X"0000_0c00";


-------------------------------------------------------------------------------
-- eventual component declaration for external user (different package for internal? : to be discussed)
-------------------------------------------------------------------------------
component bsp is

  generic (
    TOPVERSION : work.verinfopkg.verinforec;
    g_NUM_GBT_LINKS     : integer := 4
    );

  port (
    ---------------------------------------------------------------------------
    MMS_CLK     : in  std_logic;
    MMS_RESET   : in  std_logic;
    MMS_WAITREQ : out std_logic;
    MMS_ADDR    : in  std_logic_vector(23 downto 0);
    MMS_WR      : in  std_logic;
    MMS_WRDATA  : in  std_logic_vector(31 downto 0);
    MMS_RD      : in  std_logic;
    MMS_RDVAL   : out std_logic;
    MMS_RDDATA  : out std_logic_vector(31 downto 0);
    ---------------------------------------------------------------------------
    TEMPALARMS  : out std_logic_vector(3 downto 0);
    ---------------------------------------------------------------------------
    DEBUGCTRL   : out std_logic_vector(31 downto 0);
    ---------------------------------------------------------------------------
    USERCTRL    : out std_logic_vector(31 downto 0);
    USERTXSEL   : out std_logic_vector(4*g_NUM_GBT_LINKS-1 downto 0);
    ---------------------------------------------------------------------------
    spare_in       : in  std_logic_vector(31 downto 0);  -- general purpose in
    ---------------------------------------------------------------------------
    GPI         : in  std_logic_vector(31 downto 0) := (others => '0');  -- general purpose in
    GPO         : out std_logic_vector(31 downto 0);  -- general purpose out
    ---------------------------------------------------------------------------
    SCL_I       : in  std_logic_vector(15 downto 0)  := (others => '1');
    SCL_T       : out std_logic_vector(15 downto 0);
    SCL_O       : out std_logic_vector(15 downto 0);
    SDA_I       : in  std_logic_vector(15 downto 0)  := (others => '1');
    SDA_T       : out std_logic_vector(15 downto 0);
    SDA_O       : out std_logic_vector(15 downto 0);
   ---------------------------------------------------------------------------
    L2418_SPI_nCS : out std_logic;  -- LTC 2418 Interface (24 bits ADC)
    L2418_SPI_SCK : out std_logic;
    L2418_SPI_SDI : out std_logic;
    L2418_SPI_SDO : in  std_logic;
    
    L2498_SPI_nCS : out std_logic;  -- LTC 2498 Interface (24 bits ADC)
    L2498_SPI_SCK : out std_logic;
    L2498_SPI_SDI : out std_logic;
    L2498_SPI_SDO : in  std_logic
   );
end component bsp;

component syncgen is

  generic (
    COUNTTIME : integer := 50_000_000   -- in number of CLK cycles
    );

  port (
    CLK     : in  std_logic;
    RST     : in  std_logic;
    SYNCOUT : out std_logic_vector(1 downto 0)
    );

end component syncgen;

component freqmeas is

  generic (
    REFCLKFREQ : integer := 100_000_000;  -- in Hz
    DWIDTH     : integer := 32
    );

  port (
    CLK     : in  std_logic;
    RST     : in  std_logic;
    --
    SYNCIN  : in  std_logic_vector(1 downto 0) := "00";  -- cascade in, used if REFCLKFREQ = 0
    SYNCOUT : out std_logic_vector(1 downto 0);          -- cascade out
    --
    I       : in  std_logic;            -- input, treated as a clock signal
    FREQ    : out std_logic_vector(DWIDTH - 1 downto 0)
    );

end component freqmeas;

component ddg is
  generic (
    g_NUM_GBT_LINKS     : integer := 4
    );
  port(
    --------------------------------------------------------------------------------
    -- AVALON interface
    --------------------------------------------------------------------------------
    MMS_CLK        : in  std_logic;
    MMS_RESET      : in  std_logic;
    MMS_WAITREQ    : out std_logic := '0';
    MMS_ADDR       : in  std_logic_vector(19 downto 0);
    MMS_WR         : in  std_logic;
    MMS_WRDATA     : in  std_logic_vector(31 downto 0);
    MMS_RD         : in  std_logic;
    MMS_RDVAL      : out std_logic;
    MMS_RDDATA     : out std_logic_vector(31 downto 0);
    --------------------------------------------------------------------------------
    -- TRG interface
    --------------------------------------------------------------------------------
    TTC_RXCLK      : in  std_logic;
    TTC_RXRST      : in  std_logic;
    TTC_RXVALID    : in  std_logic;
    TTC_RXD        : in  std_logic_vector(199 downto 0);
    --------------------------------------------------------------------------------
    -- GBT Downlink (CRU -> FE)
    --------------------------------------------------------------------------------
    gbt_tx_ready_i : in  std_logic_vector(g_NUM_GBT_LINKS-1 downto 0);
    gbt_tx_bus_o   : out t_cru_gbt
    );
end component ddg;

component RO_CTRL is
generic(
	FIFOLOGSIZE: natural:=3; -- should permit to store up several information, 
	g_max_dur : natural :=3576*6; -- 89.4µs
	g_CHECKER_NUM : natural:=8 -- allocate the number of checker that can work in parallel
	);
port (
    ---------------------------------------------------------------------------
    MMS_CLK     : in  std_logic;
    MMS_RESET   : in  std_logic;
    MMS_WAITREQ : out std_logic := '0';
    MMS_ADDR    : in  std_logic_vector(23 downto 0);
    MMS_WR      : in  std_logic;
    MMS_WRDATA  : in  std_logic_vector(31 downto 0);
    MMS_RD      : in  std_logic;
    MMS_RDVAL   : out std_logic;
    MMS_RDDATA  : out std_logic_vector(31 downto 0);
	--
    RUN_ENABLE  : in  std_logic;
    ---------------------------------------------------------------------------
    CRUID    : in  std_logic_vector(11 downto 0); -- only 10 bit meaningful
    STAT     : in  std_logic_vector(1 downto 0);
    ---------------------------------------------------------------------------
    TTCRXCLK    : in  std_logic;
    TTCRXREADY  : in  std_logic;
    TTCRXVALID  : in  std_logic;
    TTCRXD      : in  std_logic_vector(199 downto 0);
	
	CURRENT_HBID_i : in std_logic_vector(31 downto 0);
	CURRENT_BCID_i : in std_logic_vector(11 downto 0);
    ---------------------------------------------------------------------------
    TTCTX_READY     : in  std_logic; -- data taken on last cycle high
    TTCTXD          : out  std_logic_vector(55 downto 0); -- put idle pattern or message to transmit

    ---------------------------------------------------------------------------
	BUSY       : in std_logic; -- input for TRD only
    ---------------------------------------------------------------------------
	NEW_RESULT_o     : out std_logic; -- new HBF check result (each time at one is a new result, can be almost consecutive)
	ACK_o            : out std_logic; -- ACK or NACK HBF
	HBID_CHECKED_o   : out std_logic_vector(31 downto 0); -- HBID checked
    ---------------------------------------------------------------------------
	-- packet info input
	UPD        : in std_logic_vector(0 to 1); -- new packet information (pulse)
	HBID       : in Array32bit(0 to 1);
	LINKID     : in Array8bit(0 to 1);
	page       : in Array16bit(0 to 1);
	AFULL      : in std_logic_vector(0 to 1);
	STOP       : in std_logic_vector(0 to 1);

	-- hbf acknowledge message insertion port
    SOP_DW0    : out  std_logic;
    EOP_DW0    : out  std_logic;
    VAL_DW0    : out  std_logic;
    DOUT_DW0   : out  std_logic_vector(127 downto 0);

	-- hbf decision message insertion port
    SOP_DW1    : out  std_logic;
    EOP_DW1    : out  std_logic;
    VAL_DW1    : out  std_logic;
    DOUT_DW1   : out  std_logic_vector(127 downto 0)
	);
end component RO_CTRL;

component datapath_wrapper is

  generic (
    g_SMALLFIFO_LOGSIZE    : integer := 10;  -- in 256 bit words
    g_BIGFIFO_LOGSIZE      : integer := 13;  -- in 256 bit words
    g_NUM_GBT_LINKS        : integer := 1;
    g_DWRAPPER_NUM         : integer range 0 to 1 := 0;
    g_LINKID_RO_PROT       : integer; -- LINK ID to use for the RO protocol message (13 for ack, 14 for decision)
    g_NUM_USERLOGIC_LINKS  : integer := 1
    );

  port (
    ---------------------------------------------------------------------------
    MMS_CLK     : in  std_logic;
    MMS_RESET   : in  std_logic;
    MMS_WAITREQ : out std_logic := '0';
    MMS_ADDR    : in  std_logic_vector(23 downto 0);
    MMS_WR      : in  std_logic;
    MMS_WRDATA  : in  std_logic_vector(31 downto 0);
    MMS_RD      : in  std_logic;
    MMS_RDVAL   : out std_logic;
    MMS_RDDATA  : out std_logic_vector(31 downto 0);
	--
    RUN_ENABLE  : in  std_logic;
	RUN_OVERRIDE : in  std_logic;
    CRUID       : in  std_logic_vector(11 downto 0);
    ---------------------------------------------------------------------------
    TTCRXCLK    : in  std_logic;
    TTCRXREADY  : in  std_logic;
    TTCRXVALID  : in  std_logic;
    TTCRXD      : in  std_logic_vector(199 downto 0);
    ---------------------------------------------------------------------------
    GBUS        : in  t_cru_gbt_array(g_NUM_GBT_LINKS - 1 downto 0);
    --
    USERCLK     : in  std_logic;
    USERVAL     : in  std_logic;
    USERSOP     : in  std_logic;
    USEREOP     : in  std_logic;
    USERD       : in  std_logic_vector(255 downto 0);
    AFULL_UL    : out std_logic;
    ---------------------------------------------------------------------------
	-- RO protocol port (for ack or decision message). clocked by TTCRXCLK 
    ROPROT_VAL     : in  std_logic;
    ROPROT_SOP     : in  std_logic;
    ROPROT_EOP     : in  std_logic;
    ROPROT_D       : in  std_logic_vector(127 downto 0);
    ---------------------------------------------------------------------------
    -- port on PCI clock
	PACK_UPD    : out std_logic; -- new packet information (pulse)
	PACK_HBID   : out std_logic_vector(31 downto 0);
	PACK_LINKID : out std_logic_vector(7 downto 0);
	PACK_page   : out std_logic_vector(15 downto 0);
	PACK_STOP   : out std_logic;
	PACK_AF     : out std_logic;

	CURRENT_HBID_o : out std_logic_vector(31 downto 0);
	CURRENT_BCID_o : out std_logic_vector(11 downto 0);
    ---------------------------------------------------------------------------
    PCIe_CLK        : in  std_logic;        -- clock for this FIFO interface
    PCIe_RST        : in  std_logic;
    PCIe_RD         : in  std_logic;        -- PCIe RD
    PCIe_SOP        : out std_logic;        -- start of packet
    PCIe_EOP        : out std_logic;        -- end of packet
    PCIe_VAL        : out std_logic;        -- data valid
    PCIe_EMPTy      : out std_logic;        -- data valid
    PCIe_D          : out std_logic_vector(255 downto 0)  -- actual data
   ---------------------------------------------------------------------------
    );

end component datapath_wrapper;

component cdcor is

  generic (
    N   : integer := 2;                 -- number of inputs
    LEN : integer := 3
    );

  port (
    I : in  std_logic_vector(N - 1 downto 0);  -- rst inputs
    C : in  std_logic;                         -- output clock domain's clk
    O : out std_logic
    );

end component cdcor;

component cdcnop is

  generic (
    N   : integer := 1;                 -- number of inputs
    LEN : integer := 2
    );

  port (
    I : in  std_logic_vector(N - 1 downto 0);  -- rst inputs
    C : in  std_logic;                         -- output clock domain's clk
    O : out std_logic_vector(N - 1 downto 0)
    );

end component cdcnop;

component statcnt is

  generic (
    W1    : integer := 0;  -- width of partial counter (in CLK1 clock domain)
    PRESC : boolean := true;            -- run in prescaler mode
    W2    : integer := 32;    -- width of final counter (in CLK2 clock domain)
    W3    : integer := 7  -- bit position initiating sync into to CLK3 clock domain
    );

  port (
    CLK1  : in  std_logic := '0';
    RST1  : in  std_logic := '0';
    I     : in  std_logic;
    --
    CLK2  : in  std_logic;
    RST2  : in  std_logic := '0';
    Q2    : out std_logic_vector(W2 - 1 downto 0);
    --
    CLK3  : in  std_logic := '0';
    SYNC3 : in  std_logic := '0';       -- async, can be in any clock domain
    Q3    : out std_logic_vector(W2 - 1 downto 0)
    );

end component statcnt;

component gen_counter is
generic (WIDTH    : integer := 0  -- width of counter
		);
port (
	CLK  : in  std_logic;
	RST  : in  std_logic;
	I     : in  std_logic;
	COUNT    : out std_logic_vector(WIDTH - 1 downto 0)
);

end component gen_counter;

component cdcbus is

  generic (
    FREEZE : boolean := true;           -- capture data in src clk domain?
    W      : integer := 32;             -- bus width
    N      : integer := 6               -- delay for Valid (in CLKO clocks)
    );

  port (
    CLKI : in  std_logic;               -- input clock domain's clk
    E    : in  std_logic;
    I    : in  std_logic_vector(W - 1 downto 0);
    RSTVAL : in std_logic_vector(W-1 downto 0):=(others=>'0');
    --
    CLKO : in  std_logic;               -- output clock domain's clk
    RSTO : in  std_logic := '0';
    V    : out std_logic;
    O    : out std_logic_vector(W - 1 downto 0)
    );

end component cdcbus;

component pulse_gen is
  port (
    --------------------------------------------------------------------------------
    -- clock
    --------------------------------------------------------------------------------
    clk_i    : in  std_logic;
    clk_en_i : in  std_logic := '1';
    --------------------------------------------------------------------------------
    -- signal
    --------------------------------------------------------------------------------
    i        : in  std_logic;
    p_o      : out std_logic
    );

end component;

component cdcreduce is

  generic (
    N   : integer   := 2;               -- number of inputs
    LEN : integer   := 3;
    A   : std_logic := '1'              -- active value (1 => active high)
    );

  port (
    I : in  std_logic_vector(N - 1 downto 0);  -- rst inputs
    C : in  std_logic;                         -- output clock domain's clk
    O : out std_logic
    );

end component cdcreduce;
 
component avalon_mm_bus_arbitrer is
   generic ( NM     : natural := 1;              -- no of masters
             AWIDTH : natural :=32;
             NHI    : natural := 3  );           -- no of address bits to decode
   port    (
     --------------------------------------------------------------------------
     CLK       : in  std_logic;
     RST       : in  std_logic;
     --------------------------------------------------------------------------
     M_WAITREQ : out std_logic_vector(NM - 1 downto 0);
     M_ADDR    : in  Array32bit(0 to NM - 1);
     M_WR      : in  std_logic_vector(NM - 1 downto 0);
     M_WRDATA  : in  Array32bit(0 to NM - 1 );
     M_RD      : in  std_logic_vector(NM - 1 downto 0);
     M_RDDATA  : out Array32bit(0 to NM - 1);
     M_RDVAL   : out std_logic_vector(NM - 1 downto 0);
     --------------------------------------------------------------------------
     S_WAITREQ : in  std_logic_vector(2**NHI - 1 downto 0);  -- := (others => '0');
     S_ADDR    : out Array32bit(0 to 2**NHI - 1);
     S_WR      : out std_logic_vector(2**NHI - 1 downto 0);
     S_WRDATA  : out Array32bit(0 to 2**NHI - 1);
     S_RD      : out std_logic_vector(2**NHI - 1 downto 0);
     S_RDDATA  : in  Array32bit(0 to 2**NHI - 1);            -- := (others => '1'); 
     S_RDVAL   : in  std_logic_vector(2**NHI - 1 downto 0) );-- := (others => '1'));
 end component avalon_mm_bus_arbitrer;
 
component avalon_mm_slave is
   generic ( MODE_LG  : positive := 3;
             AWIDTH   : integer := 8;
             MODE     : Array4bit(63 downto 0)  := ( others => x"4");
             RSTVAL   : Array32bit(63 downto 0) := (others => (others =>'0')) );
   port    ( CLK      : in  std_logic                     := '0';
             RESET    : in  std_logic                     := '0';
             WAITREQ  : out std_logic;
             ADDR     : in  std_logic_vector(AWIDTH - 1 downto 0)  := (others => '0');
             WR       : in  std_logic                     := '0';
             WRDATA   : in  std_logic_vector(31 downto 0) := (others => '0');
             RD       : in  std_logic                     := '0';
             RDDATA   : out std_logic_vector(31 downto 0);
             RDVAL    : out std_logic;
             --       
             ALTCLK   : in  std_logic               := '0';
             --     
             USERWR   : out std_logic_vector((MODE_LG - 1 ) downto 0);
             USERRD   : out std_logic_vector((MODE_LG - 1 ) downto 0);
             --
             din      : in  Array32bit( (MODE_LG - 1) downto 0) := (others => (others =>'0'));
             qout     : out Array32bit( (MODE_LG - 1) downto 0) );
 end component avalon_mm_slave;

-- eventual usefull procedure/functions
  function setAdd (mult : in natural; offset : unsigned) return unsigned;
  function f_order_in_gbtw (src : in std_logic_vector ) return std_logic_vector;
  function f_order_out_gbtw (src : in std_logic_vector ) return std_logic_vector;
  function f_order_in_bus_gbtw (src : in t_cru_gbt_array ) return t_cru_gbt_array;
  function f_order_out_bus_gbtw (src : in t_cru_gbt_array ) return t_cru_gbt_array;

end pack_cru_core;

package body pack_cru_core is

function setAdd(mult : in natural; offset : unsigned) return unsigned is
  variable tmp : unsigned(31 downto 0);
begin
  tmp:=to_unsigned(mult*to_integer(offset),32);
  return tmp;
end setAdd;

  function f_order_in_gbtw(src : in std_logic_vector ) return std_logic_vector is
    variable dst : std_logic_vector(src'range);
  begin
    for i in src'range loop
      dst(i) := src(swap_table(i));
    end loop;
    return dst;
  end function f_order_in_gbtw;

  function f_order_out_gbtw (src : in std_logic_vector ) return std_logic_vector is
    variable dst : std_logic_vector(src'range);
  begin
    for i in src'range loop
      dst(swap_table(i)) := src(i);
    end loop;
    return dst;
  end function f_order_out_gbtw;

  function f_order_in_bus_gbtw (src : in t_cru_gbt_array ) return t_cru_gbt_array is
    variable dst : t_cru_gbt_array(src'range);
  begin
    for i in src'range loop
      dst(i) := src(swap_table(i));
    end loop;
    return dst;
  end function f_order_in_bus_gbtw;

  function f_order_out_bus_gbtw (src : in t_cru_gbt_array ) return t_cru_gbt_array is
    variable dst : t_cru_gbt_array(src'range);
  begin
    for i in src'range loop
      dst(swap_table(i)) := src(i);
    end loop;
    return dst;
  end function f_order_out_bus_gbtw;

end pack_cru_core;
