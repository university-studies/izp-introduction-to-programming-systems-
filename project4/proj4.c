/** Subor:  proj4.c
 * Datum:   2010/12/7
 * Autor:   Pavol Loffay xloffa00@stud.fit.vutbr.cz
 * Projekt: Ceske radenie, Projekt 4. do predmetu IZP
 * Pouzitie, v prikazovom riadku:
 * $ ./proj4 -h  :program vypise tuto napovedu
 * $ ./proj4 --print [stlpec] in out :vytlaci stlpec
 * $ ./proj4 --after [stlpec] [hodnota] --print [stlpec2] in out
 * vypise prvky zo stlpec2, ktore su abecedne po hodnota
 * $ ./proj4 --before [stlpec] [hodnota] --print [stlpec2] in out
 * vypise prvky zo stlpca2, ktore su abecedne pred hodnota
 * Ak sa maju hodnoty, ktore sa idu vypisovat abecedne zoradit treba
 * pridat na miesto XX --print [stpec] XX in out, parameter --sort
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>

//vycetny typ chyb programu
enum tecodes 
{
    EOK,
    ECLWRONG, 
    EMALLOC,
    EFILE,
    EFIELDATA,
};

// vyctovy typ stavu programu
enum tstates
{
    CHELP,
    CPRINT,       //pre sam parameter --print
    CAFTER,       //pre parameter --after [] --print
    CBEFORE,      //pre parameter --before [] --print
    CSORTPRINT,
    CSORTBEFORE,  //pre parameter --before [] --print  --sort
    CSORTAFTER,   //pre parameter --after [] --print  --sort
    COTHER,
};

//vycetny typ pre pristup do tabulky
enum tableISO
{
    PRIMARY,
    SECONDARY,
};

//struktura obshujuca parametre prik. riadku
typedef struct params
{
    int ecode;
    int state;
    char *source;          //zrojovy subor
    char *dest;            //cielovy subor
    char *objectPrint;     //stlpec z ktoreho sa bude tlacit
    char *objectSearch;    //stlpec z ktoreho sa bude porovnavat
    char *objectCompare;   //retazec z ktorym sa bude porovnavat
}TParams;

//struktura obsahujuca 
typedef struct table
{
    int print;    //udava ktory stlpec sa bude tlacit
    int search;   //udava ktory v poradi stlpec sa bude porovnavat
} TTable;

//struktura polozky do zoznamu
typedef struct titem TItem;
struct titem
{
    char *strPrint;     //prvok stlpca ktory sa bude vypisovat
    TItem *next;        //ukazatel na dalsi prvok
};

//struktura obsahujuca ukzatel na prvy prvok zoznamu
typedef struct list 
{
    TItem *first;  //ukzatel na prvy prvok
}TList;

//konstanta udava zakladnu velkost pola
const int VALUE = 64;

// chybove hlasenia programu
const char *ECODEMSG[] =
{
//    EOK
  "Vsetko v poriadku.\n",
//    ECLWRONG
  "Chybne parametry prikazoveho riadku! Pre napovedu -h\n",
//     EMALLOC
  "Nepodarila sa dynamicky alokovat pamet.\n",
//      EFILE
  "Nepodarilo sa otvorit subor / Neexistuje subor.\n",
//   EFILEDATA
  "V subore sa nenachadzaju spravne data.\n",
};

// hlasenia help
const char *HELPMSG =
  "Program Ceske radenie\n"
  "Autor: Pavol Loffay 1BIT kruzok 30.\n"
  "Pouzitie, v prikazovom riadku:\n"
  "$ ./proj4 -h  :program vypise tuto napovedu\n"
  "$ ./proj4 --print [stlpec] in out :vytlaci stlpec\n"
  "$ ./proj4 --after [stlpec] [hodnota] --print [stlpec2] in out\n"
  "vypise prvky zo stlpec2, ktore su abecedne po hodnota\n"
  "$ ./proj4 --before [stlpec] [hodnota] --print [stlpec2] in out\n"
  "vypise prvky zo stlpca2, ktore su abecedne pred hodnota\n"
  "Ak sa maju hodnoty, ktore sa idu vypisovat abecedne zoradit treba\n"
  "pridat na miesto XX --print [stpec] XX in out, parameter --sort\n";
  
  /**
 * fukcia na vypis chyboveho hlasenia
 * @param ecode typ chyby 
 */
void printECode(int ecode)
{
  fprintf(stderr, "%s", ECODEMSG[ecode]);
}

/**
 * funkcia uvolni alokovane miesto pre parametre
 * prikazoveho riadku 
 * @param *params struktura typu TParams
 */
void freeParams(TParams *params)
{
    if (params->source != NULL)
        free(params->source);
    if (params->dest != NULL)
        free(params->dest);
    if (params->objectPrint != NULL)
        free(params->objectPrint);
    if (params->objectSearch != NULL)
        free(params->objectSearch);
    if (params->objectCompare != NULL)
        free(params->objectCompare);
}

/**
 * @param argv obsahuje retazec, ktory sa ulozi do fileName
 * @param fileName ulozi do neho retazec z argv
 * vrati 0 ak sa nepodaril malloc, inak 1
 */
