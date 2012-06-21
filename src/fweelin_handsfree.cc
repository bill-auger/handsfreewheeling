#include <stdio.h>
#include <sys/time.h>

#include "fweelin_handsfree.h"

// DEBUG:
#define DEBUG 1
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
void Handsfree::HandlePulse()
{
	bool isSetChange = (this->nextLoopsetIdx != this->prevLoopsetIdx) ;
	Loopset* prevLoopset ; Loopset* nextLoopset ; int prevLoopIdx , nextLoopIdx ;
	prevLoopset = this->Loopsets[this->prevLoopsetIdx] ;
	nextLoopset = this->Loopsets[this->nextLoopsetIdx] ;
	prevLoopIdx = prevLoopset->baseIdx + prevLoopset->fill ;

#if DEBUG
printf("\nDEBUG: HandlePulse() isAutoRecord = %s\n" , (nextLoopset->isAutoRecord)? "true" : "false") ;
printf("DEBUG: HandlePulse() isSetChange = %s\n" , (isSetChange)? "true" : "false") ;
printf("DEBUG: HandlePulse() no pulse %s recording\n" , (getLoopStatus(prevLoopIdx) == T_LS_Recording)? "was" : "not") ;
printf("DEBUG: HandlePulse() prevLoopset = %d nextLoopset = %d\n" , this->prevLoopsetIdx , this->nextLoopsetIdx) ;
#endif // DEBUG:

	this->prevLoopsetIdx = this->nextLoopsetIdx ;

	if (getLoopStatus(prevLoopIdx) == T_LS_Recording)
	{
		if (critters) printf("HANDSFREE: stopping recording loop %d\n" , prevLoopIdx) ;
		triggerLoop(prevLoopIdx) ; prevLoopset->fill = prevLoopset->fill + 1 ;
	}
	if (isSetChange) selectPulse(nextLoopsetIdx) ;
	if (nextLoopset->isAutoRecord && nextLoopset->fill < MAX_LOOPSET_FILL)
	{
// TODO: KLUDGE: increment this to 1 in handleKeypress() on 3rd press (xml cfg is triggering new loops now)
if (!nextLoopset->fill) nextLoopset->fill = 1 ; // we only can be here after a pulse exists

		nextLoopIdx = nextLoopset->baseIdx + nextLoopset->fill ;

#if DEBUG
printf("DEBUG: handlePulse() prevLoopIdx = %d nextLoopIdx = %d\n" , prevLoopIdx , nextLoopIdx) ;
dbgLoopsStatus(nextLoopset) ;
#endif // DEBUG:

		if (critters) printf("HANDSFREE: recording new loop %d\n" , nextLoopIdx) ;
		triggerLoop(nextLoopIdx) ;
	}

#if DEBUG
else if (!nextLoopset->isAutoRecord) printf("DEBUG: Loopset full fill=%d\n" , nextLoopset->fill) ;
#endif // DEBUG
}


// private

// event listeners
void Handsfree::addListeners()
{
	// events are listed in fweelin_event.cc:160
	printf("HANDSFREE: Registering event listeners\n") ;
	this->app->getEMG()->ListenEvent(this , 0 , T_EV_Input_Key) ;
}

void Handsfree::removeListeners()
{
	printf("HANDSFREE: Unregistering event listeners\n") ;
	this->app->getEMG()->UnlistenEvent(this , 0 , T_EV_Input_Key) ;
}


// helpers
double Handsfree::getTimestamp()
{
	struct timeval tv ; gettimeofday(&tv , NULL) ;
	return (double)tv.tv_sec + ((double)1e-6 * tv.tv_usec) ;
}

//bool Handsfree::getKeyMutexState() { return this->keyMutex ; }
bool Handsfree::isTimestampStale() { return (getTimestamp() - this->lastKeypress) > 0.25 ; }

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
}

