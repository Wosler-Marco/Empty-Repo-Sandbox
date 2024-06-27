# Utils Test Scripts
## Overview
This folder contains the test scripts for Wosler's utilities.


## Files
### ConcreteClassModuleTest.cpp

### StateChannelTest.cpp

### TMatrixPacketTest.cpp

### UDPInducedLag.cpp

### UDPSocketModuleTest.cpp

### WebSocketTest.cpp
This test script creates a WebSocket client or server and allows messages to be sent and received once the WebSocket connection is made. The server must be created before the client tries to connect to it.
#### Usage
**Server**
1. Open the `Utils` folder in a terminal and run `.\build\bin\WebSocketTest.exe ip_address port SERVER` (e.g. `.\build\bin\WebSocketTest.exe 127.0.0.1 5000 SERVER`), putting in the IP address and port number to host on
2. Wait until a client connects to the server
3. Enter one of the following combinations of inputs depending on the function you want to perform:
   - **Read:** `r`
   - **Write:** `w, string` (e.g. `w, hi`)
   - **Quit:** `q`

**Client**
1. Open the `Utils` folder in a terminal and run `.\build\bin\WebSocketTest.exe ip_address port CLIENT` (e.g. `.\build\bin\WebSocketTest.exe 127.0.0.1 5000 CLIENT`), putting in the IP address and port number of the server to connect to
2. Enter one of the following combinations of inputs depending on the function you want to perform:
   - **Read:** `r`
   - **Write:** `w, string` (e.g. `w, hi`)
   - **Quit:** `q`
