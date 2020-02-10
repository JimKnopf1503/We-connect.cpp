//#define __CarNetTest___
//#define __debug__

/*
WEB is a class to connect to Volkswagen We-Connect.
It ist based on the pythonscript "we_connect_client.py", written by Rene Boer, https://github.com/reneboer/python-carnet-client
Author of WEB: Burkhard Venus
last Date: 03.02.2020
It runs right now only on linux sytems. It must be CUL/libcurl installed. Tested with libcurl 7.38.
In the main app you musst creat an object of web like: WEB CarNet;
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
CarNet.CARNET(command); // send a command to the car as a string. Possible commands are "start clima", "stop clima",,"start windowmelt","stop windowmelt","start charge","stop charge".

You get back a CURL_Response. CURL_Response.errCode returns the error code from the command call as a string. "0" means all ok.
*/

#include "WEB.h"
#include <string.h>
#include <iostream>
#include <algorithm>
#include <locale>        
#include <sstream>
#include <unistd.h>
#include <sstream>
#include <iomanip>

#ifdef __CarNetTest___
#include <wx/string.h>
#include <wx/msgdlg.h>
#endif
#include <unistd.h>
#include <chrono>
#include <algorithm>
using namespace std;
size_t CurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, std::string *s);
static size_t header_callback(char *buffer, size_t size,size_t nitems, void *userdata);
char * string2Char (std::string Input);
static struct curl_slist *slist_get_last(struct curl_slist *list);

WEB::WEB()
{
    m_stopCarNet=false;//no thread yet
    response.PollIntervall=300;// if thread is used
    curl_global_init(CURL_GLOBAL_ALL);
}

WEB::~WEB()
{
    m_stopCarNet=true;
    StopThread();
}
void WEB::StartThread()
{
    extern  CURL_Response car;//get access to the struct in the main app
    myMutex.lock();
    if(!car.thread_runing)// only run once
    {
        m_stopCarNet = false;//m_stopCarNet is checked in the thread loop
        car.thread_runing=true;// indicate that the thread is running
        m_thread = std::thread(&WEB::theThread, this);
        cout<<"Thread gestartet\n";
    }
    myMutex.unlock();
}
void WEB::StopThread()
{
    extern  CURL_Response car;//get access to the struct in the main app
    if(car.thread_runing)
    {
        m_stopCarNet=true;// tell the thread loop to exit
        if (m_thread.joinable()) m_thread.join();
        cout<<"Thread gestoppt\n";
        car.thread_runing=false;// indicate that the thread ist not running
    }
}
void WEB::theThread()
{
    time_t lastPoll=0;// do the first poll directly
    extern CURL_Response car;//get access to the struct in the main app
    WEB CarNet;// create ojbect
    while(!m_stopCarNet)//m_stopCarNet is atomic and accessed StartThread and StopThread
    {
        myMutex.lock();//exclusive acces to the variables
        if ((long)lastPoll+(long)car.PollIntervall<=time (NULL))// only if the PollIntervall seconds have passed
        {
            lastPoll=time (NULL);// now
            if(!car.request_in_progress)  car=CarNet.CARNET();//do the requst if not already one is running, by be called directly. 
        }
        myMutex.unlock();
        std::this_thread::sleep_for (std::chrono::seconds(1));
    }
}
struct data//from curl debug
{
    char trace_ascii; /* 1 or 0 */
};

static
void dump(const char *text,
          FILE *stream, unsigned char *ptr, size_t size,
          char nohex)// also curl debug
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

static
int my_trace(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *userp)
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
CURL_Response WEB::CARNET()//request all data
{
    extern CURL_Response car;// get access of car in main app to get the userdata
    response=car;//store all data in the locale struct
    if(!car.request_in_progress)// is already a request running?
    {
        car.request_in_progress=true;// now one request IS running
        setenv("SSLKEYLOGFILE","/home/pi/sslkeylogfile.keylog",1);//for wireshark if needed and working (only on pi4 for me)
        CURL *curl;// standard curl things
        CURLcode res;
        curl = curl_easy_init();
        response.Car.successlevel = 0;// now we begin
        if(curl)
        {
            curl = curl_easy_init();
            struct data config;//curl debug
            config.trace_ascii = 1; /* enable ascii tracing */
            char buffer[CURL_ERROR_SIZE+1] = {};
            res=curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, buffer);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);//set to 1L if you want to see what curl is doing
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);//this function will store the response data
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);// s will hild the data
            curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "coockie.txt");
            curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "coockie.txt");
            curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER,1L);// set to 0 if you have problems with certificates
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST,2);// here the same
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);
            response =do_login(curl,response);// do the login run
            if(response.Car.successlevel<8) return response;// we made all 8 stages? go on
            do_request(curl);//request all the data
            do_logout(curl);// and logout
            curl_version_info_data *d = curl_version_info(CURLVERSION_NOW);// if you want to know what version you are running
//        cout<<"libcurl Version: "<<d->version<<"\n";
            if (response.Car.successlevel>8) //do_request hopefully stored some data
            {
                response=init_response(response);//first set all varialbes to a state, we can see thats not the true data
                unpack_emanager(eManagerResult);// unpack all the data
                unpack_fullyLoadedVehicles(FullyLoadedCarsResponse);
                unpack_vehicleDetails(VehicleDetailsResponse);
                unpack_position(lastLocationResult);
                unpack_vehicleStatusData(VehicleStatusResult);

                response.Car.LatestmessageList=getValue(LatestMessageResponse,"messageList");
                if (response.Car.LatestmessageList!="")cout <<"LatestmessageList"<<response.Car.LatestmessageList<<"\n";
                response.Car.NewmessageList=getValue(NewMessageResponse,"messageList");
                if (response.Car.NewmessageList!="")cout <<"NewmessageList" <<response.Car.LatestmessageList<<"\n";
            }
            time_t now=time (NULL);
            response.Car.TimestampPoll=(long int)now;
