
/* Subor:  proj1.c
 * Datum:   2010/10/14
 * Autor:   Pavol Loffay xloffa00@stu.fit.vutbr.cz
 * Projekt: Jednoducha kompesia textu, Projekt 1. do predmetu IZP
 * Popis:   Program spracuvava text z standardneho vstupu stdin a vracia ho na standardni vystup stdout.
Pri zvoleni kompresia textu, program bude hladat za sebou iduce zhodne bloky textu s dlzkou N. Ak najde vypise pocet kolko krat sa opakoval blok a vypise aj blok, pritom pocet opakovani musi byt mensi ako 10;
Pri dekompresii textu program spracuvava znak po znaku a ked najde CISLO a tak vypise CISLO krat blok textu dlzky N za sebou.
 */

#include <stdio.h>
#include <stdlib.h>
// koli funkcii strcmp()
#include <string.h>
// koli funkcii isdigit() isalpha() ispunct() isspace()
#include <ctype.h>


/** Kódy chyb programu */
enum tecodes
{
	EOK = 0,			// BEZ CHYBY
	ECLWRONG,		// CHYBNY PRIKAZOVY RIADOK
	ESTDINWRONG,	//CHYBA NA VSTUPE,, BOL ZADANY ILEGALNY ZNAK
   ESTDINWRONGDEC, // PRI DEKOMPRIMACII BOL ZADANY ZLY FORMAT TEXTU
   EMALLOC,         // NEPODARILO SA ALOKOVAT DOSTATOCNE MNOZSTVO PAMETE
	EUNKNOWN,		// NEZNAMA CHYBA
};

/** Stavové kódy programu */
enum tstates
{
	CHELP,		//NAPOVEDA
	CCOMP,		//KOMPRIMACIA
	CDECOMP,		//DEKOMPRIMACIA
};

// LIMIT sluzi na porovnanie N ci sa rovna unsigned INT.
const unsigned int LIMITN = 65535;

/** CHYBOVE HLASENIA ODPOVEDAJUCE CHYBOVIM KODOM */
const char *ECODEMSG[] =
{
// EOK
  "Vsetko v poriadku.\n",
// ECLWRONG
  "Chybne parametry prikazoveho riadku!\n",
// ESTDINWRONG
	"Bol zadany ilegalny znak!\n",
//    ESTDINWRONGDECOMPRESS
   "Bol zadany zly text na dekomprimovanie!\n",
// EMALLOC
    "Nepodaril sa malloc programbol ukonceny!\n",
//  EUNKNOWN
	"Neznama chyba!\n"
};

/** HLASENIE HELP*/
const char *HELPMSG =
  "Program jednoducha kompresia textu\n"
  "Autor: Pavol Loffay 1BIT kruzok 30.\n"
  "Program funguje takto...\n"
  "Pouzitie, v prikazovom riadku:\n"
  ":        $ ./proj1 -h    :program vypise tuto napovedu\n"
  "         $ ./proj1 -c N  :program komprimuje vstupny text\n"
  " tak ze hlada rovnake bloky textu iduce za sebou o dlzke N znakov\n"
  " ak najde vypise text v formate cislo XblokTextu.\n"
  "         $ ./proj1 -d N  : dekomprimuje vstupny text, ak najde cislo,\n"
  " tak vypise blok textu dlzky N Xkrat za sebou, cislo X naslo v texte.\n"
  "N musi byt z intervalu < 1 , 9 >\n"
;

/**
 * Struktura OBSAHUJUCA HODNOTY PRIKAZOVEHO RIADKU
 */
typedef struct params
{
  unsigned int N;   // HODNOTA N Z PRIKAZOVEHO RIADKU
  int ecode;        // CHYBOVY KOD PROGRAMU ODPOVEDA VYCTY TECODES
  int state;        // STAVOVY KOD PROGRAMU, ODPOVEDA VYCTU TSTATES
} TParams;

/**
 * VYTLACI HLASENIE ZODPOVEDAJUCEJ CHYBE
 */
void printECode(int ecode)
{
  if (ecode < EOK || ecode > EUNKNOWN)
  { ecode = EUNKNOWN; }

  fprintf(stderr, "%s", ECODEMSG[ecode]);
}

/**
 * Funkcia zisti ci v danom retazci su ulozene iba cisla
 * ak ano vrati 1, inak 0
 * @param *argv ukazatel na retazec *
 */
