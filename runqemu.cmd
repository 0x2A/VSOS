@echo off
mkdir image\efi\boot
xcopy /Y build\x64\Debug\BOOTX64.EFI image\efi\boot\
xcopy /Y build\x64\Debug\vsoskrnl.exe image\efi\boot\
xcopy /Y build\x64\Debug\vsoskrnl.pdb image\efi\boot\
"C:\Program Files\qemu\qemu-system-x86_64.exe" -L . -machine q35 -vga virtio -drive if=pflash,format=raw,file=OVMF.fd -monitor vc:1280x768 -drive id=disk,file=fat:rw:image,if=none -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 -m 2G -smp 2 -no-reboot -d cpu_reset 