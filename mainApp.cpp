/* This ist only a example how to use the class WeConnect with threads
In WeConnect::theThread you can see how to use it in serial prozess.
The threaded way is thread save using a own lock variable "request_in_progress".
The code is not final, I am waiting for my ID.4 for further checkings and cleaning up. */


#define maxFahrzeuge 8
#include "mainApp.h"

mainApp::mainApp()
{
        WeConnectValues = new sWeConnectValues[maxFahrzeuge];// allocate memory for pointer defined in mainApp.h. 
}
void mainApp::readCars()
{
#ifdef debug
    printf ("readCars main\n");
#endif // debug
    for(int i=0; i<maxFahrzeuge; i++)
    {
        if(FahrzeugSettings[i].istOnline && FahrzeugSettings[i].FahrzeugAktiv)// FahrzeugSettings is a class only holding the settings. bool "istOnline"=has online capabillity. bool "FahrzeugAktiv" do requests. 
        {
            if( !WeConnectValues[i].thread_runing)// only start thread once
            {
                WeConnectValues[i].User=FahrzeugSettings[i].User;
                WeConnectValues[i].PassWd=FahrzeugSettings[i].Passwort;
                WeConnectValues[i].actualFin=FahrzeugSettings[i].VIN;
                WeConnect.init(&WeConnectValues[i],&request_in_progress);
                WeConnect.setUser(FahrzeugSettings[i].User,FahrzeugSettings[i].Passwort);
//               printf("readCars call Fahrzeug %i: User: %s, Passwort: %s,VIN: %s\n",i,FahrzeugSettings[i].User.c_str(),FahrzeugSettings[i].Passwort.c_str(),FahrzeugSettings[i].VIN.c_str());
                WeConnect.StartThread(i);// the collected Date will be stored in "WeConnectValues[i]" which is a pointer
            }
        }
        else if( WeConnectValues[i].thread_runing)// if the settings have changed and thread ist still running
        {
            WeConnect.init(&WeConnectValues[i],&request_in_progress);
            WeConnect.StopThread(i);
        }
    }
#ifdef debug
    printf ("readCars main Ende\n");
#endif // debug

}
