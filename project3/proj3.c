/** Subor:  proj3.c
 * Datum:   2010/11/18
 * Autor:   Pavol Loffay xloffa00@stu.fit.vutbr.cz
 * Projekt: Maticove operacie, Projekt 3. do predmetu IZP
 * Popis:   Program sprauvava rozne operacie s vektormi a maticami
 *./proj3 -h                    :program vypise tuto napovedu\n"
 *./proj3 --test a.txt           :program otestuje spravny format vstupneho suboru\n"
 *./proj3 --vadd a.txt b.txt     :program pocita sucet vektorov (a+b)\n"
 *./proj3 --vscal a.txt b.txt    :program pocita skalarny sucin vektorov (a*b)\n"
 *./proj3 --mmult a.txt b.txt    :program pocita sucin dvoch matic (A*B)\n"
 *./proj3 --mexpr a.txt b.txt    :program pocita vyraz (A*B*A)\n"
 *./proj3 --eight a.txt b.txt    :program hlada vektor z a.txt v matici b.txt\n"
 *./proj3 --bubbles a.txt        :program pocita pocet bublin v matici a.txt\n";
 */
#include <stdio.h>
// pre strcmp a strlen
#include <string.h> 
// pre free a malloc
#include <stdlib.h>

// vyctovy typ chyb
enum tecodes
{
    EOK,
    ECLWRONG,
    EMALLOC,
    EFILE,
    EFILEDATA,
};
// vycetny typ stavu 
enum tstates
{
    CHELP,
    CTEST,
    CVADD,
    CVSCALL,
    CMMULT,
    CMEXPR,
    CEIGHT,
    CBUBBLES,
    COTHER,
};
// struktura obsahujuca paramatre prik. riadku
typedef struct params
{
    int ecode;
    int state;
    char *fileNameA;
    char *fileNameB;
}TParams;
// struktura obsahujuca parametre pre pracu s vektorom
typedef struct vector
{
    int object;
    int columns; 
    int *array;
}TVector;
// struktura obsahujuca parametre pre pracu s maticou
typedef struct matrix
{
    int object;
    int rows;     //riadky 
    int columns;  //stlpce | stlporadie
    int **array;
}TMatrix;
// struktura obsahujuca parametre pre pracu s vektorom matic
typedef struct vectorMatrix
{
    int object;   //ci sa jedna o vektor/maticu/vektorMatic
    int number;   // pcet matic
    int rows;     //riadky
    int columns;  //stlpce/stlporiadie
    int ***array;
}TVectorMatrix;
// chybove hlasenia programu
const char *ECODEMSG[] =
{
// EOK
  "Vsetko v poriadku.\n",
// ECLWRONG
  "Chybne parametry prikazoveho riadku! Pre napovedu -h\n",
//   EMALLOC
  "Nepodarila sa dynamicky alokovat pamet.\n",
//   EFILE
  "Nepodarilo sa otvorit subor / Neexistuje subor.\n",
//   EFILEDATA
  "V subore sa nenachadzaju spravne data.\n",
};

// hlasenia help
const char *HELPMSG =
  "Program Maticove operacie\n"
  "Autor: Pavol Loffay 1BIT kruzok 30.\n"
  "Pouzitie, v prikazovom riadku:\n"
  "$ ./proj3 -h                  :program vypise tuto napovedu\n"
  "$ ./proj3 --test a.txt        :program otestuje spravny format vstupneho suboru\n"
  "$ ./proj3 --vadd a.txt b.txt  :program pocita sucet vektorov (a+b)\n"
  "$ ./proj3 --vscal a.txt b.txt    :program pocita skalarny sucin vektorov (a*b)\n"
  "$ ./proj3 --mmult a.txt b.txt    :program pocita sucin dvoch matic (A*B)\n"
  "$ ./proj3 --mexpr a.txt b.txt    :program pocita vyraz (A*B*A)\n"
  "$ ./proj3 --eight a.txt b.txt    :program hlada vektor z a.txt v matici b.txt\n"
  "$ ./proj3 --bubbles a.txt    :program pocita pocet bublin v matici a.txt\n";
  
// konstanta false a true
const char *FALSEMSG = "false\n";
const char *TRUEMSG = "true\n";

/**
 * fukcia na vypis chyboveho hlasenia
 * @param ecode typ chyby 
 */
void printECode(int ecode)
{
  fprintf(stderr, "%s", ECODEMSG[ecode]);
}

/** 
 * @param *argv[] ukazatel na pole kde su ulozene parametre prik. riadku
 * vrati cislo podla stavu programu z vyctoveho typu stavu programu
 */
int fourParams(char *argv[])
{
    if (strcmp(argv[1], "--vadd") == 0)
        return CVADD;
    if (strcmp(argv[1], "--vscal") == 0)
        return CVSCALL;
    if (strcmp(argv[1], "--mmult") == 0)
        return CMMULT;
    if (strcmp(argv[1], "--mexpr") == 0)
        return CMEXPR;
    if (strcmp(argv[1], "--eight") == 0)
        return CEIGHT;
    return COTHER;
}

