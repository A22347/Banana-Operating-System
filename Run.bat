@echo off
NET SESSION >nul 2>&1
IF %ERRORLEVEL% EQU 0 (
timeout 0
) ELSE (
echo Please run this script with Administator privileges
pause
exit
)

@echo on

D:
cd D:/Users/Alex/Desktop/Banana
rm disacpica.txt
rm dislegacy.txt
rm diswsbe.txt
rm dis87.txt
rm discmd.txt
objdump -drwC -Mintel packages/banana/32/0002/acpica.sys >> disacpica.txt
objdump -drwC -Mintel packages/system/32/0002/legacy.sys >> dislegacy.txt
objdump -drwC -Mintel packages/banana/32/0001/wsbe.sys >> diswsbe.txt
objdump -drwC -Mintel packages/system/32/0002/em8087.sys >> dis87.txt
objdump -drwC -Mintel packages/system/32/0001/command.exe >> discmd.txt

cd D:/Users/Alex/Desktop/Banana/firmware/BOOT2
call build.bat

cd D:/Users/Alex/Desktop/Banana/firmware/pbe
call build.bat
rem cp "D:/Users/Alex/Desktop/Banana/firmware/pbe/firmware.lib" "D:/Users/Alex/Desktop/Banana/kernel/firmware.lib"

cp "D:/Users/Alex/Desktop/Banana/COPYRIGHT" "D:/Users/Alex/Desktop/Banana/packages/banana/32/0006/COPYRIGHT" || pause
cp "D:/Users/Alex/Desktop/Banana/LICENSE" "D:/Users/Alex/Desktop/Banana/packages/banana/32/0006/LICENSE" || pause

cd D:/Users/Alex/Desktop/Banana/packages
python create.py



cd D:/Users/Alex/Desktop/Banana/installer/FloppyRoot
xdelta3 -f -s D:/Users/Alex/Desktop/Banana/kernel/KRNL386.EXE D:/Users/Alex/Desktop/Banana/kernel/KRNL486.EXE 386TO4.DIF
xdelta3 -f -s D:/Users/Alex/Desktop/Banana/kernel/KRNL486.EXE D:/Users/Alex/Desktop/Banana/kernel/KRNL586.EXE 486TO5.DIF
cd D:/Users/Alex/Desktop/banana-os
del newimage.img
del basicimage.img






imdisk -a -f newimage.img -s 64M -m T: -p "/fs:fat32 /y /a:512" || pause

cd D:/Users/Alex/Desktop/Banana
robocopy D:/Users/Alex/Desktop/Banana/sysroot/ T:/ /E
rmdir T:\Banana\Dev32 /s /q
mkdir T:\Banana\Dev32
rmdir T:\Banana\Dev64 /s /q
mkdir T:\Banana\Dev64


cp D:/Users/Alex/Desktop/Banana/packages/banana.cab T:/Banana/Packages

