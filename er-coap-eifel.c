#include "er-coap-transactions.h"

void calculateRTO(coap_rtt_estimations_t *t){
	signed long long int delta;
	delta = t->rtt - t->srtt;
	int gain = 3;
	int gainbar;
	if((delta - t->rttvar)>=0)
		gainbar = gain;
	else
		gainbar = gain*gain;
	t->srtt = t->srtt + delta/gain;
	if(delta>=0)
		t->rttvar = t->rttvar + (delta - t->rttvar)/gainbar;
	t->rto = (t->srtt + t->rttvar*gain > t->rtt+2 ? t->srtt + t->rttvar*gain : t->rtt+2) ;
}

