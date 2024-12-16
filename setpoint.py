import serial

port = "COM3"
ser = serial.Serial(port, 9600, write_timeout=5)

startMarker = 60
endMarker = 62

def sendToArduino(sendStr):
    print("Sent: " + sendStr)
    try:
        ser.write(sendStr.encode('utf-8'))
    except Exception as e:
        print(e)

def recvFromArduino():
    ck = ""
    x = "z" # any value that is not an end- or startMarker
    
    # wait for the start character
    while  ord(x) != startMarker: 
        x = ser.read()
    
    # save data until the end marker is found
    while ord(x) != endMarker:
        if ord(x) != startMarker:
            ck = ck + x.decode("utf-8")
        x = ser.read()
    
    print("Recieved: " + ck)
    return ck

def waiting():
    return not (ser.inWaiting() == 0)

def waitForArduino():
    # wait until the Arduino sends 'Arduino Ready' - allows time for Arduino reset
    # it also ensures that any bytes left over from a previous message are discarded
    msg = ""
    while msg.find("Arduino is ready") == -1:

        while ser.inWaiting() == 0:
            pass
        
        msg = recvFromArduino()

        print(msg)
        print()
