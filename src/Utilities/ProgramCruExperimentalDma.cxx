///
/// \file ProgramCruExperimentalDma.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Port of https://gitlab.cern.ch/alice-cru/pciedma_eval
///


// Notes:
// The DMA transfer seems to work, but not consistently.
// With shorter MAX_ROLLING (like 1500), it seems to work about 1 out of 4 times.
// Higher (15k), it seems to nearly always work.

#include "Utilities/Program.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <pda.h>
#include <fstream>
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
#include <boost/scoped_ptr.hpp>
#include "Factory/ChannelUtilityFactory.h"
#include "RORC/ChannelFactory.h"
#include "RorcException.h"
#include "RorcDevice.h"
#include "MemoryMappedFile.h"
#include "Cru/CruRegisterIndex.h"
#include "Cru/CruFifoTable.h"
#include "Cru/CruRegisterIndex.h"
#include "Pda/PdaDevice.h"
#include "Pda/PdaBar.h"
#include "Pda/PdaDmaBuffer.h"

namespace Register = AliceO2::Rorc::CruRegisterIndex;
using namespace AliceO2::Rorc::Utilities;
using namespace AliceO2::Rorc;
using std::cout;
using std::endl;

struct descriptor_entry
{
    uint32_t src_low;
    uint32_t src_high;
    uint32_t dst_low;
    uint32_t dst_high;
    uint32_t ctrl;
    uint32_t reservd1;
    uint32_t reservd2;
    uint32_t reservd3;
};

constexpr int FIFO_ENTRIES = 4;

/// DMA page length in bytes
constexpr int DMA_PAGE_SIZE = 8 * 1024;

/// DMA page length in 32-bit words
constexpr int DMA_PAGE_SIZE_32 = DMA_PAGE_SIZE / 4;

constexpr int NUM_OF_BUFFERS = 32;

constexpr int MAX_ROLLING = 15000;

/// Offset in bytes from start of status table to data buffer
constexpr size_t DATA_OFFSET = 0x200;

constexpr size_t DMA_BUFFER_PAGES_SIZE = 8l * 1024l * 1024l; // Should be enough...

constexpr uint32_t DEFAULT_VALUE = 0xCcccCccc;

constexpr int BUFFER_INDEX_PAGES = 0;
constexpr int BUFFER_INDEX_FIFO = 0;

const std::string DMA_BUFFER_PAGES_PATH = "/mnt/hugetlbfs/rorc-cru-experimental-dma-pages";

namespace {

class ProgramCruExperimentalDma: public Program
{
  public:

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("CRU EXPERIMENTAL DMA", "!!! USE WITH CAUTION !!!",
          "./rorc-cru-experimental-dma");
    }

    virtual void addOptions(boost::program_options::options_description&)
    {
    }

    virtual void mainFunction(const boost::program_options::variables_map&)
    {
      using namespace AliceO2::Rorc;

//      cout << "Warning: this utility is highly experimental and will probably leave the CRU in a bad state, requiring "
//          << "a reload of the firmware. It might also trigger a reboot.\n";
//      cout << "  To proceed, type 'y'\n";
//      cout << "  To abort, type anything else or give SIGINT (usually Ctrl-c)\n";
//      int c = getchar();
//      if (c != 'y' || isSigInt()) {
//        return;
//      }

      cout << "Starting DMA test" << endl;
      pdaDMA();
      cout << "Finished DMA test" << endl;
    }

    boost::scoped_ptr<RorcDevice> rorcDevice;
    boost::scoped_ptr<Pda::PdaBar> pdaBar;
    boost::scoped_ptr<MemoryMappedFile> mappedFilePages;
//    boost::scoped_ptr<MemoryMappedFile> mappedFileFifo;
    boost::scoped_ptr<Pda::PdaDmaBuffer> bufferPages;
//    boost::scoped_ptr<Pda::PdaDmaBuffer> bufferFifo;
    volatile uint32_t* bar;

    CruFifoTable* fifoUser;

    uint32_t* data_user; ///< DMA buffer (userspace address)

    timespec t1;
    timespec run_start;
    timespec run_end;

