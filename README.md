# P2P_UDP
Local P2P app that shares COVID-19 data. Peers discover neighbors thanks to the discovery server.
Everything is in localhost, it doesn't work in multihost. It could be implemented with easy steps. 
Peers message each other with UDP protocol.

Compile using /script.sh

- peer : Each peer has a unique door. A door is necessary to connect to the server and to the neighbors. The door is also used to understand which register a peer has to work on. You can add data with command "add type quantity" where type = [N (= new case), T (= tampons)]. The data will be added each day at 18:00 or when the server calls "stop" or when the peer calls "exit". 
- ds: A door is necessary to boot and to let the peers discover their neighbors. 
