In fact, the files are named WEB.h and WEB.cpp. Also the class is named WEB.
WEB is a class to connect to Volkswagen We-Connect.
It ist based on the pythonscript "we_connect_client.py", written by Rene Boer, 
https://github.com/reneboer/python-carnet-client
Autor of WEB: Burkhard Venus
last Date: 10.02.2020
It runs right now only on linux sytems. It must be CUL/libcurl installed. Tested with libcurl 7.38. and libcurl 7.58
In the main app you must creat an object of WEB like: WEB CarNet;
You MUST create the object: CURL_Response car; in yor main app!
The opject CarNet will use the opject car to store the collected data and to take the userdata for the requests.
Before you can make a request set the userdata with std::string like:
car.User="user@host.com";
car.Passw="myPassword";
car.VIN="WVWxkjflkjsdf"; // only when needed, not working right now
car.sPin="4711"; //for future functions
car.PollIntervall=300;// intervall in seconds if you use start_thread
then you can mak a singel request:
CarNet.CARNET();// no object to store in! It will set the values in car!
or you can start a thread
CarNet.StartThread(); // the values in car will be upated. The interval is set in:
CarNet.Interval=300;// 300 is the default value. It is set as a long.
CarNet.StopThread(); // will close the thread
car.Car.successlevel will hold the success of the request, 1 to 8 are the login steps.
Bit 5 set(&16) = emanager success
Bit 6 set = get location success
Bit 7 set = get vsr success
Bit 8 and so on
CarNet.CARNET(command); // send a command to the car as a string. Possible commands are "start clima", "stop clima","start windowmelt","stop windowmelt","start charge","stop charge".

You get back a CURL_Response. CURL_Response.errCode returns the error code from the command call as a string. "0" means all ok.

I was not able to get it work, if the requested car is the second in the We-connect account.
I also did not use all functions using the sPin, because my car did't support it and I can't test it.


example:(not really tested)

#include WEB.h
main
{
  CUL_response car;//struct to hold all data. The class WEB will use "extern CUL_response car;" to get access to it
  WEB CarNet;
  car.User="user@host.com";
  car.Passw="myPassword";
  car=CarNet.CARNET(); //car.CAR will now hold all data from your car.
  std:string errCode=CarNet.CARNET("start clima").errCode;// will start climate the car if the errCode is "0".
}
