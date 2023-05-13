//bruteforceNaive.c
//Tambien cifra un texto cualquiera con un key arbitrario.
//OJO: asegurarse que la palabra a buscar sea lo suficientemente grande
//  evitando falsas soluciones ya que sera muy improbable que tal palabra suceda de
//  forma pseudoaleatoria en el descifrado.
//>> mpicc bruteforce.c -o desBrute
//>> mpirun -np <N> desBrute

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <rpc/des_crypt.h>	


void sieve(char* cipher, int ciphlen);

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
  double tstart, tend;

  if (argc > 1)
    the_key = strtol(argv[1], 0, 10);
  

  //INIT MPI
  MPI_Init(NULL, NULL);
  MPI_Comm comm = MPI_COMM_WORLD;
  MPI_Comm_size(comm, &N);
  MPI_Comm_rank(comm, &id);

  if (id == 0)
    printf("Searching for the key: %ld\n", the_key);

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

  //non blocking receive, revisar en el for si alguien ya encontro
  MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

  sieve(cipher, ciphlen);

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


void sieve(char* cipher, int ciphlen)
{
  long upper = (1L <<56); 
  long lower_bound;
  
  long key;

  //Primer proceso trata a 2^56 y 2^56-1 como casos especiales
  // porque no los cubre el resto del proceso
  if (id == 0)
  {
    if(tryKey(upper, cipher, ciphlen))
      {
        found = upper;
        printf("Process %d found the key %ld\n", id, upper);
      
        for(int node=0; node<N; node++){
          MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD); //avisar a otros
        }
        return;
      }
  }
  // la cantidad de veces que se parte el problema.
  long step_factor = 2;
  for(int i = 0; i < 56; i++)
  {
    MPI_Test(&req, &ready, MPI_STATUS_IGNORE);
    if(ready)
      return;  //ya encontraron, salir
    long step = upper/step_factor;

    lower_bound = step* 2 * id;
    //lower_bound = range_per_node * id;
    //upper_bound = range_per_node * (id+1) -1;
    long step_amount = step_factor/2 / N  < 1 ?  1 : step_factor/2 / N;
    if (step_amount < id)
    {
      //printf("Process %d skipped iter:\t%d\n", id, i);
      step_factor*=2;
      continue;
    }
    key = lower_bound + step;
    //if (id == 0)
    //  printf("Iter: %d\n", i);
    for(long j = 0;  j < step_amount; j++)
    {
      //if (id == 3)
      //printf("Process %d:\t%ld\t%d\t%ld\n", id, key, i, step_factor);
      MPI_Test(&req, &ready, MPI_STATUS_IGNORE);
      if(ready)
        return;  //ya encontraron, salir

      if(tryKey(key, cipher, ciphlen))
      {
        found = key;
        printf("Process %d found the key %ld\n", id, key);
      
        for(int node=0; node<N; node++){
          MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD); //avisar a otros
        }
        return;
      }
      
      MPI_Test(&req, &ready, MPI_STATUS_IGNORE);
      if(ready)
        return; 
      //Probar con el número impar adyacente
      if(tryKey(key-1, cipher, ciphlen))
      {
        found = key-1;
        printf("Process %d found the key\n", id);
        for(int node=0; node<N; node++){
          MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD); //avisar a otros
        }
        return;
      }
      key+=step*2;
    }
    step_factor *= 2;
  }
}