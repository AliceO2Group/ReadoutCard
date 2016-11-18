/// \file Exception.h
/// \brief Definition of the RORC exceptions and related functions.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <boost/exception/all.hpp>
#include <cstdint>
#include "RORC/PciAddress.h"
#include "RORC/PciId.h"
#include "RORC/CardType.h"
#include "RORC/ResetLevel.h"
#include "RORC/GeneratorPattern.h"
#include "RORC/LoopbackMode.h"

namespace AliceO2 {
namespace Rorc {

/// Helper macro for defining errinfo types for Boost exceptions
#define DEFINE_ERRINFO(name, type) \
  using errinfo_rorc_##name = boost::error_info<struct errinfo_rorc_##name##_, type>

// errinfo definitions
DEFINE_ERRINFO(bar_index, size_t);
DEFINE_ERRINFO(bar_size, size_t);
DEFINE_ERRINFO(card_type, CardType::type);
DEFINE_ERRINFO(channel_number, int);
DEFINE_ERRINFO(ddl_reset_mask, std::string);
DEFINE_ERRINFO(directory, std::string);
DEFINE_ERRINFO(diu_command, int);
DEFINE_ERRINFO(dma_buffer_pages, size_t);
DEFINE_ERRINFO(dma_buffer_size, size_t);
DEFINE_ERRINFO(dma_page_size, size_t);
DEFINE_ERRINFO(error_message, std::string);
DEFINE_ERRINFO(fifo_index, int);
DEFINE_ERRINFO(filename, std::string);
DEFINE_ERRINFO(filesize, size_t);
DEFINE_ERRINFO(filesystem_type, std::string);
DEFINE_ERRINFO(generator_event_length, size_t);
DEFINE_ERRINFO(generator_pattern, int);
DEFINE_ERRINFO(generator_seed, int);
DEFINE_ERRINFO(index, size_t);
DEFINE_ERRINFO(loopback_mode, LoopbackMode::type);
DEFINE_ERRINFO(named_mutex_name, std::string);
DEFINE_ERRINFO(page_index, int);
DEFINE_ERRINFO(pages, size_t);
DEFINE_ERRINFO(parameter_key, std::string);
DEFINE_ERRINFO(parameter_name, std::string);
DEFINE_ERRINFO(pci_address, PciAddress);
DEFINE_ERRINFO(pci_address_bus_number, int);
DEFINE_ERRINFO(pci_address_slot_number, int);
DEFINE_ERRINFO(pci_address_function_number, int);
DEFINE_ERRINFO(pci_device_index, int);
DEFINE_ERRINFO(pci_id, PciId);
DEFINE_ERRINFO(pci_ids, std::vector<PciId>);
DEFINE_ERRINFO(pda_status_code, int);
DEFINE_ERRINFO(possible_causes, std::vector<std::string>);
DEFINE_ERRINFO(range, size_t);
DEFINE_ERRINFO(readyfifo_length, int32_t);
DEFINE_ERRINFO(readyfifo_status, int);
DEFINE_ERRINFO(reset_level, ResetLevel::type);
DEFINE_ERRINFO(scatter_gather_entry_size, size_t);
DEFINE_ERRINFO(serial_number, int);
DEFINE_ERRINFO(shared_buffer_file, std::string);
DEFINE_ERRINFO(shared_fifo_file, std::string);
DEFINE_ERRINFO(shared_lock_file, std::string);
DEFINE_ERRINFO(shared_object_name, std::string);
DEFINE_ERRINFO(shared_state_file, std::string);
DEFINE_ERRINFO(siu_command, int);
DEFINE_ERRINFO(status_code, int);

// Undefine macro for header safety (we don't want to pollute the global namespace with collision-prone names)
#undef DEFINE_ERRINFO

// RORC exception definitions
struct Exception : virtual boost::exception, virtual std::exception
{
    /// The what() function is overridden to use the 'errinfo_rorc_generic_message' when available
    virtual const char* what() const noexcept override;
};

// General exception definitions
struct RorcPdaException : virtual Exception {};
struct MemoryMapException : virtual Exception {};
struct ParameterException : virtual Exception {};
struct ParseException : virtual Exception {};
struct InvalidParameterException : virtual ParameterException {};
struct OutOfRangeException : virtual Exception {};
struct LockException : virtual Exception {};
struct FileLockException : virtual LockException {};
struct NamedMutexLockException : virtual LockException {};
struct DeviceFinderException : virtual Exception {};
struct SharedStateException : virtual Exception {};
struct SharedObjectNotFoundException : virtual Exception {};

// C-RORC exception definitions
struct CrorcException : virtual Exception {};
struct CrorcArmDataGeneratorException : virtual CrorcException {};
struct CrorcArmDdlException : virtual CrorcException {};
struct CrorcInitDiuException : virtual CrorcException {};
struct CrorcCheckLinkException : virtual CrorcException {};
struct CrorcSiuCommandException : virtual CrorcException {};
struct CrorcDiuCommandException : virtual CrorcException {};
struct CrorcSiuLoopbackException : virtual CrorcException {};
struct CrorcFreeFifoException : virtual CrorcException {};
struct CrorcStartDataGeneratorException : virtual CrorcException {};
struct CrorcStartTriggerException : virtual CrorcException {};
struct CrorcStopTriggerException : virtual CrorcException {};
struct CrorcDataArrivalException : virtual CrorcException {};

// CRU exception definitions
struct CruException : virtual Exception {};

// Utility exception definition
struct UtilException : virtual boost::exception, virtual std::exception {};
struct ProgramOptionException : virtual UtilException {};
struct InvalidOptionValueException : virtual ProgramOptionException {};
struct OptionRequiredException : virtual ProgramOptionException {};

/// Adds the given possible causes to the exception object.
/// Meant for catch & re-throw site usage, to avoid overwriting old errinfo_rorc_possible_causes.
/// This is necessary, because adding an errinfo overwrites any previous one present.
void addPossibleCauses(boost::exception& exception, const std::vector<std::string>& possibleCauses);

} // namespace Rorc
} // namespace AliceO2


namespace boost {

// These functions convert the errinfos to strings for diagnostic messages
std::string to_string(const AliceO2::Rorc::errinfo_rorc_card_type& e);
std::string to_string(const AliceO2::Rorc::errinfo_rorc_error_message& e);
std::string to_string(const AliceO2::Rorc::errinfo_rorc_loopback_mode& e);
std::string to_string(const AliceO2::Rorc::errinfo_rorc_pci_address& e);
std::string to_string(const AliceO2::Rorc::errinfo_rorc_pci_id& e);
std::string to_string(const AliceO2::Rorc::errinfo_rorc_pci_ids& e);
std::string to_string(const AliceO2::Rorc::errinfo_rorc_possible_causes& e);
std::string to_string(const AliceO2::Rorc::errinfo_rorc_readyfifo_status& e);
std::string to_string(const AliceO2::Rorc::errinfo_rorc_reset_level& e);
std::string to_string(const AliceO2::Rorc::errinfo_rorc_status_code& e);

}
