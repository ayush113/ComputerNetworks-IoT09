/*
 * Wireless Networks Group, Department of Networks Engineering, UPC, Barcelonatech (2015)
 */

/**
 * \file
 *      An implementation of the CoAP Congestion Control Simple /Advanced  (draft-bormann-core-cocoa-02).
 *      This implementation only considers advanced congestion control mechanisms for the exchange of
 *      CON messages.
 *
 * \author
 *     August Betzler <august.betzler@entel.upc.edu>
 */

#include "er-coap-transactions.h"


#define DEBUG   1
#include "ip/uip-debug.h"

// #define SMALLVBF 1.5
// #define LARGEVBF 2.5
// #define LOWERVBFTHRESHOLD 1
// #define UPPERVBFTHRESHOLD 3

#define MAXIMUM_OVERALLRTO 60 * CLOCK_SECOND

// #define ALPHA 0.125
// #define BETA 0.25
// #define STRONGK 4
// #define WEAKK   1

// #define STRONGESTIMATOR 0
// #define WEAKESTIMATOR 1
// #define OVERALLESTIMATOR 2

#define GAIN 0.3

// #define G 0.01
// K-Value and weighting for the strong and weak estimators, respectively
//static uint8_t kValue[] = {4,1};
//static double weight[] = {0.5, 0.25};

uint8_t
countTransactionsForAddress(uip_ipaddr_t *addr, list_t transactions_list){
	coap_transaction_t *t = NULL;
	uint8_t counter = 0;
	  for (t = (coap_transaction_t*)list_head(transactions_list); t; t = t->next){
	    if (uip_ipaddr_cmp(addr, &t->addr)){
	     counter++;
	    }
	  }
	  return counter;
}

signed long long 
coap_update_rtt_estimation(coap_rtt_estimations_t *t){

  clock_time_t rto;
  signed long long rtoReal;
  int gainBar;
  int delta_is_pos=0;
  signed long long delta;
  signed long long rttNew = (signed long long) t->rtt;

  printf("RTT: %lu\n",t->rtt);
  if(rttNew > t->srtt ){
	delta = t->rtt - t->srtt;
	delta_is_pos = 1;
	}
  else{
	delta = t->srtt - t->rtt;
	delta_is_pos = 0;
       }

  printf("DELTA : %lld\n",delta);
  if(delta_is_pos)
  t->srtt += delta * 0.33;
  else
   t->srtt += -1 * delta * 0.33;
  printf("SRTT : %lld\n",t->srtt); 

  if(delta - t->rttvar >=0 && delta_is_pos) {
        printf("Case 1");
	gainBar = 1;
	}
  else if(delta - t->rttvar < 0 && delta_is_pos){
	 printf("Case 2");
	gainBar = 0;
	}
  else if(-delta - t->rttvar >= 0 && !delta_is_pos){
         printf("Case 3");
	gainBar = 1;
	}
  else  {
	 printf("Case 4");
	gainBar = 0;
}

  if(delta_is_pos && gainBar == 1)
	t->rttvar +=  (delta - t->rttvar) * 0.33;
  else if( delta_is_pos && gainBar ==0)
   	t->rttvar +=  (delta - t->rttvar) * 0.09; 
    
  printf("RTTVAR : %lld\n",t->rttvar);

  rtoReal = t->srtt + 3 * t->rttvar ;
 
  if( rtoReal < t->srtt + 2)
	rtoReal = t->srtt + 2;
 

  //printf("ADDRESS : %lu\n",t->addr);
  

  return rtoReal; 
}


coap_rtt_estimations_t *
coap_check_rtt_estimation(uip_ipaddr_t* transactionAddr, list_t rtt_estimations_list){
  coap_rtt_estimations_t *t = NULL;
  // Go through the list of RTT info stored for different destination endpoints
    for (t = (coap_rtt_estimations_t*)list_head(rtt_estimations_list); t; t = t->next){
      if (uip_ipaddr_cmp(transactionAddr, &t->addr)){
        return t;
      }
    }
    return NULL;
}
