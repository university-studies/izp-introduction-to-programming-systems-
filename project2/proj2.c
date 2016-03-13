/** Subor:  proj2.c
 * Datum:   2010/11/9
 * Autor:   Pavol Loffay xloffa00@stu.fit.vutbr.cz
 * Projekt: Iteracne vypocty, Projekt 2. do predmetu IZP
 * Popis:   Program spracovava lubovolne dlhu postupnost cisel double 
 * na standardnom vstupe. Program sa ukonci ked narazi na koniec suboru - EOF.
 * Ulohou bolo implementovat matematicke funkcie logax a tanh pomocou
 * zakladnych matematickych operacii (+,-,*,/) 
 * Druhou ulohou bolo implementovat algoritmi,
 * ktore pacitaju vazeny aritmeticky a vazeny kvadraticky priemer
 * 
 * --wam | pocita vazeny aritmeticky priemer  | vstup x1 h1 x2 h2 .... xn hn, x-hodnota h-vaha
 * --wqm | pocita vazeny kvadraticky priemer  | vstup x1 h1 x2 h2 .... xn hn, x-hodnota h-vaha
 * vystupom pre tieto dva prepinace je postupnost cisel o polku kratsia ako vstup
 * 
 * --tanh sigdig | pocita hyperbolicky tangens
 * --logax sigdig a | pocita logaritmus o zaklade a 
 * sigdig je prirodzene cislo, ktore udava pocet platnych cislic na ktore musi byt vysledok presny. 
 */
#include <stdio.h>
// ctype.h na isdigit
#include <ctype.h> 
// strcmp
#include <string.h>
// atoi strtod
#include <stdlib.h>
// na limit pre double
#include <limits.h>
// na matematicke funkcie
#include <math.h>
// pre DBL_DIG
#include <float.h>


// vycetny typ chyb
enum tecode
{
    EOK = 0,
    ECLWRONG,
    EMALLOC,
};
// vycetny typ stavu programu
enum tstates
{
    CHELP,    
    CWAM,
    CWQM,
    CTANH,
    CLOGAX,    
    COTHER,
};
// struktura obsahujuca parametry prik. riadku
typedef struct params
{
    int ecode;
    int state;
    double sigdig;
    double a;
} Tparams;
//struktura k fukciam kt. pocitaju priemer
typedef struct
{
    int vahaN;    //nova vaha
    unsigned int vahaS;    //stara vaha
    double x;              //aktualne cislo ku ktoremu natri nova vaha
    double priemer;        //priemer
}Tpriemer;

// konstatna e
const double IZP_E =       2.718281828459045235360287471352662497757247093699959574966;

// konstanta 359, oznacuje ze e^(2*259) je infinity pre double
const int LIMITE = 359; 

// Chybove hlasnia
const char *ECODEMSG[] =
{
// EOK
  "Vsetko v poriadku.\n",
// ECLWRONG
  "Chybne parametry prikazoveho riadku! Pre napovedu -h\n",
};

// Hlasenie help
const char *HELPMSG =
  "Program iteracne vypocty\n"
  "Autor: Pavol Loffay 1BIT kruzok 30.\n"
  "Pouzitie, v prikazovom riadku:\n"
  "$ ./proj2 -h    :program vypise tuto napovedu\n"
  "$ ./proj2 --wam :program pocita vazeny aritmeticky priemer\n"
  "$ ./proj2 --wqm :program pocita vazeny kvadraticky priemer\n"
  "$ ./proj2 --tanh sigdig    :program pocita hyperbolicky tangens\n"
  "$ ./proj2 --logax sigdig a :program pocita logaritmus\n"
  " program spracovava nekonecne dlhu postupnost cisel na vstupe az po EOF\n"
  " sigdig : cele kladne cislo znamena na kolko platnych cisel bude vysledok presny\n"
  " sigdig musi byt v intervale < 1, 14 >\n"
  " a : zaklad logaritmu v intervale (0,1) || (1,inf>\n";

