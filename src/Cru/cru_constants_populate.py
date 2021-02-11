import fileinput
import re

#'':'NUM_DROPPED_PACKETS'
roc_regs = {
### bar0 ###
'add_pcie_dma_ctrl':'DMA_CONTROL',
'add_pcie_dma_desc_h':'LINK_SUPERPAGE_ADDRESS_HIGH',
'add_pcie_dma_desc_l':'LINK_SUPERPAGE_ADDRESS_LOW',
'add_pcie_dma_desc_sz':'LINK_SUPERPAGE_PAGES',
'add_pcie_dma_spg0_ack':'LINK_SUPERPAGES_COUNT',
'add_pcie_dma_ddg_cfg0':'DATA_GENERATOR_CONTROL',
'add_pcie_dma_ddg_cfg2':'DATA_GENERATOR_INJECT_ERROR',
'add_pcie_dma_data_sel':'DATA_SOURCE_SELECT',
'add_pcie_dma_rst':'RESET_CONTROL',

### bar2 ###
'add_bsp_hkeeping_tempstat':'TEMPERATURE',
'add_bsp_info_builddate':'FIRMWARE_DATE',
'add_bsp_info_buildtime':'FIRMWARE_TIME',
'add_bsp_hkeeping_chipid_low':'FPGA_CHIP_LOW',
'add_bsp_hkeeping_chipid_high':'FPGA_CHIP_HIGH',
'add_ttc_clkgen_ttc240freq':'CTP_CLOCK',
'add_ttc_clkgen_lcl240freq':'LOCAL_CLOCK',
'add_gbt_wrapper0':'WRAPPER0',
'add_gbt_wrapper1':'WRAPPER1',
'add_ttc_clkgen_clkctrl':'CLOCK_CONTROL',
'add_ttc_data_ctrl':'TTC_DATA',
'add_ttc_onu_ctrl':'LOCK_CLOCK_TO_REF',
'add_ttc_onuerror_sticky':'TTC_ONU_STICKY',
'add_ctp_emu_runmode':'CTP_EMU_RUNMODE',
'add_ctp_emu_ctrl':'CTP_EMU_CTRL',
'add_ctp_emu_bc_max':'CTP_EMU_BCMAX',
'add_ctp_emu_hb_max':'CTP_EMU_HBMAX',
'add_ctp_emu_prescaler':'CTP_EMU_PRESCALER',
'add_ctp_emu_physdiv':'CTP_EMU_PHYSDIV',
'add_ctp_emu_caldiv':'CTP_EMU_CALDIV',
'add_ctp_emu_hcdiv':'CTP_EMU_HCDIV',
'add_ctp_emu_fbct':'CTP_EMU_FBCT',
'add_ctp_emu_orbit_init':'CTP_EMU_ORBIT_INIT',
'add_patplayer_cfg':'PATPLAYER_CFG',
'add_patplayer_idlepat0':'PATPLAYER_IDLE_PATTERN_0',
'add_patplayer_idlepat1':'PATPLAYER_IDLE_PATTERN_1',
'add_patplayer_idlepat2':'PATPLAYER_IDLE_PATTERN_2',
'add_patplayer_syncpat0':'PATPLAYER_SYNC_PATTERN_0',
'add_patplayer_syncpat1':'PATPLAYER_SYNC_PATTERN_1',
'add_patplayer_syncpat2':'PATPLAYER_SYNC_PATTERN_2',
'add_patplayer_rstpat0':'PATPLAYER_RESET_PATTERN_0',
'add_patplayer_rstpat1':'PATPLAYER_RESET_PATTERN_1',
'add_patplayer_rstpat2':'PATPLAYER_RESET_PATTERN_2',
'add_patplayer_synccnt':'PATPLAYER_SYNC_CNT',
'add_patplayer_delaycnt':'PATPLAYER_DELAY_CNT',
'add_patplayer_rstcnt':'PATPLAYER_RESET_CNT',
'add_patplayer_trigsel':'PATPLAYER_TRIGGER_SEL',
'add_datapathlink_offset':'DATAPATHLINK_OFFSET',
'add_datalink_offset':'DATALINK_OFFSET',
'add_datalink_ctrl':'DATALINK_CONTROL',
'add_datalink_feeid':'DATALINK_IDS',
'add_datalink_acc_pkt':'DATALINK_PACKETS_ACCEPTED',
'add_datalink_rej_pkt':'DATALINK_PACKETS_REJECTED',
'add_datalink_forced_pkt':'DATALINK_PACKETS_FORCED',
'add_gbt_wrapper_bank_offset':'GBT_WRAPPER_BANK_OFFSET',
'add_gbt_bank_link_offset':'GBT_BANK_LINK_OFFSET',
'add_gbt_link_regs_offset':'GBT_LINK_REGS_OFFSET',
'add_gbt_link_source_sel':'GBT_LINK_SOURCE_SELECT',
'add_gbt_link_status':'GBT_LINK_STATUS',
'add_gbt_link_clr_errcnt':'GBT_LINK_CLEAR_ERRORS',
'add_gbt_link_rxclk_cnt':'GBT_LINK_RX_CLOCK',
'add_gbt_link_txclk_cnt':'GBT_LINK_TX_CLOCK',
'add_gbt_link_xcvr_offset':'GBT_LINK_XCVR_OFFSET',
'add_gbt_link_tx_ctrl_offset':'GBT_LINK_TX_CONTROL_OFFSET',
'add_gbt_link_rx_ctrl_offset':'GBT_LINK_RX_CONTROL_OFFSET',
'add_bsp_info_usertxsel':'GBT_MUX_SELECT',
'add_bsp_info_userctrl':'BSP_USER_CONTROL',
'add_base_datapathwrapper0':'DWRAPPER_BASE0',
'add_base_datapathwrapper1':'DWRAPPER_BASE1',
'add_dwrapper_datagenctrl':'DWRAPPER_DATAGEN_CONTROL',
'add_gbt_wrapper_conf0':'GBT_WRAPPER_CONF0',
'add_gbt_wrapper_clk_cnt':'GBT_WRAPPER_CLOCK_COUNTER',
'add_gbt_wrapper_gregs':'GBT_WRAPPER_GREGS',
'add_dwrapper_gregs':'DWRAPPER_GREGS',
'add_dwrapper_enreg':'DWRAPPER_ENREG',
'add_dwrapper_drop_pkts':'DWRAPPER_DROPPED',
'add_dwrapper_tot_per_sec':'DWRAPPER_TOTAL_PACKETS_PER_SEC',
'add_dwrapper_trigsize':'DWRAPPER_TRIGGER_SIZE',
'add_ddg_ctrl':'DDG_CTRL0',
'add_ddg_ctrl2':'DDG_CTRL2',
'add_pon_wrapper_tx':'PON_WRAPPER_TX',
'add_pon_wrapper_pll':'PON_WRAPPER_PLL',
'add_pon_wrapper_reg':'PON_WRAPPER_REG',
'add_ttc_clkgen_onufpll':'CLOCK_ONU_FPLL',
'add_ttc_clkgen_pllctrlonu':'CLOCK_PLL_CONTROL_ONU',
'add_ttc_hbtrig_ltu':'LTU_HBTRIG_CNT',
'add_ttc_phystrig_ltu':'LTU_PHYSTRIG_CNT',
'add_ttc_eox_sox_ltu':'LTU_EOX_SOX_CNT',
'add_onu_user_logic':'ONU_USER_LOGIC',
'add_onu_user_refgen':'ONU_USER_REFGEN',
'add_onu_mgt_stickys':'ONU_MGT_STICKYS',
'add_refgen0_offset':'REFGEN0_OFFSET',
'add_refgen1_offset':'REFGEN1_OFFSET',
#'add_refgen2_offset':'I2C_COMMAND',
'add_flowctrl_offset':'FLOW_CONTROL_OFFSET',
'add_flowctrl_ctrlreg':'FLOW_CONTROL_REGISTER',
'add_gbt_wrapper_atx_pll':'GBT_WRAPPER_ATX_PLL',
'add_gbt_bank_fpll':'GBT_BANK_FPLL',
'add_bsp_i2c_eeprom':"BSP_I2C_EEPROM",
'add_bsp_i2c_minipods':"BSP_I2C_MINIPODS",
'add_bsp_i2c_si5345_1':'SI5345_1',
'add_bsp_i2c_si5345_2':'SI5345_2',
'add_bsp_i2c_si5344':'SI5344',
'add_pcie_dma_ep_id':'ENDPOINT_ID',
'add_ro_prot_system_id':'VIRTUAL_LINKS_IDS',

### SCA ###
'add_gbt_sc':'SC_BASE_INDEX',
'add_gbt_sca_wr_data':'SCA_WR_DATA',
'add_gbt_sca_wr_cmd':'SCA_WR_CMD',
'add_gbt_sca_wr_ctr':'SCA_WR_CTRL',

'add_gbt_sca_rd_data':'SCA_RD_DATA',
'add_gbt_sca_rd_cmd':'SCA_RD_CMD',
'add_gbt_sca_rd_ctr':'SCA_RD_CTRL',
'add_gbt_sca_rd_mon':'SCA_RD_MON',

'add_gbt_sc_link':'SC_LINK',
'add_gbt_sc_rst':'SC_RESET',

### SWT ###
'add_gbt_swt_wr_l':'SWT_WR_WORD_L',
'add_gbt_swt_wr_m':'SWT_WR_WORD_M',
'add_gbt_swt_wr_h':'SWT_WR_WORD_H',

'add_gbt_swt_rd_l':'SWT_RD_WORD_L',
'add_gbt_swt_rd_m':'SWT_RD_WORD_M',
'add_gbt_swt_rd_h':'SWT_RD_WORD_H',

'add_gbt_swt_cmd':'SWT_CMD',
'add_gbt_swt_mon':'SWT_MON',
'add_gbt_swt_word_mon':'SWT_WORD_MON'
}

