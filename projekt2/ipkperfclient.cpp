/*
 * Soubor:  ipkperfclient.cpp
 * Datum:   24. 4. 2015
 * Autor:   Sara Skutova, xskuto00@stud.fit.vutbr.cz
 * Projekt: Jednoduchy tester end-to-end parametru site, projekt c. 2 pro predmet IPK
 */

#include <iostream> // vstup, vystup, chybovy vystup
#include <fstream>
#include <string> //retezce c++
#include <string.h>
#include <getopt.h> //getopt_long
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstdlib> //strtol
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include <vector>
#include <sstream>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAXMES 3000

int sh_id, sh_idrecv, sh_idsp, sh_idrecvp;

using namespace std;

/** Kody chyb programu DOPSAT */
enum tecodes
{
  EOK = 0,     /**< Bez chyby */
  ECLWRONG,    /**< Chybny prikazovy radek*/
  ESOCKET,     /**< Chyba pri vytvoreni socketu*/
  EDNS,        /**< Chyba pri prekladu adres*/
  ESEND,        /**< Chyba pri zasilani zpravy*/
  ERECV,        /**< Chyba pri cteni zpravy*/
  EFORK,        /**< Chyba pri fork*/
  ESHM,         /**< Problem pri vytvareni sdilene pameti*/
  EUNKNOWN     /**< Neznama chyba */
};

/** Chybova hlaseni odpovidajici chybovym kodum. */ /* Problem s pouzitim znaku pole na eve, OPRAVENO */
const char *ECODEMSG[] =
{
  "Vse v poradku\n", /** EOK **/
  "Chybne parametry prikazoveho radku!\n", /** ECLWRONG **/
  "Chyba pri vytvoreni socketu",    /** ESOCKET **/
  "Chyba pri prekladu adresresy serveru/", /** EDNS **/
  "Chyba pri zasilani zpravy\n",    /** ESEND **/
  "Chyba pri cteni zpravy\n",   /** EREVC **/
  "Chyba pri fork\n",       /** EFORK **/
  "Problem pri vytvareni sdilene pameti\n", /** ESHM **/
  "Nastala nepredvidaná chyba.\n", /** EUNKNOWN **/
};

 /**
 * Struktura obsahujici hodnoty parametru prikazove radky.
 */
typedef struct params
{
  int ecode;       /**< Chybový kod programu, odpovida vyctu tecodes. */
  int p;   /**< Port serveru, implicitne 5656*/
  int s;   /**< Velikost payload paketu, implicitne 100 bajtu*/
  int r;   /**< rychlost odesilani paket/s, implicitne 10*/
  int t;   /**< doba behu klienta, implicitne nekonecno*/
  int i;   /**< interval*/
  string server;    /**< jmeno/adresa serveru*/
  string SIZES;
  string RATE;
  bool time, port, sizes, rate, interval;
} PARAMS;

typedef struct s_send
{
    int paket;  /** Pocet odeslanych paketu */

}S_SEND;

