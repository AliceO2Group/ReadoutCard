///
/// \file CruFifoTable.h
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
///

#pragma once

#include <cstdint>
#include <array>

namespace AliceO2 {
namespace Rorc {

static constexpr size_t CRU_DESCRIPTOR_ENTRIES = 128l;

/// A class representing the CRU status and descriptor tables
/// This class is meant to be used as an aliased type, reinterpret_casted from a raw memory pointer
/// Since this is an aggregate type, it does not violate the strict aliasing rule.
/// For more information, see:
///   http://en.cppreference.com/w/cpp/language/reinterpret_cast
///   http://en.cppreference.com/w/cpp/language/aggregate_initialization
struct CruFifoTable
{
    /// A class representing a CRU status table entry
    struct StatusEntry
    {
        volatile uint32_t status;
    };

    /// A class representing a CRU descriptor table entry
    struct DescriptorEntry
    {

        /// Low 32 bits of the DMA source address on the card
        volatile uint32_t srcLow;

        /// High 32 bits of the DMA source address on the card
        volatile uint32_t srcHigh;

        /// Low 32 bits of the DMA destination address on the bus
        volatile uint32_t dstLow;

        /// High 32 bits of the DMA destination address on the bus
        volatile uint32_t dstHigh;

        /// Control register
        volatile uint32_t ctrl;

        /// Reserved field 1
        volatile uint32_t reserved1;

        /// Reserved field 2
        volatile uint32_t reserved2;

        /// Reserved field 3
        volatile uint32_t reserved3;
    };

    /// Array of status entries
    std::array<StatusEntry, CRU_DESCRIPTOR_ENTRIES> statusEntries;

    /// Array of descriptor entries
    std::array<DescriptorEntry, CRU_DESCRIPTOR_ENTRIES> descriptorEntries;
};

static_assert(sizeof(CruFifoTable::StatusEntry) == (1 * 4), "Size of CruFifoTable::StatusEntry invalid");
static_assert(sizeof(CruFifoTable::DescriptorEntry) == (8 * 4), "Size of CruFifoTable::DescriptorEntry invalid");
static_assert(
    sizeof(CruFifoTable)
    == (CRU_DESCRIPTOR_ENTRIES * sizeof(CruFifoTable::StatusEntry))
     + (CRU_DESCRIPTOR_ENTRIES * sizeof(CruFifoTable::DescriptorEntry)),
     "Size of CruFifoTable invalid");

} // namespace Rorc
} // namespace AliceO2
