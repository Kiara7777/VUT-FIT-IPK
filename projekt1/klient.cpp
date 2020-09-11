/*
 * Soubor:  klient.cpp
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

#define MAXMES 3000
#define MINPORT  1024
#define MAXPORT 65535

using namespace std;

/**Napoveda, vytiskne se pri --help*/
const char *HELPMSG =
  "Program: Program pro vyhledani informaci o uzivatelich Unixoveho OS\n"
  "Autor: Sara Skutova (c) 2015\n"
  "Pouziti: client -h hostname -p port -l login ... -u uid ... -L -U -G -N -H -S\n"
  "         client -help\n"
  "Popis parametru:\n"
  " -help: vytiskne tuto napovedu\n"
  " -h hostname: jmeno severu, POVINNY PARAMETR\n"
  " -p port: cislo portu, kde ceka na pripojeni server, POVINNY PARAMETR\n"
  " -l login/ -u uid: kriterium dle ktereho se vyhledava, POVINNY PARAMETR\n"
  " Pozadovane polozky: NENI POVINNE\n"
  "         -L: uzivatelske jmeno\n"
  "         -U: UID\n"
  "         -G: GID\n"
  "         -N: cele jmeno, cely gecos pokud obsahuje dalsi polozky oddelene carkou\n"
  "         -H domovsky adresar\n"
  "         -S logovaci shell\n"
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
  EUID,        /**< UID musi byt cislo*/
  EUL,         /**< Parametr -l a -u*/
  ESOCKET,     /**< Chyba pri vytvoreni soketu*/
  EDNS,         /**< Chyba pri prekladu adresy*/
  ECONECT,      /**< Chyba pri pokusu o pripojeni k serveru*/
  ESEND,        /**< Chyba pri odeslani zpravy*/
  ECLOSE,       /**< Chyba pri uzavirani spojeni se serverem*/
  ERECV,        /**< Chyba pri prijmu zpravy ze serveru*/
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
  "Cislo portu musi byt dekadicke, nezaporne cislo v rozsahu <1024-65535>\n", /** EPNUMBER **/
  "Cislo UID musi byt dekadicke, nezaporne cislo\n", /** EUID **/
  "Parametry -l a -u nemohou byt spolecme\n", /** EUL **/
  "Soket se nepodarilo vztvorit\n", /** ESOCKET **/
  "Vyskytla se chyba pri prekladu adresy serveru\n", /** EDNS **/
  "Chyba pri pokusu o pripojeni k zadanemu serveru\n", /** ECONECT **/
  "Chyba pri odesilani zpravy na server\n", /** ESEND **/
  "Chyba pri uzavirani spojeni se serverem", /** ECLOSE **/
  "Chyba pri prijmu zpravy ze serveru\n",   /** ERECV **/
  "Nastala nepredvidaná chyba.\n", /** EUNKNOWN **/
};

 /**
 * Struktura obsahujici hodnoty parametru prikazove radky.
 */