/////////// ENDE +++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        }
        else cout<<"CURL fehler\n";// we did not got a curl handle
        car.request_in_progress=false;
    }
    return response;
}
CURL_Response WEB::CARNET(std::string command)// mostly like the one above but no requests are done, instead a command is send
{
 extern CURL_Response car;
    response=car;
    if(!car.request_in_progress)
    {
        car.request_in_progress=true;
        int MaxVersuche;
        setenv("SSLKEYLOGFILE","/home/pi/sslkeylogfile.keylog",1);
        CURL *curl;
        CURLcode res;
        curl = curl_easy_init();
        response.Car.successlevel = 0;
        if(curl)
        {
            curl = curl_easy_init();
            struct data config;
            config.trace_ascii = 1; /* enable ascii tracing */
            char buffer[CURL_ERROR_SIZE+1] = {};
            res=curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, buffer);
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
            curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "coockie.txt");
            curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "coockie.txt");
            curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER,1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST,2);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_header);
            response =do_login(curl,response);
            if(response.Car.successlevel<8) return response;
            {// insted of the request is here a command selected
               if(command=="start clima") do_command(curl,(base_json_urlVIN+"/-/emanager/trigger-climatisation"),"{\"triggerAction\":true,\"electricClima\":true}");
               if(command=="stop clima") do_command(curl,(base_json_urlVIN+"/-/emanager/trigger-climatisation"),"{\"triggerAction\":false,\"electricClima\":true}");
                if(command=="start windowmelt") do_command(curl,(base_json_urlVIN+"/-/emanager/trigger-windowheating"),"{\"triggerAction\":true}");
                if(command=="stop windowmelt") do_command(curl,(base_json_urlVIN+"/-/emanager/trigger-windowheating"),"{\"triggerAction\":false}");
                if(command=="start charge") do_command(curl,(base_json_urlVIN+"/-/emanager/charge-battery"),"{\"triggerAction\":true,\"batteryPercent\":\"100\"}");
                if(command=="stop charge") do_command(curl,(base_json_urlVIN+"/-/emanager/charge-battery"),"{\"triggerAction\":false,\"batteryPercent\":\"99\"}");
            }
            do_logout(curl);
            curl_version_info_data *d = curl_version_info(CURLVERSION_NOW);
