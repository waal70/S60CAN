# S60CAN
Arduino CANBUS Volvo S60

An adaptation of SardineCAN:
The code-base for this is in github.com/hackingvolvo, thanks to Olaf @ hackingvolvo.blogspot.com

The reason I started this version is because of the DPF (soot filter) and regeneration issues. The goal is to provide the driver with a simple feedback whenever the car is regenerating its DPF. The driver can then make the decision to keep on driving and let the process finish, or to cut the process short and run the risk of eventually getting messages such as:
"Emissions problem. Service required"
"DPF full. Regeneration required"

The changes in this version are specific for later years Volvo and include:

* Using the SD card reader/writer on the SKPANG board to log messages
* Using a connected LCD screen to display status messages (Funduino 16x2 LCD, 20x4 from Oil & Boost versions!)
* Using hardware filtering capabilities of MCP2515 (it's a must with a lot going on)
* Connect to LS and HS CAN 
* Easier mode-switching (for loopback, listenonly and normal mode)

You are welcome to use and expand on this software ON YOUR OWN RISK!

Connecting to the HS-CAN means you are possibly interfering with "under the bonnet" components (ECM, BCM, TCM) that might possibly affect the configuration and safety of your car.
IF YOU DO NOT KNOW WHAT YOU ARE DOING, DO NOT ATTEMPT TO MODIFY YOUR CAR!

SETUP:
1x Arduino Uno R3. The DIP-chip version, NOT the surface mounted one. If that should be relevant :)
1x SKPang CANBUS shield, including SD card reader
1x OBD -> serial female connector (DB9F)
1x 20x4 LCD, i2c (the four lead version: VCC, GND, SDA & SCL)
Optional:
For logging purposes: a FAT16 or FAT32 formatted micro SD card
ATTENTION: I have developed and tested with an SD-card always present. Because the SD communication
also uses the SPI-bus (as does the CAN-interface), I cannot guarantee flawless operation WITHOUT an SD-card present.

LIBRARIES:
You will need the following libraries to succesfully run this project:
1) Canbus (included in this repository)
2) LiquidCrystal_I2C (publicly available everywhere, I'm using Francisco Malpartida's version from 20/08/11)
3) TimerOne (https://github.com/PaulStoffregen/TimerOne)
Included in this repository is a makeJunction.bat. I suggest you keep the Canbus library in the same directory as the base for this git-repo. Through makeJunction.bat you can then create a symbolic link from within the Arduino libraries folder to this folder. In this way, you will keep all the source-code nicely bundled, and it will save you from copying library files hence and forth

CONFIGURATION:
Look up the proper communication speeds for your modelyear car. Mine, a P2 S60 of model year 2009, uses
125kbps for the low-speed CANBUS and
500kbps for the high-speed CANBUS.
In the setup() routine, this speed is set for the remainder of the session, so change this for your case.
This HS-CAN is what you will need for the important stuff: DPF temperature monitoring.

Between different model years, this speed changes. Configuring an incorrect speed will definitely upset your
setup and could possible even DAMAGE YOUR CAR. Always check and double-check your vehicle's communication speeds
before moving on.