int isnumber(char *argv)
{
    unsigned int i = 0;
    while ( i < strlen(argv) && isdigit(argv[i]))
        i++;
    if (i == strlen(argv))
        return 1;
    return 0;
}

/**
 * Funkcia zisti ci je dany retazec zhodny s textom "-h"
 * ak ano vrati 1, inak 0.
 * @param *argv ukazatel na dany retazec, ktory chceme testovat
 */
int argumentmHelp(char *argv)
{
    if (strcmp(argv, "-h") == 0)
        return 1;
    return 0;
}

/**
 * Fukcia vrati 1 ak sa v jednom z poli bude nachadzat "-c"
 * a v druhom bude ulozene kladne cislo v rozsahu unsigned int
 * inak vrati 0
 * @param *argv[] ukazatel na pole ukazatelov *
 */
int argumentCompress(char *argv[])
{
    if (strcmp(argv[1], "-c") == 0 && isnumber(argv[2]))
    {
        if ((unsigned int) atoi(argv[2]) <= LIMITN)
            return 1;
    }
     return 0;
}

/**
 * Fukcia vrati 1 ak sa v jednom z poli bude nachadzat "-d"
 * a v druhom bude ulozene kladne cislo v rozsahu unsigned int
 * inak vrati 0
 * @param *argv[] ukazatel na pole ukazatelov
 */
int argumentDecompress(char *argv[])
{
    if (strcmp(argv[1], "-d") == 0 && isnumber(argv[2]))
        if ((unsigned int) atoi(argv[2]) <= LIMITN)
            return 1;
     return 0;
}

/**
 * Funkcia sluzi na spracovanie argumentov z prikazoveho riadku
 * Funkcia nam vrati strukturu TParams
 * @param argc premenna ktora udava kolko argumentov sme zadali
 * na prikazovom riadku
 * @param *argv[] ukazatel na pole ukazatelov
 */
TParams getParams(int argc, char *argv[])
{
// INICIALIZACIA STRUKTURY
	TParams result =
	{
		.N = 0,
		.ecode = EOK,
		.state = CCOMP,
	};
   // ak sme zadali na prikazovom riadku jeden argument
	if (argc == 2 && argumentmHelp(argv[1]))
		result.state = CHELP;
    else //ak sme zadali na prikazovom riadku 2 argumenty
        if (argc == 3)
        {
            if (argumentCompress(argv))
            {
                result.state = CCOMP;
                result.N = atoi(argv[2]);// druhy argument prevedie na cislo
            }
            else
                if (argumentDecompress(argv))
                {
                    result.state = CDECOMP;
                    result.N = atoi(argv[2]);//druhy argument prevedie na cislo
                }
                else
                    result.ecode = ECLWRONG;
        }
        else
            result.ecode = ECLWRONG;
    return result;
}

/**
 * Funkcia otestuje ci vstupny parameter je tisknutelny znakov
 * Funkcia vrati 1 ak znak splnuje podmienky, 0 ak nie.
 * @param c vstupny parameter
 */
int printC(int c)
{
    if (isalpha(c) || ispunct(c) || isspace(c))
        return 1;
    return 0;
}

/**
 * Tato fukcia patri ku funkcii decompress() - stav 1
 * cita znaky a hhed ich tlaci pokial prejdu podmienkou tlacenia
 * vracia 0 ak narazi na koniec suboru alebo ilegalny znak
 * vracia 1 ak narazi v texte na cislo
 */
int stav1()
{
    int c;
    while ((c = getchar()) != EOF)
    {
        if (isdigit(c))
            return (c - '0');
        else
            if (printC(c))
                putchar(c);
            else
            {
                printECode(ESTDINWRONG);
                return 0;
            }
    }
   return 0;
}

/**
 * Funkcia patri k funkcii decompress() - stav2
 * Funkcia nacita blok textu dlzky N a vypise ho opakovat krat
 * vrati 1 ak vypisal cely blok,
 * vrati 0 ak isiel vytlacit netisknutelny znak, alebo narazil na EOF.
 * @param opakovat urcuje kolko krat sa ma vypisat dany blok
 * @param N urcuje aky dlhy blok textu ma nacitat *
 */
