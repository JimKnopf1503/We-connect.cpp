#ifndef CJSON_H
#define CJSON_H
#include <string>

// simple JSON parser, maybe not perfekt but suitable for my needs. Author: Burkhard Venus


struct sElemtentuValue
{
    std::string Name;
    std::string Value;
    std::string Rest;
};

class cJSON
{
public:
    cJSON();
    virtual ~cJSON();
    sElemtentuValue splitElement(std::string Message);
    std::string pickBlock(std::string Message);
    std::string easyBlock(std::string Message);
    std::string cleanupRest(std::string Message);
    std::string getElement(std::string Message,std::string Name);
    std::string getElement(std::string Message,std::string Name,int Field);
    bool checkValidCharacter(std::string Message, int iPos);
    int getValueNumberOfFields(std::string Message, std::string keyword1);
    std::string jsonString="";
    std::string createNewjsonString(std::string name, std::string Value);
    std::string createNewjsonElement(std::string name, std::string Value);
    std::string appendToJsonString(std::string oldString,std::string toValue,std::string name, std::string Value);
protected:

private:
};

#endif // CJSON_H
