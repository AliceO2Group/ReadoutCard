///
/// \file ProgramCruExperimentalDma.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Port of https://gitlab.cern.ch/alice-cru/pciedma_eval
///

#include "Utilities/Program.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <pda.h>
#include "Factory/ChannelUtilityFactory.h"
#include "RORC/ChannelFactory.h"
#include "RorcException.h"
#include "Cru/CruRegisterIndex.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <ctype.h>
#include <time.h>

using namespace AliceO2::Rorc::Utilities;
using std::cout;
using std::endl;

#define FIFO_ENTRIES 4
// DMA page length in bytes
#define DMA_PAGE_LENGTH 8192
#define NUM_OF_BUFFERS 32

namespace {

class ProgramCruExperimentalDma: public Program
{
  public:

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("CRU EXPERIMENTAL DMA", "!!! USE WITH CAUTION !!!",
          "./rorc-cru-experimental-dma --serial=12345 --channel=0");
    }

    virtual void addOptions(boost::program_options::options_description&)
    {
    }

    virtual void mainFunction(const boost::program_options::variables_map&)
    {
      using namespace AliceO2::Rorc;

      cout << "Warning: this utility is highly experimental and will probably leave the CRU in a bad state, requiring "
          << "a reload of the firmware. It might also trigger a reboot.\n";
      cout << "  To proceed, type 'y'\n";
      cout << "  To abort, type anything else or give SIGINT (usually Ctrl-c)\n";
      int c = getchar();
      if (c != 'y' || isSigInt()) {
        return;
      }

      print_data = 1;
      print_status = 1;
      print_descriptor = 1;
      dma_buffer_size = 8l * 1024l * 1024l;

      cout << "Starting DMA test" << endl;
      pdaDMA();
      cout << "Finished DMA test" << endl;
    }

    DeviceOperator *dop;
    DMABuffer *buffer; // = NULL;
    uint32_t *bar_addr; // = NULL;
    size_t dma_buffer_size;

    typedef struct _descriptor_entry
    {
        uint32_t src_low;
        uint32_t src_high;
        uint32_t dst_low;
        uint32_t dst_high;
        uint32_t ctrl;
        uint32_t reservd1;
        uint32_t reservd2;
        uint32_t reservd3;
    } descriptor_entry;
    descriptor_entry *descriptor, *descriptor_usr;

    uint32_t *status, *status_usr;
    uint32_t *data, *data_usr, *readout;
    DMABuffer_SGNode *sglist; // = NULL;
    int print_status;
    int print_descriptor;
    int print_data;
    struct timespec t1, run_start, run_end;

    int initPDA()
    {
      system("modprobe -r uio_pci_dma");
      system("modprobe uio");
      system("modprobe uio_pci_dma");

      if (PDAInit() != PDA_SUCCESS) {
        printf("Error while initialization!\n");
        abort();
      }

      // A list of PCI ID to which PDA has to attach
      const char *pci_ids[] = { "1172 e001", // CRU as registered at CERN
      NULL // Delimiter
      };

      // The device operator manages all devices with the given IDs.
      dop = DeviceOperator_new(pci_ids, PDA_ENUMERATE_DEVICES);
      if (dop == NULL) {
        printf("Unable to get device-operator!\n");
        return -1;
      }

      // Get a device object for the first found device in the list.
      PciDevice *device = NULL;
      if (PDA_SUCCESS != DeviceOperator_getPciDevice(dop, &device, 0)) {
        if (PDA_SUCCESS != DeviceOperator_delete(dop, PDA_DELETE)) {
          printf("Device generation totally failed!\n");
          return -1;
        }
        printf("Can't get device!\n");
        return -1;
      }

      // DMA-Buffer allocation
      if (
      PDA_SUCCESS != PciDevice_allocDMABuffer(device, 0, dma_buffer_size, &buffer)) {
        if (PDA_SUCCESS != DeviceOperator_delete(dop, PDA_DELETE)) {
          printf("Buffer allocation totally failed!\n");
          return -1;
        }
        printf("DMA Buffer allocation failed!\n");
        return -1;
      }

      // Get the bar structure and map the BAR
      Bar *bar;
      if (PciDevice_getBar(device, &bar, 0) != PDA_SUCCESS) {
        printf("Can't get bar\n");
        exit(EXIT_FAILURE);
      }

      uint64_t length = 0;
      if (Bar_getMap(bar, (void**) &bar_addr, &length) != PDA_SUCCESS) {
        printf("Can't get map\n");
        exit(EXIT_FAILURE);
      }

      return 0;
    }

