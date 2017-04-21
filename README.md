# RORC module


Authors
===================

* C layer: Tuan Mate Nguyen (tuan.mate@outlook.com)
* C++: Pascal Boeschoten (pascal.boeschoten@cern.ch)


Description
===================

The RORC module is the software that directly interfaces with the RORC (ReadOut and Receiver Card) PCIe cards.
RORCs can have multiple DMA channels, and each channel is treated separately by the program.
  (Note: the old C-RORC card has 6 channels, the upcoming CRU 1.)

Channel ownership lock
-------------------
Clients can acquire a master lock on a channel by instantiating a ChannelMasterInterface implementation through the
ChannelFactory class. 

The ChannelMaster class provides PDA-based functionality common to the C-RORC and CRU, and it is mostly an aggregation 
of PDA wrapper classes and shared memory handling.
The CrorcChannelMaster and CruChannelMaster provide the device-specific functionality and use the RORC C API layer.

The ChannelMaster will acquire a lock to prevent simultaneous access to a channel.

The driver uses some files in shared memory:
* `/dev/shm/alice_o2/rorc/[PCI geographical address]/channel_[channel number]/.lock` For locking channels
* `/dev/shm/sem.alice_o2_rorc_[PCI geographical address]_channel_[channel number].mutex` For locking channels
* `/dev/hugepages/alice_o2/rorc/[PCI geographical address]/channel_[channel number]/fifo` For card FIFOs

If the process crashes badly, it may be necessary to clean up the mutex manually, either by deleting with `rm` or by 
using the `rorc-channel-cleanup` utility.

Once a ChannelMaster has acquired the lock, clients can:
* Read and write registers
* Start and stop DMA
* Push and read pages

For a usage example, see the program in RORC/src/Example.cxx

Limited-access interface
-------------------
Users can also get a limited-access object (implementing ChannelSlaveInterface) from the
factory.

Dummy objects
-------------------
The ChannelFactory can instantiate a dummy object if a certain serial number (currently -1 is used) is passed to its
functions. If PDA is not available (see 'Dependencies') the factory will **always** instantiate a dummy object.

Utility programs
-------------------
The RORC module contains some utility programs to assist with RORC debugging and administration.
rorc-bench-dma uses files in these directories for DMA buffers: 
* `/var/lib/hugetlbfs/global/pagesize-2MB`
* `/var/lib/hugetlbfs/global/pagesize-1GB`
They can be inspected manually if needed, e.g. with hexdump: `hexdump -e '"%07_ax" " | " 4/8 "%08x " "\n"' <filename>`

Exceptions
-------------------
The RORC module makes use of exceptions. Nearly all of these are derived from `boost::exception`.
They are defined in the header 'RORC/Exception.h'. These exceptions may contain extensive information about the cause
of the issue in the form of `boost::error_info` structs which can aid in debugging. 
To generate a diagnostic report, you may use `boost::diagnostic_information(exception)`.      


Design notes
===================

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
* DMA buffer (the push destination)
* Card's FIFO (card updates status in here)
* For ChannelMaster (& subclasses) "persistent member variables":
  * Must be quickly updated
  * Capable of stop & resume
  * Persistent as long as system runs (not across reboots, since then cards & firmware may be changed) 
* For mutex (inter & intra process channel lock
TODO elaborate a bit more

Scatter-gather lists
-------------------
Scatter-gather lists (SGLs) contain a sequence of memory regions that the card's DMA engine can use.
The granularity of the regions is in pages. Without an SGL, the DMA buffer would have to be a contiguous piece of 
physical memory, which may be very difficult to allocate. With an SGL, we can use pages scattered over physical memory.
The regions can also presented in userspace as contiguous memory, thanks to the magic of the MMU.   


Known issues
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


Dependencies
===================

The RORC module depends on the PDA (Portable Driver Architecture) library. 
If PDA is not detected on the system, only a dummy implementation of the interface will be compiled.

PDA installation
-------------------

1. Install dependency packages
  ```
  yum install kernel-devel pciutils-devel kmod-devel libtool libhugetlbfs
  ```

2. Download & extract PDA 11.0.7
  ```
  wget https://compeng.uni-frankfurt.de/fileadmin/Images/pda/pda-11.0.7.tar.gz
  tar zxf pda-11.0.7.tar.gz
  cd pda-11.0.7
  ```

3. Compile
  ```
  ./configure --debug=false --numa=true --modprobe=true
  make install
  cd patches/linux_uio
  make install
  ```
  
4. Optionally, insert kernel module. If the utilities are run as root, PDA will do this automatically.
  ```
  modprobe uio\_pci\_dma
  ```

5. When using rorc-bench-dma, create hugetlbfs mounts
  ```
  hugeadm --create-global-mounts
  ```


Hugepages
===================

When using shared memory allocations for the DMA buffer, it is highly recommended to configure hugepage support
on the system.
Without hugepages, scatter-gather lists may be highly fragmented.
In addition, with larger RORC page sizes (> 4 KB), the driver may have difficulty finding enough contiguous memory to 
use for RORC pages.
For more information about hugepages, refer to the linux kernel docs: 
  https://www.kernel.org/doc/Documentation/vm/hugetlbpage.txt


Hugepages configuration
-------------------

At some point, we should probably use kernel boot parameters to allocate hugepages, or use some boot-time script, but 
until then, we must initialize and allocate manually.

1. Install hugetlbfs (will already be installed on most systems)
  ```
  yum install libhugetlbfs libhugetlbfs-utils
  ```

2. Allocate hugepages
  ```
  echo [number] > /proc/sys/vm/nr_hugepages
  ```
  Where [number] is enough to cover the DMA buffer needs.
  By default, hugepages are 2 MB. 

3. Check to see if they're actually available
  ```
  cat /proc/meminfo | grep Huge
  ```
 'HugePages_Total' should correspond to [number]

4. Mount hugetlbfs
  ```
  mkdir /mnt/hugetlbfs (if directory does not exist)
  mount --types hugetlbfs none /mnt/hugetlbfs -o pagesize=2M
  ```

ALICE Low-level Front-end (ALF) DIM Server
===================
The utilities contain a DIM server for DCS control of the cards 

Usage
-------------------
./rorc-alf-server --serial=11225 --channel=0
Note: if the DIM_DNS_NODE environment variable was not set, the server uses localhost. 

Service description
-------------------

Services names are under: 
"ALF/SERIAL_[a]/CHANNEL_[b]/[service name]"
where [a] = card serial number
      [b] = card channel / BAR index

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
