/** 
 * @param *argv[] ukazatel na pole kde su ulozene parametre prik. riadku
 * vrati cislo podla stavu programu z vyctoveho typu stavu programu
 */
int threeParams(char *argv[])
{
    if (strcmp(argv[1], "--test") == 0)
        return CTEST;
    if (strcmp(argv[1], "--bubbles") == 0)
        return CBUBBLES;
    return COTHER;
}        

/**
 * Funkcia spracuje argumenty prikazovehoriadku
 * nastavy chybovy kod programu a stavovy 
 * @param argc udava kolko poli znakov(argv) sme zadali
 * @param argv je ukazatel na pole znakov
 * vracia strukturu Tparams
 */
TParams getParams(int argc, char *argv[])
{
   TParams result =
    {
        .ecode = EOK,
        .state = CHELP,
        .fileNameA = NULL,
        .fileNameB = NULL,
    };
    if (argc == 2)
    {
        if (strcmp(argv[1], "-h") == 0)
            result.state = CHELP;
        else
            result.ecode = ECLWRONG;
    }
    else
        if (argc == 3) //--test -h
        {
            if ((result.state = threeParams(argv)) == COTHER)
                result.ecode = ECLWRONG;                
            else
            {
                int length = strlen(argv[2]) + 1; //dlzka nazvu subora
                result.fileNameA = (char*) malloc(length * sizeof(char));
                if (result.fileNameA == NULL)
                    result.ecode = EMALLOC;
                else
                    for (int i = 0; i < length; i ++)
                        result.fileNameA[i] = argv[2][i];//extrahujem cestu do pola
            }            
        }
    else
        if (argc == 4)
        {
            if ((result.state = fourParams(argv)) == COTHER)
                result.ecode = ECLWRONG;
            else
            {
                int length = strlen(argv[2]) + 1; //dlzka nazvu subora
                result.fileNameA = (char*) malloc(length * sizeof(char));
                if (result.fileNameA == NULL)
                    result.ecode = EMALLOC;
                else
                {
                    for (int i = 0; i < length; i++)
                        result.fileNameA[i] = argv[2][i];//extrahujem cestu do pola
                    length = strlen(argv[3]) + 1;// dlzka nazvu druheho subora
                    result.fileNameB = (char*) malloc(length * sizeof(char));
                    if (result.fileNameB == NULL)
                    {
                        free(result.fileNameA);
                        result.ecode = EMALLOC;
                    }
                    else
                        for (int i = 0; i < length; i++)
                            result.fileNameB[i] = argv[3][i]; //extrahujem cestu do pola
                }
            }
        }
    else
        result.ecode = ECLWRONG;           
    return result;
}

/**
 * funkcia alokuje 1D pole pre vektor
 * @param *vector ukazatel na strukturu typu TVector
 * vrati 1 ak sa podarilo alokovat, inak 0
 */
int allocVector(TVector *vector)
{
    vector->array = (int *)malloc(vector->columns * sizeof(int));
    if (vector->array == NULL)
    {
        printECode(EMALLOC);
        return 0;
    }    
    return 1;
}

/** 
 * funkcia alokuje 2D pole pre maticu 
 * @param *matrix ukazatel na strukturu typu TMatrix
 * vrati 1 ak sa podarilo alokovat, inka 0
 */
int allocMatrix(TMatrix *matrix)
{
    matrix->array = (int **)malloc(matrix->rows * sizeof(int*));  //riadky
    if (matrix->array == NULL)
    {
        printECode(EMALLOC);
        return 0;
    }
    for (int i = 0; i < matrix->rows; i++)
    {
        matrix->array[i] = (int *)malloc(matrix->columns * sizeof(int));
        if (matrix->array[i] == NULL) 
        {
            for (int j = 0; j < i; j++)  //dealokujem pole a koncim
                free(matrix->array[j]);
            free(matrix->array); //uvolnim celu            
            printECode(EMALLOC);
            return 0;
        }
    }
    return 1;
} 

/** 
 * funkcia alokuje 3D pole pre vektor matic
 * @param *matrix ukazatel na strukturu typu TVectorMatrix
 * vrati 1 ak sa podarilo alokovat, inak 0
 */
int allocVectorMatrix(TVectorMatrix *matrix)
{
    matrix->array = (int ***)malloc(matrix->number * sizeof(int**)); //matice
    if (matrix->array == NULL)
    {
        printECode(EMALLOC);
        return 0;
    }
    
    for (int i = 0; i < matrix->number; i++)
    {
        matrix->array[i] = (int **)malloc(matrix->rows * sizeof(int*)); //alokujem pocet riadkov v kazdej matici
        if (matrix->array[i] == NULL)
        {
            for (int x = 0; x < i; x++)
                free(matrix->array[x]);
            free(matrix->array);
            printECode(EMALLOC);
            return 0;
        }
    }
    
    for (int i = 0; i < matrix->number; i++)
    {
        for (int j = 0; j < matrix->rows; j++)
        {
            matrix->array[i][j] = (int *)malloc(matrix->columns * sizeof(int));
            if (matrix->array[i][j] == NULL)
            {
                for (int k = 0; k < j; k++)        //dealokujem aktualny riadok
                    free(matrix->array[i][k]);
                
                for (int x = 0; x < i; x++)
                    for (int y = 0; y < matrix->rows; y++)
                        free(matrix->array[x][y]);  //dealokujem ostatne riadky
                        
                free(matrix->array);                
                printECode(EMALLOC);
                return 0;
            }
        }
    }
    return 1;
}

