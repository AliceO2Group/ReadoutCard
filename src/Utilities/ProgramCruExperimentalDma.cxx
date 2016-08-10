///
/// \file ProgramCruExperimentalDma.cxx
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///
/// \brief Port of https://gitlab.cern.ch/alice-cru/pciedma_eval
///


// Notes:
// Sometimes the card is not recognized properly as PCIe (check lspci), and the program will hang. Cause unknown.

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
#include <boost/format.hpp>
#include <boost/filesystem/operations.hpp>
#include "RorcException.h"
#include "RorcDevice.h"
#include "MemoryMappedFile.h"
#include "Cru/CruRegisterIndex.h"
#include "Cru/CruFifoTable.h"
#include "Cru/CruRegisterIndex.h"
#include "Pda/PdaDevice.h"
#include "Pda/PdaBar.h"
#include "Pda/PdaDmaBuffer.h"

namespace b = boost;
namespace bfs = boost::filesystem;
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

//constexpr int MAX_ROLLING = 1500;

/// Offset in bytes from start of status table to data buffer
constexpr size_t DATA_OFFSET = 0x200;

constexpr size_t DMA_BUFFER_PAGES_SIZE = 2l * 1024l * 1024l; // One 2MiB hugepage. Should be enough...

constexpr uint32_t DEFAULT_VALUE = 0xCcccCccc;

/// PDA DMA buffer index for the pages buffer
constexpr int BUFFER_INDEX_PAGES = 0;

const bfs::path DMA_BUFFER_PAGES_PATH = "/mnt/hugetlbfs/rorc-cru-experimental-dma-pages";

const std::string PROGRESS_FORMAT = "  %-5s %-8s  %-8s  %-8s %-8s";

const std::string TO_FILE_SWITCH = "tofile";

const std::string ROLLS_SWITCH = "rolls";
constexpr int ROLLS_DEFAULT = 1500;

namespace {

class ProgramCruExperimentalDma: public Program
{
  public:

    virtual UtilsDescription getDescription()
    {
      return UtilsDescription("CRU EXPERIMENTAL DMA", "!!! USE WITH CAUTION !!!", "./rorc-cru-experimental-dma");
    }

    virtual void addOptions(boost::program_options::options_description& options)
    {
      options.add_options()(TO_FILE_SWITCH.c_str(), "Read out to file");
      options.add_options()(ROLLS_SWITCH.c_str(),
          boost::program_options::value<int>(&maxRolls)->default_value(ROLLS_DEFAULT),
          "Amount of iterations over the FIFO");
    }

    virtual void mainFunction(const boost::program_options::variables_map& map)
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

      enableFileOutput = bool(map.count(TO_FILE_SWITCH));

      cout << "Starting DMA test" << endl;
      pdaDMA();
      cout << "Finished DMA test" << endl;
    }

    int maxRolls;
    bool enableFileOutput;

    boost::scoped_ptr<RorcDevice> rorcDevice;
    boost::scoped_ptr<Pda::PdaBar> pdaBar;
    boost::scoped_ptr<MemoryMappedFile> mappedFilePages;
//    boost::scoped_ptr<MemoryMappedFile> mappedFileFifo;
    boost::scoped_ptr<Pda::PdaDmaBuffer> bufferPages;
//    boost::scoped_ptr<Pda::PdaDmaBuffer> bufferFifo;
    volatile uint32_t* bar;

    CruFifoTable* fifoUser;

    uint32_t* dataUser; ///< DMA buffer (userspace address)

    void init()
    {
      system("modprobe -r uio_pci_dma");
      system("modprobe uio_pci_dma");

      bfs::remove(DMA_BUFFER_PAGES_PATH);

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

      auto entry = bufferPages->getScatterGatherList().at(0);
      if (entry.size < (2l * 1024l * 1024l)) {
        BOOST_THROW_EXCEPTION(CruException()
            << errinfo_rorc_error_message("Unsupported configuration: DMA scatter-gather entry size less than 2 MiB")
            << errinfo_rorc_scatter_gather_entry_size(entry.size)
            << errinfo_rorc_possible_causes(
                {"DMA buffer was not allocated in hugepage shared memory (hugetlbfs may not be properly mounted)"}));
      }

#define THIS_SHOULD_WORK
#ifndef THIS_SHOULD_WORK
      // NOTE: This is known to be working

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
        auto destinationAddress = data_device + i * DMA_PAGE_SIZE_32;

        fifoUser->descriptorEntries[i].setEntry(i, DMA_PAGE_SIZE_32, sourceAddress, destinationAddress);
      }