int readStrParam(char **fileName,char **source)
{
    int length = strlen(*source) + 2;
    *fileName = (char *)malloc(length * sizeof(char));
    if (*fileName == NULL)
        return 0;
    strncpy(*fileName, *source, length);
    return 1;
}

/**
 * funkcia patri k funkcii getParams
 * @param argv ukzatel na pole poli znakov.
 * vrati 0 ak sa nepodaril naciatat (nepodaril sa malloc), inak 1
 */
int readParams(char **argv, TParams *result)
{    
    if (readStrParam(&result->objectPrint, &argv[5]) == 0)
        return 0;
    if (readStrParam(&result->objectCompare, &argv[3]) == 0) //porovnavat sa bude Novak
        return 0;
    if (readStrParam(&result->objectSearch, &argv[2]) == 0) //porovnavat sa bude Novak
        return 0;
    return 1;
}

/**
 * Funkcia spracuje argumenty prikazovehoriadku
 * nastavy chybovy kod programu a stavovy 
 * @param argc udava kolko poli znakov(argv) sme zadali
 * @param argv je ukazatel na pole znakov
 * vracia strukturu Tparams
 */
TParams getParams(int argc, char **argv)
{
    TParams result =
    {
        .ecode = EOK,
        .state = CHELP,
        .source = NULL,
        .dest = NULL,
        .objectPrint = NULL,
        .objectSearch = NULL,
        .objectCompare = NULL,
    };
    if (argc == 2)
    {
        if (strcmp(argv[1], "-h") == 0)
            result.state = CHELP;
    }
    else
        if (argc == 5 && strcmp(argv[1], "--print") == 0) //   ./proj4 --print name in out
        {
            if (readStrParam(&result.objectPrint, &argv[2]) == 0)
                result.ecode = EMALLOC;                
            if (readStrParam(&result.source, &argv[3]) == 0)
                result.ecode = EMALLOC;
            result.state = CPRINT;
            if (readStrParam(&result.dest, &argv[4]) == 0)
                result.ecode = EMALLOC;
            result.state = CPRINT;
        }
        
    else
        if (argc == 6 && strcmp(argv[1], "--print") == 0 && strcmp(argv[3], "--sort") == 0)
        {
            if (readStrParam(&result.dest, &argv[5]) == 0)
                result.ecode = EMALLOC;
            if (readStrParam(&result.source, &argv[4]) == 0)
                result.ecode = EMALLOC;
            if (readStrParam(&result.objectPrint, &argv[2]) == 0)
                result.ecode = EMALLOC;
            result.state = CSORTPRINT;
        }
    else
        if (argc == 8) // ./proj4 --before/--after surname Novák --print date in.txt out.txt
        {
            if (strcmp(argv[4], "--print") == 0)
            {
                if (strcmp(argv[1], "--before") == 0 || strcmp(argv[1], "--after") == 0)
                {
                    if (readStrParam(&result.source, &argv[6]) == 0)
                        result.ecode = EMALLOC;
                    if (readStrParam(&result.dest, &argv[7]) == 0)
                        result.ecode = EMALLOC;
                    if (readParams(argv, &result) == 0)
                        result.ecode = EMALLOC;                    
                    if (strcmp(argv[1], "--after") == 0)
                        result.state = CAFTER;
                    else
                        result.state = CBEFORE;
                }
                else 
                    result.ecode = ECLWRONG;
            }
            else
              result.ecode = ECLWRONG;
        }
    else
        if (argc == 9)  // ./proj4 --before/--after surname Novák --print date --sort in.txt out.txt
        {
            if (strcmp(argv[4], "--print") == 0 && strcmp(argv[6], "--sort") == 0)
            {
                if (strcmp(argv[1], "--before") == 0 || strcmp(argv[1], "--after") == 0)
                {
                    if (readStrParam(&result.source, &argv[7]) == 0)
                        result.ecode = EMALLOC;
                    if (readStrParam(&result.dest, &argv[8]) == 0)
                        result.ecode = EMALLOC;
                    if (readParams(argv, &result) == 0)
                        result.ecode = EMALLOC;
                    if (strcmp(argv[1], "--after") == 0)
                        result.state = CSORTAFTER;
                    else
                        result.state = CSORTBEFORE;
                }
                else 
                    result.ecode = ECLWRONG;
            }
            else
                result.ecode = ECLWRONG;
        }
        else
            result.ecode = ECLWRONG;  
              
    //ak je nastaveny chybovy kod uvolnia sa parametre
    if (result.ecode != EOK)
        freeParams(&result);
    return result;
}


