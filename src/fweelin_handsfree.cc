#include <stdio.h>
#include <sys/time.h>

#include "fweelin_handsfree.h"

// DEBUG:
bool critters = true ; // TODO: change all to CRITTERS 
void Handsfree::dbgLoopsStatus(Loopset* loopset)
{
	for (int i = 0 ; i < MAX_LOOPSET_FILL ; ++i)
	{
		int loopIdx = loopset->baseIdx + i ; char* statusMsg ;
		switch (getLoopStatus(loopIdx))
		{
			case 0: statusMsg = "T_LS_Off" ; break ;
			case 1: statusMsg = "T_LS_Recording" ; break ;
			case 2: statusMsg = "T_LS_Overdubbing" ; break ;
			case 3: statusMsg = "T_LS_Playing" ; break ;
		}
		printf("DEBUG: getLoopStatus(%d) = %s\n" , loopIdx , statusMsg) ;
	}
}
// DEBUG: end


// public

// event listeners
void Handsfree::ReceiveEvent(Event* ev, EventProducer* from)
{
  switch (ev->GetType())
  {
	  case T_EV_Input_Key: handleKeypress(ev) ;
	  break ;
	  default: return ;
  }
}

// handsfree functions
void Handsfree::HandlePulse(int wrappedPulseIdx) {
bool isCurrPulseWrapped = (~getPulseIdx() && wrappedPulseIdx == this->prevLoopsetIdx) ;
printf("DEBUG: HandlePulse() wrappedPulseIdx=%d currPulseIdx=%d wrapped - %s\n" , wrappedPulseIdx , getPulseIdx() ,
(isCurrPulseWrapped)? "triggerLoops()" : "ignoring") ;
	if (isCurrPulseWrapped) triggerLoops() ; }


// private

// event listeners
void Handsfree::addListeners()
{
	// events are listed in fweelin_event.cc:160
	printf(REGISTER_EVENT_DBG) ;
	this->app->getEMG()->ListenEvent(this , 0 , T_EV_Input_Key) ;
}

void Handsfree::removeListeners()
{
	printf(UNREGISTER_EVENT_DBG) ;
	this->app->getEMG()->UnlistenEvent(this , 0 , T_EV_Input_Key) ;
}


// helpers
double Handsfree::getTimestamp()
{
	struct timeval tv ; gettimeofday(&tv , NULL) ;
	return (double)tv.tv_sec + ((double)1e-6 * tv.tv_usec) ;
}

//bool Handsfree::getKeyMutexState() { return this->keyMutex ; }
bool Handsfree::isTimestampStale()
	{ return (getTimestamp() - this->lastKeypress) > SUPRESS_KEYPRESS_INTERVAL ; }

//void Handsfree::setKeyMutexState(bool isOn) { if (!isOn) usleep(250000) ; this->keyMutex = isOn ; }
void Handsfree::setTimestamp() { this->lastKeypress = getTimestamp() ; }

int Handsfree::getPulseIdx()
	{ LoopManager* loopMan = this->app->getLOOPMGR() ; return loopMan->GetCurPulseIndex() ; }

int Handsfree::getLoopStatus(int loopIdx) // LoopStatus enum in fweelin_core.h
	{ LoopManager* loopMan = this->app->getLOOPMGR() ; return loopMan->GetStatus(loopIdx) ; }

void Handsfree::triggerLoop(int loopIdx)
{
	TriggerLoopEvent* trigEv = (TriggerLoopEvent*)Event::GetEventByType(T_EV_TriggerLoop) ;
	trigEv->index = loopIdx ;
/*
	trigEv->engage = 4 ;
	trigEv->vol = 1.0 ;
	trigEv->shot = false ;
	trigEv->od = 4 ;
	trigEv->od_fb = 4 ;
*/
	app->getEMG()->BroadcastEventNow(trigEv , this) ;
/*
int index = 0;   // Index of loop
int engage = -1;  // -1 is the default behavior - toggles between rec/play and off
             // 0 forces the loop to off
             // 1 forces the loop to on
char shot = 0;   // Nonzero if we should only play thru once- no loop
float vol = 1.0;   // Volume of trigger
char od = 0;     // Nonzero if we should trigger an overdub
UserVariable *od_fb = 0;  // Variable which holds overdub feedback- can be continuously varied
*/
}

void Handsfree::selectPulse(int pulseIdx)
{
	SelectPulseEvent* spev = (SelectPulseEvent*) Event::GetEventByType(T_EV_SelectPulse) ;
	spev->pulse = pulseIdx ; app->getEMG()->BroadcastEventNow(spev , this) ;
	while (getPulseIdx() != pulseIdx) {

printf("DEBUG: selectPulse() waiting for pulse\n") ; usleep(SELECT_PULSE_WAIT) ; }
}