/**
 * funkcia dealokuje maticu
 * @param *matrix ukazatel na 2D pole
 */
void freeMatrix(TMatrix *matrix)
{
    for (int i = 0; i < matrix->rows; i++)
        free(matrix->array[i]);
    free(matrix->array);
}
    
/**
 * funkcia dealokuje 3D pole
 * @param *matrix ukzatel na 3D pole
 */
void freeVectorMatrix(TVectorMatrix *matrix)
{
    for (int i = 0; i < matrix->number; i++)
    {
        for (int j = 0; j < matrix->rows; j++)
            free(matrix->array[i][j]);
    }
    for (int i = 0; i < matrix->number; i++)
        free(matrix->array[i]);
    free(matrix->array); //uvolni matice
}       

/**
 * funkcia nacita vektor do struktury TVector, zo subora filename
 * a uvolni pole fileName
 * @param *vector ukazatel na strukturu TVector
 * @param *fileNam ukazatel na pole kde je ulozeny nazov suboru
 * vrati 1 ak sa podarilo vektor spravne nacitat, inak 0
 * vrati 2 ak sa nepodarilo subor otvorit, inak 0
 */
int readVector(TVector *vector, char *fileName)
{
    int check;
    FILE *fr;
    if ((fr = fopen(fileName, "r")) == NULL)
    {
        printECode(EFILE);
        return 2; 
    }
    check = fscanf(fr, "%d", &vector->object);
    if (check  != 1 || vector->object != 1) 
    { 
        fclose(fr);
        return 0; //skonci ak nenacital jedno cislo alebo nacitane cislo sa nerovna 1
    }    
 
    if ((check = fscanf(fr, "%d", &vector->columns)) != 1 || vector->columns < 1) //citam druhy riadok
    {
        fclose(fr);
        return 0;
    }
    if (allocVector(vector) == 0)
    {
        fclose(fr);
        return 0;
    }
    for (int i = 0; i < vector->columns; i++)   //nacitanie prvkov 
    {
        if ((check = fscanf(fr, "%d", &vector->array[i])) != 1)
        {
            free(vector->array);
            fclose(fr);
            return 0; 
        }
    }
    // testujem ci sa za spravne nacitanimy datami nenachadzaju este nejake prvky
    if (fscanf(fr,"%*s") != EOF)
    {
        free(vector->array);
        fclose(fr);
        return 0;
    }     
    fclose(fr);
    return 1;
}

/**
 * funkcia nacita maticu do struktury TMatrix, zo subora fileName
 * @param *matrix ukazatel na strukturu TMatrix
 * @param *fileName ukazatel na pole kde je ulozeny nazov suboru
 * vrati 1 ak sa podarilo maticu spravne nacitat, inak 0
 * vrati 2 ak sa nepodarilo otvorit subor,, inak 0
 */
int readMatrix(TMatrix *matrix, char *fileName)
{
    int check;
    FILE *fr;
    if (( fr = fopen(fileName, "r")) == NULL)
    {
        printECode(EFILE);
        return 2;
    }

    if ((check = fscanf(fr, "%d", &matrix->object)) != 1 || matrix->object != 2)
    {
        fclose(fr);
        return 0;
    }    
    
    if ((check = fscanf(fr, "%d", &matrix->rows)) != 1 || matrix->rows < 1)  
    {
        fclose(fr);
        return 0;  
    }
    
    if ((check = fscanf(fr, "%d", &matrix->columns)) != 1 || matrix->columns < 1)
    {
        fclose(fr);
        return 0;
    }
    
    if (allocMatrix(matrix) == 0)
    {
        fclose(fr);
        return 0;
    }
    
    for (int j = 0; j < matrix->rows; j++)
    {
        for (int i = 0; i < matrix->columns; i++)
            if ((check = fscanf(fr, "%d", &matrix->array[j][i])) != 1)
            {
                freeMatrix(matrix);
                fclose(fr);
                return 0;
            }
    }
    // testujem ci sa za spravne nacitanimy datami nenachadzaju este nejake prvky
    if (fscanf(fr,"%*s") != EOF)
    {
        freeMatrix(matrix);
        fclose(fr);
        return 0;
    }  
    fclose(fr);
    return 1;
}

/**
 * fukcia nacita vektor matic do struktury TVectorMatrix zo suboru fileName
 * a uvolni pole fileName
 * @param *matrix ukazatel na strukturu TVectorMatrix
 * @param *fileName ukazatel na pole kde je ulozeny nazov suboru
 * vrati 1 ak sa podarilo vektor matic nacitat, inak 0 
 */
