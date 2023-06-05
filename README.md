# READ ME: Heat-Monitor IOT Module
This system monitors a thermocouple probe. When it registers temperature above 30°C, it displays an alert code on an OLED screen and sends a notification email to the user. 
The physical components are built upon the Adafruit FeatherWing Development boards, are coded in Arduino C, and IOT connectivity is managed by the AdafruitIO dashboard and an If This Then That (IFTTT) applet. 

Each user will need an Adafruit and IFTTT account linked to a valid and utilized email. The relationship between these accounts allow for the flow of your data through the WiFi network and into each of these services. The token used to develop this code has since been refreshed and the next user will have to update the config.h file with their own.

## Physical Setup
For this system, you will need access to a consistent source of outlet power and a stable WiFi connection. If you try to run this on Purdue or Government (i.e. USDA) WiFi you may experience security barriers, so I recommend using a personal network. The device itself is built from the following components assembled with stacking headers:

### Equipment & Materials 
 - Adafruit Feather M0 with a ATSAMD21 + ATWINC1500 as the microcontroller
 - PCF8523 RealTime Clock (RTC) connected via I2C and Wire lib with the INT/SQW pin wired to an interrupt-capable input.
 - MCP9600 Therocouple amplifier board
 - OLED FeatherWing Display screen with 128x64 resolution
 - 32GB microSD card 

The component relationships and assembly are displayed in the figure below:  ![physical layout diagram](figures\layout.png)


## High-Level Code Overview
### Software Environment Setup
The thermocouple data collection, delivery, and local storage is handled by the Arduino script flashed to the microcontroller. The thermocouple data is handed off to the Adafruit IO dashboard over WiFi, where it is displayed in realtime. An IFTTT applet is connected to this dashboard, and is triggered to send an email when the thermocouple data is outside of specified bounds. The overall logic and software flow are depicted below: 
![software layout](figures\softwarelayout.png)

In preparation, the user will need to set up an Adafruit and IFTTT account. Make sure you download the "Arduino IO Library" (along with its dependencies) from an Arduino IDE v.1.6.4+ Library Manager.   

This code was compiled in the Arduino IDE version 1.8.19. and the config.h file is based on 2018 code by Dave Astels for Adafruit Industries available though the MIT License.

### Overview of Arduino script operation
The Arduino script begins with loading the required libraries and constants used later in the code.
These are followed by user-defined-functions which streamline the data delivery and storage steps. Each are described below: 
- _String print_time(DateTime ts)_ 
    This function takes a DateTime variable and converts it into a String format suitable for printing to Serial or the SD card
- _String createNewFile()_
    This function searches the SD card for filenames until it encounters one which does not exist yet, and this String is returned. The range of "L000.csv"-"L999.csv" are vaid for filenames. This search is completed by converting int values to char values in nested for-loops. The returned filename is used to generate the local storage file. 
- _void connect_AIO()_
    This function prints the status of the connection to AdafruitIO. It is used within the function which delivers data to AdafruitIO. 
- _void tc_out(float temp)_
    This function delivers float thermocouple data to AdafruitIO. 

The setup function establishes the AdafruitIO connection, leaving it open for the rest of the script's operation. This works since the data sampling frequency of 6 seconds is frequent enough to not justify putting the board into sleep between samping. This function is also responsible for initializing the RTC & SD card, MCP9600 amplifier, OLED screen, and local storage file. 

Within each 6 second pass of the void loop, the present time is identified along with the thermocouple reading. A boolean is set based on whether the thermocouple value is below the specified threshold of 30°C (0 = within spec, 1 = outside of spec). The thermocouple value is sent to the AdafruitIO feed's dashboard, and all the data is printed to the Serial monitor and the SD card file. 

## Implementation Process

`1`. Component Assembly: 
    
Wire the MCP9600 amplifier board into the I2C ports on the bottom of the RTC & SD card board. Solder in place. Since the wire you will use is likely not sheilded, having these lines at the bottom decreases the risk interferrence with the microcontroller processing. Use a screw driver to attach the exposed ends of the thermocouple probe into the MCP9600 board. 
    
Attach the boards together by stacking the OLED, Adafruit Feather M0, and RTC & SD card reader. See the assembled unit in the figure below.
![actal setup](figures\demo1.png)

`2`. Establish an AdafruitIO Feed

