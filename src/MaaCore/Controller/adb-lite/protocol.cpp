#include <iomanip>
#include <boost/asio/read.hpp>

#include "protocol.hpp"
#include "stage.hpp"

using boost::asio::ip::tcp;

namespace adb::protocol
{
/// Encoded the ADB host request.
/**
 * @param body Body of the request.
 * @return Encoded request.
 */
static inline std::string host_request(const std::string_view body)
{
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(4) << std::hex << body.size() << body;
    return ss.str();
}

/// Receive and check the response.
/**
 * @param socket Opened adb connection.
 * @param failure Error message if the response is not OKAY.
 * @return `true` if the response is OKAY, `false` otherwise.
 * @throw std::runtime_error Thrown on socket failure
 */
static inline bool host_response(tcp::socket& socket, std::string& failure)
{
    std::array<char, 4> header;
    stage::set("wait-adb-host-response-header");
    boost::asio::read(socket, boost::asio::buffer(header));
    const auto result = std::string_view(header.data(), header.size());
    if (result == "OKAY") {
        return true;
    }

    if (result != "FAIL") {
        failure = "unknown response";
        return false;
    }

    failure = host_message(socket);
    return false;
}

std::string host_message(tcp::socket& socket)
{
    std::array<char, 4> header;
    stage::set("wait-adb-host-message-length");
    boost::asio::read(socket, boost::asio::buffer(header));
    auto remain = std::stoull(std::string(header.data(), header.size()), nullptr, 16);

    std::string message;
    std::array<char, 1024> buffer;
    while (remain > 0) {
        stage::set("wait-adb-host-message-payload");
        const auto length = socket.read_some(boost::asio::buffer(buffer));
        message.append(buffer.data(), length);
        remain -= length;
    }

    return message;
}

std::string host_data(tcp::socket& socket)
{
    std::string data;
    std::array<char, 1024> buffer;
    boost::system::error_code ec;

    stage::set("wait-device-output-or-eof");
    while (!ec) {
        const auto length = socket.read_some(boost::asio::buffer(buffer), ec);
        if (ec == boost::asio::error::eof) {
            stage::set("device-stream-eof");
            break;
        }
        if (ec) {
            stage::set("device-stream-error-" + ec.message());
        }
        if (length > 0) {
            data.append(buffer.data(), length);
        }
    }

    return data;
}

std::string sync_request(const std::string_view id, const uint32_t length)
{
    const auto len = {
        static_cast<char>(length & 0xff),
        static_cast<char>((length >> 8) & 0xff),
        static_cast<char>((length >> 16) & 0xff),
        static_cast<char>((length >> 24) & 0xff),
    };

    return std::string(id) + std::string(len.begin(), len.end());
}

void sync_response(tcp::socket& socket, std::string& id, uint32_t& length)
{
    std::array<char, 8> response;
    socket.read_some(boost::asio::buffer(response));

    id = std::string(response.data(), 4);
    length = response[4] | (response[5] << 8) | (response[6] << 16) | (response[7] << 24);
}

void send_host_request(tcp::socket& socket, const std::string_view request)
{
    stage::set("write-adb-host-request");
    socket.write_some(boost::asio::buffer(protocol::host_request(request)));
    std::string failure;
    if (!protocol::host_response(socket, failure)) {
        throw std::runtime_error(failure);
    }
}

void send_sync_request(tcp::socket& socket, const std::string_view id, uint32_t length, const char* body)
{
    auto data_request = protocol::sync_request(id, length);
    socket.write_some(boost::asio::buffer(data_request));
    socket.write_some(boost::asio::buffer(body, length));
}
} // namespace adb::protocol
