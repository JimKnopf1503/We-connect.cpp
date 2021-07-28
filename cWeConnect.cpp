#include "cWeConnect.h"
#undef debug
//#define debug  // erzeugt Debugausgaben im Terminalfenster

/*
    Klasse und Hilfsfunktionen um WeConnect Daten auszutauschen
    Code zur Verwendung im eigenen Programm:
    #include cWeConnect.h
    sWeConnectValues *WeConnectValues; // Zeiger auf Struct der auszutauschenden Daten
    WeConnectValues= new sWeConnectValues; // Speicher zum Zeiger reservieren
    cWeConnect WeConnect(WeConnectValues); // Objekt erzeugen, dabei Zeiger für den Struct mitgeben. Erforderlich für threaded Aufrufe um Zugriff auf die Daten zu haben.
    Author: Burkhard Venus , burkhard @bvenus.de
    Basierend auf dem Python script von Trocotronic https://github.com/trocotronic/weconnect
    und TA2k https://github.com/TA2k/ioBroker.vw-connect
*/
int dodebug=1;
cWeConnect::cWeConnect()
{
    //ctor
    CarBrand="VW";// Standardwerte setzen
    Country="DE";
//    printf("cWeConnect Objekt erzeugt.\n");
}
void cWeConnect::init(sWeConnectValues *WeConnectValuesMain,bool *request_in_progressMain)
{
    //ctor
    WeConnectValues=WeConnectValuesMain;// Pointer der erzeugenden Instanz kopieren
    request_in_progress=request_in_progressMain;
    WeConnectValues->PollIntervallFast=60;
    WeConnectValues->PollIntervallMiddle=3600;
    WeConnectValues->PollIntervallSlow=86400;
    WeConnectValues->CarInfo.SOC=-1;

}
cWeConnect::~cWeConnect()
{
    //dtor
//   WeConnectValues->m_stopThread=true;
}

