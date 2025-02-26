@echo off
mkdir image\efi\boot
xcopy /Y build\x64\Debug\BOOTX64.EFI image\efi\boot\
xcopy /Y build\x64\Debug\vsoskrnl.exe image\efi\boot\
xcopy /Y build\x64\Debug\vsoskrnl.pdb image\efi\boot\
"C:\Program Files\qemu\qemu-system-x86_64.exe" -L . -bios OVMF.fd -hda fat:rw:image -m 2G -smp 2 -no-reboot -no-shutdown -d cpu_reset 