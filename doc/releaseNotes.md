# ReadoutCard release notes

This file describes the main feature changes for released versions of ReadoutCard library and tools.

## v0.38.0 - 05/09/2022
- Added release notes as part of source code.
- Added library version number. Can be retrieved at runtime with o2::roc::getReadoutCardVersion().
- Fixed bug in DMA reset. Old data from previous run could stay in the pipeline if previous process was stopped abruptly. 

## v0.39.0 - 07/10/2022
- Added option --status-report to roc-config, in order to dump roc-status (similar) output to given file name. Can be stdout, infologger, or a file name. The file name can be preceded with + for appending the file. Name can contain special escape sequences %t (timestamp) %T (date/time) or %i (card ID). Infologger reports are set with error code 4805.

## v0.39.1 - 16/11/2022
- Fixed CRORC start-stop-start: fifo ready counters reset, emptying last page if unused, flush order on stop, release of unused pages in internal fifo.

## v0.40.0 - 08/12/2022
- log messages level updated: moved all Ops messages to Devel. The calling layer is in charge to reporting high level operation messages.
- superpage metadata: added link id.
- added verbosity protection for "Empty counter of Superpage FIFO" warnings

## v0.40.1 - 11/01/2023
- Added support for CRORC firmware version v2.10.0 (0x221ff280)
- Minor compilation warnings fixed

## v0.40.2 - 16/02/2023
- Fixed bug with o2-roc-ctp-emulator failing to parse hexadecimal values of the init-orbit option.

## v0.41.0 - 23/02/2023
- Added glitchCounter to roc-status (monitoring + JSON output).
- Added DMA status (enabled / disabled) to roc-status (all output styles, including monitoring).
- Added FEC counter per link to roc-status (stdout + JSON output).

## v0.42.0 - 01/03/2023
- Added support for CRORC ID where missing: roc-status, roc-config JSON.
- Added FEC counter per link to roc-status (monitoring).

## v0.42.1 - 06/03/2023
- Fixed bug with roc-config JSON: crorc-id wrongly set in "cru" section. Now named crorcId and to be set under "crorc" section.

## v0.42.2 - 16/03/2023
- Fix for crorId field in configuration file.
- o2-roc-list-cards is able to identify previous versions of the firmware. Their version number is displayed and they are flagged as "old", when not on the compatibility list any more.

## v0.42.3 - 28/04/2023
- Fix for roc-status --monitoring: glitchCounter unsigned.

## v0.43.0 - 24/05/2023
- Added some counters for roc-status:
  - link (with --monitoring option only): pktProcessed, pktErrorProtocol, pktErrorCheck1, pktErrorCheck2, pktErrorOversize, orbitSor (including for UL link 15)
  - onu (with --onu option, all output modes, including --monitoring): glitchCounter

## v0.44.0 - 02/06/2023
- Added some counters for roc-status:
  - link (with --monitoring option only): orbitSOR for CRORC
- Updated list of firmwares for CRORC

## v0.44.1 - 16/06/2023
- Updated list of CRORC firmwares: hash truncated to 7 chars, because CRORC does not report all 8.
- roc-status:
  - added pciAddress for user logic link 15

## v0.44.2 - 23/06/2023
- Added a workaround to avoid HW lock issues for CRORC: prevent concurrency in the calls to feed superpages to the card. Added a check to detect write discrepencies.

## v0.44.3 - 29/08/2023
- Fix registers for FEC status counters.
- Updated list of firmwares for CRORC
- CMake cleanup: removed python2 support.

## v0.45.0 - 03/10/2023
- o2-roc-pat-player: options have changed to match the [latest firmware conventions](https://gitlab.cern.ch/alice-cru/cru-fw/-/tree/pplayer/TTC#address-table). Values can specified as decimal or hexadecimal numbers.
- Added support for pattern-player configuration parsing (used by ALF).

## v0.45.1 - 24/01/2024
- o2-roc-list-cards:  get NUMA card info from system instead of PDA PciDevice_getNumaNode() function, which reports wrong node for RH8. Fix also applies to field in RocPciDevice internal class.

## v0.45.2 - 29/05/2024
- Updated list of firmwares.
- Added CRU option dropBadRdhEnabled.

## v0.45.3 - 16/07/2024
- Added some counters for roc-status:
  - link (with --monitoring option only): rdhCorruptedDropped (for convenience - this value is extracted from the existing pktErrorCheck1 field, bits [23:16]).

## v0.45.4 - 26/09/2024
- Updated list of firmwares.

## v0.45.5 - 19/12/2024
- Added internal fallback when hugeadm tool not available to setup hugepages (e.g.for RHEL9).

## v0.45.6 - 07/02/2025
- Updated list of firmwares.

## v0.45.7 - 13/03/2025
- o2-roc-config: added (temporary) option --test-mode-ORC501 to enable testing of new firmware as described in [ORC-501](https://its.cern.ch/jira/browse/ORC-501).