const int CH_SIGN = 10;       //znak   ch
const int EXTRA_SIGN = 55;    // znak  //
const int EXTRA_SIGN_C = 74;  //znak   ~~
const int EXTRA_SIGN_D = 75;  //znak   ~=
//primarna tabulka znaky napr. a á Á maju rovnaku vahu
const unsigned char tablePrimary[256] = 
{
    [' '] = 0,
    ['a'] = 1,    ['A'] = 1,     [(unsigned char)'á'] = 1,     [(unsigned char)'Á'] = 1,    [(unsigned char)'ä'] = 1,  
    ['b'] = 2,    ['B'] = 2,
    ['c'] = 3,    ['C'] = 3,
    [(unsigned char)'è'] = 4,    [(unsigned char)'È'] = 4,
    ['d'] = 5,    ['D'] = 5,     [(unsigned char)'ï'] = 5,     [(unsigned char)'Ï'] = 5,
    ['e'] = 6,    ['E'] = 6,     [(unsigned char)'é'] = 6,     [(unsigned char)'É'] = 6,     [(unsigned char)'ì'] = 6, 	  [(unsigned char)'Ì'] = 6,	
    ['f'] = 7,    ['F'] = 7,
    ['g'] = 8,    ['G'] = 8,
    ['h'] = 9,    ['H'] = 9,
//  ['ch'] = 10,  ['CH'] = 10,
    ['i'] = 11,   ['I'] = 11,    [(unsigned char)'í'] = 11,    [(unsigned char)'Í'] = 11,
    ['j'] = 12,   ['J'] = 12,
    ['k'] = 13,   ['K'] = 13,
    ['l'] = 14,   ['L'] = 14,    [(unsigned char)'å'] = 14,    [(unsigned char)'Å'] = 14,    [(unsigned char)'µ'] = 21,   [(unsigned char)'¥'] = 21,
    ['m'] = 15,   ['M'] = 15,
    ['n'] = 16,   ['N'] = 16,    [(unsigned char)'ò'] = 16,		[(unsigned char)'Ò'] = 16,
    ['o'] = 17,   ['O'] = 17,    [(unsigned char)'ó'] = 17,    [(unsigned char)'Ó'] = 17,    [(unsigned char)'ô'] = 17,   [(unsigned char)'Ô'] = 17,
    ['p'] = 18,   ['P'] = 18,
    ['q'] = 19,   ['Q'] = 19,
    ['r'] = 20,   ['R'] = 20,    [(unsigned char)'à'] = 31,    [(unsigned char)'À'] = 31,
    [(unsigned char)'ø'] = 21,   [(unsigned char)'Ø'] = 21,
    ['s'] = 22,   ['S'] = 22,
    [(unsigned char)'¹'] = 23,   [(unsigned char)'©'] = 23,
    ['t'] = 24,   ['T'] = 24,    [(unsigned char)'»'] = 24,    [(unsigned char)'«'] = 24,
    ['u'] = 25,   ['U'] = 25,    [(unsigned char)'ú'] = 25,    [(unsigned char)'Ú'] = 25,    [(unsigned char)'ù'] = 25, 
    ['v'] = 26,   ['V'] = 26, 
    ['w'] = 27,   ['W'] = 27,
    ['x'] = 28,   ['X'] = 28,
    ['y'] = 29,   ['Y'] = 29,    [(unsigned char)'ý'] = 29,    [(unsigned char)'Ý'] = 29,
    ['z'] = 30,   ['Z'] = 30,
    [(unsigned char)'¾'] = 31,   [(unsigned char)'®'] = 31,
    ['0'] = 32,
    ['1'] = 33,
    ['2'] = 34,
    ['3'] = 35,
    ['4'] = 36,
    ['5'] = 37,
    ['6'] = 38,
    ['7'] = 39,
    ['8'] = 40,
    ['9'] = 41,
    ['.'] = 42,
    [','] = 43,
    [';'] = 44,
    ['?'] = 45,
    ['!'] = 46,
    [':'] = 47,
    ['"'] = 48,
    ['-'] = 49,
    ['|'] = 50,
    ['/'] = 51,
    ['\\'] = 52,  //spetne lomitko
    ['('] = 53,
    [')'] = 54,
   // ['//'] = 55,
    ['['] = 56,
    [']'] = 57,
    ['<'] = 58,
    ['>'] = 59,
    ['{'] = 60,
    ['}'] = 61,
    ['&'] = 62,
   // [''] = 63, my chyba jeden znak fajka
    [(unsigned char)'§'] = 64,
    ['%'] = 65,
   // [' '] = 66 tu mi chyba percento s gulickou
    ['$'] = 67,
    ['='] = 68,
    ['+'] = 69,
    [(unsigned char)'×'] = 70,
    ['*'] = 71,
    ['#'] = 72,
    ['~'] = 73
  //['~~'] = 74,
//  ['~='] = 75,
};
//sekundarna tabulka obsahuje znaky, ktore mali v primarnej rovnaku vahu
const unsigned char tableSecondary[256] = 
{
    ['a'] = 1,    ['A'] = 1,     
    [(unsigned char)'á'] = 2,    [(unsigned char)'Á'] = 2,
    [(unsigned char)'ä'] = 3,
    ['c'] = 5,    ['C'] = 5,
    [(unsigned char)'è'] = 6,    [(unsigned char)'Æ'] = 6,
    ['d'] = 7,    ['D'] = 7,
    [(unsigned char)'ï'] = 8,    [(unsigned char)'Ï'] = 8,
    ['e'] = 9,    ['E'] = 9,  
    [(unsigned char)'é'] = 10,   [(unsigned char)'É'] = 10,
    [(unsigned char)'ì'] = 11,   [(unsigned char)'Ì'] = 11,
//  ['ch'] = 12,  ['CH'] = 12,
    ['i'] = 13,   ['I'] = 13,    
    [(unsigned char)'í'] = 14,   [(unsigned char)'Í'] = 14,
    ['l'] = 15,   ['L'] = 15,
    [(unsigned char)'å'] = 16,   [(unsigned char)'Å'] = 16,
    [(unsigned char)'µ'] = 17,   [(unsigned char)'¥'] = 17,
    ['n'] = 18,   ['N'] = 18,
    [(unsigned char)'ò'] = 19,   [(unsigned char)'Ò'] = 19,
    ['o'] = 20,   ['O'] = 20,
    [(unsigned char)'ó'] = 21,   [(unsigned char)'Ó'] = 21,
    [(unsigned char)'ô'] = 22,   [(unsigned char)'Ô'] = 22,
    ['r'] = 23,   ['R'] = 23,
    [(unsigned char)'à'] = 24,   [(unsigned char)'À'] = 24,
    [(unsigned char)'ø'] = 25,   [(unsigned char)'Ø'] = 25,
    ['s'] = 26,   ['S'] = 26,
    [(unsigned char)'¹'] = 27,   [(unsigned char)'©'] = 27,
    ['t'] = 28,   ['T'] = 28,
    [(unsigned char)'»'] = 29,   [(unsigned char)'«'] = 29,
    ['u'] = 30,   ['U'] = 30,
    [(unsigned char)'ú'] = 31,   [(unsigned char)'Ú'] = 31, 
    [(unsigned char)'ù'] = 32,   [(unsigned char)'Ù'] = 32,
    ['y'] = 33,   ['Y'] = 33,
    [(unsigned char)'ý'] = 34,   [(unsigned char)'Ý'] = 34,
    ['z'] = 35,   ['Z'] = 35,
    [(unsigned char)'¾'] = 36,   [(unsigned char)'®'] = 36,
}; 
// konstanta pre lepsi pristup do tabulky
const unsigned char *table[] = { [PRIMARY] = tablePrimary, [SECONDARY] = tableSecondary };


