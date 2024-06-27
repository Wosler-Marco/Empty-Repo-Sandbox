#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <queue>
// #include <map>

#include "CommonUtilities.hpp"

typedef websocketpp::client<websocketpp::config::asio_client> client;
// typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

// using websocketpp::lib::placeholders::_1;
// using websocketpp::lib::placeholders::_2;
// using websocketpp::lib::bind;

// context_ptr on_tls_init(const char * hostname, websocketpp::connection_hdl) {
// 		context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);

// 		try {
// 			ctx->set_options(boost::asio::ssl::context::default_workarounds |
// 							boost::asio::ssl::context::no_sslv2 |
// 							boost::asio::ssl::context::no_sslv3 |
// 							boost::asio::ssl::context::single_dh_use);


// 			ctx->set_verify_mode(boost::asio::ssl::verify_none);
// 		} catch (std::exception& e) {
// 			std::cout << e.what() << std::endl;
// 		}
// 		return ctx;
// 	}

namespace wosler {
namespace utilities {

enum class ConnectionStatus {
	Connecting,
	Open,
	Failed,
	Closed,
	Invalid
};

class connection_metadata {
public:
    typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;

    connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri)
      : m_id(id)
      , m_hdl(hdl)
      , m_status(ConnectionStatus::Connecting)
      , m_uri(uri)
      , m_server("N/A")
    {}

	websocketpp::connection_hdl get_hdl() const {
		return m_hdl;
	}

	ConnectionStatus get_status() const {
		return m_status;
	}

	std::string get_uri() const {
		return m_uri;
	}

	std::string get_server() const {
		return m_server;
	}

    void on_open(client * c, websocketpp::connection_hdl hdl) {
        m_status = ConnectionStatus::Open;

        client::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
    }

    void on_fail(client * c, websocketpp::connection_hdl hdl) {
        m_status = ConnectionStatus::Failed;

        client::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
        m_error_reason = con->get_ec().message();
    }

	void on_close(client * c, websocketpp::connection_hdl hdl) {
		m_status = ConnectionStatus::Closed;
		client::connection_ptr con = c->get_con_from_hdl(hdl);
		std::stringstream s;
		s << "close code: " << con->get_remote_close_code() << " (" 
			<< websocketpp::close::status::get_string(con->get_remote_close_code()) 
			<< "), close reason: " << con->get_remote_close_reason();
		m_error_reason = s.str();
	}

	void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg) {
        if (msg->get_opcode() == websocketpp::frame::opcode::text) {
            std::string received_message = msg->get_payload();
            m_messages.push(received_message);
        } else {
            std::string received_message = websocketpp::utility::to_hex(msg->get_payload());
            m_messages.push(received_message);
        }
    }
	bool readQEmpty() {
		return m_messages.empty();
	}

	std::string on_read() {
		if (!m_messages.empty()) {
			std::string message = m_messages.front();
			m_messages.pop();
			return message;
		} else {
			return "";
		}
		
	}

    friend std::ostream & operator<< (std::ostream & out, connection_metadata const & data);
private:
    int m_id = -1;
    websocketpp::connection_hdl m_hdl;
    ConnectionStatus m_status = ConnectionStatus::Invalid;
    std::string m_uri;
    std::string m_server;
    std::string m_error_reason;
	std::queue<std::string> m_messages;
};

std::ostream & operator<< (std::ostream & out, connection_metadata const & data) {
    out << "> URI: " << data.m_uri << "\n"
        << "> Status: ";
	switch (data.m_status)
    {
        case ConnectionStatus::Connecting:
			out << "Connecting";
			break;
        case ConnectionStatus::Open:
			out << "Open";
			break;
        case ConnectionStatus::Failed:
			out << "Failed";
			break;
        case ConnectionStatus::Closed:
			out << "Closed";
			break;
		default:
			out << "Invalid Status";
	}
	out << "\n"
    	<< "> Remote Server: " << (data.m_server.empty() ? "None Specified" : data.m_server) << "\n"
        << "> Error/close reason: " << (data.m_error_reason.empty() ? "N/A" : data.m_error_reason) << "\n"
		<< "> Queue Size:" << data.m_messages.size() << "\n";

    return out;
}
class WSInterface : public std::enable_shared_from_this<WSInterface> {
	private:
		typedef std::map<int,connection_metadata::ptr> con_list;
		
		// WebSocket private variables
		client m_endpoint;
    	websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;
		std::string m_token = "";
    std::map<std::string, std::string> m_headers;

		con_list m_connection_list;
    	int m_next_id;

		// Send and receive queue private variables
		const size_t MAX_QUEUE_SIZE = 16;
		std::queue<std::string> readQ;
		std::queue<std::string> writeQ;
		std::queue<bool> isBinary;

		std::string host;
		std::string port;
		std::string uri;
		bool connected;

	public:

		/**
		 * @brief Construct a new Web Socket Client object
		 * 
		 * @param ioc The IO context that will be responsible for handling asynchronous method calls
		 */
		explicit WSInterface() {
			connected = false;

      // No Logging
			//m_endpoint.set_access_channels(websocketpp::log::alevel::none);
			//m_endpoint.set_error_channels(websocketpp::log::elevel::none);

			// Set logging to be pretty verbose (everything except message payloads)
      m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
      m_endpoint.clear_error_channels(websocketpp::log::elevel::all);

			m_endpoint.init_asio();
			m_endpoint.start_perpetual();

			m_thread = websocketpp::lib::make_shared<websocketpp::lib::thread>(&client::run, &m_endpoint);
		}

		/**
		 * @brief Method to start asynchronous operations and enable the WebSocket for use
		 * 
		 * @param target The target IP Address
		 * @param token The token for connection validation
		 */
		int connect(std::string const& host) {
			this->host = host;
			uri = "ws://" + host;
			// Register our message handler
			// m_endpoint.set_message_handler(&on_read);
			// m_endpoint.set_tls_init_handler(bind(&on_tls_init, host.c_str(), ::_1));

			websocketpp::lib::error_code ec;
      client::connection_ptr con = m_endpoint.get_connection(uri, ec);

      if (ec) {
				std::cout << "> Connect initialization error: " << ec.message() << std::endl;
				return -1;
			}

      for (const auto &ele : m_headers) {
        con->append_header(ele.first,ele.second);
        // m_headers.erase(ele);
      }
			// con->append_header("token",m_token);

			if (ec) {
				std::cout << "> Connect initialization error: " << ec.message() << std::endl;
				return -1;
			}

			int new_id = m_next_id++;
			connection_metadata::ptr metadata_ptr = websocketpp::lib::make_shared<connection_metadata>(new_id, con->get_handle(), uri);
			m_connection_list[new_id] = metadata_ptr;

			con->set_open_handler(websocketpp::lib::bind(
				&connection_metadata::on_open,
				metadata_ptr,
				&m_endpoint,
				websocketpp::lib::placeholders::_1
			));
			con->set_fail_handler(websocketpp::lib::bind(
				&connection_metadata::on_fail,
				metadata_ptr,
				&m_endpoint,
				websocketpp::lib::placeholders::_1
			));

			con->set_message_handler(websocketpp::lib::bind(
				&connection_metadata::on_message,
				metadata_ptr,
				websocketpp::lib::placeholders::_1,
				websocketpp::lib::placeholders::_2
			));

			m_endpoint.connect(con);

			return new_id;
		}
    void setHeader(std::string key, std::string value) {
      m_headers.insert({key,value});
      return;
    }

		void close(int id, websocketpp::close::status::value code, const std::string& reason) {
			websocketpp::lib::error_code ec;
			
			con_list::iterator metadata_it = m_connection_list.find(id);
			if (metadata_it == m_connection_list.end()) {
				std::cout << "> No connection found with id " << id << std::endl;
				return;
			}
			
			websocketpp::connection_hdl hdl = metadata_it->second->get_hdl();
			m_endpoint.close(hdl, code, reason, ec);
			if (ec) {
				std::cout << "> Error initiating close: " << ec.message() << std::endl;
			}
		}

		void send(int id, std::string message) {
			websocketpp::lib::error_code ec;
			
			con_list::iterator metadata_it = m_connection_list.find(id);
			if (metadata_it == m_connection_list.end()) {
				std::cout << "> No connection found with id " << id << std::endl;
				return;
			}
			
			m_endpoint.send(metadata_it->second->get_hdl(), message, websocketpp::frame::opcode::text, ec);
			if (ec) {
				std::cout << "> Error sending message: " << ec.message() << std::endl;
				std::cout << "> Message: " << message << std::endl;
				return;
			}
		}

		std::string read(int id) {
			con_list::iterator metadata_it = m_connection_list.find(id);
			if (metadata_it == m_connection_list.end()) {
				std::cout << "> No connection found with id " << id << std::endl;
				return "";
			}
			return metadata_it->second->on_read();
		}

		bool readQEmpty(int id) {
			con_list::iterator metadata_it = m_connection_list.find(id);
			if (metadata_it == m_connection_list.end()) {
				std::cout << "> No connection found with id " << id << std::endl;
				return true;
			}
			return metadata_it->second->readQEmpty();
		}

		connection_metadata::ptr get_metadata(int id) const {
			con_list::const_iterator metadata_it = m_connection_list.find(id);
			if (metadata_it == m_connection_list.end()) {
				return connection_metadata::ptr();
			} else {
				return metadata_it->second;
			}
		}
};

} // End namespace utilities
} // End namespace wosler
