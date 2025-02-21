# Very simple Operating System (VSOS)

This is early work in progress.

### Boot
The main purpose of Boot is to load the ``Kernel``, however it must also:
* Detect Graphics Device from UEFI (using Graphics Output Protocol)
* Allocate Page Table Pool for Kernel
* Allocate Page Frame Number Database for Kernel's PhysicalMemoryManager
* Allocate and load Kernel's PDB into physical memory (to allow for bugcheck stack walks)

See also: [Loader Params](src/LoaderParams.h)

### Kernel
Monolithic preemptive kernel. 

Quick Notes:
* UEFI Runtime is mapped into Kernel address space, allowing runtime services to be called


## Info
heavily based on https://github.com/toddsharpe/MetalOS