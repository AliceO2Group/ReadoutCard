ReadoutCard (RoC) module
===================
The ReadoutCard module* is a C++ library that provides a high-level interface for accessing and controlling 
high-performance data acquisition PCIe cards.

**Formerly known as the RORC module*


Table of Contents
===================
1. [Introduction](#introduction)
2. [Usage](#usage)
3. [Installation](#installation)
4. [Implementation notes](#implementation-notes)
5. [Known issues](#known-issues)
6. [ALICE Low-level Front-end (ALF) DIM Server](#alf)


Introduction  <div id='introduction'/>
===================
### Supported cards
The library currently supports the C-RORC and CRU cards.
It also provides a software-based dummy card (see the section "Dummy implementation" for more details)

[todo] BAR, DMA buffer, Superpages, pages, and other confusing terminology, memory layout

### The MMU, hugepages, and the IOMMU
Most x86-64 CPUs have an MMU that supports 4 KiB, 2 MiB and 1 GiB page sizes. 
4KB is the default, while the bigger ones are referred to as hugepages in Linux.
Since the cards can have DMA page sizes larger than 4KiB (the CRU's is 8KiB for example),
a 4KiB page size can easily become problematic if they are at all fragmented.
The roc-dma-bench program uses hugepages to ensure the buffer's memory is more contiguous and the DMA scatter-gather 
list is small.

But hugepages have to be allocated in advance. And they're not really needed with the IOMMU enabled 
(note: performance impact of using the IOMMU has not been measured).
With an enabled IOMMU, the ReadoutCard library will be able to present any user-allocated buffer that was registered
with the channel as a contiguous address space to the card, simplifying DMA buffer management.

When implementing a readout program with the ReadoutCard library, it is practically mandatory to either 
enable the IOMMU, or allocate the DMA buffer using hugepages.

Note that an IOMMU may not be available on your system.

For more detailed information about hugepages, refer to the linux kernel docs: 
  https://www.kernel.org/doc/Documentation/vm/hugetlbpage.txt
  
For information on how to configure the IOMMU and hugepages for the purposes of this library, 
see the "Installation" section.


Usage  <div id='usage'/>
===================
For a simple usage example, see the program in `src/Example.cxx`.
For high-performance readout, the benchmark program `src/CommandLineUtilities/ProgramDmaBench.cxx` may be more
instructive.

DMA channels
-------------------
Clients can acquire a lock on a DMA channel by instantiating a *ChannelMasterInterface* implementation through 
the *ChannelFactory* class. Once this object is constructed, it will provide exclusive access to the DMA channel.

The user will need to specify parameters for the channel by passing an instance of the *Parameters* 
class to the factory function. The most important parameters are the card ID (either a serial number of a PCI address),
and the BAR channel number. See the *Parameters* class's setter functions for more information about the options 
available.

The driver uses some files in shared memory:
* `/dev/shm/alice_o2/rorc/[PCI address]/channel_[channel number]/fifo` - For card FIFOs
* `/dev/shm/alice_o2/rorc/[PCI address]/channel_[channel number]/.lock` - For locking channels
* `/dev/shm/sem.alice_o2_rorc_[PCI address]_channel_[channel number].mutex` - For locking channels
* `/var/lib/hugetlbfs/global/pagesize-[page size]/rorc-dma-bench_id=[PCI address]_chan_[channel number]` - For card benchmark DMA buffers

If the process crashes badly, it may be necessary to clean up the mutex manually, either by deleting with `rm` or by 
using the `rorc-channel-cleanup` utility. There is also the possibility of automatic cleanup by setting forced unlocking
enabled using the `setForcedUnlockEnabled()` function of the *Parameters* class. However, this option should be used with
caution. See the function's documentation for more information about the risks.

Once a ChannelMaster has acquired the lock, clients can:
* Start and stop DMA
* Push and read pages
* Read and write registers of the BAR channel

BAR interface
-------------------
Users can also get a limited-access object (implementing *ChannelSlaveInterface*) from the *ChannelFactory*. 
It is restricted to reading and writing registers to the BAR. 
Currently, there are no limits imposed on which registers are allowed to be read from and written to, so it is still a
"dangerous" interface. But in the future, protections may be added.

Dummy implementation
-------------------
The ChannelFactory can instantiate a dummy object if the serial number -1 is passed to its functions.
This dummy object may at some point provide a mock DMA transfer, but currently it does not do anything.
 
If PDA is not available (see 'Dependencies') the factory will **always** instantiate a dummy object.

Utility programs
-------------------
The module contains some utility programs to assist with ReadoutCard debugging and administration.
For detailed information and usage examples, use a program's `--help` option.

Most programs will also provide more detailed output when given the `--verbose` option.

### roc-alf-client & rorc-alf-server
See section "ALICE Low-level Front-end"

### roc-bench-dma
DMA throughput and stress-testing benchmarks.
It may use files in these directories for DMA buffers: 
* `/var/lib/hugetlbfs/global/pagesize-2MB`
* `/var/lib/hugetlbfs/global/pagesize-1GB`
The program will report the exact file used. 
They can be inspected manually if needed, e.g. with hexdump: `hexdump -e '"%07_ax" " | " 4/8 "%08x " "\n"' [filename]`

### roc-channel-cleanup
In the event of a serious crash, such as a segfault, it may be necessary to clean up and reset a channel.
See section "Channel ownership lock" for more details.

### roc-example
The compiled example of `src/Example.cxx`
 
### roc-flash
Flashes firmware from a file onto the card.
Note that it is not advised to abort a flash in progress, as this will corrupt the firmware present on the card. 
Please commit to your flash.

Once a flash has completed, the host will need to be rebooted for the new firmware to be loaded.

Currently only supports the C-RORC.

### roc-flash-read
Reads from the card's flash memory.

Currently only supports the C-RORC.

### roc-list-cards
Lists the readout cards present on the system, along with their type, PCI address, vendor ID, device ID, serial number, 
and firmware version.    

### roc-reg-[read, read-range, write]
Writes and reads registers to/from a card's BAR. 
By convention, registers are 32-bit unsigned integers.
Note that their addresses are given by byte address, and not as you would index an array of 32-bit integers.

### roc-reset
Resets a card channel

### roc-run-script
*Deprecated, see section "Python interface"*

Run a Python script that can use a simple interface to use the library.


Exceptions
-------------------
The module makes use of exceptions. Nearly all of these are derived from `boost::exception`.
They are defined in the header 'ReadoutCard/Exception.h'. 
These exceptions may contain extensive information about the cause of the issue in the form of `boost::error_info` 
structs which can aid in debugging. 
To generate a diagnostic report, you may use `boost::diagnostic_information(exception)`.      

Python interface
-------------------
If the library is compiled with Boost Python available, the shared object will be usable as a Python library.
It is currently only able to read and write registers.
Example usage:
~~~
# Note: depending on your environment, you may have to be in the same directory
# as the libReadoutCard.so file to import it 
import libReadoutCard
# To open a channel, we can use the card's PCI address or serial number
# Here we open channel number 0
channel = libReadoutCard.Channel("42:0.0", 0) # PCI address
channel = libReadoutCard.Channel("12345", 0) # Serial number
channel = libReadoutCard.Channel("-1", 0) # Dummy channel

# Read register at index 0
channel.register_read(0)
# Write 123 to register at index 0
channel.register_write(0, 123)

# Print doc strings for more information
print channel.__init__.__doc__
print channel.register_read.__doc__
print channel.register_write.__doc__
~~~


Installation  <div id='installation'/>
===================
Install the dependencies below and follow the instructions for building the FLP prototype.

Dependencies
-------------------
### PDA
The module depends on the PDA (Portable Driver Architecture) library. 
If PDA is not detected on the system, only a dummy implementation of the interface will be compiled.

1. Install dependency packages
  ~~~
  yum install kernel-devel pciutils-devel kmod-devel libtool libhugetlbfs
  ~~~

2. Download & extract PDA 11.0.7
  ~~~
  wget https://compeng.uni-frankfurt.de/fileadmin/Images/pda/pda-11.0.7.tar.gz
  tar zxf pda-11.0.7.tar.gz
  cd pda-11.0.7
  ~~~

3. Compile
  ~~~
  ./configure --debug=false --numa=true --modprobe=true
  make install
  cd patches/linux_uio
  make install
  ~~~
  
4. Optionally, insert kernel module. If the utilities are run as root, PDA will do this automatically.
  ~~~
  modprobe uio\_pci\_dma
  ~~~

### Hugepages
At some point, we should probably use kernel boot parameters to allocate hugepages, or use some boot-time script, but 
until then, we must initialize and allocate manually.

1. Install hugetlbfs (will already be installed on most systems)
  ~~~
  yum install libhugetlbfs libhugetlbfs-utils
  ~~~
  
2. Set up filesystem mounts
  ~~~
  hugeadm --create-global-mounts
  ~~~

3. Allocate hugepages
  ~~~
  # 2 MiB hugepages
  echo [number] > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
  # 1 GiB
  echo [number] > /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages
  ~~~
  Where [number] is enough to cover your DMA buffer needs.

4. Check to see if they're actually available
  ~~~
  hugeadm --pool-list
  ~~~

### IOMMU
To enable the IOMMU, add `iommu=on` to the kernel boot parameters.


Implementation notes  <div id='implementation-notes'/>
===================
Channel classes
-------------------
The ChannelMasterInterface is implemented using multiple classes.
ChannelMasterBase takes care of locking and provides default implementations for utility methods.
ChannelMasterPdaBase uses PDA to take care of memory mapping, registering the DMA buffer with the IOMMU, 
creating scatter-gather lists and PDA related initialization.
Finally, CrorcChannelMaster and CruChannelMaster take care of device-specific implementation details for the C-RORC and
CRU respectively.

Enums
-------------------
Enums are surrounded by a struct, so we can group both the enum values themselves and any accompanying functions.
It also allows us to use the type of the struct as a template parameter.

Volatile variables
-------------------
Throughout the library, the variables which (may) refer to memory which the card can write to are declared as volatile. 
This is to indicate to the compiler that the values of these variables may change at any time, completely outside of the
control of the process. This means the compiler will avoid certain optimizations that may lead to unexpected behaviour. 

Shared memory usage
-------------------
Shared memory is used in several places:
* C-RORC's FIFO (card updates status of DMA transfers in here)
* For locks and mutexes (inter & intra process channel lock)
* DMA buffers (as destination for DMA transfers)

Scatter-gather lists
-------------------
Scatter-gather lists (SGLs) contain a sequence of memory regions that the card's DMA engine can use.
The granularity of the regions is in pages. Without an SGL, the DMA buffer would have to be a contiguous piece of 
physical memory, which may be very difficult to allocate. With an SGL, we can use pages scattered over physical memory.
The regions can also presented in userspace as contiguous memory, thanks to the magic of the MMU.   


Known issues  <div id='known-issues'/>
===================
C-RORC concurrent channels
-------------------
On certain machines, initializing multiple C-RORC channels concurrently has led to hard lockups.
The cause is unknown, but adding acpi=off to the Linux boot options fixed the issue.
The issue has occurred on Dell R720 servers.

Permissions
-------------------
The library must be run either by root users, or users part of the group 'pda'. In case of 'pda' group users, make sure
the `/dev/shm/alice_o2` and `/mnt/hugetlbfs/alice_o2` directories have sufficient permissions that allow those 
users to create/read/write files.
Also, the PDA kernel module must be inserted as root in any case.


ALICE Low-level Front-end (ALF) DIM Server  <div id='alf'/>
===================
The utilities contain a DIM server for DCS control of the cards 

Usage
-------------------
`./roc-alf-server --serial=11225 --channel=0`
Note: if the DIM_DNS_NODE environment variable was not set, the server uses localhost. 

Service description
-------------------

Services names are under: 
`ALF/SERIAL_[a]/CHANNEL_[b]/[service name]`
where [a] = card serial number
      [b] = BAR index

* The RPC calls take a string argument and return a string.
* When the argument must contain multiple values, they must be comma-separated.
* The return string will contain an 8-byte prefix indicating success or failure "success:" or "failure:", 
  optionally followed by a return value or error message.

For example: a register write call could have the argument string "504,42" meaning write value 42 to address 504.
The return string could be "success:" or "failure:Address out of range".

#### Register read
* Service type: RPC call
* Service name: REGISTER_READ 
* Parameters: register address
* Return: register value

#### Register write
* Service type: RPC call
* Service name: REGISTER_WRITE
* Parameters: register address, register value
* Return:

#### Temperature
Card's core temperature in degrees Celsius
* Service type: Published
* Service name: TEMPERATURE
* Value type: double