int readVectorMatrix(TVectorMatrix *matrix, char *fileName)
{
    int check;
    FILE *fr;
    if (( fr = fopen(fileName, "r")) == NULL)
    {
        printECode(EFILE);
        return 2;
    }        
    
    check = fscanf(fr, "%d", &matrix->object);
    if (check != 1 && matrix->object != 3)
    {
        fclose(fr);
        return 0;
    }
    
    check = fscanf(fr, "%d", &matrix->number);
    if (check != 1 || matrix->number < 2)  // aby slo o vektor matic tak musi tam byt viac nez 1 matica
    {
        fclose(fr);
        return 0; 
    }
    
    check = fscanf(fr, "%d", &matrix->rows);
    if (check != 1 || matrix->rows < 1)  // aby slo o maticu musi byt riadkov viac nez 1
    {
        fclose(fr);
        return 0;
    }
    
    check = fscanf(fr, "%d", &matrix->columns);
    if (check != 1 || matrix->columns < 1)  // aby slo o maticu musi byt pocet stlpcov vacsi nez 1
    {
        fclose(fr);
        return 0; 
    }
    
    if (allocVectorMatrix(matrix) == 0)
    { 
        fclose(fr);
        return 0;
    }

    for (int k = 0; k < matrix->number; k++)
    {
        for (int j = 0; j < matrix->rows; j++)
        {
            for (int i = 0; i < matrix->columns; i++)
                if ((check = fscanf(fr, "%d", &matrix->array[k][j][i])) != 1)
                {
                    freeVectorMatrix(matrix);
                    fclose(fr);
                    return 0;
                }
        }
    }
    // testujem ci sa za spravne nacitanimy datami nenachadzaju este nejake prvky
    if (fscanf(fr,"%*s") != EOF)
    {
        freeVectorMatrix(matrix);
        fclose(fr);
        return 0;
    }  
    fclose(fr);
    return 1;
}

/**
 * funkcia vytlaci na stdout vektor
 * @param *vector ukazatel na strukturu TVector
 */
void printVector(TVector *vector)
{
    printf("%d\n", vector->object);
    printf("%d\n", vector->columns);
    for (int i = 0; i < vector->columns; i++)
        printf("%d ", vector->array[i]);
    printf("\n");
}

/** 
 * funkcia vytlaci na stdout maticu
 * @param *matrix ukazatel na strukturu TMatrix
 */
void printMatrix(TMatrix *matrix)
{
    printf("%d\n", matrix->object);
    printf("%d %d\n", matrix->rows, matrix->columns);
    for (int j = 0; j < matrix->rows; j++)
    {
        for (int i = 0; i < matrix->columns; i++)
            printf("%d ", matrix->array[j][i]);
        printf("\n");
    }
}

/**
 * funkcia vytlaci na stdout vektor matic
 * @param *matrix ukazatel na strukturu TVectorMatrix
 */
void printVectorMatrix(TVectorMatrix *matrix)
{
    printf("%d\n", matrix->object);
    printf("%d %d %d\n", matrix->number, matrix->rows, matrix->columns);
    for (int k = 0; k < matrix->number; k++)
    {
        for (int j = 0; j < matrix->rows; j++)
        {
            for (int i =0; i < matrix->columns; i++)
                printf("%d ", matrix->array[k][j][i]);
            printf("\n");
        }
        printf("\n");
    }
}

/**
 * fukncia otestuje subor ci sa v nom nachadza vektor/matica/vektor mativ
 * @param *fileName ukazatel na pole kde je ulozeny nazov suboru
 * vrati 0 ak sa nepodarilo otvorit subor, inak 1
 */
int testFile(char *fileName)
{
    int object = 0;
    int check;
    FILE *fr;
    if ((fr = fopen(fileName, "r")) == NULL)
    {
        printECode(EFILE);
        return 0;
    }   
    //nacita sa aky je to objekt vektor/matica/vektor matic
    check = fscanf(fr, "%d", &object);
    if (check != 1)
    {
        fclose(fr);
        printf("%s",FALSEMSG);
        return 0;
    }
    fclose(fr);
    if (object == 1) //vektor
    {
        TVector vector;
        if (readVector(&vector, fileName) == 1)
        {            
            printVector(&vector);
            free(vector.array);
        }
        else 
            printf("%s",FALSEMSG);       
    }  
    else
        if (object == 2) //matica
        {
            TMatrix matrix;
            if (readMatrix(&matrix, fileName))
            {
                printMatrix(&matrix);
                freeMatrix(&matrix);
            }
            else
                printf("%s",FALSEMSG);
        }
        else 
            if (object == 3) //vektor matic
            {
                TVectorMatrix matrix;
                if (readVectorMatrix(&matrix, fileName))
                {
                    printVectorMatrix(&matrix);
                    freeVectorMatrix(&matrix);
                }
                else
                    printf("%s",FALSEMSG);
            }
            else
                printf("%s",FALSEMSG);
    return 1;
}

/**
 * funkcia dostane 2 vektory, spocita vyraz (a+b) a vysledok ulozi do vectorResult
 * @param *vectorA vektor ktory bude spocitavat
 * @param *vectorB vektor ktory bude pripocitavat k vectorA
 * @param *vectorResult struktura do ktorej ulozi vysledok 
 */
