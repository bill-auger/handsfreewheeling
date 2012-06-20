#include <stdio.h>

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
void Handsfree::HandlePulse()
{
	Loopset* loopset = this->Loopsets[getPulseIdx()] ;
	if (loopset->isAutoRecord)
	{
		if (loopset->fill >= MAX_LOOPSET_FILL) return ;

printf("DEBUG: AutoRecord on\n") ;
// TODO: KLUDGE: increment this to 1 in handleKeypress() on 3rd press (xml cfg is triggering new loops now)
if (!loopset->fill) loopset->fill = 1 ; // we only can be here after a pulse exists

		int loopIdx = loopset->baseIdx + loopset->fill ;

dbgLoopsStatus(loopset) ; // DEBUG:

		if (getLoopStatus(loopIdx) == T_LS_Recording)
		{
			if (critters) printf("HANDSFREE: stopping recording loop %d\n" , loopIdx) ;
			triggerLoop(loopIdx++) ; loopset->fill = loopset->fill + 1 ;
		}
		if (loopset->fill < MAX_LOOPSET_FILL)
		{
			if (critters) printf("HANDSFREE: recording new loop %d\n" , loopIdx) ;
			triggerLoop(loopIdx) ;
		}
	}
else printf("DEBUG: AutoRecord off\n") ;
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

// handsfree functions
void Handsfree::handleKeypress(Event* ev)
{
	KeyInputEvent* kev = (KeyInputEvent*)ev ; if (!kev->down || this->keyMutex) return ;

	// keysyms listed in /usr/include/SDL/SDL_keysym.h (SDLK_a == 97)
	int pulseIdx ; if (kev->keysym != SDLK_SPACE || !(~(pulseIdx = getPulseIdx()))) return ;

	this->keyMutex = true ;
	Loopset* loopset = this->Loopsets[pulseIdx] ;
	loopset->isAutoRecord = !loopset->isAutoRecord ;

//int loopIdx = 5 ; triggerLoop(loopIdx) ;

	if (critters) printf("HANDSFREE: Toggle AutoRecord (%d)\n" , loopset->isAutoRecord) ;
	usleep(250) ; this->keyMutex = false ;

/* expanded (if we need to handle more keys)
  KeyInputEvent* kev = (KeyInputEvent*)ev ; if (!kev->down || this->keyMutex) return ;

	// keysyms listed in /usr/include/SDL/SDL_keysym.h (SDLK_a == 97)
	if (kev->keysym == SDLK_SPACE)
	{
		int pulseIdx = getPulseIdx() ; if (!(~pulseIdx)) return ;

		this->keyMutex = true ;
		this->autoRecord[pulseIdx] = !this->autoRecord[pulseIdx] ;
		if (critters) printf("HANDSFREE: Toggle AutoRecord (%d)\n" , this->autoRecord[pulseIdx]) ;
		usleep(250) ; this->keyMutex = false ;
	}
*/
}

