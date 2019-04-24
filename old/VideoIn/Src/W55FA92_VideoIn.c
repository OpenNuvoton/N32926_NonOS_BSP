#include "W55FA92_VideoIn.h"
extern VINDEV_T nvt_vin0;
extern VINDEV_T nvt_vin1;
INT32 register_vin_device(UINT32 u32port, VINDEV_T* pVinDev)
{
	if(u32port==1)
		*pVinDev = nvt_vin0;
	else if(u32port==2)
		*pVinDev = nvt_vin1;
	else 	
		return -1;
	return Successful;	
}