//        cout<<"libcurl Version: "<<d->version<<"\n";
/////////// ENDE +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        }
        else cout<<"CURL fehler\n";
        car.request_in_progress=false;
    }
    return response;
}
std::string WEB::tolowerString(std::string Input)//return the given string in lowercase. Used in unpack routines
{
    std::string Output=Input;
    std::locale loc;
    for (std::string::size_type i=0; i<Input.length(); ++i)
    {
        Output[i]=std::tolower(Input[i],loc);
    }
    return Output;
}
std::string WEB::removeNewLineCaracter (std::string Input)
{
    // Erste runde
    int start_pos=0;
    char check ;
    check = Input.at(start_pos);
    while (start_pos<Input.size()-1)
    {
        if((int)check==10||(int)check==13)
        {
            Input.erase(start_pos, 1);
        }
        else
            start_pos++;
        check = Input.at(start_pos);

    }
    return Input;
}
std::string WEB::trim(std::string Input)// cut away leading and trailing spaces
{
    char check ;
    check = Input.at(0);
    while((int)check <32 || (int)check > 127|| check==32)
    {
        Input.erase(0, 1);
        check = Input.at(0);
    }
    check = Input.at(Input.length() - 1);
    while((int)check <32 || (int)check > 127|| check==32)
    {
        Input.erase(Input.length() - 1, 1);
        check = Input.at(Input.length() - 1);
    }
    return Input;
}
std::string WEB::getValue(std::string Message, std::string keyword1, int Field)// not used anymore, comes from carnet time
{
    //äußere Klammer entfernen
    int spos;

    if(Message.find("{") < Message.find("\""))
    {
        spos = Message.find("{");

        if(spos >= 0)
        {
            Message.erase(0, spos + 1);
            spos = Message.find_last_of("}");

            if(spos >= 0)
                Message.erase(spos, Message.length() - spos);
        }
    }

    std::string key = "";
    std::string value = "";
    std::string check;
    int epos, cute = 0, cuts;
    bool gefunden = false;

    //keyword suchen
    while(key.compare(keyword1) != 0)
    {
        key = "";
        value = "";
        cuts = 0;
        gefunden = false;
        spos = Message.find("\"");

        if(spos < 0)
            break;

        cuts = spos;
        spos++;
        epos = Message.find("\"", spos);
        key = Message.substr(spos, epos - spos);

        //Value zum gelesenen Keyword lesen
        spos = epos + 2;
        cute = spos;
        value = Message.substr(spos, Message.length() - spos);

        //spos=0;
        check = value.at(0);

        while(check == " ")
        {
            cute++;
            value.erase(0, 1);
            check = value.at(0);
        }

        if(check == "{")
        {
            int counter = 1;
            spos = 1;
            epos = spos;

            while(counter != 0 && epos < value.length() - 1)
            {
                epos++;
                check = value.at(epos);

                if(check == "}")
                {
                    counter--;
                }

                if(check == "{")
                {
                    counter++;
                }
            }

            if(epos < value.length() + 1 && epos > spos)
            {
                value = value.substr(spos, epos - spos);
                cute += epos+1;
                gefunden = true;

            }
        }

        check = value.at(0);

        while(check == " ")
        {
            cute++;
            value.erase(0, 1);
            check = value.at(0);
        }

        if(check == "[")
        {
            int counter = 1;
            spos = 1;
            epos = spos;

            while(counter != 0 && epos <= value.length())
            {
                epos++;
                check = value.at(epos);

                if(check == "]")
                {
                    counter--;
                }

                if(check == "[")
                {
                    counter++;
                }
            }

            if(epos >= 1)
            {
                value = value.substr(1, epos - 1);

                if(!gefunden)
                    cute += epos;

                gefunden = true;

            }
        }

        check = value.at(0);

        while(check == " ")
        {
            cute++;
            value.erase(0, 1);
            check = value.at(0);
        }

        if(check == "\"")
        {
            spos = 1;
            epos = value.find("\"", spos);

            if(epos >= 0 && epos == value.length() - 1)
            {
                value = value.substr(1, epos - 1);

                if(!gefunden)
                    cute = epos;

                gefunden = true;

            }
        }

        if(!gefunden)
        {
            epos = value.find(",", 1);

            if(epos <= 0)
                epos = value.length();

            value = value.substr(0, epos);
            cute += epos+1;

        }

        check = value.at(0);

        if(check == "\"")
        {
            spos = 1;
            epos = value.find("\"", spos);

            if(epos >= 0 && epos == value.length() - 1)
                value = value.substr(1, epos - 1);

        }

        //gelesenen Block entfernen
        Message.erase(cuts, cute);

    }

    while(Field >= 0)
    {
        value = trim(value);
        check = value.at(0);

        if(check == "{")
        {
            int counter = 1;
            spos = 1;
            epos = spos;

            while(counter != 0 && spos < value.length() - 1)
            {
                epos++;
                check = value.at(epos);

                if(check == "}")
                {
                    counter--;
                }

                if(check == "{")
                {
                    counter++;
                }
            }
        }
        else if(check == "[")
        {
            int counter = 1;
            spos = 1;
            epos = spos;

            while(counter != 0 && spos < value.length())
            {
                epos++;
                check = value.at(epos);

                if(check == "]")
                {
                    counter--;
                }

                if(check == "[")
                {
                    counter++;
                }
            }
        }
        else if(check == "\"")
        {
            spos = 1;
            epos = value.find("\"", spos);
        }

        {
            if(Field == 0)
                value = value.substr(spos, epos - 1);

            epos = value.find(",", epos);
        }

        if(Field != 0)
            value.erase(0, epos + 1);

        Field--;
    }

    if(key.compare(keyword1) != 0)
        value = "nicht gefunden";

    return value;
}
std::string WEB::getValue(std::string Message, std::string keyword1)
{
//#define __cout__  //if something dont work, uncomment this line
#ifdef __cout__
    cout<<"Get start -------------------------------\n";
#endif // __cout__
    //äußere Klammer entfernen
    int spos;
    std::string key = "";
    std::string value = "";
    int epos, cute = 0, cuts;
    bool gefunden = false;
    spos=0;
    std::string check;
#ifdef __cout__
    cout<<"Key:\n"<<keyword1<<"\n";
    cout<<"Message:\n"<<Message<<"\n";
#endif // __cout__
    while(key.compare(keyword1) != 0)//loop find right keyword
    {
        key = ""; //reset seperated keyword
        value = "";// reset seperated value
        cuts = 0;//Cut Start nothing to cut jet
        gefunden = false; // found right keyword or end of message reached
        spos = Message.find("\"");//search beginning of a keyword in message
        if(spos < 0)//no keywords at all
            break;
        cuts = spos;//all before is not nedded
        spos++;
        epos = Message.find("\"", spos);// find end of keyword in message
        key = Message.substr(spos, epos - spos);//uncecked keyword from message
#ifdef __cout__
        cout<<"key in Message:\n"<<key<<"\n";
#endif // __cout__
        spos = epos + 2;//cut away " and next char
        cute = Message.size();//for now keep it all
#ifdef __cout__
        cout<<"rest Value after Key:\n"<<Message.substr(spos,cute-spos)<<"\n"<<endl;
#endif // __cout__
unpackBrackets:
        check = Message.at(spos);
        while(check == " "&&spos<Message.size())//remove spaces
        {
            spos++;
            check = Message.at(spos);
        }
        if(check == "[") // a new container?
        {
            spos++;
            epos=findBracket(Message,"[]",spos);
            if (epos>=0&&!gefunden)
            {
                cute=epos+1;
                gefunden=true; //dont change cute anymore
            }
            else epos=Message.size();
#ifdef __cout__
            cout<<"rest Value after {}:\n"<<Message.substr(spos,epos-spos)<<"\n"<<endl;
#endif // __cout__
            goto unpackBrackets;//container in container?
        }

        check = Message.at(spos);
        if(check == "{")// a new container?
        {
            spos++;
            epos=findBracket(Message,"{}",spos);
            if (epos>=0)
            {
                if(epos>=0&&!gefunden)
                {
                    cute=epos+1;
                    gefunden=true; //dont change cute anymore
                }
            }
            else epos=Message.size()-1;
#ifdef __cout__
            cout<<"rest Value after []:\n"<<Message.substr(spos,cute-spos)<<"\n"<<endl;
#endif // __cout__
            goto unpackBrackets;//container in container?
        }//found new [] container
        if (!gefunden) //only when no container, otherwise we will return the container
        {
            epos=Message.find(",",spos);
            if (epos>=0)
            {
                cute=epos+1;
                value=Message.substr(0,epos);
            }
            else value=Message;
            check = value.at(spos);
            if(check == "\"")//only find the position
            {
                spos++;
                epos = value.find("\"", spos);
                if(epos >0 )
                {
                    if (!gefunden)
                    {
                        cute=epos+1;
                        gefunden=true; //dont change cute anymore
                    }

                }
                else epos=value.size()-1;// may be its a number or bool
            }

        }
        value=Message.substr(spos,epos-spos);//we found the borders of a value and cut it out
        //gelesenen Block entfernen
        Message.erase(0, cute);// keep only what is behind this keyword and its values
#ifdef __cout__
        cout<<"value for that key:\n"<<value<<"\n";
        cout<<"rest Message looop:\n"<<Message<<"\n";
#endif // __cout__
    }//loop find right keyword

    if(key.compare(keyword1) != 0)//no found key match
        value = "nicht gefunden";

#ifdef __cout__
    cout<<"Result:\n"<<value<<"\n";    //get first keyword in message, get the value and hold the rest. Repeat with the rest until right keyword is found
#endif // __cout__

    return value;
}
char* WEB::To_CharArray(const std::string &Text)// not used
{
    int size = Text.size();
    char *a = new char[size + 1];
    a[Text.size()] = 0;
    memcpy(a, Text.c_str(), size);
    return a;
}
static size_t header_callback(char *received_data, size_t size,size_t nitems, void *response_header)// callback for curl, taken from example
{
    size_t realsize = size * nitems;
    //  try
    {
        ((std::string*)response_header)->append((char*)received_data, size * nitems);

    }
    return realsize;
}
size_t CurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, std::string *s)// callback for curl, taken from example
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
char * string2Char (std::string Input)// not used
{

    char *Output = new char[Input.length() + 1];
    strcpy(Output, Input.c_str());
    return (Output);
}
std::string WEB::extract_url_parameter(std::string Input,std::string Key)// called from the login run
{
    std::string temp_Input;
    Key= tolowerString(Key);
    temp_Input=tolowerString(Input);
    int start_pos=0;
    int end_pos=0;
    start_pos= temp_Input.find(Key);
    if (start_pos <0)
        Input ="";
    else
    {
        start_pos+=(int)Key.size();
        start_pos++;
        end_pos=temp_Input.find("&");
        if (end_pos >0)
            Input=Input.substr(start_pos,end_pos-start_pos);
        else
            Input=Input.substr(start_pos,Input.size()-start_pos);
    }
    return Input;
}
std::string WEB::get_HeaderElement(std::string Input,std::string Key)// called from the login run
{
    std::string temp_Input;
    Key= tolowerString(Key);
    temp_Input=tolowerString(Input);
    int start_pos=0;
    int end_pos=0;
    start_pos= temp_Input.find(Key)+(int)Key.size()+1;// Key finden und Zähler auf das Ende vom gefundenen Key setzen
    if (start_pos <0)
        Input ="";//Key nicht gefunden
    else
    {
        end_pos=temp_Input.find(13,start_pos);
        if (end_pos<0) end_pos=temp_Input.size();
        Input=Input.substr(start_pos,end_pos-start_pos);
    }
    Input=trim(Input);
    return Input;
}
std::string WEB::get_HTTPValue(std::string Input,std::string Key)// called from the login run
{
    std::string temp_Input;
    Key= tolowerString(Key);
    temp_Input=tolowerString(Input);
    int start_pos=0;
    int end_pos=0;
    start_pos=temp_Input.find(Key);
    if (start_pos>=0)
    {
        start_pos+=(int)Key.size();
        start_pos++;
        Key="value";
        start_pos=temp_Input.find(Key,start_pos);
        if (start_pos<0) return "";
        start_pos+=(int)Key.size();
        start_pos=temp_Input.find("=\"",start_pos);
        if (start_pos<0) return "";
        start_pos+=2;
        end_pos=temp_Input.find("\"",start_pos);
        if (end_pos<0) return "";
        Input=Input.substr(start_pos,end_pos-start_pos);
        Input=trim(Input);
    }
    else
        Input="";
    return Input;
}
std::string WEB::extract_csrf(std::string Input,std::string Key)// called from the login run
{
    std::string temp_Key,temp_Input;
    temp_Key= tolowerString(Key);
    temp_Input=tolowerString(Input);
    int start_pos=0;
    int end_pos=0;
    start_pos=temp_Input.find(temp_Key);
    if (start_pos>=0)
    {
        start_pos+=(int)temp_Key.size();
        start_pos++;
        start_pos=temp_Input.find("=\"",start_pos);
        if (start_pos<0) return "";
        start_pos+=2;
        end_pos=temp_Input.find("\"",start_pos);
        if (end_pos<0) return "";
        Input=Input.substr(start_pos,end_pos-start_pos);
        Input=trim(Input);
    }
    else
        Input="";
    return Input;
}
std::string WEB::get_logoutURL(std::string Input,std::string Key)// called from the logout run
{
    std::string temp_Key,temp_Input;
    temp_Key= tolowerString(Key);
    temp_Input=tolowerString(Input);
    int start_pos=0;
    int end_pos=0;
    start_pos=temp_Input.find(temp_Key);
    if (start_pos>=0)
    {
        start_pos+=(int)temp_Key.size();
        start_pos++;
        start_pos=temp_Input.find(":\"",start_pos);
        if (start_pos<0) return "";
        start_pos+=2;
        end_pos=temp_Input.find('"',start_pos);
        if (end_pos<0) return "";
        Input=Input.substr(start_pos,end_pos-start_pos);
        Input=trim(Input);

    }
    else
        Input="";
    return Input;
}
int WEB::findBracket(std::string Input, std::string bracket)//not used! find the position of the corosponding bracket
{
    std::string check;
    int counter = 1; //found one {
    int spos = 1;// make a step inside
    int epos = spos;//end not befor start
    while(counter != 0 && epos < Input.length() )//find corosponding }
    {
        check = Input.at(epos);
        if(check == bracket)
        {
            counter--;
        }
        if(check == bracket)
        {
            counter++;
        }
        if (counter==0)
        {
            break;// counter must be zero when found right }
        }
        epos++;
    }

    if (counter!=0) epos=0;
    return epos;
}
int WEB::findBracket(std::string Input, std::string bracket, int startpos)// same as above but working and begin at given startposition
{
    std::string check;
    std::string left,right;
    if (bracket.size()!=2)return -1;// string bracket must be the pair of brackets you ar looking for like "{}" or "[]"
    left=bracket.at(0);
    right=bracket.at(1);
    int counter = 1; //already found one {
    int epos = startpos;//end not befor start
    while(counter != 0 && epos < Input.length() )//find corosponding }
    {
        check = Input.at(epos);
        if(check == right)
        {
            counter--;
        }
        if(check == left)
        {
            counter++;
        }
        if (counter==0)
        {
            break;// counter must be zero when found right bracket
        }
        epos++;
    }

    if (counter!=0) epos=-1;// it seems the Input was not complete
    return epos;
}
void WEB::set_requstheader (CURL *curl,curl_slist *chunk)
{
    chunk = curl_slist_append(chunk, "Accept-Encoding: gzip,deflate,br");
    chunk = curl_slist_append(chunk, "Accept-Language: en-US,nl;q=0.7,en;q=0.3");
    chunk = curl_slist_append(chunk, "Accept: text/html,application/xhtml+xml,application/xml,application/json;q=0.9,*/*;q=0.8");
    chunk = curl_slist_append(chunk, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:68.0) Gecko/20100101 Firefox/68.0");
    chunk = curl_slist_append(chunk, "Connection: keep-alive");
    chunk = curl_slist_append(chunk, "Pragma: no-cache");
    chunk = curl_slist_append(chunk, "Cache-Control: no-cache");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

}
void WEB::set_requstheader (CURL *curl,curl_slist *chunk, std::string CSRF_token, std::string Referer)
{
    chunk = curl_slist_append(chunk, "Accept-Encoding: gzip,deflate,br");
    chunk = curl_slist_append(chunk, "Accept-Language: en-US,nl;q=0.7,en;q=0.3");
    chunk = curl_slist_append(chunk, "Accept: text/html,application/xhtml+xml,application/xml,application/json;q=0.9,*/*;q=0.8");
    chunk = curl_slist_append(chunk, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:68.0) Gecko/20100101 Firefox/68.0");
    chunk = curl_slist_append(chunk, "Connection: keep-alive");
    chunk = curl_slist_append(chunk, "Pragma: no-cache");
    chunk = curl_slist_append(chunk, "Cache-Control: no-cache");
    std::string  header;
    if (Referer!="")
    {
        header=("Referer: "+Referer);
        {
            char *Output = new char[header.length() + 1];
            strcpy(Output, header.c_str());
            chunk = curl_slist_append(chunk,Output);
        }
    }
    if (CSRF_token!="")
    {
        header=("X-CSRF-Token: "+CSRF_token);
        {
            char *Output = new char[header.length()];
            strcpy(Output, header.c_str());
            chunk = curl_slist_append(chunk,Output);
        }
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

}
void WEB::set_auth_requstheader (CURL *curl,curl_slist *chunk)
{
    chunk = curl_slist_append(chunk, "Accept-Encoding: gzip, deflate, br");
    chunk = curl_slist_append(chunk, "Accept-Language: en-US,nl;q=0.7,en;q=0.3");
    chunk = curl_slist_append(chunk, "Accept: text/html,application/xhtml+xml,application/xml,application/json;q=0.9,*/*;q=0.8");
    chunk = curl_slist_append(chunk, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:68.0) Gecko/20100101 Firefox/68.0");
    chunk = curl_slist_append(chunk, "Connection: keep-alive");
    chunk = curl_slist_append(chunk, "Pragma: no-cache");
    chunk = curl_slist_append(chunk, "Cache-Control: no-cache");
    chunk = curl_slist_append(chunk, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

}
void WEB::set_auth_requstheader (CURL *curl,curl_slist *chunk, std::string CSRF_token, std::string Referer)
{
    chunk = curl_slist_append(chunk, "Accept-Encoding: gzip, deflate, br");
    chunk = curl_slist_append(chunk, "Accept-Language: en-US,nl;q=0.7,en;q=0.3");
    chunk = curl_slist_append(chunk, "Accept: text/html,application/xhtml+xml,application/xml,application/json;q=0.9,*/*;q=0.8");
    chunk = curl_slist_append(chunk, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:68.0) Gecko/20100101 Firefox/68.0");
    chunk = curl_slist_append(chunk, "Connection: keep-alive");
    chunk = curl_slist_append(chunk, "Pragma: no-cache");
    chunk = curl_slist_append(chunk, "Cache-Control: no-cache");
    chunk = curl_slist_append(chunk, "Content-Type: application/x-www-form-urlencoded");
    std::string  header;
    if (Referer!="")
    {
        header=("Referer: "+Referer);
        {
            char *Output = new char[header.length() + 1];
            strcpy(Output, header.c_str());
            chunk = curl_slist_append(chunk,Output);
        }
    }
    if (CSRF_token!="")
    {
        header=("X-CSRF-Token: "+CSRF_token);
        {
            char *Output = new char[header.length() + 1];
            strcpy(Output, header.c_str());
            chunk = curl_slist_append(chunk,Output);
        }
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

}
void WEB::set_actionheader(CURL *curl,curl_slist *chunk, std::string CSRF_token, std::string Referer)
{
    chunk = curl_slist_append(chunk, "Accept-Encoding: gzip, deflate, br");
    chunk = curl_slist_append(chunk, "Accept-Language: en-US,nl;q=0.7,en;q=0.3");
    chunk = curl_slist_append(chunk, "Accept: text/html,application/xhtml+xml,application/xml,application/json;q=0.9,*/*;q=0.8");
    chunk = curl_slist_append(chunk, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:68.0) Gecko/20100101 Firefox/68.0");
    chunk = curl_slist_append(chunk, "Connection: keep-alive");
    chunk = curl_slist_append(chunk, "Pragma: no-cache");
    chunk = curl_slist_append(chunk, "Cache-Control: no-cache");
    chunk = curl_slist_append(chunk, "Content-Type: application/json");

    std::string  header;
    if (Referer!="")
    {
        header=("Referer: "+Referer);
        {
            char *Output = new char[header.length() + 1];
            strcpy(Output, header.c_str());
            chunk = curl_slist_append(chunk,Output);
        }
    }
    if (CSRF_token!="")
    {
        header=("X-CSRF-Token: "+CSRF_token);
        {
            char *Output = new char[header.length() + 1];
            strcpy(Output, header.c_str());
            chunk = curl_slist_append(chunk,Output);
        }
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

}
CURLcode WEB::perform_request(CURL *curl,std::string  URL)
{
    {
        char *url = new char[URL.length() + 1];
        strcpy(url, URL.c_str());
        curl_easy_setopt(curl, CURLOPT_URL, url);
    }
    return curl_easy_perform(curl);

}
CURLcode WEB::perform_request(CURL *curl,std::string  URL,std::string sPostfields)
{
    if(sPostfields != "")
    {
        char *fields = new char[sPostfields.length() + 1];
        strcpy(fields, sPostfields.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,sPostfields.size());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);
        curl_easy_setopt(curl, CURLOPT_POST,1);
    }

    {
        char *url = new char[URL.length() + 1];
        strcpy(url, URL.c_str());
        curl_easy_setopt(curl, CURLOPT_URL, url);
    }
    CURLcode res=curl_easy_perform(curl);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, NULL);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,0);
//    curl_easy_setopt(curl, CURLOPT_POST, 0);
    return res;
}
CURL_Response WEB::do_login(CURL *curl,CURL_Response response)// got thru all 8 steps to login
{
    portal_base_url = "https://www.portal.volkswagen-we.com";
    auth_base_url = "https://identity.vwgroup.io";
    base_url=portal_base_url;
    landing_page_url = (base_url+"/portal/en_GB/web/guest/home");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

//        std::cout<<"Step 1++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<endl;
    s="";
    response_header="";
    curl_slist *chunk = NULL;
    response.res=perform_request(curl,landing_page_url);
    if(response.res != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        cout<<"Get CSRF war nix";
        return response;
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_responsecode);
    if(http_responsecode!=200)
    {
        cout<<("Failed getting to portal landing page\n");
        return response;
    }
    s=removeNewLineCaracter(s);
    {
        char *location;
        curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &location);
        std::string temp(location);
        ref2_url=temp;
    }
    if(ref2_url=="") return response;
    if (ref2_url!=site)// we are redirected, may be still logged in
    {
        csrf=extract_csrf(s,"_csrf");
        int Test=ref2_url.find("/delegate/dashboard");
        if (Test>0)// yes we are still logged in
        {
            cout<<"\nAbkürzung zu Step8\n";
            base_json_url=ref2_url;
            goto Step8;
        }
    }
    if (csrf=="")
    {
        csrf = extract_csrf(s, "_csrf");
    }
    if (csrf.size()==8)
        response.Car.successlevel = 1;
    if (response.Car.successlevel == 0)
    {
        cout<<"\nCSRF länge falsch\n";
        return response;
    }

//step 2
    response_header="";
    s="";
    curl_slist_free_all(chunk);
    chunk = NULL;
    set_requstheader(curl,chunk,csrf,landing_page_url);
//        std::cout<<"Step 2++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<endl;
    response.res=perform_request(curl,(base_url+"/portal/en_GB/web/guest/home/-/csrftokenhandling/get-login-url"));
    if(response.res != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        cout<<"get Login Page war nix\n";
        return response;
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_responsecode);
    if(http_responsecode!=200)
    {
        cout<<("Failed to get login url. Step2");
        return response;
    }
    response.errCode=getValue(s,"loginURL");
    if(response.errCode=="")
    {
        cout<<"Cant get loginURL\n";
        return response;
    }
    login_url =getValue(response.errCode,"path");
    if(login_url=="")
    {
        cout<<"Cant get login_url 2\n";
        return response;
    }
    client_id=extract_url_parameter(login_url,"client_id");
    if (client_id.size()!=54)
    {
        cout<<("Länge ClientID falsch");
        return response;
    }
    response.Car.successlevel=2;
// Step 3
    curl_slist_free_all(chunk);
    chunk = NULL;
    set_requstheader(curl,chunk,csrf,landing_page_url);
    {
        //rplace spaces inr UNL with %20
        int lpos=login_url.find(" ");
        while (lpos >=0)
        {
            login_url.erase(lpos,1);
            login_url.insert(lpos,"%20");
            lpos=login_url.find(" ");
        }
    }
//            std::cout<<"Step 3++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<endl;
    response_header="";
    s="";
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);
    response.res=perform_request(curl,login_url);
    if(response.res != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        return response;
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_responsecode);
    if(http_responsecode!=302)
    {
        cout<<("Failed to get authorization page. Step3");
        return response;
    }
    login_form_url=get_HeaderElement(response_header,"Location");
    if(login_form_url=="")
    {
        cout<<"Cant get login_form_url\n";
        return response;
    }
    login_relay_state_token=get_HeaderElement(login_form_url,"relayState");
    if(login_relay_state_token=="")
    {
        cout<<"Cant get login_relay_state_token\n";
        return response;
    }
    response.Car.successlevel=3;
