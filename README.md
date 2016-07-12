# RORC module


Authors
===================

* C layer: Tuan Mate Nguyen (tuan.mate@outlook.com)
* C++ interface: Pascal Boeschoten (pascal.boeschoten@cern.ch)


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

The ChannelMaster will acquire a file lock to prevent simultaneous interprocess access to a channel.
Currently, there are no protections against multi-threaded access, but this is planned for a future release.
Some of the state is kept in shared memory files so that the state is persistent and that the lock can be handed over
to other processes.
Pages are also stored in shared memory. Functionality for clients to supply their own buffer is planned for a future 
release.

The shared files are located in the directories:
* /dev/shm/alice_o2/rorc_[serial number]/channel_[channel number]/
* /mnt/hugetlbfs/alice_o2/rorc_[serial number]/channel_[channel number]/

If things crash and the state can not be recovered, these files should be deleted.
They should also be deleted with new releases of the RORC module, since their internal memory arrangement might change.

Once a ChannelMaster has acquired the lock, clients can:
* Read and write registers
* Start and stop DMA
* Pushing and reading pages

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
Currently, only utilities for reading and writing registers are available, but more are planned.


Known issues
===================

It appears that with the C-RORC, the isPageArrived() function does not work properly and returns 'true' too early.
It is suspected to be a firmware bug, where the page status in the Ready FIFO is marked as fully transferred while the
actual DMA transfer has not yet been completed.
The investigation is ongoing, but for the time being we can work around the issue with a manual delay.
The Example.cxx uses a microsecond wait, which is probably overkill.

The FIFO of the C-RORC is initialized by pushing 128 pages. The framework should wait until these pages are fully
transferred before continuing. This is not yet implemented, because of the isPageArrived() issue described above.
Again, clients must wait manually.

The CruChannelMaster implementation is not complete.

ChannelFactory should instantiate a ChannelMaster object based on card type and serial number. 
But currently, it just goes with the first C-RORC it comes across. 


Dependencies
===================

The RORC module depends on the PDA (Portable Driver Architecture) library. 
If PDA is not detected on the system, only a dummy implementation of the interface will be compiled.

PDA installation
-------------------

1. Install libpci dependency
  ~~~
  yum install pciutils-devel
  ~~~

2. Download & extract PDA 11.0.7
  ~~~
  wget https://compeng.uni-frankfurt.de/fileadmin/Images/pda/pda-11.0.7.tar.gz
  tar zxvf pda-11.0.7.tar.gz
  cd pda-11.0.7
  ~~~

3. Compile
  ~~~
  ./configure --debug=true --numa=true --modprobe=true
  make
  make install
  ~~~


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
  ~~~
  yum install libhugetlbfs
  ~~~

2. Allocate hugepages
  ~~~
  echo [number] > /proc/sys/vm/nr_hugepages
  ~~~
  Where [number] is enough to cover the DMA buffer needs.
  By default, hugepages are 2 MB. 

3. Check to see if they're actually available
  ~~~
  cat /proc/meminfo | grep Huge
  ~~~
 'HugePages_Total' should correspond to [number]

4. Mount hugetlbfs
  ~~~
  mkdir /mnt/hugetlbfs (if directory does not exist)
  mount --types hugetlbfs none /mnt/hugetlbfs -o pagesize=2M
  ~~~


