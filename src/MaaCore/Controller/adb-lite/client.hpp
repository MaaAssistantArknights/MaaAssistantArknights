#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace adb
{
/// Retrieve the version of local adb server.
/**
 * @return 4-byte string of the version number.
 * @throw std::system_error if the server is not available.
 */
std::string version();

/// Retrieve the available Android devices.
/**
 * @return A string of the list of devices.
 * @throw std::system_error if the server is not available.
 * @note Equivalent to `adb devices`.
 */
std::string devices();

/// Kill the adb server if it is running.
/**
 * @throw std::system_error if the server is not running.
 * @note Equivalent to `adb kill-server`.
 */
void kill_server();

class io_handle_impl;

/// Context for an interactive adb connection.
/**
 * @note Should be used only by the client class, after a shell request.
 * @note The socket will be closed when the handle is destroyed.
 */
class io_handle
{
public:
    virtual ~io_handle() = default;

    /// Write data to the adb connection.
    /**
     * @param data Data to write.
     * @throw std::runtime_error Thrown on socket failure.
     * @note Typically used to write stdin of a shell command.
     * @note The data should end with a newline.
     */
    virtual void write(const std::string_view data) = 0;

    /// Read data from the adb connection.
    /**
     * @param timeout Timeout in seconds. 0 means no timeout.
     * @return Data read. Empty if timeout or the connection is closed.
     * @throw std::runtime_error Thrown on socket failure.
     * @note Typically used to read stdout of a shell command.
     */
    virtual std::string read(unsigned timeout = 0) = 0;

protected:
    io_handle() = default;
};

class client_impl;

/// A client for the Android Debug Bridge.
class client
{
public:
    /// Create a client for a specific device.
    /**
     * @param serial serial number of the device.
     * @note If the serial is empty, the unique device will be used. If there
     * are multiple devices, an exception will be thrown.
     */
    static std::shared_ptr<client> create(const std::string_view serial);
    virtual ~client() = default;

    /// Connect to the device.
    /**
     * @return A string of the connection status.
     * @throw std::system_error if the server is not available.
     * @note Equivalent to `adb connect <serial>`.
     */
    virtual std::string connect() = 0;

    /// Disconnect from the device.
    /**
     * @return A string of the disconnection status.
     * @throw std::system_error if the server is not available.
     * @note Equivalent to `adb disconnect <serial>`.
     */
    virtual std::string disconnect() = 0;

    /// Retrieve the version of local adb server.
    /**
     * @return 4-byte string of the version number.
     * @throw std::system_error if the server is not available.
     * @note This function reuses the class member io_context, which is
     * thread-safe for the client.
     */
    virtual std::string version() = 0;

    /// Retrieve the available Android devices.
    /**
     * @return A string of the list of devices.
     * @throw std::system_error if the server is not available.
     * @note Equivalent to `adb devices`.
     * @note This function reuses the class member io_context, which is
     * thread-safe for the client.
     */
    virtual std::string devices() = 0;

    /// Send an one-shot shell command to the device.
    /**
     * @param command Command to execute.
     * @return A string of the command output.
     * @throw std::system_error if the server is not available.
     * @note Equivalent to `adb -s <serial> shell <command>` without stdin.
     */
    virtual std::string shell(const std::string_view command) = 0;

    /// Send an one-shot shell command to the device, using raw PTY.
    /**
     * @param command Command to execute.
     * @return A string of the command output, which is not mangled.
     * @throw std::system_error if the server is not available.
     * @note Equivalent to `adb -s <serial> exec-out <command>` without stdin.
     */
    virtual std::string exec(const std::string_view command) = 0;

    /// Send a file to the device.
    /**
     * @return true if the file is successfully sent.
     * @param src Path to the source file.
     * @param dst Path to the destination file.
     * @param perm Permission of the destination file.
     * @throw std::system_error if the server is not available.
     * @note Equivalent to `adb -s <serial> push <src> <dst>`.
     */
    virtual bool push(const std::string_view src, const std::string_view dst, int perm) = 0;

    /// Set the user of adbd to root on the device.
    /**
     * @throw std::system_error if the server is not available.
     * @note Equivalent to `adb -s <serial> root`.
     * @note The device might be offline after this command. Remember to wait
     * for the restart.
     */
    virtual std::string root() = 0;

    /// Set the user of adbd to non-root on the device.
    /**
     * @throw std::system_error if the server is not available.
     * @note Equivalent to `adb -s <serial> unroot`.
     * @note The device might be offline after this command. Remember to wait
     * for the restart.
     */
    virtual std::string unroot() = 0;

    /// Start an interactive shell session on the device.
    /**
     * @param command Command to execute.
     * @return An io_handle for the interactive session.
     * @throw std::system_error if the server is not available.
     * @note Equivalent to `adb -s <serial> shell <command>` with stdin.
     */
    virtual std::shared_ptr<io_handle> interactive_shell(const std::string_view command) = 0;

    /// Wait for the device to be available.
    /**
     * @throw std::system_error if the server is not available.
     */
    virtual void wait_for_device() = 0;

protected:
    client() = default;
};
} // namespace adb