int stav2(int opakovat, unsigned int N)
{
    unsigned int pozicia = 0;
    char *blok;
    blok = (char*)malloc((N+1) * sizeof(char));
    if (blok == NULL)
    {
        printECode(EMALLOC);
        return 0;
    }
//     nacitavanie bloku textu ak nrazi na EOF konci
    for (unsigned int i = 0; i < N; i++, pozicia++)
        if ((blok[i] = getchar()) == EOF)
            break;
//         opakovat krat vypise nacitany blok textu ak splni podmienky
    for (int j = 0; j < opakovat; j++)
    {
        for (unsigned int i = 0; i < pozicia; i++)
            if (printC(blok[i]))
                putchar(blok[i]);
            else
            {
               printECode(ESTDINWRONG);
					free(blok);
                return 0;
            }
    }
    if (blok[pozicia] == EOF)
    {
        printECode(ESTDINWRONGDEC);
		  free(blok);
        return 0;
    }
	 free(blok);
    return 1;
}
/**
 * Funkcia dekomprimuje vstupny text, zacne v funkcii stav1
 * kde cita znaky a hned vypisuje ak nenarazi na cislo ak
 * narazi na cislo tak vrati cislo a program pokracuje v fukcii stav2 *
 * vracia 1 ak narazil na koniec suboru alebo po vytlacanej
 * chybovej hlaske
 * @param N urcuje ake dlhe bloky textu sa budu vypisovat
 */
int decompress(unsigned int N)
{
    int cyklus = 1;
    int number = 0;
    while (cyklus)
    {// ma zaciatk usplna tutu podmienkou
        if (!number)
        {
            number = stav1();
            if (number == 0)
                cyklus = 0;
        }
        else
        {
            if (stav2(number, N) == 0)
                cyklus = 0;
            number = 0;
        }
    }
    return 1;
}

/**
 * Funkcia patri k funkcii compress
 * fukcia nacita do retazca znaky do pozicie position
 * vrati retazec ak nenarazil na koniec suboru
 * vrati NULL ak narazil na koniec suboru alebo
 * sa mal vytlacit ilegalny znak
 * @param *str vstupny retazec do ktore sa bude zapisovat
 * @param position udava od ktoreho indexu sa budu zapisovat znaky
 * @param N je argument z prikazoveh oriadku udava dlzku retazca
 */
char *citam(char *str, unsigned int position, unsigned int N)
{
    while ((position < N)  &&  (str[position] = getchar()) != EOF)
        position++;

    if (position == N)
        return str;

    else   // ak narazil na koniec suboru tak vytlaci znaky v poli a vrati NULL
        for (unsigned int j = 0; j < position; j++)
        {
            if (printC(str[j]))
                putchar(str[j]);
            else
            {
                printECode(ESTDINWRONG);
                break;
            }
        }
    return NULL;
}

/**
 * Funkcia patri k funkcii compress
 * Prekopiruje znaky z blok2 do blok, pricom vzdy z blok2 od pozicie 0
 * do pozicie position2
 * vracia ukazatel na pole blok
 * @param *blok pole do ktoreho sa bude zapisovat od pozicie position
 * @param *blok2 pole z ktoreho sa budu znaky citat do pozicie 0 do position2
 * @param position udava index od ktoreha sa budu znaky zapisovat do blok
 * @param position2 udava index do ktoreho sa budu znaky citat v poli blok2
 */
// prekopiruje znaky z blok2 od nuly do indexu 0, do position2, do blok od pozicie,
char *coppyBlok(char *blok, char *blok2, unsigned int position, unsigned int position2)
{
    unsigned int x = 0;
    for ( ; x < position2; position++, x++)
        blok[position] = blok2[x];
    return blok;
}

/**
 * Funkcia afrerEOF patri k fukcii compress vola sa z porovnavamBlok
 * Sluzi na dovypisovanie znakov ktore ostali v blok a blok2
 * @param blok blok textu ktory sa bude vypisovat
 * @param blok2 blok textu ktory sa bude vypisovat
 * @param position pomocna premenna na indexovanie vypisu
 * @param pocet ak sa opakoval blok textu urcuje kolko krat sa ma vypisat
 * @param N udava hodnotu argumentu precitanu z prikazoveho riadku aky dlhy ma byt blok textu
 * return 0: ak sa mal tlacit ilegalny znakov
 * return 1: ak nenarazil na ilegalny znak;
 */

