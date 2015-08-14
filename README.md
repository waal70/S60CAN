# S60CAN
Adapting SardineCAN for the MY09 S60

The code-base for this is in github.com/hackingvolvo, thanks to Olaf @ hackingvolvo.blogspot.com

The reason I started this version is because of the DPF (soot filter) and regeneration issues. The goal is to provide the driver with a simple feedback whenever the car is regenerating its DPF. The driver can then make the decision to keep on driving and let the process finish, or to cut the process short and run the risk of eventually getting messages such as:
"Emissions problem. Service required"
"DPF full. Regeneration required"

The changes in this version are specific for later years Volvo and include:

* Using hardware filtering capabilities of MCP2515 (it's a must with a lot going on)
* Connect to LS and HS CAN 
* Easier mode-switching (for loopback, listenonly and normal mode)

You are welcome to use and expand on this software ON YOUR OWN RISK!

Connecting to the HS-CAN means you are possibly interfering with "under the bonnet" components (ECM, BCM, TCM) that might possibly affect the configuration and safety of your car.
IF YOU DO NOT KNOW WHAT YOU ARE DOING, DO NOT ATTEMPT TO MODIFY YOUR CAR!
