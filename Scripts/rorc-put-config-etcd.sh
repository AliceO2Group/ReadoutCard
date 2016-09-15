#!/bin/bash

#uri="http://localhost:4001"
uri="http://aido2qc10:4001"

put ()
{
  curl -X PUT \
    "${uri}/v2/keys/RORC/card_${card_type}/serial_${serial_number}/channel_${channel_number}/parameters/${1}" \
    -d "value=${2}"  
}

card_type="CRORC"
serial_number="11225"
channel_number="0"
put "dma_page_size" "4096" # 4 KiB
put "dma_buffer_size" "8388608" # 8 MiB
put "generator_enabled" "true"
put "generator_data_size" "4096" # == page size
put "generator_loopback_mode" "RORC"

card_type="CRU"
serial_number="12345"
channel_number="0"
put "dma_page_size" "8192" # 8 KiB
put "dma_buffer_size" "8388608" # 8 MiB
put "generator_enabled" "true"
put "generator_data_size" "8192" # == page size
put "generator_loopback_mode" "RORC"
