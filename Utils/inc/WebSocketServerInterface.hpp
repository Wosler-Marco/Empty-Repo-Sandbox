#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <queue>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace wosler {
namespace utilities {

class WebSocketServerSession : public std::enable_shared_from_this<WebSocketServerSession> {
    private:
        // WebSocket private variable
        websocket::stream<beast::tcp_stream> ws;

        // Send and receive queue private variables
        std::queue<std::string> readQ;
		std::queue<std::string> writeQ;
        std::queue<bool> isBinary;

        // Message buffer private variable
		beast::flat_buffer buffer;

        bool connected;
    
        /**
		 * @brief Method to print out errors with the WebSocket
		 * 
		 * @param ec An error code object containing both a code and a text description of the specific error
		 * @param what The operation that threw an error
		 */
        void fail(beast::error_code ec, char const* what) {
            std::cerr << what << ": " << ec.message() << "\n";
        }

        /**
         * @brief Method to asynchronously perform operations to set options for the WebSocket and accept the client into the session
         * 
         */
        void on_run() {
            // Set suggested timeout settings for the websocket
            ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

            // Set a decorator to change the Server of the handshake
            ws.set_option(websocket::stream_base::decorator([](websocket::response_type& res){res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async");}));
            
            // Accept the websocket handshake
            ws.async_accept(beast::bind_front_handler(&WebSocketServerSession::on_accept, shared_from_this()));
        }

        /**
         * @brief Method to asynchronously perform operations to begin reading data off the WebSocket
         * 
         * @param ec An error code object containing both a code and a text description of the specific error
         */
        void on_accept(beast::error_code ec) {
            if (ec) {
                return fail(ec, "accept");
            }

            connected = true;

            // Call async_read to keep the io_context busy (the object is destroyed when the io_context runs out of work)
            ws.async_read(buffer, beast::bind_front_handler(&WebSocketServerSession::on_read,shared_from_this()));
        }

        /**
		 * @brief Method to asynchronously perform operations to save the received message on a queue
		 * 
		 * @param ec An error code object containing both a code and a text description of the specific error
		 * @param bytes_transferred The number of bytes received on the WebSocket
		 */
        void on_read(beast::error_code ec, std::size_t bytes_transferred) {
			boost::ignore_unused(bytes_transferred);

            // Check for specific error codes that state the Websocket was closed and don't report it
            if (ec == net::error::operation_aborted || ec == websocket::error::closed) {
				return;
			}

			if (ec) {
				return fail(ec, "read");
			}

            // Put the incoming message on the read queue
			readQ.push(beast::buffers_to_string(buffer.data()));
			buffer.consume(buffer.size());

            // Call async_read to keep the io_context busy
			ws.async_read(buffer, beast::bind_front_handler(&WebSocketServerSession::on_read, shared_from_this()));
		}

        /**
		 * @brief Method to asynchronously perform operations to handle cleanup of a written message
		 * 
		 * @param ec An error code object containing both a code and a text description of the specific error
		 * @param bytes_transferred The number of bytes sent on the WebSocket
		 */
		void on_write(beast::error_code ec, std::size_t bytes_transferred) {
			boost::ignore_unused(bytes_transferred);

            // Check for specific error codes that state the Websocket was closed and don't report it
            if (ec == net::error::operation_aborted || ec == websocket::error::closed) {
				return;
			}

			if (ec) {
				return fail(ec, "write");
			}

            // Message has been written, so pop it off the queue
            isBinary.pop();
			writeQ.pop();

            // Check to see if there are any more messages that can be sent
			if (writeQ.empty()) {
				return;
			} else {
                ws.binary(isBinary.front());

                // Call to send the next message whenever the Websocket is available
				ws.async_write(net::buffer(writeQ.front()), beast::bind_front_handler(&WebSocketServerSession::on_write, shared_from_this()));
			}
		}

        /**
		 * @brief Method to asynchronously perform operations to properly close the WebSocket
		 * 
		 * @param ec 
		 */
        void on_close(beast::error_code ec) {
            // Check for specific error codes that state the Websocket was closed and don't report it
            if (ec == net::error::operation_aborted || ec == websocket::error::closed) {
				return;
			}

            if (ec) {
                return fail(ec, "close");
            }
        }
    
    public:
        /**
         * @brief Construct a new Web Socket Server Session object
         * 
         * @param socket The TCP socket to make the WebSocket connection over
         */
        explicit WebSocketServerSession(tcp::socket&& socket) : ws(std::move(socket)) {
            connected = false;
        }

        /**
         * @brief Method to start asynchronous operations and enable the WebSocket for use
         * 
         */
        void run() {
            // Make an asynchronous operation call to begin setting up the Websocket
            net::dispatch(ws.get_executor(), beast::bind_front_handler(&WebSocketServerSession::on_run, shared_from_this()));
        }

        /**
		 * @brief Method to check to see if the WebSocket is connected to the server
		 * 
		 * @return true The WebSocket is connected
		 * @return false The WebSocket is not connected
		 */
        bool isConnected() {
            return connected;
        }

        /**
		 * @brief Method to check to see if there are any received messages stored on the read queue
		 * 
		 * @return true No messages on the queue
		 * @return false Message(s) are on the queue
		 */
        bool readQEmpty() {
            return readQ.empty();
        }

        /**
		 * @brief Method to read a message from the read queue
		 * 
		 * @return std::string The message received in UTF-8 format
		 */
        std::string readFromQueue() {
            // See if there are any messages on the read queue
			if (readQ.empty()) {
				return "";
			} else {
                // Read a message off the front of the queue and then pop it off
				std::string message = readQ.front();
				readQ.pop();
				return message;
			}
		}

        /**
		 * @brief Method to write a message in UTF-8 format on the send queue to be sent over the WebSocket
		 * 
		 * @param message A valid UTF-8 string to be sent over the WebSocket
		 */
        void writeToQueue(std::string message) {
            isBinary.push(false);
			writeQ.push(message);
			
            // Check to see if there already is a call to async_write (can't have multiple calls to the same asynchronous operation queued up)
			if (writeQ.size() > 1) {
				return;
			} else {
                ws.binary(isBinary.front());

                // Call async_write if there are no existing calls to it so the message gets sent
				ws.async_write(net::buffer(writeQ.front()), beast::bind_front_handler(&WebSocketServerSession::on_write, shared_from_this()));
			}
		}

        /**
		 * @brief Method to write a message in any binary format on the send queue to be sent over the WebSocket
		 * 
		 * @param message A valid binary string to be sent over the WebSocket
		 */
        void writeBinaryToQueue(std::string message) {
            isBinary.push(true);
			writeQ.push(message);
			
            // Check to see if there already is a call to async_write (can't have multiple calls to the same asynchronous operation queued up)
			if (writeQ.size() > 1) {
				return;
			} else {
                ws.binary(isBinary.front());
                
                // Call async_write if there are no existing calls to it so the message gets sent
				ws.async_write(net::buffer(writeQ.front()), beast::bind_front_handler(&WebSocketServerSession::on_write, shared_from_this()));
			}
		}

        /**
		 * @brief Method to close the WebSocket session
		 * 
		 */
        void close() {
            // Close the WebSocket connection
            ws.async_close(websocket::close_code::normal,beast::bind_front_handler(&WebSocketServerSession::on_close, shared_from_this()));
        }
};

class WebSocketServer : public std::enable_shared_from_this<WebSocketServer> {
    private:
        // WebSocket private variables
        net::io_context& ioc;
        tcp::acceptor acceptor;
        std::shared_ptr<WebSocketServerSession> session;

        bool connected;

        /**
		 * @brief Method to print out errors with the WebSocket
		 * 
		 * @param ec An error code object containing both a code and a text description of the specific error
		 * @param what The operation that threw an error
		 */
        void fail(beast::error_code ec, char const* what) {
            std::cerr << what << ": " << ec.message() << "\n";
        }

        /**
         * @brief Method to asynchronously perform operations to create and run a session after accepting a client
         * 
         * @param ec An error code object containing both a code and a text description of the specific error
         * @param socket The TCP socket for the WebSocket connection
         */
        void on_accept(beast::error_code ec, tcp::socket socket) {
            if (ec) {
                fail(ec, "accept");
            } else {
                // Create the session and run it
                session = std::make_shared<WebSocketServerSession>(std::move(socket));
                session->run();
                connected = true;
            }
        }

    public:
        /**
         * @brief Construct a new Web Socket Server object
         * 
         * @param iocontext The IO context that will be responsible for handling asynchronous method calls
         * @param endpoint The endpoint created for a given IP Address and Port Number
         */
        WebSocketServer(net::io_context& iocontext, tcp::endpoint endpoint) : ioc(iocontext), acceptor(iocontext) {
            connected = false;
            beast::error_code ec;

            // Open the acceptor
            acceptor.open(endpoint.protocol(), ec);
            if (ec) {
                fail(ec, "open");
                return;
            }

            // Allow address reuse
            acceptor.set_option(net::socket_base::reuse_address(true), ec);
            if (ec) {
                fail(ec, "set_option");
                return;
            }

            // Bind to the server address
            acceptor.bind(endpoint, ec);
            if (ec) {
                fail(ec, "bind");
                return;
            }

            // Start listening for connections
            acceptor.listen(net::socket_base::max_listen_connections, ec);
            if (ec) {
                fail(ec, "listen");
                return;
            }
        }

        /**
         * @brief Method to start asynchronous operations and launch the session
         * 
         */
        void run() {
            // The new connection gets its own strand
            acceptor.async_accept(net::make_strand(ioc), beast::bind_front_handler(&WebSocketServer::on_accept, shared_from_this()));
        }

        /**
		 * @brief Method to check to see if the WebSocket is connected to the server
		 * 
		 * @return true The WebSocket is connected
		 * @return false The WebSocket is not connected
		 */
        bool isConnected() {
            if (!connected) {
                return false;
            } else {
                return session->isConnected();
            }
        }

        /**
		 * @brief Method to check to see if there are any received messages stored on the read queue
		 * 
		 * @return true No messages on the queue
		 * @return false Message(s) are on the queue
		 */
        bool readQEmpty() {
            return session->readQEmpty();
        }

        /**
		 * @brief Method to read a message from the read queue
		 * 
		 * @return std::string The message received in UTF-8 format
		 */
        std::string read() {
            return session->readFromQueue();
        }

        /**
		 * @brief Method to write a message in UTF-8 format on the send queue to be sent over the WebSocket
		 * 
		 * @param message A valid UTF-8 string to be sent over the WebSocket
		 */
        void write(std::string message) {
            session->writeToQueue(message);
        }

        /**
		 * @brief Method to write a message in any binary format on the send queue to be sent over the WebSocket
		 * 
		 * @param message A valid binary string to be sent over the WebSocket
		 */
        void writeBinary(std::string message) {
            session->writeBinaryToQueue(message);
        }

        /**
		 * @brief Method to close the WebSocket session
		 * 
		 */
        void close() {
            session->close();
        }
};

} // End namespace utilities
} // End namespace wosler