/**
 * fukncia patry k myStrCmp porovnava dva znaky iduce za sebou
 * sluzi hlavne na odhalenie znaku ch a ~~, ~= 
 * ktore sa rataju ako jeden znak.
 * @param a znak
 * @param b znak
 * @param i ukazatel na index z volajucej funkcie
 * @param tableNum cislo tabulky z ktorej sa bude porovnavat
 * vrati vahu daneho znaku
 */
int cmpSign(char a, char b, int *i, int tableNum)
{
    if ((a == 'c' || a == 'C') && (b == 'h' || b == 'H') && tableNum == 0)
            {
		*i = *i + 1;
                return CH_SIGN;
            }            
     else       
            if (a == '/' && b == '/')
            {
		*i = *i + 1;
		return EXTRA_SIGN;
            }
               
    else                              
            if (a == '~' && b == '~')
            {
		*i = *i + 1;
            	return EXTRA_SIGN_C;
            }
     else             
            if (a == '~' && b == '=')
            {
                *i = *i + 1;
                return  EXTRA_SIGN_D;
            }
    else                
        return  table[tableNum][(unsigned char)a];
}

/**
 * funkcia porovna dva vstupne polia znakov
 * podla ceskej zoradovacej normi.
 * @param strA ukazatel na pole znakov 
 * @param strB ukazatel na pole znakov
 * vrati 0 ak sa terazce rovnaju, 1 ak je prvy abecedne pred,
 * -1 ak je prvy abecedne za.
 */
//ak sa retazce rovnaju tak vrati 0,, ak je A abecedne pred vrati 1, ak je A abecedne za vrati -1;
int myStrCmp(char *strA, char *strB)
{
    int valueA = 0;
    int valueB = 0;
    int lengthA = strlen(strA) +1; //berem do uvahy aj ukoncovaci znak
    int lengthB = strlen(strB) +1;
    int length = (lengthA < lengthB) ? lengthA : lengthB; // do length ulozim dlzku kratsieho z strA a strB 
	int a = 0;
	int b = 0;

    for (int tableNum = PRIMARY; tableNum <= SECONDARY; tableNum++)      
    {
        for (a = 0, b = 0; b <= length && a <= length; a++, b++)
        {	
		if (strA[a + 1] != 0)
            		valueA = cmpSign(strA[a], strA[a + 1], &a, tableNum);
		else 
			valueA = table[tableNum][(unsigned char)strA[a]];
		if (strB[b + 1] != 0)
			valueB = cmpSign(strB[b], strB[b + 1], &b, tableNum);
		else
			valueB = table[tableNum][(unsigned char)strB[b]];

            	if (valueA < valueB)
                	return 1;
            	if (valueA > valueB)
                	return -1;
        }
    }
    return 0;
}

/**
 * funkcia na inicializaciu zoznamu
 */
void listInit(TList *list)
{
    list->first = NULL;
}