int afterEOF(char *blok, char *blok2, unsigned int position, int pocet, unsigned int N)
{
    if (pocet >= '2')
    {
        putchar(pocet);
        for (unsigned int i = 0; i < N; i++)
            if (printC(blok[i]))
                putchar(blok[i]);
             else
             {
                printECode(ESTDINWRONG);
                return 0;
             }
        if (position) // ??
            for (unsigned int i = 0; i < position; i++)
                if (printC(blok2[i]))
                    putchar(blok[i]);
                else
                {
                    printECode(ESTDINWRONG);
                    return 0;
                }
    }
    else
    {
        for (unsigned int i = 0; i < N; i++)
            if (printC(blok[i]))
                putchar(blok[i]);
            else
            {
                printECode(ESTDINWRONG);
                return 0;
             }
        for (unsigned int i = 0; i < position; i++)
            if (printC(blok[i]))
                putchar(blok[i]);
             else
             {
                printECode(ESTDINWRONG);
                return 0;
             }
    }
    return 1;
}

/**
 * Funkcia patri k funkcii compress vola sa z porovnavamBlok
 * vola sa ak sa aktualny znak nerovnal s znakom v bloku, a blok textu sa
 * opakovat v texte pocet krat.
 * Funkcia vypise pred blok opakovaneho textu kolko krat sa opakoval
 * a nasledne vypise blok textu
 * vrati NULL ak sa ma vytlacit ilegalny znkak
 * vrati blok naplneny novimy znakmi, ak sa nevytlacil ilegalny znak
 * alebo nebol koniec suboru
 * @param *blok pole znakov
 * @param *blok2 nahradne pole znakov kde mame zalozne znaky
 * @param c aktualny znak z porovnavamBlok
 * @param position udava aktualny index v ktorom sme v poli blok
 * @param pocet udava pocet kolko krat sa opakoval blok textu v vstupnom texte
 * @param N udava hodnotu argumentu z prikazoveho riadku.
 */
char *prebeholPocet(char *blok, char *blok2, int c, unsigned int position, int pocet, unsigned int N)
{
    putchar(pocet);
   //vypis bloku textu
    for (unsigned int j = 0; j < N; j++)
    if (printC(blok[j]))
        putchar(blok[j]);
    else
    {// ak bol zadany ilegalny znak tak vypise chybu a ukonci
        printECode(ESTDINWRONG);
        return NULL;
    }// ak sa nerovnal ani jeden znak na vstupe z znakom v bloku
    if (position == 0)
    {
       blok[position] = c; //nacita aktualny znak
       position++;// do blok docita znaky aby sa zaplnilo pole
       if ((blok = citam(blok, position, N)) == NULL)
           return NULL;
    }
    else
    {// ak sa rovnali znaky na vstupe s znakmi v bok
//     prekopirujem znaky ulozene v blok2 do blok
        blok = coppyBlok(blok, blok2, 0, position);
        blok[position] = c;
        position++;
        if (position != N) //ak niesom na poslednej pozicii docitam znaky
        if ((blok = citam(blok, position, N)) == NULL)
            return NULL;
    }
    return blok;
}

/**
 * Funkcia patri k funkcii compress vola sa z porovnavam blok
 * fukcia vytlaci znaky z blok po aktualnu poziciu a
 * dopni pole blok znakmi.
 * vracia blok ak sa nemal vytlacit ilegalny znak
 * vracia null ak sa mal vytlacit ilegalny znak
 * @param *blok pole znakov
 * @param *blok2 pole znakov znakov kde mam ulozene znaky
 * @param position udava aktualny idex kde vsme v poli blok
 * @param N udava hodnotu argumentu z prikazoveho riadku.
 */
char *neprebeholPocet(char *blok, char *blok2, int c, unsigned int position, unsigned int N)
{
    unsigned int oldPosition = position;
    unsigned int newPosition;
//     vytlacanie bloku po aktualnu poziciu vcetne
    for (unsigned int j = 0; j <= position; j++)
        if (printC(blok[j]))
            putchar(blok[j]);
        else
        {
            printECode(ESTDINWRONG);
            return NULL;
        }
//                 posun bloku
    position++;  // posunutie znakov v poli blok na nizsie indexi od 0
    for (newPosition = 0; position < N; newPosition++, position++)
        blok[newPosition] = blok[position];
//                 ak sa ziadny precitany znak nerovnal s znakom v blok
    if (oldPosition == 0)
        blok[N-1] = c; // docitanie posledneho znaku v blok
    else
    {
// ak sa niake znaky rovnali z znakmi v blok tak prekopirujem znaky
// ulozne z blok2 do blok
        blok = coppyBlok(blok, blok2, newPosition, oldPosition);
        blok[N-1] = c;
    }
    return blok;
}


