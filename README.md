# FortiRDP

FortiRDP is a FortiGate SSL VPN port forwarder client for Windows. FortiRDP is not supported by, or associated in any way by Fortinet.  This software was developed to interoperate with their equipment at a time Java applet support was deprecated in Chrome or Firefox but still in use by our installed firewalls. FortiRDP is working with the latest version of FortiOS (tested with versions 6.4.x and 7.0.x). 

FortiRDP acts as a SSL VPN port forwarder. Once connected to the firewall, FortiRDP starts listening on a random local port on the user's computer,
launches the client application (mstsc.exe) with the required parameter to connect to the user's computer address on the chosen random port. When it receives data from the client application, FortiRDP encrypts and sends the data to the FortiGate unit, which decrypts the data and then forwards the data to the Windows Terminal Server.

FortiRDP can be customized through command line parameters to launch other clients such as VNC, Telnet, ... or other clients that support TCP communication.

FortiRDP is provided as a single Windows 64-bit executable (fortirdp.exe) and a certificate storage (fortirdp.crt). It is a portable application that does not require any installation. You only need to copy fortirdp.exe and fortirdp.crt in the same folder and run the application.  FortiRDP uses ![mbed TLS](https://github.com/Mbed-TLS/mbedtls) to encrypt the communication with the firewall and ![lwIP](https://git.savannah.nongnu.org/cgit/lwip.git) to manage the IP communication within the SSL tunnel.

