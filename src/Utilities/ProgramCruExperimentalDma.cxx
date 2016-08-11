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
#include <sstream>
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

constexpr int NUM_PAGES = FIFO_ENTRIES * NUM_OF_BUFFERS;

//constexpr int MAX_ROLLING = 1500;

/// Offset in bytes from start of status table to data buffer
constexpr size_t DATA_OFFSET = 0x200;

constexpr size_t DMA_BUFFER_PAGES_SIZE = 2l * 1024l * 1024l; // One 2MiB hugepage. Should be enough...

constexpr uint32_t BUFFER_DEFAULT_VALUE = 0xCcccCccc;

/// PDA DMA buffer index for the pages buffer
constexpr int BUFFER_INDEX_PAGES = 0;

const bfs::path DMA_BUFFER_PAGES_PATH = "/mnt/hugetlbfs/rorc-cru-experimental-dma-pages";

const std::string PROGRESS_FORMAT = "  %-5s %-8s  %-8s  %-8s %-8s";

const std::string RESET_SWITCH = "reset";
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
      options.add_options()(RESET_SWITCH.c_str(), "Reset card");
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

      enableResetCard = bool(map.count(RESET_SWITCH));
      enableFileOutput = bool(map.count(TO_FILE_SWITCH));

      cout << "Starting DMA test" << endl;
      pdaDMA();
      cout << "Finished DMA test" << endl;
    }

    int maxRolls;
    bool enableFileOutput;
    bool enableResetCard;

    boost::scoped_ptr<RorcDevice> rorcDevice;
    boost::scoped_ptr<Pda::PdaBar> pdaBar;
    boost::scoped_ptr<MemoryMappedFile> mappedFilePages;
    boost::scoped_ptr<Pda::PdaDmaBuffer> bufferPages;

    volatile uint32_t* bar;

#define OLD_INITPDA_IMPL
#ifdef OLD_INITPDA_IMPL
    descriptor_entry *descriptor;
    descriptor_entry *descriptor_usr;
    uint32_t* status_usr;
#else
    CruFifoTable* fifoUser;
#endif

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
      Util::resetSmartPtr(bufferPages, rorcDevice->getPciDevice(), mappedFilePages->getAddress(),
          mappedFilePages->getSize(), BUFFER_INDEX_PAGES);

      auto entry = bufferPages->getScatterGatherList().at(0);
      if (entry.size < (2l * 1024l * 1024l)) {
        BOOST_THROW_EXCEPTION(CruException()
            << errinfo_rorc_error_message("Unsupported configuration: DMA scatter-gather entry size less than 2 MiB")
            << errinfo_rorc_scatter_gather_entry_size(entry.size)
            << errinfo_rorc_possible_causes(
                {"DMA buffer was not allocated in hugepage shared memory (hugetlbfs may not be properly mounted)"}));
      }

      if (enableResetCard) {
        cout << "Resetting..." << std::flush;
        bar[Register::RESET_CONTROL] = 0x1;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cout << "done!" << endl;
      }

