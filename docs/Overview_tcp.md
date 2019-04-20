# Overview of TCP timer calculations

**Current TCP RTO prediction**<br/><br/>
![TCP Calc](https://imgur.com/vOpWKIe.png)

"SRTT" is the predicted value of RTT.<br/>
"RTTVAR" is the deviation of RTT from the estimated value.<br/>
"RTO" is the prediction of upper limit of the RTT.<br/>
"ticks" indicates the granularity of timers in OS.<br/>

## Problems with the current RTO prediction in TCP

Eifel Retransmission timer was proposed to handle the problems related to conventional TCP RTO calculations. Four major problems were identified with the conventional TCP RTO calculations. The first, third and fourth problem make the timer conservative whereas the second problem makes the timer aggressive. The problems with the existing TCP RTO calculations has been briefly described below.<br/>



<b>1. Prediction Flaw when the RTT drops</b><br>
The usage of the absolute value of DELTA in the RTTVAR calculations causes an undesirable behaviour in the RTO predictor. The RTO predictor goes up although the signal goes down.  When DELTA becomes negative, the connection's RTT drops to the extent that it falls below SRTT but the RTO predictor causes the RTO to rise as if the RTT has increased by the same amount. This leads to an RTO that largely over predicts the RTT, and it takes some time until the RTO has decayed to a reasonable level. <br><br>
<b>2. Failure of the "Magic Numbers"</b><br>
The calculations for the estimator gains and the variation weight were made under the assumption that only one segment per flight was timed. If the sampling rate is greater than 1, these constants fail. The variation weight becomes too low to raise the RTO, whereas the estimator gains are too high. This causes the RTO to decay into RTT too quickly. <br><br>
<b>3. The "REXMT-Restart Bug"</b><br>
REXMT is re-initialised with RTO on the arrival of ACK for the oldest outstanding segment. This does not account for the age of the new oldest outstanding segment. <br><br>
<b>4. Timer Granularity</b><br>
The higher the time granularity, the more imprecise and conservative the the RTO is. The timer granularity must be an order less than RTT. The worst case RTT's commonly found in the internet are of the order of a few 100ms. Thus a timer granularity of 500ms is inadequate and makes the timer conservative. <br><br>
 
