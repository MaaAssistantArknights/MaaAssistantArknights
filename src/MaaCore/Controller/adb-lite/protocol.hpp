#pragma once

#include <string_view>

#include <asio/ip/tcp.hpp>

namespace adb::protocol
{
/// Receive encoded data from the host.
/**
 * @param socket Opened adb connection.
 * @return Data from the host.
 * @throw std::runtime_error Thrown on socket failure.
 */
std::string host_message(asio::ip::tcp::socket& socket);

/// Receive all data from the host.
/**
 * @param socket Opened adb connection.
 * @return Data from the host.
 * @throw std::runtime_error Thrown on socket failure.
 * @note The function will keep reading until the connection is closed.
 */
std::string host_data(asio::ip::tcp::socket& socket);

/// Encode the ADB sync request.
/**
 * @param id 4-byte string of the request id.
 * @param length Length of the body.
 */
std::string sync_request(const std::string_view id, const uint32_t length);

/// Receive the sync response.
/**
 * @param socket Opened adb connection.
 * @param id 4-byte string of the response id.
 * @param length Length of the body.
 * @throw std::runtime_error Thrown on socket failure.
 */
void sync_response(asio::ip::tcp::socket& socket, std::string& id, uint32_t& length);

/// Send an ADB host request and check its response.
/**
 * @param socket Opened adb connection.
 * @param request Encoded request.
 * @throw std::runtime_error Thrown on socket failure or adb FAIL response.
 */
void send_host_request(asio::ip::tcp::socket& socket, const std::string_view request);

/// Send an ADB sync request.
/**
 * @param socket Opened adb connection.
 * @param id 4-byte string of the request id.
 * @param length Length of the body.
 * @param body Body of the request.
 * @throw std::runtime_error Thrown on socket failure.
 */
void send_sync_request(
    asio::ip::tcp::socket& socket,
    const std::string_view id,
    const uint32_t length,
    const char* body);
} // namespace adb::protocol