void cWeConnect::StartThread(int i)
{
    if(!WeConnectValues->thread_runing)// only run once
    {
 //       printf("Thread %i starten: User: %s, Passwort: %s, VIN: %s\n",i,WeConnectValues->User.c_str(),WeConnectValues->PassWd.c_str(),WeConnectValues->actualFin.c_str());
        WeConnectValues->m_stopThread = false;//m_stopCarNet is checked in the thread loop

        m_thread[i] = std::thread( [=] { cWeConnect::theThread(i,WeConnectValues->User,WeConnectValues->PassWd,WeConnectValues->actualFin); } );
        printf("Thread %i gestartet\n",i);
  //      while (!WeConnectValues->thread_runing) {}

    }
}
void cWeConnect::StopThread(int i)
{
    if(WeConnectValues->thread_runing)
    {
        printf("Thread %i stoppen: \n",i);
        WeConnectValues->m_stopThread=true;// tell the thread loop to exit
  //      while (WeConnectValues->thread_runing) {}
        if (m_thread[i].joinable())
            m_thread[i].join();
        printf("Thread %i gestoppt\n",i);
    }
}
void cWeConnect::theThread(int i,std::string User,std::string PassWd,std::string VIN)
{
    time_t lastPollSlow=0;// do the first poll directly
    time_t lastPollMiddle=0;// do the first poll directly
    time_t lastPollFast=0;// do the first poll directly
    extern CURL_Response car;//get access to the struct in the main app

    cWeConnect WeConnect;// create ojbect
    WeConnect.WeConnectValues=WeConnectValues;
    WeConnect.init(WeConnectValues,request_in_progress);
    WeConnect.setUser(User,PassWd);
 //   WeConnectValues->actualFin=VIN;
 //   printf("TheThread %i:\nUser: %s, Passwort: %s, VIN: %s\n", std::this_thread::get_id(),WeConnectValues->User.c_str(),WeConnectValues->PassWd.c_str(),WeConnectValues->actualFin.c_str());
    WeConnectValues->thread_runing=true;// indicate that the thread is running
    while(!WeConnect.WeConnectValues->m_stopThread)//m_stopCarNet is set by StartThread and StopThread
    {
        if ((long)lastPollSlow+(long)WeConnectValues->PollIntervallSlow<=time (NULL)&&!*request_in_progress)// only if the PollIntervall seconds have passed
        {
            while (*request_in_progress&&!WeConnect.WeConnectValues->m_stopThread)
            {
   //             printf("Warte ..\n");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
            if (!*request_in_progress)
            {


                *request_in_progress=true;
                lastPollSlow=time (NULL);// now
                coockiefile="WC"+WeConnect.WeConnectValues->User+"_coockie.txt";
 //               printf("Slow Poll: %i\n", i);
                WeConnect.Login();
                if(WeConnect.WeConnectValues->loginStatus) WeConnect.getPersonalData();
                if(WeConnect.WeConnectValues->loginStatus) WeConnect.get_identity_data();
                *request_in_progress=false;

            }

        }
        if ((long)lastPollMiddle+(long)WeConnectValues->PollIntervallMiddle<=time (NULL)&&!*request_in_progress)// only if the PollIntervall seconds have passed
        {
            while (*request_in_progress&&WeConnect.WeConnectValues->m_stopThread)
            {
  //              printf("Warte ..\n");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
            if (!*request_in_progress)
            {
                *request_in_progress=true;
                         lastPollMiddle=time (NULL);// now

                coockiefile="WC"+WeConnect.WeConnectValues->User+"_coockie.txt";
                //              printf("Midle Poll: %i\n", i);
   //             printf("Midle Poll %i:\nUser: %s, Passwort: %s, VIN: %s\n", i,WeConnect.WeConnectValues->User.c_str(),WeConnect.WeConnectValues->PassWd.c_str(),WeConnect.WeConnectValues->actualFin.c_str());
//               printf("Selected Fin: %s\n",WeConnectValues->actualFin.c_str());
                *request_in_progress=false;

            }
        }
        if ((long)lastPollFast+(long)WeConnectValues->PollIntervallFast<=time (NULL)&&!*request_in_progress)// only if the PollIntervall seconds have passed
        {
            while (*request_in_progress&&!WeConnect.WeConnectValues->m_stopThread)
            {
 //               printf("Warte ..\n");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
            if (!*request_in_progress)
            {
                *request_in_progress=true;
            lastPollFast=time (NULL);// now
               coockiefile="WC"+WeConnect.WeConnectValues->User+"_coockie.txt";
 //                printf("Fast Poll %i:\nUser: %s, Passwort: %s, VIN: %s ***********************\n", i,WeConnect.WeConnectValues->User.c_str(),WeConnect.WeConnectValues->PassWd.c_str(),WeConnect.WeConnectValues->actualFin.c_str());
                 WeConnect.Login();
                 if(WeConnect.WeConnectValues->loginStatus) WeConnect.getRealCarData(WeConnect.WeConnectValues->actualFin);
                if(WeConnect.WeConnectValues->loginStatus) WeConnect.getVehicleStatus(WeConnect.WeConnectValues->actualFin);
                if(WeConnect.WeConnectValues->loginStatus) WeConnect.get_climater(WeConnect.WeConnectValues->actualFin);
                if(WeConnect.WeConnectValues->loginStatus) WeConnect.get_position(WeConnect.WeConnectValues->actualFin);
                if(WeConnect.WeConnectValues->loginStatus) WeConnect.get_charger(WeConnect.WeConnectValues->actualFin);
  //              printf ("Poll %i Ende *****************************************************************************************\n",i);
                *request_in_progress=false;


            }
        }
        std::this_thread::sleep_for (std::chrono::seconds(1));
    }
    WeConnect.WeConnectValues->thread_runing=false;// indicate that the thread ist not running

}
int cWeConnect::get_identity_data()
{
 //   printf("get_identity_data\n");
    Login();
    sWEBresult r=standardWebabfrage("/identityData",profileURL+Identities.user_id,"","","");

    if (r.succsess>=0)
    {
        cJSON json;
//    printf("Header:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
//    else
  //      printf("get_identity_data = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;
}
int cWeConnect::get_users(std::string VIN)
{
    if(WeConnectValues->hasno_users>=0)
    {
 //   printf("get_users\n");
    Login();
    sWEBresult r=standardWebabfrage("/uic/v1/vin/"+VIN+"/users",USER_URL,"","","idP_IT="+identity_kit.id_token);

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        WeConnectValues->hasno_users=5;
        cJSON json;
//       printf("get_users :\nHeader:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
    else if (r.HTTP_Code==403) WeConnectValues->hasno_users--;
//    else
        //printf("get_users = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);
    return r.succsess;
    }
    return -1;

}
int cWeConnect::get_fences(std::string VIN)
{
    if(WeConnectValues->hasno_fences>=0)
 {
     //   printf("get_fences\n");
    Login();
    sWEBresult r=standardWebabfrage("/bs/geofencing/v1/VW/DE/vehicles/"+VIN+"/geofencingAlerts",BASE_URL,sc2_fal.access_token,__accept_mbb,"");

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        WeConnectValues->hasno_fences=5;
        cJSON json;
//        printf("get_fences :\nHeader:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
    else if (r.HTTP_Code==403)  WeConnectValues->hasno_fences--;
//    else
//        printf("get_fences = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;
 }
 return -1;
}
int cWeConnect::get_fences_configuration()
{
 //   printf("get_fences_configuration\n");
    if(WeConnectValues->hasno_fences_configuration>=0)
    {
        Login();
    sWEBresult r=standardWebabfrage("/bs/geofencing/v1/VW/DE/geofencingConfiguration/",BASE_URL,sc2_fal.access_token,__accept_mbb,"");

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        WeConnectValues->hasno_fences_configuration=5;
        cJSON json;
//        printf("get_fences_configuration :\nHeader:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
    else if (r.HTTP_Code==403) WeConnectValues->hasno_fences_configuration--;
//    else
//        printf("get_fences_configuration = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;
    }
    return -1;

}
int cWeConnect::get_trip_data(std::string VIN,std::string type)//type: 'longTerm', 'cyclic', 'shortTerm'
{
//    printf("get_trip_data\n");
    if(WeConnectValues->hasno_trip_data>=0)
    {
        Login();
    sWEBresult r=standardWebabfrage("/bs/tripstatistics/v1/VW/DE/vehicles/"+VIN+"/tripdata/"+type+"?type=list",BASE_URL,sc2_fal.access_token,__accept_mbb,"");

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        WeConnectValues->hasno_trip_data=5;
        cJSON json;
//       printf("get_trip_data :\nHeader:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
    else if (r.HTTP_Code==403) WeConnectValues->hasno_trip_data--;
//    else
//        printf("get_trip_data = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;
    }
    return -1;

}
int cWeConnect::get_departure_timer(std::string VIN)
{
//    printf("get_departure_timer\n");
    if(WeConnectValues->hasno_departure_timer>=0)
    {
        Login();
    sWEBresult r=standardWebabfrage("/bs/departuretimer/v1/VW/DE/vehicles/"+VIN+"/timer",BASE_URL,sc2_fal.access_token,__accept_mbb,"");

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        WeConnectValues->hasno_departure_timer=5;
        cJSON json;
//       printf("get_departure_timer :\nHeader:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
    else if (r.HTTP_Code==403) WeConnectValues->hasno_departure_timer--;
//    else
//        printf("get_departure_timer = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;
    }
    return -1;

}
int cWeConnect::get_speed_alerts(std::string VIN)
{
//    printf("get_speed_alerts\n");
    if (WeConnectValues->hasno_speed_alerts>=0)
    {
        Login();
    sWEBresult r=standardWebabfrage("/bs/speedalert/v1/VW/DE/vehicles/"+VIN+"/speedAlerts",BASE_URL,sc2_fal.access_token,__accept_mbb,"");

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        WeConnectValues->hasno_speed_alerts=5;
        cJSON json;
//       printf("get_speed_alerts :\nHeader:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
    else if (r.HTTP_Code==403) WeConnectValues->hasno_speed_alerts--;
//    else
//        printf("get_speed_alerts = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;
    }
    return -1;

}
int cWeConnect::get_speed_alerts_configuration()
{
//    printf("get_speed_alerts_configuration\n");
    if(WeConnectValues->hasno_speed_alerts_configuration>=0)
    {
        Login();
    sWEBresult r=standardWebabfrage("/bs/speedalert/v1/VW/DE/speedAlertConfiguration",BASE_URL,sc2_fal.access_token,__accept_mbb,"");

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        WeConnectValues->hasno_speed_alerts_configuration=5;
        cJSON json;
//        printf("get_speed_alerts_configuration :\nHeader:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
    else if (r.HTTP_Code==403) WeConnectValues->hasno_speed_alerts_configuration--;
//    else
//        printf("get_departure_timer = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;
    }
    return -1;

}
int cWeConnect::get_climater(std::string VIN)// Fertig
{
//    printf("get_climater\n");
    if(WeConnectValues->hasno_climater>=0)
    {
        if (VIN=="")
        if(WeConnectValues->actualFin!="")
            VIN=WeConnectValues->actualFin;// wenn keine VIN mitgegebenwurde mit dem ersten Fahrzeug im Account versuchen
        else
        {
            getRealCarData();// versuch eine VIN zu bekommen
            if(WeConnectValues->RealCarData[0].vehicleIdentificationNumber!="")
                return 0;// keine VIN im Account für das erste Fahrzeug, Abfrage nicht möglich
        }
    Login();
    sWEBresult r=standardWebabfrage("/bs/climatisation/v1/VW/DE/vehicles/"+VIN+"/climater",BASE_URL,sc2_fal.access_token,__accept_mbb,"");

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
//      printf("get_climater :\nHeader:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
        WeConnectValues->hasno_climater=5;
        cJSON json;
        std::string tempstring,sub1,sub2,theData,content;
        long long timestamptemp;
        tempstring=json.getElement(r.result,"climater");
        sub1=json.getElement(tempstring,"settings");
        theData=json.getElement(sub1,"targetTemperature");
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        WeConnectValues->CarInfo.targettemperature=Kd2float(json.getElement(theData,"content"));
//        printf("targettemperature :%s\n",timestamp2string(WeConnectValues->CarInfo.timestamp).c_str());

        theData=json.getElement(sub1,"climatisationWithoutHVpower");
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        WeConnectValues->CarInfo.climatisationWithoutHVPower=string2bool(json.getElement(theData,"content"));
//        printf("climatisationWithoutHVPower :%s\n",timestamp2string(WeConnectValues->CarInfo.timestamp).c_str());
        theData=json.getElement(sub1,"heaterSource");
        WeConnectValues->CarInfo.heaterSource=json.getElement(theData,"content");
        sub1=json.getElement(tempstring,"status");
        sub2=json.getElement(sub1,"climatisationStatusData");
        theData=json.getElement(sub2,"climatisationState");
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        WeConnectValues->CarInfo.climatisationState=json.getElement(theData,"content");
//        printf("climatisationState :%s\n",json.getElement(theData,"content").c_str());
        theData=json.getElement(sub2,"climatisationStateErrorCode");
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        WeConnectValues->CarInfo.climatisationStateErrorCode=json.getElement(theData,"content");
//        printf("climatisationStateErrorCode :%s\n",timestamp2string(WeConnectValues->CarInfo.timestamp).c_str());
        theData=json.getElement(sub2,"remainingClimatisationTime");
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
//        printf("remainingClimatisationTime :%s\n",json.getElement(theData,"content").c_str());
        if (json.getElement(theData,"content")!="")
            string2int(json.getElement(theData,"content"),&WeConnectValues->CarInfo.remainingClimatisationTime);
        else
            WeConnectValues->CarInfo.remainingClimatisationTime=-1;
//        printf("remainingClimatisationTime :%s\n",timestamp2string(WeConnectValues->CarInfo.timestamp).c_str());
        theData=json.getElement(sub2,"climatisationReason");
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        WeConnectValues->CarInfo.climatisationReason=json.getElement(theData,"content");
//        printf("climatisationReason :%s\n",timestamp2string(WeConnectValues->CarInfo.timestamp).c_str());
        sub2=json.getElement(sub1,"windowHeatingStatusData");
        theData=json.getElement(sub2,"windowHeatingStateFront");
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        WeConnectValues->CarInfo.windowHeatingStateFront=json.getElement(theData,"content");
        theData=json.getElement(sub2,"windowHeatingStateRear");
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        WeConnectValues->CarInfo.windowHeatingStateRear=json.getElement(theData,"content");
        theData=json.getElement(sub2,"windowHeatingErrorCode");
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        WeConnectValues->CarInfo.windowHeatingErrorCode=json.getElement(theData,"content");

    }
    else if (r.HTTP_Code==403) WeConnectValues->hasno_climater--;
//    else
//        printf("get_climater = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;
    }
    return -1;

}
int cWeConnect::get_position(std::string VIN)
{
//    printf("get_position\n");
    if(WeConnectValues->hasno_position>=0)
    {
        if (VIN=="")
        if(WeConnectValues->actualFin!="")
            VIN=WeConnectValues->actualFin;// wenn keine VIN mitgegebenwurde mit dem ersten Fahrzeug im Account versuchen
        else
        {
            getRealCarData();// versuch eine VIN zu bekommen
            if(WeConnectValues->RealCarData[0].vehicleIdentificationNumber!="")
                return 0;// keine VIN im Account für das erste Fahrzeug, Abfrage nicht möglich
        }
    Login();
    sWEBresult r=standardWebabfrage("/bs/cf/v1/VW/DE/vehicles/"+VIN+"/position",BASE_URL,sc2_fal.access_token,__accept_mbb,"");

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        WeConnectValues->hasno_position=5;
        cJSON json;
        std::string tempstring;
        tempstring=json.getElement(r.result,"storedPositionResponse");
        WeConnectValues->CarInfo.parkingtime=string2timestamp(json.getElement(tempstring,"parkingTimeUTC"));
        tempstring=json.getElement(tempstring,"position");
        string2int(json.getElement(json.getElement(tempstring,"heading"),"direction"),&WeConnectValues->CarInfo.direction);
        tempstring=json.getElement(tempstring,"carCoordinate");
        WeConnectValues->CarInfo.latitude=json.getElement(tempstring,"latitude");
        WeConnectValues->CarInfo.longitude=json.getElement(tempstring,"longitude");

//       printf("Position : Parkzeit: %s, Richtung: %s, latitude: %s, longitude: %s\n",timestamp2string(WeConnectValues->CarInfo.parkingtime).c_str(),std::to_string(WeConnectValues->CarInfo.direction).c_str(),WeConnectValues->CarInfo.latitude.c_str(),WeConnectValues->CarInfo.longitude.c_str());
    }
    else if (r.HTTP_Code==403) WeConnectValues->hasno_position--;
//    else
//        printf("get_position = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;
    }
    return -1;

}
int cWeConnect::get_destinations(std::string VIN)
{
//    printf("get_destinations\n");
    if(WeConnectValues->hasno_destinations>=0)
    {
        Login();
    sWEBresult r=standardWebabfrage("/destinationfeedservice/mydestinations/v1/VW/DE/vehicles/"+VIN+"/destinations",BASE_URL,sc2_fal.access_token,__accept_mbb,"");

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        WeConnectValues->hasno_destinations=5;
        cJSON json;
//        printf("get_destinations :\nHeader:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
    else if (r.HTTP_Code==403) WeConnectValues->hasno_destinations--;
//    else
//        printf("get_destinations = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;
    }
    return -1;

}
int cWeConnect::get_charger(std::string VIN)// Fertig
{
//    printf("get_charger\n");
    if(WeConnectValues->hasno_charger>=0)
    {
        if (VIN=="")
        if(WeConnectValues->actualFin!="")
            VIN=WeConnectValues->actualFin;// wenn keine VIN mitgegebenwurde mit dem ersten Fahrzeug im Account versuchen
        else
        {
            getRealCarData();// versuch eine VIN zu bekommen
            if(WeConnectValues->RealCarData[0].vehicleIdentificationNumber!="")
                return 0;// keine VIN im Account für das erste Fahrzeug, Abfrage nicht möglich
        }
    Login();
    sWEBresult r=standardWebabfrage("/bs/batterycharge/v1/VW/DE/vehicles/"+VIN+"/charger",BASE_URL,sc2_fal.access_token,__accept_mbb,"");

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        WeConnectValues->hasno_charger=5;
        cJSON json;
//        printf("get_charger :\nHeader:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
        std::string tempstring,sub1,sub2,theData,content;
        long long timestamptemp;
        tempstring=json.getElement(r.result,"charger");
        sub1=json.getElement(tempstring,"settings");
        theData=json.getElement(sub1,"maxChargeCurrent");
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        string2int(json.getElement(theData,"content"),&WeConnectValues->CarInfo.maxChargeCurrent);
        sub1=json.getElement(tempstring,"status");
        sub2=json.getElement(sub1,"chargingStatusData");
        theData=json.getElement(sub2,"chargingMode");
        WeConnectValues->CarInfo.timestamp=string2timestamp(json.getElement(theData,"timestamp"));
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        WeConnectValues->CarInfo.chargingMode=json.getElement(theData,"content");
        theData=json.getElement(sub2,"chargingStateErrorCode");
        WeConnectValues->CarInfo.timestamp=string2timestamp(json.getElement(theData,"timestamp"));
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        WeConnectValues->CarInfo.chargingStateErrorCode=json.getElement(theData,"content");
        theData=json.getElement(sub2,"chargingReason");
        WeConnectValues->CarInfo.timestamp=string2timestamp(json.getElement(theData,"timestamp"));
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        WeConnectValues->CarInfo.chargingReason=json.getElement(theData,"content");
        theData=json.getElement(sub2,"externalPowerSupplyState");
        WeConnectValues->CarInfo.timestamp=string2timestamp(json.getElement(theData,"timestamp"));
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        WeConnectValues->CarInfo.externalPowerSupplyState=json.getElement(theData,"content");
        theData=json.getElement(sub2,"energyFlow");
        WeConnectValues->CarInfo.timestamp=string2timestamp(json.getElement(theData,"timestamp"));
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        WeConnectValues->CarInfo.energyFlow=json.getElement(theData,"content");
        theData=json.getElement(sub2,"chargingState");
        WeConnectValues->CarInfo.timestamp=string2timestamp(json.getElement(theData,"timestamp"));
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        WeConnectValues->CarInfo.chargingState=json.getElement(theData,"content");
        theData=json.getElement(sub2,"chargingMode");
        WeConnectValues->CarInfo.timestamp=string2timestamp(json.getElement(theData,"timestamp"));
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        WeConnectValues->CarInfo.chargingMode=json.getElement(theData,"content");
        theData=json.getElement(sub2,"chargingMode");
        WeConnectValues->CarInfo.timestamp=string2timestamp(json.getElement(theData,"timestamp"));
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        WeConnectValues->CarInfo.chargingMode=json.getElement(theData,"content");
        sub2=json.getElement(sub1,"batteryStatusData");
        theData=json.getElement(sub2,"remainingChargingTime");
        WeConnectValues->CarInfo.timestamp=string2timestamp(json.getElement(theData,"timestamp"));
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        if (json.getElement(theData,"content")!="")
            string2int(json.getElement(theData,"content"),&WeConnectValues->CarInfo.remainingChargingTime);
        else
            WeConnectValues->CarInfo.remainingChargingTime=-1;
        theData=json.getElement(sub2,"stateOfCharge");
        WeConnectValues->CarInfo.timestamp=string2timestamp(json.getElement(theData,"timestamp"));
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        if (json.getElement(theData,"content")!="")
            string2int(json.getElement(theData,"content"),&WeConnectValues->CarInfo.SOC);
        else
            WeConnectValues->CarInfo.SOC=-1;
        sub2=json.getElement(sub1,"plugStatusData");
        theData=json.getElement(sub2,"plugState");
        WeConnectValues->CarInfo.timestamp=string2timestamp(json.getElement(theData,"timestamp"));
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        WeConnectValues->CarInfo.plugState=json.getElement(theData,"content");
        theData=json.getElement(sub2,"lockState");
        WeConnectValues->CarInfo.timestamp=string2timestamp(json.getElement(theData,"timestamp"));
        timestamptemp=string2timestamp(json.getElement(theData,"timestamp"));
        if(timestamptemp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamptemp;
        WeConnectValues->CarInfo.lockState=json.getElement(theData,"content");

    }
    else if (r.HTTP_Code==403) WeConnectValues->hasno_charger--;
//    else
//        printf("get_charger = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;
    }
    return -1;

}
int cWeConnect::get_heating_status(std::string VIN)
{
//    printf("get_heating_status\n");
    if(WeConnectValues->hasno_heating_status>=0)
    {
        Login();
    sWEBresult r=standardWebabfrage("/bs/rs/v1/VW/DE/vehicles/"+VIN+"/status",BASE_URL,sc2_fal.access_token,__accept_mbb,"");

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        WeConnectValues->hasno_heating_status=5;
        cJSON json;
//       printf("get_heating_status :\nHeader:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
    else WeConnectValues->hasno_heating_status=true;
//    else
//        printf("get_heating_status = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;
    }
    return -1;

}
int cWeConnect::get_history(std::string VIN)
{
//    printf("get_history\n");
    if(WeConnectValues->hasno_history>=0)
    {
        Login();
    sWEBresult r=standardWebabfrage("/bs/dwap/v1/VW/DE/vehicles/"+VIN+"/history",BASE_URL,sc2_fal.access_token,__accept_mbb,"");

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        WeConnectValues->hasno_history=5;
        cJSON json;
//       printf("get_history :\nHeader:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
    else if (r.HTTP_Code==403) WeConnectValues->hasno_history--;
//    else
//        printf("get_history = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;
    }
    return -1;


}
int cWeConnect::get_roles_rights(std::string VIN)
{
//    printf("get_roles_rights\n");
    if(WeConnectValues->hasno_roles_rights>=0)
       {
            Login();
    sWEBresult r=standardWebabfrage("/rolesrights/operationlist/v3/vehicles/"+VIN+"/users/"+WeConnectValues->UserData.businessIdentifierValue,MAL_URL,sc2_fal.access_token,__accept_mbb,"");

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        WeConnectValues->hasno_roles_rights=5;
        cJSON json;
//        printf("get_roles_rights :\nHeader:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
    else if (r.HTTP_Code==403) WeConnectValues->hasno_roles_rights--;
//    else
//        printf("get_roles_rights = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;
       }
       return -1;

}
int cWeConnect::get_fetched_role(std::string VIN)
{
//    printf("get_fetched_role\n");
    if(WeConnectValues->hasno_fetched_role>=0)
    {
        Login();
    sWEBresult r=standardWebabfrage("/rolesrights/permissions/v1/VW/DE/vehicles/"+VIN+"/fetched-role",BASE_URL,sc2_fal.access_token,__accept_mbb,"");

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        WeConnectValues->hasno_fetched_role=5;
        cJSON json;
//        printf("get_fetched_role :\nHeader:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
    else if (r.HTTP_Code==403) WeConnectValues->hasno_fetched_role--;
//    else
//        printf("get_fetched_role = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;
    }
    return -1;

}
int cWeConnect::request_status_update(std::string VIN)
{
//    printf("request_status_update\n");
    if(WeConnectValues->hasno_status_update>=0)
    {
        Login();
    sWEBresult r=standardWebabfrage("/bs/vsr/v1/VW/DE/vehicles/"+VIN+"/requests",BASE_URL,sc2_fal.access_token,__accept_mbb,"");

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        WeConnectValues->hasno_status_update=5;
        cJSON json;
//       printf("request_status_update :\nHeader:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
   else if (r.HTTP_Code==403) WeConnectValues->hasno_status_update--;
//    else
//        printf("request_status_update = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;
    }
    return -1;

}
int cWeConnect::__flash_and_honk(std::string VIN,std::string mode,std::string latitude,std::string longitude)// FLASH_ONLY oder HONK_AND_FLASH als mode
{
//    printf("__flash_and_honk\n");
    std::string data = "{\"honkAndFlashRequest\": {\"serviceOperationCode\": "+mode+",\"serviceDuration\": 15,\"userPosition\": {\"latitude\": "+latitude+",\"longitude\": "+longitude+"}}}";
    sWEBresult r=standardPost("/bs/rhf/v1/VW/DE/vehicles/"+VIN+"/honkAndFlash",BASE_URL,sc2_fal.access_token,__accept_mbb,data);

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        cJSON json;
//        printf("request_status_update :\nHeader:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
//    else
//        printf("request_status_update = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;

}
int cWeConnect::get_honk_and_flash_configuration()
{
//    printf("get_honk_and_flash_configuration\n");
    Login();
    sWEBresult r=standardWebabfrage("/bs/rhf/v1/VW/DE/configurations",BASE_URL,sc2_fal.access_token,__accept_mbb,"");

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        cJSON json;
//       printf("get_honk_and_flash_configuration :\nHeader:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
//    else
//        printf("get_honk_and_flash_configuration = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;

}
int cWeConnect::climatisation(std::string VIN,std::string ACTION)// startClimatisation oder stopClimatisation
{
//    printf("climatisation\n");
    std::string data = "{\"action\": {\"type\":\""+ACTION+"\"}}";
    //  std::string data = '{"action": {"type": "startClimatisation"}}';
    Login();
    sWEBresult r=standardPost("/bs/climatisation/v1/VW/DE/vehicles/"+VIN+"/climater/actions",BASE_URL,sc2_fal.access_token,__accept_mbb,data);

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        cJSON json;
//        printf("climatisation :\nHeader:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
//    else
//        printf("climatisation = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;

}
int cWeConnect::battery_charge(std::string VIN,std::string ACTION)// start oder stop
{
//    printf("battery_charge\n");
    std::string data = "{\"action\": {\"type\":\""+ACTION+"\"}}";
    Login();
    sWEBresult r=standardPost("/bs/batterycharge/v1/VW/DE/vehicles/"+VIN+"/charger/actions",BASE_URL,sc2_fal.access_token,__accept_mbb,data);

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        cJSON json;
        printf("battery_charge :\nHeader:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
//    else
//        printf("battery_charge = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;

}
int cWeConnect::climatisation_temperature(std::string VIN,float temperature)// start oder stop
{
//    printf("climatisation_temperature\n");
    int newtemperature=float2Kd(temperature);

    std::string data = "{\"action\": {\"type\":\"setSettings\", \"settings\": {\"targetTemperature\":"+std::to_string(newtemperature)+", \"climatisationWithoutHVpower\": True,\"heaterSource\": \"electric\",}}}";
    Login();
    sWEBresult r=standardPost("/bs/climatisation/v1/VW/DE/vehicles/"+VIN+"/climater/actions",BASE_URL,sc2_fal.access_token,__accept_mbb,data);

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        cJSON json;
    }
//    else
//        printf("climatisation_temperature = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;

}
int cWeConnect::window_melt(std::string VIN,std::string ACTION)// startWindowHeating oder stopWindowHeating
{
//    printf("window_melt\n");
    std::string data = "{\"action\": {\"type\":\""+ACTION+"\"}}";
    Login();
    sWEBresult r=standardPost("/bs/climatisation/v1/VW/DE/vehicles/"+VIN+"/climater/actions",BASE_URL,sc2_fal.access_token,__accept_mbb,data);

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        cJSON json;
        printf("window_melt :\nHeader:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
//    else
//        printf("window_melt = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;

}
int cWeConnect::getVehicleStatus(std::string VIN)
{
//    printf("getVehicleStatus\n");

    if(WeConnectValues->hasno_VehicleStatus>=0)
    {
        cJSON json;
    if (VIN=="")
        if(WeConnectValues->actualFin!="")
            VIN=WeConnectValues->actualFin;// wenn keine VIN mitgegebenwurde mit dem ersten Fahrzeug im Account versuchen
        else
        {
            getRealCarData();// versuch eine VIN zu bekommen
            if(WeConnectValues->RealCarData[0].vehicleIdentificationNumber!="")
                return 0;// keine VIN im Account für das erste Fahrzeug, Abfrage nicht möglich
        }
    Login();
    sWEBresult r=standardWebabfrage(("/bs/vsr/v1/VW/DE/vehicles/"+VIN+"/status"),BASE_URL,sc2_fal.access_token,__accept_mbb,"");
//    printf("Header:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    std::string tempstring,theData,sub1,sub2,sub3;
    long long timestamp;
    int tempint;
    if (r.succsess>0&&r.HTTP_Code==200)
    {
//        printf("Header:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
        WeConnectValues->hasno_VehicleStatus=5;
        theData=json.getElement(r.result,"StoredVehicleDataResponse");
//        printf("StoredVehicleDataResponse");
        theData=json.getElement(theData,"vehicleData");
        tempstring=findID(theData,"0x030102FFFF","data");
        tempstring=findID(tempstring,"0x0301020001","field");
        timestamp=string2timestamp(json.getElement(tempstring,"tsCarCaptured"));
        if (timestamp>WeConnectValues->CarInfo.timestamp)
            WeConnectValues->CarInfo.timestamp=timestamp;
        tempstring=json.getElement(tempstring,"value");
        if(tempstring!="")
        {
            string2float(tempstring,&WeConnectValues->CarInfo.aussentemperatur);
            WeConnectValues->CarInfo.aussentemperatur=Kd2float(WeConnectValues->CarInfo.aussentemperatur);
        }
        tempstring=findID(theData,"0x0203FFFFFF","data");
        tempstring=findID(tempstring,"0x0203010003","field");
        if (json.getElement(tempstring,"value")!="null"&&json.getElement(tempstring,"value")!="")
            string2int(json.getElement(tempstring,"value"),&WeConnectValues->CarInfo.InspektionDistance);
        else
            WeConnectValues->CarInfo.InspektionDistance=-1;
        tempstring=findID(theData,"0x030102FFFF","data");
        tempstring=findID(tempstring,"0x0203010004","field");
        if (json.getElement(tempstring,"value")!="null"&&json.getElement(tempstring,"value")!="")
            string2int(json.getElement(tempstring,"value"),&WeConnectValues->CarInfo.InspektionTime);
        else
            WeConnectValues->CarInfo.InspektionTime=-1;
        tempstring=findID(theData,"0x030103FFFF","data");
        tempstring=findID(tempstring,"0x0301030002","field");
        timestamp=string2timestamp(json.getElement(tempstring,"tsCarCaptured"));
        updateTimestamp(&WeConnectValues->CarInfo.timestamp,string2timestamp(json.getElement(tempstring,"tsCarCaptured")));
        if (json.getElement(tempstring,"value")!="null"&&json.getElement(tempstring,"value")!="")
            string2int(json.getElement(tempstring,"value"),&tempint);
        WeConnectValues->CarInfo.SOC=tempint;
        tempstring=findID(theData,"0x030103FFFF","data");
        sub1=findID(tempstring,"0x0301030004","field");
        timestamp=string2timestamp(json.getElement(sub1,"tsCarCaptured"));
        if (json.getElement(sub1,"value")!="null"&&json.getElement(sub1,"value")!="")
            //  string2int(json.getElement(sub1,"value"),&tempint);
            WeConnectValues->CarInfo.speed=json.getElement(sub1,"value");
        tempstring=findID(theData,"0x030103FFFF","data");
        tempstring=findID(tempstring,"0x0301030005","field");
        timestamp=string2timestamp(json.getElement(tempstring,"tsCarCaptured"));
        if (json.getElement(tempstring,"value")!="null"&&json.getElement(tempstring,"value")!="")
            string2int(json.getElement(tempstring,"value"),&WeConnectValues->CarInfo.Reichweite);
        tempstring=findID(theData,"0x030104FFFF","data");
        sub1=findID(tempstring,"0x0301040001","field");
        updateTimestamp(&WeConnectValues->CarInfo.timestamp,string2timestamp(json.getElement(sub1,"tsCarCaptured")));
        if (json.getElement(sub1,"value")!="null"&&json.getElement(sub1,"value")!="")
            string2int(json.getElement(sub1,"value"),&WeConnectValues->CarInfo.Door1.DoorLocked);// 2=locked
        sub1=findID(tempstring,"0x0301040002","field");
        updateTimestamp(&WeConnectValues->CarInfo.timestamp,string2timestamp(json.getElement(sub1,"tsCarCaptured")));
        if (json.getElement(sub1,"value")!="null"&&json.getElement(sub1,"value")!="")
            string2int(json.getElement(sub1,"value"),&WeConnectValues->CarInfo.Door1.DoorCloesed);//3=closed
        sub1=findID(tempstring,"0x0301040003","field");
        updateTimestamp(&WeConnectValues->CarInfo.timestamp,string2timestamp(json.getElement(sub1,"tsCarCaptured")));
        if (json.getElement(sub1,"value")!="null"&&json.getElement(sub1,"value")!="")
            string2int(json.getElement(sub1,"value"),&WeConnectValues->CarInfo.Door1.DoorSave);// 2=save

        sub1=findID(tempstring,"0x0301040004","field");
        updateTimestamp(&WeConnectValues->CarInfo.timestamp,string2timestamp(json.getElement(sub1,"tsCarCaptured")));
        if (json.getElement(sub1,"value")!="null"&&json.getElement(sub1,"value")!="")
            string2int(json.getElement(sub1,"value"),&WeConnectValues->CarInfo.Door2.DoorLocked);// 2=locked
        sub1=findID(tempstring,"0x0301040005","field");
        updateTimestamp(&WeConnectValues->CarInfo.timestamp,string2timestamp(json.getElement(sub1,"tsCarCaptured")));
        if (json.getElement(sub1,"value")!="null"&&json.getElement(sub1,"value")!="")
            string2int(json.getElement(sub1,"value"),&WeConnectValues->CarInfo.Door2.DoorCloesed);//3=closed
        sub1=findID(tempstring,"0x0301040006","field");
        updateTimestamp(&WeConnectValues->CarInfo.timestamp,string2timestamp(json.getElement(sub1,"tsCarCaptured")));
        if (json.getElement(sub1,"value")!="null"&&json.getElement(sub1,"value")!="")
            string2int(json.getElement(sub1,"value"),&WeConnectValues->CarInfo.Door2.DoorSave);// 2=save

        sub1=findID(tempstring,"0x0301040007","field");
        updateTimestamp(&WeConnectValues->CarInfo.timestamp,string2timestamp(json.getElement(sub1,"tsCarCaptured")));
        if (json.getElement(sub1,"value")!="null"&&json.getElement(sub1,"value")!="")
            string2int(json.getElement(sub1,"value"),&WeConnectValues->CarInfo.Door3.DoorLocked);// 2=locked
        sub1=findID(tempstring,"0x0301040008","field");
        updateTimestamp(&WeConnectValues->CarInfo.timestamp,string2timestamp(json.getElement(sub1,"tsCarCaptured")));
        if (json.getElement(sub1,"value")!="null"&&json.getElement(sub1,"value")!="")
            string2int(json.getElement(sub1,"value"),&WeConnectValues->CarInfo.Door3.DoorCloesed);//3=closed
        sub1=findID(tempstring,"0x0301040009","field");
        updateTimestamp(&WeConnectValues->CarInfo.timestamp,string2timestamp(json.getElement(sub1,"tsCarCaptured")));
        if (json.getElement(sub1,"value")!="null"&&json.getElement(sub1,"value")!="")
            string2int(json.getElement(sub1,"value"),&WeConnectValues->CarInfo.Door3.DoorSave);// 2=save

        sub1=findID(tempstring,"0x0301040010","field");
        updateTimestamp(&WeConnectValues->CarInfo.timestamp,string2timestamp(json.getElement(sub1,"tsCarCaptured")));
        if (json.getElement(sub1,"value")!="null"&&json.getElement(sub1,"value")!="")
            string2int(json.getElement(sub1,"value"),&WeConnectValues->CarInfo.Door4.DoorLocked);// 2=locked
        sub1=findID(tempstring,"0x0301040011","field");
        updateTimestamp(&WeConnectValues->CarInfo.timestamp,string2timestamp(json.getElement(sub1,"tsCarCaptured")));
        if (json.getElement(sub1,"value")!="null"&&json.getElement(sub1,"value")!="")
            string2int(json.getElement(sub1,"value"),&WeConnectValues->CarInfo.Door4.DoorCloesed);//3=closed
        sub1=findID(tempstring,"0x0301040012","field");
        updateTimestamp(&WeConnectValues->CarInfo.timestamp,string2timestamp(json.getElement(sub1,"tsCarCaptured")));
        if (json.getElement(sub1,"value")!="null"&&json.getElement(sub1,"value")!="")
            string2int(json.getElement(sub1,"value"),&WeConnectValues->CarInfo.Door4.DoorSave);// 2=save


    }
    else if (r.HTTP_Code==403)
    {
        WeConnectValues->hasno_VehicleStatus--;
        WeConnectValues->CarInfo.SOC=-1;
    }
//    else
//        printf("getVehicleData = Fehlernummer: %i, Onlinestatus: %i, FIN: %s\n",r.succsess,WeConnectValues->loginStatus,VIN.c_str());


    return r.succsess;
    }
    return -1;
}
int cWeConnect::getVehicleData(std::string VIN)
{
//    printf("getVehicleData\n");
    if (VIN=="")
        return 0;
    Login();
    sWEBresult r=standardWebabfrage(("/vehicleMgmt/vehicledata/v2/VW/DE/vehicles/"+VIN),BASE_URL,sc2_fal.access_token,"application/vnd.vwg.mbb.vehicleDataDetail_v2_1_0+json, application/vnd.vwg.mbb.genericError_v1_0_2+json","");
    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        cJSON json;
        std::string tempstring=json.getElement(r.result,"vehicleDataDetail");
//        printf("vehicleDataDetail:\n%s\n",tempstring.c_str());
        WeConnectValues->CarInfo.systemId=json.getElement(tempstring,"systemId");
        WeConnectValues->CarInfo.requestId=json.getElement(tempstring,"requestId");
        WeConnectValues->CarInfo.brand=json.getElement(tempstring,"brand");
        WeConnectValues->CarInfo.country=json.getElement(tempstring,"country");
        WeConnectValues->CarInfo.isConnect=json.getElement(tempstring,"isConnect");
        WeConnectValues->CarInfo.isConnectSorglosReady=json.getElement(tempstring,"isConnectSorglosReady");
        std::string vehicledevices=json.getElement(tempstring,"vehicleDevices");
//        printf("vehicledevices: %s\n",vehicledevices.c_str());
        int counter=json.getValueNumberOfFields(vehicledevices,"vehicleDevice");
        std::string vehicleDevice;
        std::string embeddedSim;
        for (int i=0; i<counter; i++)
        {
            vehicleDevice=json.getElement(vehicledevices,"vehicleDevice",i);
//            printf("vehicleDevice: %s\n",vehicleDevice.c_str());
            WeConnectValues->CarInfo.VehicleDevice[i].deviceType=json.getElement(vehicleDevice,"deviceType");
            WeConnectValues->CarInfo.VehicleDevice[i].ecuGeneration=json.getElement(vehicleDevice,"ecuGeneration");
            WeConnectValues->CarInfo.VehicleDevice[i].deviceId=json.getElement(vehicleDevice,"deviceId");
            embeddedSim=json.getElement(vehicleDevice,"embeddedSim");
            WeConnectValues->CarInfo.VehicleDevice[i].EmbeddedSim.Identification.type=json.getElement(json.getElement(embeddedSim,"identification"),"type");
            WeConnectValues->CarInfo.VehicleDevice[i].EmbeddedSim.Identification.content=json.getElement(json.getElement(embeddedSim,"identification"),"content");
            WeConnectValues->CarInfo.VehicleDevice[i].EmbeddedSim.imei=json.getElement(embeddedSim,"imei");
            WeConnectValues->CarInfo.VehicleDevice[i].EmbeddedSim.mno=json.getElement(embeddedSim,"mno");
        }
        std::string carportData=json.getElement(tempstring,"carportData");
//        printf("vehicleDataDetail:\n%s\n",tempstring.c_str());
        WeConnectValues->CarInfo.CarportData.modelCode=json.getElement(carportData,"modelCode");
        WeConnectValues->CarInfo.CarportData.modelName=json.getElement(carportData,"modelName");
        WeConnectValues->CarInfo.CarportData.modelYear=json.getElement(carportData,"modelYear");
        WeConnectValues->CarInfo.CarportData.color=json.getElement(carportData,"color");
        WeConnectValues->CarInfo.CarportData.countryCode=json.getElement(carportData,"countryCode");
        WeConnectValues->CarInfo.CarportData.engine=json.getElement(carportData,"engine");
        WeConnectValues->CarInfo.CarportData.mmi=json.getElement(carportData,"mmi");
        WeConnectValues->CarInfo.CarportData.transmission=json.getElement(carportData,"transmission");

//         WeConnectValues->CarInfo.isConnectSorglosReady=json.getElement(tempstring,"isConnectSorglosReady");
//         printf("Message:\n%s\nResult:\n%s\n\n\n",tempstring.c_str(),WeConnectValues->CarInfo.isConnectSorglosReady.c_str());

//        printf("Header:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
//    else
//        printf("getVehicleData = Fehlernummer: %i, Onlinestatus: %i, FIN: %s\n",r.succsess,WeConnectValues->loginStatus,VIN.c_str());

    return r.succsess;
}
int cWeConnect::getVehicles()
{
//    printf("getVehicles\n");
    Login();
    sWEBresult r=standardWebabfrage("/usermanagement/users/v1/VW/DE/vehicles",BASE_URL,sc2_fal.access_token,"","");

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
//        printf("Header:\n%s\nResult:\n%s\n",r.header.c_str(),r.result.c_str());
    }
//    else
//        printf("getVehicles = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;
}
int cWeConnect::get_mbbStatus()
{
//    __accept = 'application/vnd.vwg.mbb.vehicleDataDetail_v2_1_0+json, application/vnd.vwg.mbb.genericError_v1_0_2+json'
//       r = self.__command('/vehicleMgmt/vehicledata/v2/VW/DE/vehicles/'+vin, dashboard=self.BASE_URL, scope=self.__oauth['sc2:fal'], accept=__accept)
//    host="mbboauth-1d.prd.ece.vwg-connect.com";
//    printf("get_mbbStatus\n");
    Login();
    sWEBresult r=standardWebabfrage("/mbbStatusData",profileURL+Identities.user_id,"","","");
    //    host="customer-profile.apps.emea.vwapps.io";

    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        cJSON json;

        WeConnectValues->mbbStatus.profileCompleted=json.getElement(r.result,"profileCompleted");
        WeConnectValues->mbbStatus.spinDefined=json.getElement(r.result,"spinDefined");
        WeConnectValues->mbbStatus.carnetEnrollmentCountry=json.getElement(r.result,"carnetEnrollmentCountry");
        WeConnectValues->mbbStatus.etag=get_HeaderElement(r.header,"etag");

    }
//    else
//        printf("getmbbStatus = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;
}
int cWeConnect::getRealCarData()
{
    Login();
//    printf("getRealCarData \n");
//    printf("Identities.profile_url: %s\n",Identities.profile_url.c_str());
    sWEBresult r=standardWebabfrage("/realCarData",Identities.profile_url,"","","");
    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        std::string tempstring;
        cJSON json;
        WeConnectValues->AnzFahrzeuge=json.getValueNumberOfFields(r.result,"realCars");
//        printf ("AnzFahrzeuge %i:\n",WeConnectValues->AnzFahrzeuge);
        sRealCarData tempRealCarData;
        for (int i=0; i< WeConnectValues->AnzFahrzeuge; i++)
        {
            tempstring=json.getElement(r.result,"realCars",i);

            getRealCarDataFillCars(tempstring,&tempRealCarData);
            WeConnectValues->RealCarData[i]=tempRealCarData;
//            printf ("RealCarData Fahrzeug %i: %s, FIN: %s\n",i,WeConnectValues->RealCarData[i].nickname.c_str(),WeConnectValues->RealCarData[i].vehicleIdentificationNumber.c_str());
            if (i==maxCars)
                break;
        }
        if (WeConnectValues->AnzFahrzeuge==1)
            WeConnectValues->actualFin=WeConnectValues->RealCarData->vehicleIdentificationNumber;
        else
            WeConnectValues->actualFin="";
    }
//    else
//        printf("getRealCar = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;
}
int cWeConnect::getRealCarData(std::string FIN)
{
    Login();
//    printf("getRealCarData mit VIN \n");
//    printf("Identities.profile_url: %s\n",Identities.profile_url.c_str());
    sWEBresult r=standardWebabfrage("/realCarData",Identities.profile_url,"","","");
    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        std::string tempstring;
        cJSON json;
        int AnzFahrzeuge=json.getValueNumberOfFields(r.result,"realCars");
//        printf ("AnzFahrzeuge %i:\n",WeConnectValues->AnzFahrzeuge);
        sRealCarData tempRealCarData;
        for (int i=0; i< AnzFahrzeuge; i++)
        {
            tempstring=json.getElement(r.result,"realCars",i);

            getRealCarDataFillCars(tempstring,&tempRealCarData);
            if(tempRealCarData.vehicleIdentificationNumber==FIN)
            {
                WeConnectValues->nickname=tempRealCarData.nickname;
                WeConnectValues->licensePlateNumber=tempRealCarData.licensePlateNumber;
            }
            if (i==maxCars)
                break;
        }

    }