typedef struct params
{
  int ecode;       /**< Chybový kod programu, odpovida vyctu tecodes. */
  int port, uid;
  string hostname, login;
  bool L, U, G, N, H, S; /**< indikator nepovinnych paramentru*/
  bool h, p, l, u; /**< indikator povinnych parametru*/
  int state;       /**< Stavový kod programu, odpovida vvctu tstates. */
  vector<char> poradi_params;
  vector<string> all_login;
  vector<string> all_uid; /**< Ono je to cislo, ale ja to prevedu hned na string*/
  char last;
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
  PARAMS result; /* Problem s inicializaci na eve, UDELEJ JIMAK */
  result.ecode = EOK;
  result.port = result.uid = 0;
  result.hostname = result.login = "";
  result.state = SCOM;
  result.h = result.p = result.l = result.u = false;
  result.L = result.U = result.G = result.N = result.H = result.S = false;
  result.poradi_params.clear();
  result.all_login.clear();
  result.all_uid.clear();
  result.last = '0';
  /* OPRAVENO!! HAHA!!! Sara: 1 Eva: 1024 DOHANIM JI! */

   if (argc == 2 && (strcmp("-help", argv[1]) == 0)) //chceme napovedu
  {
      result.ecode = EOK;
      result.state = SHELP;
      return result;
  }

  if (argc < 7) // 7 - minimalni pocet parametru
    {
        result.ecode = EPARAM;
        return result;
    }


  int c;
  char *pEnd;
  opterr = 0; //aby getopt nevypisoval chybove hlasky
  int index;

  while ((c = getopt(argc, argv, "h:p:l:u:LUGNHS")) != -1) //cary mary!
  {
      switch (c) //nejdriv nepovinne a pak povinne parametry
      {
		  case 'L':
		      if (result.L)
                break;
              result.poradi_params.push_back('L');
              result.L = true;
              break;
          case 'U':
              if (result.U)
                break;
              result.poradi_params.push_back('U');
              result.U = true;
              break;
          case 'G':
              if (result.G)
                break;
              result.poradi_params.push_back('G');
              result.G = true;
              break;
          case 'N':
              if (result.N)
                break;
              result.poradi_params.push_back('N');
              result.N = true;
              break;
          case 'H':
              if (result.H)
                break;
              result.poradi_params.push_back('H');
              result.H = true;
              break;
          case 'S':
              if (result.S)
                break;
              result.poradi_params.push_back('S');
              result.S = true;
              break;
          case 'h':
              if (result.h)
                break;
              result.h = true;
              result.hostname = optarg; //jmeno serveru, tento parametr musi byt zadany
              break;
          case 'p':
              if (result.p)
                break;
              result.p = true;
              result.port = strtol(optarg, &pEnd, 10); //prevod retezce na cislo, v pEnd bude prvni znak co neni retezec
              if(*pEnd != '\0')
              {
                  result.ecode = EPNUMBER; //port neni cislo
                  return result;
              }
              break;
          case 'l':
              if (result.l)
                break; // case
              result.all_login.clear();
              result.l = true;
              result.last = 'l';
              optind--;
              index = optind;
              while(index < argc) // dostanu i ty dalsi loginy
              {
                  char *argument = argv[index];
                  if (argument[0] == '-')
                    break; //while
                  else
                  {
                      result.all_login.push_back(argv[index]);
                      optind++;
                      index = optind;
                  }
              }
              break;
          case 'u':
              if (result.u)
                break; //case
              result.all_uid.clear();
              result.u = true;
              result.last = 'u';
              optind--;
              index = optind;
              while(index < argc) // dostanu i ty dalsi loginy
              {
                  char *argument = argv[index];
                  if (argument[0] == '-')
                    break; //while
                  else
                  {
                      result.uid = strtol(argv[index], &pEnd, 10);
                      if(*pEnd != '\0')
                      {
                          result.ecode = EUID; //uid neni cislo
                          return result;
                      }

                        // UID nemuze byt mensi nez 0 !!!!! KONTROLA zda existuji jeste jine podminky
                      if (result.uid < 0)
                      {
                          result.ecode = EUID;
                          return result;
                      }

                      //prevod cisla na string
                      string numer;
                      ostringstream convert;
                      convert << result.uid;
                      numer = convert.str();
                      result.all_uid.push_back(numer);

                      optind++;
                      index = optind;
                  }
              }
              break;
          default:
            result.ecode = ECLWRONG;
            return result;
      }
  }

  //kontrola zda existuji POVINNE PARAMETRY
  if (result.h == false || result.p == false || (result.l == false && result.u == false))
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
     if ((*soket = socket(PF_INET, SOCK_STREAM, 0 )) == -1)
     {
         return ESOCKET; //chyba soket se nevztvoril
     }
     return EOK;
 }

 /**
  * Konfigurace spojeni a navazani spojeni s serverem
  * @param soket deskriptor soketu
  * @param params parametry prikazove radky
  * @return EOK po bezchybnem navazani spojeni jinak nejaka chyba
  */
  int configConnect(int soket, PARAMS params)
  {
      struct sockaddr_in dest_addr; //pro cilovou ardresu, pro adresu serveru
      struct hostent *dns_addr; //pro DNS preklad

        memset(&dest_addr, '\0', sizeof(sockaddr_in)); //vynuluje pamet

      if ((dns_addr = gethostbyname(params.hostname.c_str())) == NULL) //prevede jmeno serveru na adresu, NEJAK!, pri chybe NULL
        return EDNS;

      memcpy(&dest_addr.sin_addr, dns_addr->h_addr, dns_addr->h_length); // nahraje adresu serveru do struktury

      dest_addr.sin_family = PF_INET;
      dest_addr.sin_port = htons(params.port); //prevod portu na sitovy tvat

     if (connect(soket, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) == -1) //pripojeni k serveru, -1 pri chybe
        return ECONECT;

    return EOK;

  }
  /**
  * Vytvori zpravu ktera se odesle na server
  * @param params parametry prikazove radky
  * @return vytvorena zprava
  */
  string message(PARAMS params)
  {
        string polozky = "";
        string od_koho = "";
        string all;

        for(unsigned int i = 0; i < params.poradi_params.size(); i++) //co chci od kaydeho loginu/uid
            polozky += params.poradi_params[i];

        if (polozky.empty())
            polozky += "EXIST!";

        if (params.last == 'l') //posledni byl login
        {
            od_koho = "l";
            for(unsigned int i = 0; i < params.all_login.size(); i++) //co chci od kaydeho loginu/uid
                od_koho += " " + params.all_login[i];
        }
        else //je tam u nic jinaciho tam byt nemuze
        {
            od_koho = "u";
            for(unsigned int i = 0; i < params.all_uid.size(); i++) //co chci od kaydeho loginu/uid
                od_koho += " " + params.all_uid[i];
        }

        all = "You are The Chosen One! Master! The Force want " + polozky + " from " + od_koho + " THE_END!";

        return all;


  }
