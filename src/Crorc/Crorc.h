/// \file Crorc.h
/// \brief Definition of low level C-RORC functions
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <atomic>
#include <iostream>
#include <string>
#include "ReadoutCard/ParameterTypes/GeneratorPattern.h"
#include "ReadoutCard/RegisterReadWriteInterface.h"
#include "RxFreeFifoState.h"
#include "StWord.h"

namespace AliceO2 {
namespace roc {
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
    void siuCommand(int command);

    /// Send a command to the DIU
    /// \param command The command to send to the SIU. These are probably the macros under 'interface commands' in
    ///   the header ddl_def.h
    void diuCommand(int command);

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

    static void scaInit(RegisterReadWriteInterface& bar2);

    struct ScaWriteCommand
    {
        uint8_t command;
        uint8_t transaction;
        uint8_t channel;
    };

    struct ScaReadResult
    {
        uint32_t data;
        uint32_t command;
        uint32_t time;
    };

    static void scaWrite(RegisterReadWriteInterface& bar2, uint32_t command, uint32_t data);

    static ScaReadResult scaRead(RegisterReadWriteInterface& bar2);

    static ScaReadResult scaGpioWrite(RegisterReadWriteInterface& bar2, uint32_t data);

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

    void ddlResetSiu(int cycle, long long int time);
    void ddlSendCommand(int dest, uint32_t command, int transid, uint32_t param, long long int time);
    void ddlWaitStatus(long long int timeout);
    StWord ddlReadStatus();
    StWord ddlReadDiu(int transid, long long int time);
    StWord ddlReadSiu(int transid, long long int time);
    StWord ddlReadCTSTW(int transid, int destination, long long int time);
    void emptyDataFifos(int timeoutMicroseconds);
    StWord ddlSetSiuLoopBack(const DiuConfig& diuConfig);
    void ddlInterpretIFSTW(uint32_t ifstw);
};

} // namespace Crorc
} // namespace roc
} // namespace AliceO2
