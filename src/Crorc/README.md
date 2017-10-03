# `src/Crorc`
This directory contains the classes related to the C-RORC (Common Readout and Receiver Card) driver backend.

The core of the driver is formed by the classes:
##### Constants
Contains constants, mainly related to the register layout.

##### ReadyFifo
Describes the structure of the "Ready FIFO", a data structure in the host's memory which is updated through DMA by the card
to notify of completed data transfers.

##### Crorc
Functions interacting with the registers described in `Constants`, abstracting away the lowest-level details.
This is based on old code, and the translation has been through multiple authors. Some of the meaning and understanding
has been lost in translation.

##### CrorcDmaChannel 
Contains the control and procedure logic and is where it all comes together. Its interaction with the card goes through
the `Crorc` functions.  

#### Other classes
##### CrorcBar
Implementation of `BarInterface`. In the future, this class may impose restrictions on reads and writes, but currently 
it allows everything.  
##### RxFreeFifoState 
Describes the state of the FIFO used to push addresses of data transfer targets to the card.
##### StWord
Struct describing a 32-bit word used by the C-RORC to communicate status.
