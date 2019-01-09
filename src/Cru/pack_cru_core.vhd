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
type Array4bit is array(natural range <>) of std_logic_vector(3 downto 0);
type Array8bit is array(natural range <>) of std_logic_vector(7 downto 0);
type Array16bit is array(natural range <>) of std_logic_vector(15 downto 0);
type Array32bit is array(natural range <>) of std_logic_vector(31 downto 0);
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
----                       Constant definition                             ----
-------------------------------------------------------------------------------
 constant c_GBT_FRAME   : integer := 0;  --! GBT-FRAME encoding (constant definition)
 constant c_WIDE_BUS    : integer := 1;  --! WideBus encoding (constant definition)
 constant c_GBT_DYNAMIC : integer := 2;  --! GBT-FRAME or WideBus encoding can be changed dynamically (constant definition)
 
-- g_GBT_user_type definition : each word correspond to the summ of the 2 4 bit word of the ASCII character

 constant c_GBT         : std_logic_vector(11 downto 0) := x"B69"; -- x"47_42_54"
 constant c_TRD         : std_logic_vector(11 downto 0) := x"978"; -- x"54_52_44"


-------------------------------------------------------------------------------
-- GBT address tables
-------------------------------------------------------------------------------
-- GBT wrapper pages
constant add_gbt_wrapper0			: unsigned(31 downto 0):=X"0040_0000";
constant add_gbt_wrapper1			: unsigned(31 downto 0):=X"0050_0000";

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
constant add_gbt_link_dbgdata0		: unsigned(31 downto 0):=X"0000_0010"; -- RO
constant add_gbt_link_dbgdata1		: unsigned(31 downto 0):=X"0000_0014"; -- RO
constant add_gbt_link_dbgdata2		: unsigned(31 downto 0):=X"0000_0018"; -- RO
constant add_gbt_link_rx_err_cnt	: unsigned(31 downto 0):=X"0000_001C"; -- RO
constant add_gbt_link_mask_hi       : unsigned(31 downto 0):=x"0000_0020"; -- W with loopBack
constant add_gbt_link_mask_med	    : unsigned(31 downto 0):=x"0000_0024"; -- W with loopBack
constant add_gbt_link_mask_lo	    : unsigned(31 downto 0):=x"0000_0028"; -- W with loopBack
constant add_gbt_link_header_errcnt_offset : unsigned(31 downto 0) := x"0000_002C";
constant add_gbt_link_data_errcnt_offset   : unsigned(31 downto 0) := x"0000_0030";
constant add_gbt_link_tx_ctrl_offset : unsigned(31 downto 0) := x"0000_0034";
constant add_gbt_link_source_sel	 : unsigned(31 downto 0):=X"0000_0038"; -- W with loopBack
constant add_gbt_link_FEC_monitoring : unsigned(31 downto 0):=X"0000_003C";
constant add_gbt_link_rx_ctrl_offset : unsigned(31 downto 0) := x"0000_0040";

-------------------------------------------------------------------------------
-- GBTSC address tables
-------------------------------------------------------------------------------
-- GBTSCA wrapper pages
constant add_gbt_sc	        		: unsigned(31 downto 0):=X"00f0_0000";
-- WR
constant add_gbt_sca_wr_data        		: unsigned(31 downto 0):=X"0000_0000";
constant add_gbt_sca_wr_cmd        		: unsigned(31 downto 0):=X"0000_0004";
constant add_gbt_sca_wr_ctr        		: unsigned(31 downto 0):=X"0000_0008";
-- RD
constant add_gbt_sca_rd_data        		: unsigned(31 downto 0):=X"0000_0010";
constant add_gbt_sca_rd_cmd        		: unsigned(31 downto 0):=X"0000_0014";
constant add_gbt_sca_rd_ctr        		: unsigned(31 downto 0):=X"0000_0018";
constant add_gbt_sca_rd_mon        		: unsigned(31 downto 0):=X"0000_001c";
-- SWT
constant add_gbt_swt_wr_l                       : unsigned(31 downto 0):=X"0000_0040";
constant add_gbt_swt_wr_m                       : unsigned(31 downto 0):=X"0000_0044";
constant add_gbt_swt_wr_h                       : unsigned(31 downto 0):=X"0000_0048";

constant add_gbt_swt_cmd                        : unsigned(31 downto 0):=X"0000_004c";

