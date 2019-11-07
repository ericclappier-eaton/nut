Integration tests with MQTT genepi card: 
---------------------------------------- 
 
The goal of the tests is to simulate alarms and status of an UPS and check 
result in final NUT objects throw the NUT MQTT driver.    
 
For tests, the following items are needed: 
- 1 Windows OS platform for running the daemon simulator (HidSimD.exe) 
- 1 Converter USB/Serial 
- 1 genepi card board  
- 1 genepi card 
- 1 IPM2 OVA with devel version 
 
How to test:  
- Plug the genepi card in its board with a power cable. 
- Plug in the USB/serial cable between the card board and the Windows machine. 
- On The Windows machine, install the driver VCP driver for the USB/serial 
  converter (see FTDI devices on https://www.ftdichip.com/Drivers/VCP.htm). 
- Copy Windows files (win directory) in a directory on the Windows machine. 
- On this directory, open the HidSim.ini file and change the com port with the 
  port detected by the USB/serial converter. 
- Execute the daemon simulator (HidSimD.exe). 
- Execute the client simulator on a console (HidSim.exe) with command:
  HidSim.exe open 
- Check in daemon simulator if the communication is established with the card. 
- Open a console on the IPM2 OVA. 
- Copy the test files into a directory. 
- On this directory, open the HidSim.ini file and set the IP address of the 
  Windows machine in the "address" item. 
- Open the web pages of the genepi card and enable SNMP v1 access. Set "public" 
  community name for read access and "private" for read/write. 
  Note: You can also change default values directly in the test_genepi.sh file. 
- Execute the test_genepi.sh bash to execute tests and check results at the end. 
  ./test_genepi.sh -i <Genepi ip address> -a <Genepi asset name>  