typedef struct s_recv
{
    int paket;  /** Pocet prijatych paketu*/
    int ooorder; /** Pocet Out Of Order*/
}S_RECV;

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
    PARAMS result = {EOK, 5656, 100, 10, -1, 10, "", "", "", false, false, false, false, false};

    int option_index = 0;
    static struct option long_options[] = {
    {"port", required_argument, NULL, 'p'},
    {"size", required_argument, NULL, 's'},
    {"rate", required_argument, NULL, 'r'},
    {"timeout", required_argument, NULL, 't'},
    {"interval", required_argument, NULL, 'i'},
    {0,0,0,0}
    };

    int c;
    opterr = 0; //nervi getopt
    char *pEnd;

    /************************** PREVZATO Z MANUALOVE STRANKY GETOPT **********************************/
    //funguje? FUNGUJE! + prvni projekt
    while (1)
    {
        c = getopt_long(argc, argv, "p:s:r:t:i:", long_options, &option_index);
        if ( c == -1)
            break;
          switch(c)
          {
                case 'p':
                    result.p = strtol(optarg, &pEnd, 10); //prevod retezce na cislo, v pEnd bude prvni znak co neni retezec
                    if(*pEnd != '\0' || result.p < 0 || result.port)
                    {
                        result.ecode = ECLWRONG; //port neni cislo
                        return result;
                    }
                    result.port = true;
                    break;
                case 's':
                    result.s = strtol(optarg, &pEnd, 10); //prevod retezce na cislo, v pEnd bude prvni znak co neni retezec
                    if(*pEnd != '\0' || result.s < 10 || result.sizes || result.s > 1472) // 1472 MAX MTU
                    {
                        result.ecode = ECLWRONG; //port neni cislo
                        return result;
                    }
                    result.SIZES = optarg;
                    result.sizes = true;
                    break;
                case 'r':
                    result.r = strtol(optarg, &pEnd, 10); //prevod retezce na cislo, v pEnd bude prvni znak co neni retezec
                    if(*pEnd != '\0' || result.r < 0 || result.rate)
                    {
                        result.ecode = ECLWRONG; //port neni cislo
                        return result;
                    }
                    result.RATE = optarg;
                    result.rate = true;
                    break;
                case 't':
                    result.t = strtol(optarg, &pEnd, 10); //prevod retezce na cislo, v pEnd bude prvni znak co neni retezec
                    if(*pEnd != '\0' || result.t < 0 || result.time)
                    {
                        result.ecode = ECLWRONG; //port neni cislo
                        return result;
                    }
                    result.time = true;
                    break;
                case 'i':
                    result.i = strtol(optarg, &pEnd, 10); //prevod retezce na cislo, v pEnd bude prvni znak co neni retezec
                    if(*pEnd != '\0' || result.i < 0 || result.interval)
                    {
                        result.ecode = ECLWRONG; //port neni cislo
                        return result;
                    }
                    result.interval = true;
                    break;
                case '?':
                    result.ecode = ECLWRONG; //port neni cislo
                    return result;
                default:
                    result.ecode = ECLWRONG; //port neni cislo
                    return result;
          }
    }

    if (optind < argc)
    {
        while (optind < argc)
        {
            if (result.server != "")
                result.ecode = ECLWRONG;
            result.server = argv[optind];
            optind++;
        }

    }

    if (result.server == "")
        result.ecode = ECLWRONG;

    if (result.rate && !result.interval)
        result.interval = 100/result.rate;

    if (!result.rate)
        result.RATE = "10";
    if (!result.sizes)
        result.SIZES = "100";

    return result;

}

