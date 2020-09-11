# VUT FIT IPK

## Projekt 1

Program pro vyhledání informací o uživatelích Unixového OS.

Vytvořte program pro klienta a server, s využitím rozhraní schránek (sockets API), který implementuje jednoduchou adresářovou službu. 
- navrhněte aplikační protokol pro komunikaci mezi klientem a serverem
- vytvořte oba programy v jazyce C/C++ přeložitelné na studentském unixovém serveru eva.fit.vutbr.cz (FreeBSD) včetně funkčního Makefile souboru (oba programy přeložitelné po zadání příkazu make)
- vytvořte dokumentaci popisující aplikační protokol (max. 1 strana A4, formát pdf)

Programy využívají spojovanou službu (protokol TCP). Server musí být konkurentní tzn. bude požadována současná obsluha více klientů (implementujte pomocí procesů nebo vláken). Jméno programu pro server po přeložení bude server a pro klienta client. Server předpokládá jeden povinný parametry: -p následovaný číslem portu, na kterém bude očekávat spojení od klienta (příklad spuštění serveru: server -p 10000 &). Klient předpokládá alespoň tři povinné parametry, jméno serveru a číslo portu a jedním parametrem pro vyhledání (-l nebo -u). Protokol síťové vrstvy použijte IPv4. Oznámení o chybách, které mohou nastat (neznámé uživatelské jméno, neznámý login, chybný formát zdrojové databáze, chyba při komunikaci, ...), bude vytištěno na standardní chybový výstup (stderr). Data přenášená mezi klientem a serverem jsou pouze ta, která jsou požadována klientem (nepřenášejte celé záznamy). Tyto vlastnosti navrhněte a implementujte vhodným aplikačním protokolem.

Klient získává od serveru požadované informace o uživatelích OS Unix, na kterém je spuštěn server. Tyto informace server získává z /etc/passwd souboru. Každý záznam v souboru je na samostatném řádku, kde jednotlivé položky jsou odděleny : (podrobný formát je uveden v manuálové stránce passwd v sekci 5, viz: man 5 passwd). Získané informace (záznamy) klient vypíše na standardní výstup na jednotlivé řádky v pořadí, jaké je uvedeno u klienta. Jednotlivé položky ve výstupu oddělte mezerou. Položky, které klient může požadovat, jsou reprezentovány následujícími parametry:
- -L uživatelské jméno
- -U UID
- -G GID
- -N celé jméno, celý gecos pokud obsahuje další položky oddělené čárkou
- -H domovský adresář
- -S logovací shell
Vyhledávejte pouze podle jednoho kritéria, uživatelského jména nebo UIDu (podle posledního parametru -l nebo -u). Vyhledávání nerozlišuje malé a velké znaky. U výpisu uvažujte, že se daná položka může objevit maximálně jednou.

## Projekt 2

Vytvořte jednoduchý tester, pro zjištění a sledování parametrů end-to-end komunikace. Tato aplikace se bude sestávat z klientské části a serverové části. Tento tester bude používat UDP pro komunikaci. V rámci implementace budete muset navrhnout vhodnou strukturu komunikačního protokolu zahrnující například číslování datových bloků a časová razítka.

Server bude pouze odpovídat na měřící informace ze strany klientské části. Klient bude zasílat data dle specifikace na server a měřit odpověď na takto zaslaná data. Charakteristiku zasílaných dat bude možné specifikovat.

### SERVER 

Spuštění serveru bude následující: 
- ./ipkperfserver [-p port]
Parametry:
- -p --port UDP port, na kterém bude server naslouchat (implicitně 5656) 
Server se korektně vypne po obdržení signálu SIGINT.

### KLIENT
Spuštění klienta:   
- ./ipkperfclient [-p server_port] [-s packet_size] [-r packet_rate] [-t runtime] [-i interval] SERVER
Parametry:
- -p --port: UDP port serveru
- -s --size: velikost payload paketu, obsah paketu může být fixní nebo náhodně vygenerován, implicitní hodnota je 100 bajtů, minimální hodnota je 10 bajtů. Pozor zadaná hodnota odpovídá velikosti obsahu UDP payload paketu.   
- -r --rate: rychlost odesílání v packet/s, implicitní hodnota je 10
- -t --timeout: doba běhu klienta [s], není-li zadána pak bude klient běžet nekončeně dlouho
- -i --interval: interval (viz vysvětlení níže), implicitní hodnota = 100/rate.
- SERVER jméno nebo adresa serveru

Klient bude provádět měření po určenou dobu nebo do ukončení pomocí SIGINT. 
Klient bude ukládat výsledky do souboru "ipkperf-SERVERIP-SIZE-RATE". Bude-li uvedený soubor již existovat, nová data k němu připojí (append).
Výsledky budou uloženy v csv formátu takto:
Time, IntLen, Pckt Sent, Pckt Recv, Bytes Send, Bytes Recv, Avg Delay, Jitter, OutOfOrder
kde:
- Time - označuje začátek měřeného intervalu (formát YYYY-MM-DD-HH:MM:SS)
- IntLen - označuje délku každého měřeného intervalu (nastaveného parametrem -i/--interval)
- Pckt Sent - označuje množství odeslaných paketů v tomto intervalu
- Pckt Recv - označuje množství paketů přijatých v tomto intervalu
- Bytes Send - označuje množství dat odeslaných (v bytech)
- Bytes Recv - označuje množství přijatých dat (v bytech)
- Avg Delay - pro přijaté pakety v tomto intervalu spočítá jejich průměrný RTT
- Jitter - pro přijaté pakety v tomto intervalu spočítá maximální hodnota Packet Delay Variation (způsob viz níže). 
- OutOfOrder - pro přijaté pakety v tomto intervalu spočítá počet paketů v nesprávném pořadí / počet paketů celkem (v %) 

Podobně jako nástroje ping či traceroute, RTT uvádějte v ms s přesností na 3 desetinná místa.
Pro výpočet statistik uvažujte velikost "aplikačních" dat, tedy nemusíte počítat velikost hlaviček použitých protokolů.
 
Packet Delay Variation
Je hodnota, které vyjadřuje odchylku ve zpoždění paketu oproti definované nominální hodnotě:
 
PDV(x) = t(x) - A(x)
 
kde t(x) je doba přenosu paketu x a A(x) je nominální hodnota doby přenosu. Pro výpočet uvažujte, že A(x) je nejmenší doba přenosu paketů v rámci měřeného intervalu. 
 
Pro výpočet rozptylu použijte následující vztah:
 
Jitter = MAX_i[PDV(x_i)], pro všechny pakety x_1..x_n z měřeného intervalu. 
 
Více informací o PDV, viz RFC5481.  
 
### IMPLEMENTACE
Implementační jazyk C/C++, Python, nebo Perl. 

Aplikace bude povinně obsahovat README s popisem implementace a příklady spuštění. Pokud nestihnete implementovat část funkcionality, bude to taktéž povinně zmíněno v README.

Přeložení aplikace zajistí Makefile, který při překladu stáhne požadované schválené knihovny a zajistí překlad bez manuálního zásahu uživatele. Překlad musí být úspěšný bez chyb a varování! Makefile bude dále povinně implementovat PHONY clean, test a pack s následující funkcionalitou:

- make clean - smaže vše kromě zdrojových a testovacích souborů
- make test - spustí navrženou sadu testů a vypíše výsledek (i postup) na STDOUT
- make pack - vytvoří archiv k odevzdání
Je vyžadováno, aby sada testů pokrývala kompletně implementovanou funkcionalitu. 