/**
 * funkcia uvolni prvy prvok zoznamu
 * @param list ukzatel na prvy prvok
 */
void listDeleteFirst(TList *list)
{
    TItem *item;
    item = list->first;
    list->first = item->next;
    free(item->strPrint);
    free(item);
}

/**
 * funkcia uvolni zoznam
 * @param list ukzatel na prvy prvok zoznamu
 */
void listFree(TList *list)
{
    while(list->first != NULL)
    {
        listDeleteFirst(list);
    }
}

/**
 * funkcia vlozi na zaciatok listu novy item
 * @param list ukazatel na prvy item zoznamu
 * @param item ukazatel na novy item 
 */
void listInsertFirst(TList *list, TItem *item)
{
    item->next = list->first;
    list->first = item;
}

/**
 * funkia dostane 2 ukzatele na polia a vytvori novy polozku
 * pritom od nej skopiruje dane polia
 * @param stPrint pole ktore sa bude tlacit
 * @param strCompare pole ktore sa bude porovnavat
 * vrati NULL ak sa neporarilo, inak ukzatel na item.
 */
TItem *itemCreate(char *strPrint)
{
    TItem *item = (TItem *)malloc(sizeof(TItem));
    if (item == NULL)
        return NULL;
    int length = strlen(strPrint) +2;
    item->strPrint = (char *)malloc(length * sizeof(char));
    if (item->strPrint == NULL)
        return NULL;
    strncpy(item->strPrint, strPrint, length);
    return item;
}

/**
 * funkcia zoradi polozky zoznamu
 * porovnavaju sa polia item.strPrint
 * @param list ukazatel na prvy item zoznamu
 */
void listSort(TList *list)
{
    TItem *itemA;
    TItem *itemB;
    int stop = 1;
    while (stop)
    {
        stop = 0;
        itemA = list->first;
        itemB = itemA->next;
        for ( ;  itemB != NULL; itemA = itemA->next)
        {         
            itemB = itemA->next;
            if (itemB == NULL)
                break;
            if (myStrCmp(itemA->strPrint, itemB->strPrint) == -1) //A je abecedne za B
            {         
		//printf("PRVE JE ABECEDNE ZA %s  %s\n",itemA->strPrint, itemB->strPrint);       
                stop = 1;
                //vymena
                char *pom = itemA->strPrint;
                itemA->strPrint = itemB->strPrint; 
                itemB->strPrint = pom;      
            }
            
        }
    }
}

/**
 * funkcia vytlaci zoznam do subora s nazvom dest
 * @param list ukzatel na prvu polozku v zozname
 * @param dest meno subora kde sa ma zoznam "vytlacit"
 * vrati 0 ak sa nepodaril vytvorit subor, inak 1
 */
int listPrint(TList *list, char *dest)
{
    FILE *fw;
    fw = fopen(dest, "w");
    if (fw == NULL)
        return 2;
    for (TItem *item = list->first; item != NULL; item = item->next)
        fprintf(fw, "%s\n", item->strPrint);
    fclose(fw);
    return 1;
}

/**
 * funkcia preskoci stlpce v tabulke pokial nenarazi na znak attribute
 * @param *fr ukzatel na miesto v subore
 * @param attribute znak po ktory sa mame v subore dostat
 * vrati
 */
void fSkipStr(FILE *fr, char attribute)
{
    int c;
    int i = 0;    
    while((c = fgetc(fr)) != EOF)
    {
        if (attribute == ' ')
        {       
            if (i != 0 && isblank(c))
                break;  
            if (!isblank(c))
                i++;
        }
        else
        {
            if (c != attribute)
                i++;
            if (i != 0 && c == attribute)
                break;
        }
    }
}

/**
 * funkcia skopiruje retazec zo subora do destination az po
 * medzeru alebo koniec riadka podla toho aky znak bude skor.
 * @param *fr ukzatal na miesto kde sa nachadzame v subore
 * @param destination pole kde sa ulozi hodnota zo subora
 * vrati 0 ak sa nepodaril malloc, 2 ak narazil na koniec riadka
 * inak vracia 1,
 */
int  fLoadStr(FILE *fr, char **destination)
{
    if (feof(fr))
        return 1;
    int c;
    int i = 0;
    int size = VALUE * 2;
    *destination = (char *)malloc(VALUE *sizeof(char));       
    if (*destination == NULL)
        return 0;
    while ((c = fgetc(fr)) != EOF)
    {
        if (isspace(c) && i != 0)
            break;
        if (!isblank(c))
        {
            if (i !=0 && (i % (VALUE - 2)) == 0)
            {
                *destination = (char *)realloc(*destination, (size) * sizeof(char));
                if (*destination == NULL)
                    return 0;
                size *= 2;                
            }
            (*destination)[i] = c;   
            i++;           
        }       
    }
    (*destination)[i] = 0;
    if (c == '\n')
        return 2;
    if (c == EOF)
    {
        free(*destination);
        *destination = NULL;
    }
    return 1;
}

