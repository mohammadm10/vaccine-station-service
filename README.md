# vaccine-station-service

#### This was an assignment in one of my classes where we were tasked with using mutex's and queue's to create a vaccine station service with a specific number of patients and a user input for the number of stations

To start off, in the main I parsed through the input text file and read how many lines there
were, and set that value to num_people. I also initialized num_stations as the first arguement
passed in by the user, which is the number of vaccine stations present. The next thing I do is
initialize the vaccination threads, and I did that in a for loop. Each vaccination thread gets
created and is then sent to vac_station where is waits for it's signal to execute. Once the 
amount of num_people threads have been created, I create a single reg_desk thread. This thread
is sent to reg_desk. Once in reg_desk, I copy over all the person info into the newly declared 
person, then lock the mutex. Once the mutex is locked it's in its critical section, here it 
enqueues the person onto the queue, unlocks the mutex, then signals the threads waiting in 
vac_station that someone has been enqueued. This process is taken by each person created.
Once a thread is signalled within vac_station, it enters the while loop, which runs as long as 
there is something in the queue. In the while loop, the mutex is locked, the person is dequeued,
then the mutex is unlocked. Once the person is dequeued, the vaccination starts and the thread
is taken to isVacComplete, which sleeps until the person is done, then broadcasts that it is 
complete for the next thread to continue the process. Within vac_station, if there are no stations
available, the thread waits until one is complete before it can continue, this is implemented 
using **cnt** as a counter.