    int initDMA()
    {
      if (DMABuffer_getSGList(buffer, &sglist) != PDA_SUCCESS) {
        printf("Can't get list ...\n");
        return -1;
      }

      // IOMMU addresses
      status = (uint32_t*) sglist->d_pointer;
      // the start address of the descriptor table is always at the same place:
      // adding a 0x200 offset (=status+128) to the status start address, no
      // matter how many entries there are

      descriptor = (descriptor_entry*) (status + 128);
      data = (uint32_t*) (descriptor + FIFO_ENTRIES);

      // Virtual addresses
      status_usr = (uint32_t*) sglist->u_pointer;
      descriptor_usr = (descriptor_entry*) (status_usr + 128);
      data_usr = (uint32_t*) (descriptor_usr + FIFO_ENTRIES);

      // Initializing statuses with 0
      for (int i = 0; i < FIFO_ENTRIES; i++) {
        status_usr[i] = 0;
      }

      // Allocating memory where the data will be read out to
      // readout = (uint32_t*)calloc(FIFO_ENTRIES, 256); //dma length=256 byte
      readout = (uint32_t*) calloc(FIFO_ENTRIES, DMA_PAGE_LENGTH);

      // Initializing the descriptor table
      for (int i = 0; i < FIFO_ENTRIES; i++) {
        const uint32_t ctrl = (i << 18) + (DMA_PAGE_LENGTH / 4);
        descriptor_usr[i].ctrl = ctrl;
        // Adresses in the card's memory (DMA source)
        descriptor_usr[i].src_low = i * DMA_PAGE_LENGTH;
        descriptor_usr[i].src_high = 0x0;
        // Addresses in the RAM (DMA destination)
        auto dst = (data + i * DMA_PAGE_LENGTH / sizeof(uint32_t));
        descriptor_usr[i].dst_low = (uint64_t) dst & 0xffffffff;
        descriptor_usr[i].dst_high = (uint64_t) dst >> 32;
        //fill the reserved bit with zero
        descriptor_usr[i].reservd1 = 0x0;
        descriptor_usr[i].reservd2 = 0x0;
        descriptor_usr[i].reservd3 = 0x0;
      }

      // Setting the buffer to the default value of 0xcccccccc
      for (int i = 0; i < DMA_PAGE_LENGTH / sizeof(uint32_t) * FIFO_ENTRIES; i++) {
        data_usr[i] = 0xcccccccc;
      }

      // Status base address, low and high part, respectively
      bar_addr[0] = (uint64_t) status & 0xffffffff;
      bar_addr[1] = (uint64_t) status >> 32;

      // Destination (card's memory) addresses, low and high part, respectively
      bar_addr[2] = 0x8000;
      bar_addr[3] = 0x0;

      // Set descriptor table size, same as number of available pages-1
      bar_addr[5] = FIFO_ENTRIES - 1;

      // Timestamp for test
      clock_gettime(CLOCK_REALTIME, &t1);

      // Number of available pages-1
      bar_addr[4] = FIFO_ENTRIES - 1;

      // make pcie reday signal
      bar_addr[129] = 0x1;

      // Programming the user module to trigger the data emulator
      bar_addr[128] = 0x1;

      // Set status to send status for every page not only for the last one
      bar_addr[6] = 0x1;

      return 0;
    }

