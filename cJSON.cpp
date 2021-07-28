#include "cJSON.h"
cJSON::cJSON()
{
    //ctor
}
cJSON::~cJSON()
{
    //dtor
}
std::string cJSON::getElement(std::string Message,std::string Name)
{
    if (Message.length()<(size_t)3)
        return "";
    sElemtentuValue ElementValue;
    ElementValue.Rest=Message;
    do
    {
//        printf("Ausgangstext getElement:\n%s\n",ElementValue.Rest.c_str());
        ElementValue=splitElement(ElementValue.Rest);
//        printf("Entedeckter Name: %s\nValue:\n%s\nRest:\n%s\n",ElementValue.Name.c_str(),ElementValue.Value.c_str(),ElementValue.Rest.c_str());

    }
    while (ElementValue.Name!=Name&&ElementValue.Rest.length()>(size_t)2);  // solange durchsuchen bis der Name passt oder das Ende erreicht wurde
    if (ElementValue.Name!=Name)
        {
            Message="";// nicht den passenden Wert gefunden, dann leeren String zurückgeben

        }
    else
 //       printf("Rückgabe Name: %s\nValue:\n%s\nRest:\n%s\n",ElementValue.Name.c_str(),ElementValue.Value.c_str(),ElementValue.Rest.c_str());
        Message=ElementValue.Value;//Namen passen überein, dass Value zurückgeben
    return Message;
}
std::string cJSON::getElement(std::string Message,std::string Name,int Field)
{
    if (Message.length()<(size_t)3)
        return "";
    sElemtentuValue ElementValue;
    ElementValue.Rest=Message;
    do
    {
//        printf("Ausgangstext getElement mit Field:\n%s\n",ElementValue.Rest.c_str());
        ElementValue=splitElement(ElementValue.Rest);
//        printf("Entedeckter Name: %s\nValue:\n%s\nRest:\n%s\n",ElementValue.Name.c_str(),ElementValue.Value.c_str(),ElementValue.Rest.c_str());
    }
    while (ElementValue.Name!=Name&&ElementValue.Rest.length()>(size_t)2);  // solange durchsuchen bis der Name passt oder das Ende erreicht wurde
    if (ElementValue.Name!=Name)
        return "";// nicht den passenden Wert gefunden, dann leeren String zurückgeben
    else
        Message=ElementValue.Value;//Namen passen überein, mit Value weiterarbeiten
    int laenge=0;
//    printf("Ausgangstext Fields:\n%s\n",Message.c_str());
    if (Message.find("[")==(size_t)0)
        Message=pickBlock(Message);
    else
        return "";
    for (int i=0; i<Field; i++)
    {
        laenge=pickBlock(Message).length()+2;// plus 1 für die nicht zurückgegeben Klammer, plus 1 um dahinter zu kommen
        Message=Message.substr(laenge,Message.length()-laenge);
        Message=cleanupRest(Message);// entfernt eventuelles Komma und Leerstellen
//        printf("Rest Fields:\n%s\n",Message.c_str());
    }
    Message=pickBlock(Message);
    Message=cleanupRest(Message);
    return Message;
}
sElemtentuValue cJSON::splitElement(std::string Message) // holt das erste Element und gibt den Namen, Wert und Rest zurück
{
//    printf("Message gleich am Anfang :\n%s\n",Message.c_str());
    if (Message.find("{")==(size_t)0)
        Message=pickBlock(Message);// sollte Message noch mit {} umschlossen sein wird es entfernt
//    printf("Message gleich am Anfang :\n%s\n",Message.c_str());
    int ElementHeaderLength;
    int Anfang=0;
    int Ende=0;
    int Matruschka=0; // um zu prüfen ob der Doppelpunkt innerhalb von Anfürhungszeichen ist
    sElemtentuValue Element;
    bool erstesQuote= false;
    std::string Value;
    while (Message.find(" ",Anfang) ==(size_t)Anfang)
        Anfang++;// führende Leerzeichen entfernen
    Ende=Anfang;
    int check=0;
    do
    {
        Ende=Message.find (":",Ende);// den ersten trenner finden
//        printf("Doppelgunkt gefunden :%i\n",Ende);
        do
        {
            check=Message.find("\"",check);// da das Schlüsselwort in Anfürhungszeichen steht, sollten wir im ersten Durchlauf was finden.
            if (check<Ende&&check>=0) //wir haben Anfürhungszeichen gefunden und das vor dem Doppelpunkt
            {
                if(erstesQuote)// es ist das zweite Anfürhungszeichen eines Paares
                {
//                    printf("Zweites Anführungszeichen gefunden: %i, Matruschka: %i\n",check,Matruschka);
                    erstesQuote=false;
                    Matruschka --;

                }
                else // wir hatten noch kein erstes gefunden also ist dies das erste
                {
//                    printf("Erstes Anführungszeichen gefunden: %i, Matruschka: %i\n",check,Matruschka);
                    erstesQuote=true;
                    Matruschka++;// deshalb müssen wir das zweite suchen
                }
                check++;
            }
            else
                Matruschka =0;// keine Anfürhungszeichen mehr, also Ende

        }
        while (Matruschka&&check >=0&& check<Ende);  //wenn erstesQuote == false dann sind alle Anfürhungszeichen vorher geschlossen worden
        Ende++; // wenn das gerade gefundene Zeichen ungültig ist, ab dem Zeichen weiter suchen.
//        if (erstesQuote) printf("Doppelpunkt war ungültig\n");
    }
    while (erstesQuote);  // so lange ein : von Anfürhungszeichen umschlossen ist das nächste suchen
    Ende --;// Ende wurde erhöht, selbst wenn das Zeichen gültig war
    Element.Name=Message.substr(Anfang,Ende-Anfang);//
    while (Element.Name.find(" ",0)==0)
        Element.Name.erase(0,1); // führende Leerzeichen entfernen
    Element.Name=pickBlock(Element.Name);// ist das Element umschlossen, auspacken
//    printf("Name :\n%s\n",Element.Name.c_str());
    ElementHeaderLength=Ende+1;// Zeiger hinter den Doppelpunkt setzten
    Message=Message.substr(ElementHeaderLength,Message.length()-ElementHeaderLength);
    while (Message.find(" ",0) == (size_t)0)
        Message.erase(0,1);

//    printf("Message :\n%s\n",Message.c_str());
    if (Message.find("{")==(size_t)0)
    {
        Element.Value=pickBlock(Message);
        Element.Rest=Message.substr(Element.Value.length()+2,Message.length()-(Element.Value.length()+2));
        Element.Rest=cleanupRest(Element.Rest);
        return Element;
    }
    if (Message.find("[")==(size_t)0)
    {
//            printf("Field erkannt :\n%s\n",Message.c_str());
        Element.Value=pickBlock(Message);
//            printf("Field block ausgeschnitten :\n%s\n",Element.Value.c_str());
        Element.Value.insert(0,"[");
        Element.Value+="]";
//            printf("Field block Klammern wieder hinzugefüngt :\n%s\n",Element.Value.c_str());
        if (Message.length()>Element.Value.length())
            Element.Rest=Message.substr(Element.Value.length(),Message.length()-Element.Value.length());
        else
            Element.Rest="";
//            printf("Rest:\n%s\n",Element.Rest.c_str());
        Element.Rest=cleanupRest(Element.Rest);
//            printf("Rest bereinigt:\n%s\n",Element.Rest.c_str());
        return Element;
    }
    int nextElementPos=0;
    //    do
//      {
    nextElementPos=Message.find(",",nextElementPos);
//           nextElementPos ++;
//           printf("nextElementPos: %i\n",nextElementPos);

    //      }while (nextElementPos>=0&&!checkValidCharacter(Element.Value,nextElementPos-1));
//       nextElementPos--;
    if (nextElementPos<0)
        nextElementPos=Message.length();
    Element.Value=Message.substr(0,nextElementPos);
//        printf("Roh Value:\n%s\n",Element.Value.c_str());
    nextElementPos++;// Zeiger hinter das Komma setzten
//    printf("Nextelementpos: %i,Element.Rest.length(): %i,Element.Rest.length()-nextElementPos: %i\n ",nextElementPos,Element.Rest.length(),Element.Rest.length()-nextElementPos);
    if (Message.length()>nextElementPos)
        Element.Rest=Message.substr(nextElementPos,Element.Rest.length()-nextElementPos);
    else
        Element.Rest="";
//        printf("Rest roh :\n%s\n",Element.Rest.c_str());
    Value=pickBlock(Element.Value);
//        printf("Value nach pickBlock:\n%s\n",Value.c_str());
    Element.Rest=cleanupRest(Element.Rest);
//        printf("Rest cleanup:\n%s\n",Element.Rest.c_str());
    Element.Value=Value;
    return Element;
}
std::string cJSON::pickBlock(std::string Message)// gibt den Inhalt von umschließenden Klammern oder Anfürhungszeichen zurück
{
    if (Message.length()<2)
        return "";// wenn kein Text enthalten ist, sind wir schon fertig
    std::string searchString=Message.substr(0,1);// das erste Zeichen soll einen Block kennzeichenen
    std::string Endstring="";
    if (searchString=="{")
        Endstring="}";// die gegenstücke ermitteln und festlegen
    else if (searchString=="[")
        Endstring="]";
    else if (searchString=="(")
        Endstring=")";
    else if (searchString=="\"")
        return easyBlock(Message);
    else if (Message.find(",")>=(size_t)0)
        return Message.substr(0,Message.find(","));// wenn ein Value keine Klammer enthält
    else
        return Message; // kein Zeichen erkannt das einen Block umschließt
//   printf("Suchstring: %s, Endstring: %s\n",searchString.c_str(),Endstring.c_str());
    int Anfang=0;
    int Matruschka=1;
    int Ende=1;
    do
    {
        Anfang ++;// das erste oder zuletzt gefundene Zeichen wird nicht mehr geprüft und auch nicht zurückgegeben
        Ende=Message.find(Endstring,Ende);// erstes schließendes Zeichen ab neuer Position finden
        if (Ende<0)
        {
            Matruschka=0;// Hätte nicht passierend dürfen, also aussteigen
            break;// hätte nicht passieren dürfen
        }
        Matruschka--;
        Anfang=Message.find(searchString,Anfang);// ist in dem Bereich noch ein neuer Anfang gewesen?
        if (Anfang>=0&& Anfang<Ende)// ein Anfangszeichen gefunden und noch vor dem zulettzt gefundenen Schlusszeichen
            Matruschka ++;// dann sind wir noch nicht fertig.
        Ende++;// wenn die Schleife weiter läuft müssen nach dem gefunden Abschlusszeichen noch mehr kommen

    }
    while (Matruschka&&Ende >=0);

    Message=Message.substr(1,Ende-2);// länge gleich -2, weil der Anfang auf 1 liegt und auch das letzte Zeichen entfernt werden soll
    return Message;
}
std::string cJSON::easyBlock(std::string Message)// wird durch pickBloc aufgerufen und gibt den Inhaolt von Anfürhungszeichen zurück
{
    Message.erase(0,1);// wir sind hier weil wir ein Anfürhungszeichen am Anfang gefunden hatten, dies wird jetzt entfernt
    Message=Message.substr(0,Message.find("\""));// Das nächste " ist auf jeden Fall das Ende des Blocks
    return Message;
}
std::string cJSON::cleanupRest(std::string Message)// wenn der Rest von getElement noch weitere Elemente enthält werden Komma und Leerzeichen entfernt
{
    int Anfang=0;
    Anfang =Message.find(",");
    if (Anfang >=0)
    {
        if ((size_t)Anfang > Message.find(":"))
        {
            Anfang =0;// vor dem Doppelpunkt kein Komma, also brauchen wir das nicht entfernen
        }
        else
            Anfang ++; //anderfalls müssen wir den Zeiger hinter das Komma setzten
    }
    while (Message.find(" ",Anfang)==(size_t)Anfang)
        Anfang++;// Solange am Anfang Leerzeichensind den Anfang nach hinten verschieben

    return Message.substr(Anfang,Message.length()-Anfang);// Text ohne den Teil vor Anfang zurückgeben
}
int cJSON::getValueNumberOfFields(std::string Message, std::string keyword1)
{
    std::string workstring=getElement(Message,keyword1);
    int counterFields=0;
    int Matruschka=0;
    int FieldAnfang=Message.find("{");
    int FieldEnde,subfieldAnfang,SubfieldEnde;
    int iPos;
    while (FieldAnfang>=0)
    {
        counterFields++;
        Matruschka=1;
        subfieldAnfang=FieldAnfang;//nach der ersten Klammer weiter prüfen
        SubfieldEnde=subfieldAnfang;
        while (Matruschka)// Felder im Feld überspringen
        {
            SubfieldEnde=Message.find("}",SubfieldEnde);
            if (subfieldAnfang<0)
                break;
            Matruschka --;
            SubfieldEnde++;
            subfieldAnfang=Message.find("{",subfieldAnfang+1,Message.length()-SubfieldEnde);
            if (subfieldAnfang<SubfieldEnde&&subfieldAnfang>=0)
                Matruschka++;
        }
        if (SubfieldEnde>=0)
            FieldAnfang=Message.find("{",SubfieldEnde+1);
        else
            break;
    }

    return counterFields;
}
bool cJSON::checkValidCharacter(std::string Message, int iPos)
{
    int check=0;
    int Matruschka=0;
    bool erstesQuote=false;
    do
    {
        check=Message.find("\"",check);// da das Schlüsselwort in Anfürhungszeichen steht, sollten wir im ersten Durchlauf was finden.
        if (check<iPos&&check>=0) //wir haben Anfürhungszeichen gefunden und das vor dem Doppelpunkt
        {
            if(erstesQuote)// es ist das zweite Anfürhungszeichen eines Paares
            {
//                    printf("Zweites Anführungszeichen gefunden: %i, Matruschka: %i\n",check,Matruschka);
                erstesQuote=false;
                Matruschka --;

            }
            else // wir hatten noch kein erstes gefunden also ist dies das erste
            {
//                    printf("Erstes Anführungszeichen gefunden: %i, Matruschka: %i\n",check,Matruschka);
                erstesQuote=true;
                Matruschka++;// deshalb müssen wir das zweite suchen
            }
            check++;
        }
        else
            Matruschka =0;// keine Anfürhungszeichen mehr, also Ende

    }
    while (Matruschka&&check >=0&& check<iPos);
//       printf("Exit checkValidCharacter\n");
    return erstesQuote;

}
std::string cJSON::createNewjsonString(std::string name, std::string Value)
{
    std::string output="{\""+name+"\":";

    if (Value=="")
        output+="{}";
    else
        output+="\""+Value+"\"}";
    output+="}";
    return output;
}
std::string cJSON::createNewjsonElement(std::string name, std::string Value)
{
    std::string output="\""+name+"\":";

    if (Value=="")
        output+="{}";
    else
        output+="\""+Value+"\"";
    return output;
}
std::string cJSON::appendToJsonString(std::string oldString,std::string toValue,std::string name, std::string Value)// vorraussetzung toValue ist einmalig in oldString
{
    std::string  insertstring;
    insertstring=createNewjsonElement(name,Value);
//    printf("oldString:\n%s\n",oldString.c_str());
//    printf("Inserstring:\n%s\n",insertstring.c_str());
    size_t nPos,Anfang,Ende;
    if (toValue=="")
    {
        nPos=oldString.find_last_of("}");
        if (nPos>=1)
        {
            if(oldString.find("{",nPos-1)!=nPos-1)
            {
                oldString.insert(nPos,",");
                nPos++;
            }
            oldString.insert(nPos,insertstring);
        }
    }
    else
    {
        nPos=oldString.find(toValue);
        if (nPos >=0)
        {
            nPos+=toValue.length();
            nPos=oldString.find(":",nPos);
            if (nPos >=0)
            {
                nPos++;
                Anfang=nPos;
                Ende=nPos;
                do
                {
//                    printf("Anfang Schleife: Anfang: %i, Ende: %i\n",Anfang,Ende);
                    Ende=oldString.find("}",Ende);
                    if (Ende <0)
                        break;// dürfte nich passieren
                    Ende ++;
                    Anfang=oldString.find("{",Anfang);
                    if (Anfang<0|| Anfang > Ende)
                        break;
                    Anfang ++;
//                    printf("Ende Schleife: Anfang: %i, Ende: %i\n",Anfang,Ende);

                }
                while (Anfang>=0);
                if (Ende <2)
                    return oldString;
                Ende--; // oben wurde zwangsweise einmal erhöht nach find
                Ende--; //Wieder vor die Klammer kommen
                if(oldString.find("{",Ende-1)!=Ende-1)
                {
                    oldString.insert(Ende,",");
                    Ende++;
                }
                oldString.insert(Ende,insertstring);
            }
        }
    }
    return oldString;
}


