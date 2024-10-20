#include <string>
#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>


static void die(const std::string &msg) {
    throw std::runtime_error(msg + ": " + std::strerror(errno));
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("Failed to create socket");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);  // 127.0.0.1

    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("Failed to connect");
    }

    std::string msg = "hello";
    ssize_t bytes_sent = write(fd, msg.c_str(), msg.size());
    if (bytes_sent < 0) {
        die("Failed to send message");
    }

    std::array<char, 64> rbuf = {};
    ssize_t n = read(fd, rbuf.data(), rbuf.size() - 1);
    if (n < 0) {
        die("Failed to read message");
    }

    // print the message received from the server
    std::cout << "server says: " << rbuf.data() << std::endl;

    close(fd);

    return 0;
}