# e.g. 'TEMPERATURE':0x00010008
to_replace = {}

def string_between(string, before, after):

  regex = before + "(.*)" + after 
  ret = re.split(regex, string)
  try:
      return ret[1].strip() #remove whitespace
  except IndexError:
      return ''

def parse_vhdl_hex(vhdl_lines, line):
  to_add = string_between(line, ":=", "\+")
  if (to_add != ''):
    for tline in vhdl_lines:
      if (to_add in tline):
        return parse_vhdl_hex(vhdl_lines, tline) + int(re.sub("_", "", string_between(line, "[Xx]\"", "\"\s*;")), 16)
    return 0
  else:
    try:
      return int(re.sub("_", "", string_between(line, "[Xx]\"", "\"\s*;")), 16)
    except ValueError:
      return 0


#create cru-fw table
vhdl_file = open('pack_cru_core.vhd', encoding="utf-8")
vhdl_lines = vhdl_file.readlines()

for (key, value) in roc_regs.items():
  for (i, line) in enumerate(vhdl_lines): #cru-fw regs here
    if (re.search("constant " + key + "\s*:", line)): #line has reg info
      ret = parse_vhdl_hex(vhdl_lines, line)
      to_replace[value] = "0x" + hex(ret)[2:].zfill(8) #add leading zeros for readability

print(to_replace)

# Update Cru/Constants.h
cfile = open('Constants.h')
contents = cfile.readlines()

for key,value in to_replace.items():
  for (i, line) in enumerate(contents):
    if(re.search("\s+Register\s*" + key, line)):
      contents[i] = re.sub("\([^)]*\)", '(' + value + ')', line)
    elif(re.search("\s+IntervalRegister\s*" + key, line)):
      contents[i] = re.sub("\([^,]*\,", '(' + value + ',', line)


cfile = open('Constants.h', 'w')
cfile.writelines(contents)

# Update include/ReadoutCard/Cru.h
cfile = open('../../include/ReadoutCard/Cru.h')
contents = cfile.readlines()

for key,value in to_replace.items():
  for (i, line) in enumerate(contents):
    if(re.search("\s+Register\s*" + key, line)):
      contents[i] = re.sub("\([^)]*\)", '(' + value + ')', line)
    elif(re.search("\s+IntervalRegister\s*" + key, line)):
      contents[i] = re.sub("\([^,]*\,", '(' + value + ',', line)


cfile = open('../../include/ReadoutCard/Cru.h', 'w')
cfile.writelines(contents)