// Step 4
//            std::cout<<"Step 4++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<endl;
    s="";
    response_header="";
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    response.res=perform_request(curl,login_form_url);
    if(response.res != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        cout<<"Step4 war nix";
        return response;
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_responsecode);
    if(http_responsecode!=200)
    {
        cout<<("Failed to get sign-in page.Step4");
        return response;
    }
    s=removeNewLineCaracter(s);
    hmac_token1=get_HTTPValue(s,"hmac");
    if(hmac_token1=="") return response;
    login_csrf=get_HTTPValue(s,"_csrf");
    if(login_csrf=="") return response;
    response.Car.successlevel=4;
//Step5
//            std::cout<<"Step 5++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<endl;
    response_header="";
    s="";
    curl_slist_free_all(chunk);
    chunk = NULL;
    set_auth_requstheader(curl,chunk,"",login_form_url);
    sPostfields = ("email="+response.User+"&relayState="+login_relay_state_token+ "&hmac="+hmac_token1+"&_csrf="+login_csrf);
    client_id=trim(client_id);
    login_action_url=(auth_base_url+"/signin-service/v1/"+client_id +"/login/identifier");
    response.res=perform_request(curl,login_action_url,sPostfields);
    if(response.res != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        cout<<"Step5 war nix";
        return response;
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_responsecode);
    if(http_responsecode!=200)
    {
        cout<<("Failed to get login/identiefer page.Step5");
        return response;
    }
    s=removeNewLineCaracter(s);
    hmac_token2=get_HTTPValue(s,"hmac");
    if(hmac_token2=="") return response;
    response.Car.successlevel=5;
