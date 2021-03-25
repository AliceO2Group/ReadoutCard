// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Parameters.h
/// \brief Definition of the RORC Parameters and associated functions
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef O2_READOUTCARD_INCLUDE_PARAMETERS_H_
#define O2_READOUTCARD_INCLUDE_PARAMETERS_H_

#include "ReadoutCard/NamespaceAlias.h"
#include <map>
#include <memory>
#include <set>
#include <string>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include "ReadoutCard/ParameterTypes/BufferParameters.h"
#include "ReadoutCard/ParameterTypes/DataSource.h"
#include "ReadoutCard/ParameterTypes/PciAddress.h"
#include "ReadoutCard/ParameterTypes/PciSequenceNumber.h"
#include "ReadoutCard/ParameterTypes/SerialId.h"
#include "ReadoutCard/ParameterTypes/Hex.h"

// CRU Specific
#include "ReadoutCard/ParameterTypes/Clock.h"
#include "ReadoutCard/ParameterTypes/DatapathMode.h"
#include "ReadoutCard/ParameterTypes/DownstreamData.h"
#include "ReadoutCard/ParameterTypes/GbtMode.h"
#include "ReadoutCard/ParameterTypes/GbtMux.h"

namespace o2
{
namespace roc
{

struct ParametersPimpl;

/// Class that holds parameters for channels
/// Per parameter, it has three functions:
/// * Setter
/// * Non-throwing getter that returns the value wrapped in an optional if it is present, or an empty optional if it
///   was not
/// * Throwing getter that returns the value if it is present, or throws a ParameterException
class Parameters
{
 public:
  Parameters();
  ~Parameters();
  Parameters(const Parameters& other);
  Parameters(Parameters&& other);
  Parameters& operator=(const Parameters& other);
  Parameters& operator=(Parameters&& other);

  // Types for parameter values

  /// Type for buffer parameters. It can hold Memory, File or Null buffer parameters.
  using BufferParametersType = boost::variant<buffer_parameters::Memory, buffer_parameters::File,
                                              buffer_parameters::Null>;

  /// Type for the CardId parameter. It can hold a SerialId, a PciAddress or a PciSequenceNumber.
  using CardIdType = boost::variant<::o2::roc::PciAddress, ::o2::roc::PciSequenceNumber, ::o2::roc::SerialId>;

  /// Type for the ChannelNumber parameter
  using ChannelNumberType = int32_t;

  /// Type for the DMA page size parameter
  using DmaPageSizeType = size_t;

  /// Type for the DataSource parameter
  using DataSourceType = DataSource::type;

  /// Type for the link mask parameter
  using LinkMaskType = std::set<uint32_t>;

  /// Type for the gbt mux map parameter
  using GbtMuxMapType = std::map<uint32_t, GbtMux::type>;

  /// Type for the allow rejection parameter
  using AllowRejectionType = bool;

  /// Type for the clock parameter
  using ClockType = Clock::type;

  /// Type for the CRORC ID
  using CrorcIdType = Hex::type;

  /// Type for the CRU ID
  using CruIdType = Hex::type;

  /// Type for the datapath mode parameter
  using DatapathModeType = DatapathMode::type;

  /// Type for the downstream data parameter
  using DownstreamDataType = DownstreamData::type;

  /// Type for the gbt mux parameter
  using GbtMuxType = GbtMux::type;

  /// Type for the gbt mode parameter
  using GbtModeType = GbtMode::type;

  /// Type for the link loopback enabled parameter
  using LinkLoopbackEnabledType = bool;

  /// Type for the PON upstream enabled parameter
  using PonUpstreamEnabledType = bool;

  /// Type for the Dynamic Offset enabled parameter
  using DynamicOffsetEnabledType = bool;

  /// Type for the User Logic enabled parameter
  using UserLogicEnabledType = bool;

  /// Type for the Run Statistics enabled parameter
  using RunStatsEnabledType = bool;

  /// Type for the User and Common Logic enabled parameter
  using UserAndCommonLogicEnabledType = bool;

  /// Type for the GBT enabled parameter
  using GbtEnabledType = bool;

  /// Type for the GBT enabled parameter
  using FirmwareCheckEnabledType = bool;