// handsfree functions
void Handsfree::handleKeypress(Event* ev)
{
	KeyInputEvent* kev = (KeyInputEvent*)ev ;
	if ((kev->keysym == LOOPSET_KEY || kev->keysym == RECORD_KEY) &&
		kev->down && isTimestampStale()) setTimestamp() ; else return ;

#if DEBUGVB
if (kev->down) printf("handleKeypress() key is down\n") ; else return ;
if (!getKeyMutexState()) printf("handleKeypress() mutex is free\n") ; else return ;
if (kev->keysym == LOOPSET_KEY) printf("handleKeypress() key is LOOPSET_KEY\n") ;
else if (kev->keysym == RECORD_KEY) printf("handleKeypress() key is RECORD_KEY\n") ;
#endif // DEBUGVB

	if (kev->keysym == LOOPSET_KEY) toggleLoopset() ;
	else if (kev->keysym == RECORD_KEY)
		if (~this->Loopsets[this->prevLoopsetIdx]->pulseIdx) toggleAutoRecord() ;
		else triggerLoops() ;
}

void Handsfree::toggleLoopset()
	{ this->nextLoopsetIdx = (this->nextLoopsetIdx + 1) % N_LOOPSETS ; }

void Handsfree::triggerLoops()
{
	// 1st call per loopset - on keypress or pulse wrap - record first loop
	// 2nd call per loopset - on keypress - create pulse - record second loop
	// sunbsequent calls - on pulse wrap - record next loop
	bool isSetChange = (this->nextLoopsetIdx != this->prevLoopsetIdx) ;
	Loopset* prevLoopset ; Loopset* nextLoopset ; int prevLoopIdx , nextLoopIdx ;
	prevLoopset = this->Loopsets[this->prevLoopsetIdx] ;
	nextLoopset = this->Loopsets[this->nextLoopsetIdx] ;
	prevLoopIdx = prevLoopset->baseIdx + prevLoopset->fill ;

#if DEBUG
printf("\nDEBUG: triggerLoops() isAutoRecord = %s\n" , (nextLoopset->isAutoRecord)? "true" : "false") ;
printf("DEBUG: triggerLoops() isSetChange = %s\n" , (isSetChange)? "true" : "false") ;
printf("DEBUG: triggerLoops() pulse %d - %s recording\n" , getPulseIdx() , (getLoopStatus(prevLoopIdx) == T_LS_Recording)? "was" : "not") ;
printf("DEBUG: triggerLoops() prevLoopset = %d nextLoopset = %d\n" , this->prevLoopsetIdx , this->nextLoopsetIdx) ;
#endif // DEBUG:

	bool wasRecording = (getLoopStatus(prevLoopIdx) == T_LS_Recording) ;
	if (wasRecording)
	{
		bool isPulseExist = (prevLoopset->pulseIdx == PULSE_NONE) ;
		triggerLoop(prevLoopIdx) ; prevLoopset->fill = prevLoopset->fill + 1 ;
		if (isPulseExist) selectPulse(prevLoopset->pulseIdx = this->prevLoopsetIdx) ;

if (critters) if (isPulseExist) printf(CREATE_PULSE_DBG , prevLoopIdx) ;
else printf(END_RECORD_DBG , prevLoopIdx) ;
	}

	if (isSetChange) selectPulse(nextLoopset->pulseIdx) ;
	if (nextLoopset->isAutoRecord && nextLoopset->fill < MAX_LOOPSET_FILL)
	{
		nextLoopIdx = nextLoopset->baseIdx + nextLoopset->fill ; triggerLoop(nextLoopIdx) ;

if (critters)	if(wasRecording) printf(NEW_LOOP_DBG , nextLoopIdx) ;
	else printf(INITIAL_LOOP_DBG , nextLoopIdx) ;// isManual
	}

#if DEBUG
else if (nextLoopset->isAutoRecord) printf("DEBUG: triggerLoops() Loopset full fill=%d\n" , nextLoopset->fill) ;
#endif // DEBUG

	this->prevLoopsetIdx = this->nextLoopsetIdx ;

#if DEBUG
printf("DEBUG: triggerLoops() prevLoopIdx = %d nextLoopIdx = %d\n" , prevLoopIdx , nextLoopIdx) ;
#endif // DEBUG:
}

void Handsfree::toggleAutoRecord()
{
#if DEBUG
printf("DEBUG: handleKeypress() pulse exists\n") ;
#endif // DEBUG

	Loopset* nextLoopset = this->Loopsets[this->nextLoopsetIdx] ;
	nextLoopset->isAutoRecord = !nextLoopset->isAutoRecord ;

if (critters) printf(TOGGLE_AUTORECORD_DBG , nextLoopset->isAutoRecord) ;
}

