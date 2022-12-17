#include "queue.c"
#include <stdbool.h>
#include <pthread.h> // pthread_*
#include <sys/time.h>
#include <string.h>

int simulationTime = 120;    // simulation time
int seed = 10;               // seed for randomness
int emergencyFrequency = 30; // frequency of emergency gift requests from New Zealand

time_t simulationStartTime = 0;
time_t simulationEndTime = 0;
int n = 30;    // start time for printing queues

void create_new_thread(void *(func)(void* vargp));
void PrintQueue(Queue *q);

void* ElfA(void *arg); // the one that can paint
void* ElfB(void *arg); // the one that can assemble
void* Santa(void *arg); 
//void* ControlThread(void *arg); // handles printing and queues (up to you)
void* GiftRequest(void *arg);
void recordLog(int taskID, Task t, char *worker, char taskType, int request_time, int tt);
void *PrintCurrentQueues(void *arg);

Queue *assembly;			//the queue that assembling jobs were first placed when they were created
Queue *painting;			//the queue that painting jobs were first placed when they were created
Queue *packaging; 			//the queue that packaging jobs were first placed when they were created
Queue *QA;				//the queue that QA jobs were first placed when they were created
Queue *delivering;			//the queue that delivering jobs were first placed when they were created


pthread_mutex_t mutex_assembly;		//to block changes to the assembly queue
pthread_mutex_t mutex_painting;		//to block changes to the painting queue
pthread_mutex_t mutex_QA;		//to block changes to the QA queue
pthread_mutex_t mutex_delivering;	//to block changes to the delivering queue	
pthread_mutex_t mutex_packaging;	//to block changes to the packaging queue
pthread_mutex_t mutex_handler_type4;	//to handle type4 gift, used when checking both painting and QA queues
pthread_mutex_t mutex_handler_type5;	//to handle type5 gift, used when checking both assembly and QA queues

pthread_mutex_t mutex_log;		//to block changes to log file
pthread_mutex_t mutex_taskID;		//to block changes to taskID


int taskID = 0;
int giftID = 0;
double currentTime;

// pthread sleeper function
int pthread_sleep (int seconds)
{
    pthread_mutex_t mutex;
    pthread_cond_t conditionvar;
    struct timespec timetoexpire;
    if(pthread_mutex_init(&mutex,NULL))
    {
        return -1;
    }
    if(pthread_cond_init(&conditionvar,NULL))
    {
        return -1;
    }
    struct timeval tp;
    //When to expire is an absolute time, so get the current time and add it to our delay time
    gettimeofday(&tp, NULL);
    timetoexpire.tv_sec = tp.tv_sec + seconds; timetoexpire.tv_nsec = tp.tv_usec * 1000;
    
    pthread_mutex_lock(&mutex);
    int res =  pthread_cond_timedwait(&conditionvar, &mutex, &timetoexpire);
    pthread_mutex_unlock(&mutex);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&conditionvar);
    
    //Upon successful completion, a value of zero shall be returned
    return res;
}


int main(int argc,char **argv){
    // -t (int) => simulation time in seconds
    // -s (int) => change the random seed
    for(int i=1; i<argc; i++){
        if(!strcmp(argv[i], "-t")) {simulationTime = atoi(argv[++i]);}
        else if(!strcmp(argv[i], "-s"))  {seed = atoi(argv[++i]);}
        else if (!strcmp(argv[i], "-n")){n = atoi(argv[++i]);}
    }
    
    srand(seed); // feed the seed
    
    /* Queue usage example
        Queue *myQ = ConstructQueue(1000);
        Task t;
        t.ID = myID;
        t.type = 2;
        Enqueue(myQ, t);
        Task ret = Dequeue(myQ);
        DestructQueue(myQ);
    */

    // your code goes here
    // you can simulate gift request creation in here, 
    // but make sure to launch the threads first

    simulationStartTime = time(NULL);

    FILE *log_file = fopen("log_part2.txt", "w");
    fprintf(log_file, "       TaskID   GiftID	GiftType   TaskType   RequestTime   TaskArrival    TT    Responsible\n");
    fprintf(log_file, "--------------------------------------------------------------------------------------------\n");
    fclose(log_file);

    simulationEndTime = time(NULL) + simulationTime;   


    packaging = ConstructQueue(1000);
    painting = ConstructQueue(1000);
    assembly = ConstructQueue(1000);
    QA = ConstructQueue(1000);
    delivering = ConstructQueue(1000);


    pthread_t elf_a;
    pthread_t elf_b;
    pthread_t santa;
    pthread_t gift_request_t;
    pthread_t print_queues_t;


    pthread_create(&elf_a, NULL, &ElfA, NULL);
    pthread_create(&elf_b, NULL, &ElfB, NULL);
    pthread_create(&santa, NULL, &Santa, NULL);
    pthread_create(&gift_request_t, NULL, &GiftRequest, NULL);
    pthread_create(&print_queues_t, NULL, &PrintCurrentQueues, NULL);


// Wait for all threads to finish execution   
    pthread_join(elf_a, NULL);
    pthread_join(elf_b, NULL);
    pthread_join(santa, NULL);
    pthread_join(gift_request_t, NULL);
    pthread_join(print_queues_t, NULL);

    DestructQueue(packaging);
    DestructQueue(painting);
    DestructQueue(assembly);
    DestructQueue(QA);
    DestructQueue(delivering);
    
    return 0;
}