//    else
//        printf("getRealCar = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);

    return r.succsess;
}
sWEBresult cWeConnect::standardWebabfrage(std::string urlAnhang,std::string Dashboard,std::string token,std::string _accept,std::string _post)
{
#ifdef debug
    std::cout<<"Standard Webabfrage ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";

#endif // debug
//    printf("Dashboard: %s\n",Dashboard.c_str());
    Login();
    sWEBresult r;
    r.succsess=-1;// erst mal vom Versagen ausgehen
    if(WeConnectValues->loginStatus<0)
    {
        r.succsess=-1;
        return r; // wenn wir nicht eingeloggt sind macht das weitere keinen Sinn
    }
    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if(curl)
    {
        struct data config;
        config.trace_ascii = 1; /* enable ascii tracing */
        char buffer[CURL_ERROR_SIZE+1] = {};
        res=curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, buffer);
        struct curl_slist *chunk = NULL;
        chunk = curl_slist_append(chunk,"user-agent: python-requests/2.25.1");
        chunk = curl_slist_append(chunk,"Accept-Encoding: gzip, deflate");
        chunk = curl_slist_append(chunk,"Connection: eep-alive");
        chunk = curl_slist_append(chunk,"X-App-Name: We Connect");
        chunk = curl_slist_append(chunk,"Accept-Language: en-US");
        optionString="X-App-version:"+xappname;
        chunk = curl_slist_append(chunk,optionString.c_str());
        if(token=="")
            token=tokens.access_token;
//       printf("Accesstoken:\n%s\n",token.c_str());
        optionString="authorization: Bearer "+token;

        chunk = curl_slist_append(chunk,optionString.c_str());
        if(_accept=="")
            optionString = "accept: application/json";
        else
            optionString="accept: "+_accept;
        chunk = curl_slist_append(chunk,optionString.c_str());
        optionString="Host: "+host;
        chunk = curl_slist_append(chunk,optionString.c_str());
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
        curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, coockiefile.c_str());
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, coockiefile.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER,0);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
        sPostfields="";
        site=(Dashboard+urlAnhang);
        curl_easy_setopt(curl, CURLOPT_URL, site.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,sPostfields.size());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sPostfields.c_str());
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        response_header="";
        s="";
        response.res = curl_easy_perform(curl);
        curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &HTTPCode );
        r.HTTP_Code=HTTPCode;
        if(response.res != CURLE_OK||HTTPCode!=200)
        {
//            response.Message="Step getRealCars war nix\n";
            r.succsess=0;
//            printf("Fehler! HTTPCode: %i\n",(int)HTTPCode);
//            printf("Site:  %s\n",site.c_str());
            r.result=s;
            r.header=response_header;
//            printf("Site:\n%s\n",site.c_str());
//            if (response_header!="")
//                printf("ResponseHeader:\n%s\n",response_header.c_str());
//            if (s!="")
//                printf("Response:\n%s\n",s.c_str());


//////////////////////////// Aufräumen ///////////////////
            curl_easy_cleanup(curl);
            curl_slist_free_all(chunk);
            curl_global_cleanup();
///////////////////////
//            printf ("Webabfrage Ende. Result: %i\n",r.succsess);
            return r;
        }
        r.succsess=1;
        r.result=s;
        r.header=response_header;