    int pdaDMA()
    {
      int pattern[1023];
      {
        for (int i = 2; i < 1024; i++) {
          pattern[i] = i - 1;
        }
        pattern[0] = 0;
        pattern[1] = 0;
      }

      if (initPDA() != 0) {
        printf("initPDA error ...\n");
        return -1;
      }

      if (initDMA() != 0) {
        printf("initDMA error ...\n");
        return -1;
      }

      FILE *fp;
      fp = fopen("dma_data.txt", "w");

      // DMA; rolling
      int i = 0;
      int lock = 0;
      int counter = 0;
      int current_page = 0;
      int num_of_err = 0;
      int rolling = 0;

      // deassert reset for led module
      // bar_addr[136] = 0xd;
      // write data in led module
      // bar_addr[140] = 0x3;
      clock_gettime(CLOCK_REALTIME, &run_start);

      while (i != FIFO_ENTRIES * NUM_OF_BUFFERS) {
        if (isSigInt()) {
          break;
        }

        printf("pdaDMA is running\n");

        // LEDs off
        bar_addr[152] = 0xff;

        if (status_usr[i] == 1) {

          memcpy(readout + i * 256 / sizeof(uint32_t), data_usr + i * 256 / sizeof(uint32_t), 256);

          // Writing data to file
          fprintf(fp, "%d", i);
          fprintf(fp, "\n");

          for (int j = 0; j < DMA_PAGE_LENGTH / 32; j++) {
            if (isSigInt()) {
              break;
            }

            for (int k = 7; k >= 0; k--) {
              fprintf(fp, "%d", data_usr[i * DMA_PAGE_LENGTH / sizeof(uint32_t) + j * 8 + k]);
            }
            fprintf(fp, "\n");

            // Data error checking
            size_t usr_index = i * (DMA_PAGE_LENGTH / sizeof(uint32_t)) + j * 8;
            size_t pattern_index = current_page * DMA_PAGE_LENGTH / 32 + j;
            if (data_usr[usr_index] != pattern[pattern_index]) {
              num_of_err++;
              printf("data error at rolling %d, page %d, data %d, %d - %d\n", rolling, i, j * 8, data_usr[usr_index],
                  pattern[pattern_index]);
            }
          }

          fprintf(fp, "\n");

          current_page++;

          // Setting the buffer to the default value of 0xcccccccc after the readout
          for (size_t j = 0; j < DMA_PAGE_LENGTH / sizeof(uint32_t); j++) {
            data_usr[i * DMA_PAGE_LENGTH / sizeof(uint32_t) + j] = 0xcccccccc;
          }

          printf(" %d %x\n", i, status_usr[i]);

          //generates lock state for if l==0 case
          if (i == 1) {
            lock = 1;
          }

          //make status zero for each descriptor for next rolling
          status_usr[i] = 0;

          //points to the first descriptor
          if (lock == 0) {
            bar_addr[4] = FIFO_ENTRIES * NUM_OF_BUFFERS - 1;
          }

          //push the same descriptor again when its gets executed
          bar_addr[4] = i;

          counter++;

          // LEDs on
          bar_addr[152] = 0x00;

          //after every 32Kbytes reset the current_page for online checking
          if ((i % FIFO_ENTRIES) == FIFO_ENTRIES - 1) {

            current_page = 0;

            printf("counter = %d, rolling = %d\n", counter - 1, rolling);
            printf(" rolling = %d\n", rolling);

            //counter = 0;

            rolling++;

            // Measuring time difference
            printf("t1:\t%d s\t%ld us\n", t1.tv_sec, t1.tv_nsec);

            // Only if we want a fixed number of rolling
            if (rolling == 1000) {
              printf("Rolling == 1000 -> stopping\n");
              break;
            }

            clock_gettime(CLOCK_REALTIME, &t1);
          }

          // loop over 128 descriptors or 1MByte DMA transfer
          if (i == FIFO_ENTRIES * NUM_OF_BUFFERS - 1) {
            // read status count after 1MByte dma transfer
            printf("%d\n", bar_addr[148]);
            i = 0;
          } else {
            i++;
          }
        }
      }

      fclose(fp);

      if (isSigInt()) {
        printf("Interrupted\n");
      }

      // Uncomment this when measuring throughput
      clock_gettime(CLOCK_REALTIME, &run_end);

      // Calculating throughput
      uint32_t sectime = run_end.tv_sec - run_start.tv_sec;
      int64_t nsectime = run_end.tv_nsec - run_start.tv_nsec;
      if (nsectime < 0) {
        sectime--;
        nsectime += 1000000000;
      }
      double dtime = sectime + nsectime / 1000000000.0;
      double throughput = (double) rolling / dtime * FIFO_ENTRIES * DMA_PAGE_LENGTH / (1024 * 1024 * 1024);
      printf("Throughput is %f GB/s or %f Gb/s.\n", throughput, throughput * 8);

      // Uncomment this while online checking
      printf("Number of errors is %d\n", num_of_err);

      return DeviceOperator_delete(dop, PDA_DELETE_PERSISTANT);
    }
};

} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramCruExperimentalDma().execute(argc, argv);
}
