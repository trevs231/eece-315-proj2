#include "PriorityScheduler.h"
#include <iostream>

using namespace std;

PriorityScheduler::PriorityScheduler(int quantumTime, bool doesInterrupt, bool doesTimeSlice) {
	mDoesTimeSlice = doesTimeSlice;
	mQuantumTime = quantumTime;
	mDoesInterrupt = doesInterrupt;
	mAlpha = 0;
}

// Returns the PCB with the highest priority
PCB* PriorityScheduler::schedule(ReadyQueue* q) {
	PCB* selectedProcess;
	PCB* currentProcess;
	q->begin();
	selectedProcess = q->getCurrent();
	currentProcess = q->getCurrent();
	while (currentProcess != NULL) {
		if ( currentProcess->getPriority() > selectedProcess->getPriority() ) {
			selectedProcess = q->getCurrent();
		}		
		currentProcess = q->getNext();
	}
	q->remove(selectedProcess);
	return selectedProcess;
}





