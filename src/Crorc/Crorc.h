
// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
/// \file Crorc.h
/// \brief Definition of low level C-RORC functions
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_SRC_CRORC_CRORC_H_
#define O2_READOUTCARD_SRC_CRORC_CRORC_H_

#include <atomic>
#include <iostream>
#include <string>
#include <vector>
#include <boost/optional.hpp>
#include "ReadoutCard/RegisterReadWriteInterface.h"
#include "StWord.h"

namespace o2
{
namespace roc
{
namespace Crorc
{

/// Retrieve the serial number from the C-RORC's flash memory.
/// Note that the BAR must be from channel 0, other channels do not have access to the flash.
boost::optional<int32_t> getSerial(RegisterReadWriteInterface& bar0);

/// Set the serial number on the C-RORC's flash memory.
void setSerial(RegisterReadWriteInterface& bar0, int serial);

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

  struct DiuConfig {
    double pciLoopPerUsec = 0;
  };

  /// Sends reset command
  void resetCommand(int option, const DiuConfig& diuConfig);

  /// Arms DDL
  /// \param resetMask The reset mask. See the RORC_RESET_* macros in rorc.h
  /// \param diuConfig DIU configuration
  void armDdl(/*int resetMask, const DiuConfig& diuConfig*/);

  /// Arms C-RORC data generator
  int armDataGenerator(int dataSize,
                       uint32_t initEventNumber = 0,
                       uint32_t initDataWord = 0,
                       int dataPattern = 5,
                       int seed = 0);

  void startDataGenerator(uint32_t maxLoop = 0);

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

  /// Starts the trigger (RDYRX or STBRD)
  void startTrigger(const DiuConfig& diuConfig, uint32_t command);
  //void startTrigger(uint32_t command);

  /// Stops the trigger
  void stopTrigger(const DiuConfig& diuConfig);

  /// Set SIU loopback
  void setSiuLoopback(const DiuConfig& diuConfig);

  /// Set DIU loopback
  void setDiuLoopback(const DiuConfig& diuConfig);

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

  static void scaInit(RegisterReadWriteInterface& bar2);

  struct ScaWriteCommand {
    uint8_t command;
    uint8_t transaction;
    uint8_t channel;
  };

  struct ScaReadResult {
    uint32_t data;
    uint32_t command;
    uint32_t time;
  };

  static void scaWrite(RegisterReadWriteInterface& bar2, uint32_t command, uint32_t data);

  static ScaReadResult scaRead(RegisterReadWriteInterface& bar2);

  static ScaReadResult scaGpioWrite(RegisterReadWriteInterface& bar2, uint32_t data);

  /// Set C-RORC for triggered readout
  static void initReadoutTriggered(RegisterReadWriteInterface& bar2);

  /// Get SIU Status
  std::tuple<std::string, uint32_t> siuStatus();

  /// Interprest SIU status
  std::vector<std::string> ddlInterpretIfstw(uint32_t ifstw);

  StWord ddlReadDiu(int transid, long long int time);
  StWord ddlReadSiu(int transid, long long int time);

 private:
  RegisterReadWriteInterface& bar;

  bool arch64()
  {
    return (sizeof(void*) > 4);
  }

  uint32_t read(uint32_t index)
  {
    return bar.readRegister(index);
  }

  void write(uint32_t index, uint32_t value)
  {
    bar.writeRegister(index, value);
  }

  uint8_t ddlReadHw(int destination, int address, long long int time);
  std::string ddlGetHwInfo(int destination, long long int time);
  uint32_t ddlPrintStatus(int destination, int time);
  void ddlResetSiu(int cycle, long long int time);
  void ddlSendCommand(int dest, uint32_t command, int transid, uint32_t param, long long int time);
  long long int ddlWaitStatus(long long int timeout);
  StWord ddlReadStatus();
  StWord ddlReadCTSTW(int transid, int destination, long long int time);
  StWord ddlSetSiuLoopBack(const DiuConfig& diuConfig);
  StWord ddlSetDiuLoopBack(const DiuConfig& diuConfig);
  std::vector<std::string> ddlInterpretIFSTW(uint32_t ifstw);
};

} // namespace Crorc
} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_SRC_CRORC_CRORC_H_
