# Comp 304 Project 2 Report

Çisem Özden-69707
Gülbarin Maçin-69163


All parts are working. 

# Part 1:
	We created 5 threads to run with ElfA, ElfB, Santa, GiftRequest and PrintCurrentQueues functions. When controlling job flow we carefully used mutex locks and unlocks to avoid possible race conditions and deadlocks.
	GiftRequest handles the creation of the gift according to given probabilities. 
	PrintCurrentQueues prints the each task on the queues each second starting from given n second. 
	Other functions handles the task process according to their job given in the pdf. 
	To handle type 4 and type 5 gifts, we enqueue particular gifts to both relevant queues. Specifically, ElfA does painting and it checks whether the QA job is done for that particular gift and it only enqueues that task to the next job’s queue if the QA job was done. Santa works similar to ElfA when handling type 4 gifts. Santa does QA and it checks whether the painting job is done for that particular gift and it only enqueues that task to the next job’s queue if the painting job was done. ElfB and Santa follow the same process for type 5 gifts.

# Part 2:
	For this part we added the necessary conditions mentioned in the pdf to the required places in the Santa function. By checking emptiness of delivering queue and the size of the QA queue, we are able to resolve the starvation of the QA jobs.

# Part 3:
	When creating gifts for the first time, we checked whether the time is multiple of 30 to see whether the gift is coming from New Zealand.  If so, we inserted the particular gift to the beginning of the queue corresponding to the first  required task. After dequeuing the tasks, we had ElfA,ElfB and Santa check if the dequeued task came from New Zealand and if so, insert it at the top of the next task’s queue. We performed the mentioned operation by checking whether the arrival time of the dequeued task is a multiple of 30. With this implementation, we were able to prioritize the gifts coming from New Zealand for every task it requires. 

# Part 4:
	We kept track of all the relevant logs by calling the recordLog function right after the completion of tasks in Santa, ElfA and ElfB functions. 
