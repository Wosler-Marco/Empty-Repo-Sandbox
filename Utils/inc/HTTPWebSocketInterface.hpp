#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
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
namespace ssl = boost::asio::ssl;          // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace wosler {
namespace utilities {

class HTTPWebSocket : public std::enable_shared_from_this<HTTPWebSocket> {
	private:
		// WebSocket private variables
		tcp::resolver resolver;
		websocket::stream<beast::ssl_stream<tcp::socket>> ws;
		std::string token;

		// Send and receive queue private variables
		std::queue<std::string> readQ;
		std::queue<std::string> writeQ;
		std::queue<bool> isBinary;

		// Message buffer private variable
		beast::flat_buffer buffer;

		std::string host;
		bool connected;

		/**
		 * @brief Method to print out errors with the WebSocket
		 * 
		 * @param ec An error code object containing both a code and a text description of the specific error
		 * @param what The operation that threw an error
		 */
		void fail(beast::error_code ec, char const* what) {
			std::cout << what << ": " << ec.message() << "\n";
		}

		/**
		 * @brief Method to asynchronously perform operations to connect the WebSocket after the host has been found
		 * 
		 * @param ec An error code object containing both a code and a text description of the specific error
		 * @param results A list of endpoints found for the IP Address and Port
		 */
		void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
			if (ec) {
				return fail(ec, "resolve");
			}

			// Make the connection on the IP address we get from a lookup
			net::async_connect(get_lowest_layer(ws), results, beast::bind_front_handler(&HTTPWebSocket::on_connect, shared_from_this()));
		}

		/**
		 * @brief Method to asynchronously perform the SSL handshake after the connection is made
		 * 
		 * @param ec An error code object containing both a code and a text description of the specific error
		 * @param ep Unused parameter specifying the endpoint the WebSocket will connect to
		 */
		void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep) {
			if (ec) {
				return fail(ec, "connect");
			}
			
			// Set a decorator to change the User-Agent of the handshake
			ws.set_option(websocket::stream_base::decorator([](websocket::request_type& req) {req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async");}));

			// Perform the SSL handshake
			ws.next_layer().async_handshake(ssl::stream_base::client, beast::bind_front_handler(&HTTPWebSocket::on_ssl_handshake, shared_from_this()));
		}

		/**
		 * @brief Method to asynchronously set options for the WebSocket and perform the web socket handshake after the SSL handshake
		 * 
		 * @param ec An error code object containing both a code and a text description of the specific error
		 */
		void on_ssl_handshake(beast::error_code ec) {
			if (ec) {
				return fail(ec, "handshake-ssl");
			}
			
			// Set a decorator to change the User-Agent of the handshake
			ws.set_option(websocket::stream_base::decorator(
				[&](websocket::request_type& req)
				{
					req.set("token", token);
				}));

			// Perform the websocket handshake
			ws.async_handshake(host, "/", beast::bind_front_handler(&HTTPWebSocket::on_ws_handshake, shared_from_this()));
		}

		/**
		 * @brief Method to asynchronously perform operations to begin reading data off the WebSocket
		 * 
		 * @param ec An error code object containing both a code and a text description of the specific error
		 */
		void on_ws_handshake(beast::error_code ec) {
			if (ec) {
				return fail(ec, "handshake-ws");
			}
			
			connected = true;

			// Call async_read to keep the io_context busy (the object is destroyed when the io_context runs out of work)
			ws.async_read(buffer, beast::bind_front_handler(&HTTPWebSocket::on_read, shared_from_this()));
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
			ws.async_read(buffer, beast::bind_front_handler(&HTTPWebSocket::on_read, shared_from_this()));
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

			// Message has been written, so pop it off of the queue
			isBinary.pop();
			writeQ.pop();

			// Check to see if there are more messages that can be sent
			if (writeQ.empty()) {
				return;
			} else {
				ws.binary(isBinary.front());
				
				// Call to send the next message whenever the Websocket is available
				ws.async_write(net::buffer(writeQ.front()), beast::bind_front_handler(&HTTPWebSocket::on_write, shared_from_this()));
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
		 * @brief Construct a new Web Socket Client object
		 * 
		 * @param ioc The IO context that will be responsible for handling asynchronous method calls
		 */
		explicit HTTPWebSocket(net::io_context& ioc, ssl::context& ctx) : resolver(net::make_strand(ioc)), ws(net::make_strand(ioc), ctx) {
			connected = false;
		}

		/**
		 * @brief Method to start asynchronous operations and enable the WebSocket for use
		 * 
		 * @param target The target IP Address
		 * @param token The token for connection validation
		 */
		void run(std::string target, std::string token) {
			host = target;
			this->token = token;

			// Look up the domain name
			resolver.async_resolve(host, "https", beast::bind_front_handler(&HTTPWebSocket::on_resolve, shared_from_this()));
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
		std::string read() {
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
		void write(std::string message) {
			isBinary.push(false);
			writeQ.push(message);
			
			// Check to see if there already is a call to async_write (can't have multiple calls to the same asynchronous operation queued up)
			if (writeQ.size() > 1) {
				return;
			} else {
				ws.binary(isBinary.front());

				// Call async_write if there are no existing calls to it so the message gets sent
				ws.async_write(net::buffer(writeQ.front()), beast::bind_front_handler(&HTTPWebSocket::on_write, shared_from_this()));
			}
		}

		/**
		 * @brief Method to write a message in any binary format on the send queue to be sent over the WebSocket
		 * 
		 * @param message A valid binary string to be sent over the WebSocket
		 */
		void writeBinary(std::string message) {
			isBinary.push(true);
			writeQ.push(message);
			
			// Check to see if there already is a call to async_write (can't have multiple calls to the same asynchronous operation queued up)
			if (writeQ.size() > 1) {
				return;
			} else {
				ws.binary(isBinary.front());

				// Call async_write if there are no existing calls to it so the message gets sent
				ws.async_write(net::buffer(writeQ.front()), beast::bind_front_handler(&HTTPWebSocket::on_write, shared_from_this()));
			}
		}

		/**
		 * @brief Method to close the WebSocket session
		 * 
		 */
		void close() {
            // Close the WebSocket connection
            ws.async_close(websocket::close_code::normal,beast::bind_front_handler(&HTTPWebSocket::on_close, shared_from_this()));
        }
};

} // End namespace utilities
} // End namespace wosler
