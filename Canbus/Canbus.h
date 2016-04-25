/**
 * CAN BUS
 *
 * Copyright (c) 2010 Sukkin Pang All rights reserved.
 */

#ifndef canbus__h
#define canbus__h

#define CANSPEED_125 	7		// CAN speed at 125 kbps
#define CANSPEED_250  	3		// CAN speed at 250 kbps
#define CANSPEED_500	1		// CAN speed at 500 kbps

class CanbusClass
{
  public:

	CanbusClass();
    char init(unsigned long speed);
	char setMode(unsigned int mode);
	char getMode();
	char* getDisplayFilter();
private:
	char isSupportedBaudrate(unsigned long speed);
	
};
extern CanbusClass Canbus;

#endif