#ifdef OLD_INITPDA_IMPL

      /** IOMMU addresses */
      uint32_t *status = (uint32_t*)entry.addressBus;
      /* the start address of the descriptor table is always at the same place:
         adding a 0x200 offset (=status+128) to the status start address, no
         matter how many entries there are
      */
      descriptor = (descriptor_entry*)(status + 128);
      uint32_t* data = (uint32_t*)(descriptor + FIFO_ENTRIES*NUM_OF_BUFFERS);

      /** Virtual addresses */
      status_usr = (uint32_t*) entry.addressUser;
      descriptor_usr = (descriptor_entry*)(status_usr + 128);
      dataUser = (uint32_t*)(descriptor_usr + FIFO_ENTRIES*NUM_OF_BUFFERS);

      /** Initializing statuses with 0 */
      for(int i = 0; i < FIFO_ENTRIES*NUM_OF_BUFFERS; i++){
        status_usr[i] = 0;
      }

      /** Initializing the descriptor table */
      for(int i = 0; i < FIFO_ENTRIES*NUM_OF_BUFFERS; i++){
        uint32_t ctrl = 0;
        uint32_t temp_ctrl = 0;
        uint32_t temp_i = i;
        temp_ctrl = temp_i << 18;
        ctrl += temp_ctrl;
        ctrl += DMA_PAGE_SIZE/4;
        descriptor_usr[i].ctrl = ctrl;
        /** Adresses in the card's memory (DMA source) */
        descriptor_usr[i].src_low = (i%NUM_OF_BUFFERS)*DMA_PAGE_SIZE;
        descriptor_usr[i].src_high = 0x0;
        /** Addresses in the RAM (DMA destination) */
        descriptor_usr[i].dst_low = (uint64_t)(data+i*DMA_PAGE_SIZE/sizeof(uint32_t)) & 0xffffffff;
        descriptor_usr[i].dst_high = (uint64_t)(data+i*DMA_PAGE_SIZE/sizeof(uint32_t)) >> 32;
        /*fill the reserved bit with zero*/
        descriptor_usr[i].reservd1 = 0x0;
        descriptor_usr[i].reservd2 = 0x0;
        descriptor_usr[i].reservd3 = 0x0;
      }

    /** Setting the buffer to the default value of 0xcccccccc */
    for(int i = 0; i < DMA_PAGE_SIZE/sizeof(uint32_t)*FIFO_ENTRIES*NUM_OF_BUFFERS; i++){
      *(dataUser + i) = 0xcccccccc;
    }
    usleep(1000);


    /** Status base address, low and high part, respectively */
    *bar = (uint64_t)status & 0xffffffff;
    *(bar+1) = (uint64_t)status >> 32;

    /** Destination (card's memory) addresses, low and high part, respectively */
    *(bar+2) = 0x8000;
    *(bar+3) = 0x0;

    /** Set descriptor table size, same as number of available pages-1 */
    *(bar+5) = FIFO_ENTRIES*NUM_OF_BUFFERS-1;

    /** Timestamp for test */
    //    clock_gettime(CLOCK_REALTIME, &t1);

    /** Set status to send status for every page not only for the last one */
    *(bar+6) = 0x1;
    //usleep(100);

    /** Number of available pages-1 */
    *(bar+4) = FIFO_ENTRIES*NUM_OF_BUFFERS-1;

    //    sleep(1);

    /* make buffer ready signal*/
    *(bar+129) = 0x1;
    sleep(1);

    /** Programming the user module to trigger the data emulator */
    *(bar+128) = 0x1;
    sleep(1);



#else

//#  define THIS_SHOULD_WORK
#  ifndef THIS_SHOULD_WORK
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
        dataUser[i] = BUFFER_DEFAULT_VALUE;
      }

      // Status base address, low and high part, respectively
      CruFifoTable* fifoDevice = reinterpret_cast<CruFifoTable*>(entry.addressBus);

#  else
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
        dataUser[i] = BUFFER_DEFAULT_VALUE;
      }
#  endif

      auto prnBar = [&](std::string label, size_t address) {
        cout << label << Common::makeRegisterString(address, bar[address]);
      };

      // Status base address, low and high part, respectively
      bar[Register::STATUS_BASE_BUS_HIGH] = Util::getUpper32Bits(uint64_t(fifoDevice));
      bar[Register::STATUS_BASE_BUS_LOW] = Util::getLower32Bits(uint64_t(fifoDevice));
      prnBar("STATUS BUS LO   ", Register::STATUS_BASE_BUS_LOW);
      prnBar("STATUS BUS HI   ", Register::STATUS_BASE_BUS_HIGH);

      // Destination (card's memory) addresses, low and high part, respectively
      bar[Register::STATUS_BASE_CARD_HIGH] = 0x0;
      bar[Register::STATUS_BASE_CARD_LOW] = 0x8000;
      prnBar("STATUS CARD LO  ", Register::STATUS_BASE_CARD_LOW);
      prnBar("STATUS CARD HI  ", Register::STATUS_BASE_CARD_HIGH);

      // Set descriptor table size, same as number of available pages-1
      bar[Register::DESCRIPTOR_TABLE_SIZE] = NUM_PAGES - 1;
      prnBar("DSCR TBL SIZE   ", Register::DESCRIPTOR_TABLE_SIZE);

      // Number of available pages-1
      bar[Register::DMA_POINTER] = NUM_PAGES - 1;
      prnBar("DMA POINTER     ", Register::DMA_POINTER);

      // make buffer ready signal
      bar[Register::BUFFER_READY] = 0x1;

      // Programming the user module to trigger the data emulator
      bar[Register::DATA_EMULATOR_ENABLE] = 0x1;

      // Set status to send status for every page not only for the last one
      bar[Register::DONE_CONTROL] = 0x1;

