#include "ljack.h"

ljack::ljack(string device,string connection_type,int serial_nb)
/*
 * Description
 * ===========
 * 	Will try to establish a connection with the labjack.
 * 	If successfull, you can check the status of the connection
 * 	with the connected member.
 */ 
{
	if (LJM_OpenS(device.c_str(), connection_type.c_str(), to_string(serial_nb).c_str(), &handle)!=0)
	{
		connected=false;
	}
	else {
		connected=true;
	}

}
ljack::~ljack()
/*
 * Description
 * ===========
 * 	Disconnect from the labjack. 
 * 
 * NOTE
 * ----
 * 	Very important if you are using the USB connection.
 */ 
{
	if (connected){
		LJM_Close(handle);
	}
}
bool ljack::read_DIO(std::string dio_name)
{
    double v=0;
    if (LJM_eReadName(handle, dio_name.c_str(), &v)!=0)
	{
		printf("[WARNING] !!!!!! wrong DIO Name\n");
        return false;
	}
    if (v<1.0){return false;}
    return true;
}
int ljack::read_temperature(temperature *tmp)
/*
 * Description
 * ===========
 * 	Read the limit switch status. If you want to 
 * 	fetch the value use the limit structure lim.
 * NOTE:
 * -----
 * 	If an error (!=0) is return, the values are not 
 * updated
 * 
 * Return
 * ------
 *  0 -> success, anything else == error
 */ 
{
	double v=0;
	//set default values for the temperature
	
	tmp->nutec = -999;
	tmp->rm8=-999;
	tmp->TTM=-999;
	tmp->bench=-999;
	tmp->nose=-999;
    tmp->base=-999;
    tmp->nutec_ctrl=-999;
	if (!connected){return 1;}
	
	if (LJM_eReadName(handle, "AIN1", &v)!=0)
	{
		return -1;
	}
	else {tmp->TTM=lmt85_2_temperature(v);}
	
	if (LJM_eReadName(handle, "AIN2", &v)!=0)
	{
		return -1;
	}
	else {tmp->bench=lmt85_2_temperature(v);}
	
	if (LJM_eReadName(handle, "AIN3", &v)!=0)
	{
		return -1;
	}
	else {tmp->nutec=lmt85_2_temperature(v);}
	if (LJM_eReadName(handle, "AIN0", &v)!=0)
	{
		return -1;
	}
	else {tmp->rm8=lmt85_2_temperature(v);}
	if (LJM_eReadName(handle, "AIN5", &v)!=0)
	{
		return -1;
	}
	else {tmp->nose=lmt85_2_temperature(v);}
    if (LJM_eReadName(handle, "AIN6", &v)!=0)
	{
		return -1;
	}
	else {tmp->nose=lmt85_2_temperature(v);}
    if (LJM_eReadName(handle, "AIN5", &v)!=0)
	{
		return -1;
	}
	else {tmp->base=lmt85_2_temperature(v);}
    if (LJM_eReadName(handle, "AIN7", &v)!=0)
	{
		return -1;
	}
	else {tmp->nutec_ctrl=lmt85_2_temperature(v);}
	
    
    
    
    return 0;
}

double lmt85_2_temperature(double voltage)
{
	double B = pow( pow(8.194,2)+ 0.01048 * (1324.0 - voltage*1000.0)    ,0.5);
	double A = ( 8.194- B ) / -0.00524;
	return A+30;
}


void read_temps_t(instHandle *handle)
{
	std::string model="T7",mode="Ethernet";
	int serial=470015647;
	ui_get_string("/opt/HiCIBaS/config/network.conf","LJMODEL",&model);
	ui_get_int("/opt/HiCIBaS/config/network.conf","LJSERIAL",&serial);
	ui_get_string("/opt/HiCIBaS/config/network.conf","LJMODE",&mode);
	
	ljack myLabJack(model,mode,serial);	
	if (!myLabJack.connected)
	{
		handle->tcs->tcs_tel->devices = handle->tcs->tcs_tel->devices & 127;
		printf("\n::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
		printf(":::   [!!!Warning!!!] Labjack is  not connected.   :::\n");
		printf("::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
		return ;
	}
	else{
		handle->tcs->tcs_tel->devices = handle->tcs->tcs_tel->devices | 128;
		
		printf("Lim. model: %s\n",model.c_str());
		printf("Lim. serial: %d\n",serial);
		printf("Lim. mode: %s\n",mode.c_str());
		}
	int err=0;
	temperature T;
	while(1)
	{
		if (myLabJack.read_temperature(&T)!=0)
		{
			break;
		}
		handle->tcs->tcs_tel->T1 = T.TTM;
		handle->tcs->tcs_tel->T2 = T.rm8;
		handle->tcs->tcs_tel->T3 = T.bench;
		handle->tcs->tcs_tel->T4 = T.nutec;
		handle->tcs->tcs_tel->T5 = T.nose;
        handle->tcs->tcs_tel->T6 = T.base;
        handle->tcs->tcs_tel->T7 = T.nutec_ctrl;
		std::this_thread::sleep_for (std::chrono::seconds(1));	
	}
	handle->tcs->tcs_tel->devices = (handle->tcs->tcs_tel->devices & 127);
	return ;
}

int ljack::dio_1(std::string dio_name)
{
    if (!connected)
    {return -1;}
    if (LJM_eWriteName(handle, dio_name.c_str(),1)!=0)
    {return -1;}
    return 0;
}
int ljack::dio_0(std::string dio_name)
{
    if (!connected)
    {return -1;}
    if (LJM_eWriteName(handle, dio_name.c_str(),0)!=0)
    {return -1;}
    return 0;
}