constant add_gbt_swt_rd_l                       : unsigned(31 downto 0):=X"0000_0050";
constant add_gbt_swt_rd_m                       : unsigned(31 downto 0):=X"0000_0054";
constant add_gbt_swt_rd_h                       : unsigned(31 downto 0):=X"0000_0058";

constant add_gbt_swt_mon                        : unsigned(31 downto 0):=X"0000_005c";
constant add_gbt_swt_word_mon                   : unsigned(31 downto 0):=X"0000_0060";

constant add_gbt_sc_link                        : unsigned(31 downto 0):=X"0000_0078";
constant add_gbt_sc_rst                         : unsigned(31 downto 0):=X"0000_007c";

-------------------------------------------------------------------------------
-- TTC PON address tables
-------------------------------------------------------------------------------
constant add_ttc_pon			: unsigned(31 downto 0):=X"0020_0000";

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

constant add_ttc_clkgen_ttc240freq	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_0000";
constant add_ttc_clkgen_lcl240freq	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_0004";
constant add_ttc_clkgen_ref240freq	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_0008";
constant add_ttc_clkgen_clknotokcnt  	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_000C";
constant add_ttc_clkgen_clkctrl 	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_0010";
constant add_ttc_clkgen_clkstat 	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_0014";
constant add_ttc_clkgen_pllctrlonu 	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_0018";
constant add_ttc_clkgen_pllstatonu 	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_001C";
constant add_ttc_clkgen_phasecnt 	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_0020";
constant add_ttc_clkgen_phasestat 	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_0024";

constant add_ttc_clkgen_onufpll 	: unsigned(31 downto 0):=add_ttc_clkgen+X"0000_8000";

-- ONU zone
constant add_ttc_onu_ctrl               : unsigned(31 downto 0):=add_ttc_onu+X"0000_0000";

constant add_pon_verinfo	        : unsigned(31 downto 0):=add_ttc_onu+X"0000_0000";
constant add_pon_wrapper_reg	        : unsigned(31 downto 0):=add_ttc_onu+X"0000_2000";
constant add_pon_wrapper_pll	        : unsigned(31 downto 0):=add_ttc_onu+X"0000_4000";
constant add_pon_wrapper_tx		: unsigned(31 downto 0):=add_ttc_onu+X"0000_6000";
constant add_onu_user_logic	        : unsigned(31 downto 0):=add_ttc_onu+X"0000_A000";
constant add_onu_user_refgen	        : unsigned(31 downto 0):=add_ttc_onu+X"0000_C000";
constant add_onu_freq_meas		: unsigned(31 downto 0):=add_ttc_onu+X"0000_E000";

constant add_refgen0_offset             : unsigned(31 downto 0):=X"0000_0000";
constant add_refgen1_offset             : unsigned(31 downto 0):=X"0000_0004";
constant add_onu_refgen_cnt		: unsigned(31 downto 0):=add_onu_user_refgen+X"0000_0018";

constant add_onu_rxref_freq		: unsigned(31 downto 0):=add_onu_freq_meas+X"0000_0000";
constant add_onu_rxuser_freq	        : unsigned(31 downto 0):=add_onu_freq_meas+X"0000_0004";
constant add_onu_refout_freq	        : unsigned(31 downto 0):=add_onu_freq_meas+X"0000_0008";
constant add_onu_tx0ref_freq	        : unsigned(31 downto 0):=add_onu_freq_meas+X"0000_000C";
constant add_onu_tx1ref_freq	        : unsigned(31 downto 0):=add_onu_freq_meas+X"0000_0010";
constant add_onu_tx2ref_freq	        : unsigned(31 downto 0):=add_onu_freq_meas+X"0000_0014";
constant add_onu_tx3ref_freq	        : unsigned(31 downto 0):=add_onu_freq_meas+X"0000_0018";
constant add_onu_txuser_freq	        : unsigned(31 downto 0):=add_onu_freq_meas+X"0000_001C";


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
constant add_ctp_emu_hb_keep	        : unsigned(31 downto 0):=add_ctp_emu_core+X"0000_000C";

