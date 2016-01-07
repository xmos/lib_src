/*
 * time.xc
 *
 *  Created on: Jul 9, 2015
 *      Author: Ed
 */

timer t;
unsigned start_time, stop_time;

void start_timer(void){
    t :> start_time;
}

unsigned stop_timer(void){
    t :> stop_time;
    return (stop_time-start_time);
}