/**
 * Funckia vytlaci chybove hlasenie
 */
void printECode(int ecode)
{
  fprintf(stderr, "%s", ECODEMSG[ecode]);
}

/**
 * Funkcia prevedie sigdig na eps 0.xxx 
 * @param *sigdig zmeni ho na eps
 */
void sigdig(double *sigdig)
{
    double eps = 0.1;
    while (*sigdig > 0)
    {
        *sigdig = *sigdig - 1;
        eps *= 0.1;
    }
    *sigdig = eps;    
}
/**
 * Funkcia ktora zisti ci je dane pole znakov kladne cele cislo 
 * @param argv je vstupny parameter ukazatel na pole znakov
 * vratcia 1 ak dane pole znakov je cele kladne cislo, ak nie 0
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
 * Funckia podla argumentov na prikazovom riadku 
 * vrati pozadovany stav 
 * @param argv1 je ukazatel na pole znakov 
 * vratcia stavovy kod programu
 */
int paramArgv1(char *argv1)
{
    if (strcmp(argv1, "-h") == 0)
        return CHELP;
    if (strcmp(argv1, "--wam") == 0)
        return CWAM;
    if (strcmp(argv1, "--wqm") == 0)
        return CWQM;
    return COTHER;
}

/**
 * Funkcia podla argumentov z prikazoveho riadku zisti
 * ci sa ma spracovavat tanh 
 * @param argv[] ukazatel na polia poli znakov z prikazoveho riadku
 * vrati 1 ak ano, inak 0
 */
int paramTanh(char *argv[])
{
    if (strcmp(argv[1],"--tanh") == 0 && isnumber(argv[2]))
    {
        unsigned int i = atol(argv[2]);
        if (i > 0 && i < DBL_DIG)
            return 1;
    }
    return 0;
} 

/** 
 * @param argv ukazatel na pole znakov z prikazoveho riadku
 * vrati 1 ak je dane pole znakov cele alebo desatinne cislo rozsahu double, 
 * inak 0
 */
int paramA(char *argv)
{
    int dlzka = strlen(argv);
    int bodka = 1;
    int i = 0;
        
    while ( i < dlzka && (isdigit(argv[i]) || (argv[i] == '.' && bodka)))
    {
        if (argv[i] == '.')
            bodka = 0;
        i++;
    }
    if (i == dlzka)
    {
        double a;
        a = strtod(argv, NULL);
        if (a > 0 && a != 1 && a <= LONG_MAX)
        return 1;
    }
    if (strcmp(argv, "infinity") == 0 || strcmp(argv, "INFINITY") == 0 || strcmp(argv, "inf") == 0)
        return 1;
    return 0;
}

/**
 * Funkcia podla argumentov z prikazoveho riadku zisti
 * ci sa ma cpracovavat logaX 
 * @param argv[] ukazatel na polia poli znakov z prikazoveho riadku
 * vrati 1 ak ano, inak 0
 */   
int paramLogax(char *argv[])
{
    if (strcmp(argv[1], "--logax") == 0 && isnumber(argv[2]) && paramA(argv[3]))
    {
        unsigned int i = atol(argv[2]);
        if (i > 0 && i < DBL_DIG)
            return 1;
    }
    return 0;
}       

/**
 * Funkcia spracuje argumenty prikazovehoriadku
 * nastavy chybovy kod programu a stavovy 
 * @param argc udava kolko poli znakov(argv) sme zadali
 * @param argv je ukazatel na pole znakov
 * vracia strukturu Tparams
 */