void vectorAdd(TVector *vectorA, TVector *vectorB, TVector *vectorResult)
{
    for (int i = 0; i < vectorA->columns; i++)
        vectorResult->array[i] = vectorA->array[i] + vectorB->array[i];
}

/**
 * funkcia spocita 2 vektory zo suborov filenameA a filenameB
 * @param *filenameA ukazatel na pole kde je ulozeny nazov suboru filenameA
 * @param *fileNameB ukazatel na pole kde je ulozeny nazov suboru fileNameB
 * fukncia vrati 1 ak sa vektory spocitali, inak 0
 */
int vectorAddInit(char *fileNameA, char *fileNameB)
{
    TVector vectorA, vectorB;
    int stateFile = readVector(&vectorA, fileNameA);    
    if (stateFile == 0 || stateFile == 2) // 0 - data neboli v poriadku
    {//2 - neotvorilo suvor
        if (stateFile == 0)            
            printECode(EFILEDATA);
        return 0;
    }    
    stateFile = readVector(&vectorB, fileNameB);
    if (stateFile == 0 || stateFile == 2)
    {
        if (stateFile == 0)
            printECode(EFILEDATA);
        free(vectorA.array);
        return 0;
    }
    
    if (vectorA.columns != vectorB.columns)
    {
        free(vectorA.array);
        free(vectorB.array);
        printf("%s", FALSEMSG);
        return 0;
    }
    TVector vectorResult;
    vectorResult.object = vectorA.object;
    vectorResult.columns = vectorB.columns;
    if (allocVector(&vectorResult) == 0)
    {
        free(vectorA.array);
        free(vectorB.array);
        return 0;
    }    
    vectorAdd(&vectorA, &vectorB, &vectorResult);
    printVector(&vectorResult);  
    free(vectorResult.array);
    free(vectorA.array);
    free(vectorB.array);
    return 1;
}

/**
 * funkcia pocita skalarny sucin vektorov vektorA a vektorB
 * @param *vectorA ukazatel na vektorA
 * @param *vectorB ukazatel na vektorB
 * funkcia vrati skalarny sucin dvoch stupnych vektorov * 
 */
int vectorScalar(TVector *vectorA, TVector *vectorB)
{
    int scalar = 0;
    for (int i = 0; i < vectorA->columns; i++)
        scalar += vectorA->array[i] * vectorB->array[i];
    return scalar;    
}

/**
 * funckia vypita skalarny sucin dvoch vektorov
 * @param *filenameA ukazatel na pole kde je ulozeny nazov suboru filenameA
 * @param *fileNameB ukazatel na pole kde je ulozeny nazov suboru fileNameB
 * vrati 1 ak sa urobil skalarny sucin dvoch vektorov, inak 0
 */
int vectorScalarInit(char *fileNameA, char *fileNameB)
{
    TVector vectorA, vectorB;
    
    int stateFile = readVector(&vectorA, fileNameA);    
    if (stateFile == 0 || stateFile == 2) // 0 - data neboli v poriadku
    {//2 - neotvorilo suvor
        if (stateFile == 0)            
            printECode(EFILEDATA);
        return 0;
    }    
    stateFile = readVector(&vectorB, fileNameB);
    if (stateFile == 0 || stateFile == 2)
    {
        if (stateFile == 0)
            printECode(EFILEDATA);
        free(vectorA.array);
        return 0;
    }    
   
    if (vectorA.columns != vectorB.columns)
    {
        free(vectorA.array);
        free(vectorB.array);
        printf("%s", FALSEMSG);
        return 0;
    }
    
    int scalar = vectorScalar(&vectorA, &vectorB);    
    printf("%d\n", scalar);
    free(vectorA.array);
    free(vectorB.array);
    return 1;
}

/**
 * funkcia pocita maticovy vyraz (A*B), vyslednu maticu ulozi do struktury matrixResult 
 * @param *matrixA ukazatel na maticu A
 * @param *matrixB ukazatel na maticu B
 * @param *matrixResult ukazatel na vyslednu maticu kde ulozi vysledok
 */
void matrixMulAB(TMatrix *matrixA, TMatrix *matrixB, TMatrix *matrixResult)
{
    for (int i = 0; i < matrixResult->rows; i++)
        for (int j = 0; j < matrixResult->columns; j++)//nulujem maticu
            matrixResult->array[i][j] = 0;
        
    for (int i = 0; i < matrixResult->rows; i++)
        for (int j = 0; j < matrixResult->columns; j++)
        {
            for (int x = 0; x < matrixA->columns; x++)            
                matrixResult->array[i][j] += matrixA->array[i][x] * matrixB->array[x][j]; 
        }   
}
/**
 * funkcia vynasobi dve matice (A*B), 
 * ktore sa nachadzaju v suboroch filenameA a fileNameB 
 * @param *filenameA ukazatel na pole kde je ulozeny nazov suboru filenameA
 * @param *fileNameB ukazatel na pole kde je ulozeny nazov suboru fileNameB
 * vrati 1 ak sa operacia spravne previedla, inak 0
 */
