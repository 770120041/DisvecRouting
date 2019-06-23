
sleep 1s &&
./manager_send 1 send 2 "hello"  && 
sleep 3s &&
sudo iptables -D OUTPUT -s 10.1.1.6 -d 10.1.1.7 -j ACCEPT &&
sudo iptables -D OUTPUT -s 10.1.1.7 -d 10.1.1.6 -j ACCEPT &&
sudo iptables -D OUTPUT -s 10.1.1.1 -d 10.1.1.4 -j ACCEPT &&
sudo iptables -D OUTPUT -s 10.1.1.4 -d 10.1.1.1 -j ACCEPT &&
sudo iptables -D OUTPUT -s 10.1.1.2 -d 10.1.1.3 -j ACCEPT &&
sudo iptables -D OUTPUT -s 10.1.1.3 -d 10.1.1.2 -j ACCEPT &&
sleep 3s &&
./manager_send 7 send 8 "7 to 8 first"  && 
./manager_send 4 send 0 "4 to 0 first"  && 
./manager_send 7 send 6 "7 to 6 first"  && 
./manager_send 0 send 3 "0 to 3 first"  &&
sleep 3s &&
./manager_send 1 cost 2 1  &&
./manager_send 2 cost 1 1  &&
sudo iptables -I OUTPUT -s 10.1.1.6 -d 10.1.1.7 -j ACCEPT &&
sudo iptables -I OUTPUT -s 10.1.1.7 -d 10.1.1.6 -j ACCEPT &&
sudo iptables -I OUTPUT -s 10.1.1.1 -d 10.1.1.4 -j ACCEPT &&
sudo iptables -I OUTPUT -s 10.1.1.4 -d 10.1.1.1 -j ACCEPT &&
sudo iptables -I OUTPUT -s 10.1.1.2 -d 10.1.1.3 -j ACCEPT &&
sudo iptables -I OUTPUT -s 10.1.1.3 -d 10.1.1.2 -j ACCEPT &&
sleep 3s &&
./manager_send 4 send 0 "4 to 0 second"  && 
./manager_send 0 send 3 "0 to 3 second"  &&
./manager_send 7 send 6 "7 to 6 second"  && 
./manager_send 6 send 7 "6 to 7 second"  &&
./manager_send 1 send 2 "1 to 2 second"  