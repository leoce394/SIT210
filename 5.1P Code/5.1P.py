from tkinter import *
import tkinter.font
from gpiozero import LED
import RPi.GPIO
RPi.GPIO.setmode(RPi.GPIO.BCM)

livingRoom = LED(17)
bathroom = LED(18)
closet = LED(22)

win = Tk()
win.title("LED Toggler")
myFont = tkinter.font.Font(family = 'Helvetica', size = 12, weight = 'bold')

def livingRoomToggle():
    if livingRoom.is_lit:
        livingRoom.off()
    else:
        livingRoom.on()
        
def bathroomToggle():
    if bathroom.is_lit:
        bathroom.off()
    else:
        bathroom.on()
        
def closetToggle():
    if closet.is_lit:
        closet.off()
    else:
        closet.on()

livingRoomButton = Button(win, text = 'Living Room', font = myFont, command = livingRoomToggle, bg = 'bisque2', height = 1, width = 24)
livingRoomButton.grid(row=0,column=0)
bathroomButton = Button(win, text = 'Bathroom', font = myFont, command = bathroomToggle, bg = 'bisque2', height = 1, width = 24)
bathroomButton.grid(row=1,column=0)
closetButton = Button(win, text = 'Closet', font = myFont, command = closetToggle, bg = 'bisque2', height = 1, width = 24)
closetButton.grid(row=2,column=0)
