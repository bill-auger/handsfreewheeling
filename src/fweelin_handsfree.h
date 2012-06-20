#ifndef __FWEELIN_HANDSFREE_H
#define __FWEELIN_HANDSFREE_H

#include <math.h>
/* #include <math.h> else compile error
In file included from fweelin_event.h:230:0, from fweelin_handsfree.h:5, from fweelin_handsfree.cc:5:
fweelin_datatypes.h: In member function ‘UserVariable UserVariable::GetDelta(UserVariable&)’:
fweelin_datatypes.h:217:56: error: ‘fabsf’ was not declared in this scope
*/

#include "fweelin_core.h"
#include "fweelin_event.h"

#define HANDSFREE 1
#define MAX_LOOPSET_FILL 9


struct Loopset
{
	int baseIdx ; // loopIdx of first loop
	int fill ; // current number of loops in this set
	bool isAutoRecord ; // flag to auto-record on each pulse wrap - set by key event
} ;

class Handsfree : public EventProducer , public EventListener
{
	public:
		// called from fweelin_core.cc just before entering SDL loop
		Handsfree(Fweelin* aFweelin) : app(aFweelin)
		{
			for (int i = 0 ; i < MAX_PULSES ; ++i)
			{
				Loopset* loopset = new Loopset() ;
				loopset->baseIdx = (i + 1) * 10 ; // TODO: magic #s
				loopset->fill = 0 ; loopset->isAutoRecord = true ;
				this->Loopsets[i] = loopset ;
			}
			this->addListeners() ;	
		} ;
		~Handsfree () { this->removeListeners() ; } ;

		// event listeners
		void ReceiveEvent(Event *ev, EventProducer *from) ;

		// handsfree functions
		void HandlePulse() ; // called from fweelin_core_dsp.cc on pulse wrap

	private:
		Fweelin* app ; // handle to main app instance (for event listener setup/teardown)
		bool keyMutex ; // workaround for duplicate key events (TODO: is this a bug?)
		Loopset* Loopsets[MAX_PULSES] ; // array of Loopset data structs

		// event listeners
		void addListeners() ;
		void removeListeners() ;

		// helpers
		int getPulseIdx() ;
		int getLoopStatus(int loopIdx) ;
		void triggerLoop(int loopIdx) ;

		// handsfree functions
		void handleKeypress(Event* ev) ;

// DEBUG:
void dbgLoopsStatus(Loopset* loopset) ;
// DEBUG: end
} ;

#endif // __FWEELIN_HANDSFREE_H