// handsfree functions
void Handsfree::handleKeypress(Event* ev)
{
	KeyInputEvent* kev = (KeyInputEvent*)ev ;
	if ((kev->keysym == TRIGGER_LOOP_KEY || kev->keysym == TOGGLE_LOOPSET_KEY) &&
		kev->down && isTimestampStale()) setTimestamp() ; else return ;

#if DEBUGVB
if (kev->down) printf("handleKeypress() key is down\n") ; else return ;
if (!getKeyMutexState()) printf("handleKeypress() mutex is free\n") ; else return ;
if (kev->keysym == TRIGGER_LOOP_KEY) printf("handleKeypress() key is TRIGGER_LOOP_KEY\n") ;
if (kev->keysym == TOGGLE_LOOPSET_KEY) printf("handleKeypress() key is TOGGLE_LOOPSET_KEY\n") ;
#endif // DEBUGVB

	if (kev->keysym == TOGGLE_LOOPSET_KEY) handleToggleLoopsetKey() ;
	else if (kev->keysym == TRIGGER_LOOP_KEY) handleTriggerLoopKey() ;
}

void Handsfree::handleToggleLoopsetKey()
	{ this->nextLoopsetIdx = (this->nextLoopsetIdx + 1) % N_LOOPSETS ; }

void Handsfree::handleTriggerLoopKey()
{
	// 1st call per loopset - record first loop
	// 2nd call per loopset - increment counter - create pulse - record second loop		
	// sunbsequent calls - loops will now auto-record on pulse wrap in HandlePulse()
	if (!(~getPulseIdx())) // no pulse
	{
		bool isSetChange = (this->nextLoopsetIdx != this->prevLoopsetIdx) ;
		Loopset* prevLoopset ; Loopset* nextLoopset ; int prevLoopIdx , nextLoopIdx ;
		prevLoopset = this->Loopsets[this->prevLoopsetIdx] ;
		nextLoopset = this->Loopsets[this->nextLoopsetIdx] ;
		prevLoopIdx = prevLoopset->baseIdx + prevLoopset->fill ;

// TODO: what we really want to know is if the first loopid is empty
//		for now if not T_LS_Recording assume the first loop does not exist
#if DEBUG
printf("\nDEBUG: handleKeypress() isAutoRecord = %s\n" , (nextLoopset->isAutoRecord)? "true" : "false") ;
printf("DEBUG: handleKeypress() isSetChange = %s\n" , (isSetChange)? "true" : "false") ;
printf("DEBUG: handleKeypress() no pulse %s recording\n" , (getLoopStatus(prevLoopIdx) == T_LS_Recording)? "was" : "not") ;
printf("DEBUG: handleKeypress() prevLoopset = %d nextLoopset = %d\n" , this->prevLoopsetIdx , this->nextLoopsetIdx) ;
#endif // DEBUG:

		if (getLoopStatus(prevLoopIdx) == T_LS_Recording)
		{
			triggerLoop(prevLoopIdx) ; prevLoopset->fill = prevLoopset->fill + 1 ; 
			selectPulse(this->prevLoopsetIdx) ;

while (!(~getPulseIdx())) { printf("waiting for pulse\n") ; usleep(100000) ; }
		}

		if (isSetChange) selectPulse(this->nextLoopsetIdx) ;
		nextLoopIdx = nextLoopset->baseIdx + nextLoopset->fill ;
		if (nextLoopset->isAutoRecord && nextLoopset->fill < MAX_LOOPSET_FILL)
			triggerLoop(nextLoopIdx) ;

		this->prevLoopsetIdx = this->nextLoopsetIdx ;

#if DEBUG
printf("DEBUG: handleKeypress() prevLoopIdx = %d nextLoopIdx = %d\n" , prevLoopIdx , nextLoopIdx) ;
#endif // DEBUG:
	}
	else // pulse exists - just toggle isAutoRecord
	{
#if DEBUG
printf("handleKeypress() pulse exists\n") ;
#endif // DEBUG

		Loopset* nextLoopset = this->Loopsets[this->nextLoopsetIdx] ;
		nextLoopset->isAutoRecord = !nextLoopset->isAutoRecord ;
		if (critters) printf("HANDSFREE: AutoRecord (%d)\n" , nextLoopset->isAutoRecord) ;
	}
}