      // Setting the buffer to the default value
      dataUser = reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(fifoUser) + sizeof(CruFifoTable));
      for (int i = 0; i < DMA_PAGE_SIZE_32 * FIFO_ENTRIES; i++) {
        dataUser[i] = DEFAULT_VALUE;
      }

      // Status base address, low and high part, respectively
      CruFifoTable* fifoDevice = reinterpret_cast<CruFifoTable*>(entry.addressBus);
#else
      // NOTE: This should also work, but it sometimes didn't

      auto getDataAddress = [](void* fifoAddress) {
        // Data starts directly after the FIFO
        return reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(fifoAddress) + sizeof(CruFifoTable));
      };

      // Addresses in the RAM (DMA destination)
      // the start address of the descriptor table is always at the same place:
      // adding a 0x200 offset (= sizeof(CruFifoTable)) to the status start address, no
      // matter how many entries there are
      fifoUser = reinterpret_cast<CruFifoTable*>(entry.addressUser);
      dataUser = getDataAddress(fifoUser);
      CruFifoTable* fifoDevice = reinterpret_cast<CruFifoTable*>(entry.addressBus);
      uint32_t* dataDevice = getDataAddress(fifoDevice);

      // Initializing the descriptor table
      fifoUser->resetStatusEntries();

      for (int i = 0; i < FIFO_ENTRIES * NUM_OF_BUFFERS; i++) {
        auto sourceAddress = reinterpret_cast<void*>((i % NUM_OF_BUFFERS) * DMA_PAGE_SIZE);
        auto destinationAddress = dataDevice + i * DMA_PAGE_SIZE_32;
        fifoUser->descriptorEntries[i].setEntry(i, DMA_PAGE_SIZE_32, sourceAddress, destinationAddress);
      }

      // Setting the buffer to the default value

//      cout << "Data address user       : " << (void*) data_user << '\n';
      for (int i = 0; i < DMA_PAGE_SIZE_32 * FIFO_ENTRIES; i++) {
        dataUser[i] = DEFAULT_VALUE;
      }
#endif

      // Status base address, low and high part, respectively
