#include <stdio.h>

#include "fweelin_core.h"
#include "fweelin_handsfree.h"


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
	if (this->autoRecord[getPulseIdx()])
	{
		printf("HANDSFREE: AutoRecord on\n") ; 
	}
	else printf("HANDSFREE: AutoRecord off\n") ; 
}


// private

// event listeners
void Handsfree::addListeners()
{
	// these are listed in fweelin_event.cc:160
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


// handsfree functions
void Handsfree::handleKeypress(Event* ev)
{
	KeyInputEvent* kev = (KeyInputEvent*)ev ; if (!kev->down || this->keyMutex) return ;

	// keysyms listed in /usr/include/SDL/SDL_keysym.h (SDLK_a == 97)
	int pulseIdx ; if (kev->keysym != SDLK_SPACE || !(~(pulseIdx = getPulseIdx()))) return ;

	this->keyMutex = true ;
	this->autoRecord[pulseIdx] = !this->autoRecord[pulseIdx] ;
	printf("HANDSFREE: Toggle AutoRecord (%d)\n" , this->autoRecord[pulseIdx]) ;
	usleep(250) ; this->keyMutex = false ;

/* expanded (if we need to handle more keys)
  KeyInputEvent* kev = (KeyInputEvent*)ev ; if (!kev->down || this->keyMutex) return ;

	// keysyms listed in /usr/include/SDL/SDL_keysym.h (SDLK_a == 97)
	if (kev->keysym == SDLK_SPACE)
	{
		int pulseIdx = getPulseIdx() ; if (!(~pulseIdx)) return ;

		this->keyMutex = true ;
		this->autoRecord[pulseIdx] = !this->autoRecord[pulseIdx] ;
		printf("HANDSFREE: Toggle AutoRecord (%d)\n" , this->autoRecord[pulseIdx]) ;
		usleep(250) ; this->keyMutex = false ;
	}
*/
}
