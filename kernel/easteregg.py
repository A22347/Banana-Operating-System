
data1 = open('D:/Users/Alex/Desktop/Banana/kernel/KERNEL32.EXE', 'rb').read().decode('latin-1') #+=
data2 = open('D:/Users/Alex/Desktop/Banana/kernel/KRNLP2.EXE', 'rb').read().decode('latin-1') #+=

data1 = bytearray(data1.encode('latin-1'))
data2 = bytearray(data2.encode('latin-1'))
msg = "\n\n  This software is dedicated to N. Inglis\n\n"

for i in range(len(msg)):
    data1[0x100 + i] = ord(msg[i])
    data2[0x100 + i] = ord(msg[i])

open('D:/Users/Alex/Desktop/Banana/kernel/KERNEL32.EXE', 'wb').write(data1)
open('D:/Users/Alex/Desktop/Banana/kernel/KRNLP2.EXE', 'wb').write(data2)