//      cout << "FIFO address device     : " << (void*) fifoDevice<< '\n';
      bar[Register::STATUS_BASE_BUS_HIGH] = Util::getUpper32Bits(uint64_t(fifoDevice));
      bar[Register::STATUS_BASE_BUS_LOW] = Util::getLower32Bits(uint64_t(fifoDevice));

      // Destination (card's memory) addresses, low and high part, respectively
      bar[Register::FIFO_BASE_CARD_HIGH] = 0x0;
      bar[Register::FIFO_BASE_CARD_LOW] = 0x8000;

      // Set descriptor table size, same as number of available pages-1
      bar[Register::DESCRIPTOR_TABLE_SIZE] = FIFO_ENTRIES * NUM_OF_BUFFERS - 1;

      // Number of available pages-1
      bar[Register::DMA_POINTER] = FIFO_ENTRIES * NUM_OF_BUFFERS - 1;

      // make buffer ready signal
      bar[Register::BUFFER_READY] = 0x1;

      // Programming the user module to trigger the data emulator
      bar[Register::DATA_EMULATOR_ENABLE] = 0x1;

      // Set status to send status for every page not only for the last one
      bar[Register::SEND_STATUS] = 0x1;
    }

    int pdaDMA()
    {
      init();

      cout << "\nFirmware info:\n" << Utilities::Common::make32hexString(bar[Register::FIRMWARE_COMPILE_INFO]) << '\n';

      int pattern[1023];
      {
        for (int i = 2; i < 1024; i++) {
          pattern[i] = i - 1;
        }
        pattern[0] = 0;
        pattern[1] = 0;
      }

      std::ofstream file;
      if (enableFileOutput) { file.open("dma_data.txt"); }

      int i = 0;
      int lock = 0;
      int counter = 0;
      int currentPage = 0;
      int errorCount = 0;
      int rolling = 0;
      int statusCount = 0;

      // Allocating memory where the data will be read out to
      using PageBuffer = std::array<uint32_t, DMA_PAGE_SIZE_32>; ///< Array the size of a page
      std::array<PageBuffer, FIFO_ENTRIES * NUM_OF_BUFFERS> readoutPages; ///< Array of pages

      if (isVerbose()) {
        cout << '\n' << b::str(b::format(PROGRESS_FORMAT) % "i" % "Counter" % "Rolling" % "Status" % "Errors") << '\n';
      }

      auto timeStart = std::chrono::high_resolution_clock::now();

      while (true) {
        if (isSigInt()) {
          break;
        }

        // LEDs off
        bar[Register::LED_STATUS] = 0xff;

        if (fifoUser->statusEntries[i].isPageArrived()) {
          auto& readoutPage = readoutPages[i];

          memcpy(readoutPage.data(), &dataUser[i * DMA_PAGE_SIZE_32], DMA_PAGE_SIZE);

          if (enableFileOutput) { file << i << '\n'; }

          for (int j = 0; j < DMA_PAGE_SIZE / 32; j++) {
            if (enableFileOutput) {
              for (int k = 7; k >= 0; k--) {
                file << readoutPage[j * 8 + k];
              }
              file << '\n';
            }

            // Data error checking
            size_t pattern_index = currentPage * DMA_PAGE_SIZE / 32 + j;
            if (readoutPage[j * 8] != pattern[pattern_index]) {
              errorCount++;
              if (isVerbose()) {
                cout << "data error at rolling " << rolling
                    << ", page " << i
                    << ", data " << j * 8
                    << ", " << readoutPage[j * 8]
                    << " - " << pattern[pattern_index] << '\n';
              }
            }
          }

          if (enableFileOutput) { file << '\n'; }

          currentPage++;

          // Setting the buffer to the default value after the readout
          for (size_t j = 0; j < DMA_PAGE_SIZE_32; j++) {
            dataUser[i * DMA_PAGE_SIZE_32 + j] = DEFAULT_VALUE;
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

            currentPage = 0;

            //counter = 0;

            rolling++;

            if (isVerbose()) {
              cout << b::str(b::format(PROGRESS_FORMAT) % i % (counter - 1) % rolling % statusCount % errorCount)
                  << '\r';
            }

            // Only if we want a fixed number of rolling
            if (rolling == maxRolls) {
              cout << "\n\nrolling == " << maxRolls << " -> stopping\n";
              break;
            }
          }

          if (i == FIFO_ENTRIES * NUM_OF_BUFFERS - 1) {
            // read status count after 1MByte dma transfer
            statusCount = bar[Register::READ_STATUS_COUNT];
            i = 0;
          } else {
            i++;
          }
        }
      }

      auto timeEnd = std::chrono::high_resolution_clock::now();

      if (isSigInt()) {
        cout << "Interrupted\n";
      }

      // Calculating throughput
      double runTime = std::chrono::duration<double>(timeEnd - timeStart).count();
      double GiB = double(rolling) * FIFO_ENTRIES * DMA_PAGE_SIZE / (1024 * 1024 * 1024);
      double throughputGiBs = GiB / runTime;
      double throughputGibs = throughputGibs * 8.0;

      auto format = "  %-10s  %-10s\n";
      cout << '\n';
      cout << b::str(b::format(format) % "Seconds" % runTime);
      cout << b::str(b::format(format) % "GiB" % GiB);
      cout << b::str(b::format(format) % "GiB/s" % throughputGiBs);
      cout << b::str(b::format(format) % "Gib/s" % throughputGibs);
      cout << b::str(b::format(format) % "Errors" % errorCount);
      cout << '\n';
      return 0;
    }
};

} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramCruExperimentalDma().execute(argc, argv);
}
