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

Implemented:
   * ACPI
   * APIC/IOAPIC
   * PCI (rudimentary)
   * AHCI (rudimentary)
   * PS/2 Keyboard/Mouse
   * very basic VMWare SVGAII support


## How to Compile

Open up vs/VSOS.sln in Visual studio 2019+ and run compile solution

## How to run

### Windows
  Install qemu in "C:\Program Files\qemu\" and run `runqemu.cmd`

## Info
heavily based on https://github.com/toddsharpe/MetalOS, https://github.com/TretornESP/bloodmoon and https://github.com/maxtyson123/MaxOS
