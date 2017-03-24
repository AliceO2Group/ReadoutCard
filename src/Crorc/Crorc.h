/// \file Crorc.h
/// \brief Definition of low level C-RORC functions
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <atomic>
#include <iostream>
#include <string>
#include "RORC/ParameterTypes/GeneratorPattern.h"
#include "RORC/RegisterReadWriteInterface.h"
#include "RxFreeFifoState.h"
#include "c/rorc/stword.h"

namespace AliceO2 {
namespace Rorc {
namespace Crorc {

/// Retrieve the serial number from the C-RORC's flash memory.
/// Note that the BAR must be from channel 0, other channels do not have access to the flash.
int getSerial(RegisterReadWriteInterface& bar0);

/// Program flash using given data file
void programFlash(RegisterReadWriteInterface& bar0, std::string dataFilePath, int addressFlash, std::ostream& out,
    const std::atomic<bool>* interrupt = nullptr);

/// Read flash range
void readFlashRange(RegisterReadWriteInterface& bar0, int addressFlash, int wordNumber, std::ostream& out);

class Crorc
{
  public:
    Crorc(RegisterReadWriteInterface& bar)
        : bar(bar)
    {
    }

    struct DiuConfig
    {
        long long int loopPerUsec = 0;
        double pciLoopPerUsec = 0;
    };

    /// Sends reset command
    void resetCommand(int option, const DiuConfig& diuConfig);

    /// Arms DDL
    /// \param resetMask The reset mask. See the RORC_RESET_* macros in rorc.h
    void armDdl(int resetMask, const DiuConfig& diuConfig);

    /// Arms C-RORC data generator
    int armDataGenerator(uint32_t initEventNumber, uint32_t initDataWord, GeneratorPattern::type dataPattern,
        int dataSize, int seed);

    void startDataGenerator(uint32_t maxLoop);

    /// Stops C-RORC data generator
    void stopDataGenerator();

    /// Starts data receiving
    void startDataReceiver(uintptr_t readyFifoBusAddress);

    /// Stops C-RORC data receiver
    void stopDataReceiver();

    /// Find and store DIU version
    DiuConfig initDiuVersion();

    /// Checks if link is up
    bool isLinkUp();

    /// Checks if link is up
    void assertLinkUp();

    /// Send a command to the SIU
    /// \param command The command to send to the SIU. These are probably the macros under 'interface commands' in
    ///   the header ddl_def.h
    void siuCommand(int command, const DiuConfig& diuConfig);

    /// Send a command to the DIU
    /// \param command The command to send to the SIU. These are probably the macros under 'interface commands' in
    ///   the header ddl_def.h
    void diuCommand(int command, const DiuConfig& diuConfig);

    /// Checks if the C-RORC's Free FIFO is empty
    bool isFreeFifoEmpty();

    /// Checks if the C-RORC's Free FIFO is empty
    void assertFreeFifoEmpty();

    /// Starts the trigger
    void startTrigger(const DiuConfig& diuConfig);

    /// Stops the trigger
    void stopTrigger(const DiuConfig& diuConfig);

    /// Set SIU loopback
    void setSiuLoopback(const DiuConfig& diuConfig);

    /// Not sure
    void setLoopbackOn();

    /// Not sure
    void setLoopbackOff();

    /// Not sure
    bool isLoopbackOn();

    /// Not sure
    void toggleLoopback();

    /// Not sure
    uint32_t checkCommandRegister();

    /// Not sure
    void putCommandRegister(uint32_t command);

    /// Not sure
    uint32_t checkRxStatus();

    /// Not sure
    uint32_t checkRxData();

    void pushRxFreeFifo(uintptr_t blockAddress, uint32_t blockLength, uint32_t readyFifoIndex);

    RxFreeFifoState getRxFreeFifoState();

    /// Set C-RORC for continuous readout
    static void initReadoutContinuous(RegisterReadWriteInterface& bar2);

    /// Enable (or re-enable) continuous readout
    static void startReadoutContinuous(RegisterReadWriteInterface& bar2);

    /// Set C-RORC for triggered readout
    static void initReadoutTriggered(RegisterReadWriteInterface& bar2);

  private:
    RegisterReadWriteInterface& bar;

    bool arch64()
    {
      return (sizeof(void *) > 4);
    }

    uint32_t read(uint32_t index) {
      return bar.readRegister(index);
    }

    void write(uint32_t index, uint32_t value) {
      bar.writeRegister(index, value);
    }

    void ddlResetSiu(int print, int cycle, long long int time, const DiuConfig& diuConfig);
    void ddlSendCommand(int dest, uint32_t command, int transid, uint32_t param, long long int time);
    void ddlWaitStatus(long long int timeout);
    stword_t ddlReadStatus();
    stword_t ddlReadDiu(int transid, long long int time);
    stword_t ddlReadSiu(int transid, long long int time);
    stword_t ddlReadCTSTW(int transid, int destination, long long int time);
//    void ddlLinkUp(int master, int print, int stop, long long int time, const DiuConfig& diuConfig);
//    void ddlLinkUp_NEW(int master, int print, int stop, long long int time, const DiuConfig& diuConfig);
    void emptyDataFifos(int timeoutMicroseconds);
    stword_t ddlSetSiuLoopBack(const DiuConfig& diuConfig);

    void ddlInterpretIFSTW(uint32_t ifstw, const char *pref, const char *suff);
};

} // namespace Crorc
} // namespace Rorc
} // namespace AliceO2