//Step6
//            std::cout<<"Step 6++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<endl;
    s="";
    response_header="";
    curl_slist_free_all(chunk);
    chunk = NULL;
    set_auth_requstheader(curl,chunk,"",login_action_url);
    sPostfields = ("email="+response.User+"&password="+response.Passw+"&relayState="+login_relay_state_token+ "&hmac="+hmac_token2+"&_csrf="+login_csrf+"&login=true");
    login_action2_url=(auth_base_url+"/signin-service/v1/"+client_id +"/login/authenticate");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,1L);
    response.res=perform_request(curl,login_action2_url,sPostfields);
    if(response.res != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        cout<<"Step6 war nix";
        return response;
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_responsecode);
    if(http_responsecode!=200)
    {
        cout<<("Failed to process login sequence.Step6");
        return response;
    }
    {
        char *location;
        curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &location);
        std::string temp(location);
        ref2_url=temp;
    }
    if(ref2_url=="") return response;

ContinueSession:

    portlet_code=extract_url_parameter(ref2_url,"code");
    if(portlet_code=="") return response;
    state=extract_url_parameter(ref2_url,"state");
    if(state=="") return response;
    response.Car.successlevel=6;
//Step7
//            std::cout<<"Step 7++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<endl;
    s="";
    response_header="";
    curl_slist_free_all(chunk);
    chunk=NULL;
    set_requstheader(curl,chunk,"",ref2_url);
    sPostfields = ("_33_WAR_cored5portlet_code="+portlet_code);
    get_login_url=(base_url+"/portal/web/guest/complete-login?p_auth="+state+"&p_p_id=33_WAR_cored5portlet&p_p_lifecycle=1&p_p_state=normal&p_p_mode=view&p_p_col_id=column-1&p_p_col_count=1&_33_WAR_cored5portlet_javax.portlet.action=getLoginStatus");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER,0);
    response.res = perform_request(curl,get_login_url,sPostfields);
    if(response.res != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        response.errCode="Step7 war nix";
        return response;
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_responsecode);
    if(http_responsecode!=302)
    {
        cout<<("Failed to get login url.Step7");
        return response;
    }
    base_json_url=get_HeaderElement(response_header,"location");
    if(base_json_url=="") return response;
    response.Car.successlevel=7;
