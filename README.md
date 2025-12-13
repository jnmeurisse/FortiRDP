# FortiRDP
FortiRDP is a local FortiGate SSL VPN port forwarder client for Windows Remote Desktop connection.  
This software was developed to interoperate with the [SSLVPN Web portforward mode](https://help.fortinet.com/cli/fos50hlp/54/index.htm#FortiOS/fortiOS-cli-ref-54/config/vpn/ssl_web_user-group-bookmark.htm) at a time the Java applet support
was deprecated in Chrome or Firefox but still in use by our installed firewalls. FortiRDP is working with recent version
of FortiOS (tested on FortiGate-60E and FortiGate-60F with FortiOS versions 6.4.x, 7.0.x, 7.1.x).

The port forwarding is a mechanism to send arbitrary TCP traffic over an encrypted SSL tunnel between FortiRDP and a
FortiGate firewall.  Its main usage is to forward RDP traffic. It was developed to simplify connection to a Windows
terminal server. First, FortiRDP establishes an encrypted SSL tunnel with the Fortigate Firewall.  Once connected,
FortiRDP starts listening on a random local port on localhost (127.0.0.1) and launches the client application
(by default mstsc.exe) with the required parameters to connect to 127.0.0.1 on the chosen random port.  Data is
encrypted and sent to the FortiGate unit through the tunnel, which then forwards the traffic to the application server.

FortiRDP is provided as a single Windows 64-bit executable (fortirdp.exe) and a certificate storage (fortirdp.crt). It
is a portable application that does not require any installation. You only need to copy fortirdp.exe and fortirdp.crt in
the same folder and run the application.  FortiRDP uses [mbed TLS](https://github.com/Mbed-TLS/mbedtls) to establish the secure tunnel with the firewall
and [lwIP](https://git.savannah.nongnu.org/cgit/lwip.git/) to handle the IP communication within the tunnel.

FortiRDP can be customized through command line parameters to launch other client applications such as VNC, telnet, ... 
or other clients that support TCP communication. 

## Interactive usage

## Command line usage
```
fortirdp [-v [-t]] [-A auth] [-u username] [-c cacert_file] [-x app] [-f] [-a] [-s] [-p port]
[-r rdp_file] [-m] [-l] [-C] [-M] [-n] [-w width] [-h height]
firewall-ip[:port1] remote-ip[:port2]
```

## Command Line Options
### Debugging

| Option | Description                                                    |
|--------|----------------------------------------------------------------|
| `-v`   | Enables verbose mode.                                          |
| `-t`   | Traces TLS conversations (requires `-v`; very high verbosity). |

### Authentication & Certificates

| Option             | Description                                                             |
|--------------------|-------------------------------------------------------------------------|
| `-A auth`          | Authentication mode: `basic`, `cert`, `saml`.                           |
| `-u username`      | Username for basic authentication.|
| `-U usercert_file` | Path to User Certificate file.                                          |                                                                  
| `-c cacert_file`   | Path to Certificate Authority file.                                     |
*Notes:*
- `-u username` is used only with `basic` authentication. It supplies the username for firewall login.
- `-U usercert_file` is used only with `cert` authentication.  
- `-c cacert_file` sets a custom CA certificate file to validate the firewall’s certificate.  The application tries 
also to validate the certificate using the Windows certificate store.

### Application Launching

| Option   | Description                            |
|----------|----------------------------------------|
| `-x app` | Application to launch instead of mstsc.|

*Notes:*
- syntax: `path{;parameter;parameter...}`
- supported variables inside a parameter:
  - `${host}` — resolved to local host.
  - `${port}` — assigned (or static) local port.
- specifying an empty string (`""`) disables automatic application launching.

### RDP Display & Session Options
The following options are available only if the `-x app` argument is omitted.
When no custom application is specified, fortirdp launches the default RDP client using:
`c:\windows\system32\mstsc.exe;/v:${host}:${port}`.</br>
Additional parameters can then be supplied:

| Option        | Description                                                                                  |
|---------------|----------------------------------------------------------------------------------------------|
| `-f`          | Start RDP in full-screen mode.</br>Adds `/f` argument to mstsc.                              |
| `-a`          | Administration mode.</br>Adds `/admin` argument to to mstsc.                                 |
| `-s`          | Span mode.</br>Adds `/span` argument to to mstsc.                                            |
| `-m`          | Multi-monitor mode.</br>Adds `/multimon` argument to to mstsc.                               |
| `-w width`    | Specify the width of the Remote Desktop Window.</br>Adds `/w:<width>` argument to to mstc.   |
| `-h height`   | Specify the height of the Remote Desktop Window.</br>Adds `/h:<height>` argument to to mstc. |
| `-r rdp_file` | Pass an `.rdp` file to mstsc.                                                                |
| `-C`          | Clear the last stored RDP username.                                                          |

**Notes:** Span mode and multi-monitor mode are mutually exclusive options. Additionally, Remote Desktop window size
settings cannot be combined with full-screen display.

### Tunnel Behavior

| Option    | Description                                                                                                                              |
|-----------|------------------------------------------------------------------------------------------------------------------------------------------|
| `-p port` | Use a static local port instead of a dynamic one.</br>The `${port}` variable in the `-x app` command is replaced with this static value. | 
| `-M`      | Enables the tunnel to accept multiple incoming connections.                                                                              |
| `-n`      | Disable Nagle’s algorithm.                                                                                                               |

**Notes:** when the `-M` option is enabled, fortirdp keeps the local TCP listener open and allows multiple incoming
client connections. Simultaneous connections are supported, subject to firewall policy and remote host limitations.
This mode is especially suited for web traffic forwarding.

### Positional Arguments

`firewall-ip[:port1]`
- Hostname or IP address of the firewall to connect to.
- Default port: 10443, use `:port1` to specify an alternate port.

`remote-ip[:port2]`
- Hostname or IP address of the remote computer.
- Default RDP port: 3389, use `:port2` to specify an alternate port

---

## Examples
All examples below use 10.34.1.37 as the firewall IP and 192.168.1.10 as the protected host.

### Basic RDP tunnel
fortirdp 10.34.1.37 192.168.1.10

### Verbose mode with certificate authentication
fortirdp -v -A cert -c myca.pem 10.34.1.37 192.168.1.10

### Full-screen RDP
fortirdp -f 10.34.1.37 192.168.1.10

### Launch a custom application
fortirdp -x "C:\Tools\MyViewer.exe;\${host}:\${port}" 10.34.1.37 192.168.1.10

### Launch a browser application
fortirdp -x "C:\Tools\MyViewer.exe;\${host}:\${port}" -M 10.34.1.37 192.168.1.10

### Manual application launch & listen to a fixed port
fortirdp -x "" -p 8443 10.34.1.37 192.168.1.10</br>
C:\Tools\MyViewer.exe localhost:8443

### Launch a browser application
fortirdp -M -x "c:\Program Files\Google\Chrome\Application\chrome.exe;https://\${host}:\${port}" 10.34.1.37 192.168.1.10:443
