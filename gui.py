# -*- coding: cp1252 -*-
import setpoint
import threading
from PySimpleGUI import PySimpleGUI as sg

sg.theme('Reddit')

layout = [
    [sg.Text('Sistema de Transferencia')],
    [sg.Text('Set-Point:'),sg.Input(key='sp',size=(3,1)),sg.Text('ºC')],
    [sg.Text('Temperatura:'),sg.Text("", size=(3, 1), key='temperatura'),sg.Text('ºC')],
    [sg.Button('ON', key='ON', bind_return_key=True),sg.Button('OFF', key='OFF')],
    [sg.Text('* Ornelas Corporation *')],
    [sg.Text('(revision by Thiago Mattos)')]
    ]

janela = sg.Window('Set-Point',layout)
started = False

def serial_listener():
    while True:
        while not setpoint.waiting():
            pass
        
        msg = setpoint.recvFromArduino()

        if msg.find("Arduino is ready") != -1:
            print(msg)
            started = True
        else:
            if msg.find("sp") != -1:
                s = msg.split(",")
                cicle, temp = s[1], s[2]
                janela['temperatura'].update(temp)
                if cicle == '0':
                    janela['ON'].update(button_color='grey')
                    janela['OFF'].update(button_color=sg.theme_button_color_background())
                else:
                    janela['ON'].update(button_color=sg.theme_button_color_background())
                    janela['OFF'].update(button_color='grey')

threading.Thread(target=serial_listener, daemon=True).start()

while True:
    event, values = janela.read()
    print(event)

    if event == sg.WINDOW_CLOSED or event == 'Cancel':
        break

    if event == 'ON':
        data = '<sp,1,%s>' % values['sp']
        setpoint.sendToArduino(data)

    if event == 'OFF':
        data = '<sp,0,%s>' % values['sp']
        setpoint.sendToArduino(data)
    print("OK")

window.close()

