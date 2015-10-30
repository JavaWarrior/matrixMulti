#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <error.h>
#include <sys/time.h>
#include <string.h>

#define ERROR_MATRIX_SIZE_MISMATCH 1
#define ERROR_RESERVING_MAT 2
#define ERROR_FILE_NOT_FOUND 3
#define SUCCESS 0

int N=0,M=0,K;
double ** resMatRowMethod,**resCellMethod,**mat1,**mat2;
//----
char line[1000];
char * mat1FileName , * mat2FileName,*outFileName,*newOutFileName;
FILE * outputFile;
//----
struct timeval curTime1,curTime2;
//----
int cnt = 0,mxCnt = 0;

struct unionStruct{
    int x,y;
};
//prototypes
void printOutput(double ** mtrx , FILE * fp); // prints matrix mtrx to file fp
void printTime(); // prints time difference between curTime1 and curTime2 in output file and stdout


int readMat(); // reads matrices mat1 , mat2 and allocates result matrices

void parseSize(char * arr, int * x, int * y); // parses the "row=x col=y" from the file
int reserveMatrices();//allocates matrices mat1,mat2,res
double ** reserveMatrixSize(int n , int m); //return a pointer to a matrix of size n ,m
void load(FILE * fp , double ** mtrx , int x, int y);//loads values from file fp to matrix mtrx with size x*y;

void execCell();//executes cell method
void execRow(); //executes row method
void startCellMethod(); // launches threads for  cell method
void startRowMethod(); // launches threads for  row method

void * calcCell(void * data);//one thread function that calculate each cell in the result matrix
void * calcRow(void * data);//one thread function that calculate each row in the result matrix

//main
int main(int argv, char ** argc)
{
    if(argv != 4 && argv != 1){
        perror("Wrong number of parameters");return 0;
    }

    if(argv == 1){
        mat1FileName = "a.txt";
        mat2FileName = "b.txt";
        outFileName = "c";
    }else{
        mat1FileName = argc[1];
        mat2FileName = argc[2];
        outFileName = argc[3];
    }
    int retVal = readMat();
    if(retVal == ERROR_MATRIX_SIZE_MISMATCH){
        perror("invalid matrices size");
    }else if(retVal == ERROR_RESERVING_MAT){
        perror("error reserving matrices");
    }else if (retVal == ERROR_FILE_NOT_FOUND){
        perror("one of or both input files not found");
    }else{
        newOutFileName = malloc(strlen(outFileName) + 4);
        strcpy(newOutFileName,outFileName);
        strcat(newOutFileName,"_1");
        outputFile = fopen(newOutFileName,"w");
        execRow();
        fclose(outputFile);
        strcpy(newOutFileName,outFileName);
        strcat(newOutFileName,"_2");
        outputFile = fopen(newOutFileName,"w");
        execCell();
        fclose(outputFile);
    }
    return 0;
}

//definitions
void printOutput(double ** mtrx ,FILE * fp){
    int i , j ;
    for(i = 0 ; i < N ;i ++){
        for(j = 0 ; j  < M ; j++){
            fprintf(fp,"%lf\t",mtrx[i][j]);
        }
        fprintf(fp,"\n");
    }
}
int readMat(){
    int mat1x ,mat1y , mat2x ,mat2y;
    FILE *fp1 = fopen(mat1FileName,"r");
    FILE *fp2 = fopen(mat2FileName,"r");
    if(fp1 == NULL || fp2 == NULL){
        return ERROR_FILE_NOT_FOUND;
    }
    fscanf(fp1,"row=%d col=%d\n",&mat1x,&mat1y);
    fscanf(fp2,"row=%d col=%d\n",&mat2x,&mat2y);
    if(mat1y != mat2x){
        return ERROR_MATRIX_SIZE_MISMATCH;
    }
    N = mat1x;M=mat2y;K=mat1y;
    int retVal = reserveMatrices();
    if(retVal != SUCCESS)
        return retVal;
    load(fp1,mat1,N,K);
    load(fp2,mat2,K,M);
    fclose(fp1);
    fclose(fp2);
    return SUCCESS;
}


