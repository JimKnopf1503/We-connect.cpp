#ifndef WEB_H
#define WEB_H

#include <wx/string.h>
#include <curl/curl.h>
#include <curl/stdcheaders.h>
#include <curl/easy.h>

#include <vector>
#include <thread>
#include <atomic>
#include <mutex>


    struct Car_Info
    {
        int successlevel;
        long TimestampPoll;
        std::string VIN;
        std::string CarName;
        int stateOfCharge;
        std::string chargingState;
        std::string remainingChargingTime;
        std::string chargingReason;
        std::string pluginState;
        std::string lockState;
        std::string extPowerSupplyState;
        int electricRange;
        int chargerMaxCurrent;
        int maxAmpere;
        std::string climatisationState;
        int climatisationRemaningTime;
        std::string windowHeatingStateFront;
        std::string windowHeatingStateRear;
        std::string climatisationReason;
        bool windowHeatingAvailable;
        float targetTemperature;
        bool climatisationWithoutHVPower;
        std::string climaterActionState;
        std::string lat;
        std::string lng;
        std::string parkingLight;
        std::string hood;
        int DoorLF;
        int DoorRF;
        int DoorLB;
        int DoorRB;
        int DoorTrunk;
        int LockDoorLF;
        int LockDoorRF;
        int LockDoorLB;
        int LockDoorRB;
        int LockDoorTrunk;
        int WindowLF;
        int WindowRF;
        int WindowLB;
        int WindowRB;
        int WindowSunroof;
        std::string LatestmessageList;
        long lastConnectionTimeStamp;
        int distanceCovered;
        int serviceInDays;
        int serviceInKm;
        std::string NewmessageList;


    };
  struct CURL_Response
    {
        CURLcode res;
        bool CarnetDebug;
        long response_code;
        wxString Redirect_to;
        wxString CSRF;
        std::string Message;
        std::string errCode;
        std::string csrf,User,Passw,VIN,sPin;
        int PollIntervall;
        Car_Info Car;
        bool thread_runing;
        bool request_in_progress;
    };

class WEB
{
    public:
        WEB();
        virtual ~WEB();
        bool dodebug;
        CURL_Response CARNET();
        CURL_Response CARNET(std::string command);
        void StartThread();
        void setUser(std::string CARNET_USERNAME_t, std::string CARNET_PASSWORD_t);
        void StopThread();
    std::atomic<bool> m_stopCarNet{true};
    protected:

    private:
        CURL_Response init_response(CURL_Response response);
        void unpack_emanager(std::string message);
        void unpack_fullyLoadedVehicles(std::string message);
        void unpack_vehicleDetails(std::string message);
        void unpack_position(std::string message);
        void unpack_vehicleStatusData(std::string message);
        CURL_Response do_login (CURL *curl,CURL_Response car);
        void do_request (CURL *curl);
        void do_command (CURL *curl,std::string c_url,std::string command);
        void do_logout(CURL *curl);
        void set_requstheader (CURL *curl,curl_slist *chunk);
        void set_requstheader (CURL *curl,curl_slist *chunk, std::string CSRF_token, std::string Referer);
        void set_actionheader (CURL *curl,curl_slist *chunk, std::string CSRF_token, std::string Referer);
        void set_auth_requstheader (CURL *curl,curl_slist *chunk);
        void set_auth_requstheader (CURL *curl,curl_slist *chunk, std::string CSRF_token, std::string Referer);
        std::string tolowerString(std::string Input);
        std::string removeNewLineCaracter (std::string Input);
        std::string trim(std::string Input);
        std::string extract_url_parameter(std::string Input,std::string Key);
    std::string get_HeaderElement(std::string Input,std::string Key);
    std::string get_HTTPValue(std::string Input,std::string Key);
    std::string extract_csrf(std::string Input,std::string Key);
    std::string get_logoutURL(std::string Input,std::string Key);
        CURLcode perform_request(CURL *curl,std::string  URL);
        CURLcode perform_request(CURL *curl,std::string  URL,std::string sPostfields);
        CURL_Response response;
        std::string getValue(std::string Message, std::string keyword1);
        std::string getValue(std::string Message, std::string keyword1,int Field);
        char* To_CharArray(const std::string &Text);
        std::string site,portal_base_url,auth_base_url,landing_page_url,base_url,login_url,login_form_url,login_action_url,login_action2_url,ref2_url,portlet_code,state,final_login_url,base_json_url,header,client_id,login_relay_state_token,hmac_token1,hmac_token2,login_csrf,csrf,get_login_url,base_json_urlVIN,eManagerResult,lastLocationResult,VehicleStatusResult,FullyLoadedCarsResponse,LatestMessageResponse,VehicleDetailsResponse,NewMessageResponse,RahStatusResponse;
        std::string s;
        long http_responsecode;
        std::string sPostfields;
        std::string response_header="";
        std::string token;
        std::thread m_thread;
        void theThread();
        std::string CARNET_USERNAME;
        std::string CARNET_PASSWORD;
        std::mutex myMutex;
        int findBracket(std::string Input,std::string bracket);
        int findBracket(std::string Input,std::string bracket,int startpos);
};

#endif // WEB_H