  /// Type for the ONU address parameter
  using OnuAddressType = Hex::type;

  /// Type for the STBRD enabled parameter
  using StbrdEnabledType = bool;

  // Type for the Trigger Window Size parameter
  using TriggerWindowSizeType = uint32_t;

  // Type for the Time Frame Length parameter
  using TimeFrameLengthType = uint32_t;

  /// Type for the Time Frame Detection enabled parameter
  using TimeFrameDetectionEnabledType = bool;

  /// Type for the System ID parameter
  using SystemIdType = uint32_t;

  /// Type for the FEE ID parameter
  using FeeIdType = uint32_t;

  /// Type for the FEE ID map parameter
  using FeeIdMapType = std::map<uint32_t, uint32_t>;

  // Setters

  /// Sets the CardId parameter
  ///
  /// This can be either a PciAddress, or an integer representing a serial number.
  /// It may be -1 to instantiate the dummy driver.
  ///
  /// Required parameter.
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setCardId(CardIdType value) -> Parameters&;

  /// Sets the ChannelNumber parameter
  ///
  /// This indicates which DMA channel should be opened.
  /// * The C-RORC has 6 available channels (numbers 0 to 5).
  /// * The CRU has one (number 0).
  ///
  /// Required parameter.
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setChannelNumber(ChannelNumberType value) -> Parameters&;

  /// Sets the DmaPageSize parameter
  ///
  /// Supported values:
  /// * C-RORC: ??? (it seems to be very flexible)
  /// * CRU: 8 KiB
  ///
  /// If not set, the card's driver will select a sensible default
  ///
  /// NOTE: Will probably be removed. In which case for the C-RORC this will be set per superpage to the superpage
  ///   size. For the CRU, it is non-configurable anyway.
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setDmaPageSize(DmaPageSizeType value) -> Parameters&;

  /// Sets the LinkLoopbackEnabled parameter
  ///
  /// If enabled the link is on loopback mode  enabling the use of the DDG.
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setLinkLoopbackEnabled(LinkLoopbackEnabledType value) -> Parameters&;

  /// Sets the PonUpstreamEnabled parameter
  ///
  /// If enabled the PON upstream is used.
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setPonUpstreamEnabled(PonUpstreamEnabledType value) -> Parameters&;

  /// Sets the DynamicOffsetEnabled parameter
  ///
  /// If enabled the Dynamic Offset is used.
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setDynamicOffsetEnabled(DynamicOffsetEnabledType value) -> Parameters&;

  /// Sets the GbtEnabled parameter
  ///
  /// If enabled the GBT is used.
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setGbtEnabled(GbtEnabledType value) -> Parameters&;

  /// Sets the FirmwareCheckEnabled parameter
  ///
  /// If enabled the Firmware check is enforced.
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setFirmwareCheckEnabled(FirmwareCheckEnabledType value) -> Parameters&;

  /// Sets the UserLogicEnabled parameter
  ///
  /// If enabled the User Logic is used.
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setUserLogicEnabled(UserLogicEnabledType value) -> Parameters&;

  /// Sets the RunStatsEnabled parameter
  ///
  /// If enabled the Run Statistics link is enabled
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setRunStatsEnabled(RunStatsEnabledType value) -> Parameters&;

  /// Sets the UserAndCommonLogicEnabled parameter
  ///
  /// If enabled both User and Common Logic is enabled
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setUserAndCommonLogicEnabled(UserAndCommonLogicEnabledType value) -> Parameters&;

  /// Sets the OnuAddress parameter
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setOnuAddress(OnuAddressType value) -> Parameters&;

  /// Sets the DataSource parameter
  ///
  /// Controls the routing of the generated data.
  /// Supported data source modes:
  /// * C-RORC: all modes
  /// * CRU: internal, fee
  ///
  /// If not set, the driver will default to internal loopback.
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setDataSource(DataSourceType value) -> Parameters&;

