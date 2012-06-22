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

#define DEBUG 1

#define HANDSFREE 1
#define N_LOOPSETS 3
#define MAX_LOOPSET_FILL 9
#define BASE_LOOPIDX_MULTIPLIER 10 
#define PULSE_NONE -1
// keysyms listed in /usr/include/SDL/SDL_keysym.h (SDLK_a == 97)
#define LOOPSET_KEY SDLK_KP0
#define RECORD_KEY SDLK_SPACE
#define SUPRESS_KEYPRESS_INTERVAL 0.25
#define SELECT_PULSE_WAIT 100000
#define REGISTER_EVENT_DBG "HANDSFREE: Registering event listeners\n"
#define UNREGISTER_EVENT_DBG "HANDSFREE: Unregistering event listeners\n"
#define INITIAL_LOOP_DBG "HANDSFREE: recording initial loop %d\n"
#define CREATE_PULSE_DBG "HANDSFREE: end recording initial loop %d and set pulse\n"
#define NEW_LOOP_DBG "HANDSFREE: recording new loop %d\n"
#define END_RECORD_DBG "HANDSFREE: end recording loop %d\n"
#define TOGGLE_AUTORECORD_DBG "HANDSFREE: AutoRecord (%d)\n"

struct Loopset
{
	int baseIdx ; // loopIdx of first loop
	int fill ; // current number of loops in this set
	int pulseIdx ; // matches index into the Loopset array or -1 when no pulse exists
	bool isAutoRecord ; // flag to auto-record on each pulse wrap - set by key event
} ;

class Handsfree : public EventProducer , public EventListener
{
	public:
		// called from fweelin_core.cc just before entering SDL loop
		Handsfree(Fweelin* aFweelin) :
				app(aFweelin) , lastKeypress(0.0) , nextLoopsetIdx(0) , prevLoopsetIdx(0)
		{
			for (int i = 0 ; i < MAX_PULSES ; ++i)
			{
				Loopset* loopset = new Loopset() ;
				loopset->baseIdx = (i + 1) * BASE_LOOPIDX_MULTIPLIER ; loopset->fill = 0 ;
				loopset->pulseIdx = PULSE_NONE ; loopset->isAutoRecord = true ;				
				this->Loopsets[i] = loopset ;
			}
			this->addListeners() ;	
		} ;
		~Handsfree () { this->removeListeners() ; } ;

		// event listeners
		void ReceiveEvent(Event *ev, EventProducer *from) ;

		// handsfree functions
		void HandlePulse(int wrappedPulseIdx) ; // called from fweelin_core_dsp.cc on pulse wrap

	private:
		Fweelin* app ; // handle to main app instance (for event listener setup/teardown)
		double lastKeypress ; // workaround for duplicate key events (TODO: is this a bug?)
		Loopset* Loopsets[MAX_PULSES] ; // array of Loopset data structs
		int nextLoopsetIdx ; // idx into Loopset array - set by key event
		int prevLoopsetIdx ; // cache for transitions

		// event listeners
		void addListeners() ;
		void removeListeners() ;

		// helpers
		double getTimestamp() ; // workaround for duplicate key events (TODO: is this a bug?)
		bool isTimestampStale() ;
		void setTimestamp() ;
		int getPulseIdx() ;
		int getLoopStatus(int loopIdx) ;
		void triggerLoop(int loopIdx) ;
		void selectPulse(int pulseIdx) ;

		// handsfree functions
		void handleKeypress(Event* ev) ;
		void toggleLoopset() ;
		void triggerLoops() ;
		void toggleAutoRecord() ;

// DEBUG:
void dbgLoopsStatus(Loopset* loopset) ;
// DEBUG: end
} ;

#endif // __FWEELIN_HANDSFREE_H
