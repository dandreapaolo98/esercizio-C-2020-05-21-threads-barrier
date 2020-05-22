/*
 * esercizio-C-2020-05-21-threads-barrier.c
 *
 *  Created on: May 19, 2020
 *      Author: marco
 */
/**********TESTO ESERCIZIO**********
un processo apre un file in scrittura (se esiste già sovrascrive i contenuti del file),
poi lancia M (=10) threads.

"fase 1" vuol dire: dormire per un intervallo random di tempo compreso tra 0 e 3 secondi,
					poi scrivere nel file il messaggio: "fase 1, thread id=, sleep period= secondi"

"fase 2" vuol dire: scrivere nel file il messaggio "fase 2, thread id=, dopo la barriera"
					poi dormire per 10 millisecondi, scrivere nel file il messggio "thread id= bye!".

per ogni thread: effettuare "fase 1", poi aspettare che tutti i thread abbiano completato la fase 1
(barriera: little book of semaphores, pag. 29); poi effettuare "fase 2" e terminare il thread.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>

#define CHECK_ERR(a,msg) {if ((a) == -1) { perror((msg)); exit(EXIT_FAILURE); } }
#define M 10

sem_t * mutex;
sem_t * barrier;

sem_t * mutex_file;

int count;
int fd;

void * thread_function(void * arg);
int randomNumber(int lower, int upper);	//funzione per la generazione casuale di un intero in [lower, upper]

int main(int argc, char * argv[]) {
	pthread_t threads[M];
	int s;

	char * file_name;
	char * text_to_write;
	int text_to_write_len;

	/*if (argc == 1) {
		printf("specificare come parametro il nome del file da creare e su cui scrivere\n");
		exit(EXIT_FAILURE);
	}

	file_name = argv[1];
	printf("scrivo nel file %s\n", file_name);
	 */

	file_name = "/home/utente/es2020_05_21";
	fd = open(file_name,
				  O_CREAT | O_TRUNC | O_WRONLY,
				  S_IRUSR | S_IWUSR // l'utente proprietario del file avrà i permessi di lettura e scrittura sul nuovo file
				 );
	if (fd == -1) { // errore!
		perror("open()");
		exit(EXIT_FAILURE);
	}

	barrier = malloc(sizeof(sem_t));
	mutex = malloc(sizeof(sem_t));
	mutex_file = malloc(sizeof(sem_t));

	s = sem_init(barrier,
					0, // 1 => il semaforo è condiviso tra processi,
					   // 0 => il semaforo è condiviso tra threads del processo
					0 // valore iniziale del semaforo
				  );

	CHECK_ERR(s,"sem_init")

	s = sem_init(mutex,
					0, // 1 => il semaforo è condiviso tra processi,
					   // 0 => il semaforo è condiviso tra threads del processo
					1 // valore iniziale del semaforo
				  );

	CHECK_ERR(s,"sem_init")

	s = sem_init(mutex_file,
					0, // 1 => il semaforo è condiviso tra processi,
					   // 0 => il semaforo è condiviso tra threads del processo
					1 // valore iniziale del semaforo
				  );

	CHECK_ERR(s,"sem_init")

	srand (time(0));
	for(int i = 0; i < M; i++){
		s = pthread_create(&threads[i], NULL, thread_function, NULL);
		if (s != 0) {
			perror("pthread_create");
		}
	}
	for(int i = 0; i < M; i++){
		s = pthread_join(threads[i], NULL);
		if (s != 0) {
			perror("pthread_join");
		}
	}

	s = sem_destroy(mutex);
	CHECK_ERR(s,"sem_destroy")

	s = sem_destroy(barrier);
	CHECK_ERR(s,"sem_destroy")

	s = sem_destroy(mutex_file);
	CHECK_ERR(s,"sem_destroy")

	if (close(fd) == -1) {
		perror("close");
		exit(EXIT_FAILURE);
	}

	printf("bye\n");

	return 0;
}

void * thread_function(void * arg) {
	/*pag 29 libro semafori*/
	printf("rendezvous: \n");
	int sec = randomNumber(0, 3);
	printf("fase 1: dormo per %d secondi\n", sec);
	sleep(sec);

	//testo da scrivere nella fase 1:
	char t_id_c[64];
	sprintf(t_id_c, "%lu", pthread_self());
	char* sec_c[64];
	sprintf(sec_c, "%d", sec);

	int text_l = strlen("fase 1, thread id=") + strlen(", sleep period= secondi")
			+ strlen(t_id_c) + strlen(sec_c) + 1;
	char * text = malloc(text_l);
	text[0] = '\0';
	strcat(text, "fase 1, thread id=");
	strcat(text, t_id_c);
	strcat(text, ", sleep period=");
	strcat(text, sec_c);
	strcat(text, " secondi\n");

	//scrittura nel file:
	int res;
	if (sem_wait(mutex_file) == -1) {
		perror("sem_wait");
		exit(EXIT_FAILURE);
	}

	if ((res = write(fd, text, text_l)) == -1) {
		perror("write()");
		exit(EXIT_FAILURE);
	}

	if (sem_post(mutex_file) == -1) {
		perror("sem_post");
		exit(EXIT_FAILURE);
	}

	//inizio barriera:
	if (sem_wait(mutex) == -1) {
		perror("sem_wait");
		exit(EXIT_FAILURE);
	}

	count++;

	if (sem_post(mutex) == -1) {
		perror("sem_post");
		exit(EXIT_FAILURE);
	}

	if(count == M){
		if (sem_post(barrier) == -1) {
			perror("sem_post");
			exit(EXIT_FAILURE);
		}
	}

	if (sem_wait(barrier) == -1) {
		perror("sem_wait");
		exit(EXIT_FAILURE);
	}

	if (sem_post(barrier) == -1) {
		perror("sem_post");
		exit(EXIT_FAILURE);
	}

	printf("critical point:\n");
	text_l = strlen("fase 2, thread id=") + strlen(", dopo la barriera")
				+ strlen(t_id_c) + 1;
	text = realloc(text, text_l);
	text[0] = '\0';
	strcat(text, "fase 2, thread id=");
	strcat(text, t_id_c);
	strcat(text, ", dopo la barriera\n");

	//scrittura nel file:
	if (sem_wait(mutex_file) == -1) {
		perror("sem_wait");
		exit(EXIT_FAILURE);
	}

	if ((res = write(fd, text, text_l)) == -1) {
		perror("write()");
		exit(EXIT_FAILURE);
	}

	if (sem_post(mutex_file) == -1) {
		perror("sem_post");
		exit(EXIT_FAILURE);
	}

	struct timespec t;

	struct timespec remaining;

	t.tv_sec = 0;  // seconds
	t.tv_nsec = 10000; // nanoseconds (10 ms)

	if (nanosleep(&t, &remaining) == -1) {
		perror("nanosleep");
	}

	text_l = strlen("thread id= bye!") + strlen(t_id_c) + 1;
	text = realloc(text, text_l);
	text[0] = '\0';
	strcat(text, "thread id=");
	strcat(text, t_id_c);
	strcat(text, " bye!\n");

	if (sem_wait(mutex_file) == -1) {
		perror("sem_wait");
		exit(EXIT_FAILURE);
	}

	if ((res = write(fd, text, text_l)) == -1) {
		perror("write()");
		exit(EXIT_FAILURE);
	}

	if (sem_post(mutex_file) == -1) {
		perror("sem_post");
		exit(EXIT_FAILURE);
	}

	return 0;

}

int randomNumber(int lower, int upper) {
    return (rand() % (upper - lower + 1)) + lower;
}