int matrixMulABInit(char *fileNameA, char *fileNameB)
{
    TMatrix matrixA, matrixB;
    int stateFile = readMatrix(&matrixA, fileNameA);
    if (stateFile == 0 || stateFile == 2)
    {
        if (stateFile == 0)
            printECode(EFILEDATA);
        return 0;
    }
    stateFile = readMatrix(&matrixB, fileNameB);
    if (stateFile == 0 || stateFile == 2)
    {
        if (stateFile == 0)
            printECode(EFILEDATA);
        freeMatrix(&matrixA);
        return 0;
    }  
    
    if (matrixA.columns != matrixB.rows) //test ci sa matice mozu nasobit
    {
        freeMatrix(&matrixA);
        freeMatrix(&matrixB);
        printf("%s", FALSEMSG);
        return 0;
    }
    TMatrix matrixResult;
    matrixResult.object = matrixA.object;
    matrixResult.rows = matrixA.rows;
    matrixResult.columns = matrixB.columns;
    if ((allocMatrix(&matrixResult)) == 0)
    {
        freeMatrix(&matrixA);
        freeMatrix(&matrixB);
        return 0;
    }
    matrixMulAB(&matrixA, &matrixB, &matrixResult);
    printMatrix(&matrixResult);
    freeMatrix(&matrixA);
    freeMatrix(&matrixB);
    freeMatrix(&matrixResult);    
    return 1;
}

/**
 * funkcia vypocita maticovy vyraz (A*B*A)
 * matice cerpa zo suborov fileNamA a fileNameB
 * @param *filenameA ukazatel na pole kde je ulozeny nazov suboru filenameA
 * @param *fileNameB ukazatel na pole kde je ulozeny nazov suboru fileNameB
 * vrati 1 ak sa operacia spravne previedla, inak 0
 */
int matrixMulABAInit(char *fileNameA, char *fileNameB)
{
    TMatrix matrixA, matrixB;
    int stateFile = readMatrix(&matrixA, fileNameA);
    if (stateFile == 0 || stateFile == 2)
    {
        if (stateFile == 0)
            printECode(EFILEDATA);
        return 0;
    }
    stateFile = readMatrix(&matrixB, fileNameB);
    if (stateFile == 0 || stateFile == 2)
    {
        if (stateFile == 0)
            printECode(EFILEDATA);
        freeMatrix(&matrixA);
        return 0;
    } 
    
    if (matrixA.columns != matrixB.rows)
    {
        freeMatrix(&matrixA);
        freeMatrix(&matrixB);
        printf("%s", FALSEMSG);
        return 0;
    }
    TMatrix matrixResult;
    matrixResult.object = matrixA.object;
    matrixResult.rows = matrixA.rows;
    matrixResult.columns = matrixB.columns;
    if ((allocMatrix(&matrixResult)) == 0)
    {
        freeMatrix(&matrixA);
        freeMatrix(&matrixB);
        return 0;
    }
    // zavolam funkciu na pocitanie (A*B) vysledok bude v matrixResult
    matrixMulAB(&matrixA, &matrixB, &matrixResult);
    freeMatrix(&matrixB);
    if (matrixResult.columns != matrixA.rows)
    {
        freeMatrix(&matrixA);
        freeMatrix(&matrixResult);
        printf("%s", FALSEMSG);
        return 0;
    }
    TMatrix matrixResultFinal;
    matrixResultFinal.object = matrixA.object;
    matrixResultFinal.rows = matrixA.rows;
    matrixResultFinal.columns = matrixB.columns;
    if ((allocMatrix(&matrixResultFinal)) == 0)
    {
        freeMatrix(&matrixA);
        freeMatrix(&matrixResult);
        return 0;
    }
    //zavolam funkciu na pocitanie (A*B) vysledok bude v matrixResultFinal
    matrixMulAB(&matrixResult, &matrixA, &matrixResultFinal);
    printMatrix(&matrixResultFinal);
    freeMatrix(&matrixA);
    freeMatrix(&matrixResult);
    freeMatrix(&matrixResultFinal);   
    return 1;
}

/**
 * funkcia patri k funkcii findVectorInMatrix
 * @param atom prvok z pola matice
 * @param index udava na ktorom sme indexe v hladadom vektore
 * @param *vector ukazatel na pole v ktorom je vektor ktory hladame v matici
 * @param length pocet stlpcov vektora
 * ak sa atom v matici zhodoval v prvok v vektore vrati index
 * dalsieho prvku v vektore, inak 0 
 */
int locateVector(int atom, int index, int *vector, int length)
{ 
    if (atom == vector[index] || atom == vector[(length - 1) - index])
    {
        index++;
        return index;
    }    
    return 0;
}

/**
 * funkcia patri k funkcii findVectorInMatrix LtoR- left to right
 * prejde maticu po diagonalach z laveho horneho rohu do praveho dolneho
 * @param *vector ukazatel na pole v ktorom je ulozeny hladany vektor
 * @param *matrix ukazatel na maticu v kotorej hlada vektor
 * vrati 1 ak najde vektor, inak 0
 */
