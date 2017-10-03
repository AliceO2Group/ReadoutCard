# `src/Pda`
This directory contains C++ wrapper classes around the PDA library.
It is limited in scope, only wrapping the parts of PDA that are used by ReadoutCard.

There are plans to split it off into a separate library, which ReadoutCard would then depend on.

The obstacle to this is that some the exceptions thrown are somewhat ReadoutCard specific.
For example, in `PdaDmaBuffer.cxx`, one of the exceptions presents the message:
*"Scatter-gather node smaller than 2 MiB (minimum hugepage size. This means the IOMMU is off and the buffer is not 
backed by hugepages - an unsupported buffer configuration."*
While this is helpful in the context of the way it is used in ReadoutCard, it may not apply in different situations.