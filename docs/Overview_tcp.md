# Overview of Eifel Retransmission Timer
In order to tackle the problems of the existing TCP timer, the Eifel Timer was introduced. The RTO predictor is described by the following equations - <br>
![Eifel Retransmission Timer ](https://i.imgur.com/WObFdkm.png)

The Eifel Retransmission Timer handles the problems in the following manner - <br><br>
<b>1. Predicting a decreasing RTT</b><br>
We define RTTVAR to be constant when DELTA is smaller than zero. Thus the RTO decreases only as fast as the SRTT decreases. Thus RTO derived from Eifel does experience the spikes seen with TCP timer when RTT drops.<br><br>
<b>2. Scaling the Gains and the Variation Weight</b><br>
The Eifel Timer replaces the constant estimator gains described in the TCP timer with a single gain for RTTVAR and SRTT. The gain scales with sender's load which depends on RTT sampling rate. The idea is to distribute the entire weight of 1 equally over the number of RTT samples per flight. The Eifel timer describes a different gain factor for different sampling rates. With a RTT sampling rate of 1 this leads to an estimator gain which is the reciprocal of the sender's load, and it leads to twice that gain when delayed ACKs are used. If only one RTT sample is obtained per RTT, we define 1/3 as the estimator gain. This number proves to be safe against spurious timeouts. Likewise, we define the variation weight as the reciprocal of the estimator gain and thereby also make it scale with the sender's load.<br><br>
<b>3. Shock absorbers</b><br>
During the second half of each flight we can see the RTTVAR decrease rapidly. At the end of each flight we can see RTO get too close to RTT. This is especially problematic for small loads. To avoid this, Eifel defines the gain of RTTVAR to be the square of the estimator gain of SRTT whenever RTTVAR is decreasing. Thus, the variation goes up quickly but comes down slowly. <br><br>
<b>4. The RTO Minimum</b><br>
The RTO minimum is necessary to protect against spurious timeouts when RTO is close to the timer granularity. In addition to the 2 ticks, it is also safe to assume that the RTO must not drop below the latest RTT sample. The the Eifel Timer re-defines the RTO minimum to be the sum of 2 ticks and the latest RTT sample.<br><br>
<b>5. Implementing REXMT precisely</b><br>
The Eifel Timer stores the timestamp of when each segment is sent. This helps to implement the REXMT precisely. The Eifel Timer implements the REXMT according to the following definition -<br>
REXMT = RTO - 'Age of the oldest outstanding segment'.<br><br>