#endif

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

      std::ofstream fileStream;
      std::ostringstream errorStream;

      if (enableFileOutput) {
        fileStream.open("dma_data.txt");
      }

      int i = 0;
      int lock = 0;
      int counter = 0;
      int currentPage = 0;
      int errorCount = 0;
      int rolling = 0;
      int statusCount = 0;

      // Allocating memory where the data will be read out to
      using PageBuffer = std::array<uint32_t, DMA_PAGE_SIZE_32>; ///< Array the size of a page
      std::array<PageBuffer, NUM_PAGES> readoutPages; ///< Array of pages

      if (isVerbose()) {
        cout << '\n' << b::str(b::format(PROGRESS_FORMAT) % "i" % "Counter" % "Rolling" % "Status" % "Errors") << '\n'
        << b::str(b::format(PROGRESS_FORMAT) % '-' % '-' % '-' % '-' % '-') << std::flush;
      }

      auto timeStart = std::chrono::high_resolution_clock::now();

      while (true) {
        if (isSigInt()) {
          break;
        }

        // Test waiting
//        {
//          int w = 0;
//          while(not fifoUser->statusEntries[i].isPageArrived()) {
//            if (isSigInt()) { break; }
//
//            cout << "Waiting (" << w << ") ";
//
//            for (size_t j = 0; j < fifoUser->statusEntries.size(); ++j) {
//              if (j != 0 && ((j % 8) == 0)) {
//                cout << '.';
//              }
//              cout << (fifoUser->statusEntries[j].isPageArrived() ? 'X' : ' ');
//            }
//            cout << '\r';
//            std::this_thread::sleep_for(std::chrono::milliseconds(1));
//            w++;
//          }
//          cout << '\n';
//        }

#ifdef OLD_INITPDA_IMPL
        if (status_usr[i] == 1) {
#else
        if (fifoUser->statusEntries[i].isPageArrived()) {
#endif
          auto& readoutPage = readoutPages[i];

          memcpy(readoutPage.data(), &dataUser[i * DMA_PAGE_SIZE_32], DMA_PAGE_SIZE);

          // File output
          if (enableFileOutput) {
            fileStream << i << '\n';
            for (int j = 0; j < DMA_PAGE_SIZE / 32; j++) {
              if (enableFileOutput) {
                for (int k = 7; k >= 0; k--) {
                  fileStream << readoutPage[j * 8 + k];
                }
                fileStream << '\n';
              }
            }
            fileStream << '\n';
          }

          // Data error checking
          for (int j = 0; j < DMA_PAGE_SIZE / 32; j++) {
            size_t pattern_index = currentPage * DMA_PAGE_SIZE / 32 + j;
            if (readoutPage[j * 8] != pattern[pattern_index]) {
              errorCount++;
              if (isVerbose()) {
                errorStream << "data error at rolling " << rolling
                    << ", page " << i
                    << ", data " << j * 8
                    << ", " << readoutPage[j * 8]
                    << " - " << pattern[pattern_index] << '\n';
              }
            }
          }

          currentPage++;

          // Setting the buffer to the default value after the readout
          for (size_t j = 0; j < DMA_PAGE_SIZE_32; j++) {
            dataUser[i * DMA_PAGE_SIZE_32 + j] = BUFFER_DEFAULT_VALUE;
          }

          //cout << " " << i << " " << fifoUser->statusEntries[i].status << '\n';

          //generates lock state for if l==0 case
          if (i == 1) {
            lock = 1;
          }

          //make status zero for each descriptor for next rolling
#ifdef OLD_INITPDA_IMPL
          status_usr[i] = 0;
#else
          fifoUser->statusEntries[i].reset();
#endif

          //points to the first descriptor
          if (lock == 0) {
            bar[Register::DMA_POINTER] = NUM_PAGES - 1;
          }

          //push the same descriptor again when its gets executed
          bar[Register::DMA_POINTER] = i;

          counter++;

          //after every 32Kbytes reset the current_page for online checking
          if ((i % FIFO_ENTRIES) == FIFO_ENTRIES - 1) {

            currentPage = 0;

            //counter = 0;

            rolling++;

            if (isVerbose()) {
              cout << '\r'
                  << b::str(b::format(PROGRESS_FORMAT) % i % (counter - 1) % rolling % statusCount % errorCount);
            }

            // Only if we want a fixed number of rolling
            if (rolling == maxRolls) {
              cout << "\n\nrolling == " << maxRolls << " -> stopping\n";
              break;
            }
          }

          if (i == NUM_PAGES - 1) {
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
        cout << "\n\nInterrupted\n";
      }

      if (isVerbose()) {
        cout << "Errors: " << errorStream.str() << '\n';
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
      if (GiB > 0.00001) {
        cout << b::str(b::format(format) % "GiB/s" % throughputGiBs);
        cout << b::str(b::format(format) % "Gibit/s" % throughputGibs);
        cout << b::str(b::format(format) % "Errors" % errorCount);
      }
      cout << '\n';
      return 0;
    }
};

} // Anonymous namespace

int main(int argc, char** argv)
{
  return ProgramCruExperimentalDma().execute(argc, argv);
}