void* ElfA(void *arg){
	while(time(NULL) < simulationEndTime){
		pthread_mutex_lock(&mutex_packaging);
		if(!isEmpty(packaging)){		//prioritize packaging
			pthread_mutex_lock(&mutex_log);
			pthread_mutex_lock(&mutex_taskID);
			taskID++;
			int id = taskID;
			pthread_mutex_unlock(&mutex_taskID);

			Task t = Dequeue(packaging);
			pthread_mutex_unlock(&mutex_packaging);

			recordLog(id, t, "ElfA", 'C', t.packaging_request_time, (time(NULL) - simulationStartTime) - t.packaging_request_time + 1);
			pthread_mutex_unlock(&mutex_log);

			pthread_sleep(1);

			pthread_mutex_lock(&mutex_delivering);

			t.delivering_request_time = time(NULL) - simulationStartTime;
			Enqueue(delivering, t);
			pthread_mutex_unlock(&mutex_delivering);

		}else{
			pthread_mutex_unlock(&mutex_packaging);
			pthread_mutex_lock(&mutex_painting);

			if(!isEmpty(painting)){

				NODE *curr = painting->head;
				int curr_id = curr->data.ID;

				if(curr->data.type == 2){		//type2 gift, enqueue normally when the task is done
					pthread_mutex_lock(&mutex_log);
					pthread_mutex_lock(&mutex_taskID);
					taskID++;
					int id = taskID;
					pthread_mutex_unlock(&mutex_taskID);

					Task t = Dequeue(painting);
					pthread_mutex_unlock(&mutex_painting);

					recordLog(id, t, "ElfA", 'P', t.painting_request_time, (time(NULL) - simulationStartTime) - t.painting_request_time + 3);
					pthread_mutex_unlock(&mutex_log);

				        pthread_sleep(3);

					pthread_mutex_lock(&mutex_packaging);

					t.packaging_request_time = time(NULL) - simulationStartTime;
					Enqueue(packaging, t);
					pthread_mutex_unlock(&mutex_packaging);

				}else{					//type4 gift, check QA queue, enqueue if it is necessary

					pthread_mutex_lock(&mutex_log);
					pthread_mutex_lock(&mutex_taskID);
					taskID++;
					int id = taskID;
					pthread_mutex_unlock(&mutex_taskID);

					pthread_mutex_lock(&mutex_handler_type4);

					Task t = Dequeue(painting);
					pthread_mutex_unlock(&mutex_painting);

					pthread_mutex_lock(&mutex_QA);
					NODE *curr = QA->head;
					bool is_QA_done = true;
					while (curr != NULL){
					    if(curr->data.ID == curr_id){
						is_QA_done = false;
					    }

					    curr = curr->prev;
					}
					pthread_mutex_unlock(&mutex_QA);
					pthread_mutex_unlock(&mutex_handler_type4);

					recordLog(id, t, "ElfA", 'P', t.painting_request_time, (time(NULL) - simulationStartTime) - t.painting_request_time + 3);
					pthread_mutex_unlock(&mutex_log);

				        pthread_sleep(3);

					if(is_QA_done){
						pthread_mutex_lock(&mutex_packaging);
						t.packaging_request_time = time(NULL) - simulationStartTime;
						Enqueue(packaging, t);
						pthread_mutex_unlock(&mutex_packaging);
					}
				}

			}
			else{
				pthread_mutex_unlock(&mutex_painting);
			}
		}
	}
}

