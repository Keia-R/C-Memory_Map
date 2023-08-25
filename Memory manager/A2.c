#include "stdio.h"
#include "stdlib.h"
#include "string.h"

char types[13][20] = {"int *", "int[]", "int", "float *", "float[]", "float", "char **", "char**", "char *", "char[]", "char", "void *", "void"};

char* variableCheck(char currLine[999]){
    char *pHolder = strdup(currLine);
    char firstLetter = pHolder[0];
    char *token;

    while(firstLetter == ' ') {
        pHolder++;
        firstLetter = pHolder[0];}

    for (int i=0; i<13; i++){
        if (strstr(pHolder, types[i]) == pHolder) { return types[i]; }
    }
    
    return NULL;
}

int sizeFinder(char *type, int num){
    int size;

    if (strcmp(type, "int") == 0) size = sizeof(int);
    if (strcmp(type, "int *") == 0) size = sizeof(int *);
    if (strcmp(type, "int[]") == 0) size = sizeof(int *);

    if (strcmp(type, "float") == 0) size = sizeof(float);
    if (strcmp(type, "float *") == 0) size = sizeof(float *);
    if (strcmp(type, "float[]") == 0) size = sizeof(float *);

    if (strcmp(type, "char") == 0) size = sizeof(char);
    if (strcmp(type, "char *") == 0) size = sizeof(char *);
    if (strcmp(type, "char[]") == 0) size = sizeof(char *);
    if (strcmp(type, "char **") == 0) size = sizeof(char **);
    if (strcmp(type, "char**") == 0) size = sizeof(char **);

    return size*num;
}


void addVar(char lists[4][9999], char *currLine, char *name, char *type, int location, char *func, int number){
    //printf("you called addVar\n"); //debug line
    //0=stack, 1=heap, 2=static, 3=ReadOnly
    //name, scope, size

    //Find actual number for arrays
    if (strstr(name, "[") != NULL){
        char * arrayInside = strdup(name);
        char * token = strtok(arrayInside, "[");
        token = strtok(NULL, "[");
        token = strtok(token, "]");
        number = strtol(token, NULL, 10);
        //Get rid of the brackets from the name
        token = strtok(name, "[");
    }

    char info[999];
    //ROD
    if (location == 3 ) {
        //int num = (int) (currLine - strchr(currLine, "\"")); //num is set to index of "
        sprintf(info, "%s  %s  %s  %d*sizeof(char)\n", name, func, type, number);
        strcat(lists[3], info);
    }
    //Static
    if (location == 2) {
        sprintf(info, "%s  %s  %s  %d\n", name, func, type, sizeFinder(type, number));
        strcat(lists[2], info);
    }
    //Heap
    if (location == 1) {
        //Find what's inbetween the brackets of the calloc/malloc
        char pHolder[999];
        strcpy(pHolder, currLine);
        char *token = strtok(pHolder, "(");
        token = strtok(NULL, "(");
        token = strtok(token, ")");
        sprintf(info, "%s  %s  %s  %s\n", name, func, type, token);
        strcat(lists[1], info);
    }
    //Stack
    if (location == 0) {
        sprintf(info, "%s  %s  %s  %d\n", name, func, type, sizeFinder(type, number));
        strcat(lists[0], info);
    }
}

int variableEval(char lists[4][9999], char *currLine, char *returnType, char *func){
    /*function goes through line of code with a variable declaration 
    find variable name calls varAdder for each one
    returns num of variables added from the line
    */
   //fix arrays and const characters

    //Find where variable(s) are stored before editing 
    int memLocation = 0; //0=stack, 1=heap, 2=static, 3=ROD
    //Heap check
    if (strstr(currLine, "calloc") != NULL || strstr(currLine, "malloc") != NULL) memLocation = 1;
    //Static check
    else if (strstr(currLine, "static") != NULL || strcmp(func, "GLOBAL") == 0) memLocation = 2;
    //ROD check
    else if (strcmp(returnType ,"char *") == 0 && strstr(currLine, "=") != NULL && strstr(currLine, "\"") != NULL) memLocation = 3;

    int number = 1;
    if (memLocation ==3){
        char *afterQuote = strstr(currLine, "\"") + 1;
        number = strstr(afterQuote, "\"") - strstr(currLine, "\"");
    }

    //printf("return type: [%s]\n", returnType); //debug line

    //Now skip to after the type declaration
    char *pHolder2 = strstr(currLine, returnType) + strlen(returnType);
    char pHolder[999];
    strcpy(pHolder, pHolder2);
    
    //So at this point we are dealing with everything to the right of the type signature
    //printf("pholder: %s", pHolder); //debug line
    char *token = strtok(pHolder, "="); //Get rid of a potential equals sign
    //Arrays
    //Find number of elements for size calculations

    //Get rid of potential semicolons
    token = strtok(token, ";");

    int variablesAdded = 0;
    //printf("token before loop: [%s], location: %d\n", token, memLocation); //debug line

    //Non list shit
    if (strstr(token, ",") == NULL) {
        addVar(lists, currLine, token, returnType, memLocation, func, number);
        return 1;}

    //Now dealing with list
    token = strtok(token, ",");
    addVar(lists, currLine, token, returnType, memLocation, func, 1);

    while ((token = strtok(NULL, ",") ) != NULL){
        //printf("in loop: [%s]\n", token);    //debug line
        addVar(lists, currLine, token, returnType, memLocation, func, 1);
        variablesAdded++;
    } 
    return variablesAdded;
}

