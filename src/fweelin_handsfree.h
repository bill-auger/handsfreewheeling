#ifndef __FWEELIN_HANDSFREE_H
#define __FWEELIN_HANDSFREE_H

/* #include <math.h> else compile error
In file included from fweelin_event.h:230:0, from fweelin_handsfree.h:5, from fweelin_handsfree.cc:5:
fweelin_datatypes.h: In member function ‘UserVariable UserVariable::GetDelta(UserVariable&)’:
fweelin_datatypes.h:217:56: error: ‘fabsf’ was not declared in this scope
*/
#include <math.h>
#include "fweelin_event.h"

#define HANDSFREE 1


class Handsfree : public EventListener
{
	public:
		// called from fweelin_core.cc just before entering SDL loop
		Handsfree(Fweelin* aFweelin) : app(aFweelin)
		{
			for (int i = 0 ; i < MAX_PULSES ; ++i) this->autoRecord[i] = true ;
			this->addListeners() ;	
		} ;
		~Handsfree () { this->removeListeners() ; } ;

		// event listeners
		void ReceiveEvent(Event *ev, EventProducer *from) ;

		// handsfree functions
		void HandlePulse() ; // called from fweelin_core_dsp.cc on pulse wrap

	private:
		Fweelin* app ; // handle to main app instance (for event listener setup/teardown)
		bool autoRecord[MAX_PULSES] ; // flag to auto-record on each pulse wrap - set by key event
		bool keyMutex ; // workaround for duplicate key events (TODO: is this a bug?)

		// event listeners
		void addListeners() ;
		void removeListeners() ;

		// helpers
		int getPulseIdx() ;

		// handsfree functions
		void handleKeypress(Event* ev) ;
} ;

#endif // __FWEELIN_HANDSFREE_H
