import serial

def ports():
    
    ports = ["COM%s" % (i+1) for i in range(255)]
    
    for p in ports:
        try:
            s = serial.Serial(p, 9600)
            s.close()
            print(p)
            print(s.name)
            print()
        except:
            pass
