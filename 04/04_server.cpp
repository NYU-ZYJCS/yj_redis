#include <array>
#include <cassert>
#include <string>
#include <span>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

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

static int32_t one_request(int connfd) {
    // 4 bytes header
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(connfd, std::span(rbuf, 4));
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // request body
    err = read_full(connfd, std::span(&rbuf[4], len));
    if (err) {
        msg("read() error");
        return err;
    }

    // do something
    rbuf[4 + len] = '\0';
    std::cout << "client says: " << &rbuf[4] << std::endl;

    // reply using the same protocol
    const std::string reply = "world";
    char wbuf[4 + reply.size()];
    len = static_cast<uint32_t>(reply.size());
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply.data(), len);

    return write_all(connfd, std::span(wbuf, 4 + len));
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    // this is needed for most server applications
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_ANY);    // wildcard address 0.0.0.0
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("bind()");
    }

    // listen
    rv = listen(fd, SOMAXCONN);
    if (rv) {
        die("listen()");
    }

    while (true) {
        // accept
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, reinterpret_cast<sockaddr*>(&client_addr), &socklen);
        if (connfd < 0) {
            continue;   // error
        }

        while (true) {
            // here the server only serves one client connection at once
            int32_t err = one_request(connfd);
            if (err) {
                break;
            }
        }
        close(connfd);
    }

    return 0;
}
