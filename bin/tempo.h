/*

Simple libreria para temporizar eventos en arduino.

Simple library for events timing in arduino.

*/

#ifndef tempo_h
#define tempo_h
#include <Arduino.h>

class Tempo {
public:

	Tempo(int dt){
		_dt=dt;
		_prevmillis=0;
	};

	bool state(){
		if((millis() - _prevmillis) > _dt){
			_prevmillis=millis();
			return true;
		}
		else{return false;};
	}
	
	void setTempo(int dt){ _dt=dt;};
	
	


private:
	
	unsigned long _prevmillis;
	int _dt;

};

#endif