//Step8
Step8:
//            std::cout<<"Step 8++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<endl;
    s="";
    response_header="";
    curl_slist_free_all(chunk);
    chunk=NULL;
    set_requstheader(curl,chunk,"",ref2_url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,1L);
    response.res=perform_request(curl,base_json_url);
    if(response.res != CURLE_OK)
    {
        cout<<"Step8 war nix";
        return response;
    }
    s=removeNewLineCaracter(s);
    csrf = extract_csrf(s, "_csrf");
    if(login_csrf=="") return response;
    base_json_urlVIN=base_json_url;
    response.Car.successlevel=8;
    if(response.VIN !="" )// if a VIN is given in parameter
    {
        int lpos=base_json_url.find_last_of("/");
        if (lpos>0)
        {
            lpos++;
            base_json_urlVIN=base_json_url.substr(0,lpos);
            base_json_urlVIN.append(response.VIN);
            base_json_urlVIN.append("/");
        }
    }
    curl_slist_free_all(chunk);
    cout<<"login ok\n";
//        return response;

    return response;
}
void WEB::do_request(CURL *curl)
{
// --------------------------------Abfrage ----------
StartAbfrage:
    s="";
    response_header="";
    curl_slist *chunk = NULL;
    set_requstheader(curl,chunk,csrf,base_json_url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,0);
    response.res=perform_request(curl,(base_json_urlVIN+"/-/cf/get-location"));
    if(response.res != CURLE_OK)
    {
        cout<<"Req location war nix\n";
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_responsecode);
    if(http_responsecode!=200)
    {
        cout<<"Failed to get location \n";
    }
    else
    {
        response.Car.successlevel+=32;
        lastLocationResult=s;
    }
    s="";
    response_header="";
    response.res=perform_request(curl,(base_json_urlVIN+"/-/vsr/get-vsr"));
    if(response.res != CURLE_OK)
    {
        cout<<"Req vsr war nix\n";
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_responsecode);
    if(http_responsecode!=200)
    {
        cout<<"Failed to get vsr \n";
    }
    else
    {
        response.Car.successlevel+=64;
        VehicleStatusResult=s;
    }
    s="";
    response_header="";
    cout<<"Use url with VIN: "<<(base_json_urlVIN+"/-/mainnavigation/get-fully-loaded-cars")<<"\n";
    response.res=perform_request(curl,(base_json_urlVIN+"/-/mainnavigation/get-fully-loaded-cars"));
    if(response.res != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        response.errCode="fully-loaded-cars war nix\n";
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_responsecode);
    if(http_responsecode!=200)
    {
        cout<<("Failed to get fully-loaded-cars\n");
    }
    else
    {
        response.Car.successlevel+=64;
        s=removeNewLineCaracter(s);
        FullyLoadedCarsResponse=s;
    }
    s="";
    response_header="";
    response.res=perform_request(curl,(base_json_urlVIN+"/-/emanager/get-emanager"));
    if(response.res != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        cout<<"Req Emanager war nix\n";
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_responsecode);
    if(http_responsecode!=200)
    {
        cout<<("Failed to get emanager\n");
    }
    else
    {
        response.Car.successlevel+=16;
        s=removeNewLineCaracter(s);
        eManagerResult=s;
    }
    s="";
    response_header="";
    response.res=perform_request(curl,(base_json_urlVIN+"/-/msgc/get-latest-messages"));
    if(response.res != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        cout<<"latest-messages war nix\n";
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_responsecode);
    if(http_responsecode!=200)
    {
        cout<<("Failed to get latest-messages\n");
    }
    else
    {
        response.Car.successlevel+=64;
        s=removeNewLineCaracter(s);
        LatestMessageResponse=s;
    }
    s="";
    response_header="";
    response.res=perform_request(curl,(base_json_urlVIN+"/-/vehicle-info/get-vehicle-details"));
    if(response.res != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        cout<<"vehicle-details war nix\n";
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_responsecode);
    if(http_responsecode!=200)
    {
        cout<<("Failed to get vehicle-details\n");
    }
    else
    {
        response.Car.successlevel+=64;
        s=removeNewLineCaracter(s);
        VehicleDetailsResponse=s;
    }
    s="";
    response_header="";
    response.res=perform_request(curl,(base_json_urlVIN+"/-/msgc/get-new-messages"));
    if(response.res != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        response.errCode="get-new-messages war nix\n";
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_responsecode);
    if(http_responsecode!=200)
    {
        cout<<("Failed to get new messages\n");
    }
    else
    {
        response.Car.successlevel+=64;
        s=removeNewLineCaracter(s);
        NewMessageResponse=s;
    }
// the following request are possible but not really interisting
//            site =(base_json_urlVIN+"/-/rah/get-status");
//            site =(base_json_urlVIN+"/-/vhr/get-latest-report");
//            site =(base_json_urlVIN+"/-/rsa/get-alerts");
//            site =(base_json_urlVIN+"/-/geofence/get-fences");
//            site =(base_json_urlVIN+"/-/mainnavigation/check-unanswered-invitations");
//            site =(base_json_urlVIN+"/-/service-container/get-apple-music-status");
//            site =(base_json_urlVIN+"/-/mainnavigation/get-config");
//            site =(base_json_urlVIN+"/-/mainnavigation/load-car-details"+car.VIN);
//            site =(base_json_urlVIN+"/-/mainnavigation/get-psp-status");
//           cout<<"eManager: "<<eManagerResult<<"\n";
//           cout<<"last location: "<<lastLocationResult<<"\n";
//           cout<<"Vehicle Status: "<<VehicleStatusResult<<"\n";
    curl_slist_free_all(chunk);
}
void WEB::do_command(CURL *curl,std::string c_url,std::string command)
{
    s="";
    response_header="";
    curl_slist *chunk = NULL;
    set_actionheader(curl,chunk,csrf,base_json_url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,0);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");// this was really the trick! It don't work without
    client_id=trim(client_id);
    response.res=perform_request(curl,c_url,command);
    if(response.res != CURLE_OK)
    {
        cout<<"send "<<command<<" war nix\n";
    }
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_responsecode);
    if(http_responsecode!=200)
    {
        cout<<"Failed to send "<<command<<"\n";
        cout<<"response header: "<<response_header<<"\n";
    }
    response.errCode=getValue(s,"errorCode");
    cout<<"URL: "<<c_url<<"\n";
    cout<<"command: "<<command<<"\n";
    cout<<"Antwort: "<<s<<"\n";
      curl_slist_free_all(chunk);
}
void WEB::do_logout(CURL *curl)
{
    //logout
Logout:
//            std::cout<<"logout++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"<<endl;
    s="";
    response_header="";
    curl_slist *chunk = NULL;
    set_requstheader(curl,chunk,csrf,"");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,1L);
    response.res=perform_request(curl,(base_json_url+"/-/logout/revoke"));
    if(response.res != CURLE_OK)
    {
        curl_easy_cleanup(curl);
        cout<<"Logout war nix";
    }
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,1L);
    response.res=perform_request(curl,get_logoutURL(s,"logoutURL"));
    curl_slist_free_all(chunk);
    curl_easy_cleanup(curl);


}
CURL_Response WEB::init_response(CURL_Response response)
{
    response.Car.VIN="";
    response.Car.CarName="";
    response.Car.chargingState;
    response.Car.remainingChargingTime="";
    response.Car.chargingReason="";
    response.Car.pluginState="";
    response.Car.lockState="";
    response.Car.extPowerSupplyState="";
    response.Car.climatisationState="";
    response.Car.climatisationRemaningTime=-99;
    response.Car.windowHeatingStateFront="";
    response.Car.windowHeatingStateRear="";
    response.Car.climatisationReason="";
    response.Car.windowHeatingAvailable="";
    response.Car.climatisationWithoutHVPower=false;
    response.Car.climaterActionState="";
    response.Car.lat="";
    response.Car.lng="";
    response.Car.parkingLight="";
    response.Car.hood="";
    response.Car.LatestmessageList="";
    response.Car.NewmessageList="";
    response.Car.lastConnectionTimeStamp=-99;
    response.Car.targetTemperature=-99;
    response.Car.stateOfCharge=-99;
    response.Car.electricRange=-99;
    response.Car.chargerMaxCurrent=-99;
    response.Car.maxAmpere=-99;
    response.Car.DoorLF=-99;
    response.Car.DoorRF=-99;
    response.Car.DoorLB=-99;
    response.Car.DoorRB=-99;
    response.Car.DoorTrunk=-99;
    response.Car.LockDoorLF=-99;
    response.Car.LockDoorRF=-99;
    response.Car.LockDoorLB=-99;
    response.Car.LockDoorRB=-99;
    response.Car.LockDoorTrunk=-99;
    response.Car.WindowLF=-99;
    response.Car.WindowRF=-99;
    response.Car.WindowLB=-99;
    response.Car.WindowRB=-99;
    response.Car.WindowSunroof=-99;
    response.Car.distanceCovered=-99;
    response.Car.serviceInDays=-99;
    response.Car.serviceInKm=-99;
    return response;
}
void WEB::unpack_emanager (std::string Message)// parse json
{
    std::string Gruppe,tempstring,subGruppe1,subGruppe2,subGruppe3;
    Gruppe=getValue(Message,"EManager");
    if (Gruppe!="nicht gefunden")
    {
        subGruppe1=getValue(Gruppe,"rbc");
        if (subGruppe1!="nicht gefunden")
        {
            subGruppe2=getValue(subGruppe1,"status");
            if (subGruppe2!="nicht gefunden")
            {
                tempstring=getValue(subGruppe2,"batteryPercentage");
                response.Car.stateOfCharge=::atoi(tempstring.c_str());
                tempstring=getValue(subGruppe2,"electricRange");
                response.Car.electricRange=::atoi(tempstring.c_str());
                response.Car.chargingState=getValue(subGruppe2,"chargingState");
                response.Car.remainingChargingTime=getValue(subGruppe2,"chargingRemaningHour");
                response.Car.remainingChargingTime.append(":");
                response.Car.remainingChargingTime.append(getValue(subGruppe2,"chargingRemaningMinute"));
                response.Car.chargingReason=getValue(subGruppe2,"chargingReason");
                response.Car.pluginState=getValue(subGruppe2,"pluginState");
                response.Car.lockState=getValue(subGruppe2,"lockState");
                response.Car.extPowerSupplyState=getValue(subGruppe2,"extPowerSupplyState");
            }
            subGruppe2=getValue(subGruppe1,"settings");
            if (subGruppe2!="nicht gefunden")
            {
                tempstring=getValue(subGruppe2,"chargerMaxCurrent");
                response.Car.chargerMaxCurrent=::atoi(tempstring.c_str());
                tempstring=getValue(subGruppe2,"maxAmpere");
                response.Car.maxAmpere=::atoi(tempstring.c_str());
            }
        }
        subGruppe1=getValue(Gruppe,"rpc");
        if (subGruppe1!="nicht gefunden")
        {
            subGruppe2=getValue(subGruppe1,"status");
            if (subGruppe2!="nicht gefunden")
            {
                tempstring=getValue(subGruppe2,"climatisationRemaningTime");
                response.Car.climatisationRemaningTime=::atoi(tempstring.c_str());
                response.Car.windowHeatingStateFront=getValue(subGruppe2,"windowHeatingStateFront");
                response.Car.windowHeatingStateRear=getValue(subGruppe2,"windowHeatingStateRear");
                response.Car.climatisationReason=getValue(subGruppe2,"climatisationReason");
                response.Car.climatisationState=getValue(subGruppe2,"climatisationState");
                tempstring=getValue(subGruppe2,"windowHeatingAvailable");
                if (tempstring=="true")response.Car.windowHeatingAvailable=true;
                else response.Car.windowHeatingAvailable=false;
            }
            subGruppe2=getValue(subGruppe1,"settings");
            if (subGruppe2!="nicht gefunden")
            {
                response.Car.climaterActionState=getValue(subGruppe1,"climaterActionState");
                tempstring=getValue(subGruppe2,"targetTemperature");
                response.Car.targetTemperature=::atof(tempstring.c_str());
                tempstring=getValue(subGruppe2,"climatisationWithoutHVPower");
                if (tempstring=="true")response.Car.climatisationWithoutHVPower=true;
            }
        }
    }
}