void* ElfB(void *arg){
        while(time(NULL) < simulationEndTime){
                pthread_mutex_lock(&mutex_packaging);
                if(!isEmpty(packaging)){		//prioritize packaging
			pthread_mutex_lock(&mutex_log);
			pthread_mutex_lock(&mutex_taskID);
			taskID++;
			int id = taskID;
			pthread_mutex_unlock(&mutex_taskID);

			Task t = Dequeue(packaging);
                        pthread_mutex_unlock(&mutex_packaging);

			recordLog(id, t, "ElfB", 'C', t.packaging_request_time, (time(NULL) - simulationStartTime) - t.packaging_request_time + 1);
			pthread_mutex_unlock(&mutex_log);

                        pthread_sleep(1);

                        pthread_mutex_lock(&mutex_delivering);

			t.delivering_request_time = time(NULL) - simulationStartTime;
                        Enqueue(delivering, t);
                        pthread_mutex_unlock(&mutex_delivering);

                }else{
			pthread_mutex_unlock(&mutex_packaging);
                        pthread_mutex_lock(&mutex_assembly);

			if(!isEmpty(assembly)){
				NODE *curr = assembly->head;
				int curr_id = curr->data.ID;
				if(curr->data.type == 3){		//type3 gift, enqueue normally when the task is done

					pthread_mutex_lock(&mutex_log);
					pthread_mutex_lock(&mutex_taskID);
					taskID++;
					int id = taskID;
					pthread_mutex_unlock(&mutex_taskID);

					Task t = Dequeue(assembly);
					pthread_mutex_unlock(&mutex_assembly);

					recordLog(id, t, "ElfB", 'A', t.assembly_request_time, (time(NULL) - simulationStartTime) - t.assembly_request_time + 2);
					pthread_mutex_unlock(&mutex_log);

				        pthread_sleep(2);

					pthread_mutex_lock(&mutex_packaging);

					t.packaging_request_time = time(NULL) - simulationStartTime;
					Enqueue(packaging, t);
					pthread_mutex_unlock(&mutex_packaging);

				}else{					//type5 gift, check QA queue, enqueue if it is necessary

					pthread_mutex_lock(&mutex_log);
					pthread_mutex_lock(&mutex_taskID);
					taskID++;
					int id = taskID;
					pthread_mutex_unlock(&mutex_taskID);

					pthread_mutex_lock(&mutex_handler_type5);					

					Task t = Dequeue(assembly);
					pthread_mutex_unlock(&mutex_assembly);

					pthread_mutex_lock(&mutex_QA);
					NODE *curr = QA->head;
					bool is_QA_done = true;
					while (curr != NULL){
					    if(curr->data.ID == curr_id){
						is_QA_done = false;
					    }

					    curr = curr->prev;
					}
					pthread_mutex_unlock(&mutex_QA);	
					pthread_mutex_unlock(&mutex_handler_type5);					

					recordLog(id, t, "ElfB", 'A', t.assembly_request_time, (time(NULL) - simulationStartTime) - t.assembly_request_time + 2);
					pthread_mutex_unlock(&mutex_log);

				        pthread_sleep(2);


					if(is_QA_done){
						pthread_mutex_lock(&mutex_packaging);
						t.packaging_request_time = time(NULL) - simulationStartTime;
						Enqueue(packaging, t);
						pthread_mutex_unlock(&mutex_packaging);
					}

				}

			}
			else{
				pthread_mutex_unlock(&mutex_assembly);

			}
		} 
        }

}

