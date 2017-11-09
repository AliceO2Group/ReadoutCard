# `src/Pda`
This directory contains C++ wrapper classes around the PDA library.
It is limited in scope, only wrapping the parts of PDA that are used by ReadoutCard.

## PDA
PDA consists of a userspace library and a kernel module.

The kernel module takes care of registering buffers with the IOMMU (if enabled) and creating the scatter-gather list 
(SGL). The SGL is a list of pointers to (userspace, kernel, physical address spaces) and sizes of each physically 
contiguous segment of the buffer. 
The kernel module exposes it's functionality through sysfs files under:
`/sys/bus/pci/drivers/uio_pci_dma/[device PCI address]/dma/`

Under this directory, registration of a buffer is done by the PDA userspace library by writing a struct to the file 
`./request`, and freeing by writing the buffer ID to: `./free`

After registration, the buffer and SGL are exposed through the files: `./[buffer ID]/map` and `./[buffer ID]/sg`.

These files are memory-mapped by the PDA userspace library.

In the case of a crash, the userspace component may not be able to do the freeing. This means the kernel module will
hang onto the buffer, and the associated memory. This can be a real problem in memory-constrained environments.
The function `freeUnusedChannelBuffers()` in `Driver.h` takes care of this by detecting unused buffers and freeing them 
by writing the buffer ID into the `./free` file.

## Separation
There are plans to split this part off into a separate library, which ReadoutCard would then depend on.

The obstacle to this is that some of the exceptions thrown are somewhat ReadoutCard specific.
For example, in `PdaDmaBuffer.cxx`, one of the exceptions presents the message:
*"Scatter-gather node smaller than 2 MiB (minimum hugepage size. This means the IOMMU is off and the buffer is not 
backed by hugepages - an unsupported buffer configuration."*
While this is helpful in the context of the way it is used in ReadoutCard, it may not apply in different situations.