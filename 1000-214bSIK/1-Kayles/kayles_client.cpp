#include <cerrno>
#include <exception>
#include <iostream>
#include <system_error>

#include "argparse.h"
#include "messages.h"
#include "sockets.h"

using std::cerr;
using std::cout;
using std::exception;
using std::system_error;

int main(int argc, char *argv[]) {
    try {
        auto [address, port, message, timeout] =
            parse_args<Arg::ADDRESS, Arg::PORT, Arg::MESSAGE, Arg::TIMEOUT>(argc, argv);

        ClientSocket socket(timeout);

        socket.send(message, address, port);

        cout << socket.receive() << '\n';
    }
    catch (const system_error& ex) {
        if (ex.code().value() == EAGAIN || ex.code().value() == EWOULDBLOCK) {
            cout << "Timed out\n";
            return 0;
        }

        cerr << "Error : " << ex.what() << '\n';
        return 1;
    }
    catch (const exception& ex) {
        cerr << "Error : " << ex.what() << '\n';
        return 1;
    }
}
