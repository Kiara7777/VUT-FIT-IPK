/*
 * Soubor:  ipkperfclient.cpp
 * Datum:   24. 4. 2015
 * Autor:   Sara Skutova, xskuto00@stud.fit.vutbr.cz
 * Projekt: Jednoduchy tester end-to-end parametru site, projekt c. 2 pro predmet IPK
 */

#include <iostream> // vstup, vystup, chybovy vystup
#include <fstream>
#include <string> //retezce c++
#include <getopt.h> //getopt_long
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstdlib> //strtol
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sstream>
#include <signal.h>

#define MAXMES 3000
using namespace std;

/** Kody chyb programu DOPSAT */
enum tecodes
{
  EOK = 0,     /**< Bez chyby */
  ECLWRONG,    /**< Chybny prikazovy radek*/
  ESOCKET,     /**< Chyba pri vytvoreni socketu*/
  EBIND,        /**< Problem s navazanim na port*/
  ERECV,        /**< Chyba pri cteni zpravy*/
  ESEND,        /**< Chyba pri odesilani zpravy*/
  EUNKNOWN     /**< Neznama chyba */
};

/** Chybova hlaseni odpovidajici chybovym kodum. */ /* Problem s pouzitim znaku pole na eve, OPRAVENO */
const char *ECODEMSG[] =
{
  "Vse v poradku\n", /** EOK **/
  "Chybne parametry prikazoveho radku!\n", /** ECLWRONG **/
  "Chyba pri vytvoreni socketu\n",    /** ESOCKET **/
  "Problem s navazanim na port\n",    /** EBIND **/
  "Chyba pri cteni zpravy\n",       /** ERECV **/
  "Chyba pri odesilani zpravy\n",   /** ESEND **/
  "Nastala nepredvidaná chyba.\n", /** EUNKNOWN **/
};

 /**
 * Struktura obsahujici hodnoty parametru prikazove radky.
 */
typedef struct params
{
  int ecode;       /**< Chybový kod programu, odpovida vyctu tecodes. */
  int p;   /**< Port serveru, implicitne 5656*/
  bool port;
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
    PARAMS result = {EOK, 5656, false};

    int option_index = 0;
    static struct option long_options[] = {
    {"port", required_argument, NULL, 'p'},
    {0,0,0,0}
    };

    int c;
    opterr = 0; //nervi getopt
    char *pEnd;

    /************************** PREVZATO Z MANUALOVE STRANKY GETOPT **********************************/
    //funguje? FUNGUJE! + prvni projekt
    while ((c = getopt_long(argc, argv, "p:", long_options, &option_index)) != -1) //cary mary!
    {
      switch (c)
      {
          case 'p':
              result.p = strtol(optarg, &pEnd, 10); //prevod retezce na cislo, v pEnd bude prvni znak co neni cislo
              if(*pEnd != '\0' || result.p < 0 || result.port)
              {
                  result.ecode = ECLWRONG; //port neni cislo
                  return result;
              }
              result.port = true;
              break;
          default:
            result.ecode = ECLWRONG;
            return result;
      }
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
     if ((*soket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
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

     memset(&socketAddress, 0, sizeof(sockaddr_in)); //vynuluje pamet

    socketAddress.sin_family = AF_INET;
    socketAddress.sin_port = htons(params.p);  //nastaveni portu na jakem budu cekat/poslochat
    socketAddress.sin_addr.s_addr  = htonl(INADDR_ANY);  //INADDR_ANY - bude se moct pripojit kdokoliv z jakekoliv site

     if (bind(soket, (struct sockaddr *)&socketAddress, sizeof(socketAddress) ) == -1)
        return EBIND;

    return EOK;

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
int serverForClient(int soket)
{
    socklen_t clientAddressLen;
    struct sockaddr_in clientAddress;
    char clientMes[MAXMES]; // tady je schovana zprava od klienta

    memset(&clientMes, 0, sizeof(clientMes));

    clientAddressLen = sizeof(clientAddress);

    signal(SIGINT, The_END);
    //int i = 0;
    while(true) //klient te potrebuje porad
    {

            if ((recvfrom(soket, clientMes, MAXMES, 0, (struct sockaddr *) &clientAddress, &clientAddressLen)) < 0) //cist od klienta
                return ERECV;

            if (sendto(soket, clientMes, MAXMES, 0, (struct sockaddr *) &clientAddress, sizeof(clientAddress)) < 0) //poslat zpravu klientu
                return ESEND;

            memset(&clientMes, 0, sizeof(clientMes)); //snad to pomuze, byl tam nejaky bordel od predchoziho clienta, VYRESENO!!!!

    }

    return EOK;

}


int main(int argc, char *argv[])
{
      PARAMS params = getParams(argc, argv);

    if (params.ecode != EOK)
    {
        printECode(params.ecode);
        return EXIT_FAILURE;
    }

    int navrat = EOK;
    int soket;
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

    navrat = serverForClient(soket);
    if (navrat != EOK)
    {
        printECode(navrat);
        return EXIT_FAILURE;
    }

    close(soket);

    return 0;
}