constant add_ctp_emu_runmode	        : unsigned(31 downto 0):=add_ctp_emu_core+X"0000_0010";
constant add_ctp_emu_physdiv	        : unsigned(31 downto 0):=add_ctp_emu_core+X"0000_0014";
constant add_ctp_emu_hcdiv	            : unsigned(31 downto 0):=add_ctp_emu_core+X"0000_0018";
constant add_ctp_emu_userbits	        : unsigned(31 downto 0):=add_ctp_emu_core+X"0000_001C";
constant add_ctp_emu_caldiv	            : unsigned(31 downto 0):=add_ctp_emu_core+X"0000_0020";
constant add_ctp_emu_fbct	            : unsigned(31 downto 0):=add_ctp_emu_core+X"0000_0024";


-------------------------------------------------------------------------------
-- DDG address tables
-------------------------------------------------------------------------------
constant add_ddg		  : unsigned(31 downto 0):=X"00D0_0000";
constant add_ddg_ctrl	  : unsigned(31 downto 0):=add_ddg+X"0000_0000";
constant add_ddg_ctrl2	  : unsigned(31 downto 0):=add_ddg+X"0000_0004";
constant add_ddg_pkt_cnt  :  unsigned(31 downto 0):=add_ddg+X"0000_0008";

-------------------------------------------------------------------------------
-- datapath wrapper address tables
-------------------------------------------------------------------------------
constant add_base_datapathwrapper0  : unsigned(31 downto 0):=X"0060_0000";
constant add_base_datapathwrapper1  : unsigned(31 downto 0):=X"0070_0000";

constant add_dwrapper_gregs		  : unsigned(31 downto 0):=X"0000_0000";
constant add_datapathlink_offset  : unsigned(31 downto 0):=X"0004_0000"; -- add link offset to access it
constant add_mingler_offset		  : unsigned(31 downto 0):=X"0008_0000";
constant add_flowctrl_offset	  : unsigned(31 downto 0):=X"000C_0000";

-- datapath link page access
constant add_datalink_offset		: unsigned(31 downto 0):=X"0000_1000"; -- to multiply by 0 to 23

-- datapath wrapper global registers
constant add_dwrapper_enreg 	   : unsigned(31 downto 0):=X"0000_0000"; -- WO
constant add_dwrapper_muxctrl	   : unsigned(31 downto 0):=X"0000_0004"; -- WO (cdc)

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
--datapath link registers
constant add_datalink_ctrl	      : unsigned(31 downto 0):=X"0000_0000";

constant add_datalink_rej_pkt     : unsigned(31 downto 0):=X"0000_000C";
constant add_datalink_acc_pkt     : unsigned(31 downto 0):=X"0000_0010";
constant add_datalink_forced_pkt  : unsigned(31 downto 0):=X"0000_0014";

--mingler registers
constant add_mingler_linkid		    : unsigned(31 downto 0):=X"0000_0000"; -- WO cdc
constant add_mingler_linkid_mod	    : unsigned(31 downto 0):=X"0000_000C"; -- RO cdc

-- flow control registers
constant add_flowctrl_ctrlreg		: unsigned(31 downto 0):=X"0000_0000";
constant add_flowctrl_pkt_rej		: unsigned(31 downto 0):=X"0000_0004";
constant add_flowctrl_pkt_tot		: unsigned(31 downto 0):=X"0000_0008";

-------------------------------------------------------------------------------
-- BSP address tables
-------------------------------------------------------------------------------
constant add_bsp		          : unsigned(31 downto 0):=X"0000_0000";
constant add_bsp_info	          : unsigned(31 downto 0):=add_bsp+X"0000_0000";
constant add_bsp_hkeeping	      : unsigned(31 downto 0):=add_bsp+X"0001_0000";
constant add_bsp_i2c	          : unsigned(31 downto 0):=add_bsp+X"0003_0000";

constant add_bsp_info_dirtystatus  : unsigned(31 downto 0)   :=add_bsp_info+X"0000_0000";
constant add_bsp_info_shorthash    : unsigned(31 downto 0)   :=add_bsp_info+X"0000_0004";
constant add_bsp_info_builddate    : unsigned(31 downto 0)   :=add_bsp_info+X"0000_0008";
constant add_bsp_info_buildtime    : unsigned(31 downto 0)   :=add_bsp_info+X"0000_000C";
constant add_bsp_info_debug        : unsigned(31 downto 0)   :=add_bsp_info+X"0000_0010";
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