rem cp D:/Users/Alex/Desktop/Banana/packages/*.cab T:/Banana/Packages
rem rm T:\Banana\Packages\devel.cab
rem rm T:\Banana\Packages\system.cab
rem rm T:\Banana\Packages\pci.cab
rem rm T:\Banana\Packages\wallpaper.cab

robocopy D:/Users/Alex/Desktop/Banana/packages/system/32/0001 T:/Banana/System /E
robocopy D:/Users/Alex/Desktop/Banana/packages/system/32/0002 T:/Banana/Drivers /E
cd kernel
copy "D:/Users/Alex/Desktop/Banana/kernel/BANANABT" "T:/Banana/BANANABT" || pause
copy "D:/Users/Alex/Desktop/Banana/kernel/FIRMWARE.LIB" "T:/Banana/FIRMWARE.LIB" || pause
rem copy "D:/Users/Alex/Desktop/Banana/kernel/KERNEL.EXE"   "T:/BANANA/System/KERNEL64.EXE" || pause
copy "D:/Users/Alex/Desktop/Banana/kernel/KERNEL32.EXE" "T:/BANANA/System/KERNEL32.EXE" || pause
copy "D:/Users/Alex/Desktop/Banana/kernel/TRAMP.EXE" "T:/Banana/System/tramp.exe" || pause
rem python ../genCoreFileBackups.py || pause

cd ../tools
copy "D:/Users/Alex/Desktop/Banana/tools/lotsof0s.bin" "T:/dummy.txt"
copy "D:/Users/Alex/Desktop/Banana/tools/lotsof0s.bin" "T:/dummy2.txt"
rm T:\dummy.txt
rm T:\dummy2.txt
copy "D:/Users/Alex/Desktop/Banana/tools/lotsof0s.bin" "T:/dummy.txt"
copy "D:/Users/Alex/Desktop/Banana/tools/lotsof0s.bin" "T:/dummy2.txt"
copy "D:/Users/Alex/Desktop/Banana/tools/lotsof0s.bin" "T:/dummy3.txt"
copy "D:/Users/Alex/Desktop/Banana/tools/lotsof0s.bin" "T:/dummy4.txt"
copy "D:/Users/Alex/Desktop/Banana/tools/lotsof0s.bin" "T:/dummy5.txt"
copy "D:/Users/Alex/Desktop/Banana/tools/lotsof0s.bin" "T:/dummy6.txt"
copy "D:/Users/Alex/Desktop/Banana/tools/lotsof0s.bin" "T:/dummy7.txt"
copy "D:/Users/Alex/Desktop/Banana/tools/lotsof0s.bin" "T:/dummy8.txt"
copy "D:/Users/Alex/Desktop/Banana/tools/lotsof0s.bin" "T:/dummy9.txt"
copy "D:/Users/Alex/Desktop/Banana/tools/lotsof0s.bin" "T:/dummyA.txt"
copy "D:/Users/Alex/Desktop/Banana/tools/lotsof0s.bin" "T:/dummyB.txt"
copy "D:/Users/Alex/Desktop/Banana/tools/lotsof0s.bin" "T:/dummyC.txt"
copy "D:/Users/Alex/Desktop/Banana/tools/lotsof0s.bin" "T:/dummyD.txt"
copy "D:/Users/Alex/Desktop/Banana/tools/lotsof0s.bin" "T:/dummyE.txt"
copy "D:/Users/Alex/Desktop/Banana/tools/lotsof0s.bin" "T:/dummyF.txt"
rm T:\dummy.txt
rm T:\dummy2.txt
rm T:\dummy3.txt
rm T:\dummy4.txt
rm T:\dummy5.txt
rm T:\dummy6.txt
rm T:\dummy7.txt
rm T:\dummy8.txt
rm T:\dummy9.txt
rm T:\dummyA.txt
rm T:\dummyB.txt
rm T:\dummyC.txt
rm T:\dummyD.txt
rm T:\dummyE.txt
rm T:\dummyF.txt

cd D:/Users/Alex/Desktop/
attrib +r "T:/Banana/BANANABT" 
attrib +r "T:/Banana/FIRMWARE.LIB" 
rem attrib +h +r "T:/JUMPER32.SYS" 
attrib +r "T:/BANANA/SYSTEM/KERNEL64.EXE" 
attrib +r "T:/BANANA/SYSTEM/KERNEL32.EXE" 
attrib "T:/BANANA/SYSTEM/TRAMP.EXE" 
imdisk -D -m T:

python Banana/join.py || pause


cd banana-os
copy newimage.img basicimage.img
cd ../


rem -serial file:log.txt

if exist Banana/qemuinhibit.txt (
    echo a
) else (
rem "C:/Program Files/QEMU/qemu-system-i386" -L "C:\Program Files\qemu" 
    qemu-system-i386 -cpu pentium3 -serial file:log3.txt -m 32 -rtc base=utc -soundhw pcspk,sb16 -d guest_errors,cpu_reset -monitor stdio -hda banana-os/newimage.img -cdrom D:/Users/Alex/Desktop/Banana/Installer/BANANA.ISO -hdb mikeos.flp
    rem -drive file="banana-os/newimage.img",id=abcdefg,if=none -device ich9-ahci,id=ahci -device ide-drive,drive=abcdefg,bus=ahci.0 
    rem -cdrom D:/Users/Alex/Desktop/banana-os/Installer/BANANA.ISO 
    rem -hda banana-os/newimage.img 
)

imdisk -a -f banana-os/newimage.img -m W:
rd /S /Q output 
robocopy W:/ output /E
imdisk -D -m W:
