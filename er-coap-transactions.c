/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *      CoAP module for reliable transport
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#include "contiki.h"
#include "contiki-net.h"
#include "er-coap-transactions.h"
#include "er-coap-observe.h"

#define DEBUG 0
#include "ip/uip-debug.h"



/*---------------------------------------------------------------------------*/
MEMB(transactions_memb, coap_transaction_t, COAP_MAX_OPEN_TRANSACTIONS);
LIST(transactions_list);
MEMB(rtt_estimations_memb, coap_rtt_estimations_t, COAP_MAX_RTT_ESTIMATIONS);
LIST(rtt_estimations_list);

static struct process *transaction_handler_process = NULL;

/*---------------------------------------------------------------------------*/
/*- Internal API ------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
void
coap_register_as_transaction_handler()
{
  transaction_handler_process = PROCESS_CURRENT();
}

coap_transaction_t *
coap_new_transaction(uint16_t mid, uip_ipaddr_t * addr, uint16_t port)
{
  #ifdef COCOA
  //AUGUST
  if(countTransactionsForAddress(addr, transactions_list) >= NSTART){
	PRINTF("NSTART limit reached!\n");
	return NULL;
  }
  #endif
  coap_transaction_t *t = memb_alloc(&transactions_memb);

  if(t) {
    t->mid = mid;
    t->retrans_counter = 0;


    /* save client address */
    uip_ipaddr_copy(&t->addr, addr);
    t->port = port;

    list_add(transactions_list, t); /* list itself makes sure same element is not added twice */

  }

  return t;
}
/*---------------------------------------------------------------------------*/
void
coap_send_transaction(coap_transaction_t * t)
{
  coap_rtt_estimations_t *prevseg;
  //printf("Hello again");
  PRINTF("Sending transaction %u\n", t->mid);

  coap_send_message(&t->addr, t->port, t->packet, t->packet_len);

  if(COAP_TYPE_CON ==
     ((COAP_HEADER_TYPE_MASK & t->packet[0]) >> COAP_HEADER_TYPE_POSITION)) {
    if(t->retrans_counter < COAP_MAX_RETRANSMIT) {
      /* not timed out yet */
      PRINTF("Keeping transaction %u\n", t->mid);

      if(t->retrans_counter == 0) {
      //August
	#ifdef COCOA
        prevseg = coap_check_rtt_estimation(&t->addr, rtt_estimations_list);
        //printf("Address :%lu\n",t->addr);
        if(prevseg){
          t->rto = (clock_time_t) coap_update_rtt_estimation(prevseg);
    	  
          if(t->rto < CLOCK_SECOND)
 		t->rto = CLOCK_SECOND;
          else if( t->rto > 60 * CLOCK_SECOND)
		t->rto = 60 * CLOCK_SECOND;
	printf("RTO  :%lu\n",t->rto);
	//printf("FOUND");
        }

        else{
          t->rto = COAP_INITIAL_RTO;
          //printf("ADDRESS :%u RTO %lu:\n",t->addr,COAP_INITIAL_RTO);
        }
	  
        t->retrans_timer.timer.interval = t->rto;


        t->timestamp = clock_time();
	//printf("Timestamp: %lu\n",t->timestamp);
      #else
        t->retrans_timer.timer.interval =
          COAP_RESPONSE_TIMEOUT_TICKS + (random_rand()
                                         %
                                         (clock_time_t)
                                         COAP_RESPONSE_TIMEOUT_BACKOFF_MASK);
	  #endif
        PRINTF("Initial interval %u\n",
               t->retrans_timer.timer.interval / CLOCK_SECOND);
      } else {
	
 
        t->retrans_timer.timer.interval <<= 1;  /* double */
        PRINTF("Doubled (%u) interval %u\n", t->retrans_counter,
                      t->retrans_timer.timer.interval / CLOCK_SECOND);
      }

      /*FIXME
       * Hack: Setting timer for responsible process.
       * Maybe there is a better way, but avoid posting everything to the process.
       */
      struct process *process_actual = PROCESS_CURRENT();

      process_current = transaction_handler_process;
      etimer_restart(&t->retrans_timer);        /* interval updated above */
      process_current = process_actual;

      t = NULL;
    } else {
      /* timed out */
      PRINTF("Timeout\n");
      restful_response_handler callback = t->callback;
      void *callback_data = t->callback_data;

      /* handle observers */
      coap_remove_observer_by_client(&t->addr, t->port);

      coap_clear_transaction(t);

      if(callback) {
        callback(callback_data, NULL);
      }
    }
  } else {
    coap_clear_transaction(t);
  }
}

/*---------------------------------------------------------------------------*/
void
coap_clear_transaction(coap_transaction_t *t)
{
  coap_rtt_estimations_t *prevseg = NULL;
  int pktnotfound = 1;
  clock_time_t rtt;
  if(t) {
    PRINTF("Freeing transaction %u: %p\n", t->mid, t);
    #ifdef COCOA
    if(t->retrans_counter < COAP_MAX_RETRANSMIT){
      if(COAP_TYPE_CON==((COAP_HEADER_TYPE_MASK & t->packet[0])>>COAP_HEADER_TYPE_POSITION)){
        rtt = clock_time() - t->timestamp;
	if(rtt<CLOCK_SECOND)
		rtt = CLOCK_SECOND;
        for (prevseg = (coap_rtt_estimations_t*)list_head(rtt_estimations_list); prevseg; prevseg = prevseg->next){
          if (uip_ipaddr_cmp(&prevseg->addr, &t->addr)){
              //printf("Clear transaction");
              prevseg->rtt = rtt;
              prevseg->lastupdated = clock_time();
              pktnotfound = 0;
              break;
          }
        }
        if(pktnotfound){
          coap_rtt_estimations_t *e = memb_alloc(&rtt_estimations_memb);
          if(e){
	   e->rtt = rtt;
          e->rttvar = 0;
          e->lastupdated = clock_time();
          e->srtt = 0;
          uip_ipaddr_copy(&e->addr, &t->addr);
          list_add(rtt_estimations_list, e);
        }
        // else{
        //    coap_delete_rtt_by_freshness();
        //   return coap_clear_transaction(t);
        }


        
    }
	
    }
    #endif
    
 

    etimer_stop(&t->retrans_timer);
    list_remove(transactions_list, t);
    memb_free(&transactions_memb, t);
  }
}

coap_transaction_t *
coap_get_transaction_by_mid(uint16_t mid)
{
  coap_transaction_t *t = NULL;

  for(t = (coap_transaction_t *) list_head(transactions_list); t; t = t->next) {
    if(t->mid == mid) {
      PRINTF("Found transaction for MID %u: %p\n", t->mid, t);
      return t;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
void
coap_check_transactions()
{
  coap_transaction_t *t = NULL;

  for(t = (coap_transaction_t *) list_head(transactions_list); t; t = t->next) {
    if(etimer_expired(&t->retrans_timer)) {
      ++(t->retrans_counter);
      PRINTF("Retransmitting %u (%u)\n", t->mid, t->retrans_counter);
      coap_send_transaction(t);
    }
  }
}
/*---------------------------------------------------------------------------*/
