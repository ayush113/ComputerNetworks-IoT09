## TCP and Eifel Timer
<br/>
1. **[Overview on TCP Retransmission Timer](Overview_tcp.md)**<br/>
2. **[Overview on Eifel Timer](Overview_eifel.md)**<br/>

### Differences between the two timers
<br/>
There are primarily two differences between the timers:<br/>

1. **Change in knobs** :The TCP timer page of our wiki specifies the how the retransmission timer interval is calculated in TCP. Certain knobs such 1/4 , 1/8 and 4 are used as constants in those equations. These are referred to as knobs.Eifel re-evaluates the use of these knobs. Eifel retransmission timer adjusts makes these knobs adaptive based on the flight of the sender and the sampling rate. If the sampling rate is 1 i.e every packet is being sampled the knob is set to 1/FLIGHT meaning that every packet's RTT will be taken into account for that window. Similarly for delayed acks the knob changes to 2/FLIGHT. For other cases the gain is defines as 1/3.
In our project , since any sender has only one packet in flight for a destination the gain is set to 1/3 always.

2. **Value of delta** : TCP uses the absolute value of delta which can create an issue when a timeout occurs and the rtt falls. To overcome this issue as mentioned in the page for problems with TCP timer , Eifel uses teh actual value of delta rather than absolute value. The value of rrtvar is adjusted depending on the sign of delta.


