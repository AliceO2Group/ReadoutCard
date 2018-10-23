# `src/Cru`
This directory contains the classes related to the CRU (Common Readout Unit) driver backend.
For information on the way the CRU's DMA engine works, the registers, etc., see the documentation attached to the 
following repository: https://gitlab.cern.ch/alice-cru/pciedma

The core of the driver is formed by the classes:
##### Constants
Contains constants, mainly related to the register layout.

##### CruBar
Implementation of `BarInterface`. Handles interacting with the registers described in `Constants`, abstracting away the lowest-level details.

##### CruDmaChannel 
Contains the control and procedure logic and is where it all comes together. Its interaction with the card goes through
the `CruBar`.  

##### FirmwareFeatures
Describes which features may or may not be enabled on the CRU, since some firmwares do not implement all features. 
`CruDmaChannel` uses it to check whether certain operations are allowed. 

#### Other classes
##### DataFormat
Contains a preliminary description of the CRU's data format.
