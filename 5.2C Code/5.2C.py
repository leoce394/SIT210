from tkinter import *
import tkinter.font
from gpiozero import LED
from gpiozero import PWMLED
import RPi.GPIO
RPi.GPIO.setmode(RPi.GPIO.BCM)

livingRoom = PWMLED(17)
bathroom = LED(18)
closet = LED(22)

value = float(1)
on = False

win = Tk()
win.title("LED Toggler")
myFont = tkinter.font.Font(family = 'Helvetica', size = 12, weight = 'bold')

def sliderValue(val):
    global value, on
    value = float(val)/10
    if not on:
        return
    else:
        livingRoom.value = value

def livingRoomToggle():
    global value, on
    on = not on
    if not on:
        livingRoom.value = 0
    else:
        livingRoom.value = value
        
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

slider = Scale(win, from_=0,to=10, orient = "horizontal",length=300,width=20,sliderlength=40, command = sliderValue)
slider.grid(row=3,column=0)