Tparams getParams(int argc, char *argv[])
{
    Tparams result = 
    {// inicializacia struktury
        .ecode = EOK,
        .state = CHELP,
        .sigdig = 0,
        .a = 0,
    };
    
    if (argc == 2)
    { 
        if ((result.state = paramArgv1(argv[1])) == COTHER)
            result.ecode = ECLWRONG;       
    }
    else 
        if (argc == CTANH) // argc = 3 
        {
            if (paramTanh(argv))
            {
                result.state = CTANH;
                result.sigdig = strtod(argv[2], NULL);
                sigdig(&result.sigdig);
            }
            else
                result.ecode = ECLWRONG;                
        }
        else
            if (argc == CLOGAX) //argc = 4 
            {
                if (paramLogax(argv))
                {                    
                    result.state = CLOGAX;
                    result.sigdig = strtod(argv[2], NULL);
                    sigdig(&result.sigdig);
                    result.a = strtod(argv[3], NULL);
                }
                else
                {
                    result.ecode = ECLWRONG;
                }
            }
            else result.ecode = ECLWRONG;
    return result;
}

/**
 * Funkcia vypocita vazeny kvadraticky priemer
 * @param vys je vstupny parameter struktury Tpriemer
 * vracia 1 ak dostala nezapornu novu vahu, inak 0
 */
int vazenyAP(Tpriemer *vys)
{
    if (vys->vahaN >= 0)
    {
        if (vys->vahaS == 0 && vys->vahaN == 0)
            vys->priemer = NAN;        
        else
            vys->priemer = (vys->priemer*vys->vahaS + vys->x*vys->vahaN)/(vys->vahaN+vys->vahaS);
        vys->vahaS = vys->vahaS + vys->vahaN;
    }
    else
    {
        vys->priemer = NAN;
        return 0;
    }
    return 1;   
}

/**
 * Funckia vypocita vazeny kvadraticky priemer
 * @param vys je vstupny parameter struktury Tpriemer.
 * vracia 1 ak dostala nezapornu novu vahu, inak 0
 */
int vazenyQP(Tpriemer *vys)
{
    if (vys->vahaN >= 0)
    {
        if (vys->vahaS == 0 && vys->vahaN == 0)
            vys->priemer = NAN;        
        else
            vys->priemer = sqrt((vys->priemer*vys->priemer*vys->vahaS + vys->x*vys->x*vys->vahaN)/(vys->vahaN+vys->vahaS));
        vys->vahaS = vys->vahaN + vys->vahaS;
    }
    else
    {
        vys->priemer = NAN;
        return 0;
    }
    return 1;    
}

/**
 * Funkcia cita cisla zo standardneho vstupu,
 * predava ich fukciam na vypocet vazeneho aritmetickeho priemeru
 * alebo funkcii na vypocet vazeneho kvadratickeho priemeru.
 * @param state vstupny parameter, udava ci sa vypocita Quad.P alebo AritP.
 * vracia NAN ak pocet cisel na vstupe bol neparny pocet, inak vrati priemer
 */
int citajTlacPriemer(int state)
{
    Tpriemer vysledok = 
    {
        .vahaN = 0,
        .vahaS = 0,
        .priemer = 0,
        .x = 0,      
    }; 
    int pocet = 1;
    int prveCislo = 1;
    int nebolnan = 1;
    while (pocet != EOF)
    {
        if (prveCislo)
            {
                pocet = scanf("%lf", &vysledok.x);
                prveCislo = 0;
            }
        else
        {
            pocet = scanf("%d", &vysledok.vahaN);
            prveCislo = 1;
        }
     
        if (pocet != 1)
        {
            pocet = scanf("%*s");
            vysledok.priemer = NAN;
            nebolnan = 0;
            
        }
        if (prveCislo == 1)
        {
            if (nebolnan)
            {
                if (state == CWAM)
                    nebolnan = vazenyAP(&vysledok);
                else
                    nebolnan = vazenyQP(&vysledok);
            }
            printf("%.10e\n", vysledok.priemer);
            if (vysledok.vahaS == 0 && vysledok.vahaN == 0)
                vysledok.priemer = 0;  
        }                
    }
    return 1;
}