//////////////////////////// Aufräumen ///////////////////
        curl_easy_cleanup(curl);
        curl_slist_free_all(chunk);
        curl_global_cleanup();
///////////////////////
//        printf ("Webabfrage Ende. Result: %i\n",r.succsess);
        return r;
    }// ende if (curl)
//   else printf("Curl nicht bereit\n");
    r.succsess=-99;// curl nicht bereit, hier sollten wir nicht hinkommen
//    printf ("Webabfrage Ende. Result: %i\n",r.succsess);
    return r;
    #ifdef debug
    printf ("standardWebabfrage cWeConnect Ende\n");
#endif // debug

}
sWEBresult cWeConnect::standardPost(std::string urlAnhang,std::string Dashboard,std::string token,std::string _accept,std::string _post)
{
#ifdef debug
    std::cout<<"request personal Data ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";

#endif // debug
    Login();

    sWEBresult r;
//    std::string optionString="X-App-version:"+xappname;
//     if(token=="")
//            optionString+="&authorization: Bearer "+tokens.access_token;
//        else
//        {
//            optionString+="&authorization: Bearer "+token;
//        }
//        if(_accept=="")
//            optionString += "&accept: application/json";
//        else
//            optionString+="&accept: "+_accept;
//        r.succsess=GET_POST_URL(Dashboard+urlAnhang,"Accept-Encoding: gzip, deflate&Connection: eep-alive&X-App-Name: We Connect&Accept-Language: en-US&"+optionString,"","",_post,1).success;
//        return r;
    r.succsess=-1;// erst mal vom Versagen ausgehen
    if(WeConnectValues->loginStatus<0)
    {
        r.succsess=-1;
        return r; // wenn wir nicht eingeloggt sind macht das weitere keinen Sinn
    }
    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    std::string ContentType="Content-Type: application/json";
    if(curl)
    {
        struct data config;
        config.trace_ascii = 1; /* enable ascii tracing */
        char buffer[CURL_ERROR_SIZE+1] = {};
        res=curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, buffer);
        struct curl_slist *chunk = NULL;
//       chunk = curl_slist_append(chunk,"user-agent: python-requests/2.25.1");
        chunk = curl_slist_append(chunk,"Accept-Encoding: gzip, deflate");
        chunk = curl_slist_append(chunk,"Connection: eep-alive");
        chunk = curl_slist_append(chunk,"X-App-Name: We Connect");
        chunk = curl_slist_append(chunk,"Accept-Language: en-US");
        chunk = curl_slist_append(chunk,"Content-Type: application/json");
//        curl_easy_setopt(curl, CURLOPT_HEADER, "Content-Type: application/json");

        chunk = curl_slist_append(chunk,optionString.c_str());
        if(token=="")
            optionString="authorization: Bearer "+tokens.access_token;
        else
        {
            optionString="authorization: Bearer "+token;
        }
        chunk = curl_slist_append(chunk,optionString.c_str());
        if(_accept=="")
            optionString = "accept: application/json";
        else
            optionString="accept: "+_accept;
        chunk = curl_slist_append(chunk,optionString.c_str());
        optionString="Host: "+host;
        chunk = curl_slist_append(chunk,optionString.c_str());
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
        curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "SolarCharge/vw-connect");
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, coockiefile.c_str());
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, coockiefile.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER,0);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
        sPostfields=_post;
        site=(Dashboard+urlAnhang);
        curl_easy_setopt(curl, CURLOPT_URL, site.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,sPostfields.length());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sPostfields.c_str());
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        response_header="";
        s="";
        response.res = curl_easy_perform(curl);
        curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &HTTPCode );
//        printf("Post gesendet :\n%s\n",sPostfields.c_str());

        if(response.res != CURLE_OK||HTTPCode!=200)
        {
//            response.Message="Step getRealCars war nix\n";
            r.succsess=0;
//            printf("Fehler! HTTPCode: %i\n",(int)HTTPCode);
//            printf("Site:  %s\n",site.c_str());
            r.result=s;
            r.header=response_header;
//            if (response_header!="")
//                printf("ResponseHeader:\n%s\n",response_header.c_str());
//            if (s!="")
//                printf("Response:\n%s\n",s.c_str());


//////////////////////////// Aufräumen ///////////////////
            curl_easy_cleanup(curl);
            curl_slist_free_all(chunk);
            curl_global_cleanup();
///////////////////////
            return r;
        }
        r.succsess=1;
        r.result=s;
        r.header=response_header;
//////////////////////////// Aufräumen ///////////////////
        curl_easy_cleanup(curl);
        curl_slist_free_all(chunk);
        curl_global_cleanup();
///////////////////////
        return r;
    }// ende if (curl)
//   else printf("Curl nicht bereit\n");
    r.succsess=-99;// curl nicht bereit, hier sollten wir nicht hinkommen
    return r;
}
CURL_Response cWeConnect::GET_POST_URL (std::string URL,std::string HEADER,std::string GET,std::string POST,std::string JSON,int FOLLOW)
{
    CURL_Response response;
    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    curl_easy_reset(curl);
    long HTTPCode;
    response.header="";
    response.result="";
    if(curl)
    {
        struct data config;
        config.trace_ascii = 1; /* enable ascii tracing */
        char buffer[CURL_ERROR_SIZE+1] = {};
        res=curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, buffer);
        struct curl_slist *chunk = NULL;
        chunk = curl_slist_append(chunk,HEADER.c_str());
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
        curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.result);
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, coockiefile.c_str());
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, coockiefile.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER,0);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response.header);
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
        chunk = curl_slist_append(chunk,HEADER.c_str());
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        if ((POST!=""||JSON!="")&&GET=="")
        {
            if(JSON!="")
            {
                chunk = curl_slist_append(chunk,"Content-Type: application/json");
                sPostfields=JSON;
            }
            else
                sPostfields=POST;
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
            curl_easy_setopt(curl, CURLOPT_POST, 1);
        }
        else
        {
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            sPostfields=GET;
        }
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

        curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,sPostfields.size());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sPostfields.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, FOLLOW);

        response.res = curl_easy_perform(curl);
        curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &response.HTTP_Code );

//////////////////////////// Aufräumen ///////////////////
        curl_easy_cleanup(curl);
        curl_slist_free_all(chunk);
        curl_global_cleanup();
///////////////////////
        return response;
    }// ende if (curl)
//   else printf("Curl nicht bereit\n");
    response.success =-1;
    return response;

}
int cWeConnect::getPersonalData()
{
#ifdef debug
    std::cout<<"request personal Data ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";

#endif // debug
//     r = self.__command('/personalData', dashboard=self.__identities['profile_url'])

//    printf("getPersonalData\n");
    Login();
    sWEBresult r=standardWebabfrage("/personalData",profileURL+Identities.user_id,"","","");
    if (r.succsess>=0&&r.HTTP_Code==200)
    {
        cJSON json;
        WeConnectValues->UserData.firstName=json.getElement(r.result,"firstName");
        WeConnectValues->UserData.lastName=json.getElement(r.result,"lastName");
        WeConnectValues->UserData.salutation=json.getElement(r.result,"salutation");
        WeConnectValues->UserData.dateOfBirth=json.getElement(r.result,"dateOfBirth");
        WeConnectValues->UserData.preferredContactChannel=json.getElement(r.result,"preferredContactChannel");
        WeConnectValues->UserData.nickname=json.getElement(r.result,"nickname");
        WeConnectValues->UserData.businessIdentifierType=json.getElement(r.result,"businessIdentifierType");
        WeConnectValues->UserData.businessIdentifierValue=json.getElement(r.result,"businessIdentifierValue");
        WeConnectValues->UserData.userIsPrivateIndicator=json.getElement(r.result,"userIsPrivateIndicator");
        WeConnectValues->UserData.preferredLanguage=json.getElement(r.result,"preferredLanguage");
        sADRESS Tempadress;
        getPersonalDataFillAdress(json.getElement(r.result,"addresses",0),&Tempadress);
        WeConnectValues->UserData.Homeadress=Tempadress;
        getPersonalDataFillAdress(json.getElement(r.result,"addresses",1),&Tempadress);
        WeConnectValues->UserData.Billingadress=Tempadress;
//        printf("Name Hauptadresse:\n%s\n",WeConnectValues->UserData.Homeadress.name.c_str());
    }
//    else
//        printf("getPersonalData = Fehlernummer: %i, Onlinestatus: %i\n",r.succsess,WeConnectValues->loginStatus);
    r.succsess=1;
    return r.succsess;
}
void cWeConnect::getPersonalDataFillAdress(std::string input, sADRESS *Adress)
{
    cJSON json;
    Adress->country=json.getElement(input,"country");
    Adress->usageType=json.getElement(input,"usageType");
    Adress->uuid=json.getElement(input,"uuid");
    Adress->primary=json.getElement(input,"primary");
    Adress->name=json.getElement(input,"name");
    Adress->street=json.getElement(input,"street");
    Adress->housenumber=json.getElement(input,"housenumber");
    Adress->zipCode=json.getElement(input,"zipCode");
    Adress->city=json.getElement(input,"city");
    Adress->_updated=json.getElement(input,"_updated");
    Adress->type=json.getElement(input,"type");

}
void cWeConnect::getRealCarDataFillCars(std::string input,sRealCarData *tRealCarData)
{
    int i=1;
    cJSON json;
    tRealCarData->nickname=json.getElement(input,"nickname");
    tRealCarData->vehicleIdentificationNumber=json.getElement(input,"vehicleIdentificationNumber");
    tRealCarData->licensePlateNumber=json.getElement(input,"licensePlateNumber");
    tRealCarData->allocatedDealerCountry=json.getElement(input,"allocatedDealerCountry");
    tRealCarData->allocatedDealerId=json.getElement(input,"allocatedDealerId");
    tRealCarData->allocatedDealerBrandCode=json.getElement(input,"allocatedDealerBrandCode");
    tRealCarData->carnetAllocationTimestamp=json.getElement(input,"carnetAllocationTimestamp");
    tRealCarData->carnetAllocationType=json.getElement(input,"carnetAllocationType");
    tRealCarData->carNetIndicator=json.getElement(input,"carNetIndicator");
    tRealCarData->commissionNumber=json.getElement(input,"commissionNumber");
    tRealCarData->deactivated=json.getElement(input,"deactivated");
    tRealCarData->deactivationReason=json.getElement(input,"deactivationReason");
    tRealCarData->modelCode=json.getElement(input,"modelCode");
    tRealCarData->commissionNumberYear=json.getElement(input,"commissionNumberYear");

}
int cWeConnect::Login()
{
 //   printf("Login %i User: %s\n",std::this_thread::get_id(),WeConnectValues->User.c_str());
 #ifdef debug
    printf ("Login cWeConnect\n");
#endif // debug
   WeConnectValues->loginStatus=-1;
    readAccess();
    if (checkTokens()!=-1)
    {
        WeConnectValues->loginStatus=1;
        WeConnectValues->LoginLevel=10;
        WeConnectValues->LoginMessage="Online";
//        printf("Online *****************************************************************\n");
#ifdef debug
    printf ("Login cWeConnect Ende\n");
#endif // debug
        return 1;
    }
    deleteCookies();

    WeConnectValues->loginStatus=forcedLogin();
#ifdef debug
    printf ("Login cWeConnect Ende\n");
#endif // debug
    return WeConnectValues->loginStatus;

}
int cWeConnect::checkTokens()
{
//    printf("Check Tokens\n");
    if (checkKitTokens()==-1||checkOuthTokens()==-1)
        return -1;
    return 1;
}
int cWeConnect::checkKitTokens()
{
//    printf("checkKitTokens \n");
#ifdef debug
    printf ("checkKitTokens cWeConnect\n");
#endif // debug
    if(tokens.timestamp==""||tokens.expires_in=="")
    {
//        printf("KitToken kein Timestamp\n");
        return -1;
    }
//    printf("Strings: Timestamp: %s,Expiresin:%s\n",tokens.timestamp.c_str(),tokens.expires_in.c_str());
    long long int longTimestamp=0;
    long long int longexpire=0;
    long long int longnow;
    if(tokens.timestamp!="")
        longTimestamp =std::stoll(tokens.timestamp,0,0);
    if(tokens.expires_in!="")
        longexpire =std::stoll(tokens.expires_in,0,0)*1000-60000;
    longnow = std::stoll(makeTimestampNow(),0,0);
//    printf("long long: longTimestamp:\n%lld\n%lld\nlongexpire\n",longTimestamp,longexpire);
//    printf("long long: Now:\n%lld %s\n%lld\nAbgelaufen um %s\n",longnow,timestamp2string(longnow).c_str(),longTimestamp+longexpire,timestamp2string(longTimestamp+longexpire).c_str());

    if(longTimestamp+longexpire>longnow)
    {
//        printf("KitToken valid\n");
#ifdef debug
    printf ("checkKitTokens cWeConnect Ende\n");
#endif // debug
        return 1;
    }
//    printf("Kittoken not valid\n");
//    printf("RefreshKitTokens\n");
    CURL_Response response=GET_POST_URL(TOKEN_URL+"/refreshTokens","","refresh_token="+tokens.refresh_token,"","",1);
//    printf("HTTP Code: %ld\n",response.HTTP_Code);
//    if (response.header!="")
//        printf("ResponseHeader:\n%s\n",response.header.c_str());
//    if (response.result!="")
//        printf("Response:\n%s\n",response.result.c_str());
    if(response.HTTP_Code==200)
    {
        cJSON json;
        tokens.access_token=json.getElement(response.result,"access_token");
        tokens.expires_in=json.getElement(response.result,"expires_in");
        tokens.refresh_token=json.getElement(response.result,"refresh_token");
        tokens.token_type=json.getElement(response.result,"token_type");
        tokens.timestamp=makeTimestampNow();
        writeAccess();
//        printf("KitToken Refresh erfolgreich\n");
        if(tokens.access_token!="")
            {
#ifdef debug
    printf ("checkKitTokens cWeConnect Ende\n");
#endif // debug
                return 1;
            }
    }
//    printf("KitToken Refresh fehlgeschlagen\n");
#ifdef debug
    printf ("checkKitTokens cWeConnect Ende\n");
#endif // debug
    return -1;
}
int cWeConnect::checkOuthTokens()
{
//    printf("checkOuthTokens \n");
#ifdef debug
    printf ("checkOuthTokens cWeConnect\n");
#endif // debug
    sc2_fal.tokenForRefresh=sc2_fal.refresh_token;
#ifdef debug
    printf ("checkOuthTokens cWeConnect Ende\n");
#endif // debug
    if (checkOuthScope(&sc2_fal)==-1)
        return -1;
    t2_v_cubic.tokenForRefresh=sc2_fal.refresh_token;
    if(checkOuthScope(&t2_v_cubic)==-1)
        return -1;
    else
        return 1;
}

