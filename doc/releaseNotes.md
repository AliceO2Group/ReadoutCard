# ReadoutCard release notes

This file describes the main feature changes for released versions of ReadoutCard library and tools.

## v0.38.0 - 05/09/2022
- Added release notes as part of source code.
- Added library version number. Can be retrieved at runtime with o2::roc::getReadoutCardVersion().
- Fixed bug in DMA reset. Old data from previous run could stay in the pipeline if previous process was stopped abruptly. 
