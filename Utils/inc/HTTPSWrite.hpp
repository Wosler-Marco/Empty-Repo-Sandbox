
#include <boost/asio/ssl.hpp>
#include <boost/beast/websocket.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = boost::beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;  
namespace ssl = boost::asio::ssl;          // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; 

wosler::utilities::Error HTTPSWrite(std::string addr, http::request<http::string_body>& req, http::response<http::string_body>& res) {
    // Setting up the https stream
    net::io_context ioc;
    
    ssl::context ctx(ssl::context::tlsv12_client);
    // Load CA certificates for server verification
    // ctx.load_verify_file("path/to/ca/cert.pem");
    
    tcp::resolver resolver(ioc);
    
    // Resolve the server address
    auto const results = resolver.resolve(addr, "https");
        
    // Create and connect a socket
    ssl::stream<tcp::socket> stream(ioc, ctx);
    net::connect(stream.next_layer(), results.begin(), results.end());
    stream.handshake(ssl::stream_base::client);

    http::write(stream, req);
    
    // Receive the HTTPS response
    boost::beast::flat_buffer buffer;
    size_t readSize = http::read(stream, buffer, res);
    // std::cout << res << "\n\n";

    boost::beast::error_code ec;
    stream.shutdown(ec);
    stream.next_layer().close();

    ioc.run();

    if(readSize != 0) {
        return wosler::utilities::Error::SUCCESS;
    }
    else {
        return wosler::utilities::Error::FAILED;
    }
}