/* cpu-sch-sim.cpp
 * Main loop for CPU scheduler
 *
 * EECE 315
 * Group BB4
 */

#include<iostream>
#include<sstream>
#include<fstream>
#include<string>
#include<vector>
#include<limits>
#include "PCB.h"
#include "WorkloadParser.h"
#include "Scheduler.h"
#include "SchedulerFactory.h"
#include "ReadyQueue.h"
#include "IOQueues.h"
#include "CPU.h"
#include "Logger.h"
#include "GanttChart.h"


int main(){
	using namespace std;

	int algorithmIndex = 0;
	int quantumTime = 0;
	int time = 0;
	double weightedAverage = 0;
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
	GanttChart ganttChart;
	
	//The action logger
	Logger actLog("actions.log");
	
	cout<<"Please enter workload file name: ";

	do{
		cin>>filename;
	}while(!(WorkloadParser::validFileName(filename)));
	
	processes = parser.parseWorkload(filename);

	//Get the algorithm to be used
	cout<<endl<<"\t"<<"1) FCFS   2) RR   3) Polite Premptive Priority"<<endl
		      <<"\t"<<"4) Impatient Premptive Priority"<<endl
			  <<"\t"<<"5) Non Premprive Priority   6) SJF   7) SPB"<<endl<<endl;


	while(algorithmIndex < FCFS || algorithmIndex > SPB){
		cout<<"Select scheduling algorithm:";
		while(!(cin>>algorithmIndex)){
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');	
			cout<<"Select scheduling algorithm:";
		}
	}

	if( algorithmIndex != FCFS && algorithmIndex != NPP){
		while( quantumTime <= 0 ){
			cout<<"Please enter a value for quantum time:";
			while(!(cin>>quantumTime)){
				cin.clear();
				cin.ignore(numeric_limits<streamsize>::max(), '\n');	
				cout<<"Please enter a value for quantum time:";
			}
		}
	}

	if(algorithmIndex == SPB){
		do{
			cout<<"Please enter a value between 0 and 1 for the weighted average:";
			while(!(cin>>weightedAverage)){
				cin.clear();
				cin.ignore(numeric_limits<streamsize>::max(), '\n');	
				cout<<"Please enter a value between 0 and 1 for the weighted average:";
			}
		}while(weightedAverage <= 0 || weightedAverage >= 1);
	}

	//Create scheduler algorithm object selected by user
	scheduler = schFactory.makeScheduler(algorithmIndex, quantumTime, weightedAverage);

	//Main running loop. Time begins to flow here.
	while(!allProcessesDone){
		//Insert any newly arrived processes
		for(unsigned int i = 0; i < processes.size();i++){
			if(processes[i]->getTARQ() == time){
				readyQueue.insert(processes[i], true);
				actLog.logCreateProcess(processes[i]);
			}
		}
	
		//Put any processes done IO into the ready queue
		if(ioQueues.getSize() != 0){
			doneIO= ioQueues.removeReadyProcess();
			while(doneIO != NULL){
				readyQueue.insert(doneIO, true);
				actLog.logDoneIoBurst(doneIO);
				doneIO = ioQueues.removeReadyProcess();
			}	
		}

		//If Time slice has ended, or process has finished, place in IO
		if(simCPU.getProcess() != NULL){ 
			//Process has finished
			if(	simCPU.getProcess()->isDone() ){
				actLog.logProcessFinished(simCPU.getProcess());
				simCPU.setProcess(NULL);
			//Process needs IO
			//Reset priority
			}else if( simCPU.getProcess()->getCurrentBurst() % 2 != 0 ){
				ioQueues.insert(simCPU.getProcess());
				simCPU.getProcess()->resetRelPriority();
				simCPU.getProcess()->updateAvPrevBurst(simCPU.getBurstDuration(), scheduler->getAlpha());
				actLog.logDoneCPUBurst(simCPU.getProcess());
				simCPU.setProcess(NULL);
			//Time slice has expired
			//Reset priority
			} else if( scheduler->doesTimeSlice() && 
			simCPU.getBurstDuration() == scheduler->getQuantumTime()){
				readyQueue.insert(simCPU.getProcess(), true);
				simCPU.getProcess()->resetRelPriority();
				simCPU.getProcess()->updateAvPrevBurst(simCPU.getBurstDuration(), scheduler->getAlpha());
				actLog.logTimeSlice(simCPU.getProcess());
				simCPU.setProcess(NULL);

				//If there are interrupts, preempt with higher priority process
			} else if ( scheduler->doesInterrupt() && readyQueue.getSize() != 0 ){
			PCB* impatientProcess = scheduler->schedule(&readyQueue);
				if(	impatientProcess->getPriority() > simCPU.getProcess()->getPriority() ){
					actLog.logInterrupt(impatientProcess, simCPU.getProcess());
					readyQueue.insert(simCPU.getProcess(), true);
					simCPU.getProcess()->updateAvPrevBurst(simCPU.getBurstDuration(), scheduler->getAlpha());
					simCPU.setProcess(impatientProcess);

				
				} else {
					readyQueue.insert(impatientProcess, false);
				}
			}
		}
		//Put process into cpu if cpu is empty
		if((simCPU.getProcess() == NULL) && (readyQueue.getSize() != 0)){
			simCPU.setProcess(scheduler->schedule(&readyQueue));
			actLog.logNextProcess(simCPU.getProcess());
			readyQueue.updatePriority();
		}

		//record PID of process in cpu for Gantt chart
		ganttChart.recordPID(simCPU.getProcess()); 

		//Increment wait time on ready queue
		readyQueue.updateWaitTime();

		//Decrement time remianing on CPU
		simCPU.decPCBTime();

		//Decrement time remaining on IO
		ioQueues.updateTimeRemaining();

		//Check if processes are all done, and CPU has been emptied
		if(simCPU.getProcess() == NULL){
			for(unsigned int i = 0; i< processes.size();i++){
				if(!(processes[i]->isDone()))
					break;
				else if(i == (processes.size() - 1))
					allProcessesDone = true;
			}	
		}

		time++;
		actLog.incTime();
	}		
		
	//Done Logging
	actLog.logDone(time);

	//Draw the Gantt Chart
	ganttChart.draw(processes);
	ganttChart.getMetrics(processes, time-1);

	delete scheduler;
	for(unsigned int i=0; i< processes.size();i++)
		delete processes[i];	

	return 0;
}