constant add_bsp_i2c_tsensor      : unsigned(31 downto 0):=add_bsp_i2c+X"0000_0000";
constant add_bsp_i2c_cpcie        : unsigned(31 downto 0):=add_bsp_i2c+X"0000_0200";
constant add_bsp_i2c_sfp1         : unsigned(31 downto 0):=add_bsp_i2c+X"0000_0400";
constant add_bsp_i2c_minipods     : unsigned(31 downto 0):=add_bsp_i2c+X"0000_0600";
constant add_bsp_i2c_si5344       : unsigned(31 downto 0):=add_bsp_i2c+X"0000_0800";
constant add_bsp_i2c_si5345_1     : unsigned(31 downto 0):=add_bsp_i2c+X"0000_0a00";
constant add_bsp_i2c_si5345_2     : unsigned(31 downto 0):=add_bsp_i2c+X"0000_0c00";
constant add_bsp_i2c_sfp2         : unsigned(31 downto 0):=add_bsp_i2c+X"0000_0e00";


-------------------------------------------------------------------------------
-- User logic
-------------------------------------------------------------------------------
constant add_userlogic			: unsigned(31 downto 0):=X"00C0_0000";

constant add_userlogic_ctrl_offset	: unsigned(31 downto 0):=X"0000_0000"; 
constant add_userlogic_bcidmax_offset	: unsigned(31 downto 0):=X"0000_0004";
constant add_userlogic_hbbcerror_offset	: unsigned(31 downto 0):=X"0000_0008";
constant add_userlogic_errcntall_offset	: unsigned(31 downto 0):=X"0000_000C";

--------------------------------------------------------------------------------
-- BAR 0 REGISTERs (DMA)
--------------------------------------------------------------------------------
constant add_pcie_dma_ctrl         : unsigned(31 downto 0)   :=add_bsp_info+X"0000_0200";
-- DESCRIPTOR SW -> CRU
constant add_pcie_dma_desc_h       : unsigned(31 downto 0)   :=add_bsp_info+X"0000_0204";
constant add_pcie_dma_desc_l       : unsigned(31 downto 0)   :=add_bsp_info+X"0000_0208";
constant add_pcie_dma_desc_sz      : unsigned(31 downto 0)   :=add_bsp_info+X"0000_020c";
--
constant add_pcie_dma_rst          : unsigned(31 downto 0)   :=add_bsp_info+X"0000_0400";
constant add_pcie_dma_ep_id        : unsigned(31 downto 0)   :=add_bsp_info+X"0000_0500";
-- DDG
constant add_pcie_dma_ddg_cfg0     : unsigned(31 downto 0)   :=add_bsp_info+X"0000_0600";
constant add_pcie_dma_ddg_cfg1     : unsigned(31 downto 0)   :=add_bsp_info+X"0000_0604";
constant add_pcie_dma_ddg_cfg2     : unsigned(31 downto 0)   :=add_bsp_info+X"0000_0608";
constant add_pcie_dma_ddg_cfg3     : unsigned(31 downto 0)   :=add_bsp_info+X"0000_060c";
--
constant add_pcie_dma_data_sel     : unsigned(31 downto 0)   :=add_bsp_info+X"0000_0700";
-- SUPERPAGE REPORT CRU > SW
constant add_pcie_dma_spg0_ack     : unsigned(31 downto 0)   :=add_bsp_info+X"0000_0800";
--
constant add_pcie_dma_dbg          : unsigned(31 downto 0)   :=add_bsp_info+X"0000_0c00";


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
    USERTXSEL   : out std_logic_vector(2*g_NUM_GBT_LINKS-1 downto 0);
    ---------------------------------------------------------------------------
    spare_in       : in  std_logic_vector(31 downto 0);  -- general purpose in
    ---------------------------------------------------------------------------
    GPI         : in  std_logic_vector(31 downto 0) := (others => '0');  -- general purpose in
    GPO         : out std_logic_vector(31 downto 0);  -- general purpose out
    ---------------------------------------------------------------------------
    SCL_I       : in  std_logic_vector(7 downto 0)  := (others => '1');
    SCL_T       : out std_logic_vector(7 downto 0);
    SCL_O       : out std_logic_vector(7 downto 0);
    SDA_I       : in  std_logic_vector(7 downto 0)  := (others => '1');
    SDA_T       : out std_logic_vector(7 downto 0);
    SDA_O       : out std_logic_vector(7 downto 0)
   ---------------------------------------------------------------------------
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
    FREQRAW : out std_logic_vector(DWIDTH - 1 downto 0);  -- valid in I clkdomain
    FREQ    : out std_logic_vector(DWIDTH - 1 downto 0)
    );

