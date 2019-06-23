
sleep 1s &&
./manager_send 1 send 2 "1 to 2 first"  && 
./manager_send 1 send 7 "1 to 7 first"  && 
sleep 5s &&
./manager_send 7 send 8 "7 to 8 first"  && 
./manager_send 4 send 0 "4 to 0 first"  && 
./manager_send 7 send 6 "7 to 6 first"  && 
./manager_send 0 send 3 "0 to 3 first"  &&
sleep 5s &&
./manager_send 1 cost 2 1  &&
./manager_send 2 cost 1 1  &&
sleep 5s &&
./manager_send 1 send 2 "1 to 2 second"  && 
./manager_send 4 send 0 "4 to 0 second"  && 
./manager_send 0 send 3 "0 to 3 second"  &&
./manager_send 7 send 6 "7 to 6 second"  && 
./manager_send 6 send 7 "6 to 7 second"  &&
./manager_send 1 send 2 "1 to 2 second"  