int reserveMatrices(){
    mat1 = reserveMatrixSize(N,K);
    mat2 = reserveMatrixSize(K,M);
    resCellMethod = reserveMatrixSize(N,M);
    resMatRowMethod = reserveMatrixSize(N,M);
    if(mat1 == NULL ||mat2 == NULL ||resCellMethod == NULL ||resMatRowMethod == NULL){
        return ERROR_RESERVING_MAT;
    }
    return SUCCESS;
}
double ** reserveMatrixSize(int n , int m){
    double ** pntr;
    pntr = (double **) malloc(n * sizeof(double *));
    int i;
    for( i =0 ; i < n ;i ++){
        pntr[i] = (double *)calloc(m , sizeof(double));
    }
    return pntr;
}
void load(FILE * fp , double ** mtrx , int x,  int y){
    int i , j;
    for( i =0 ; i < x ; i++){
        for(j = 0 ; j < y ; j++){
            fscanf(fp,"%lf",&mtrx[i][j]);
        }
    }
}

void execCell(){
    cnt=0;mxCnt=0;
    puts("------- Cell Method -------");
    gettimeofday(&curTime1,NULL);
    startCellMethod();
    gettimeofday(&curTime2,NULL);
    printTime();
    printOutput(resCellMethod,outputFile);
    //printOutput(resCellMethod,stdout);
    printf("number of threads is: %d\n",N*M);
}
void execRow(){
    cnt=0;mxCnt=0;
    puts("------- ROW Method -------");
    gettimeofday(&curTime1,NULL);
    startRowMethod();
    gettimeofday(&curTime2,NULL);
    printTime();
    printOutput(resMatRowMethod,outputFile);
    //printOutput(resMatRowMethod,stdout);
    printf("number of threads is: %d\n",N);
}


void startCellMethod(){
    int i , j;
    pthread_t *tempThrds = (pthread_t *) malloc(sizeof(pthread_t)*N*M);
    for(i = 0 ; i < N ; i++){
        for(j = 0 ; j < M ;j++){
            struct unionStruct * tmpstrct = (struct unionStruct *)malloc(sizeof(struct unionStruct));
            tmpstrct->x = i;
            tmpstrct->y = j;
            if(pthread_create(&tempThrds[i*M + j],NULL,calcCell,(void *)tmpstrct)){
                perror("cell thread creation error");
                exit(EXIT_FAILURE);
            }
        }
    }
    for(i = 0 ; i < N*M ; i++){
            pthread_join(tempThrds[i],NULL);
    }
}
void startRowMethod(){
    int i ;
    pthread_t *thrds = (pthread_t *) malloc(sizeof(pthread_t)*N);
    for(i = 0 ; i < N ;i++){
        if(pthread_create(&thrds[i],NULL,calcRow,(void *)i)){
            perror("row thread creation error");
            exit(EXIT_FAILURE);
        }
    }
    for(i = 0 ; i < N ;i++){
        pthread_join(thrds[i],NULL);
    }
}

void* calcRow(void * data){
    int row = (int)data;
    int col =0,k=0;
    for(col = 0 ; col < M ; col++){
        for(k = 0 ; k < K ; k++){
            resMatRowMethod[row][col]+=mat1[row][k] * mat2[k][col];
        }
    }
    pthread_exit(NULL);
}
void * calcCell(void * data){
    int row = ((struct unionStruct *)data)->x , col = ((struct unionStruct *)data)-> y,k =0;
    for(k = 0 ; k < K ;k++){
        resCellMethod[row][col]+= mat1[row][k] * mat2[k][col];
    }
    pthread_exit(NULL);
}


void printTime(){
//    printf(" taken: %lu\n", curTime2.tv_sec - curTime1.tv_sec);
//    fprintf(outputFile,"Seconds taken: %lu\n", curTime2.tv_sec - curTime1.tv_sec);
//    fprintf(outputFile,"Microseconds taken: %lu\n", (curTime2.tv_usec - curTime1.tv_usec));
    printf("Microseconds taken: %lu\n\n", (curTime2.tv_usec+curTime2.tv_sec*((int)1e6) - curTime1.tv_usec-curTime1.tv_sec*((int)1e6)));
}