/**
 * funkcia precita hlavicku subora, dane stlpce su oddelene
 * lobovolnym poctom medzier,vzdy skonci na konci riadku,
 * alebo na konci subora 
 * @param *fr ukazatel na miesto kde sme v subore
 * @param *objectPrint retazec ktory hladame v hlavicke
 * @param *objectSearch retazec ktory hladame v hlavicke
 * vrati strukturu kde su dne int hodnoty, oznacuju kolko stlpcov
 * treba preskocit aby sme sa dostali k danemu hladanemu stlpcu
 * ak nastavi table.print na -2 tak na nepodaril malloc, ak -1
 * nastavi table.print alebo table.search tak znamena ze nanasiel
 * prislusny stlpec na tlacenie.
 */
TTable fLoadHead(FILE *fr, char *objectPrint, char *objectSearch, int state)
{
    TTable table = 
    {
        .print = 0,
        .search = 0,
    };
    int p = 1;
    int s = 1; 
    int c;
    char *str = NULL;    
    int secure = 0;
    while(!feof(fr))
    {
        secure++;
        c = fLoadStr(fr, &str);
	if (c == 0)
	{
		table.print = -2;
		return table;
	}
        if (strcmp(objectPrint, str) != 0 && s == 1)
            table.print++;
        else 
            s = 0;
        if (state != CPRINT && state != CSORTPRINT)
        {
            if (strcmp(objectSearch, str) != 0 && p == 1)
                table.search++;
            else 
                p = 0;
        }
        free(str);
        if (c == 2) //ak narazil na koniec riadku
            break;
    }    
    table.print = (table.print == secure) ? -1 : table.print;
    table.search = (table.search == secure) ? -1 : table.search;
    if (state == CPRINT || state == CSORTPRINT) //koli tomu ak je iba print tak 
        table.search = table.print + 1;
   // printf("table.search = %d table.print = %d\n", table.search, table.print);
    return table;
}

/**
 * funkcia dostane dva ukazatele, alokuje miesto 
 * velkosti strA pre ukazatel strB a skopiruje tam
 * znaky z strA
 * @param *strA ukazatel na zdrojovy retazec
 * @param *strB ukazatel na cielovy retazec
 * vrati 0 ak sa nepodaril malloc, inak 0
 */
int strCreateCopy(char *strA, char **strB)
{
    int length = strlen(strA) + 1; //na konci bude 0
    *strB = (char*)malloc(length * sizeof(char));
    if (*strB == NULL)
        return 0;
    strncpy(*strB, strA, length);
    return 1;
}

/**
 * funkcia patri k funkcii fStrReadPrint a fStrReadPrintList
 * podla poradia stlpcom nacita prve pole bud strSearch alebo strPrint
 * @param *fr ukzatel kde sa nachadzame v subore
 * @param *strCompare ukazatel na pole ktore chceme nacitat
 * @param *strPrint ukazatel na pole ktore chceme nacitat
 * @param search cislo ktore urcuje cislo stlpca v tabulke z ktoreho mame naciatat strSearch
 * @param print cislo ktore urcuje cislo stlpca v tabulke z ktoreho mame nacitat strPrint
 * vrati 0 ak sa nepodarilo nacitat (malloc), inak 1
 */
int tableReadFirst(FILE *fr, char **strCompare, char **strPrint, int search, int print)
{
    int check;
    if (search < print)
    {
        check = fLoadStr(fr, strCompare);
        if (check == 0)
            return 0;
        if (check == 2)
            return 2;
    }
    else
        {
            check = fLoadStr(fr, strPrint);
            if (check == 0)
                return 0;
            if (check == 2)
                return 2;
        }
    return 1;
}

/**
 * funkcia patri k funkcii fStrReadPrint a fStrReadPrintList
 * podla poradia stlpcov nacita druhe pole bud strSearch alebo strPrint
 * @param *fr ukzatel kde sa nachadzame v subore
 * @param *strCompare ukazatel na pole ktore chceme nacitat
 * @param *strPrint ukazatel na pole ktore chceme nacitat
 * @param search cislo ktore urcuje cislo stlpca v tabulke z ktoreho mame naciatat strSearch
 * @param print cislo ktore urcuje cislo stlpca v tabulke z ktoreho mame nacitat strPrint
 * vrati 0 ak sa nepodarilo nacitat (malloc), inak 1
 */
int tableReadSecond(FILE *fr, char **strCompare, char **strPrint, int search, int print)
{
    int check;
    if (search == print)
    {
        if (strCreateCopy(*strPrint, strCompare) == 0)
        {
            free(strPrint);
            return 0;
        }
    }
    if (search < print)
    {
        check = fLoadStr(fr, strPrint);
        if (check == 0)
        {
            free(strCompare);
            return 0;
        }
        if (check == 2)
            return 2;
    }
    else
        {
            check = fLoadStr(fr, strCompare);
            if (check == 0)
            {
                free(strPrint);
                return 0;
            }  
            if (check == 2)
                return 2;
        }
    return 1;
}

/**
 * patri k funkcii fStrReadPrint, podla stavu porovna dva polia a vypise alebo nie
 * @param *strCompare naciatany retazec z tabulky
 * @param *objectCompare retazec zadany na prikazovom riadku
 * @param state indiduje stav programu
 * @param dest nazov suoru kde sa ma vytlacit strPrint
 */