/**
 * Funkcia sa vola z compress
 * fukncia cita znaky z vstupu a porovnava ich s znakmi v
 * poli blok a sa znaky zhoduju tak porovnava dalsi znakm
 * pritom aktualne znaky si uklada do blok2. *
 * vrati 0 ak sa vytlacil ilegalny
 * vrati 1 ak sme narazili na EOF
 * @param *blok pole znakov uz naplnene
 * @param N udava hodnotu argumentu z prikazoveho riadku.
 */

int porovnavamBlok(char *blok, unsigned int N)
{
    unsigned int position = 0;
    int c;
    int pocet = '1';
    char *blok2;
    blok2 = (char*)malloc((N+1) * sizeof(char));
     if (blok2 == NULL)
    {
        printECode(EMALLOC);
        return 0;
    }

    while ((c = getchar()) != EOF)
    {// ak sa znak zo vstupu rovna a blok sa opakoval maximane 8 krat.
        if (c == blok[position] && pocet < '9')
        {
            blok2[position] = c;
            position++;
            if (position == N)
            {// ak sme presli cele pole blok cize vsetky znaky sa rovnali
            // mam opakovanie
                pocet++;
                position = 0;
            }
        }// ak sa znaky nerovnali
        else
        { // ak me mali za sebou zhodne bloky textu
            if (pocet >= '2')
            {
                blok = prebeholPocet(blok, blok2, c, position, pocet, N);
                position = 0;
                pocet = '1';
                if (blok == NULL)
                    return 0;
            }
            else // ak sme nemali za sebou zhodne bloky textu
            {
                blok = neprebeholPocet(blok, blok2, c, position, N);
                position = 0;
                if (blok == NULL)
                    return 0;
            }
        }
    }
    // dotlacanie znakov z blok a blok2 po EOF
    if (c == EOF)
        afterEOF(blok, blok2, position, pocet, N);
    free(blok2);
    return 1;
}

/**
 * Funkcia na komprimaciu textu
 * vrati 1 ak me narazili na EOF
 * vrati 0 ak sa mal vytlacit ilegalny znak
 * @param N je to prevziaty argument z prikazoveho riadku
 * udava ake dlhe zhodne bloky textu iduce za sebou sa budu hladat
 */
int compress(unsigned int N)
{
    int vrati;
    unsigned int pozicia = 0;
    char *blok;
    blok = (char*)malloc((N+1) * sizeof(char));
    if (blok == NULL)
    {
        printECode(EMALLOC);
        return 0;
    }
    // nacitam znaky do bloku ak prebehlo v poriadku pokracujem
//     v porovnavani v funkcii provnavamBlok
    if ((blok = citam(blok, pozicia, N)) != NULL)
        vrati = porovnavamBlok(blok, N);

    free(blok);
    return vrati;
}

/**
 * HLAVNY PROGRAM
 */
int main(int argc, char *argv[])
{
// VOLANIE FUNKCIE NA SPRACOVANIE PARAMETROV
	TParams params = getParams(argc, argv);
// AK NASTALA CHYBA VYPISE JU, VOLANIE FUNKCIE printECode
	if (params.ecode != EOK)
	{
		printECode(params.ecode);
		return EXIT_FAILURE;
	}

// AK SME ZADALI ./PROJ1 -h VYPISE NAPOVEDU
	if (params.state == CHELP)
	{
		printf("%s", HELPMSG);
		return EXIT_SUCCESS;
	}
// AK PARAMETRE PRIKAZOVEJ RIADKY URCUJU FUNKCIU DEKOMPRIMACIE ZAVOLA SA
	if (params.state == CDECOMP)
		decompress(params.N);
// AK PARAMETRE PRIKAZOVEJ RIADKY URCUJU FUNKCIU KOMPRIMACIE ZAVOLA SA
	if (params.state == CCOMP)
		compress(params.N);

	return EXIT_SUCCESS;
}
