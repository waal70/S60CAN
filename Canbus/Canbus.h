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
    char init(unsigned char);
	char isSupportedBaudrate(unsigned char speed);
	char setMode(unsigned int mode);
private:
	
};
extern CanbusClass Canbus;

#endif