/**
 * Funkcia dostane cislo a upravi ho do intervalu (0.5;1.5)
 * kde funkcia lnx najlepsie konverguje, 
 * Funkcia vrati kolko krat sa ma cislo 1 odpocitat/propocitat k lnX
 * @param **cislo nasobi alebo deli cislom e pokial cislo nedostane 
 * do vhodneho intervalu (0.5;1.5).
 */

int heurUpravaLn(double *cislo)
{
    int i = 0;
    while (*cislo > 1.5)
    {
        *cislo = *cislo / IZP_E;
        i++;
    }
    while (*cislo < 0.5)
    {
        *cislo = *cislo * IZP_E;
        i--;
    } 
    return i;
}
/**
 * Funkcia dostane cislo a vypocita z neho prirodzeny logaritmus, s zakladom e
 * @param *cislo vypocita z neho ln a ulozi ho na jeho adresu
 * @param *sigdig udava presnost vysledku na cifry
 * funkcia vracia 1 ak cislo bolo v definickom obore logaritmu 0 ak nie
 */
int lnX(double *cislo, double *sigdig)
{
    if (*cislo == 0) //ak je rovno 0
    {
        *cislo = -INFINITY;
        return EOK;
    }
    if (*cislo < 0 ) //ak je mensie ako nu;a
    {
        *cislo = NAN;
        return 0;
    }
    if (isnan(*cislo)) //ak je nan
    {
        *cislo = NAN;
        return 0;
    }
    if (isinf(*cislo))
    {
        *cislo = INFINITY;
        return 1;
    }   
    int k = heurUpravaLn(cislo); //x som dostal do (0.5,1.5)    
    double Yi = (*cislo) - 1;
    double urychlenie = 1 - *cislo ;
    int i = 2;
    *cislo = Yi;
    double pom = 0;
    while (fabs(*cislo - pom) >= *sigdig)
    {  
        pom = *cislo;
        Yi *= (urychlenie*(i-1))/i;
        i += 1;
        *cislo += Yi;   
    }     
    *cislo = *cislo + k;
    return 1;
}

/**
 * Funkcia vypocita z vstupneho parametra cislo, e^cislo a ulozi ho 
 * na jeho adresu.
 * vstupny parameter cislo si upravi tak ze ho rozdeli na desatinnu cast a celu
 * z desatinnej casti pocita e^desatinna nekonecnym radom, celu cast pocita e^celu
 * cyklom for.
 * vrati 0 ak vseto prebehlo v poriadku. 
 * @param *cislo vstupny parameter vypocita z neho e^cislo a ulozi na adresu cislo
 * @param *sigdig udava presnost vypoctu na cifry
 */
int enaX(double *cislo, double *sigdig)
{
    double eNaCela = 1;
    double cela, desatinna;
    desatinna = modf(*cislo, &cela);  
    double Yi = 1;
    double suma = Yi;
    double pom = 0;
    int i = 1;
    if (desatinna != 0)
    {
        while (fabs(pom -suma) >= *sigdig)  
        { 
            pom = suma;
            Yi = (Yi * desatinna)/i;   
            i++;
            suma += Yi;   
        }
    }
    for (double j = 0; j < cela; j += 1)
        eNaCela *= IZP_E;
    for (double j = 0; j > cela; j -= 1)
        eNaCela *= IZP_E;
    if (cela < 0)
    {
        eNaCela = 1 / eNaCela;
    }
    *cislo = suma * eNaCela;    
    return EOK; 
}

/**
 * Funkcia dostane cislo a vypocita z neho sinh a ulozi vysledok na jeho adresu
 * pocita pomocou nekonecneho radu.
 * @param *cislo vypocita sinh z toho cisla a uloziho na jeho adresu
 * @param *sigdig udava presnost vysledku na cifry
 */
void sinhyperB(double *cislo, double *sigdig)
{
    double Yi = *cislo;
    double pom = 1;
    double urychlenie = *cislo * *cislo;  
    int i = 0;   
    while (fabs(Yi-pom) >= *sigdig)             
    {
        pom = Yi;
        i = i + 2;
        Yi *= urychlenie/((i+1)*i);
        *cislo = *cislo + Yi;        
    }    
}

