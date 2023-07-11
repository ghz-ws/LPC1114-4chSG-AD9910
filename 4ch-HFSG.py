import serial
inst=serial.Serial("COM8",115200)   ##9859->COM8, 9910->COM9
Freq=[100,100,100,100]  ##MHz unit
Pha=[0,0,0,0]               ##deg. unit
Ampl=[-10,-10,-10,-10]  ##dBm unit. 50ohm loaded.

buf=f'{Freq[0]*1000000:09}'+f'{Pha[0]:03}'+f'{Ampl[0]:+03}'+f'{Freq[1]*1000000:09}'+f'{Pha[1]:03}'+f'{Ampl[1]:+03}'+f'{Freq[2]*1000000:09}'+f'{Pha[2]:03}'+f'{Ampl[2]:+03}'+f'{Freq[3]*1000000:09}'+f'{Pha[3]:03}'+f'{Ampl[3]:+03}'
print(buf)
inst.write(buf.encode())
print('Ch1 Freq=',Freq[0],'MHz, Pha=',Pha[0],'deg., Ampl=',Ampl[0],'dBm\nCh2 Freq=',Freq[1],'MHz, Pha=',Pha[1],'deg., Ampl=',Ampl[1],'dBm\nCh3 Freq=',Freq[2],'MHz, Pha=',Pha[2],'deg., Ampl=',Ampl[2],'dBm\nCh4 Freq=',Freq[3],'MHz, Pha=',Pha[3],'deg., Ampl=',Ampl[3],'dBm\n')