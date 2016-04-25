/**
 * CAN BUS
 *
 * Copyright (c) 2010 Sukkin Pang All rights reserved.
 */

#ifndef canbus__h
#define canbus__h

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
