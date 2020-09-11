/*
 * Soubor:  server.cpp
 * Datum:   21. 2. 2015
 * Autor:   Sara Skutova, xskuto00@stud.fit.vutbr.cz
 * Projekt: Komunikace Klient/Server, projekt c. 1 pro predmet IPK
 * Popis:   Klient zazada od severu informaci o uzivatelich daneho serveru a pak pozadovane informace vypise
 */


#include <iostream> // vstup, vystup, chybovy vystup
#include <string> //retezce c++
#include <vector> //vektor pro zapamatovani poradi, proc? protoze tohle v teto chvili znam
#include <unistd.h>
#include <cstring>
#include <cstdlib> //strtol
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sstream>
#include <fstream>
#include <sys/wait.h>
#include <signal.h>

#define MAXMES 3000
#define MINPORT  1024
#define MAXPORT 65535

using namespace std;

/**Napoveda, vytiskne se pri --help*/
const char *HELPMSG =
  "Program: Program pro vyhledani informaci o uzivatelich Unixoveho OS\n"
  "Autor: Sara Skutova (c) 2015\n"
  "Pouziti: server -p port\n"
  "         server -help\n"
  "Popis parametru:\n"
  " -help: vytiskne tuto napovedu\n"
  " -p port: cislo portu, na kterem server ceka na pripojeni klienta,\n"
  "          POVINNY PARAMETR\n"
  "Zaver: PAMATUJ! Program server musi na serveru bezet jinak je to na nic\n"
  "       Pokud tohle nekdo cte, lituji ze jsi nenasel lepsi program\n"
  "Necht tento program/projekt provazi Sila.";

  /** Kody chyb programu DOPSAT */
enum tecodes
{
  EOK = 0,     /**< Bez chyby */
  ECLWRONG,    /**< Chybny prikazovy radek*/
  EPARAM,      /**< Nedostateecny pocet parametru*/
  EPNUMBER,    /**< Cislo portu musi byt dekadicke cislo*/
  ESOCKET,      /**< Nepovedlo se vytvorit soket*/
  EBIND,        /**< Problem s navazanim na port*/
  ELISTEN,      /**< Chyba pri naslouchani/cekani na klienta*/
  EACCEPT,      /**< Chyba pri akcepovani pripojeni klinta*/
  ERECV,        /**< Chyba pri cteni zpravy klienta*/
  ECLOSE,       /**< Chyba pri uzavirani soketu*/
  ESEND,        /**< Chyba pri posilani zpravy klientu*/
  EFORK,        /**< Chyba pri vytvoreni procesu pro klienta*/
  EPASSWD,      /**< Problem se souborem passwd*/
  EUNKNOWN     /**< Neznama chyba */
};

/** Kody stavu programu*/
enum testes
{
  SHELP,         /**< Napoveda */
  SCOM           /**< Komunikace Klient/Server */
};

/** Chybova hlaseni odpovidajici chybovym kodum. */ /* Problem s pouzitim znaku pole na eve, OPRAVENO */
const char *ECODEMSG[] =
{
  "Vse v poradku\n", /** EOK **/
  "Chybne parametry prikazoveho radku!\nPro napovedu zadejte parametr -help\n", /** ECLWRONG **/
  "Nedostatecny pocet parametru\n", /** EPARAM **/
  "Cislo portu musi byt dekadicke, nezaporne cislo\n", /** EPNUMBER **/
  "Soket se nepodarilo vztvorit\n", /** ESOCKET **/
  "Nepodarilo se navazat na pozadovany port\n", /** EBIND **/
  "Chyba pri naslouchani/cekani na klienta\n", /** ELISTEN **/
  "Chyba pri akcepovani pripojeni klinta\n", /** EACCEPT **/
  "Chyba pri cteni zpravy klienta\n", /** ERECV **/
  "Chyba pri uzavirani soketu\n", /** ECLOSE **/
  "Chyba pri posilani zpravy klientu\n", /** ESEND **/
  "Chyba pri vytvoreni procesu pro klienta\n", /** EFORK **/
  "Chyba pri otervirani nebo pracovani se souborem /etc/passwd\n", /** EPASSWD **/
  "Nastala nepredvidaná chyba.\n", /** EUNKNOWN **/
};

 /**
 * Struktura obsahujici hodnoty parametru prikazove radky + dalsi veci.
 */
