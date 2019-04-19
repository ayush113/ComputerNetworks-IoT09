#### We have faced the following issues during implementation and testing : <br>

1. Initially we faced a memory error(overflow) when we implemented Eifel in CoAP.
2. While testing with a dumbbell topology we observed that some clients were not receiving packets at all.  
3. While testing with a simple topology we observed that no CONFIRMABLE packets were being transmitted.
4. RTO values obtained are always linearly increasing because RTT is always linearly increasing and RTT.
5. The above issue is due to mishandling of negative values in CoAP. So we looked at results of authors of Eifel Retransmission timer and realised they have scaled their values. As scaling was not possible due to time constraints, with simple mathematical manipulation and looping we handled negative values.

#### Issues left pending : <br>
The calculated RTT lies between 0-2 most of the time. This causes an issue in our RTO prediction as it tries to converge to 0-2 range but encounters a spike in RTT which causes it to start over. 