/**
  * Komunikace se severem, zaslani a prijem zpravy
  * @param soket deskriptor soketu
  * @param params parametry prikazove radky
  * @return EOK po bezchybne kominukaci jinak nejaka chyba
  */
int comunicate(int soket, PARAMS params)
{
    string server_message = message(params); //vytvoreni zpravy
    char serverMes[MAXMES];
    int mesLen = 0;
    vector<string> all;
    string zprava;

    memset(&serverMes, 0, sizeof(serverMes));

    if (send(soket, server_message.c_str(), server_message.length(), 0) == -1) //poslat zpravu, vrati se delka zpravy
        return ESEND;

    while ((mesLen = recv(soket, serverMes, MAXMES, 0)) > 0) //dostat celou zpravu
    {
        string helpMEs(serverMes);
        zprava += helpMEs;
		memset(&serverMes, 0, sizeof(serverMes));
    }

    if (mesLen < 0)
    {
        return ERECV;
    }

    string anotherS;

    stringstream test(zprava);
    while(getline(test, anotherS, ':')) //rozdelit na jednotline casti
        all.push_back(anotherS);

    for (unsigned int i = 0; i < all.size(); i++)
    {
        all[i].erase(0,1);
        if (all[i].find("Chyba!", 0) == 0)
            cerr << all[i] << endl;
        else if(all[i].find("FAIL_PASSWD", 0) == 0)
            cerr << all[i] << endl;
        else
            cout << all[i] << endl;

    }


    if (close(soket) == -1)
        return ECLOSE;

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

    navrat = configConnect(soket, params);
    if (navrat != EOK)
    {
        printECode(navrat);
        return EXIT_FAILURE;
    }

    navrat = comunicate(soket, params);
    if (navrat != EOK)
    {
        printECode(navrat);
        return EXIT_FAILURE;
    }



    return 0;
}