int matrixDiagonalLtoR(TVector *vector, TMatrix *matrix)
{
    int index = 0;
    int oldI = 0;
    int oldJ = 0;
    int i = 0;
    int j = 0;
    while ( i != (matrix->rows-1) || j != (matrix->columns-1))
    { 
        index = locateVector(matrix->array[i][j], index, vector->array, vector->columns);
        if (index == vector->columns)
            return 1;        
         if (i == 0 || j == matrix->columns - 1)
         {
             index  = 0;
             if (oldI == matrix->rows -1) //ak som bol dole
             {
                 if ( oldJ == matrix->columns - 2)
                 {
                     i++;
                 }
                     
                 oldJ++;
                 j = oldJ;
                 i = oldI;
             }
             else
             {
                 oldI++;
                 i = oldI;
                 j = 0;                
             }
         }
         else
         {
             j++;
             i--;
         }
    }
    return 0;
}

/**
 * funkcia patri k funkcii findVectorInMatrix  TtoL right to left
 * prejde maticu po diagonalach z praveho horneho rohu do laveho dolneho
 * @param *vector ukazatel na pole v ktorom je ulozeny hladany vektor
 * @param *matrix ukazatel na maticu v kotorej sa hlada vektor
 * vrati 1 ak najde vektor, inak 0
 */
int matrixDiagonalRtoL(TVector *vector, TMatrix *matrix)
{    
    int index = 0;
    int oldI = 0;
    int oldJ = matrix->columns - 1;
    int i = 0;
    int j = matrix->columns - 1;
    while ( i != matrix->rows - 1 || j != 0)
    {
        index = locateVector(matrix->array[i][j], index, vector->array, vector->columns);
        if (index == vector->columns)
            return 1;         
         if (i == 0 || j == 0)
         {
             index = 0;
             if (oldI == matrix->rows -1) //ak som bol dole
             {
                 if ( oldJ == 2)
                 {
                     i++;
                 }
                     
                 oldJ--;
                 j = oldJ;
                 i = oldI;
             }
             else
             {
                 oldI++;
                 i = oldI;
                 j = matrix->columns - 1;                
             }
         }
         else
         {
             j--;
             i--;
         }
    }   
    return 0;
}

/**
 * funkcia pratri k funkcii findVectorInMatrix
 * funkcia prejde maticu po riadkoch a stlpcoch
 * hlada v matici vektor 
 * @param *vector ukazatel na pole v ktorom je ulozeny hladany vektor
 * @param *matrix ukazatel na maticu v ktorej sa hlada vektor
 * vtati 1 ak najde vektor, inak 0
 */
int matrixRowsColumns(TVector *vector, TMatrix *matrix)
{
    int index = 0; 
    for (int i = 0; i < matrix->rows; i++)
    {
        for (int j = 0; j < matrix->columns; j++)
        {
            index = locateVector(matrix->array[i][j], index, vector->array, vector->columns);
            if (index == vector->columns)
            {              
                return 1;
            }
        }
        index = 0; 
    }
    index = 0;//priechod zhora dole
    for (int i = 0; i < matrix->columns; i++)
    {
        for (int j = 0; j < matrix->rows; j++)
        {
            index = locateVector(matrix->array[j][i], index, vector->array, vector->columns);
            if (index == vector->columns)
            {
                return 1;
            }
        }
        index = 0;
    }    
    return 0;
}

/**
 * funkcia hlada vektor v matici ak ho najde vypise true ak nie false
 * @param *fileNameA ukazatel na subor kde je vektor
 * @param *fileNameB ukazatel na subor kde je matica
 * vrati 0 ak sa nepodarilo uspesne nacitat maticu alebo vektor
 * inak vrati 1;
 */
int findVectorInMatrix(char *fileNameA, char *fileNameB)
{
    TVector vector;
    int state = readVector(&vector, fileNameA);
    if (state == 0 || state == 2)
    {
        if (state == 0)
            printECode(EFILEDATA);
        return 0;
    }
    
    TMatrix matrix;
    state = readMatrix(&matrix, fileNameB);
    if (state == 0 || state == 2)
    {
        if (state == 0)
            printECode(EFILEDATA);
        free(vector.array);
        return 0;
    }    
    
    state = matrixRowsColumns(&vector, &matrix); 
    if (matrix.rows > 1 && matrix.columns > 1 && state != 1)
    {
        state = matrixDiagonalRtoL(&vector, &matrix);
        if (state != 1)
            state = matrixDiagonalLtoR(&vector, &matrix);
    }
    
    if (state == 1)
        printf("%s", TRUEMSG);
    else
        printf("%s", FALSEMSG);
    
    freeMatrix(&matrix);
    free(vector.array);
    return 1;
}