int functionCheck(char *currLine){
    // check if there is a parenthesis also in the first line
    if (strstr(currLine, "(") == NULL) return -1; 
    if (strstr(currLine, "=") != NULL) return -1; 

   return 0;
}

int functionHeader(char lists[4][9999], char *currLine, char* func){
    char *pHolder = strdup(currLine);

    if (strstr(currLine, "(")[1] == ')') return 0;

    //try strsep and _r
    //Step now go to arguments
    //printf("line : %s\n", pHolder);
    char *token = strtok(pHolder, "(");
    token = strtok(NULL, "(");
    token = strtok(token, ")"); //token should be everything between brackets

   // printf("func: %s, arguments: %s\n", func, token);
    //Case 1: only 1 argument
    if (strstr(token, ",") == NULL) return variableEval(lists, token, variableCheck(token), func);

    //Case 2: multiple arguments
    int varsAdded = 0;

    char *listHandler = strdup(token);
    while(strstr(listHandler, ",") != NULL){
        char* currentVar = strdup(listHandler);
        int commaIndex = (int) (strstr(listHandler, ",") - listHandler);
        currentVar[commaIndex] = '\0';

        //printf("current line: [%s]\ncurrent Var: [%s]\n", listHandler, currentVar);
        varsAdded+= variableEval(lists, currentVar, variableCheck(currentVar), func);

        listHandler = strstr(listHandler, ",") + 1;

        //printf("new line: [%s]\n", listHandler);
    }
    
    varsAdded+= variableEval(lists, listHandler, variableCheck(listHandler), func);

    
    return varsAdded;
}

int functionEval(FILE * f, char lists[4][9999], char funcLineData[999], char funcVarData[999], char funcNames[999], char *currLine, char *returnType){
    //We're dealing with a funciton here
    char pHolder[999];
    strcpy(pHolder, currLine);

    //step 1: find function return name
    char *pHolder2 = strstr(currLine, returnType) + strlen(returnType);
    strcpy(pHolder, pHolder2);
    char *token = strtok(pHolder, "("); //before (
    char name[99];
    strcpy(name, token);

    char namepHolder[99];
    sprintf(namepHolder, "%s, ", name);
    strcat(funcNames, namepHolder);

    int varsAdded = 0;
    //Step 2: evaluate arguments
    varsAdded += functionHeader(lists, currLine, name);
    //printf("in func:%s, value after headercall: %d\n\n", name, varsAdded);
//
    //Step 3, rest of function
    int bracketCount = 0;
    if (strstr(currLine, "{") != NULL) bracketCount++; //If there is a { in header add it before moving on
    int lineCount = 0;

    while (fgets(currLine, 9999, f)){
        lineCount++;
        //Check if there is a type declaration
        char *type = variableCheck(currLine);
        if (type != NULL){
           // printf("%s\n", type);
            //printf("found a variable\n");
            variableEval(lists, currLine, type, name);
            varsAdded++;
        }

        if (strstr(currLine, "{") != NULL) bracketCount++;
        if (strstr(currLine, "}") != NULL && bracketCount > 1) bracketCount--;
        else if (strstr(currLine, "}") != NULL && bracketCount == 1) {
            char newData[99];
            sprintf(newData, "%s: %d\n", name, lineCount);
            strcat(funcLineData, newData);
            sprintf(newData, "%s: %d\n", name, varsAdded);
            strcat(funcVarData, newData);
            return lineCount;
        }
    }

    return -1;

}


int main(int argc, char *argv[]){
    FILE *f = fopen(argv[1], "r"); //Open file for reading
    char currLine[9999]; //Get a string to read through the file with

    //Make an array that stores strings that hold all the data for their respective memory addresses.
    char lists[4][9999];
    sprintf(lists[0], "### stack ###\n");
    sprintf(lists[1], "### heap ###\n");
    sprintf(lists[2], "### static data ###\n");
    sprintf(lists[3], "### ROData ###       scope type  size\n");

    char funcLineData[999];
    strcat(funcLineData, "- Total number of lines per functions:\n");
    char funcVarData[999];
    strcat(funcVarData, "- Total number of variables per function:\n");
    char funcNames[999];
    int functionCount = 0;
    char stats[999];
    sprintf(stats, "**** STATS ****\n");

    int lineCount = 0;
    while (fgets(currLine, 9999, f)){
        //printf("line number: %d\n", lineCount);
        lineCount++;
        //Check if there is a type declaration
        char *type = variableCheck(currLine);
        if (type != NULL){
           // printf("%s\n", type);
            //function check
            if (functionCheck(currLine) == 0) {
               // printf("found a function\n");
                functionCount++;
                lineCount += functionEval(f, lists, funcLineData, funcVarData, funcNames, currLine, type);
            }
            else {
                //printf("found a variable\n");
                variableEval(lists, currLine, type, "GLOBAL");
            }
        }
    }

    char pHolder[999];
    sprintf(pHolder, "Total number of lines in the file: %d\n", lineCount);
    strcat(stats, pHolder);
    sprintf(pHolder, "Total number of functions: %d\n", functionCount);
    strcat(stats, pHolder);
    sprintf(pHolder, "%s\n", funcNames); //print all function names
    strcat(stats, pHolder);
    //add list of function lines and vars
    strcat(stats, funcLineData);
    strcat(stats, funcVarData);

    sprintf(pHolder, "\n\n####################\n####buffer space#### \n####################\n\n");
    strcat(lists[1], pHolder);

    //When done reading the file, print the data
    printf("\n***  exec // text ***\n");
    printf("%s\n\n", argv[1]);
    for (int i =3; i>=0; i--) printf("%s\n", lists[i]);

    printf("%s \n", stats);

    return 0;
}