/**
 * Vytvori soket socket
 * @param *soket deskriptor soketu
 * @return EOK pri vytvoreni soketu, chybu pri nevytvoreni
 */
 int getSocket(int *soket)
 {
     if ((*soket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
         return ESOCKET; //chyba soket se nevztvoril
     return EOK;
 }
 /**
  * Posila pakety na server
  * @param *soket deskriptor soketu
  *  @return EOK pokud bude vse v poradku
  */
 int sending(int soket, struct sockaddr_in server_addr, PARAMS *params, S_SEND *sends, struct timeval *time_s)
 {
    string numer;
    ostringstream convert;
    struct timeval tv;
    double sec, umils;

    gettimeofday(&tv, NULL); //zacatek testovani
    time_t sec1 = tv.tv_sec;
    suseconds_t umils1 = tv.tv_usec;

    int pakets = 1;
    string server_message;
    int interval = 0;
    double all = 0.0;
    while(true)
    {
        //Priprava zpravy, cislo paketu
        convert << sends->paket;
        numer = convert.str();
        convert.str("");
        server_message = "N. " + numer + " ";
        server_message.resize(params->s, '-');

        //casove razitko se nebude posilat, ale lokalne ukladat, snad to bude fungovat
        gettimeofday(&tv, NULL);
        sec = tv.tv_sec - sec1;
        umils = tv.tv_usec - umils1;
        sec = sec + (umils / 1000000);

        if (sec <= 1.0 && pakets <= params->r) //odeslat x pakets/s
        {
            if (sendto(soket, server_message.c_str(), server_message.length(), 0, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) //poslat zpravu, vrati se delka zpravy
                return ESEND;
            pakets++;
            time_s[sends->paket].tv_sec = tv.tv_sec;
            time_s[sends->paket].tv_usec = tv.tv_usec;
            sends->paket++;

        }
        else
        {
            if (sec < 1.0) //pokud se to vsechno stihlo poslat ya mene nez sekundu, tak musime pockat na dalsi interval
                continue;
            else //priprava na dalsi interval
            {
                pakets = 1; //vynulovat pro dalsi sekundu
                sec = 0;
                interval++;
                if (interval == params->i) //interval, nejspis se budou vyskztovat mensi odchylky
                    break;
                gettimeofday(&tv, NULL); //novy cas pro interval
                sec1 = tv.tv_sec;
                umils1 = tv.tv_usec;

            }
        }
    }

    return EOK;
 }

/**
 *  Cte pakety ktere zasila server
 *  @param *soket deskriptor soketu
 *  @return EOK pokud bude vse v poradku
 */
 int receving(int soket, S_RECV *recvs, PARAMS *params, struct timeval *time_r)
 {
    struct sockaddr_in from_addr;
    char serverMes[MAXMES];
     memset(&serverMes, 0, sizeof(serverMes));
    socklen_t fromSize = sizeof(from_addr);
    vector<string> words;
    string word;

    int paket_number;
    int pre_num = 0;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    double sec, umils;

    while (true)
    {
        words.clear();
        if ((recvfrom(soket, serverMes, MAXMES, 0, (struct sockaddr *) &from_addr, &fromSize)) < 0) //poslat zpravu, vrati se delka zpravy
            return ERECV;

        gettimeofday(&tv, NULL); //cas kdz to prislo
        recvs->paket++; //pocet prijatych paketu;

        //dostat z toho jake je cislo paketu
        string endms(serverMes);
        stringstream ss(endms);
        while(ss >> word)
            words.push_back(word); //cislo paketu bude ve words[1]

        //dostat cislo, zkountrolovat yda neni out of order
        paket_number = atoi(words[1].c_str());
        if (paket_number < pre_num)
           recvs->ooorder++;
        pre_num = paket_number;

        time_r[paket_number].tv_sec = tv.tv_sec;
        time_r[paket_number].tv_usec = tv.tv_usec;


    }

    return EOK;
 }

void The_END(int sig)
{
    cout << "\nKlient konci se signalem: " << sig << endl;
    shmctl (sh_id, IPC_RMID, NULL);
    shmctl (sh_idrecv, IPC_RMID, NULL);
    shmctl (sh_idsp, IPC_RMID, NULL);
    shmctl (sh_idrecvp, IPC_RMID, NULL);

    exit(0);
}

 /**
  * Konfigurace spojeni a navazani spojeni s serverem
  * @param soket deskriptor soketu
  * @param params parametry prikazove radky
  * @return EOK po bezchybnem navazani spojeni jinak nejaka chyba
  */
int configConnect(int soket, PARAMS *params, S_SEND *sends, S_RECV *recvs, struct timeval *time_s, struct timeval *time_r)
{

    struct sockaddr_in dest_addr; //pro cilovou ardresu, pro adresu serveru
    struct hostent *dns_addr; //pro DNS prekla

    memset(&dest_addr, 0, sizeof(sockaddr_in)); //vynuluje pamet

    if ((dns_addr = gethostbyname(params->server.c_str())) == NULL) //prevede jmeno serveru na adresu, NEJAK!, pri chybe NULL
        return EDNS;

    memcpy(&dest_addr.sin_addr, dns_addr->h_addr, dns_addr->h_length); // nahraje adresu serveru do struktury

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(params->p); //prevod portu na sitovy tvat

    int navrat = EOK;
    pid_t client_send;
    pid_t client_recv;

    struct timeval tv;
    gettimeofday(&tv, NULL); //zacatek testovani
    time_t sec1 = tv.tv_sec;
    suseconds_t umils1 = tv.tv_usec;
    double sec2 = 0.0;
    double H_umils;


    signal(SIGINT, The_END);

while (true) // defaultne nekonecko, z parametru to bude -1
{
    if (params->t!= -1){
        gettimeofday(&tv, NULL); //zacatek testovani

        sec2 = tv.tv_sec - sec1;
        H_umils = tv.tv_usec - umils1;

        sec2 = sec2 + (H_umils / 1000000);

        if (sec2 > params->t)
            break;
    }
    if ((client_send = fork()) < 0)
        return EFORK;
    if (client_send == 0) //POTOMEK pro odesilani
    {
        navrat = sending(soket, dest_addr, params, sends, time_s);
        exit(navrat);
    }
    else //HLAVNI RODIC
    {
        if((client_recv = fork()) < 0)
            return EFORK;
        if(client_recv == 0) //POTOMEK pro prijem
        {
           navrat = receving(soket, recvs, params, time_r);
           exit(navrat);
        }
        else //RODIC PRO PRIJEM
        {
            sleep(params->i);  //zadne cekani na zkozcene

            kill(client_recv, SIGKILL);
            while (wait(NULL) > 0);
        }
        time_t now = time(0);

        while (wait(NULL) > 0);

        vector<double> msec;
        vector<double> jitter;
        double sec, umils;
        double RTT = 0;
        double minA = 0;
        double max_J = 0;

        for(int i = 0; i < sends->paket; i++)
        {
            if (time_r[i].tv_sec == 0)//paket se asi ytratil
                continue;
            sec = time_r[i].tv_sec - time_s[i].tv_sec;
            umils = time_r[i].tv_usec - time_s[i].tv_usec;
            sec = sec + (umils / 1000000); // vysledek je v sekundach
            sec = sec * 1000; //prevod na milisekundy
            msec.push_back(sec);
        }

        //RTT
        for (int i = 0; i < msec.size(); i++)
        {
            RTT += msec[i];
            if(i == 0)//poprve
            {
                minA = msec[0];
            }
            else
            {
                if (msec[i] < minA)
                    minA = msec[i];
            }
        }
        RTT /= msec.size();

        //JITTER
        for(int i = 0; i < msec.size(); i++)
            jitter.push_back(msec[i] - minA);

        for(int i = 0; i < jitter.size(); i++)//hledani maxima
        {
            if (i == 0)//prvni
                max_J = jitter[i];
            else
            {
                if(jitter[i] > max_J)
                    max_J = jitter[i];
            }
        }
        struct tm * timeinfo;
        timeinfo = localtime(&now);
        char buff[80];
        strftime (buff,80,"%F-%X",timeinfo);
        ofstream file;
        string name;
        name = "ipkperf-" + params->server + "-" + params->SIZES + "-" + params->RATE;
        file.open(name.c_str(), ios_base::app);
        file << buff << "," << params->i << "," << sends->paket << "," << recvs->paket << ",";
        file << sends->paket * params->s << "," << recvs->paket * params->s << ",";
        file.setf(ios::fixed,ios::floatfield);
        file.precision(3);
        file << RTT << "," << max_J << "," << recvs->ooorder << endl;

        sends->paket = recvs->ooorder = recvs->paket = 0;
        for (int i = 0; i < params->i * params->r; i++)
            time_r[i].tv_sec = time_s[i].tv_sec = time_r[i].tv_usec = time_s[i].tv_usec = 0;
    }
}
    return navrat;

}

void uklid_pamet_send(int sh_id, S_SEND *p_memory)
{
    shmdt (p_memory);
    shmctl (sh_id, IPC_RMID, NULL);
}

void uklid_pamet_recv(int sh_id, S_RECV *p_memory)
{
    shmdt (p_memory);
    shmctl (sh_id, IPC_RMID, NULL);
}
void uklid_pamet_time_s(int sh_id, struct timeval *p_memory)
{
    shmdt (p_memory);
    shmctl (sh_id, IPC_RMID, NULL);
}

void uklid_pamet_time_r(int sh_id, struct timeval *p_memory)
{
    shmdt (p_memory);
    shmctl (sh_id, IPC_RMID, NULL);
}


int main(int argc, char *argv[])
{
    //parametry
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
/***********************************************************************************************************/
    //sdilene promenne, prevzato z meho projektu do IOS, prekvapive to funguje
    S_SEND *sends;
    S_RECV *recvs;
    struct timeval *time_s;
    struct timeval *time_r;
////////////////////////////////////////////////////////////////////
    // 1. klic pro sdilenou pamet
    key_t sh_key = ftok("/tmp", 4);
    if (sh_key == (key_t)-1)
    {
        printECode(ESHM);
        return EXIT_FAILURE;
    }

    key_t sh_keyrecv = ftok("/tmp", 5);
    if (sh_keyrecv == (key_t)-1)
    {
        printECode(ESHM);
        return EXIT_FAILURE;
    }
    key_t sh_keysp = ftok("/tmp", 6);
    if (sh_keysp == (key_t)-1)
    {
        printECode(ESHM);
        return EXIT_FAILURE;
    }

    key_t sh_keyrecvp = ftok("/tmp", 7);
    if (sh_keyrecvp == (key_t)-1)
    {
        printECode(ESHM);
        return EXIT_FAILURE;
    }
////////////////////////////////////////////////////////////////////////////
    //2. alokace sdilene pameti
    sh_id = shmget(sh_key, sizeof(S_SEND), IPC_CREAT | 0666);
    if (sh_id < 0)
    {
        printECode(ESHM);
        return EXIT_FAILURE;
    }
    sh_idrecv = shmget(sh_keyrecv, sizeof(S_RECV), IPC_CREAT | 0666);
    if (sh_idrecv < 0)
    {
        printECode(ESHM);
        return EXIT_FAILURE;
    }
    sh_idsp = shmget(sh_keysp, params.r * params.i * sizeof(struct timeval), IPC_CREAT | 0666);
    if (sh_idsp < 0)
    {
        printECode(ESHM);
        return EXIT_FAILURE;
    }
    sh_idrecvp = shmget(sh_keyrecvp, params.r * params.i * sizeof(struct timeval), IPC_CREAT | 0666);
    if (sh_idrecvp < 0)
    {
        printECode(ESHM);
        return EXIT_FAILURE;
    }
///////////////////////////////////////////////////////////////////////////////////
    //3. pripojeni ke sdilene pameti, rodic pripojen == decka pripojena
    sends = (S_SEND *)shmat(sh_id, NULL, 0);
    if (sends == (void *) -1)
    {
        uklid_pamet_send(sh_id, NULL);
        printECode(ESHM);
        return EXIT_FAILURE;
    }

    recvs = (S_RECV *)shmat(sh_idrecv, NULL, 0);
    if (recvs == (void *) -1)
    {
        uklid_pamet_recv(sh_idrecv, NULL);
        uklid_pamet_send(sh_id, sends);
        printECode(ESHM);
        return EXIT_FAILURE;
    }
    time_s = (struct timeval *)shmat(sh_idsp, NULL, 0);
    if (sends == (void *) -1)
    {
        uklid_pamet_time_s(sh_idsp, NULL);
        uklid_pamet_recv(sh_idrecv, recvs);
        uklid_pamet_send(sh_id, sends);
        printECode(ESHM);
        return EXIT_FAILURE;
    }

    time_r = (struct timeval *)shmat(sh_idrecvp, NULL, 0);
    if (recvs == (void *) -1)
    {
        uklid_pamet_time_r(sh_idrecvp, NULL);
        uklid_pamet_time_s(sh_idsp, time_s);
        uklid_pamet_recv(sh_idrecv, recvs);
        uklid_pamet_send(sh_id, sends);
        printECode(ESHM);
        return EXIT_FAILURE;
    }

    sends->paket = 0;
    recvs->paket = recvs->ooorder = 0;
    for (int i = 0; i < params.i * params.r; i++)
        time_r[i].tv_sec = time_s[i].tv_sec = time_r[i].tv_usec = time_s[i].tv_usec = 0;
/************************************************************************************************************/

    navrat = configConnect(soket, &params, sends, recvs, time_s, time_r);
    if (navrat != EOK)
    {
        printECode(navrat);
        close(soket);
        uklid_pamet_send(sh_id, sends);
        uklid_pamet_recv(sh_idrecv, recvs);
        return EXIT_FAILURE;
    }

    close(soket);
    uklid_pamet_send(sh_id, sends);
    uklid_pamet_recv(sh_idrecv, recvs);
    uklid_pamet_time_s(sh_idsp, time_s);
    uklid_pamet_time_r(sh_idrecvp, time_r);

    return 0;
}
