import fileinput
import re
import cru_table as table
import os

#'':'NUM_DROPPED_PACKETS'
roc_regs = {'add_bsp_hkeeping_tempstat':'TEMPERATURE',
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
'add_bsp_i2c_si5344':'SI5344'
}

# e.g. 'TEMPERATURE':0x00010008
to_replace = {}

for key0,value0 in roc_regs.iteritems():
  for key,value in table.CRUADD.iteritems():
    if (key0 == key):
      to_replace[value0] = '0x' + str(format(value, '08x'))

print to_replace 

cfile = open('Constants.h')
contents = cfile.readlines()

for key,value in to_replace.iteritems():
  for (i, line) in enumerate(contents):
    if (key in line):
      contents[i] = re.sub("\([^)]*\)", '(' + value + ')', line)

cfile = open('Constants.h', 'w')
cfile.writelines(contents)