end component freqmeas;

component ddg is
  generic (
    g_TTC_DOWNLINK_BITS : integer := 192;
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
    TTC_RXD        : in  std_logic_vector(G_TTC_DOWNLINK_BITS-1 downto 0);
    --------------------------------------------------------------------------------
    -- GBT Downlink (CRU -> FE)
    --------------------------------------------------------------------------------
    gbt_tx_ready_i : in  std_logic_vector(g_NUM_GBT_LINKS-1 downto 0);

    gbt_tx_bus_o   : out t_cru_gbt;
    --------------------------------------------------------------------------------
    -- GBT Uplink (FE -> CRU)
    --------------------------------------------------------------------------------
    gbt_rx_ready_i : in  std_logic_vector(g_NUM_GBT_LINKS-1 downto 0);
    gbt_rx_bus_i   : in  t_cru_gbt_array (g_NUM_GBT_LINKS-1 downto 0);
    --------------------------------------------------------------------------------
    -- DMA interface
    --------------------------------------------------------------------------------
    FCLK0          : out std_logic;
    FVAL0          : out std_logic;
    FSOP0          : out std_logic;
    FEOP0          : out std_logic;
    FD0            : out std_logic_vector(255 downto 0)
    );
end component ddg;

component flowstat is
  generic(
    g_ID_BIT_LENGTH    : natural := 6;  -- Length of packet ID in bits.
    g_CLASS_BIT_LENGTH : natural := 2;  -- Length of packet CLASS in bits.

    g_PKT_SIZE_BIT_LENGTH : natural := 16;  -- Length of data vector containing
                                            -- accumulated packet sizes in bits

    g_PKT_COUNT_BIT_LENGTH : natural := 16;  -- Length of data vector containing
                                        -- the total number of packets with
                                        -- the same ID and CLASS
    g_EARLY_LENGTH         : natural := 50;
    g_LATE_LENGTH          : natural := 50
    );
  port(
    ---------------------------------------------------------------------------
    clk_i             : in  std_logic;
    rst_i             : in  std_logic;
    noupdate_i        : in  std_logic;
    noramclr_i        : in  std_logic;
    val_i             : in  std_logic;
    sop_i             : in  std_logic;
    eop_i             : in  std_logic;
    id_i              : in  std_logic_vector(31 downto 0);
    class_i           : in  std_logic_vector(g_CLASS_BIT_LENGTH-1 downto 0);
    current_id        : in  std_logic_vector(31 downto 0);
    max_rdout_entry_i : in  std_logic_vector(7 downto 0) := X"10";
    output_lenctrl_i  : in  std_logic;
    --
    ttc_clk_i         : in  std_logic;
    ttc_early_val_i   : in  std_logic;
    ttc_early_data_i  : in  std_logic_vector(g_EARLY_LENGTH - 1 downto 0);
    ttc_late_val_i    : in  std_logic;
    ttc_late_data_i   : in  std_logic_vector(g_LATE_LENGTH - 1 downto 0);
    --
    rsop_o            : out std_logic;
    reop_o            : out std_logic;
    rval_o            : out std_logic;
    rq_o              : out std_logic_vector(127 downto 0)
   ---------------------------------------------------------------------------
    );
end component flowstat;

component datapath_wrapper is

  generic (
    g_SMALLFIFO_LOGSIZE    : integer := 10;  -- in 256 bit words
    g_BIGFIFO_LOGSIZE      : integer := 13;  -- in 256 bit words
    g_NUM_GBT_LINKS        : integer := 1;
    g_DWRAPPER_NUM         : integer range 0 to 1 := 0;
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
    ---------------------------------------------------------------------------
    PCIe_CLK        : in  std_logic;        -- clock for this FIFO interface
    PCIe_RST        : in  std_logic;
    PCIe_AFULL      : in  std_logic;        -- almost full
    PCIe_SOP        : out std_logic;        -- start of packet
    PCIe_EOP        : out std_logic;        -- end of packet
    PCIe_VAL        : out std_logic;        -- data valid
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
function setAdd(mult : in natural; offset : unsigned) return unsigned;

end pack_cru_core;

package body pack_cru_core is
function setAdd(mult : in natural; offset : unsigned) return unsigned is
variable tmp : unsigned(31 downto 0);
begin
tmp:=to_unsigned(mult*to_integer(offset),32);
return tmp;
end setAdd;
end pack_cru_core;