int cWeConnect::checkOuthScope(s__oauth *Scope)
{
//    printf("checkOuthScope \n");
#ifdef debug
    printf ("checkOuthScope cWeConnect\n");
#endif // debug
    if(Scope->timestamp=="")
    {
//        printf("%s kein Timestamp\n",Scope->scope.c_str());
#ifdef debug
    printf ("checkOuthScope cWeConnect Ende\n");
#endif // debug
        return -1;
    }
//    printf("Strings: Timestamp: %s,Expiresin:%s\n",Scope->timestamp.c_str(),Scope->expires_in.c_str());
    long long int longTimestamp=0;
    long long int longexpire=0;
    long long int longnow;
    if(Scope->timestamp!="")
        longTimestamp =std::stoll(Scope->timestamp,0,0);
    if(Scope->expires_in!="")
        longexpire =std::stoll(Scope->expires_in,0,0)*1000-60000;
    longnow = std::stoll(makeTimestampNow(),0,0);
//    printf("long long: longTimestamp:\n%lld\n%lld\nlongexpire\n",longTimestamp,longexpire);
//    printf("long long: Now:\n%lld %s\n%lld\nAbgelaufen um %s\n",longnow,timestamp2string(longnow).c_str(),longTimestamp+longexpire,timestamp2string(longTimestamp+longexpire).c_str());


    if(longTimestamp+longexpire>longnow)
    {
//        printf("%s Valied\n",Scope->scope.c_str());
#ifdef debug
    printf ("checkOuthScope cWeConnect Ende\n");
#endif // debug
        return 1;
    }

//    printf("%s abgelaufen\n",Scope->scope.c_str());
    int ergebniss= refreshToken(Scope,OAUTH_URL,"refresh_token","");
    if (ergebniss==1)
        writeAccess();
#ifdef debug
    printf ("checkOuthScope cWeConnect Ende\n");
#endif // debug
    return ergebniss;

}

int cWeConnect::forcedLogin()
{
#ifdef debug
    printf ("forcedLogin cWeConnect\n");
#endif // debug

    if(WeConnectValues->User==""||WeConnectValues->PassWd=="")
    {
      printf("Anmeldedaten unvollständig!\n");
      return -1;
    }
    printf("forcedLogin\n");
    deleteCookies();
    createCodeChallenge();
    WeConnectValues->loginStatus=-1;
#ifdef debug
    printf("User :%s, Passwort: %s, SPin: %s, FIN: %s, CarBrand: %s\n",WeConnectValues->User.c_str(),WeConnectValues->PassWd.c_str(),SPin.c_str(),Fin.c_str(),CarBrand.c_str());
#endif // debug

    setValuesFromBrand();
    if (!checkSPin(SPin))
    {
#ifdef debug
        printf("SPin ungültig. Abbruch.\n");
#endif // debug

        return -2;
    }
    Identities.client_id=clientId;
    CURL *curl;
    CURLcode res;
    url="https://identity.vwgroup.io/oidc/v1/authorize?";;
    url += "client_id=9496332b-ea03-4091-a224-8c746b885068@apps_vw-dilab_com";//+clientId;
    url += "&prompt=login";
    url += "&state="+createRandomString(43);
    url +="&response_type=code%20id_token%20token";//+responseType;
//    if (type == "vw" || type == "vwv2" || type == "go")
    {
        url += "&code_challenge=" + code_challenge + "&code_challenge_method=S256";
    }

    if (type == "audi")
    {
        url += "&ui_locales=de-DE%20de";
    }
    if (type == "id" && type != "Wc")
    {
        //     url = await this.receiveLoginUrl().catch(() =>
        {
//           this.log.warn("Failed to get login url");
        }
//        if (!url)
        {
            //          url = "https://login.apps.emea.vwapps.io/authorize?nonce=" + this.randomString(16) + "&redirect_uri=weconnect://authenticated";
        }
    }

    url += "&scope="+scope;
    url +="&redirect_uri=carnet://identity-kit/login";
//    url += redirect;
    url += "&nonce="+createRandomString(43);

#ifdef debug
    printf ("Erste URL: %s\n",url.c_str());
#endif // debug
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    WeConnectValues->LoginLevel=0;
    WeConnectValues->LoginMessage="Starte Login";
    if(curl)
    {
        struct data config;
        config.trace_ascii = 1; /* enable ascii tracing */
        char buffer[CURL_ERROR_SIZE+1] = {};
        res=curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, buffer);
        struct curl_slist *chunk = NULL;
        chunk = curl_slist_append(chunk, "python-requests/2.25.1");
        chunk = curl_slist_append(chunk,  "Accept:*/*");
        chunk = curl_slist_append(chunk,  "Connection:keep-alive");
        // chunk = curl_slist_append(chunk, "Accept-Language: de-DE,de;q=0.9");
        chunk = curl_slist_append(chunk, "Accept-Encoding: gzip, deflate");
        optionString="x-requested-with: ";
        optionString+=xrequest;
        chunk = curl_slist_append(chunk, optionString.c_str());
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
        curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, coockiefile.c_str());
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, coockiefile.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER,0);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        site = url;
        std::cout<<"Step 1++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
        curl_easy_setopt(curl, CURLOPT_URL, site.c_str());
        response_header="";
        response.res = curl_easy_perform(curl);
#ifdef debug
        printf("site:\n%s\n",site.c_str());
        printf("sPostfields:\n%s\n",sPostfields.c_str());

        if (response_header!="")
            printf("ResponseHeader:\n%s\n",response_header.c_str());
        if (s!="")
            printf("Response:\n%s\n",s.c_str());
#endif // debug
        if(response.res != CURLE_OK)
        {
            std::cout<<"Request war nix\n";
            curl_easy_cleanup(curl);// ein anderer Fehler
            curl_slist_free_all(chunk);
            WeConnectValues->LoginLevel=0;
            WeConnectValues->LoginMessage="Anfrage1 Fehlerhaft!";
#ifdef debug
    printf ("forcedLogin cWeConnect Ende\n");
#endif // debug
            return -3;
        }
        //    return -1;
        if (s.length()<2)
        {
            curl_easy_cleanup(curl);
            curl_slist_free_all(chunk);
            WeConnectValues->LoginLevel=0;
            WeConnectValues->LoginMessage="Anfrage1 Antwort zu kurz!";
#ifdef debug
    printf ("forcedLogin cWeConnect Ende\n");
#endif // debug
            return -3;
        }
        Form=parse(s,"form","id","emailPasswordForm");
        if (Form=="")
        {
            WeConnectValues->LoginLevel=0;
            WeConnectValues->LoginMessage="Antwort1 falscher Inhalt!";
#ifdef debug
    printf ("forcedLogin cWeConnect Ende\n");
#endif // debug
            return -4; // Form nicht gefunden
        }
#ifdef debug
        printf ("Form: \n%s\n",Form.c_str());
        printf ("Pase action:\n");
#endif // debug
        action=parse(Form,"action");
        if (action=="")
        {
            curl_easy_cleanup(curl);
            curl_slist_free_all(chunk);
            WeConnectValues->LoginLevel=0;
            WeConnectValues->LoginMessage="Antwort1 Folgeseite nicht gefunden!";
#ifdef debug
    printf ("forcedLogin cWeConnect Ende\n");
#endif // debug
            return -5;
        }
        std::string upr=get_HeaderElement(response_header,"location");
        upr=parseUPR(upr);
#ifdef debug
        printf ("action: \n%s\n",action.c_str());
        printf("upr: %s\n",upr.c_str());
#endif // debug
        if((CSRF=parse(Form,"input","name","_csrf"))=="")
            return -98;
        if((CSRF=parse(CSRF,"value"))=="")
            return -99;

        if((Identities.relayState=parse(Form,"input","name","relayState"))=="")
            return -100;
        if((Identities.relayState=parse(Identities.relayState,"value"))=="")
            return -101;
        if((Identities.hmac=parse(Form,"input","name","hmac"))=="")
            return -102;
        if((Identities.hmac=parse(Identities.hmac,"value"))=="")
            return -103;
        WeConnectValues->LoginLevel=1;
        WeConnectValues->LoginMessage="Starte Schritt 2!";
        std::cout<<"Step 2++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
        curl_easy_reset(curl);
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
        curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, coockiefile.c_str());
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, coockiefile.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER,0);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        s="";
        sPostfields = ("email=" + WeConnectValues->User +"&relayState="+Identities.relayState+ "&hmac="+Identities.hmac+"&_csrf="+CSRF);
        site=upr+action;
        curl_easy_setopt(curl, CURLOPT_URL, site.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,sPostfields.size());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sPostfields.c_str());
        response.res = curl_easy_perform(curl);
#ifdef debug
        printf("site:\n%s\n",site.c_str());
        printf("sPostfields:\n%s\n",sPostfields.c_str());
        if (response_header!="")
            printf("ResponseHeader:\n%s\n",response_header.c_str());
        if (s!="")
            printf("Response:\n%s\n",s.c_str());
#endif // debug
        if(response.res != CURLE_OK)
        {
            printf("Step2 war nix\n");
            curl_easy_cleanup(curl);
            curl_slist_free_all(chunk);
            WeConnectValues->LoginLevel=0;
            WeConnectValues->LoginMessage="Anfrage2 fehlerhaft!";
#ifdef debug
    printf ("forcedLogin cWeConnect Ende\n");
#endif // debug
            return -6;
        }
        if (s=="")
        {
            curl_easy_cleanup(curl);
            curl_slist_free_all(chunk);
            WeConnectValues->LoginLevel=0;
            WeConnectValues->LoginMessage="Antwort2 kein Inhalt!";
#ifdef debug
    printf ("forcedLogin cWeConnect Ende\n");
#endif // debug
            return -6;
        }
#ifdef debug
        printf ("Pase Form:\n");
#endif // debug
        Form=parse(s,"form","id","credentialsForm");
#ifdef debug
        printf ("Form: \n%s\n",Form.c_str());
#endif // debug
        if (Form=="")
        {
            curl_easy_cleanup(curl);
            curl_slist_free_all(chunk);
            WeConnectValues->LoginLevel=0;
            WeConnectValues->LoginMessage="Antwort2 Loginfeld nicht gefunden!";
            return -7; // Form nicht gefunden
        }
#ifdef debug
        printf ("Parse action:\n");
#endif // debug
        action=parse(Form,"action");
#ifdef debug
        printf ("action: \n%s\n",action.c_str());
#endif // debug
        if (action=="")
        {
            curl_easy_cleanup(curl);
            curl_slist_free_all(chunk);
            WeConnectValues->LoginLevel=0;
            WeConnectValues->LoginMessage="Antwort2 Folgeseite nicht gefunden!";
#ifdef debug
    printf ("forcedLogin cWeConnect Ende\n");
#endif // debug
            return -8;
        }
        upr=get_HeaderElement(response_header,"location");
        upr=parseUPR(upr);
        if((Identities.relayState=parse(Form,"input","name","relayState"))=="")
            return -104;
        if((Identities.relayState=parse(Identities.relayState,"value"))=="")
            return -105;
        if((CSRF=parse(Form,"input","name","_csrf"))=="")
            return -106;
        if((CSRF=parse(CSRF,"value"))=="")
            return -107;
        if((Identities.hmac=parse(Form,"input","name","hmac"))=="")
            return -108;
        if((Identities.hmac=parse(Identities.hmac,"value"))=="")
            return -109;
        WeConnectValues->LoginLevel=2;
        WeConnectValues->LoginMessage="Starte Schritt 3";
        std::cout<<"Step 3++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
        curl_easy_reset(curl);
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
        curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, coockiefile.c_str());
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, coockiefile.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER,0);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
//       curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        s="";
        sPostfields = ("email=" + WeConnectValues->User+"&password="+WeConnectValues->PassWd +"&relayState="+Identities.relayState+ "&hmac="+Identities.hmac+"&_csrf="+CSRF);
        site=upr+action;
        curl_easy_setopt(curl, CURLOPT_URL, site.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,sPostfields.size());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sPostfields.c_str());
        response_header="";
        response.res = curl_easy_perform(curl);