Within you Adafruit account, navigate to https://io.adafruit.com  or click "IO" in the head banner to find your AIO key. This is linked to YOU, so keep this safe and if you ever regenerate it remember to update your work with the new parameter.  
![actal setup](figures\iottutorial1.png)

Go the the "Feeds" tab and select "New Feed". Give it a name and hit "Create". This will be the real-time dashboard for your WiFi-connected data stream! 
![actal setup](figures\iottutorial3.png)


`3`. Create an IFTTT Applet connected to AdafruitIO data

Take the time to make an IFTTT account, making sure to use an email you actually check. 
![actal setup](figures\iottutorial9.png)

Once you are logged in, navigate to https://ifttt.com/adafruit 

Select "Connect". 
![actal setup](figures\iottutorial10.png)

Select "Authorize". 
![actal setup](figures\iottutorial11.png)

Once you are bact at IFTTT, select "Create" to get started on your first applet. You can make 2 for free! 
![actal setup](figures\iottutorial12.png)

Specify the name of the feed you wish to pull data from, the relationship you want to monitor and the threshold. All these details together constitute as the "trigger" for the applet. 
![actal setup](figures\iottutorial13.png)

Now you specify the "action" of the applet. In our case, we want to send an email. Provide a subject line and verbage for the body. Select "Create action".
![actal setup](figures\iottutorial16.png)


`4`. Microcontroller status and Arduino code 
### _Firmware Update_

Make sure the firmware and SSL certificates for the Adafruit Feather M0 are up to date to connect to WiFi. This process is covered in more depth in this tutorial (https://learn.adafruit.com/adafruit-feather-m0-wifi-atwinc1500/using-the-wifi-module
), but the quick summary is you will need to: 
- Flash the board with the WiFi101 example script "CheckWifiFirmwareVersion". Define the "WiFi.setPins()" in setup beforehand. The Serial print output will inform if you need to update your board
- Flash the board with the WiFi101 example script "FirmwareUpdater". Define the "WiFi.setPins()" in setup beforehand.   

![actal setup](figures\iottutorial7.png)

- Open the "WiFi101 / WiFNINA Firmware/Certificates Updater" from "Examples". With a stable microUSB connection, select the COM the board is using and "Test connection". 
- Next, select "Add domain" and include io.adafruit.com to fetch the SSL certificate. 
- Select "Upload Certificates to WiFi module". 

![actal setup](figures\iottutorial8.png)

### _Software Update_

The user will need to update the provided Adruino scripts for this project to work on their system. 

If you plan on adding boards to this system, you may need to evaluate which pins are defined to which sensors as clashes have been documented to happen when defaults are left unchanged.

Update the function _tc_out()_ with the name of your AdafruitIO feed you specified earlier: 
![actal setup](figures\iottutorial4.png)

Update the config.h file with your relevant WiFi access and AIO key parameters. 
![actal setup](figures\iottutorial2.png)

A problem that can occur when you are updating this file is buried in the Windows preferences--file extensions can be hidden from the name. When this happens, it can mask whether you are actually editing the '.h' file. 
![actal setup](figures\iottutorial6.png)

Go into the prefrences for 'Folder Options->View' and unselect.

![actal setup](figures\iottutorial5.png)


    Flash the board.  

## Demo & Output

In the following gif, I demonstrate: 
-  How the assembled unit returns a boolean indicating the thermocouple probe's temperature is within spec on the OLED screen while the AdafruitIO dashboard logs incoming temperatures. 
- After I put the thermocouple into my hot tea, I show how the AdafruitIO dashboard registers the spike in temperature. 
- Simultanously, the alert boolean on the OLED screen shifts from 0 to 1. 
- Finally I pan to my email where I open a notification that just arrived telling me that the registered temperature was outside my defined threshhold. 
 
![demo gif](figures\IMG_1108.gif)

Since the system was recording data locally as well, I can visualize this session to get the following plot: 

![alert annotated temp plot](figures\tempimage.png)



## Future Work and Next Steps
This system can be improved in the following ways: 
- A physical, realtime temperature and alert plot could be accomplished on a larger LCD screen in addition to live-transmitting the data over WiFi
- Data could be centralized to a MySQL database which could make a real-time interface more nuanced with a tool like Grafana
- OLED screen buttons could be used to enable and disable alerts manually or even reset the alert threshold with the A, B, and C buttons