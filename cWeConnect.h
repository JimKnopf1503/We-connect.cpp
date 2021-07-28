#ifndef CWECONNECT_H
#define CWECONNECT_H
#include <string>
#include <cstring>
#include <curl/curl.h>
#include <curl/stdcheaders.h>
#include <curl/easy.h>
#include "openssl/sha.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <random>
#include <unistd.h>
#include <fstream>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <mutex>
#include "myFunctions.h"
#include "cJSON.h"
#define maxCars 9
/*
needs libssl-dev,libcurl4-openssl-dev,
cJSON.h aand cJSON.cpp by Burkhard Venus
*/




// Funktionen die nicht in Klassen untergebracht werden können oder sollen
static int my_trace(CURL *handle, curl_infotype type,char *data, size_t size, void *userp);
static void dump(const char *text, FILE *stream, unsigned char *ptr, size_t size, char nohex);
static std::string Encode(const std::string & in);
std::string createHash(const std::string str);
static std::string Encode(const std::string & in);
std::string createHash(const std::string str);
size_t CurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, std::string *s);
static size_t header_callback(char *received_data, size_t size,size_t nitems, void *response_header);
//////////////////////
struct sIdentities
{
    std::string user_id;
    std::string client_id;
    std::string scopes;
    std::string consentedScopes;
    std::string relayState;
    std::string hmac;
    std::string profile_url;
    std::string business_id;
};
struct sidentity_kit
{
     std::string state;
     std::string code;
     std::string access_token;
     std::string expires_in;
     std::string token_type;
     std::string id_token;
    std::string timestamp;
};
struct s__oauth
{
    std::string id_token;
    std::string access_token;
    std::string token_type;
    std::string refresh_token;
    std::string tokenForRefresh;
    std::string expires_in;
    std::string scope;
    std::string timestamp;
};
struct sWEBresult
{
    int succsess;
    std::string result;
    std::string header;
    int HTTP_Code;

};
struct data
{
    char trace_ascii; /* 1 or 0 */
};
struct sConnectionData
{
    s__oauth sc2_fal;
    s__oauth t2_v_cubic;
};
struct CURL_Response
{
    CURLcode res;
    std::string header;
    std::string result;
    long HTTP_Code;
    int success;
};
struct sADRESS
{
    std::string country;
    std::string usageType;
    std::string uuid;
    std::string primary;
    std::string name;
    std::string street;
    std::string housenumber;
    std::string zipCode;
    std::string city;
    std::string _updated;
    std::string type;

};
struct sUserData
{
    sADRESS Homeadress;
    sADRESS Billingadress;
    std::string firstName;
    std::string lastName;
    std::string salutation;
    std::string dateOfBirth;
    std::string preferredContactChannel;
    std::string nickname;
    std::string businessIdentifierType;
    std::string businessIdentifierValue;
    std::string userIsPrivateIndicator;
    std::string preferredLanguage;

};
struct sRealCarData
{
    std::string nickname;
    std::string licensePlateNumber;
    std::string allocatedDealerCountry;
    std::string allocatedDealerId;
    std::string allocatedDealerBrandCode;
    std::string carnetAllocationTimestamp;
    std::string carnetAllocationType;
    std::string carNetIndicator;
    std::string commissionNumber;
    std::string vehicleIdentificationNumber;
    std::string deactivated;
    std::string deactivationReason;
    std::string modelCode;
    std::string commissionNumberYear;
};
struct sMbbStatus
{
     std::string profileCompleted;
      std::string spinDefined;
       std::string carnetEnrollmentCountry;
        std::string etag;

};
struct sEmbeddedSimIdentification
{
    std::string  type;
    std::string  content;

};
struct sEmbeddedSim
{
    sEmbeddedSimIdentification Identification;
    std::string  identification;
    std::string  imei;
    std::string  mno;

};
struct sCarportData
{
    std::string  modelCode;
    std::string  modelName;
    std::string  modelYear;
    std::string  color;
    std::string  countryCode;
    std::string  engine;
    std::string  mmi;
    std::string  transmission;
};
struct sVehicleDevice
{
    std::string  deviceType;
    std::string  ecuGeneration;
    std::string  deviceId;
    sEmbeddedSim EmbeddedSim;
};
struct sDoor
{
    int DoorLocked;
    int DoorCloesed;
    int DoorSave;
};
struct sCarInfos// takes Values from getVehicleData() and getVehicleStatus
{
    std::string  systemId;
    std::string  requestId;
    std::string  brand;
    std::string  country;
    std::string  isConnect;
    std::string  isConnectSorglosReady;
    sVehicleDevice VehicleDevice[9];
    sCarportData CarportData;
    float aussentemperatur;
    int SOC=-1;
    long long timestamp;
    int InspektionDistance;
    int InspektionTime;
    int Reichweite;
    std::string latitude;
    std::string longitude;
    long long parkingtime;
    int direction;
    float targettemperature;
    bool climatisationWithoutHVPower;
    std::string heaterSource;
    std::string climatisationState;
    std::string climatisationStateErrorCode;
    int remainingClimatisationTime;
    std::string climatisationReason;
    std::string windowHeatingStateFront;
    std::string windowHeatingStateRear;
    std::string windowHeatingErrorCode;
    int maxChargeCurrent;
    std::string chargingMode;
    std::string chargingStateErrorCode;
    std::string chargingReason;
    std::string externalPowerSupplyState;
    std::string energyFlow;
    std::string chargingState;
    int remainingChargingTime;
    std::string plugState;
    std::string lockState;
    std::string speed;
    sDoor Door1;
    sDoor Door2;
    sDoor Door3;
    sDoor Door4;
    sDoor Door5;
    sDoor Door6;


};
struct sWeConnectValues
{
    std::string User="";
    std::string PassWd="";
    std::string nickname="";
    std::string licensePlateNumber="";
    int loginStatus;
    int LoginLevel=0;
    std::string LoginMessage="";
    int successLevel;
    int AnzFahrzeuge;
    sUserData UserData;
    sRealCarData RealCarData[maxCars];
    sMbbStatus mbbStatus;
    sCarInfos CarInfo;
    std::string actualFin;
    int PollIntervallSlow;
    int PollIntervallMiddle;
    int PollIntervallFast;
    bool thread_runing;
    bool request_in_progress;
    bool m_stopThread;
    int hasno_users=2;
    int hasno_fences=2;
    int hasno_fences_configuration=2;
    int hasno_VehicleData=2;
    int hasno_VehicleStatus=2;
    int hasno_trip_data=2;//type: 'longTerm', 'cyclic', 'shortTerm'
    int hasno_departure_timer=2;
    int hasno_status_update=2;
    int hasno_speed_alerts=2;
    int hasno_speed_alerts_configuration=2;
    int hasno_climater=2;
    int hasno_position=2;
    int hasno_destinations=2;
    int hasno_charger=2;
    int hasno_heating_status=2;
    int hasno_history=2;
    int hasno_fetched_role=2;
    int hasno_roles_rights=2;


};
class cWeConnect
{
public:
 //   sConnectionData ConnectionData;
    sWeConnectValues *WeConnectValues;
    bool *request_in_progress;
    cWeConnect();
    sIdentities Identities;
    sidentity_kit identity_kit;
    s__oauth tokens;
    s__oauth sc2_fal;
    s__oauth t2_v_cubic;
    void init(sWeConnectValues *WeConnectValuesMain,bool *request_in_progressMaoin);
    void setUser(std::string sUser,std::string sPassWd);
    void setUser(std::string sUser,std::string sPassWd, std::string sSPin);
    void setUser(std::string sUser,std::string sPassWd,std::string sSPin, std::string sFin);
    void setUser(std::string sUser,std::string sPassWd,std::string sSPin, std::string sFin,std::string sCarBrand);
    void StartThread(int i);
    void StopThread(int i);
    void pollSlowData(cWeConnect *WeConnect);
    void pollMiddleData(cWeConnect *WeConnect);
    void pollFastData(cWeConnect *WeConnect);
    int checkOuthTokens();
    int checkOuthScope(s__oauth *Scope);
    int checkKitTokens();
    int checkTokens();
    int Login();
    int forcedLogin();
    int refreshToken(s__oauth *s_scope,std::string URL,std::string grandType,std::string Post);
    CURL_Response GET_POST_URL(std::string URL,std::string HEADER,std::string GET,std::string POST,std::string JSON,int FOLLOW);
    int getPersonalData();
    int getRealCarData();
    int getRealCarData(std::string FIN);
    int get_mbbStatus();
    int getVehicles();
    int get_identity_data();
    int get_users(std::string VIN);
    int get_fences(std::string VIN);
    int get_fences_configuration();
    int getVehicleData(std::string VIN);
    int getVehicleStatus(std::string VIN);
    int get_trip_data(std::string VIN,std::string type);//type: 'longTerm', 'cyclic', 'shortTerm'
    int get_departure_timer(std::string VIN);
    int get_speed_alerts(std::string VIN);
    int get_speed_alerts_configuration();
    int get_climater(std::string VIN);
    int get_position(std::string VIN);
    int get_destinations(std::string VIN);
    int get_charger(std::string VIN);
    int get_heating_status(std::string VIN);
    int get_history(std::string VIN);
    int get_roles_rights(std::string VIN);
    int get_fetched_role(std::string VIN);
    int request_status_update(std::string VIN);
    int __flash_and_honk(std::string VIN,std::string mode,std::string latitude,std::string longitude);
    int get_honk_and_flash_configuration();
    int climatisation(std::string VIN,std::string ACTION);
    int battery_charge(std::string VIN,std::string ACTION);
    int climatisation_temperature(std::string VIN,float temperature);
    int window_melt(std::string VIN,std::string ACTION);
    sWEBresult standardWebabfrage(std::string urlAnhang,std::string Dashboard,std::string scope,std::string _accept,std::string _post);
    sWEBresult standardPost(std::string urlAnhang,std::string Dashboard,std::string scope,std::string _accept,std::string _post);
    void getPersonalDataFillAdress(std::string input,sADRESS *Adress);
    void getRealCarDataFillCars(std::string input,sRealCarData *tRealCarData);
    std::string createRandomString(int Anzahl);
    void createCodeChallenge();//erzeugt CodeChallenge und CodeVeryfier
    void setValuesFromBrand();
    bool checkSPin(std::string sSPin);
    std::string parse(std::string Message, std::string Objekt,std::string Identifier,std::string key);
    std::string parse(std::string Message,std::string key);
    std::string parse(std::string Message,std::string key,std::string Separator);
    std::string get_HeaderElement(std::string Input,std::string Key);
    void writeAccess();
    int readAccess();
    void deleteCookies();
    std::string findID(std::string allData, std::string ID,std::string key);
    std::string makeTimestampNow();
    float Kd2float(float input);
    float Kd2float(std::string Stringinput);
    float float2Kd(float input);
    long long string2timestamp(std::string sTimestamp);
    std::string timestamp2string(long long sTimestamp);
    void updateTimestamp(long long *oldTimestamp,long long newTimestamp);
    std::string CarBrand;
    std::string type="vw";
    std::string Country;
    std::string clientId="9496332b-ea03-4091-a224-8c746b885068%40apps_vw-dilab_com";
    std::string xclientId;// = "38761134-34d0-41f3-9a73-c4be88d7d337";
    std::string scope = "openid%20profile%20mbb%20email%20cars%20birthdate%20badge%20address%20vin";
    std::string redirect = "carnet%3A%2F%2Fidentity-kit%2Flogin";
    std::string xrequest = "de.volkswagen.carnet.eu.eremote";
    std::string responseType = "id_token%20token%20code";
    std::string profileURL="https://customer-profile.apps.emea.vwapps.io/v1/customers/";
    std::string USER_URL = "https://userinformationservice.apps.emea.vwapps.io/iaa";
    std::string OAUTH_URL = "https://mbboauth-1d.prd.ece.vwg-connect.com/mbbcoauth/mobile/oauth2/v1/token";
    std::string BASE_URL = "https://msg.volkswagen.de/fs-car";
    std::string host="customer-profile.apps.emea.vwapps.io";
    std::string __accept_mbb = "application/json, text/plain, application/vnd.volkswagenag.com-error-v1+json, */*";
    std::string MAL_URL = "https://mal-1a.prd.ece.vwg-connect.com/api";
    std::string coockiefile="coockie.txt";
    std::string TOKEN_URL = "https://tokenrefreshservice.apps.emea.vwapps.io";

    std::string userId;
    std::string state;
    std::string xappversion = "5.3.2";
    std::string xappname = "We Connect";
    std::string code_challenge;
    std::string code_verifier;
//    std::string User;
//    std::string PassWd;
    std::string SPin="1234";
    std::string Fin;
    int LoginStatus=-1;// 0= keine Fehler und eingeloggt, -1 kein Fehler aber nicht eingeloggt, -2 SPin ungültig
    std::string timestamp;
    virtual ~cWeConnect();
void theThread(int i,std::string User,std::string PassWd,std::string VIN);

protected:

private:
    CURL_Response response;
    std::string url="";
    std::string site="";
    std::string optionString="";
    std::string s;
    std::string response_header="";
    std::string sPostfields;
    std::string action;
    std::string hmac;
    std::string relayState;
    std::string CSRF;
    std::string Form;
    long HTTPCode;
    std::string auth_code;
    std::string id_token;
    std::string access_token;
    std::string expires_in;
    std::string refresh_token;
    std::thread m_thread[maxCars];
    std::string  parseUPR(std::string input);

};

#endif // CWECONNECT_H
