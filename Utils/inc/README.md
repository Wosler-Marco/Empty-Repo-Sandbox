# Utils Include Files
## Overview
This folder contains the header files for all of Wosler's utilities. It consists of commonly used structures and formats created by Wosler and used frequently throughout the code.


## Files
### AsyncRunnable.hpp

### CalibrationConfig.hpp

### CommonUtilities.hpp

### Context.hpp

### DataLogger.hpp
A `DataLogger` object to save common data types as well as a `Timer` object to save times to a file.
#### Diagrams
**Timer**

![image](https://github.com/Wosler-Marco/SonoStation/assets/109979093/d224adab-7d36-464f-92ec-2dec1778a70a)

**DataLogger**

![image](https://github.com/Wosler-Marco/SonoStation/assets/109979093/9d8d7d0a-53aa-419f-9c49-a329a5e8f648)

#### Methods
- **(Timer) startTimer:** Starts the timer
- **(Timer) endTimer:** Ends the timer if it has been started, determines the time elasped, and saves the time to a file
- **(Timer) endTimerPrintoutMilliseconds:** Ends the timer if it has been started, determines the time elasped, saves the time to a file, and prints the time in milliseconds to the console
- **(Timer) endTimerPrintoutSeconds:** Ends the timer if it has been started, determines the time elasped, saves the time to a file, and prints the time in seconds to the condsole
- **logData:** Logs any of the following data types to a file:
  - Numerical types (`int`, `float`, `double`, etc.)
  - `String`
  - `char`
  - `bool`
  - Arrays of any of the above types
  - Vectors of any of the above types
  - `Eigen::Matrix4f`
- **logDataPrintout:** Logs any of the data types from `logData` to a file and prints the value of the variable to the console

### Event.hpp

### EventChannel.hpp

### GenericModule.hpp

### IPCConfig.hpp

### JSONMessage.hpp
A collection of C++ objects that are turned into the JSON messages used to communicate data through SonoLink. The conversions between C++ objects and JSON strings are handled by the [Nlohmann JSON Library](https://json.nlohmann.me/).
#### Diagrams
**message**

![image](https://github.com/Wosler-Marco/SonoStation/assets/109979093/468d00ba-7ad0-4026-ba7f-3186c7409da9)

**authenticationMessage**

![image](https://github.com/Wosler-Marco/SonoStation/assets/109979093/9b40692c-2375-444d-af38-f1361119e80c)

**controlMessage**

![image](https://github.com/Wosler-Marco/SonoStation/assets/109979093/3c81a9db-a5a1-43a0-8995-85c78e94a92f)

**toolMessage**

![image](https://github.com/Wosler-Marco/SonoStation/assets/109979093/ff86a5c0-da2b-41a9-a35c-acc0330e7ef4)

**gelDispenseMessage**

![image](https://github.com/Wosler-Marco/SonoStation/assets/109979093/6d9d125c-8650-4a48-817f-def80d3bdd2f)

**homingMessage**

![image](https://github.com/Wosler-Marco/SonoStation/assets/109979093/852a964c-1ec5-4d95-9726-edab3b32d9d9)

#### Methods
- **(All) printMessage:** Prints out the data in the message to the console
- **(All) getMessageID:** Returns the unique message ID of the message
- **(All) getCommandID:** Returns the command ID in the message
- **(All) getUTCTime:** Returns the UTC timestamp in the message
- **(All) getErrorCode:** Returns the error code in the message
- **(All) from_json:** Converts a JSON string into a specific message type
- **(All) to_json:** Converts a specific message type into a JSON string
- **(authenticationMessage) getClinicID:** Returns the clinic ID in the message
- **(authenticationMessage) getRoboticArm:** Returns the robotic arm ID in the message
- **(authenticationMessage) getRoomID:** Returns the room ID in the message
- **(controlMessage/homingMessage) getPacket:** Returns the T-matrix packet in the message
- **(toolMessage/homingMessage) getToolID:** Returns the tool ID in the message
- **(gelDispenseMessage) getAmount:** Returns the amount of gel to dispense in the message

### ModuleEvents.hpp

### SerializationInterface.hpp

### SocketInterface.hpp

### StateChannel.hpp

### TMatrixPacket.hpp

### WebSocketClientInterface.hpp
A WebSocket client to connect to a server over a network and send messages using [Transmission Control Protocol (TCP)](https://en.wikipedia.org/wiki/Transmission_Control_Protocol). The WebSocket code uses the [Boost Beast Library](https://www.boost.org/doc/libs/1_83_0/libs/beast/doc/html/index.html) and uses asynchronous operations through Boost ASIO. As the WebSocket uses asynchronous operations, the I/O context must always have work to do (read or write) and only one of each type of operation can be queued at a time (e.g. cannot queue a read operation if one is already queued).
#### Diagram
![image](https://github.com/Wosler-Marco/SonoStation/assets/109979093/47a5dd4c-787c-4a9f-9ebb-66011761fe82)
#### Methods
- **fail:** A handler to print out errors that occur with the WebSocket
- **on_resolve:** A handler to try to connect to the IP address and port number found when resolving the URL
- **on_connect:** A handler to set the WebSocket options and performing the WebSocket handshake once the connection to the server is made
- **on_handshake:** A handler to queue a read operation to keep the I/O context busy
- **on_read:** A handler to save the message received to a queue and queue up another read operation
- **on_write:** A handler to pop the message sent off the send queue and queue up another write operation if there are other messages on the send queue
- **on_close:** A handler to ensure the error code when closing the WebSocket is normal
- **run:** Begins the operations necessary to connect to the server and upgrade the connection to a WebSocket
- **isConnected:** Returns true if the WebSocket is connected to the server and false otherwise
- **readQEmpty:** Returns true if the read queue is empty and false otherwise
- **read:** Returns the first message on the read queue
- **write:** Puts a message on the write queue to send to the server
- **writeBinary:** Puts a binary message on the write queue to send to the server
- **close:** Closes the WebSocket connection to the server

### WebSocketServerInterface.hpp
A WebSocket server to accept incoming connections over a network and send messages using TCP. The WebSocket code uses the Boost Beast Library and uses uses asynchronous operations through Boost ASIO. As the WebSocket uses asynchronous operations, the I/O context must always have work to do (read or write) and only one of each type of operation can be queued at a time (e.g. cannot queue a read operation if one is already queued). The WebSocket server is separated into two components:
  - **WebSocketServer:** A server that listens for an incoming connection and upgrades the connection to a WebSocket
  - **WebSocketServerSesssion:** A session for the WebSocket to send and receive messages

Although the structure of the `WebSocketServer` differs from the `WebSocketClient`, the API for the user is the same.
#### Diagrams
**WebSocketServer**

![image](https://github.com/Wosler-Marco/SonoStation/assets/109979093/c7906a61-140e-452a-9413-941a28514702)

**WebSocketServerSesssion**

![image](https://github.com/Wosler-Marco/SonoStation/assets/109979093/9d6bc2c3-3698-438c-bd33-a2c064a0bb28)

#### Methods
- **(All) fail:** A handler to print out errors that occur with the WebSocket
- **(All) isConnected:** Returns true if the WebSocket is connected to the client and false otherwise
- **(All) readQEmpty:** Returns true if the read queue is empty and false otherwise
- **(WebSocketServer) on_accept:** A handler to give the ownership of the `WebSocketServerSession` to the `WebSocketServer`
- **(WebSocketServer) run:** Begins the operations necessary to accept an incoming connection
- **(WebSocketServer) read:** A wrapper over the `readFromQueue` method in the `WebSocketServerSession` to read the first message on the read queue
- **(WebSocketServer) write:** A wrapper over the `writeToQueue` method in the `WebSocketServerSession` to put a message on the write queue to send to the client
- **(WebSocketServer) writeBinary:** A wrapper over the `writeBinaryToQueue` method in the `WebSocketServerSession` to put a binary message on the write queue to send to the client
- **(WebSocketServer) close:** A wrapper over the `close` method in the `WebSocketServerSession` to close the WebSocket connection to the client
- **(WebSocketServerSession) on_run:** A handler to set the WebSocket options and accepting the WebSocket connection
- **(WebSocketServerSession) on_accept:** A handler to queue a read operation to keep the I/O context busy
- **(WebSocketServerSession) on_read:** A handler to save the message received to a queue and queue up another read operation
- **(WebSocketServerSession) on_write:** A handler to pop the message sent off the send queue and queue up another write operation if there are other messages on the send queue
- **(WebSocketServerSession) on_close:** A handler to ensure the error code when closing the WebSocket is normal
- **(WebSocketServerSession) run:** Begins asynchronous operations for the `WebSocketServerSession`
- **(WebSocketServerSession) readFromQueue:** Returns the first message on the read queue
- **(WebSocketServerSession) writeToQueue:** Puts a message on the write queue to send to the client
- **(WebSocketServerSession) writeBinaryToQueue:** Puts a binary message on the write queue to send to the client
- **(WebSocketServerSession) close:** Closes the WebSocket connection to the client