//        printf("Site::\n%s\n",site.c_str());
//        printf("sPostfields:\n%s\n",sPostfields.c_str());

        curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &HTTPCode );
//        printf("Responsecode: %ld\n",HTTPCode);
//        if (response_header!="")
//            printf("ResponseHeader:\n%s\n",response_header.c_str());
//        if (s!="")
//            printf("Response:\n%s\n",s.c_str());
        if(response.res == CURLE_OK || HTTPCode != (long) 302)////////// Achtung! dismal wollen wir dass ein Fehler auftritt!!!!!
        {
            if ( HTTPCode == (long) 200&&s!="")
            {
                WeConnectValues->LoginLevel=2;
                WeConnectValues->LoginMessage="Anfrage3 umgeleitet. Persönliches Login erforderlicht?";
                char *Redir_url = NULL;
                curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &Redir_url);
                //            if(Redir_url)
                //                printf("Redirect to: %s\n", Redir_url);
                site=Redir_url;
                Form=parse(s,"form","method","post");
                //             printf("Form:\n%s\n",Form.c_str());
                std::string openid=parse(Form,"input","value","openid");
                openid=parse(openid,"value");
                std::string profile=parse(Form,"input","value","profile");
                profile=parse(profile,"value");
                std::string mbb=parse(Form,"input","value","mbb");
                mbb=parse(mbb,"value");
                std::string email=parse(Form,"input","value","email");
                email=parse(email,"value");
                std::string cars=parse(Form,"input","value","cars");
                cars=parse(cars,"value");
                std::string birthdate=parse(Form,"input","value","birthdate");
                birthdate=parse(birthdate,"value");
                std::string address=parse(Form,"input","value","address");
                address=parse(address,"value");
                std::string vin=parse(Form,"input","value","vin");
                vin=parse(vin,"value");
                std::string internalAndAlreadyConsentedScopesHmac=parse(Form,"input","name","internalAndAlreadyConsentedScopesHmac");
                internalAndAlreadyConsentedScopesHmac=parse(internalAndAlreadyConsentedScopesHmac,"value");
                curl_easy_reset(curl);
                res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
                curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
                curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);
                curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
                curl_easy_setopt(curl, CURLOPT_COOKIEFILE, coockiefile.c_str());
                curl_easy_setopt(curl, CURLOPT_COOKIEJAR, coockiefile.c_str());
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER,0);
                curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
                curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
//       curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
                s="";
                sPostfields = ("internalAndAlreadyConsentedScopes=" + openid+"&internalAndAlreadyConsentedScopes="+profile +"&internalAndAlreadyConsentedScopes="+mbb+ "&internalAndAlreadyConsentedScopes="+email+"&internalAndAlreadyConsentedScopes="+cars+"&internalAndAlreadyConsentedScopes="+birthdate+"&internalAndAlreadyConsentedScopes="+address+"&internalAndAlreadyConsentedScopes="+vin+"&internalAndAlreadyConsentedScopesHmac="+internalAndAlreadyConsentedScopesHmac+"&_csrf="+CSRF);
                curl_easy_setopt(curl, CURLOPT_URL, site.c_str());
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,sPostfields.size());
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sPostfields.c_str());
                response_header="";
                response.res = curl_easy_perform(curl);
//            printf("Site::\n%s\n",site.c_str());
//            printf("sPostfields:\n%s\n",sPostfields.c_str());

                curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &HTTPCode );
//           printf("Responsecode: %ld\n",HTTPCode);
//           if (response_header!="")
//               printf("ResponseHeader:\n%s\n",response_header.c_str());
//            if (s!="")
//               printf("Response:\n%s\n",s.c_str());
            }
            if(response.res == CURLE_OK || HTTPCode != (long) 302)////////// Achtung! dismal wollen wir dass ein Fehler auftritt!!!!!
            {
                printf("Step3 war nix\n");
                curl_easy_cleanup(curl);
                curl_slist_free_all(chunk);
                WeConnectValues->LoginLevel=0;
                WeConnectValues->LoginMessage="Evtl. einloggen und Nutzungbedingunge bestätigen!";
#ifdef debug
    printf ("forcedLogin cWeConnect Ende\n");
#endif // debug
                return -9;
            }
        }
        curl_slist_free_all(chunk);


        std::string location=get_HeaderElement(response_header,"location");
        if ((Identities.user_id=get_HeaderElement(location,"user_id"))=="")
            Identities.user_id=get_HeaderElement(location,"userId") ;
        if ((Identities.client_id=get_HeaderElement(location,"client_id"))=="")
            Identities.client_id=get_HeaderElement(location,"clientId");
        Identities.scopes=scope;
        Identities.profile_url=profileURL+Identities.user_id;
        location=get_HeaderElement(response_header,"carnet");

        identity_kit.code=get_HeaderElement(location,"code");
        identity_kit.id_token=get_HeaderElement(location,"id_token");
        identity_kit.access_token=get_HeaderElement(location,"access_token");
        identity_kit.expires_in=get_HeaderElement(location,"expires_in");
        identity_kit.state=get_HeaderElement(location,"state");
        identity_kit.token_type=get_HeaderElement(location,"token_type");
        identity_kit.timestamp=makeTimestampNow();
//       return -1;
        WeConnectValues->LoginLevel=3;
        WeConnectValues->LoginMessage="Starte Schritt 4!";
        std::cout<<"Step 4++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
        curl_easy_reset(curl);
        curl_slist *chunk2 = NULL;
        optionString="X-App-version:";
        optionString+=xappversion;
        chunk2 = curl_slist_append(chunk2,optionString.c_str());
        chunk2 = curl_slist_append(chunk2,  "content-type: application/x-www-form-urlencoded");
        chunk2 = curl_slist_append(chunk2,  "user-agent: Car-Net/60 CFNetwork/1121.2.2 Darwin/19.3.0");
        chunk2 = curl_slist_append(chunk2,  "accept-encoding: gzip, deflate, br");
        optionString="x-app-name";
        optionString+=xappname;
        chunk2 = curl_slist_append(chunk2,optionString.c_str());
        chunk2 = curl_slist_append(chunk2,"accept: application/json");
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk2);
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
        curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, coockiefile.c_str());
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, coockiefile.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER,0);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        sPostfields = ("auth_code=" + identity_kit.code+"&code_verifier="+code_verifier  +"&id_token="+identity_kit.id_token);
        site=TOKEN_URL+ "/exchangeAuthCode";
        curl_easy_setopt(curl, CURLOPT_URL, site.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,sPostfields.size());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sPostfields.c_str());
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);
        s="";
        response_header="";
        response.res = curl_easy_perform(curl);
//        printf("site:\n%s\n",site.c_str());
//        printf("sPostfields:\n%s\n",sPostfields.c_str());

//        if (response_header!="")
//            printf("ResponseHeader:\n%s\n",response_header.c_str());
//        if (s!="")
//            printf("Response:\n%s\n",s.c_str());
        curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &HTTPCode );
#ifdef debug
        printf("site:\n%s\n",site.c_str());
        printf("sPostfields:\n%s\n",sPostfields.c_str());
        if (response_header!="")
            printf("ResponseHeader:\n%s\n",response_header.c_str());
        if (s!="")
            printf("Response:\n%s\n",s.c_str());
#endif // debug
        if(response.res != CURLE_OK||HTTPCode!=200)
        {
            curl_easy_cleanup(curl);
//            response.Message="Step4 war nix\n";
            curl_slist_free_all(chunk2);
            WeConnectValues->LoginLevel=0;
            WeConnectValues->LoginMessage="Anfrage4 fehlerhaft!";
#ifdef debug
    printf ("forcedLogin cWeConnect Ende\n");
#endif // debug
            return -10;
        }
        if (s=="")
        {
            curl_easy_cleanup(curl);
            curl_slist_free_all(chunk2);
            WeConnectValues->LoginLevel=0;
            WeConnectValues->LoginMessage="Antwort4 kein Inhalt!";
#ifdef debug
    printf ("forcedLogin cWeConnect Ende\n");
#endif // debug
            return -11;
        }
        cJSON json;
        //     id_token=parse(s,"id_token","\":\"");
//        printf("Start Json\n");
        tokens.access_token=json.getElement(s,"access_token");
//        printf("access_token:\n%s\n",tokens.access_token.c_str());
        tokens.refresh_token=json.getElement(s,"refresh_token");
//        printf("refresh_token:\n%s\n",tokens.refresh_token.c_str());
        tokens.token_type=json.getElement(s,"token_type");
//        printf("token_type:\n%s\n",tokens.token_type.c_str());
        tokens.expires_in=json.getElement(s,"expires_in");
//        printf("expires_in:\n%s\n",tokens.expires_in.c_str());
        tokens.id_token=json.getElement(s,"id_token");
//        printf("id_token:\n%s\n",tokens.id_token.c_str());
        tokens.timestamp=makeTimestampNow();

//        printf("timestamp:\n%s\n",tokens.timestamp.c_str());


        WeConnectValues->LoginLevel=4;
        WeConnectValues->LoginMessage="Starte Schritt 5!";

        if (xclientId=="")
        {
            std::cout<<"Step 5++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
            curl_easy_reset(curl);
            curl_slist *chunk2 = NULL;
            chunk2 = curl_slist_append(chunk2,"Content-Type: application/json");

            res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk2);
            curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
            curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
            curl_easy_setopt(curl, CURLOPT_COOKIEFILE, coockiefile.c_str());
            curl_easy_setopt(curl, CURLOPT_COOKIEJAR, coockiefile.c_str());
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER,0);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);
            //       curl_easy_setopt(curl, CURLOPT_POST, 1L);
            sPostfields = "{\"appId\": \"de.volkswagen.car-net.eu.e-remote\", \"appName\": \"We Connect\", \"appVersion\": \"5.3.2\", \"client_brand\": \"VW\", \"client_name\": \"iPhone\", \"platform\": \"iOS\"}";
            site="https://mbboauth-1d.prd.ece.vwg-connect.com/mbbcoauth/mobile/register/v1";
            curl_easy_setopt(curl, CURLOPT_URL, site.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,sPostfields.size());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sPostfields.c_str());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
            curl_easy_setopt(curl, CURLOPT_POST, 1);
            s="";
            response_header="";
            response.res = curl_easy_perform(curl);
            curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &HTTPCode );
#ifdef debug
        printf("site:\n%s\n",site.c_str());
        printf("sPostfields:\n%s\n",sPostfields.c_str());
        printf("Http Code: %ld\n",HTTPCode);
        if (response_header!="")
            printf("ResponseHeader:\n%s\n",response_header.c_str());
        if (s!="")
            printf("Response:\n%s\n",s.c_str());
#endif // debug
            if(response.res != CURLE_OK|| HTTPCode!=200)
            {

//                response.Message="Step4 war nix\n";
                curl_easy_cleanup(curl);
                curl_slist_free_all(chunk2);
                WeConnectValues->LoginLevel=0;
                WeConnectValues->LoginMessage="Anfrage 5 fehlerhaft!";
#ifdef debug
    printf ("forcedLogin cWeConnect Ende\n");
#endif // debug
                return -12;
            }
        }
        xclientId=json.getElement(s,"client_id");

        WeConnectValues->LoginLevel=5;
        WeConnectValues->LoginMessage="Start Schritt 6!";
        std::cout<<"Step 6 refresh sc2:fal ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
        sc2_fal.tokenForRefresh=tokens.id_token;
        sc2_fal.scope="sc2:fal";
        if (refreshToken(&sc2_fal,OAUTH_URL,"id_token","")<0)
        {
            WeConnectValues->LoginLevel=0;
            WeConnectValues->LoginMessage="Schritt 6 Refresch erfolglos!";
#ifdef debug
    printf ("forcedLogin cWeConnect Ende\n");
#endif // debug
            return -13;
        }
        WeConnectValues->LoginLevel=6;
        WeConnectValues->LoginMessage="Start Schritt 7!";
        std::cout<<"Step 7   refresh t2_v:cubic++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
        t2_v_cubic.tokenForRefresh=sc2_fal.refresh_token;
        t2_v_cubic.scope="t2_v:cubic";
        if (refreshToken(&t2_v_cubic,OAUTH_URL,"refresh_token","")<0)
        {
            WeConnectValues->LoginLevel=0;
            WeConnectValues->LoginMessage="Schritt 6 Refresch erfolglos!";
#ifdef debug
    printf ("forcedLogin cWeConnect Ende\n");
#endif // debug
            return -14;
        }
        WeConnectValues->LoginLevel=7;
        WeConnectValues->LoginMessage="Login erfolgreich!";
#ifdef debug
        printf("site:\n%s\n",site.c_str());
        printf("sPostfields:\n%s\n",sPostfields.c_str());
        if (response_header!="")
            printf("ResponseHeader:\n%s\n",response_header.c_str());
        if (s!="")
            printf("Response:\n%s\n",s.c_str());
#endif // debug

        curl_easy_cleanup(curl);
        curl_slist_free_all(chunk2);
        curl_global_cleanup();
        writeAccess();

    }// ende if (curl)
//   else printf("Curl nicht bereit\n");
    WeConnectValues->loginStatus=1;
#ifdef debug
    printf ("forcedLogin cWeConnect Ende\n");
#endif // debug
    return WeConnectValues->loginStatus;// bis hierhin geschafft, also sind wir eingeloggt
}
int cWeConnect::refreshToken(s__oauth *s_scope,std::string URL,std::string grandType,std::string Post)
{
#ifdef debug
    printf ("refreshToken cWeConnect\n");
#endif // debug
//    printf("refreshToken\n");
    /*    printf("refreshToken \n");
        printf ("Refresh Token Scope: %s\n",s_scope->scope.c_str());
        printf("access_token:\%s\n",s_scope->access_token.c_str());
        printf("token_type:\%s\n",s_scope->token_type.c_str());
        printf("refresh_token:\%s\n",s_scope->refresh_token.c_str());
        printf("expires_in:\%s\n",s_scope->expires_in.c_str());
        printf("scope:\%s\n",s_scope->scope.c_str());
        printf("timestamp:\%s\n",s_scope->timestamp.c_str());
        printf("tokenForRefresh:\%s\n",s_scope->tokenForRefresh.c_str());
     */   CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if(curl)
    {
        struct data config;
        config.trace_ascii = 1; /* enable ascii tracing */
        char buffer[CURL_ERROR_SIZE+1] = {};
        res=curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, buffer);
        curl_slist *chunk2 = NULL;
        curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
        curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, coockiefile.c_str());
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, coockiefile.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER,0);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);
        curl_easy_setopt(curl, CURLOPT_URL, site.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        optionString="X-Client-Id:";
        optionString+=xclientId;
        chunk2 = curl_slist_append(chunk2,optionString.c_str());
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk2);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (Post=="")
            sPostfields = ("grant_type="+grandType+"&scope="+s_scope->scope+"&token="+s_scope->tokenForRefresh);
        else
            sPostfields=Post;
        site=URL;
        curl_easy_setopt(curl, CURLOPT_URL, site.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,sPostfields.size());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, sPostfields.c_str());
        s="";
        response_header="";
        response.res = curl_easy_perform(curl);
        curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &HTTPCode );
        if(response.res != CURLE_OK||HTTPCode>=400)
        {
            printf("Refresh %s war nix\n",s_scope->scope.c_str());
#ifdef debug

            printf("Site:\n%s\n",site.c_str());
            printf("Postfields:\n%s\n",sPostfields.c_str());
            if (response_header!="")
                printf("ResponseHeader:\n%s\n",response_header.c_str());
            if (s!="")
                printf("Response:\n%s\n",s.c_str());
    printf ("refreshToken cWeConnect Ende\n");
#endif // debug
            curl_easy_cleanup(curl);
            curl_slist_free_all(chunk2);
            return -1;
        }
        cJSON json;
        std::string tempstring;
        tempstring=json.getElement(s,"refresh_token");
        if( tempstring!="")
            s_scope->refresh_token=json.getElement(s,"refresh_token");