typedef struct params
{
  int ecode;       /**< Chybový kod programu, odpovida vyctu tecodes. */
  int port;        /**< cislo portu*/
  bool p;          /**< indikator povinnych parametru*/
  int state;       /**< Stavový kod programu, odpovida vvctu tstates. */
  bool L, U, G, N, H, S; /**< indikator nepovinnych paramentru*/
} PARAMS;

/**
 * Vytiskne hlaseni odpovidajici chybovemu kodu.
 * @param ecode kod chyby programu
 */
void printECode(int ecode)
{
  if (ecode < EOK || ecode > EUNKNOWN)
  { ecode = EUNKNOWN; }

  cerr << ECODEMSG[ecode];
}

/**
 * Zpracuje argumenty prikazoveho radku a vrati je ve strukture PARAMS.
 * Pokud je formát argumentu chybny, ukonci program s chybovym kodem.
 * @param argc Pocet argumentu.
 * @param argv Pole textových retezcu s argumenty.
 * @return Vraci analyzovane argumenty prikazoveho radku.
 */
PARAMS getParams(int argc, char *argv[])
{
  PARAMS result =
  {
     EOK, 0, false, SCOM, false, false, false, false, false, false
  };

   if (argc == 2 && (strcmp("-help", argv[1]) == 0)) //chceme napovedu
  {
      result.ecode = EOK;
      result.state = SHELP;
      return result;
  }

  if (argc < 3 || argc > 3) // 3 - minimalni/maximalni pocet parametru server -p port = 1 2 3
    {
        result.ecode = EPARAM;
        return result;
    }


  int c;
  char *pEnd;
  opterr = 0; //aby getopt nevypisoval chybove hlasky

  while ((c = getopt(argc, argv, "p:")) != -1) //cary mary!
  {
      switch (c)
      {
          case 'p':
              result.p = true;
              result.port = strtol(optarg, &pEnd, 10); //prevod retezce na cislo, v pEnd bude prvni znak co neni cislo
              if(*pEnd != '\0')
              {
                  result.ecode = EPNUMBER; //port neni cislo
                  return result;
              }
              break;
          default:
            result.ecode = ECLWRONG;
            return result;
      }
  }

  //kontrola zda existuji POVINNE PARAMETRY
  if (result.p == false)
   {
       result.ecode = EPARAM;
       return result;
   }
   // port nemuze byt mensi nez 0 !!!!! KONTROLA zda existuji jeste jine podminky
  if (result.port < MINPORT || result.port > MAXPORT)
   {
        result.ecode = EPNUMBER;
        return result;
   }
    return result;

}

/**
 * Vytvori soket socket
 * @param *soket deskriptor soketu
 * @return EOK pri vytvoreni soketu, chybu pri nevytvoreni
 */
 int getSocket(int *soket)
 {
     if ((*soket = socket(PF_INET, SOCK_STREAM, 0 )) == -1) //tento soket bude slouzit pouze pro prijem spojeni
     {
         return ESOCKET; //chyba soket se nevztvoril
     }
     return EOK;
 }

 /**
  * Pojemnuje soket/navaye soket na port zadany uyivatelem
  * @param soket deckriptor soketu
  * @param params struktura s parametry
  * @return EOK pri bezchybnem navazani, jinka chybovy kod
  */
int bindMe(int soket, PARAMS params)
{
    struct sockaddr_in socketAddress;

     memset(&socketAddress, '\0', sizeof(sockaddr_in)); //vynuluje pamet

    socketAddress.sin_family = PF_INET;
    socketAddress.sin_port = htons(params.port);  //nastaveni portu na jakem budu cekat/poslochat
    socketAddress.sin_addr.s_addr  = INADDR_ANY;  //INADDR_ANY - bude se moct pripojit kdokoliv z jakekoliv site

     if (bind(soket, (struct sockaddr *)&socketAddress, sizeof(socketAddress) ) == -1)
        return EBIND;

    return EOK;

}

/**
  * Zpracuje zpravu od klienta, a vrati konecnou zpravu pro klienta
  * @param client retezec ze spravou od klienta
  * @param *params struktura s parametry
  * @return koncova zprava pro klienta
  */
