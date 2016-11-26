# Compile CAB Program at CAB_SDN folder
mkdir build
cd build
cmake ../
cd ../bin

# Run CABDaemon
./CABDaemon rulefile 

# Run ryu-controller
cd ../controller
ryu-manager cab_switch_cab_v6 ../config/ryu_config.ini

# Install client echoer module @ client machine
### this ensures that every packet sent to the client will be pump back.
cd ../trace_gen
make
lsmod | grep cecho
sudo rmmod cecho
sudo insmod ./client_echo.ko
lsmod | grep cecho