int strStatePrint(FILE *fw, char *strCompare, char *strPrint, char *objectCompare, int state)
{
    if (strPrint == NULL)
	return 1;
    if (state == CBEFORE)
    {
        if (myStrCmp(strCompare, objectCompare) == 1)
            fprintf(fw, "%s\n", strPrint);
    }        
    if (state == CAFTER)
    {
        if (myStrCmp(strCompare, objectCompare) == -1)
            fprintf(fw, "%s\n", strPrint);
    }
    if (state == CPRINT)
        fprintf(fw, "%s\n", strPrint);
    return 0;
}

/**
 * funkcia cita slova zo subora source a podla 
 * pravidiel ich vypise
 * @param source vstupny subor kde su data
 * @param dest vystupny subor kde sa ulozia vystupne data
 * @param objectPrint nazov stlpca ktory sa bude tlacit
 * @param objectSearch nazov stlpva v ktorom sa bude porovnavat
 * @param objectCompare hodnota ktora sa bude abecedne porovnavat
 * @param state stav programu --print alebo --after/before 
 * vrati 0, ak sa nepodaril malloc, 2 ak sa nepodaril otvorit subor, inak 1
 */
int fStrReadPrint(char *source, char *dest, char *objectPrint, char *objectSearch, char *objectCompare, int state)
{
    FILE *fr = fopen(source, "r");
    if (fr == NULL)
        return 2;
    FILE *fw = fopen(dest, "w");
    if (fw == NULL)
    {
        fclose(fr);
        return 2;
    }
    TTable table = fLoadHead(fr, objectPrint, objectSearch, state);
    if (table.print == -2)
    {
        fclose(fw);
        fclose(fr);
        return 0;
    }        
    if (table.print == -1)//ak sa nenasiel stlpec co sa ma tlacit tak koniec.
    {
        fclose(fw);
        fclose(fr);
        return 1;
    }
    if ((state == CAFTER || state == CBEFORE) && table.search == - 1)
    {
        fclose(fw);
        fclose(fr);
        return 1;//ak sa nenasiel stlpec kde sa ma hodnota porovnavat tak koniec   
    }
    int check; //kontroluje ci sa nepodaril malloc alebo ci nebol znak '\n'
    char *strPrint = NULL;
    char *strCompare = NULL;
    int a, b = 0;
    if (state == CPRINT)
        a = table.print;
    else
    {   //do a nacitam mensiu vzdialenost
        a = (table.search < table.print) ? table.search : table.print;
        // do b dam rozdiel najvacsieho a a,
        b = (table.search < table.print) ? table.print - a -1 : table.search - a -1;
    }   
    while(!feof(fr))
    {//skocim na prvy stlpec ktory potrebujem
	strPrint = NULL;
	strCompare = NULL;
        for (int i = 0; i < a; i++)
            fSkipStr(fr, ' ');
        //nacitam prvu hodnotu
       check = tableReadFirst(fr, &strCompare, &strPrint, table.search, table. print);    
       if (check == 0)
       {
           fclose(fr);
           return 0;
       }
       if (state != CPRINT)
       {
        // posuniem sa na druhy stlpec ktory potrebujem
            for (int i = 0; i < b; i++)
                fSkipStr(fr, ' ');
            check = tableReadSecond(fr, &strCompare, &strPrint, table.search, table.print);
            if (check == 0)
            {
                fclose(fr);
                fclose(fw);
                return 0;
            }
        }
        strStatePrint(fw, strCompare, strPrint, objectCompare, state);
        //posuniem sa na koniec riadku aby som stadial mohol znova citat
         if (check != 2)
             fSkipStr(fr, '\n');
	if (strPrint != NULL)
        	free(strPrint);
        if (strCompare != NULL)
            free(strCompare);
    }
    fclose(fw);
    fclose(fr);
    return 1;
}

/**
 * patri k funkcii fStrReadPrintList, podla stavu urcuje ci sa vytvory novi item do listu
 * @param *strCompare retazec nacitany z tabulky
 * @param *strPrint retazec nacitany z tabulky
 * @param *objectCompare retazec zadany na prikazovom riadku
 * @param state indikuje stav programu
 * vrati 2 ak sa vytvorila nova polozka,
 * ak sa nova polozka nevitvorila pretoze sa nepodaril malloc vrati 0
 * inak 1
 */
int strStateLoad(char *strCompare, char *strPrint, char *objectCompare, TItem **item, int state)
{
    if (strPrint == NULL)
        return 1;
    if (state == CSORTPRINT)
    {
        if ((*item = itemCreate(strPrint)) == NULL)
            return 0;
        else 
            return 2;
    }
    if (state == CSORTBEFORE)
    {
        if (myStrCmp(strCompare, objectCompare) == 1)
        {
            if ((*item = itemCreate(strPrint)) == NULL)
                return 0;
            else 
                return 2;
            }
        }        
    if (state == CSORTAFTER)
    {
        if (myStrCmp(strCompare, objectCompare) == -1)
        {
            if ((*item = itemCreate(strPrint)) == NULL)
                return 0;
            else 
                return 2;
        }
    }
    // printf("ITEM VO FUNKCII %s\n", item->strPrint);
    return 1;
}

