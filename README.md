# ble-morse
Bluetooth Low Energy interface for morse key. Can be used with 
[iOS IRMC](https://github.com/morse-code-over-ip/irmc-ios).
This is a prototype under development...

This is an implementation of a [MOIP](https://github.com/morse-code-over-ip/moip)
client.

## Compatibility:
ble_morse 0.2 and ios-irmc 0.2 (ext branch)
ble_morse 0.4 and ios-irmc 0.4

![Hello World](/img/hello_morse.jpg?raw=true "Hello world")

Follow the instructions roughly given in install.txt.
The arduino code is located in src.

## Why include binary files?
Because files vanish (i.e. N810 produced 2008, firmwares all gone from the web in 2014).

Based on the following codes:
https://github.com/adafruit/Bluefruit_LE_Connect

## Code Quality
This is experimental code.




# Hardware Requirements:
* radino nRF8001
* radino Leonoardo (4)

WIRING:
http://arduino.cc/en/Tutorial/Button (pins 12 and 13)

COMPORT Drivers:
http://www.ftdichip.com/Drivers/VCP/MacOSX/FTDIUSBSerialDriver_v2_2_18.dmg

Software Requirements:
* Arduino IDE (1.0.5): http://arduino.googlecode.com/files/arduino-1.0.5-macosx.zip

# Installing Radino Libs and HW

Remarks on the installation process:
On OSX the folder for the Arduino files is ~/Documents/Arduino
Follow the instructions given in (2) to 
assert: it is possible to detect the programmer and upload a sketch.
Custom HW boards will be added using an interface (1)

USB modem interface here: /dev/tty.usbmodem1411

Testing the UART:
nRF UART App https://itunes.apple.com/de/app/nrf-uart/id614594903?mt=8
with radione_nRF8001_IO_HF_USB_Test


(2) doc/305000074A_radino_nRF8001.pdf
(4) Radino Leonardo: http://shop.in-circuit.de/product_info.php?cPath=22_28&products_id=35
(5) Wiki: http://wiki.in-circuit.de/index.php5?title=radino_Modules
(6) Radino Support Files: http://wiki.in-circuit.de/index.php5?title=radino_Leonardo#Downloads