// manages Santa's tasks
void* Santa(void *arg){                                       
        while(time(NULL) < simulationEndTime){
                pthread_mutex_lock(&mutex_delivering);
		pthread_mutex_lock(&mutex_QA);

                if(!isEmpty(delivering) && QA->size < 3){	//do delivering only if the QA size is less than 3 (to solve starvation of QA jobs)
			pthread_mutex_unlock(&mutex_QA);
			pthread_mutex_lock(&mutex_log);
			pthread_mutex_lock(&mutex_taskID);
			taskID++;
			int id = taskID;
			pthread_mutex_unlock(&mutex_taskID);

                        Task t = Dequeue(delivering);
                        pthread_mutex_unlock(&mutex_delivering);

			recordLog(id, t, "Santa", 'D', t.delivering_request_time, (time(NULL) - simulationStartTime) - t.delivering_request_time + 1);
			pthread_mutex_unlock(&mutex_log);

                        pthread_sleep(1);

                }
		else{
			pthread_mutex_unlock(&mutex_delivering);
			pthread_mutex_unlock(&mutex_QA);
			pthread_mutex_lock(&mutex_QA);
			
			if(!isEmpty(QA)){
				NODE *curr = QA->head;
				int curr_id = curr->data.ID;

				if(curr->data.type == 4){			//type4 gift, check painting queue, enqueue if it is necessary

					pthread_mutex_lock(&mutex_log);
					pthread_mutex_lock(&mutex_taskID);
					taskID++;
					int id = taskID;
					pthread_mutex_unlock(&mutex_taskID);

					pthread_mutex_lock(&mutex_handler_type4);

				        Task t = Dequeue(QA);
				        pthread_mutex_unlock(&mutex_QA);

					pthread_mutex_lock(&mutex_painting);
					NODE *curr = painting->head;
					bool is_painting_done = true;
					while (curr != NULL){
					    if(curr->data.ID == curr_id){
						is_painting_done = false;
					    }

					    curr = curr->prev;
					}
					pthread_mutex_unlock(&mutex_painting);	
					pthread_mutex_unlock(&mutex_handler_type4);

					recordLog(id, t, "Santa", 'Q', t.QA_request_time, (time(NULL) - simulationStartTime) - t.QA_request_time + 1);
					pthread_mutex_unlock(&mutex_log);

				        pthread_sleep(1);


					if(is_painting_done){
						pthread_mutex_lock(&mutex_packaging);
						t.packaging_request_time = time(NULL) - simulationStartTime;
						Enqueue(packaging, t);
						pthread_mutex_unlock(&mutex_packaging);
					}

				}else{						//type5 gift, check assembly queue, enqueue if it is necessary

					pthread_mutex_lock(&mutex_log);
					pthread_mutex_lock(&mutex_taskID);
					taskID++;
					int id = taskID;
					pthread_mutex_unlock(&mutex_taskID);

					pthread_mutex_lock(&mutex_handler_type5);

				        Task t = Dequeue(QA);
				        pthread_mutex_unlock(&mutex_QA);

					pthread_mutex_lock(&mutex_assembly);
					NODE *curr = assembly->head;
					bool is_assembly_done = true;
					while (curr != NULL){
					    if(curr->data.ID == curr_id){
						is_assembly_done = false;
					    }

					    curr = curr->prev;
					}
					pthread_mutex_unlock(&mutex_assembly);	
					pthread_mutex_unlock(&mutex_handler_type5);

					recordLog(id, t, "Santa", 'Q', t.QA_request_time, (time(NULL) - simulationStartTime) - t.QA_request_time + 1);
					pthread_mutex_unlock(&mutex_log);

				        pthread_sleep(1);


					if(is_assembly_done){
						pthread_mutex_lock(&mutex_packaging);
						t.packaging_request_time = time(NULL) - simulationStartTime;
						Enqueue(packaging, t);
						pthread_mutex_unlock(&mutex_packaging);
					}

				}   

			}else{
				pthread_mutex_unlock(&mutex_QA);
			}


		}
        }
}

