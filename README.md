# BPT
## Hardware and Schematic:
TODO - Include detailed schematic of the entire setup.

## Using the Code
The `rf_gen/rg_gen.ino` file is responsible for allowing the Arduino to interface with the ADF4351 board. Assuming the hardware is wired up correctly, you will need to run the code onto the Arduino and then open up the serial monitor. This can be done be navigating to **Tools->Serial Monitor** in the Arduino IDE.  

Once the code is uploaded, you can select from two modes:  
1. Manual - After typing an **m** into the serial monitor, we can type in the desired frequncies for the first and second ADF4351 boards through the serial monitor.
2. Sweep - AFter typing an **s** into the serial monitor, we can type in the lowest frequency, the highest frequency, and the incremenation value through the serial monitor. This will then sweep through the specified range maintaining a difference of 127.8 MHz between the two boards.

TODO - Add serial interface for `RFPWR` and `PDFRFout` values.

## Resources:
[ADF4351 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ADF4351.pdf)  
[Original Code](http://f6kbf.free.fr/html/ADF4351%20and%20Arduino_Fr_Gb.htm)  

### ![Arduino Pinout](pro-mini-pinout.png)
