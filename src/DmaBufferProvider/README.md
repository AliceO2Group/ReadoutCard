# `src/DmaBufferProvider`
This directory contains the classes that ecapsulate creation of DMA buffers and the handling of their scatter-gather 
lists.

The `PdaDmaBufferProvider` and `FilePdaDmaBufferProvider` are used for real DMA buffers from memory regions or
memory-mapped files, registered with PDA.
The `NullDmaBufferProvider` may be used to instantiate a `DmaChannel` without a real buffer, e.g. for testing
purposes.