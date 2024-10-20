#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <span>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

const size_t k_max_msg = 4096;

static void msg(const std::string &msg) {
    std::cerr << msg << std::endl;
}

[[noreturn]] void die(const std::string &msg) {
    throw std::runtime_error(msg + ": " + std::strerror(errno));
}

static int32_t read_full(int fd, std::span<char> buf) {
    while (!buf.empty()) {
        ssize_t rv = read(fd, buf.data(), buf.size());
        if (rv <= 0) {
            return -1;  // error, or unexpected EOF
        }
        assert(static_cast<size_t>(rv) <= buf.size());
        buf = buf.subspan(rv);
    }
    return 0;
}

static int32_t write_all(int fd, std::span<const char> buf) {
    while (!buf.empty()) {
        ssize_t rv = write(fd, buf.data(), buf.size());
        if (rv <= 0) {
            return -1;  // error
        }
        assert(static_cast<size_t>(rv) <= buf.size());
        buf = buf.subspan(rv);
    }
    return 0;
}

static int32_t query(int fd, const std::string &text) {
    uint32_t len = static_cast<uint32_t>(text.size());
    if (len > k_max_msg) {
        return -1;
    }

    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4);  // assume little endian
    memcpy(&wbuf[4], text.data(), len);

    if (int32_t err = write_all(fd, std::span(wbuf, 4 + len))) {
        return err;
    }

    // 4 bytes header
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(fd, std::span(rbuf, 4));
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }

    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // reply body
    err = read_full(fd, std::span(&rbuf[4], len));
    if (err) {
        msg("read() error");
        return err;
    }

    // do something
    rbuf[4 + len] = '\0';
    std::cout << "server says: " << &rbuf[4] << std::endl;
    return 0;
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);  // 127.0.0.1

    int rv = connect(fd, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr));
    if (rv) {
        die("connect");
    }

    // multiple requests
    try {
        if (query(fd, "hello1")) throw std::runtime_error("Query failed");
        if (query(fd, "hello2")) throw std::runtime_error("Query failed");
        if (query(fd, "hello3")) throw std::runtime_error("Query failed");
    } catch (const std::exception &e) {
        msg(e.what());
    }

    close(fd);
    return 0;
}
