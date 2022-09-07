# nsconn
A connector for the NeuroSky Mindwave Mobile 2 EEG headset.

NOTE: Do not use this as it's old, horrible and clunky - please check out the updated version at https://github.com/SWelshUWS/nsconn_v2.0 \
Nsconn is designed to emulate the functionality of the official ThinkGear connector, but with the added UNIX friendly bonus of printing to stdout (can be used with grep, output to a file etc). \
If networking is specified, clients can connect to nsconn via a client socket on 127.0.0.1:9291 to begin recieving JSON headset data.\

For further usage information please check the man page.

# Installation:

git clone https://github.com/SWelshUWS/nsconn \
cd nsconn \
sudo apt update \
sudo chmod +x install \
sudo ./install 

Before using nsconn, please change the configuration file at /etc/nsconn.conf DEFAULT_MAC to use your own headset. Alternatively, a MAC can be specified as the last argument when starting up nsconn.


