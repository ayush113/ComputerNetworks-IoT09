## Implementation Details in COAP

**There were two options to implement the Eifel Retransmission Timer**:<br/>
**1. Implementing in COAP**<br/>
**2. Implementing in CoCoA**<br/>

<br/>
CoCoA already has RTO estimation functionality which has a strong estimator and a weak estimator<br/>
This adds a layer of complexity to the implementation, thus we chose to go ahead with COAP.

## Implementation in COAP
COAP uses a constant value for RTO and it doesn't have any mechanism to predict the RTO.<br/>
It doesn't have any mechanism for RTT prediction which is needed for the Eifel RTO prediction.<br/>
We have to start with RTT estimation and then move on to calculating the RTO values based on these estimations.<br/>
### 1. Adding RTT Estimation
RTT estimation was added in two steps, the first task was to store the time at which the packet is being sent<br/>
and then use it in the second step to calculate the RTT. The RTT is calculated at the time of receiving the ACK,<br/>
the time at which ACK is received is subtracted from the time stored at the beginning.<br/>
This latest rtt sample between a source and a destination is stored in a list.<br>
The following code show the implementation of RTT Estimation.
<br/>

#### Declaring timestamp
```C
typedef struct coap_transaction {
  struct coap_transaction *next;       
  uint16_t mid;
  struct etimer retrans_timer;
  uint8_t retrans_counter;
  uip_ipaddr_t addr;
  uint16_t port;
  restful_response_handler callback;
  void *callback_data;
  uint16_t packet_len;
  uint8_t packet[COAP_MAX_PACKET_SIZE + 1];
  clock_time_t timestamp;                                                     
} coap_transaction_t;
```
#### Calculating RTT at the arrival of ACK
```C
coap_clear_transaction(coap_transaction_t *t)
{
  coap_rtt_estimations_t *prevseg = NULL;
  int pktnotfound = 1;
  if(t) {
    if(COAP_TYPE_CON == ((COAP_HEADER_TYPE_MASK & t->packet[0]) >> COAP_HEADER_TYPE_POSITION)) {
        if(t->retrans_counter < COAP_MAX_RETRANSMIT) {
   
            clock_time_t rtt = clock_time() - t->timestamp; 
        for (prevseg = (coap_rtt_estimations_t*)list_head(rtt_estimations_list); prevseg; prevseg = prevseg->next){
                      if (uip_ipaddr_cmp(&prevseg->addr, &t->addr)){
                              prevseg->rtt = rtt;
                              pktnotfound = 0;
                              break;
                      }
                }
                if(pktnotfound){
                      coap_rtt_estimations_t *e = memb_alloc(&rtt_estimations_memb);
                      if(e){
                        e->rtt = rtt;
                          e->rttvar = 0;
                          e->srtt = 0;
                          e->srttCoef = 1;
                          e->rttvarCoef = 1;

                          uip_ipaddr_copy(&e->addr, &t->addr);
                          list_add(rtt_estimations_list, e);
                       prevseg = e;
                }
              }
        calculateRTO(prevseg);
            //printf("RTT %lu\n",rtt);
    }
    }

    etimer_stop(&t->retrans_timer);
    list_remove(transactions_list, t);
    memb_free(&transactions_memb, t);
  }
}
```
### 2. Calculating RTO from RTT
RTO calculation utilises two functions, **check_rtt** which is used to retrieve RTO estimation for packets and **calculateRTO** which is used to do the actual calculation. The images below contain the code snippets for the two functions.<br/>
#### Retrieves the RTO estimation
```C
clock_time_t check_rtt(coap_transaction_t *t){
    coap_rtt_estimations_t *prevseg = NULL;

    for (prevseg = (coap_rtt_estimations_t*)list_head(rtt_estimations_list); prevseg; prevseg = prevseg->next){
                      if (uip_ipaddr_cmp(&prevseg->addr, &t->addr)){
                    return prevseg->rto;
                }
    }
    return COAP_INITIAL_RTO;    
}
```
#### RTO calculation using Eifel
```C
void calculateRTO(coap_rtt_estimations_t *t){
    clock_time_t delta;
    clock_time_t rto;
    int deltaCoef,gainbar,gain=3;

    //Calculate delta
    if(t->srttCoef == 1){
      if(t->rtt >= t->srtt){
        delta = t->rtt - t->srtt;
        deltaCoef = 1;
      }
      else{
        delta = t->srtt - t->rtt;
        deltaCoef = 0;
      }
    }
    else{
      delta = t->rtt + t->srtt;
      deltaCoef = 1;
    }

    //printf("DELTA COEF:%d\n",deltaCoef);
    //printf("DELTA:%ld\n",delta);
    //calculate inverse of gain
    if(deltaCoef == 1){
      if(t->rttvarCoef == 0){
        gainbar = 3;
      }
      else{
        if(delta >= t->rttvar)
          gainbar = 3;
        else
          gainbar = 9;
      }
    }
    else{
      if(t->rttvarCoef == 1)
        gainbar = 9;
      else{
        if(t->rttvar >= delta)
          gainbar = 3;
        else
          gainbar = 9;
      }
    }

    //Calculate srtt
    if(t->srttCoef == 1){
      if(deltaCoef == 1){
        t->srtt = t->srtt + delta/3;
        t->srttCoef = 1;
      }
      else{
        if(t->srtt >= delta/3){
          t->srtt = t->srtt - delta/3;
          t->srttCoef = 1;
        }
        else{
          t->srtt = delta/3 - t->srtt;
          t->srttCoef = 0;
        }
      }
    }
    else{
      if(deltaCoef == 1){
        if(delta/3 >= t->srtt){
          t->srtt = delta/3 - t->srtt;
          t->srttCoef = 1;
        }
        else{
          t->srtt = t->srtt - delta/3;
          t->srttCoef = 0;
        }
      }
      else{
        t->srtt = t->srtt + delta/3;
        t->srttCoef = 0;
      }
    }
    //printf("SRTT COEF:%d\n",t->srttCoef );
    //printf("SRTT:%ld\n",t->srtt);
    //Calculate rttvar
    if(deltaCoef == 1){
      if(t->rttvarCoef == 1){
        if(delta >= t->rttvar){
          //printf("case 1\n");
          t->rttvar = t->rttvar + (delta - t->rttvar)/gainbar ;
          t->rttvarCoef = 1;
        }
        else{
          if(t->rttvar >= (t->rttvar - delta)/gainbar){
        //printf("case 2\n");
        t->rttvar = t->rttvar - (t->rttvar - delta)/gainbar ;
        t->rttvarCoef = 1;
          }
          else{
        //printf("case 3\n");
        t->rttvar = (t->rttvar - delta)/gainbar - t->rttvar ;
        t->rttvarCoef = 0;
          }
        }
      }
      else{
        if(t->rttvar <= (delta + t->rttvar)/gainbar){
          //printf("case 4\n");
          t->rttvar = (delta + t->rttvar)/gainbar - t->rttvar;
          t->rttvarCoef = 1;
        }
        else{
          //printf("case 5\n");
          t->rttvar = t->rttvar - (delta + t->rttvar)/gainbar;
          t->rttvarCoef = 0;
        }
      }
    }

    //printf("RTTVAR COEF:%d\n",t->rttvarCoef );
    //printf("RTTVAR:%ld\n",t->rttvar);

    if(t->srttCoef == 1){
      if(t->rttvarCoef == 1){
        if((t->rtt + 2) > t->srtt + t->rttvar*gain)
          rto = t->rtt + 2;
        else
          rto = t->srtt + t->rttvar*gain;

      }
      else{
        if(t->srtt >= t->rttvar*gain){
          if((t->rtt + 2) > t->srtt - t->rttvar*gain)
        rto = t->rtt + 2;
          else
        rto = t->srtt - t->rttvar*gain;
        }
        else{
          rto = t->rtt + 2;
        }
      }
    }
    else{
      if(t->rttvarCoef == 1){
        if(t->rttvar*gain >= t->srtt){
          if((t->rttvar*gain - t->srtt) >= t->rtt + 2)
        rto = t->rttvar*gain - t->srtt;
          else
        rto = t->rtt + 2;
        }
      }
      else
        rto = t->rtt + 2;
    }
    printf("RTO : %lu\n",rto);
    t->rto = rto;
}
```

