
C:
cd C:/Users/Alex/Desktop/Banana
git add *.*
git add COPYRIGHT
git add LICENSE
git add kernel
git add firmware
git add applications
git add bochs
git add drivers
git add doc
git add packages -f
git add sysroot
git add ip
git add disasms -f
git add installer/*.txt -f
git add installer/*.bat -f
git add installer/*.s -f
git add installer/*.py -f
git add installer/*.vbs -f
git add libraries/userclip -f

set /p commitmsg="Enter message: "
git commit -a --allow-empty-message -m "%commitmsg%"
git remote add origin https://github.com/A22347/Banana-Operating-System.git
git push -u origin main

pause