  /// Sets the BufferParameters parameter
  ///
  /// Registers a memory (with BufferParameters::Memory) or file (with BufferParameters::File) buffer with the
  /// DMA channel.
  ///
  /// Note that if the IOMMU is not enabled, the buffer may not be presented as a contiguous physical space to the
  /// readout card. In this case, the user is responsible for ensuring that superpages given to the driver are
  /// physically contiguous.
  ///
  /// It is recommended to use hugepages for the buffer to increase contiguousness, for example by opening a
  /// MemoryMappedFile in a hugetlbfs filesystem.
  /// See the README.md file for more information about hugepages.
  ///
  /// There is also a BufferParameters::Null option, which can be used to instantiate the DmaChannel without
  /// initiating data transfer, e.g. for testing purposes.
  ///
  /// Required parameter for the C-RORC and CRU drivers.
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setBufferParameters(BufferParametersType value) -> Parameters&;

  /// Sets the LinkMask parameter
  ///
  /// The BAR channel may transfer data from multiple links.
  /// When this parameter is set, the links corresponding to the given number are enabled.
  /// When this parameter is not set, ??? links will be enabled (none? one? to be determined...)
  ///
  /// When an invalid link is given, the DMA channel may throw an InvalidLinkId exception.
  ///
  /// Note: the linkMaskFromString() function may be used to convert a std::string to a LinkMaskType that can be
  /// passed to this setter, which may be convenient when converting from string-based configuration values.
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setLinkMask(LinkMaskType value) -> Parameters&;

  /// Sets the AllowRejection parameter
  ///
  /// If enabled the PON upstream is used.
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setAllowRejection(AllowRejectionType value) -> Parameters&;

  /// Sets the Clock Parameter
  ///
  /// The Clock parameter refers to the selection of the TTC or Local clock for the CRU configuration
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setClock(ClockType value) -> Parameters&;

  /// Sets the CrorcId parameter
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setCrorcId(CrorcIdType value) -> Parameters&;

  /// Sets the CruId parameter
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setCruId(CruIdType value) -> Parameters&;

  /// Sets the DatapathMode Parameter
  ///
  /// The Datapath Mode parameter refers to the selection of the Datapath Mode for the CRU configuration
  /// The Datapath Mode may be PACKET or STREAMING
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setDatapathMode(DatapathModeType value) -> Parameters&;

  /// Sets the DownstreamData Parameter
  ///
  /// The Downstream Data parameter refers to the selection of the Downstream Data for the CRU configuration
  /// The Downstream Data may be CTP, PATTERN or MIDTRG
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setDownstreamData(DownstreamDataType value) -> Parameters&;

  /// Sets the GbtMode Parameter
  ///
  /// The GBT Mode parameter refers to the selection of the GBT Mode for the CRU configuration
  /// The GBT Mode may be GBT or WB
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setGbtMode(GbtModeType value) -> Parameters&;

  /// Sets the GbtMux Parameter
  ///
  /// The GBT Mux parameter refers to the selection of the GBT Mux for the CRU configuration
  /// The GBT Mux may be TTC, DDG or SC
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setGbtMux(GbtMuxType value) -> Parameters&;

  /// Sets the GbtMuxMap Parameter
  ///
  /// The Gbt Mux Map parameter refers to the mapping of GBT Mux selection per link
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setGbtMuxMap(GbtMuxMapType value) -> Parameters&;

  /// Sets the StbrdEnabled parameter
  ///
  /// If enabled the STBRD command is used to start the CRORC trigger.
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setStbrdEnabled(StbrdEnabledType value) -> Parameters&;

  /// Sets the TriggerWindowSize parameter
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setTriggerWindowSize(TriggerWindowSizeType value) -> Parameters&;

  /// Sets the TimeFrameLength parameter
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setTimeFrameLength(TimeFrameLengthType value) -> Parameters&;

  /// Sets the TimeFrameDetectionEnabled parameter
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setTimeFrameDetectionEnabled(TimeFrameDetectionEnabledType value) -> Parameters&;

  /// Sets the SystemId parameter
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setSystemId(SystemIdType value) -> Parameters&;

  /// Sets the FeeId parameter
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setFeeId(FeeIdType value) -> Parameters&;

  /// Sets the FeeIdMap parameter
  ///
  /// \param value The value to set
  /// \return Reference to this object for chaining calls
  auto setFeeIdMap(FeeIdMapType value) -> Parameters&;

  // non-throwing getters

  /// Gets the AllowRejection parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getAllowRejection() const -> boost::optional<AllowRejectionType>;

  /// Gets the CardId parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getCardId() const -> boost::optional<CardIdType>;

  /// Gets the CrorcId parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getCrorcId() const -> boost::optional<CrorcIdType>;

  /// Gets the CruId parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getCruId() const -> boost::optional<CruIdType>;

  /// Gets the ChannelNumber parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getChannelNumber() const -> boost::optional<ChannelNumberType>;

  /// Gets the DmaPageSize parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getDmaPageSize() const -> boost::optional<DmaPageSizeType>;

  /// Gets the LinkLoopbackEnabled parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getLinkLoopbackEnabled() const -> boost::optional<LinkLoopbackEnabledType>;

  /// Gets the PonUpstreamEnabled parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getPonUpstreamEnabled() const -> boost::optional<PonUpstreamEnabledType>;

  /// Gets the DynamicOffsetEnabled parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getDynamicOffsetEnabled() const -> boost::optional<DynamicOffsetEnabledType>;

  /// Gets the GbtEnabled parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getGbtEnabled() const -> boost::optional<GbtEnabledType>;

  /// Gets the FirmwareCheckEnabled parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getFirmwareCheckEnabled() const -> boost::optional<FirmwareCheckEnabledType>;

  /// Gets the UserLogicEnabled parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getUserLogicEnabled() const -> boost::optional<UserLogicEnabledType>;

  /// Gets the RunStatsEnabled parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getRunStatsEnabled() const -> boost::optional<RunStatsEnabledType>;

  /// Gets the UserAndCommonLogicEnabled parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getUserAndCommonLogicEnabled() const -> boost::optional<UserAndCommonLogicEnabledType>;

  /// Gets the OnuAddress parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getOnuAddress() const -> boost::optional<OnuAddressType>;

  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getDataSource() const -> boost::optional<DataSourceType>;

  /// Gets the BufferParameters parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getBufferParameters() const -> boost::optional<BufferParametersType>;

  /// Gets the LinkMask parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getLinkMask() const -> boost::optional<LinkMaskType>;

  /// Gets the Clock Parameter
  /// \return The value
  auto getClock() const -> boost::optional<ClockType>;

  /// Gets the DatapathMode Parameter
  /// \return The value
  auto getDatapathMode() const -> boost::optional<DatapathModeType>;

  /// Gets the DownstreamData Parameter
  /// \return The value
  auto getDownstreamData() const -> boost::optional<DownstreamDataType>;

  /// Gets the GbtMode Parameter
  /// \return The value
  auto getGbtMode() const -> boost::optional<GbtModeType>;

  /// Gets the GbtMux Parameter
  /// \return The value
  auto getGbtMux() const -> boost::optional<GbtMuxType>;

  /// Gets the GbtMuxMap Parameter
  /// \return The value
  auto getGbtMuxMap() const -> boost::optional<GbtMuxMapType>;

  /// Gets the StbrdEnabled parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getStbrdEnabled() const -> boost::optional<StbrdEnabledType>;

  /// Gets the TriggerWindowSize parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getTriggerWindowSize() const -> boost::optional<TriggerWindowSizeType>;

  /// Gets the TimeFrameLength parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getTimeFrameLength() const -> boost::optional<TimeFrameLengthType>;

  /// Gets the TimeFrameDetectionEnabled parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getTimeFrameDetectionEnabled() const -> boost::optional<TimeFrameDetectionEnabledType>;

  /// Gets the SystemId parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getSystemId() const -> boost::optional<SystemIdType>;

  /// Gets the FeeId parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getFeeId() const -> boost::optional<FeeIdType>;

  /// Gets the FeeIdMap parameter
  /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
  auto getFeeIdMap() const -> boost::optional<FeeIdMapType>;

  // Throwing getters

  /// Gets the AllowRejection parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getAllowRejectionRequired() const -> AllowRejectionType;

  /// Gets the CardId parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getCardIdRequired() const -> CardIdType;

  /// Gets the ChannelNumber parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getChannelNumberRequired() const -> ChannelNumberType;

  /// Gets the DmaPageSize parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getDmaPageSizeRequired() const -> DmaPageSizeType;