/** 
 * Funkcia dostane cislo a vypocita z neho cosh a ulozi vysledok na jeho adresu
 * pocita pomocou nekonecneho radu
 * @param *cislo vypocita cosh z tohoto cisla a uloziho na jeho adresu
 * @param *sigdig udava presnost vysledku na cifry 
 */
void coshyperB(double *cislo, double *sigdig)
{
    double Yi = 1;
    double pom = 0;
    double urychlenie = *cislo * *cislo;
    *cislo = Yi;
    int i = 0; 
    while (fabs(Yi-pom) >= *sigdig)            
    {
        pom = Yi;
        i = i + 2;
        Yi *= urychlenie/(i*(i-1));
        *cislo = *cislo + Yi;
    }    
}

/**
 * Funkcia dostane cislo z ktoreho vypocita tanh
 * ak je vstupny parameter cislo v intervale (-1,1)
 * tak tanh sa pocita pomocou sinh/cosh ak nie tak 
 * pomocou exponencialneho tvaru tanh = (e^x - e^(-x)) / (e^x + e^(-x))
 * vracia 1 ak sa pocital tanh pomocou sinh/cosh ak nie tak vracia 0;
 * @param *cilsloA je cislo z ktoreho vypocta tanhyperB
 * @param *sigdig udava presnost vysledku na cifry
 */
int tanhyperB(double *cislo, double *sigdig)
{
    if (*cislo > LIMITE)
    {
        *cislo = 1;
        return EOK;
    }
    if (*cislo < -LIMITE)
    {
        *cislo = -1;
        return EOK;
    }
    if (*cislo == 0)
    {
        *cislo = 0;
        return EOK;
    }
    
    if (*cislo < 1 && *cislo > -1)
    {
        double cisloB = *cislo;
        sinhyperB(cislo, sigdig);
        coshyperB(&cisloB, sigdig);
        *cislo = *cislo / cisloB;
        return 1;
    }    
    *cislo = *cislo * 2;   
    enaX(cislo, sigdig);    
    *cislo = (*cislo - 1) / (*cislo + 1);    
    return 0;
}

/**
 * Funkcia cita zo standardneho vstupu cisla typu double a
 * tlaci ich hodnoty logaX alebo tanh
 * @param state urcuje ci sa bude pocitat logaX alebo tanh
 * @param a zaklad logaritmu
 * @param sigdig udava presnost vysledku na cifry
 */
int citajTlacCisla(int state, double a, double sigdig)
{
    if (state == CLOGAX) // ak sa bude pocitat log tak vypocita zo zakladu lna
        lnX(&a, &sigdig);
    double hodnota;
    int kontrola;  //kontroluje ci som nacital cislo ak nie preskoci vstup

    while ((kontrola = scanf("%lf", &hodnota)) != EOF)
    {
        if (kontrola != 1)
        {
           kontrola = scanf("%*s");
           hodnota = NAN;
           printf("%.10e\n", hodnota);
        }
        else
            if (state == CLOGAX)
            {
                lnX(&hodnota, &sigdig);
                printf("%.10e\n", hodnota/a); 
            }
            else
            {
                tanhyperB(&hodnota, &sigdig);
                printf("%.10e\n", hodnota);
            }            
    }
    return EOK;
}
/**
 * Hlavny program  
 */
int main(int argc, char *argv[])
{
    Tparams params = getParams(argc, argv);    
   
    if (params.ecode != EOK)
    {
        printECode(params.ecode);
        return EXIT_FAILURE;
    }
    
    if (params.state == CHELP)
        printf("%s", HELPMSG);
    
    if (params.state == CWAM || params.state == CWQM)
        citajTlacPriemer(params.state);    
    
    if (params.state == CTANH || params.state == CLOGAX)
        citajTlacCisla(params.state, params.a, params.sigdig);    
    return 0;
}