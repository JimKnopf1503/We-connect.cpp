#include "myFunctions.h"

std::string HexToString(std::string hex)
{
    int len = hex.length();
    std::string newString;
    for(int i=0; i< len; i+=2)
    {
        std::string byte = hex.substr(i,2);
        char chr = (char) (int)strtol(byte.c_str(), NULL, 16);
        newString.push_back(chr);
    }
    return newString;
}
char * string2Char (std::string Input)
{

    char *Output = new char[Input.length() + 1];
    strcpy(Output, Input.c_str());
    return (Output);
}
bool string2bool(std::string Input)
{
    if (Input=="true")
        return true;
    else
        return false;
}
std::string bool2string(bool input)
{
    if (input)
        return "true";
    else
        return "false";
}
int string2int(const std::string str, int *p_value)
{
    // wrapping string2int because it may throw an exception
    std::size_t* pos = 0;
    int base = 10;
    try {
        *p_value = std::stoi(str, pos, base);
        return 0;
    }

    catch (const std::invalid_argument& ia) {
        //std::cerr << "Invalid argument: " << ia.what() << std::endl;
        return -1;
    }

    catch (const std::out_of_range& oor) {
        //std::cerr << "Out of Range error: " << oor.what() << std::endl;
        return -2;
    }

    catch (const std::exception& e)
    {
        //std::cerr << "Undefined error: " << e.what() << std::endl;
        return -3;
    }
}
int string2float(const std::string str, float *p_value)
{
    // wrapping string2int because it may throw an exception
    std::size_t* pos = 0;
    try {
        *p_value = std::stof(str, pos);
        return 0;
    }

    catch (const std::invalid_argument& ia) {
        //std::cerr << "Invalid argument: " << ia.what() << std::endl;
        return -1;
    }

    catch (const std::out_of_range& oor) {
        //std::cerr << "Out of Range error: " << oor.what() << std::endl;
        return -2;
    }

    catch (const std::exception& e)
    {
        //std::cerr << "Undefined error: " << e.what() << std::endl;
        return -3;
    }
}