    int initPDA()
    {
      system("modprobe -r uio_pci_dma");
      system("modprobe uio_pci_dma");

      int serial = 12345;
      int channel = 0;
      Util::resetSmartPtr(rorcDevice, serial);
      Util::resetSmartPtr(pdaBar, rorcDevice->getPciDevice(), channel);
      bar = pdaBar->getUserspaceAddressU32();
      Util::resetSmartPtr(mappedFilePages, DMA_BUFFER_PAGES_PATH.c_str(), DMA_BUFFER_PAGES_SIZE);
//      Util::resetSmartPtr(mappedFileFifo, "/tmp/rorc-cru-experimental-dma-fifo", DMA_BUFFER_FIFO_SIZE);
      Util::resetSmartPtr(bufferPages, rorcDevice->getPciDevice(), mappedFilePages->getAddress(),
          mappedFilePages->getSize(), BUFFER_INDEX_PAGES);
//      Util::resetSmartPtr(bufferFifo, rorcDevice->getPciDevice(), mappedFileFifo->getAddress(),
//          mappedFileFifo->getSize(), BUFFER_INDEX_PAGES);
      return 0;
    }

    int initDMA()
    {
      auto list = bufferPages->getScatterGatherList();
      cout << "SGL entries: " << list.size() << '\n';
      auto entry = list.at(0);
      cout << "SG Length: " << double(entry.size)/1024.0/1024.0 << " MiB\n";

      // Initializing the descriptor table
      fifoUser = reinterpret_cast<CruFifoTable*>(entry.addressUser);
//      cout << "FIFO address user       : " << (void*) fifoUser << '\n';
      fifoUser->resetStatusEntries();

      for (int i = 0; i < FIFO_ENTRIES * NUM_OF_BUFFERS; i++) {
        auto sourceAddress = reinterpret_cast<void*>((i % NUM_OF_BUFFERS) * DMA_PAGE_SIZE);

        // Addresses in the RAM (DMA destination)
        // the start address of the descriptor table is always at the same place:
        // adding a 0x200 offset (=status+128) to the status start address, no
        // matter how many entries there are
        uint32_t* status_device = (uint32_t*) entry.addressBus;
        descriptor_entry* descriptor_device = (descriptor_entry*) (status_device + 128);
        uint32_t* data_device = (uint32_t*) (descriptor_device + FIFO_ENTRIES * NUM_OF_BUFFERS);
//        cout << "Status address device   : " << (void*) status_device << '\n';
//        cout << "Data address device     : " << (void*) data_device << '\n';
        auto destinationAddress = data_device + i * DMA_PAGE_SIZE_32;

        fifoUser->descriptorEntries[i].setEntry(i, DMA_PAGE_SIZE_32, sourceAddress, destinationAddress);
      }

      // Setting the buffer to the default value
      data_user = reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(fifoUser) + sizeof(CruFifoTable));
//      cout << "Data address user       : " << (void*) data_user << '\n';
      for (int i = 0; i < DMA_PAGE_SIZE_32 * FIFO_ENTRIES; i++) {
        data_user[i] = DEFAULT_VALUE;
      }

      // Status base address, low and high part, respectively
      CruFifoTable* fifoDevice = reinterpret_cast<CruFifoTable*>(entry.addressBus);
//      cout << "FIFO address device     : " << (void*) fifoDevice<< '\n';
      bar[Register::STATUS_BASE_BUS_HIGH] = Util::getUpper32Bits(uint64_t(fifoDevice));
      bar[Register::STATUS_BASE_BUS_LOW] = Util::getLower32Bits(uint64_t(fifoDevice));

      // Destination (card's memory) addresses, low and high part, respectively
      bar[Register::FIFO_BASE_CARD_HIGH] = 0x0;
      bar[Register::FIFO_BASE_CARD_LOW] = 0x8000;

      // Set descriptor table size, same as number of available pages-1
      bar[Register::DESCRIPTOR_TABLE_SIZE] = FIFO_ENTRIES * NUM_OF_BUFFERS - 1;

      // Timestamp for test
      clock_gettime(CLOCK_REALTIME, &t1);

      // Number of available pages-1
      bar[Register::DMA_POINTER] = FIFO_ENTRIES * NUM_OF_BUFFERS - 1;

      // make buffer ready signal
      bar[Register::BUFFER_READY] = 0x1;

      // Programming the user module to trigger the data emulator
      bar[Register::DATA_EMULATOR_ENABLE] = 0x1;

      // Set status to send status for every page not only for the last one
      bar[Register::SEND_STATUS] = 0x1;

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