//  0   1   2    3      4    5      6    7     8       9          10     11     12........
/**You are The Chosen One! Master! The Force want  + polozky +  from  + l/u + od_koho**/
/**polozky bude jeden retezec, od_koho bude  1 az n retezcu**/
vector<string> playString(string client, PARAMS *params)
{
    //sleep(5); // po testovani konkurentnosti
    vector<string> words;
    vector<string> od_koho;
    vector<string> all;
    string word;
    string polozky;
    bool login = false;
    bool uid = false;
    params->G = true;

// oddelit jednotliva slova abych ziskala polozky a od_koho
    istringstream in(client);
    while(in >> word)
        words.push_back(word);

// prochzet slova, pchi presne uz jenom polozky a od_koho
    for (unsigned int i = 0; i < words.size(); i++)
    {
        if (words[i] == "THE_END!")
            break;

        if (i == 9) //polozky
            polozky = words[i];
        else if (i == 11) //login nebo UID?
            {
                if (words[i] == "l")//bude se vyhledavat dle loginu
                    login = true;
                else // vyhledavame dle uid
                    uid = true;
            }
        else if (i > 11) //od_koho
            od_koho.push_back(words[i]);
        else
            continue;
    }

// BOJ! o Vysledky z /etc/passwd aneb nejak to dostat k klientovi
    string s;
    string anotherS;
    vector<string> help;
    bool nalezeno = false;
    string vypis;
    string helpME;
    vector<string> bylo;
    bool exist = false;
    //bool uz_bylo = false;
    int NECO_NALEZENO = false;

    for (unsigned int i = 0; i < od_koho.size(); i++)
    {

        ifstream file("/etc/passwd");
        if (file);
        else
        {
            all.clear();
            all.push_back("FAIL_PASSWD");
            return all;
        }

        s.clear(); anotherS.clear(); help.clear(); vypis.clear();
        vypis = "";
        nalezeno = false;
        exist = false;
        NECO_NALEZENO = false;

        //radek z "/etc/passwd", ONO TO FUNGUJE!!!! NEHYBAT S TIM!!!!!!
        while(getline(file, s))
        {
            help.clear(); //NESAHAT, MUSI BYT!!!!!!
            stringstream test(s);
            nalezeno = false;
            vypis.clear();
            //dostat jednotliva slova oddelena :, protoye v passwd jsou oddelena :
            while(getline(test, anotherS, ':'))
                help.push_back(anotherS);

            /** 0      1      2   3     4      5      6   tohle poradi v vectoru help*/
            /**LOGIN:PASSWORD:UID:GID:GECOS:HOME_DIR:SHELL*/
            /** L              U   G    N       H      S*/
            //ej to radek ktery hledam?
            if (login && (strcasecmp(od_koho[i].c_str(), help[0].c_str()) == 0)) //HEEEE!!!! HNUS! vyhledavam dle loginu, ignoring case
                nalezeno = true;
            else if (uid && (od_koho[i].compare(help[2]) == 0))
                nalezeno = true;
            else
                continue; //neni zhoda, jdeme na dalsi radek

            if(nalezeno) //ZHODAAAAAAAAAAA
            {
                NECO_NALEZENO = true; // tohle je pro pro pripad kdz by bylo vicekrat stejne iud/login v passwd
                if(polozky == "EXIST!")
                    exist = true;
                else
                {
                //ziskat to co chci v poradi jakem chci, ulozi do retezce a ten ulozit do vektoru, prochayet reteyec po znacich
                //jednotlive polozky budou oddelene meyerami
                    for (string::iterator a = polozky.begin(); a != polozky.end(); a++)
                    {
                        if (*a == 'L')
                            vypis += " " + help[0]; //login
                        else if (*a == 'U')
                            vypis += " " + help[2]; //UID
                        else if (*a == 'G')
                            vypis += " " + help[3]; //GID
                        else if (*a == 'N')
                            vypis += " " + help[4]; //GECOS
                        else if (*a == 'H')
                            vypis += " " + help[5]; //HOME_DIR
                        else if (*a == 'S')
                            vypis += " " + help[6]; //SHELL
                        else
                            continue;
                    }
                }
            }//if

                if (nalezeno && NECO_NALEZENO)
                {
                    if (exist)
                    {
                        if (login)
                            vypis += " Uzivatel s loginem " + od_koho[i] + " existuje:";
                        else
                            vypis += " Uzivatel s UID " + od_koho[i] + " existuje:";
                    }
                    else
                        vypis += ":";

                    all.push_back(vypis);

                }

        }//while

        if(!NECO_NALEZENO) //kdyz nebyl nalezen zadny zaznam
        {
            if(login)
                vypis += " Chyba! Neexistujici uzivatel s loginem " + od_koho[i] + ":";
            else
                vypis += " Chyba! Neexistujici uzivatel s UID " + od_koho[i] + ":";
            all.push_back(vypis);//ta mezera je tam chvalne!!!!
        }
    }//for

        return all;


}

