#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <rpc/des_crypt.h>	


void DNCbruteForce(char* cipher, int ciphlen, long lower, long upper);

//descifra un texto dado una llave
void decrypt(long key, char *ciph, int len);

//cifra un texto dado una llave
void encrypt(long key, char *ciph);

//palabra clave a buscar en texto descifrado para determinar si se rompio el codigo
char search[] = "es una prueba de";
int tryKey(long key, char *ciph, int len);

long the_key = 123456L;
//2^56 / 4 es exactamente 18014398509481983
//long the_key = 18014398509481983L;
//long the_key = 18014398509481983L +1L;


MPI_Status st;
MPI_Request req;
long found = 0L;
int ready = 0;
int N, id;

int main(int argc, char *argv[]){ //char **argv
  long upper = (1L <<56); //upper bound DES keys 2^56
  long mylower, myupper;
  double tstart, tend;

  if (argc > 1)
    the_key = strtol(argv[1], 0, 10);
  

  //INIT MPI
  MPI_Init(NULL, NULL);
  MPI_Comm comm = MPI_COMM_WORLD;
  MPI_Comm_size(comm, &N);
  MPI_Comm_rank(comm, &id);

  

  char buffer[1024];
  FILE *inputFile;
  int ciphlen;
  if (id == 0)
  {
    inputFile = fopen("foo.txt", "r");

    if (inputFile == 0)
    {
      printf("Couldn't open input file.\n*dies*");
      return -1;
    }

    
    fgets(buffer, 1024, inputFile);
    fclose(inputFile);
    printf("Input:\n%s\n", buffer);
    ciphlen = strlen(buffer);
  }

  //Enviar el cifrado junto con la información necesaria para que cada proceso se provisione de la memoria necesaria
  MPI_Bcast(&ciphlen, 1, MPI_INT, 0, comm);
  char cipher[ciphlen+1];
  if (id == 0)
  {
    //cifrar el texto
    memcpy(cipher, buffer, ciphlen);
    cipher[ciphlen]=0;
    encrypt(the_key, cipher);
  }
  MPI_Bcast(cipher, ciphlen, MPI_CHAR, 0, comm);
  
  tstart = MPI_Wtime();

  //distribuir trabajo de forma naive
  long range_per_node = upper / N;
  mylower = range_per_node * id;
  myupper = range_per_node * (id+1) -1;
  if(id == N-1){
    //compensar residuo
    myupper = upper;
  }
  printf("Process %d lower %ld upper %ld\n", id, mylower, myupper);

  //non blocking receive, revisar en el for si alguien ya encontro
  MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

#define threads_per_process 2
#pragma omp parallel num_threads(threads_per_process)
{
#pragma omp single
{
  DNCbruteForce(cipher, ciphlen, mylower, myupper);
}
}

  //wait y luego imprimir el texto
  if(id==0){
    MPI_Wait(&req, &st);
    tend = MPI_Wtime();
    decrypt(found, cipher, ciphlen);
    printf("Key = %li\n\n", found);
    printf("%s\n", cipher);
    printf("\nTook: %fms\n", (tend-tstart)*1000);
  }
  printf("Process %d exiting\n", id);

  //FIN entorno MPI
  MPI_Finalize();
}

//cantidad de valores por barrer luego de la mitad del rango
#define sweep 100
void DNCbruteForce(char* cipher, int ciphlen, long lower, long upper)
{
  if (lower >= upper) return;
  MPI_Test(&req, &ready, MPI_STATUS_IGNORE);
  if(ready)
    return;  //ya encontraron, salir

  //selección de la mitad del rango como pivote
  long mid = (lower+upper)/2;
  long limit = mid+sweep > upper? upper : mid+sweep;

  //Barrido a partir de la mitad del rango
  for(long i = mid; i < limit; ++i)
    if(tryKey(i, cipher, ciphlen))
    {
      found = i;
      printf("Process %d found the key\n", id);
      for(int node=0; node<N; node++){
        MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD); //avisar a otros
      }
      return;
    }

  /*Recurrir para continuar la solución del trabajo.
  Se crea una tarea para la primer llamada recursiva y se 
  continua con la otra llamada recursiva sin crear una tarea
  porque estamos en el flujo control de una thread que acaba 
  de terminar sus responsabilidades. No tiene sentido incurrir 
  en overhead de orquestamiento cuando esta thread puede hacerlo 
  por su cuenta*/
#pragma omp task if (mid-lower > 100) mergeable
  DNCbruteForce(cipher, ciphlen, lower, mid);
  DNCbruteForce(cipher, ciphlen, mid+sweep+1, upper);
}


int tryKey(long key, char *ciph, int len)
{
  char temp[len+1]; //+1 por el caracter terminal
  memcpy(temp, ciph, len);
  temp[len]=0;	//caracter terminal
  decrypt(key, temp, len);
  return strstr((char *)temp, search) != NULL;
}

void decrypt(long key, char *ciph, int len)
{
  //set parity of key and do decrypt
  long k = 0;
  for(int i=0; i<8; ++i){
    key <<= 1;
    k += (key & (0xFE << i*8));
  }
  des_setparity((char *)&k);  //el poder del casteo y &
  ecb_crypt((char *)&k, (char *) ciph, 16, DES_DECRYPT);
}


void encrypt(long key, char *ciph)
{
  //set parity of key and do encrypt
  long k = 0;
  for(int i=0; i<8; ++i){
    key <<= 1;
    k += (key & (0xFE << i*8));
  }
  des_setparity((char *)&k);  //el poder del casteo y &
  ecb_crypt((char *)&k, (char *) ciph, 16, DES_ENCRYPT);
}