  /// Gets the LinkLoopbackEnabled parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getLinkLoopbackEnabledRequired() const -> LinkLoopbackEnabledType;

  /// Gets the PonUpstreamEnabled parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getPonUpstreamEnabledRequired() const -> PonUpstreamEnabledType;

  /// Gets the DynamicOffsetEnabled parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getDynamicOffsetEnabledRequired() const -> DynamicOffsetEnabledType;

  /// Gets the GbtEnabled parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getGbtEnabledRequired() const -> GbtEnabledType;

  /// Gets the FirmwareCheckEnabled parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getFirmwareCheckEnabledRequired() const -> FirmwareCheckEnabledType;

  /// Gets the UserLogicEnabled parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getUserLogicEnabledRequired() const -> UserLogicEnabledType;

  /// Gets the RunStatsEnabled parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getRunStatsEnabledRequired() const -> RunStatsEnabledType;

  /// Gets the UserAndCommonLogicEnabled parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getUserAndCommonLogicEnabledRequired() const -> UserAndCommonLogicEnabledType;

  /// Gets the OnuAddress parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getOnuAddressRequired() const -> OnuAddressType;

  /// Gets the DataSource parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getDataSourceRequired() const -> DataSourceType;

  /// Gets the BufferParameters parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getBufferParametersRequired() const -> BufferParametersType;

  /// Gets the LinkMask parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getLinkMaskRequired() const -> LinkMaskType;

  /// Gets the Clock Parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getClockRequired() const -> ClockType;

  /// Gets the CrorcId parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getCrorcIdRequired() const -> CrorcIdType;

  /// Gets the CruId parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getCruIdRequired() const -> CruIdType;

  /// Gets the DatapathMode Parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getDatapathModeRequired() const -> DatapathModeType;

  /// Gets the DownstreamData Parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getDownstreamDataRequired() const -> DownstreamDataType;

  /// Gets the GbtMode Parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getGbtModeRequired() const -> GbtModeType;

  /// Gets the GbtMux Parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getGbtMuxRequired() const -> GbtMuxType;

  /// Gets the GbtMuxMap Parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getGbtMuxMapRequired() const -> GbtMuxMapType;

  /// Gets the StbrdEnabled parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getStbrdEnabledRequired() const -> StbrdEnabledType;

  /// Gets the TriggerWindowSize parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getTriggerWindowSizeRequired() const -> TriggerWindowSizeType;

  /// Gets the TimeFrameLength parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getTimeFrameLengthRequired() const -> TimeFrameLengthType;

  /// Gets the TimeFrameDetectionEnabled parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getTimeFrameDetectionEnabledRequired() const -> TimeFrameDetectionEnabledType;

  /// Gets the SystemId parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getSystemIdRequired() const -> SystemIdType;

  /// Gets the FeeId parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getFeeIdRequired() const -> FeeIdType;

  /// Gets the FeeIdMap parameter
  /// \exception ParameterException The parameter was not present
  /// \return The value
  auto getFeeIdMapRequired() const -> FeeIdMapType;

  // Helper functions

  /// Convenience function to make a Parameters object with card ID and channel number, since these are the most
  /// frequently used parameters
  static Parameters makeParameters(CardIdType cardId, ChannelNumberType channel = 0)
  {
    return Parameters().setCardId(cardId).setChannelNumber(channel);
  }

  /// Convert a string to a set of link IDs for the setLinkMask() function.
  /// Can contain comma separated integers or ranges. For example:
  /// * "0,1,2,8-10" for links 0, 1, 2, 8, 9 and 10
  /// * "0-19,21-23" for links 0 to 23 except 20
  /// \throw ParseException on failure to parse
  static LinkMaskType linkMaskFromString(const std::string& string);

  /// Convert a string to a CardIdType for the setCardId() function.
  /// Can contain an integer or PCI address. For example:
  /// * "12345"
  /// * "42:0.0"
  /// \throw ParseException on failure to parse
  /// \throw ParameterException on PciAddress numbers out of range
  static CardIdType cardIdFromString(const std::string& string);

 private:
  std::unique_ptr<ParametersPimpl> mPimpl;
};

} // namespace roc
} // namespace o2

#endif // O2_READOUTCARD_INCLUDE_PARAMETERS_H_
