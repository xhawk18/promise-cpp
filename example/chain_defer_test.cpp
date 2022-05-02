#include <promise-cpp/promise.hpp>
#include <sstream>
#include <iostream>

promise::Promise writeTo(std::ostream& out) {
    return promise::newPromise().then([&out](int value) {
        out << value;
        return promise::reject(std::string(" testErrorReason "));
    }, [&out](const std::string& reason) {
        out << reason;
        return 456;
    });
}

int main() {
    promise::Promise d = promise::newPromise();
    std::ostringstream out;
    promise::Promise writeToOut = writeTo(out);
    d.then(writeToOut).then(writeTo(out)).then(writeTo(out));
    d.resolve(123);

    std::string expected = "123 testErrorReason 456";
    if (out.str() != expected) {
        std::cout << "FAIL chain_defer_test got \"" << out.str() << "\", "
                  << "expected \"" << expected << "\"\n";
        return 1;
    }

    std::cout << "PASS\n";
    return 0;
}
