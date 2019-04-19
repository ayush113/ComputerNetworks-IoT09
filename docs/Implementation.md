## Implementation Details in COAP

**There were two options to implement the Eifel Retransmission Timer**:<br/>
**1. Implementing in COAP**<br/>
**2. Implementing in CoCoA**<br/>

<br/>
CoCoA already has RTO estimation functionality which has a strong estimator and a weak estimator<br/>
This adds a layer of complexity to the implementation, thus we chose to go ahead with COAP.

#### Implementation in COAP
COAP uses a constant value for RTO and it doesn't have any mechanism to predict the RTO.<br/>
It doesn't have any mechanism for RTT prediction which is needed for the Eifel RTO prediction.<br/>
We have to start with RTT estimation and then move on to calculating the RTO values based on these estimations.<br/>

##### 1. Adding RTT Estimation
RTT estimation was added in two steps, the first task was to store the time at which the packet is being sent<br/>
and then use it in the second step to calculate the RTT. The RTT is calculated at the time of receiving the ACK,<br/>
the time at which ACK is received is subtracted from the time stored at the beginning.<br/>
The following images show the implementation of RTT Estimation.<br/><br/>
Declaring Timestamp<br/>
![Timestamp](https://imgur.com/1ATwI5X.png)
 <br/>
Calculating RTT<br/>
![RTT](https://imgur.com/XYRq7Fa.png)

##### 2. Calculating RTO from RTT
RTO calculation utilises two functions, **check_rtt** which is used to retrieve RTO estimation for packets and **calculateRTO** which is used to do the actual calculation. The images below contain the code snippets for the two functions.<br/><br/>
**check_rtt**<br/>
![check_rtt](https://imgur.com/28WEiRk.png)<br/>
**calculateRTO**<br/><br/>
![RTO1](https://imgur.com/9riWcuO.png) ![RTO2](https://imgur.com/OAGo393.png)![RTO3](https://imgur.com/AJWDyS8.png) ![RTO4](https://imgur.com/sLUP0Qm.png)

