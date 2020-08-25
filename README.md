[![Build Status](https://travis-ci.com/AliceO2Group/ReadoutCard.svg?branch=master)](https://travis-ci.com/AliceO2Group/ReadoutCard)
[![JIRA](https://img.shields.io/badge/JIRA-Report%20issue-blue.svg)](https://alice.its.cern.ch/jira/secure/CreateIssue.jspa?pid=11700&issuetype=1) [![](http://ali-ci.cern.ch/repo/buildstatus/AliceO2Group/ReadoutCard/master/build_ReadoutCard_o2-dataflow.svg)](https://ali-ci.cern.ch/repo/logs/AliceO2Group/ReadoutCard/master/latest/build_ReadoutCard_o2-dataflow/fullLog.txt)

ReadoutCard (RoC) module
===================

Table of Contents
===================
1. [Introduction](#introduction)
2. [Usage](#usage)
  * [DMA channels](#dma-channels)
  * [Card Configurator](#card-configurator)
  * [BAR interface](#bar-interface)
  * [Dummy implementation](#dummy-implementation)
  * [Parameters](#parameters-1)
  * [Utility programs](#utility-programs)
  * [Exceptions](#exceptions)
  * [Python interface](#python-interface)
3. [Installation](#installation)
4. [Implementation notes](#implementation-notes)
5. [Known issues](#known-issues)


Introduction
===================
The ReadoutCard module`*` is a C++ library that provides a high-level interface for accessing and controlling 
high-performance data acquisition PCIe cards.

Included in the library are several supporting command-line utilities for listing cards, accessing registers, 
performing tests, maintenance, benchmarks, etc. See the section 'Utility programs' for more information. 

If you are just interested in reading and writing to the BAR, without particularly high performance requirements,
feel free to skip ahead to the section "Python interface" for the most convenient way to do so.

The library currently supports the C-RORC and CRU cards.
It also provides a software-based dummy card (see the section "Dummy implementation" for more details).

`*` *Formerly known as the RORC module*

## Terminology
The following table provides an overview of the units in the memory layout that you might encounter 

| Unit | Description | Typical size |
| --- | --- | --- |
| Channel buffer | A typically large buffer for DMA transfers from one DMA channel. Typically created by readout process by allocating hugepages | Several GiB |
| Hugepage       | A large CPU MMU page | 2 MiB or 1 GiB |  
| Superpage      | A physically contiguous subdivision of a hugepage (1 MiB multiple). It is passed to the driver, which will fill it with DMA pages without further intervention | 2 MiB |
| DMA page       | The unit of individual DMA transfers used by the card | 8 KiB for CRU, configurable for C-RORC |

### The MMU, hugepages, and the IOMMU
Most x86-64 CPUs have an MMU that supports 4 KiB, 2 MiB and 1 GiB page sizes. 
4KB is the default, while the bigger ones are referred to as hugepages in Linux.
Since the cards can have DMA page sizes larger than 4KiB (the CRU's is 8KiB for example),
a 4KiB page size can easily become problematic if they are at all fragmented.
The roc-dma-bench program uses hugepages to ensure the buffer's memory is more contiguous and the DMA scatter-gather 
list is small.

But hugepages have to be allocated in advance. And they're not really needed with the IOMMU enabled 
(note: the IOMMU has an impact on DMA throughput and CPU usage).
With an enabled IOMMU, the ReadoutCard library will be able to present any user-allocated buffer that was registered
with the channel as a contiguous address space to the card, simplifying DMA buffer management.

When implementing a readout program with the ReadoutCard library, it is practically mandatory to either 
enable the IOMMU, or allocate the DMA buffer using hugepages.

Note that an IOMMU may not be available on your system.

For more detailed information about hugepages, refer to the linux kernel docs: 
  https://www.kernel.org/doc/Documentation/vm/hugetlbpage.txt
  
For information on how to configure the IOMMU and hugepages for the purposes of this library, 
see the "Installation" section.


Usage
===================
For a simple usage example, see the program in `src/Example.cxx`.
For high-performance readout, the benchmark program `src/CommandLineUtilities/ProgramDmaBench.cxx` may be more
instructive.

DMA channels
-------------------
Clients can acquire a lock on a DMA channel by instantiating a `DmaChannelInterface` implementation through 
the `ChannelFactory` class. Once this object is constructed, it will provide exclusive access to the DMA channel.

The user will need to specify parameters for the channel by passing an instance of the `Parameters` 
class to the factory function. 
The most important parameters are the card ID (either a serial number or a PCI address), the channel number, and the
buffer parameters.
The serial number and PCI address (as well as additional information) can be listed using the `roc-list-cards` 
utility.
The buffer parameters specify which region of memory, or which file to map, to use as DMA buffer.
See the `Parameters` class's setter functions for more information about the options available, or the
[Parameters](#parameters-1) section of this README.

Once a DMA channel has acquired the lock, clients can call `startDma()` and start pushing superpages to the driver's
transfer queue.
The user can check how many superpage slots are still available with `getTransferQueueAvailable()`.
For reasons of performance and simplicity, the driver operates in the user's thread and thus depends on the user calling `fillSuperpages()` periodically.
This function will start data transfers, and users can check for arrived superpages using `getReadyQueueSize()`.
If one or more superpage have arrived, they can be inspected and popped using the `getSuperpage()` and 
`popSuperpage()` functions.

DMA can be paused and resumed at any time using `stopDma()` and `startDma()`

### Data Source

#### CRU

The `Data Source` parameter for the CRU DMA Channel should be used as follows:

| `DataSource`        | Data Source |
| ------------------- | ----------- |
| `Fee`               | FEE (GBT)   |
| `Ddg`               | DDG (GBT)   |
| `Internal`          | DG          |

#### CRORC

| `DataSource`        | Data Source |
| ------------------- | ----------- |
| `Fee`               | FEE (GBT)   |
| `Siu`               | SIU         |
| `Diu`               | DIU         |
| `Internal`          | DG          |


Card Configurator
-------------------
The `CardConfigurator` class offers an interface to configure the Readout Card (_currently only implemented for the CRU_). In
order to configure the CRU one has to create a `CardConfigurator` object. The constructor can either be called with a list of
parameters, or a path to a configuration file, specifying these parameters.

### Parameters

The `CardConfigurator` utilizes the `Parameters` class, the same class where Parameters are specified for DMA channels. For the
Card Configurator, the parameters need to be initialized for the card on BAR2. The command that
achieves that is `makeParameters(cardId, 2)`. Refer to the [Parameters](#parameters-1) section for more information.

The Parameters that affect the configuration of the CRU, their possible values (in ()) and their default values (in []) are as follows:

`AllowRejection (true | false) [false]`

`CruId (0x0-0xffff) [0x0]`

`Clock (LOCAL | TTC) [LOCAL]`

`DatapathMode (PACKET | STREAMING) [PACKET]`

`DownstreamData (CTP | PATTERN | MIDTRG) [CTP]`

`GbtMode (GBT | WB) [GBT]`

`GbtMux (TTC | DDG | SWT | TTCUP) [TTC]`

`LinkLoopbackEnabled (true | false) [false]`

`PonUpstreamEnabled (true | false) [false]`

`OnuAddress (0-4294967296) [0]`

`DynamicOffsetEnabled (true | false) [false]`

`TriggerWindowSize (0 - 65535) [1000]`

`GbtEnabled (true | false) [true]`

`UserLogicEnabled (true | false) [false]`

To set any of the above parameters the usual template can be followed.

```
params.set[ParamName](Parameters::[ParamName]::fromString(paramValueString));
params.set[ParamName](Parameters::[ParamName]::type::[paramValue]);
```

For example, to set the `Clock` one can use on of the following:

```
params.setClock(Parameters::Clock::fromString(clockString));
params.setClock(Parameters::Clock::type::Local);
```

The above parameters will be set for the enabled links, as specified by the `LinkMask` parameter. See the [LinkMask](#linkmask) section
for more info.

Note that for `AllowRejection`, `LinkLoopbackEnabled`, `PonUpstreamEnabled`, `DynamicOffsetEnabled`, `GbtEnabled` and `UserLogicEnabled` it is sufficient to do the following, as they are simply booleans.

```
params.setAllowRejection(true);
params.setLinkLoopbackEnabled(true);
params.setPonUpstreamEnabled(true);
params.setDynamicOffsetEnabled(true);
...
```

Likewise for `OnuAddress`, passing the hex is enough.

```
params.setOnuAddress(0x0badcafe)
```

### Configuration File

The string containing the path to the configuration file has to start with "file:", otherwise the
`CardConfigurator` will disregard it as invalid. Parameters are split between "global" and "per-link". 

The "global" parameters are:

```
clock
cruId
datapathMode
loopback
gbtMode
downstreamData
ponUpstream
onuAddress
dynamicOffsetEnabled
triggerWindowSize
gbtEnabled
UserLogicEnabled
```

The "per link" parameters are
```
enabled
gbtMux
```

The configuration file separates the parameter into three groups.

1. `[cru]`

    This parts concerns global (i.e. non-link specfic) cru configuration.

2. `[links]`

    This part refers to all the links. Configuration that goes in this group will be applied to all links, unless specifically
    setting parameters for individual links in the next section. For example to enable all links with SWT MUX by default:

    ```
    [links]
    enabled=true
    gbtMux=swt
    ```

3. `[link*]`

    This part configures only the individual link and __overrides__ any previous parameters for the specific link. For example:
    ```
    [link4]
    enabled=true
    gbtMux=ttc
    ```

An example configuration file is provided with [cru_template.cfg](cru_template.cfg).

---

An example of using the `CardConfigurator`, with `Parameters` or a config file, can be found in [ProgramConfig.cxx](src/CommandLineUtilities/ProgramConfig.cxx)

BAR interface
-------------------
Users can also get a limited-access object (implementing `BarInterface`) from the `ChannelFactory`. 
This provides an interface to reading and writing registers to the BAR.
Currently, there are no limits imposed on which registers are allowed to be read from and written to, so it is still a
"dangerous" interface. But in the future, protections may be added.

Parameters
-------------------
The `Parameters` class holds parameters used for the DMA Channel, the BAR and the Card Configurator. In order to instanciate a
`Parameters` object one needs to specify at the minimum the card ID and the channel number (i.e. the BAR# to access on the CRU).
(To be updated with CRORC. Please assume for now that everything below is CRU-specific.)

### Card ID
To make an instance of the `Parameters` class the card ID has to be passed to `makeParameters()` as a `Parameters::CardIdType` object. To construct this from a string one has to use the function `Parameters::cardIdFromString(cardIdString)`.

### Channel Number
The BAR to access. Normally DMA transactions are done through BAR0 and configuration and status reports are done through BAR2.
Also needs to be passed to `makeParameters()` resulting in the following call:
```
Parameters params = Parameters::makeParameters(cardId, channelNumber);
```

### BufferParameters
The parameters of the user-provided DMA buffer. Can be a memory address, or a file.

```
params.setBufferParameters(buffer_parameters::Memory {address, size});
params.setBufferParameters(buffer_parameters::File {pathString, size});
```

### LinkMask 
The link mask indicates which links to use. The `LinkMask` has to be set through a string that may contain comma separated
integers or ranges. For example: `0,1,2,8-10` or `0-19,21-23`.

```
params.setLinkMask(LinkMaskFromString(linkMaskString));
```

### FirmwareCheck
The firmware check parameter is by default enabled. It can be used to disable the firmware check when opening a DMA channel.
```
params.setFirmwareCheckEnabled(true);
```

### Other parameters
Operations on all other parameters can be done through setter, getter and getterRequired() functions, as seen in [Parameters.h](include/ReadoutCard/Parameters.h)

Dummy implementation
-------------------
The `ChannelFactory` can instantiate a dummy object if the serial number -1 is passed to its functions.
This dummy object may at some point provide a mock DMA transfer, but currently it does not do anything.
 
If PDA is not available (see 'Dependencies') the factory will **always** instantiate a dummy object.

Utility programs
-------------------
The module contains some utility programs to assist with ReadoutCard debugging and administration.
For detailed information and usage examples, use a program's `--help` option.

Most programs will also provide more detailed output when given the `--verbose` option.

### roc-bar-stress
Tool to stress BAR accesses and evaluate performance.

### roc-bench-dma
DMA throughput and stress-testing benchmarks.
It may use files in these directories for DMA buffers: 
* `/var/lib/hugetlbfs/global/pagesize-2MB`
* `/var/lib/hugetlbfs/global/pagesize-1GB`
The program will report the exact file used. 
They can be inspected manually if needed, e.g. with hexdump: `hexdump -e '"%07_ax" " | " 4/8 "%08x " "\n"' [filename]`

### roc-cleanup
In the event of a serious crash, such as a segfault, it may be necessary to clean up and reset.
This tool serves this purpose and is intended to be run as root. Be aware that this will make every
running instance of readout.exe or roc-bench-dma fail.

### roc-config
Configures the CRU. Can be executed with a list of parameters, or with a [configuration file](#configuration-file). Uses the [Card Configurator](#card-configurator). For more details refer to the `--help` dialog of the binary.

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
Lists the readout cards present on the system along with information documented in the following table. Every entry represents
an endpoint. For every physical card present in the system, two endpoint entries should be extended for the CRU and one for the
CRORC.

| Parameter    | Description                                                                           |
| ------------ | ------------------------------------------------------------------------------------- |
| `#`          | Sequence ID. Used for addressing within `ReadoutCard` (int)                           |
| `Type`       | The card type (`CRU` or `CRORC`)                                                      |
| `PCI Addr`   | PCI address of the card                                                               |
| `Serial`     | The serial of the card (3-5 digit int)                                                |
| `Endpoint`   | Endpoint ID (`0/1` for a CRU, `0` for a CRORC)                                        |
| `NUMA`       | NUMA node of the card (`0` or `1`)                                                    |
| `FW Version` | Firmware version installed (`vx.y.z` if identified and supported, git hash otherwise) |
| `UL Version` | User Logic version installed (git hash)                                               |

Output may be in ASCII table (default), or JSON format (`--json-out` option). Example outputs can be found [here](doc/examples/roc-list-cards/).

### roc-metrics
Outputs metrics for the ReadoutCards. Output may be in an ASCII table (default) or in JSON (`--json-out` option) format. Example outputs can be found [here](doc/examples/roc-metrics/).

Parameter information can be extracted from the monitoring table below.

#### Monitoring metrics

To directly send metrics to the Alice O2 Monitoring library, the argument `--monitoring` is necessary.

###### Metric: `"card"`

| Value name                | Value | type   |
| ------------------------- | ----- | ------ |
| `"pciAddress"`            | -     | string |
| `"temperature"`           | -     | double |
| `"droppedPackets"`        | -     | int    |
| `"ctpClock"`              | -     | double |
| `"localClock"`            | -     | double |
| `"totalPacketsPerSecond"` | -     | int    |

| Tag key               | Value                 |
| --------------------- | --------------------- |
| `tags::Key::SerialId` | Serial ID of the card |
| `tags::Key::Endpoint` | Endpoint of the card  |
| `tags::Key::ID`       | ID of the card        |
| `tags::Key::Type`     | Type of the card      |

### roc-pkt-monitor
Monitors packet statistics per link and per CRU wrapper. Output may be in an ASCII table (default) or in JSON (`json-out` option)
format. Example outputs can be found [here](doc/examples/roc-pkt-monitor/).

Parameter information can be extracted from the monitoring table below.

#### Monitoring metrics

To directly send metrics to the Alice O2 Monitoring library, the argument `--monitoring` is necessary.

##### CRORC

###### Metric: `"link"`

| Value name              | Value  | Type   |
| ----------------------- | ------ | ------ |
| `"pciAddress"`          | -      | string |
| `"acquisitionRate"`     | -      | int    |
| `"packetsReceived"`     | -      | int    |

| Tag key               | Value                 |
| --------------------- | --------------------- |
| `tags::Key::SerialId` | Serial ID of the card |
| `tags::Key::CRORC`    | ID of the CRORC       |
| `tags::Key::ID`       | ID of the link        |
| `tags::Key::Type`     | `tags::Value::CRORC`  |

##### CRU

###### Metric: `"link"`

| Value name       | Value  | Type   |
| ---------------- | ------ | ------ |
| `"pciAddress"`   | -      | string |
| `"accepted"`     | -      | int    |
| `"rejected"`     | -      | int    |
| `"forced"`       | -      | int    |

| Tag key               | Value                 |
| --------------------- | --------------------- |
| `tags::Key::SerialId` | Serial ID of the card |
| `tags::Key::Endpoint` | Endpoint of the card  |
| `tags::Key::CRU`      | ID of the CRU         |
| `tags::Key::ID`       | ID of the link        |
| `tags::Key::Type`     | `tags::Value::CRU`    |

###### `Metric: `"wrapper"`

| Value name                 | Value  | Type   |
| -------------------------- | ------ | ------ |
| `"pciAddress"`             | -      | string |
| `"dropped"`                | -      | int    |
| `"totalPacketsPerSec"`     | -      | int    |
| `"forced"`                 | -      | int    |

| Tag key               | Value                 |
| --------------------- | --------------------- |
| `tags::Key::SerialId` | Serial ID of the card |
| `tags::Key::Endpoint` | Endpoint of the card  |
| `tags::Key::CRU`      | ID of the CRU         |
| `tags::Key::ID`       | ID of the link        |
| `tags::Key::Type`     | `tags::Value::CRU`    |

### roc-reg-[read, read-range, write]
Writes and reads registers to/from a card's BAR. 
By convention, registers are 32-bit unsigned integers.
Note that their addresses are given by byte address, and not as you would index an array of 32-bit integers.

### roc-reg-modify
Modifies certain bits of a card's register through the BAR.

### roc-reset
Resets a card channel

### roc-run-script
*Deprecated, see section "Python interface"*
Run a Python script that can use a simple interface to use the library.

### roc-setup-hugetlbfs
Setup hugetlbfs directories & mounts. If using hugepages, should be run once per boot.

### roc-status
Reports status on the card's global and per-link configuration. Output may be in an ASCII table (default) or in JSON (`--json-out` option) format. Example outputs can be found [here](doc/examples/roc-status/).

Parameter information can be extracted from the monitoring tables below. Please note that for "UP/DOWN" and "Enabled/Disabled"
states while the monitoring format is an int (0/1), in all other formats a string representation is used.

#### Monitoring status

To directly send metrics from `roc-status` to the Alice O2 Monitoring library, the argument `--monitoring` is necessary. The
metric format for the CRORC and the CRU is different, as different parameters are relevant for each card type.

##### CRORC

###### Metric: `"CRORC"`

| Value name             | Value                    | Type   |
| ---------------------- | ------------------------ | ------ |
| `"pciAddress"`         | -                        | string |
| `"qsfp"`               | 0/1 (Disabled/Enabled)   | int    |
| `"dynamicOffset"`      | 0/1 (Disabled/Enabled)   | int    |
| `"timeFrameDetection"` | 0/1 (Disabled/Enabled)   | int    |
| `"timeFrameLength"`    | -                        | int    |

| Tag key               | Value                 |
| --------------------- | --------------------- |
| `tags::Key::SerialId` | Serial ID of the card |
| `tags::Key::ID`       | ID of the card        | 
| `tags::Key::Type`     | `tags::Value::CRORC`  |

###### Metric: `"link"`

| Value name       | Value              | Type   |
| ---------------- | --------------     | ------ |
| `"pciAddress"`   | -                  | string |
| `"status"`       | 0/1 (DOWN/UP)      | int    |
| `"opticalPower"` | -                  | double |

| Tag key               | Value                 |
| --------------------- | --------------------- |
| `tags::Key::SerialId` | Serial ID of the card |
| `tags::Key::CRORC`    | ID of the CRORC       |
| `tags::Key::ID`       | ID of the link        |
| `tags::Key::Type`     | `tags::Value::CRORC`  |

##### CRU

###### Metric: `"CRU"`

| Value name                  | Value                   | Type   | 
| --------------------------- | ----------------------- | ------ | 
| `"pciAddress"`              | -                       | string |
| `"CRU ID"`                  | Assigned CRU ID         | int    |
| `"clock"`                   | "TTC" or "Local"        | string |
| `"dynamicOffset"`           | 0/1 (Disabled/Enabled)  | int    |
| `"userLogic"`               | 0/1 (Disabled/Enabled)  | int    |
| `"runStats"`                | 0/1 (Disabled/Enabled)  | int    |
| `"commonAndUserLogic"`      | 0/1 (Disabled/Enabled)  | int    |

| Tag key               | Value                 |
| --------------------- | --------------------- |
| `tags::Key::SerialId` | Serial ID of the card |
| `tags::Key::Endpoint` | Endpoint of the card  |
| `tags::Key::ID`       | ID of the card        |
| `tags::Key::Type`     | `tags::Value::CRU`    |

###### Metric: `"onu"`

| Value name         | Value                       | Type   | 
| -------------------| --------------------------- | ------ | 
| `"onuStatus"`      | 0/1/2 (DOWN/UP/UP was DOWN) | int    |
| `"onuAddress"`     | ONU Address                 | string |
| `"rx40Locked"`     | 0/1 (False/True)            | int    |
| `"phaseGood"`      | 0/1 (False/True)            | int    |
| `"rxLocked"`       | 0/1 (False/True)            | int    |
| `"operational"`    | 0/1 (False/True)            | int    |
| `"mgtTxReady"`     | 0/1 (False/True)            | int    |
| `"mgtRxReady"`     | 0/1 (False/True)            | int    |
| `"mgtTxPllLocked"` | 0/1 (False/True)            | int    |
| `"mgtRxPllLocked"` | 0/1 (False/True)            | int    |


###### Metric: `"link"`

| Value name       | Value                                                             | Type   | 
| ---------------- | ----------------------------------------------------------------- | ------ | 
| `"pciAddress"`   | -                                                                 | string |
| `"gbtMode"`      | "GBT/GBT" or "GBT/WB"                                             | string |
| `"loopback"`     | 0/1 (Enabled/Disabled)                                            | int    |
| `"gbtMux"`       | "DDG", "SWT", "TTC:CTP", "TTC:PATTERN", "TTC:MIDTRG", or "TTCUP"  | string |
| `"datapathMode"` | "PACKET" or "STREAMING"                                           | string |
| `"datapath"`     | 0/1 (Disabled/Enabled)                                            | int    |
| `"rxFreq"`       | -                                                                 | double |
| `"txFreq"`       | -                                                                 | double |
| `"status"`       | 0/1/2 (DOWN/UP/UP was DOWN)                                       | int    |
| `"opticalPower"` | -                                                                 | double |

| Tag key               | Value                 |
| --------------------- | --------------------- |
| `tags::Key::SerialId` | Serial ID of the card |
| `tags::Key::Endpoint` | Endpoint of the card  |
| `tags::Key::CRU`      | ID of the CRU         |
| `tags::Key::ID`       | ID of the link        |
| `tags::Key::Type`     | `tags::Value::CRU`    |

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
import libReadoutCard
# To open a BAR channel, we can use the card's PCI address or serial number
# Here we open channel number 0
bar = libReadoutCard.BarChannel("42:0.0", 0) # PCI address
bar = libReadoutCard.BarChannel("12345", 0) # Serial number
bar = libReadoutCard.BarChannel("-1", 0) # Dummy channel

# Read register at index 0
bar.register_read(0)
# Write 123 to register at index 0
bar.register_write(0, 123)
# Modify bits 3-5 to at index 0 to 0x101
bar.register_modify(0, 3, 3, 0x101)

# Print doc strings for more information
print bar.__init__.__doc__
print bar.register_read.__doc__
print bar.register_write.__doc__
print bar.register_modify.__doc__
~~~
Note: depending on your environment, you may have to be in the same directory as the libReadoutCard.so file to import 
it.
You can also set the PYTHONPATH environment variable to the directory containing the libReadoutCard.so file.

Installation
===================
Install the dependencies below and follow the instructions for building the FLP prototype.

Alternatively, you can install the [FLP Suite](https://alice-o2-project.web.cern.ch/flp-suite) to completely set up the system.

Dependencies
-------------------
### Compatibility

In order to use a CRU the package versions have to adhere to the following table.

| ReadoutCard | CRU firmware  | CRORC firmware | PDA Driver  | PDA Library  |
| ----------- | ------------- | -------------- | ----------- | ------------ |
| v0.10.*     | v3.0.0/v3.1.0 | -              | v1.0.3+     | v12.0.0      |
| v0.11.*     | v3.0.0/v3.1.0 | -              | v1.0.4+     | v12.0.0      | 
| v0.12.*     | v3.2.0/v3.3.0 | -              | v1.0.4+     | v12.0.0      | 
| v0.13.*     | v3.2.0/v3.3.0 | v2.4.0         | v1.0.4+     | v12.0.0      | 
| v0.14.*     | v3.2.0/v3.3.0 | v2.4.0         | v1.0.4+     | v12.0.0      | 
| v0.14.5     | v3.4.0        | v2.4.0         | v1.0.4+     | v12.0.0      |
| v0.14.5     | v3.5.0        | v2.4.0         | v1.0.4+     | v12.0.0      |
| v0.15.0     | v3.5.1        | v2.4.0         | v1.0.4+     | v12.0.0      |
| v0.16.0     | v3.5.2        | v2.4.0         | v1.0.4+     | v12.0.0      |
| v0.19.2     | v3.5.2        | v2.4.1         | v1.0.4+     | v12.0.0      |
| v0.21.1     | v3.6.1        | v2.6.1         | v1.0.4+     | v12.0.0      |
| v0.22.0     | v3.7.0/v3.8.0 | v2.6.1         | v1.0.4+     | v12.0.0      |

The _PDA Driver_ entry refers to the `pda-kadapter-dkms-*.rpm` package which is availabe through the [o2-daq-yum](http://alice-daq-yum-o2.web.cern.ch/alice-daq-yum-o2/cc7_64/) repo as an RPM.

The _PDA Library_ entry refers to the `alisw-PDA*.rpm` package which is pulled as a dependency of the ReadoutCard rpm.

Both _PDA_ packages can also be installed from source as described on the next section.

For the _CRU firmware_ see the [gitlab](https://gitlab.cern.ch/alice-cru/cru-fw) repo.

### PDA
The module depends on the PDA (Portable Driver Architecture) library and driver.
If PDA is not detected on the system, only a dummy implementation of the interface will be compiled.

1. Install dependency packages
  ~~~
  yum install kernel-devel pciutils-devel kmod-devel libtool
  ~~~

2. Download PDA
  ~~~
  git clone https://github.com/AliceO2Group/pda.git
  cd pda
  ~~~

3. Compile
  ~~~
  ./configure --debug=false --numa=true --modprobe=true
  make install #installs the pda library
  cd patches/linux_uio
  make install #installs the pda driver
  ~~~
  
4. Optionally, insert kernel module. If the utilities are run as root, PDA will do this automatically.
  ~~~
  modprobe uio_pci_dma
  ~~~

### Hugepages
At some point, we should probably use kernel boot parameters to allocate hugepages, or use some boot-time script, but 
until then, we must initialize and allocate manually.

Either use the script `roc-setup-hugetlbfs.sh` (located in the src directory), or do manually:

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

Note that after every reboot it is necessary to run the `roc-setup-hugetlbfs.sh` script again or repeat the previous manual steps.


Configuration
-------------------
### IOMMU
To enable the IOMMU, add `iommu=on` to the kernel boot parameters.

### memlimit
DMA buffer registration involves a system call called mlock(2). This locks memory areas in the RAM so that they don't get paged
to swap. Linux users have to abide to a global limit, which can be seen by running `ulimit -l`.

Lift that limit for the pda group by adding the following in `/etc/security/limits.conf`:

~~~
@pda hard memlock unlimited
@pda soft memlock unlimited
~~~

### Permissions
The library must be run either by root users, or users part of the group 'pda'.
The PDA kernel module must be inserted as root in any case.


Implementation notes
===================
Channel classes
-------------------
The `DmaChannelInterface` is implemented using multiple classes.
`DmaChannelBase` takes care of locking and provides default implementations for utility methods.
`DmaChannelPdaBase` uses PDA to take care of memory mapping, registering the DMA buffer with the IOMMU, 
creating scatter-gather lists and PDA related initialization.
Finally, `CrorcDmaChannel` and `CruDmaChannel` take care of device-specific implementation details for the C-RORC and
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
* DMA buffers (as destination for DMA transfers)

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