//        printf("refresh_token:\n%s\n",s_scope->refresh_token.c_str());
        tempstring=json.getElement(s,"expires_in");
        if( tempstring!="")
            s_scope->expires_in=json.getElement(s,"expires_in");
//        printf("expires_in:\n%s\n",s_scope->expires_in.c_str());
        tempstring=json.getElement(s,"access_token");
        if( tempstring!="")
            s_scope->access_token=json.getElement(s,"access_token");
//        printf("access_token:\n%s\n",s_scope->access_token.c_str());
        tempstring=json.getElement(s,"token_type");
        if( tempstring!="")
            s_scope->token_type=json.getElement(s,"token_type");
//        printf("token_type:\n%s\n",s_scope->token_type.c_str());
        s_scope->timestamp=makeTimestampNow();
//        printf("timestamp:\n%s\n",s_scope->timestamp.c_str());
//        printf("scope:\n%s\n",s_scope->scope.c_str());
//        printf ("Refresh %s erfolgreich \n",s_scope->scope.c_str());

#ifdef debug
    printf ("refreshToken cWeConnect Ende\n");
#endif // debug
        return 1;
    }
    printf ("Refresh %s fehlgeschlagen \n",s_scope->scope.c_str());
 #ifdef debug
    printf ("refreshToken cWeConnect Ende\n");
#endif // debug
   return -1;
}
void cWeConnect::writeAccess()
{
    cJSON json;
    std::string jsonString=json.createNewjsonString("identities","");
    jsonString=json.appendToJsonString(jsonString,"identities","user_id",Identities.user_id);
    jsonString=json.appendToJsonString(jsonString,"identities","client_id",Identities.client_id);
    jsonString=json.appendToJsonString(jsonString,"identities","scopes",Identities.scopes);
    jsonString=json.appendToJsonString(jsonString,"identities","consentedScopes",Identities.consentedScopes);
    jsonString=json.appendToJsonString(jsonString,"identities","relayState",Identities.relayState);
    jsonString=json.appendToJsonString(jsonString,"identities","hmac",Identities.hmac);
    jsonString=json.appendToJsonString(jsonString,"identities","profile_url",Identities.profile_url);
    jsonString=json.appendToJsonString(jsonString,"identities","business_id",Identities.business_id);

    jsonString=json.appendToJsonString(jsonString,"","identity_kit","");
    jsonString=json.appendToJsonString(jsonString,"identity_kit","state",identity_kit.state);
    jsonString=json.appendToJsonString(jsonString,"identity_kit","access_token",identity_kit.access_token);
    jsonString=json.appendToJsonString(jsonString,"identity_kit","code",identity_kit.code);
    jsonString=json.appendToJsonString(jsonString,"identity_kit","id_token",identity_kit.id_token);
    jsonString=json.appendToJsonString(jsonString,"identity_kit","token_type",identity_kit.token_type);
    jsonString=json.appendToJsonString(jsonString,"identity_kit","expires_in",identity_kit.expires_in);
    jsonString=json.appendToJsonString(jsonString,"identity_kit","timestamp",identity_kit.timestamp);

    jsonString=json.appendToJsonString(jsonString,"","tokens","");
    jsonString=json.appendToJsonString(jsonString,"tokens","access_token",tokens.access_token);
    jsonString=json.appendToJsonString(jsonString,"tokens","refresh_token",tokens.refresh_token);
    jsonString=json.appendToJsonString(jsonString,"tokens","id_token",tokens.id_token);
    jsonString=json.appendToJsonString(jsonString,"tokens","token_type",tokens.token_type);
    jsonString=json.appendToJsonString(jsonString,"tokens","expires_in",tokens.expires_in);
    jsonString=json.appendToJsonString(jsonString,"tokens","timestamp",tokens.timestamp);
    jsonString=json.appendToJsonString(jsonString,"","xclientId",xclientId);

    jsonString=json.appendToJsonString(jsonString,"","__oauth","");
    jsonString=json.appendToJsonString(jsonString,"__oauth","sc2:fal","");
    jsonString=json.appendToJsonString(jsonString,"sc2:fal","access_token",sc2_fal.access_token);
    jsonString=json.appendToJsonString(jsonString,"sc2:fal","token_type",sc2_fal.token_type);
    jsonString=json.appendToJsonString(jsonString,"sc2:fal","refresh_token",sc2_fal.refresh_token);
    jsonString=json.appendToJsonString(jsonString,"sc2:fal","expires_in",sc2_fal.expires_in);
    jsonString=json.appendToJsonString(jsonString,"sc2:fal","scope",sc2_fal.scope);
    jsonString=json.appendToJsonString(jsonString,"sc2:fal","timestamp",sc2_fal.timestamp);

    jsonString=json.appendToJsonString(jsonString,"__oauth","t2_v:cubic","");
    jsonString=json.appendToJsonString(jsonString,"t2_v:cubic","access_token",t2_v_cubic.access_token);
    jsonString=json.appendToJsonString(jsonString,"t2_v:cubic","token_type",t2_v_cubic.token_type);
    jsonString=json.appendToJsonString(jsonString,"t2_v:cubic","refresh_token",t2_v_cubic.refresh_token);
    jsonString=json.appendToJsonString(jsonString,"t2_v:cubic","expires_in",t2_v_cubic.expires_in);
    jsonString=json.appendToJsonString(jsonString,"t2_v:cubic","scope",t2_v_cubic.scope);
    jsonString=json.appendToJsonString(jsonString,"t2_v:cubic","timestamp",t2_v_cubic.timestamp);
    using namespace std;
    std::fstream f;
    std::string filename="WC"+WeConnectValues->User+"_Access.dat";
    f.open(filename, ios::out);
    f << jsonString <<std::endl;
    f.close();
//   printf("%s geschrieben.\n",filename.c_str());
}
int cWeConnect::readAccess()
{
    using namespace std;
    std::ifstream f;  // Datei-Handle
    std::string s,daten;
    std::string filename="WC"+WeConnectValues->User+"_Access.dat";
    daten="";
    f.open(filename, ios::in); // Öffne Datei aus Parameter
//    while (!f.eof())          // Solange noch Daten vorliegen
    {
        getline(f, s);
        daten+=s;
//        daten+=13;     // Lese eine Zeile
//        printf("Lese ......\n");
    }
    f.close();                // Datei wieder schließen
    if (s=="")
    {
        printf("%s konnte nicht gelesen werden\n",filename.c_str());
        return 0;
    }
//    printf("%s konnte gelesen werden\n",filename.c_str());
    cJSON json;
    std::string sub1,sub2;
    sub1=json.getElement(s,"identities");
    Identities.client_id=json.getElement(sub1,"client_id");
    Identities.consentedScopes=json.getElement(sub1,"consentedScopes");
    Identities.relayState=json.getElement(sub1,"relayState");
    Identities.hmac=json.getElement(sub1,"hmac");
    Identities.profile_url=json.getElement(sub1,"profile_url");
    Identities.business_id=json.getElement(sub1,"business_id");
    Identities.user_id=json.getElement(sub1,"user_id");

    sub1=json.getElement(s,"identity_kit");
    identity_kit.code=json.getElement(sub1,"code");
    identity_kit.access_token=json.getElement(sub1,"access_token");
    identity_kit.expires_in=json.getElement(sub1,"expires_in");
    identity_kit.id_token=json.getElement(sub1,"id_token");
    identity_kit.state=json.getElement(sub1,"state");
    identity_kit.token_type=json.getElement(sub1,"token_type");
    identity_kit.timestamp=json.getElement(sub1,"timestamp");

    sub1=json.getElement(s,"tokens");
    tokens.access_token=json.getElement(sub1,"access_token");
    tokens.refresh_token=json.getElement(sub1,"refresh_token");
    tokens.id_token=json.getElement(sub1,"id_token");
    tokens.token_type=json.getElement(sub1,"token_type");
    tokens.expires_in=json.getElement(sub1,"expires_in");
    tokens.timestamp=json.getElement(sub1,"timestamp");
    xclientId=json.getElement(s,"xclientId");

    sub1=json.getElement(s,"__oauth");
    sub2=json.getElement(sub1,"sc2:fal");
    sc2_fal.access_token=json.getElement(sub2,"access_token");
    sc2_fal.token_type=json.getElement(sub2,"token_type");
    sc2_fal.refresh_token=json.getElement(sub2,"refresh_token");
    sc2_fal.expires_in=json.getElement(sub2,"expires_in");
    sc2_fal.scope="sc2:fal";
    sc2_fal.timestamp=json.getElement(sub2,"timestamp");

    sub2=json.getElement(sub1,"t2_v:cubic");
    t2_v_cubic.access_token=json.getElement(sub2,"access_token");
    t2_v_cubic.token_type=json.getElement(sub2,"token_type");
    t2_v_cubic.refresh_token=json.getElement(sub2,"refresh_token");
    t2_v_cubic.expires_in=json.getElement(sub2,"expires_in");
    t2_v_cubic.scope="t2_v:cubic";
    t2_v_cubic.timestamp=json.getElement(sub2,"timestamp");



    /*
        printf ("identities:\n");
        printf("client_id:\%s\n",Identities.client_id.c_str());
        printf("consentedScopes:\%s\n",Identities.consentedScopes.c_str());
        printf("relayState:\%s\n",Identities.relayState.c_str());
        printf("hmac:\%s\n",Identities.hmac.c_str());
        printf("profile_url:\%s\n",Identities.profile_url.c_str());
        printf("business_id:\%s\n",Identities.business_id.c_str());
        printf("user_id:\%s\n",Identities.user_id.c_str());

        printf ("identity_kit:\n");
        printf("code:\%s\n",identity_kit.code.c_str());
        printf("access_token:\%s\n",identity_kit.access_token.c_str());
        printf("expires_in:\%s\n",identity_kit.expires_in.c_str());
        printf("id_token:\%s\n",identity_kit.id_token.c_str());
        printf("state:\%s\n",identity_kit.state.c_str());
        printf("token_type:\%s\n",identity_kit.token_type.c_str());
        printf("timestamp:\%s\n",identity_kit.timestamp.c_str());

        printf ("tokens:\n");
        printf("access_token:\%s\n",tokens.access_token.c_str());
        printf("refresh_token:\%s\n",tokens.refresh_token.c_str());
        printf("id_token:\%s\n",tokens.id_token.c_str());
        printf("token_type:\%s\n",tokens.token_type.c_str());
        printf("expires_in:\%s\n",tokens.expires_in.c_str());
        printf("timestamp:\%s\n",tokens.timestamp.c_str());
        printf("xclientId:\%s\n",xclientId.c_str());

        printf ("sc2:fal:\n");
        printf("access_token:\%s\n",sc2_fal.access_token.c_str());
        printf("token_type:\%s\n",sc2_fal.token_type.c_str());
        printf("refresh_token:\%s\n",sc2_fal.refresh_token.c_str());
        printf("expires_in:\%s\n",sc2_fal.expires_in.c_str());
        printf("scope:\%s\n",sc2_fal.scope.c_str());
        printf("timestamp:\%s\n",sc2_fal.timestamp.c_str());

        printf ("t2_v_cubic:\n");
        printf("access_token:\%s\n",t2_v_cubic.access_token.c_str());
        printf("token_type:\%s\n",t2_v_cubic.token_type.c_str());
        printf("refresh_token:\%s\n",t2_v_cubic.refresh_token.c_str());
        printf("expires_in:\%s\n",t2_v_cubic.expires_in.c_str());
        printf("scope:\%s\n",t2_v_cubic.scope.c_str());
        printf("timestamp:\%s\n",t2_v_cubic.timestamp.c_str());
     */
    return 1;

}
void cWeConnect::deleteCookies()
{
    std::remove(coockiefile.c_str());
    std::remove(("WC"+WeConnectValues->User+"_Access.dat").c_str());
}
std::string cWeConnect::findID(std::string allData, std::string ID,std::string key)
{
    cJSON json;
//    printf("\nSuche: %s\n",ID.c_str());
    int numberOfData,numberOfFields;
    numberOfData=json.getValueNumberOfFields(allData,key);
    std::string Data;
    for (int i=0; i<numberOfData; i++)
    {
//        printf("Data: %s\n",Data.c_str());
        Data=json.getElement(allData,key,i);
        if (json.getElement(Data,"id")==ID)
        {
//            printf("Gefunden!\n\n\n%s\n",Data.c_str());
            return Data;
        }
    }

    return "";

}
std::string cWeConnect::parse(std::string Message, std::string Objekt,std::string Identifier,std::string key)
{
    int Anfang=0;
    int Ende=0;
    int Matruschka=0;// Zähler für Objekt im Objekt
    std::string suchstring="<";
    suchstring += Objekt;
    std::string endstring="</";
    endstring += Objekt;
    Objekt=suchstring;//sichern, da suchstring später verändert wird
    std::string teststring, teilstring,Header;
    bool gefunden=false;
    int check;
    while (!gefunden)// alles durchsuchen bis gefunden oder Ende erreicht
    {
//    printf("Suchparameter: Suchstring: %s, Endstring: %s\n",suchstring.c_str(),endstring.c_str());
        Anfang =Message.find(suchstring,Anfang);
        if (Anfang <0)
            break; //nicht gefunden
        Ende=Message.find(">",Anfang);// Ende des Headers suchen
        if (Ende <0)
            break; //Dokument fehlerhaft
//    printf ("Ein  Objekt gefunden, nun prüfen ob es das gesuchte ist\n");
        Header=Message.substr(Anfang,Ende+1-Anfang);// Header des Objekts extrahiert, nun prüfen ob es der richtige ist
//    printf ("Header: %s\n",Header.c_str());
        suchstring=Identifier;//welcher Typ wird gesucht
        suchstring+="=\"";
        suchstring+=key;// mit welchem Wert
//    printf("Suchparameter: Suchstring: %s\n",suchstring.c_str());
        check =Header.find(suchstring);
        if (check>=0)
        {
            gefunden=true;// Kombination gefunden, nun das ganze Objekt ermitteln
//            printf("Key passt\n");
            if (Objekt=="<input")
                return Header;// wird mit ">" bereits beendet
            Matruschka=1; // wir hatten ein Objekt gefunden und suchen jetzt das passende Ende

        }
        else
            Anfang=Ende+1;// dieses Objekt ist nicht das gesuchte, Ab dem Ende des Objektes weitersuchen
        suchstring=Objekt;//suchstring wieder zurücksetzten
        while (Matruschka)// so lange durchlaufen bis ein break kommt (Fehler) oder auf 0 runtergezählt wurde
        {
//            printf("Durchlauf %i\n",Matruschka);
            Ende=Message.find(endstring,Anfang);
//           printf("Endstring: %s, Ende= %i\n",endstring.c_str(),Ende);
            if (Ende <0)
                break; // hätte nicht passieren dürfen, Dokument fehlerhaft
            else
                Matruschka --; // ein Abschluss gefunden
            Ende +=endstring.length();
            Ende ++; // noch ein Zeichen für ">" das im Endstring nicht enthalten ist
            check=Message.find(suchstring,Anfang+suchstring.length());// kommt das Objekt nochmal vor?
            if (check<Ende&& check >=0)
                Matruschka ++;// das Objekt ein weiteres mal gefunden und das innerhalb des gesuchten. Also ein Abschluss mehr suchen
//            printf("Matrushka: %i,Ende: %i, neuer Anfang: %i\n",Matruschka,Ende,check);
        }
        if (Ende <0)
            return "";// das Dokument ist fehlerhaft, einen leeren String zurückgeben
    }
    Message=Message.substr(Anfang,Ende-Anfang);
//    printf("Ergebnis:\%s\n",Message.c_str());
//    if (!gefunden) printf("Nicht gefunden\n");
//    else printf ("gefunden\n");

    return Message;
}
std::string cWeConnect::parse(std::string Message,std::string key)
{
    int Anfang=0;
    int Ende=0;
    std::string suchstring=key;
    suchstring +="=\"";
    std::string endstring="\"";
    Anfang=Message.find (suchstring);
    if (Anfang <0)
        return "";
    Anfang+=suchstring.length();
    Ende=Message.find(endstring,Anfang);
    Message=Message.substr(Anfang,Ende-Anfang);
    return Message;
}
std::string cWeConnect::parse(std::string Message,std::string key,std::string Separator)
{
    int Anfang=0;
    int Ende=0;
    std::string suchstring=key;
    suchstring +Separator;
    Anfang=Message.find (suchstring);
    if (Anfang <0)
        return "";
    Anfang+=suchstring.length();
    Anfang+=Separator.length();
    Ende=Message.find(13,Anfang);// ein CR suchen
    if (Ende<0)
        Ende=Message.length();
    int neuEnde=Message.find("\"",Anfang);
    if(neuEnde<Ende&&neuEnde>=0)
        Ende=neuEnde;
    neuEnde=Message.find("&",Anfang);
    if(neuEnde<Ende&&neuEnde>=0)
        Ende=neuEnde;
    neuEnde=Message.find("}",Anfang);
    if(neuEnde<Ende&&neuEnde>=0)
        Ende=neuEnde;
    Message=Message.substr(Anfang,Ende-Anfang);
    return Message;
}