void* GiftRequest(void *arg){
    while(time(NULL) < simulationEndTime){
            int random = rand() % 20;
	    Task gift;
	    giftID++;
	    gift.ID = giftID;

	    gift.painting_status = -1;
	    gift.assembly_status = -1;
	    gift.QA_status = -1;
	    gift.arrival_time = time(NULL) - simulationStartTime;
	    gift.packaging_request_time = -1;
	    gift.painting_request_time = -1;
	    gift.assembly_request_time = -1;
	    gift.delivering_request_time = -1;
	    gift.QA_request_time = -1;
	    if(random <= 9){            //type1 gift is requested
	            gift.type = 1;
		    gift.packaging_request_time = time(NULL) - simulationStartTime;
		    pthread_mutex_lock(&mutex_packaging);
	            Enqueue(packaging, gift);
		    pthread_mutex_unlock(&mutex_packaging);

	    }else if(random <= 13){     //type2 gift is requested
	            gift.type = 2;
		    gift.painting_request_time = time(NULL) - simulationStartTime;
	            pthread_mutex_lock(&mutex_painting);
	            Enqueue(painting, gift);
	            pthread_mutex_unlock(&mutex_painting);

	    }else if(random <= 17){     //type3 gift is requested
	            gift.type = 3;
		    gift.assembly_request_time = time(NULL) - simulationStartTime;
	            pthread_mutex_lock(&mutex_assembly);
	            Enqueue(assembly, gift);
	            pthread_mutex_unlock(&mutex_assembly);

	    }else if(random == 18){     //type4 gift is requested, send it to both painting and QA queue
	            gift.type = 4;
		    gift.painting_status = 0;
		    gift.QA_status = 0;
		    gift.painting_request_time = time(NULL) - simulationStartTime;
		    gift.QA_request_time = time(NULL) - simulationStartTime;

		    pthread_mutex_lock(&mutex_painting);
	            Enqueue(painting, gift);
		    pthread_mutex_unlock(&mutex_painting);

		    pthread_mutex_lock(&mutex_QA);
	            Enqueue(QA, gift);
		    pthread_mutex_unlock(&mutex_QA); 

	    }else if(random == 19){     //type5 gift is requested, send it to both assembly and QA queue
	            gift.type = 5;
		    gift.assembly_status = 0;
		    gift.QA_status = 0;
		    gift.assembly_request_time = time(NULL) - simulationStartTime;
		    gift.QA_request_time = time(NULL) - simulationStartTime;

		    pthread_mutex_lock(&mutex_assembly);
	            Enqueue(assembly, gift);
		    pthread_mutex_unlock(&mutex_assembly);

		    pthread_mutex_lock(&mutex_QA);
	            Enqueue(QA, gift);
		    pthread_mutex_unlock(&mutex_QA); 
	    }
	    pthread_sleep(1);
    }

}



void recordLog(int taskID, Task t, char *worker, char taskType, int request_time, int tt){
    time_t endTime = time(NULL) - simulationStartTime;
    char log[150];
    sprintf(log, "%11d%11d%11d%11c%11d%11d%11d%11s\n", taskID, t.ID, t.type, taskType, request_time, t.arrival_time, tt, worker);

    FILE *fptr = fopen("log_part2.txt", "a");
    fprintf(fptr, "%s", log);
    fclose(fptr);
}


void PrintQueue(Queue *q){
    if (isEmpty(q)){
        printf("Empty\n");
    }
    else{
        NODE *curr = q->head;
        while (curr != NULL){
            printf("%d ", curr->data.ID);
            curr = curr->prev;
        }
        printf("\n");
    }
}


void *PrintCurrentQueues(void *arg){
    while (time(NULL) < simulationEndTime){
        int current_time = time(NULL) - simulationStartTime;
        if (n <= current_time){
		pthread_mutex_lock(&mutex_packaging);
	    	printf("At %d sec packaging: ", current_time);
	    	PrintQueue(packaging);
	    	pthread_mutex_unlock(&mutex_packaging);

	    	pthread_mutex_lock(&mutex_painting);
	    	printf("At %d sec painting: ", current_time);
	    	PrintQueue(painting);
	    	pthread_mutex_unlock(&mutex_painting);

	    	pthread_mutex_lock(&mutex_assembly);
	    	printf("At %d sec assembly: ", current_time);
	    	PrintQueue(assembly);
	    	pthread_mutex_unlock(&mutex_assembly);
		
	    	pthread_mutex_lock(&mutex_delivering);
	    	printf("At %d sec delivering: ", current_time);
	    	PrintQueue(delivering);
	    	pthread_mutex_unlock(&mutex_delivering);

	    	pthread_mutex_lock(&mutex_QA);
	    	printf("At %d sec QA: ", current_time);
	    	PrintQueue(QA);
	    	pthread_mutex_unlock(&mutex_QA);

	    	printf("\n");
	}
        pthread_sleep(1);
    }
    return NULL;
}

