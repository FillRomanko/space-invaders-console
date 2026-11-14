#include <iostream>
#include "utils/console_utils.h"

int main() {
    if (!console_utils::InitializeConsole(400, 200)) {
        return 1;
    }

    char a[100];
    std::cin >> a;
    return 0;
}