bool cWeConnect::checkSPin(std::string sSPin)
{
    int x=sSPin.size();
    if (x !=4)
        return false;
    for (int  i=0; i<4; i++)
    {
        if (sSPin[i]<48 || sSPin[i]>57)
            return false;
    }
    return true;
}
void cWeConnect::setValuesFromBrand()
{
    if (CarBrand== "id")
    {
        type = "Id";
        Country = "DE";
        clientId = "a24fba63-34b3-4d43-b181-942111e6bda8@apps_vw-dilab_com";
        xclientId = "";
        scope = "openid profile badge cars dealers birthdate vin";
        redirect = "weconnect://authenticated";
        xrequest = "com.volkswagen.weconnect";
        responseType = "code id_token token";
        xappversion = "";
        xappname = "";
    }
    if (CarBrand== "skoda")
    {
        type = "Skoda";
        Country = "CZ";
        clientId = "7f045eee-7003-4379-9968-9355ed2adb06%40apps_vw-dilab_com";
        xclientId = "28cd30c6-dee7-4529-a0e6-b1e07ff90b79";
        scope = "openid%20profile%20phone%20address%20cars%20email%20birthdate%20badge%20dealers%20driversLicense%20mbb";
        redirect = "skodaconnect%3A%2F%2Foidc.login%2F";
        xrequest = "cz.skodaauto.connect";
        responseType = "code%20id_token";
        xappversion = "3.2.6";
        xappname = "cz.skodaauto.connect";
    }
    if (CarBrand== "seat")
    {
        type = "Seat";
        Country = "ES";
        clientId = "50f215ac-4444-4230-9fb1-fe15cd1a9bcc@apps_vw-dilab_com";
        xclientId = "9dcc70f0-8e79-423a-a3fa-4065d99088b4";
        scope = "openid profile mbb cars birthdate nickname address phone";
        redirect = "seatconnect://identity-kit/login";
        xrequest = "cz.skodaauto.connect";
        responseType = "code%20id_token";
        xappversion = "1.1.29";
        xappname = "SEATConnect";
    }
    if (CarBrand== "vwv2")
    {
        type = "VW";
        Country = "DE";
        clientId = "9496332b-ea03-4091-a224-8c746b885068@apps_vw-dilab_com";
        xclientId = "89312f5d-b853-4965-a471-b0859ee468af";
        scope = "openid profile mbb cars birthdate nickname address phone";
        redirect = "carnet://identity-kit/login";
        xrequest = "de.volkswagen.car-net.eu.e-remote";
        responseType = "id_token%20token%20code";
        xappversion = "5.6.7";
        xappname = "We Connect";
    }
    if (CarBrand== "audi")
    {
        type = "Audi";
        Country = "DE";
        clientId = "09b6cbec-cd19-4589-82fd-363dfa8c24da@apps_vw-dilab_com";
        xclientId = "77869e21-e30a-4a92-b016-48ab7d3db1d8";
        scope = "address profile badge birthdate birthplace nationalIdentifier nationality profession email vin phone nickname name picture mbb gallery openid";
        redirect = "myaudi:///";
        xrequest = "de.myaudi.mobile.assistant";
        responseType = "token%20id_token";
        // responseType = "code";
        xappversion = "3.22.0";
        xappname = "myAudi";
    }
    if (CarBrand== "go")
    {
        type = "";
        Country = "";
        clientId = "ac42b0fa-3b11-48a0-a941-43a399e7ef84@apps_vw-dilab_com";
        xclientId = "";
        scope = "openid%20profile%20address%20email%20phone";
        redirect = "vwconnect%3A%2F%2Fde.volkswagen.vwconnect%2Foauth2redirect%2Fidentitykit";
        xrequest = "";
        responseType = "code";
        xappversion = "";
        xappname = "";
    }

}
void cWeConnect::setUser(std::string sUser,std::string sPassWd)
{
  WeConnectValues->User=sUser;
   WeConnectValues->PassWd=sPassWd;
    coockiefile="WC"+WeConnectValues->User+"_coockie.txt";


}
void cWeConnect::setUser(std::string sUser,std::string sPassWd, std::string sSPin)
{
        WeConnectValues->User=sUser;
    WeConnectValues->PassWd=sPassWd;
    SPin=sSPin;
    coockiefile="WC"+WeConnectValues->User+"_coockie.txt";

}
void cWeConnect::setUser(std::string sUser,std::string sPassWd,std::string sSPin, std::string sFin)
{
    WeConnectValues->User=sUser;
    WeConnectValues->PassWd=sPassWd;
    SPin=sSPin;
    Fin=sFin;
    coockiefile="WC"+WeConnectValues->User+"_coockie.txt";

}
void cWeConnect::setUser(std::string sUser,std::string sPassWd,std::string sSPin, std::string sFin,std::string sCarBrand)
{
    WeConnectValues->User=sUser;
    WeConnectValues->PassWd=sPassWd;
    SPin=sSPin;
    Fin=sFin;
    CarBrand=sCarBrand;
    coockiefile="WC"+WeConnectValues->User+"_coockie.txt";

}
float cWeConnect::Kd2float(float input)
{
    input-=2731;
    input=input/10;
    return input;
}
float cWeConnect::Kd2float(std::string Stringinput)
{
    int input;
    string2int(Stringinput,&input);
    input-=2731;
    input=input/10;
    return input;
}
float cWeConnect::float2Kd(float input)
{
    input=input*10;
    input+=2731;
    return input;
}

std::string cWeConnect::createRandomString(int Anzahl)
{
    char RandomLetter[]="0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_";
    std::random_device rd;
    std::default_random_engine eng(rd());
    std::uniform_int_distribution<int> distr(0, sizeof(RandomLetter)-2);
//    printf ("RandomLetterSize= %i\n",sizeof(RandomLetter)-2);
    std::string RandomString="";
    for (int i=0; i< Anzahl; i++)
    {
        RandomString += RandomLetter[distr(eng)];
    }
#ifdef debug
    printf ("Zufallsstring: %s\n",RandomString.c_str());
#endif // debug
    return RandomString;
}
void cWeConnect::createCodeChallenge()
{
    state = createRandomString(43);
    code_verifier=createRandomString(43);
    code_challenge=createHash(code_verifier);
    code_challenge=HexToString(code_challenge);
    code_challenge=Encode(code_challenge);
#ifdef debug
    printf ("code_veryfier: %s\n",code_verifier.c_str());
    printf ("code_challenge: %s\n",code_challenge.c_str());
#endif // debug

}
std::string cWeConnect::get_HeaderElement(std::string Input,std::string Key)
{
//    printf("Suchparameter: %s, %s\n", Input.c_str(), Key.c_str());
    int Anfang= Input.find(Key);
    if (Anfang <0)
        return "";// Key nicht gefunden, Abbruch
//   printf("Suche nach %s ergab: %i\n%s\n",Key.c_str(),Anfang,Input.substr(Anfang,Input.length()-Anfang).c_str());
    Anfang+=Key.length();// Anfang hinter das Schlüsselwort setzten
    while (Input.find(" ",Anfang,1)==Anfang||Input.find(":",Anfang,1)==Anfang||Input.find("=",Anfang,1)==Anfang||Input.find("\"",Anfang,1)==Anfang)
        Anfang++;// Anfangsposition trimmen
    int Ende=Input.find(13,Anfang);// erst mal Ende der Zeile suchen
    if (Ende <0)
        Ende = Input.length();// kein LF also ganzer Input, kein Mehrzeiler
    else
        return Input.substr(Anfang,Ende-Anfang);// bei mehrzeiligen Inputs die ganze Zeile zurückgeben, nicht weiter zerstückeln
//    printf("LF suche ergab: %i\n%s\n",Ende,Input.substr(Anfang,Ende-Anfang).c_str());
    int newEnde = Input.find('&',Anfang);// kommt vor dem Ende noch ein & ?
//    printf("& suche ergab: %i\n%s\n",newEnde,Input.substr(Anfang,newEnde-Anfang).c_str());
    if (newEnde <0|| newEnde>Ende)
        newEnde=Ende;// kein & gefunden also alten Zustand herstellen
    Ende=newEnde;

    newEnde = Input.find('\"',Anfang);// kommt vor dem Ende noch ein & ?
//    printf("& suche ergab: %i\n%s\n",newEnde,Input.substr(Anfang,newEnde-Anfang).c_str());
    if (newEnde <0|| newEnde>Ende)
        newEnde=Ende;// kein & gefunden also alten Zustand herstellen
//    printf("\"suche ergab: %i\n%s\n",newEnde,Input.substr(Anfang,Ende-Anfang).c_str());
    Ende=newEnde;

    newEnde = Input.find('?',Anfang);// kommt vor dem Ende noch ein & ?
//    printf("& suche ergab: %i\n%s\n",newEnde,Input.substr(Anfang,newEnde-Anfang).c_str());
    if (newEnde <0|| newEnde>Ende)
        newEnde=Ende;// kein & gefunden also alten Zustand herstellen
//    printf("\"suche ergab: %i\n%s\n",newEnde,Input.substr(Anfang,Ende-Anfang).c_str());

    Input=Input.substr(Anfang,newEnde-Anfang);


    return Input;
}
std::string cWeConnect::makeTimestampNow()
{
    return std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
}
std::string cWeConnect::timestamp2string(long long sTimestamp)
{
    std::time_t temp = sTimestamp/1000;
    std::tm* t = std::localtime(&temp);
    std::stringstream ss; // or if you're going to print, just input directly into the output stream
    ss << std::put_time(t, "%d.%m.%Y %H:%M");
    std::string output = ss.str();
    return output;
}

long long cWeConnect::string2timestamp(std::string sTimestamp)
{
    std::tm t{};
    std::istringstream ss(sTimestamp);

    ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail())
    {
        return (long long)0;

    }
    return (long long) (mktime(&t)-timezone)*1000;
}

std::string  cWeConnect::parseUPR(std::string input)
{
    int pos=input.find("://");
    pos +=4;
    pos=input.find("/",pos);
    return input.substr(0,pos);
}
void cWeConnect::updateTimestamp(long long *oldTimestamp,long long newTimestamp)
{
        if (newTimestamp > *oldTimestamp)
        *oldTimestamp=newTimestamp;
}

///////// ab hier Funktionen die nicht zu Klassen gehören
std::string createHash(const std::string str)
{
    using namespace std;

    char HEX[2];
    HEX[0]=92;
    HEX[1]=120;
    HEX[2]=0;
    unsigned char digest[SHA256_DIGEST_LENGTH];
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str();

}
static std::string Encode(const std::string & in)
{

    const char base64_url_alphabet[] =
    {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_'
    };

    std::string out;
    int val =0, valb=-6;
    size_t len = in.length();
    unsigned int i = 0;
    for (i = 0; i < len; i++)
    {
        unsigned char c = in[i];
        val = (val<<8) + c;
        valb += 8;
        while (valb >= 0)
        {
            out.push_back(base64_url_alphabet[(val>>valb)&0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6)
    {
        out.push_back(base64_url_alphabet[((val<<8)>>(valb+8))&0x3F]);
    }
    return out;
}
static void dump(const char *text, FILE *stream, unsigned char *ptr, size_t size, char nohex)
{
    size_t i;
    size_t c;

    unsigned int width = 0x10;

    if(nohex)
        /* without the hex output, we can fit more on screen */
        width = 0x40;

    fprintf(stream, "%s, %10.10lu bytes (0x%8.8lx)\n",
            text, (unsigned long)size, (unsigned long)size);

    for(i = 0; i<size; i += width)
    {

        fprintf(stream, "%4.4lx: ", (unsigned long)i);

        if(!nohex)
        {
            /* hex not disabled, show it */
            for(c = 0; c < width; c++)
                if(i + c < size)
                    fprintf(stream, "%02x ", ptr[i + c]);
                else
                    fputs("   ", stream);
        }

        for(c = 0; (c < width) && (i + c < size); c++)
        {
            /* check for 0D0A; if found, skip past and start a new line of output */
            if(nohex && (i + c + 1 < size) && ptr[i + c] == 0x0D &&
                    ptr[i + c + 1] == 0x0A)
            {
                i += (c + 2 - width);
                break;
            }
            fprintf(stream, "%c",
                    (ptr[i + c] >= 0x20) && (ptr[i + c]<0x80)?ptr[i + c]:'.');
            /* check again for 0D0A, to avoid an extra \n if it's at width */
            if(nohex && (i + c + 2 < size) && ptr[i + c + 1] == 0x0D &&
                    ptr[i + c + 2] == 0x0A)
            {
                i += (c + 3 - width);
                break;
            }
        }
        fputc('\n', stream); /* newline */
    }
    fflush(stream);
}
static int my_trace(CURL *handle, curl_infotype type, char *data, size_t size, void *userp)
{
    struct data *config = (struct data *)userp;
    const char *text;
    (void)handle; /* prevent compiler warning */

    switch(type)
    {
    case CURLINFO_TEXT:
        fprintf(stderr, "== Info: %s", data);
    /* FALLTHROUGH */
    default: /* in case a new one is introduced to shock us */
        return 0;

    case CURLINFO_HEADER_OUT:
        text = "=> Send header";
        break;
    case CURLINFO_DATA_OUT:
        text = "=> Send data";
        break;
    case CURLINFO_SSL_DATA_OUT:
        text = "=> Send SSL data";
        break;
    case CURLINFO_HEADER_IN:
        text = "<= Recv header";
        break;
    case CURLINFO_DATA_IN:
        text = "<= Recv data";
        break;
    case CURLINFO_SSL_DATA_IN:
        text = "<= Recv SSL data";
        break;
    }

    dump(text, stderr, (unsigned char *)data, size, config->trace_ascii);
    return 0;
}
size_t CurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, std::string *s)
{
//    size_t newLength = size * nmemb;
//    size_t oldLength = s->size();
    size_t realsize = size * nmemb;
    //  try
    {
        ((std::string*)s)->append((char*)contents, size * nmemb);

    }
    /*    catch(std::bad_alloc &e)
        {
            //handle memory problem
            return 0;
        }
    */    //  std::copy((char*)contents, (char*)contents + newLength, s->begin() + oldLength);
    return realsize;
}
static size_t header_callback(char *received_data, size_t size,size_t nitems, void *response_header)
{
    size_t realsize = size * nitems;
    //  try
    {
        ((std::string*)response_header)->append((char*)received_data, size * nitems);

    }
    return realsize;
}