      std::ofstream file("dma_data.txt");

      // DMA; rolling
      int i = 0;
      int lock = 0;
      int counter = 0;
      int current_page = 0;
      int num_of_err = 0;
      int rolling = 0;

      clock_gettime(CLOCK_REALTIME, &run_start);

      // Allocating memory where the data will be read out to
      std::array<uint32_t, FIFO_ENTRIES * NUM_OF_BUFFERS * DMA_PAGE_SIZE_32> readout;

      while (i != FIFO_ENTRIES * NUM_OF_BUFFERS) {
        if (isSigInt()) {
          break;
        }

        //cout << "pdaDMA is running\n";

        // LEDs off
        bar[Register::LED_STATUS] = 0xff;

        if (fifoUser->statusEntries[i].isPageArrived()) {

          memcpy(&readout[i * DMA_PAGE_SIZE_32], &data_user[i * DMA_PAGE_SIZE_32], DMA_PAGE_SIZE);

          // Writing data to file
          file << i << '\n';

          for (int j = 0; j < DMA_PAGE_SIZE / 32; j++) {
            if (isSigInt()) {
              break;
            }

            for (int k = 7; k >= 0; k--) {
              file << readout[i * DMA_PAGE_SIZE_32 + j * 8 + k];
            }
            file << '\n';

            // Data error checking
            size_t usr_index = i * DMA_PAGE_SIZE_32 + j * 8;
            size_t pattern_index = current_page * DMA_PAGE_SIZE / 32 + j;
            if (readout[usr_index] != pattern[pattern_index]) {
              num_of_err++;
              cout << "data error at rolling " << rolling
                  << ", page " << i
                  << ", data " << j * 8
                  << ", " << readout[usr_index]
                  << " - " << pattern[pattern_index] << '\n';
            }
          }
          file << '\n';

          current_page++;

          // Setting the buffer to the default value after the readout
          for (size_t j = 0; j < DMA_PAGE_SIZE_32; j++) {
            data_user[i * DMA_PAGE_SIZE_32 + j] = DEFAULT_VALUE;
          }

          //cout << " " << i << " " << fifoUser->statusEntries[i].status << '\n';

          //generates lock state for if l==0 case
          if (i == 1) {
            lock = 1;
          }

          //make status zero for each descriptor for next rolling
          fifoUser->statusEntries[i].reset();

          //points to the first descriptor
          if (lock == 0) {
            bar[Register::DMA_POINTER] = FIFO_ENTRIES * NUM_OF_BUFFERS - 1;
          }

          //push the same descriptor again when its gets executed
          bar[Register::DMA_POINTER] = i;

          counter++;

          // LEDs on
          bar[Register::LED_STATUS] = 0x00;

          //after every 32Kbytes reset the current_page for online checking
          if ((i % FIFO_ENTRIES) == FIFO_ENTRIES - 1) {

            current_page = 0;

//            cout << "counter = " << counter - 1 << '\n';
            cout << "rolling = " << rolling << '\n';

            //counter = 0;

            rolling++;

            // Measuring time difference
//            cout << "t1:\t" << t1.tv_sec << "s " << "\t" << t1.tv_nsec << " us\n";

            // Only if we want a fixed number of rolling
            if (rolling == MAX_ROLLING) {
              cout << "Rolling == " << MAX_ROLLING << " -> stopping\n";
              break;
            }

            clock_gettime(CLOCK_REALTIME, &t1);
          }

          if (i == FIFO_ENTRIES * NUM_OF_BUFFERS - 1) {
            // read status count after 1MByte dma transfer
//            cout << bar[Register::READ_STATUS_COUNT] << '\n';
            i = 0;
          } else {
            i++;
          }
        }
      }

      if (isSigInt()) {
        cout << "Interrupted\n";
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
      double throughput = (double) rolling / dtime * FIFO_ENTRIES * DMA_PAGE_SIZE / (1024 * 1024 * 1024);
      cout << "Throughput is " << throughput << " GB/s or " << throughput * 8 << " Gb/s.\n";
      cout << "Number of errors is " << num_of_err << '\n';
      cout << "Firmware info: " << Utilities::Common::make32hexString(bar[Register::FIRMWARE_COMPILE_INFO]) << '\n';
    }
};

} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramCruExperimentalDma().execute(argc, argv);
}