void WEB::unpack_fullyLoadedVehicles(std::string message)
{
    std::string Gruppe,tempstring,subGruppe1,subGruppe2,subGruppe3;
    Gruppe=getValue(message,"fullyLoadedVehiclesResponse");
    if (Gruppe!="nicht gefunden")
    {
        subGruppe1=getValue(Gruppe,"vehiclesNotFullyLoaded");
        if (subGruppe1!="nicht gefunden")
        {
            response.Car.VIN=getValue(subGruppe1,"vin");
            response.Car.CarName=getValue(subGruppe1,"name");
        }
    }

}
void WEB::unpack_vehicleDetails(std::string message)
{
    std::string Gruppe,tempstring,subGruppe1,subGruppe2,subGruppe3;
    Gruppe=getValue(message,"vehicleDetails");
    if (Gruppe!="nicht gefunden")
    {
        tempstring=getValue(Gruppe,"lastConnectionTimeStamp");
        if(tempstring!="nicht gefunden")
        {
            for (int i=0; i<tempstring.size(); i++)
            {
                if (tempstring.compare(i,1,"\"")==0) tempstring.erase(i,1);
            }
            for (int i=0; i<tempstring.size(); i++)
            {
                if (tempstring.compare(i,1,",")==0) tempstring.replace(i,1," ");
            }

            struct tm tm={ 0 };
            if(strptime(tempstring.c_str(), "%d.%m.%Y %H:%M", &tm))
                response.Car.lastConnectionTimeStamp =(long int) mktime(&tm);
            else cout<<"Time convert error\n";

        }
        else cout<<"lastConnectionTimeStamp nicht gefunden\n";
        tempstring=getValue(Gruppe,"distanceCovered");
        for (int i=0; i<tempstring.size(); i++)
        {
            if (tempstring.compare(i,1,".")==0) tempstring.erase(i,1);
        }
        response.Car.distanceCovered=::atoi(tempstring.c_str());
        tempstring=getValue(Gruppe,"serviceInspectionData");
        int endpos=tempstring.find(" ");
        response.Car.serviceInDays=::atoi(tempstring.substr(0,endpos).c_str());
        endpos=tempstring.find("/")+1;
        tempstring.erase(0,endpos);
        for (int i=0; i<tempstring.size(); i++)
        {
            if (tempstring.compare(i,1,".")==0) tempstring.erase(i,1);
        }
        tempstring=trim(tempstring);
        endpos=tempstring.find(" ");
        response.Car.serviceInKm=::atoi(tempstring.substr(0,endpos).c_str());


    }

}
void WEB::unpack_position(std::string message)
{
    std::string Gruppe,tempstring,subGruppe1,subGruppe2,subGruppe3;
    Gruppe=getValue(message,"position");
    if (Gruppe!="nicht gefunden")
    {
        response.Car.lat=getValue(Gruppe,"lat");
        response.Car.lng=getValue(Gruppe,"lng");
    }

}
void WEB::unpack_vehicleStatusData(std::string message)
{
    std::string Gruppe,tempstring,subGruppe1,subGruppe2,subGruppe3;
    Gruppe=getValue(message,"vehicleStatusData");
    if (Gruppe!="nicht gefunden")
    {
        subGruppe1=getValue(Gruppe,"carRenderData");
        if (subGruppe1!="nicht gefunden")
        {
            response.Car.parkingLight=getValue(subGruppe1,"parkingLights");
            response.Car.hood=getValue(subGruppe1,"hood");
            tempstring=getValue(subGruppe2,"sunroof");
            response.Car.WindowSunroof=::atoi(tempstring.c_str());

            subGruppe2=getValue(subGruppe1,"doors");
            if (subGruppe2!="nicht gefunden")
            {
                tempstring=getValue(subGruppe2,"left_front");
                response.Car.DoorLF=::atoi(tempstring.c_str());
                tempstring=getValue(subGruppe2,"right_front");
                response.Car.DoorRF=::atoi(tempstring.c_str());
                tempstring=getValue(subGruppe2,"left_back");
                response.Car.DoorLB=::atoi(tempstring.c_str());
                tempstring=getValue(subGruppe2,"right_back");
                response.Car.DoorRB=::atoi(tempstring.c_str());
                tempstring=getValue(subGruppe2,"trunk");
                response.Car.DoorTrunk=::atoi(tempstring.c_str());
            }
            subGruppe2=getValue(subGruppe1,"windows");
            if (subGruppe2!="nicht gefunden")
            {
                tempstring=getValue(subGruppe2,"left_front");
                response.Car.WindowLF=::atoi(tempstring.c_str());
                tempstring=getValue(subGruppe2,"right_front");
                response.Car.WindowRF=::atoi(tempstring.c_str());
                tempstring=getValue(subGruppe2,"left_back");
                response.Car.WindowLB=::atoi(tempstring.c_str());
                tempstring=getValue(subGruppe2,"right_back");
                response.Car.WindowRB=::atoi(tempstring.c_str());
            }
            subGruppe2=getValue(Gruppe,"lockData");
            if (subGruppe2!="nicht gefunden")
            {
                tempstring=getValue(subGruppe2,"left_front");
                response.Car.LockDoorLF=::atoi(tempstring.c_str());
                tempstring=getValue(subGruppe2,"right_front");
                response.Car.LockDoorRF=::atoi(tempstring.c_str());
                tempstring=getValue(subGruppe2,"left_back");
                response.Car.LockDoorLB=::atoi(tempstring.c_str());
                tempstring=getValue(subGruppe2,"right_back");
                response.Car.LockDoorRB=::atoi(tempstring.c_str());
                tempstring=getValue(subGruppe2,"trunk");
                response.Car.LockDoorTrunk=::atoi(tempstring.c_str());
            }
        }
    }

}
// thats it
