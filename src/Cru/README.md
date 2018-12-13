# `src/Cru`
This directory contains the classes related to the CRU (Common Readout Unit) driver backend.
For information on the way the CRU's DMA engine works, the registers, etc., see the documentation attached to the 
following repository: https://gitlab.cern.ch/alice-cru/pciedma

##### Common
A collection of data structures and functions used by the CRU classes.

##### Constants
Contains constants, including register addresses and relevant parameters used for interfacing with the card. 

##### CruBar
Implementation of `BarInterface`. Handles interacting with the registers described in `Constants`, abstracting away the lowest-level details.

##### CruDmaChannel 
Class that contains the control and procedure logic of the DMA transfers. It makes use of the `CruBar`.

##### FirmwareFeatures
Describes which features may or may not be enabled on the CRU, since some firmwares do not implement all features. 
`CruDmaChannel` uses it to check whether certain operations are allowed. 

##### DataFormat
Contains a preliminary description of the CRU's data format.

#### CRU Configuration
##### DatapathWrapper
Class that implements functions related to the configuration of the CRU Datapath Wrapper.

##### Gbt
Class that implements functions related to the configuration of the CRU GBT module.

##### I2c
Class that implements operations on the CRU I2C bus.

##### Ttc
Class that implements operations related to the configuration of TTC.