/**
 * funkcia patri k funkcii matrixBubbles
 * @param *atom prvok matice v ktorej hladame bublinu 
 * @param *index prvok pola kde mam ulozeny ci sa v predchodzom riadku
 * na tom mieste nachadzala bublina
 * @param *inBuble ak inBuble = 1 znamena ze atom je v bubline
 * @param *identificator 1D pole kde ukladame 
 * @param j aktualny index stlpca
 * @param *value hodnota ktora sa priradzuje do pla indetifikatorov je vzdy unikatna
 * @param columns pocet stlcov matice v ktorej hladame bubliny 
 * vrati 1 ak atom bol rovny nule a index taktiez bol rovny nule
 * a predosla vratena honota bola rovna 0;
 */
//    bubbleFind(matrix.array[i][j], &index[j], &oldRet, &inBuble, &identificator, j, matrix.columns, &value);
int bubbleFind(int atom, int *index, int *inBuble, int identificator[], int j, double *value, int columns)
{   
    if (atom != 0)
    {
        *index = 0;
        *inBuble = 0;
        identificator[j] = 0;
        return 0;        
    }    
//     najdem bublinu
    if (atom == 0 && *index == 0 && *inBuble == 0)
    {
        *index = 1;
        *inBuble = 1;     
        *value += 1; 
        identificator[j] = *value;
        return 1;
    }
//     som v bubline
    if (atom == 0 && *index == 0 && *inBuble == 1)
    {
        *index = 1;
        *inBuble = 1;
        identificator[j] = identificator[j-1]; 
        return 0;
    }    
    if (atom == 0 && *index == 1 && *inBuble == 0)           //&& *inBuble == 0
    {
        *index = 1;
        *inBuble = 1;
        return 0;
    }      
    if (atom == 0 && *index == 1 && *inBuble == 1 )
    {

        if (identificator[j - 1] != identificator[j])
        {
            *index = 1;
            *inBuble = 1;
            int k = identificator[j - 1];
            int h = identificator[j];
            int x = (identificator[j] < identificator[j - 1]) ? identificator[j] : identificator[j - 1]; // v x mam mensiu hodnotu identificatorov
            for (int f = 0; f < columns; f++) //prejdem cele pole indetifikatorov a zmenim na x, kde predchodzi alebo aktualny
            {
                if (identificator[f] == k || identificator[f] == h)
                    identificator[f] = x;
            } 
            return -1;            
        }
        else
        {
            *index = 1;
            *inBuble = 1;
            return 0;
        }
    }     
    return 0;    
}

/**
 * funkcia hlada v matici miesta s prvkom 0 - bubliny 
 * funkcia nacita maticu do struktury TMatrix 
 * potom prechadza maticu po riadkoch a pri kazdom 
 * prvku sa zavola funkcia bubbleFind ktora hlada bubliny
 * @param *filename subor v krotom sa nachadza matica
 * vracia
 */
int matrixBubbles(char *fileName)
{
    TMatrix matrix;
    int stateFile = readMatrix(&matrix, fileName);
    if (stateFile == 0 || stateFile == 2)
    {
        if (stateFile == 0)
            printECode(EFILEDATA);
        return 0;
    }
    
    int *index;
    index = (int *)malloc(matrix.columns * sizeof(int));
    if (index == NULL)
    {
        freeMatrix(&matrix);
        printECode(EMALLOC);
        return 0;
    }  
    int *identificator;
    identificator = (int *)malloc(matrix.columns * sizeof(int));
    if (identificator == NULL)
    {
        freeMatrix(&matrix);
        free(index);
        printECode(EMALLOC);
        return 0;
    }
    
    double value = 0;
    for (int i = 0; i < matrix.columns; i++)
    {
        index[i] = 0;
        identificator[i] = value;
    }    
    int bubbles = 0;
    int inBuble = 0;
    for (int i = 0; i < matrix.rows; i++)
    {
        for (int j = 0; j < matrix.columns; j++)
            bubbles += bubbleFind(matrix.array[i][j], &index[j], &inBuble, identificator, j, &value, matrix.columns);
        inBuble = 0;
    }   
    
    printf("%d\n", bubbles);
    freeMatrix(&matrix);
    free(index); 
    free(identificator);
    return 1;
}

/**
 * Hlavny program
 */
int main(int argc, char *argv[])
{
    TParams params = getParams(argc,argv);
    
    if (params.ecode != EOK)
    {
        printECode(params.ecode);
        return EXIT_FAILURE;
    }
    
    if (params.state == CHELP)
        printf("%s", HELPMSG);
    
    if (params.state == CTEST)
        testFile(params.fileNameA);

    if (params.state == CVADD)
        vectorAddInit(params.fileNameA, params.fileNameB);
    
    if (params.state == CVSCALL)
        vectorScalarInit(params.fileNameA, params.fileNameB);
    
    if (params.state == CMMULT)
        matrixMulABInit(params.fileNameA, params.fileNameB);
    
    if (params.state == CMEXPR)
        matrixMulABAInit(params.fileNameA, params.fileNameB);
    
    if (params.state == CEIGHT)
        findVectorInMatrix(params.fileNameA, params.fileNameB);
    
    if (params.state == CBUBBLES)
        matrixBubbles(params.fileNameA);

    if (params.fileNameA != NULL)
        free(params.fileNameA);

    if (params.fileNameB != NULL)
        free(params.fileNameB);       
    return 0;
}
