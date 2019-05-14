import fileinput
import re

#'':'NUM_DROPPED_PACKETS'
roc_regs = {
### bar0 ###
'add_pcie_dma_ctrl':'DMA_CONTROL',
'add_pcie_dma_desc_h':'LINK_SUPERPAGE_ADDRESS_HIGH',
'add_pcie_dma_desc_l':'LINK_SUPERPAGE_ADDRESS_LOW',
'add_pcie_dma_desc_sz':'LINK_SUPERPAGE_SIZE',
'add_pcie_dma_spg0_ack':'LINK_SUPERPAGES_PUSHED',
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
'add_datapathlink_offset':'DATAPATHLINK_OFFSET',
'add_datalink_offset':'DATALINK_OFFSET',
'add_datalink_ctrl':'DATALINK_CONTROL',
'add_gbt_wrapper_bank_offset':'GBT_WRAPPER_BANK_OFFSET',
'add_gbt_bank_link_offset':'GBT_BANK_LINK_OFFSET',
'add_gbt_link_regs_offset':'GBT_LINK_REGS_OFFSET',
'add_gbt_link_source_sel':'GBT_LINK_SOURCE_SELECT',
'add_gbt_link_xcvr_offset':'GBT_LINK_XCVR_OFFSET',
'add_gbt_link_tx_ctrl_offset':'GBT_LINK_TX_CONTROL_OFFSET',
'add_gbt_link_rx_ctrl_offset':'GBT_LINK_RX_CONTROL_OFFSET',
'add_bsp_info_usertxsel':'GBT_MUX_SELECT',
'add_bsp_info_userctrl':'BSP_USER_CONTROL',
'add_base_datapathwrapper0':'DWRAPPER_BASE0',
'add_base_datapathwrapper1':'DWRAPPER_BASE1',
'add_gbt_wrapper_conf0':'GBT_WRAPPER_CONF0',
'add_gbt_wrapper_clk_cnt':'GBT_WRAPPER_CLOCK_COUNTER',
'add_gbt_wrapper_gregs':'GBT_WRAPPER_GREGS',
'add_dwrapper_gregs':'DWRAPPER_GREGS',
'add_dwrapper_enreg':'DWRAPPER_ENREG',
'add_ddg_ctrl':'DDG_CTRL0',
'add_ddg_ctrl2':'DDG_CTRL2',
'add_pon_wrapper_tx':'PON_WRAPPER_TX',
'add_pon_wrapper_pll':'PON_WRAPPER_PLL',
'add_ttc_clkgen_onufpll':'CLOCK_ONU_FPLL',
'add_ttc_clkgen_pllctrlonu':'CLOCK_PLL_CONTROL_ONU',
'add_onu_user_logic':'ONU_USER_LOGIC',
'add_onu_user_refgen':'ONU_USER_REFGEN',
'add_refgen0_offset':'REFGEN0_OFFSET',
'add_refgen1_offset':'REFGEN1_OFFSET',
#'add_refgen2_offset':'I2C_COMMAND',
'add_flowctrl_offset':'FLOW_CONTROL_OFFSET',
'add_flowctrl_ctrlreg':'FLOW_CONTROL_REGISTER',
'add_dwrapper_muxctrl':'DWRAPPER_MUX_CONTROL',
'add_gbt_wrapper_atx_pll':'GBT_WRAPPER_ATX_PLL',
'add_gbt_bank_fpll':'GBT_BANK_FPLL',
'add_bsp_i2c_si5345_1':'SI5345_1',
'add_bsp_i2c_si5345_2':'SI5345_2',
'add_bsp_i2c_si5344':'SI5344',
'add_pcie_dma_ep_id':'ENDPOINT_ID'
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
vhdl_file = open('pack_cru_core.vhd')
vhdl_lines = vhdl_file.readlines()

for (key, value) in roc_regs.items():
  for (i, line) in enumerate(vhdl_lines): #cru-fw regs here
    if (re.search("constant " + key + "\s*:", line)): #line has reg info
      ret = parse_vhdl_hex(vhdl_lines, line)
      to_replace[value] = "0x" + hex(ret)[2:].zfill(8) #add leading zeros for readability

print(to_replace)

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