/**
 * funkcia cita slova zo subora source a podla 
 * pravidiel ich vypise
 * @param source vstupny subor kde su data
 * @param dest vystupny subor kde sa ulozia vystupne data
 * @param objectPrint nazov stlpca ktory sa bude tlacit
 * @param objectSearch nazov stlpva v ktorom sa bude porovnavat
 * @param objectCompare hodnota ktora sa bude abecedne porovnavat
 * @param state stav programu --print alebo --after/before 
 * vrati 0, ak sa nepodaril malloc, 2 ak sa nepodaril otvorit subor, inak 1
 */
int fStrReadPrintList(char *source, char *dest, char *objectPrint, char *objectSearch, char *objectCompare, int state)
{
    FILE *fr = fopen(source, "r");
    if (fr == NULL)
        return 2;

    TTable table = fLoadHead(fr, objectPrint, objectSearch, state);
    if (table.print == -2)
    {
        fclose(fr);
        return 0;
    }
    if (table.print == -1)//ak sa nenasiel stlpec co sa ma tlacit tak koniec.
    {
        fclose(fr);
        return 1;
    }
    if ((state == CSORTAFTER || state == CSORTBEFORE) && table.search == - 1)
    {
        fclose(fr);
        return 1;//ak sa nenasiel stlpec kde sa ma hodnota porovnavat tak koniec        
    }
    int check; //kontroluje ci sa nepodaril malloc alebo ci nebol znak '\n'
    char *strPrint;
    char *strCompare;
    int a, b = 0;
    if (state == CSORTPRINT)
        a = table.print;
    else
    {
        //do a nacitam mensiu vzdialenost
        a = (table.search < table.print) ? table.search : table.print;
        // do b dam rozdiel najvacsieho a a,
        b = (table.search < table.print) ? table.print - a -1 : table.search - a -1;
    }
    TList list;
    listInit(&list);
    TItem *item;  
    while(!feof(fr))
    {//skocim na prvy stlpec ktory potrebujem
        strPrint = NULL;
        strCompare = NULL;
        item = NULL;
        for (int i = 0; i < a; i++)
            fSkipStr(fr, ' ');
        //nacitam prve pole
        check = tableReadFirst(fr, &strCompare, &strPrint, table.search, table.print);
        if (check == 0)
        {
            if (list.first != NULL)
                listFree(&list);
            fclose(fr);
            return 0;   
        }
        if (state != CSORTPRINT)
        {
            // posuniem sa na druhy stlpec ktory potrebujem
            for (int i = 0; i < b; i++)
                fSkipStr(fr, ' ');
            //nacitam druhe pole
            check = tableReadSecond(fr, &strCompare, &strPrint, table.search, table.print);
            if (check == 0)
            {
                if (list.first != NULL)
                    listFree(&list);
                fclose(fr);
                return 0;
            }
        }
        
        //posuniem sa na koniec riadku aby som stadial mohol znova citat
        if (check != 2)
        {
           fSkipStr(fr, '\n'); 
        }
        // porovnavanie   
       check = strStateLoad(strCompare, strPrint, objectCompare, &item, state);  
       if (check == 0)
       {
           if (list.first != NULL)
               listFree(&list);
           fclose(fr);
           return 0;
       }
       else
           if (check == 2 && strPrint != NULL)
           {
                listInsertFirst(&list, item);  
           }
        //printf("ITEM %s\n", item->strPrint);
       if (strPrint != NULL)
        free(strPrint);
       if (strCompare != NULL)
        free(strCompare);
    }
 
    if (list.first != NULL)
    {
        listSort(&list);
        if (listPrint(&list, dest) == 0)
        {
            listFree(&list);
            fclose(fr);
            return 2;
        }
        listFree(&list);
    }
    fclose(fr);
    return 1;
}

/**
 * Hlavny program 
 */
int main(int argc, char **argv)
{   
    TParams params = getParams(argc, argv);  
   
    if(params.ecode != EOK)
    {
        printECode(params.ecode);
        return EXIT_FAILURE;
    }
    
    if (params.state == CHELP)
        printf("%s",HELPMSG);    
     
    if (params.state == CPRINT || params.state == CAFTER || params.state == CBEFORE)
    {
        params.ecode = fStrReadPrint(params.source, params.dest, params.objectPrint, params.objectSearch, params.objectCompare, params.state);
       if (params.ecode == 0)
           printECode(EMALLOC);
       if (params.ecode == 2)
           printECode(EFILE);
    }
    if (params.state == CSORTAFTER || params.state == CSORTBEFORE || params.state == CSORTPRINT)
    {
        params.ecode = fStrReadPrintList(params.source, params.dest, params.objectPrint, params.objectSearch, params.objectCompare, params.state);
        if (params.ecode == 0)
            printECode(EMALLOC);
        if (params.ecode == 2)
            printECode(EFILE);
    }

    freeParams(&params);    
    return 0;
}