void The_END(int sig)
{
    cout << "\nServer konci se signalem: " << sig << endl;
    exit(0);
}

 /**
  * Bude cekat na pripojeni klienta, navaze se na klienta
  * @param soket deckriptor soketu
  * @param params struktura s parametry
  * @return EOK pri bezchybnem navazani, jinka chybovy kod
  */
int serverForClient(int soket, PARAMS *params)
{
    int clientSoket, mesLen;
    socklen_t clientAddressLen;
    struct sockaddr_in clientAddress;
    char clientMes[MAXMES]; // tady je schovana zprava od klienta
    vector<string> forClient;
    pid_t clientID;
    string help = "";
    size_t found;

    memset(&clientMes, 0, sizeof(clientMes));

    if (listen(soket, 5) == -1) //naslouchani, ta 5 znamena ze muzu maximalne na 5 klientu cekat, vztvori se fronta
        return ELISTEN;

        clientAddressLen = sizeof(clientAddress);

        signal(SIGINT, The_END);
        signal(SIGCHLD, SIG_IGN); //a doufat ze se nic nepokazi, ignorovat signal od potomku

    while(true) //klient te potrebuje porad
    {
        //bude z fronty vybirat pozadavky klientu, vrati soket klienta, na niem bude pobihat komunikace
        if ((clientSoket = accept(soket, (struct sockaddr *) &clientAddress, &clientAddressLen)) < 0)
            return EACCEPT;

        //fork fork fork pork, vytvorit proces pro klienta
        if ((clientID = fork()) < 0)
            return EFORK;

        if (clientID == 0) //POTOMEK, KLIENT
        {
            close(soket); //zavrit otce..
            while ((mesLen = recv(clientSoket, clientMes, MAXMES, 0)) > 0) //cist od klienta
            {
               help += clientMes;
               found = help.find("THE_END!");
               if (found != string::npos) // konec zpravy
                    break;
            // tohle musi byt, v pripade ze se zmensil velikoct u primju, tak se tam vypisoval jesce nejaky neporadek
                memset(&clientMes, 0, sizeof(clientMes));
            }
            if (mesLen < 0) //chyba pri cteni
            {
                printECode(ERECV);
                exit(ERECV);
            }
            string client = help;

            memset(&clientMes, 0, sizeof(clientMes)); //snad to pomuze, byl tam nejaky bordel od predchoziho clienta, VYRESENO!!!!

            forClient = playString(client, params);


            if (forClient[0] == "FAIL_PASSWD")
            {
                printECode(EPASSWD);
                forClient[0] = " FAIL_PASSWD Server neocekavane ukoncil spojeni";
            }

            for(unsigned int i = 0; i < forClient.size(); i++)
            {
                if (send(clientSoket, forClient[i].c_str(), forClient[i].length(), 0) == -1) //poslat zpravu klientu
                {
                    printECode(ESEND);
                    exit(ESEND);
                }
            }

            if (close(clientSoket) == -1) // zavrit soket klienta
            {
                printECode(ECLOSE);
                exit(ECLOSE);
            }

            exit(0);
        }
        else //rodic
        {
            if (close(clientSoket) == -1) // zavrit soket klienta
            {
                printECode(ECLOSE);
                exit(ECLOSE);
            }
        }
    }

    return EOK;

}

int main(int argc, char *argv[])
{
    int soket;
    int navrat;
    PARAMS params = getParams(argc, argv);

    if (params.ecode != EOK)
    {
        printECode(params.ecode);
        return EXIT_FAILURE;
    }

    if (params.state == SHELP)
    {
        cout << HELPMSG << endl;
        return EXIT_SUCCESS;
    }

    navrat = getSocket(&soket);
    if (navrat != EOK)
    {
        printECode(navrat);
        return EXIT_FAILURE;
    }

    navrat = bindMe(soket, params);
    if (navrat != EOK)
    {
        printECode(navrat);
        return EXIT_FAILURE;
    }

    navrat = serverForClient(soket, &params);
    if (navrat != EOK)
    {
        printECode(navrat);
        return EXIT_FAILURE;
    }


    return EXIT_SUCCESS;
}
