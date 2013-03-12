/* cpu-sch-sim.cpp
 * Main loop for CPU scheduler
 *
 * EECE 315
 * Group BB4
 */

#include<iostream>
#include<unistd.h>
#include<fstream>
#include<string>
#include<vector>
#include "PCB.h"
#include "WorkloadParser.h"
#include "Scheduler.h"
#include "SchedulerFactory.h"
#include "ReadyQueue.h"
#include "IOQueues.h"
#include "CPU.h"


int main(){
	using namespace std;

	int algorithmIndex = 0;
	int quantumTime = 0;
	int time = 0;
	bool allProcessesDone = false;
	WorkloadParser parser;
	vector<PCB*> processes;
	string filename;
	CPU simCPU;
	Scheduler* scheduler;
	SchedulerFactory schFactory;
	ReadyQueue readyQueue;
	IOQueues ioQueues;
	PCB* doneIO;

	//Setting up log file
	char dirBuffer[1024];
	string logFileName;
	ssize_t dirLen = readlink("/proc/self/exe", dirBuffer, sizeof(dirBuffer)-1);

	if (dirLen != -1) {
		dirBuffer[dirLen] = '\0';
		logFileName = string(dirBuffer);
	} else {
		cout << "ERROR: could not open log file"<<endl;
		return 1;
	}

	while(logFileName[logFileName.size()-1] != '/'){
		logFileName.pop_back();
	}

	logFileName += "actions.log";
	ofstream logFile (logFileName);

	if( !logFile.is_open() ){
		cout << "ERROR: could not open log file"<<endl;
		return 1;
	}	
	
	cout<<"Please enter workload file name: ";
	/* for testing smoothness
	do{
		cin>>filename;
	}while(!(WorkloadParser::validFileName(filename)));
	*/
	filename = "workloads/testWorkload1.txt";
	processes = parser.parseWorkload(filename);

	//Get the algorithm to be used
	cout<<endl<<"\t"<<"1) FCFS   2) RR   3) Polite Premptive Priority"<<endl
		      <<"\t"<<"4) Impatient Premptive Priority"<<endl
			  <<"\t"<<"5) Non Premprive Priority   6) SJF   7) SPB"<<endl<<endl;

	while( algorithmIndex < FCFS || algorithmIndex > SPB ){
		cout<<"Select scheduling algorithm:";
		cin>>algorithmIndex;
	}

	if( algorithmIndex != FCFS ){
		while( quantumTime <= 0 ){
			cout<<"Please enter a value for quantum time:";
			cin>>quantumTime;
		}
	}

	scheduler = schFactory.makeScheduler(algorithmIndex, quantumTime);

	//Main running loop. Time begins to flow here.
	while(!allProcessesDone){
		//Insert any newly arrived processes
		for(unsigned int i = 0; i < processes.size();i++){
			if(processes[i]->getTARQ() == time){
				readyQueue.insert(processes[i]);
				logFile<<"Start: PID "<<processes[i]->getPID()
					<<" @ time: "<<time<<endl;					//LOG
			}
		}
	
		//Put any processes done IO into the ready queue
		if(ioQueues.getSize() != 0){
			doneIO= ioQueues.removeReadyProcess();
			while(doneIO != NULL){
				readyQueue.insert(doneIO);
				logFile<<"Done IO: PID "<<doneIO->getPID()
					<<" @ time:"<<time<<endl;				//LOG
				doneIO = ioQueues.removeReadyProcess();
			}	
		}

		//If Time slice has ended, or process has finished, place in IO
		if(simCPU.getProcess() != NULL){ 
			//Process has finished
			if(	simCPU.getProcess()->isDone() ){
				logFile<<"Process finished: PID "
					<<simCPU.getProcess()->getPID()
					<<" @ time:"<<time<<endl;				//LOG
				simCPU.setProcess(NULL);
			//Process needs IO
			}else if( simCPU.getProcess()->getCurrentBurst() % 2 != 0 ){
				ioQueues.insert(simCPU.getProcess());
				logFile<<"Done CPU burst: PID "
					<<simCPU.getProcess()->getPID()
					<<" @ time:"<<time<<endl;				//LOG
				simCPU.setProcess(NULL);
			//Time slice has expired
			} else if( scheduler->doesTimeSlice() && 
			simCPU.getBurstDuration() == scheduler->getQuantumTime()){
				readyQueue.insert(simCPU.getProcess());
				logFile<<"Time slice: PID "<<simCPU.getProcess()->getPID()
					<<" @ time:"<<time<<endl;				//LOG
				simCPU.setProcess(NULL);
			}
		//If there are interrupts, preempt with higher priority process
		} else if ( scheduler->doesInterrupt() ){
			PCB* impatientProcess = scheduler->schedule(&readyQueue);
			if( impatientProcess->getPriority() > 
					simCPU.getProcess()->getPriority() ){
				readyQueue.insert(simCPU.getProcess());
				simCPU.setProcess(impatientProcess);

				logFile<<"Interrupt: PID "<<impatientProcess->getPID()
					<<" @ time:"<<time<<endl;				//LOG
			} else {
				readyQueue.insert(impatientProcess);
			}
		}

		//Put process into cpu if cpu is empty
		if((simCPU.getProcess() == NULL) && (readyQueue.getSize() != 0)){
			simCPU.setProcess(scheduler->schedule(&readyQueue));
			logFile<<"Next process in CPU @ time:"<<time<<endl;		//LOG
		}

		//Increment wait time on ready queue
		readyQueue.update();

		//Decrement time remianing on CPU
		simCPU.decPCBTime();

		//Decrement time remaining on IO
		ioQueues.updateTimeRemaining();

		for(unsigned int i = 0; i< processes.size();i++){
			if(!(processes[i]->isDone()))
				break;
			else if(i == (processes.size() - 1))
				allProcessesDone = true;
		}	

		time++;
	}				

	for(unsigned int i=0; i< processes.size();i++)
		delete processes[i];	

	logFile.close();
	return 0;
}
