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

## next version
- Added glitchCounter to roc-status (monitoring + JSON output).
- Added DMA status (enabled / disabled) to roc-status (all output styles, including monitoring).
