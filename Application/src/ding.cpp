#include <iostream>
#include "lib.hpp"


int ding()
{
    std::cout << "Hello world!\n" << "The meaning of life is " << DummyLibNamespace::libFunc() << "\n";
    return 0;
}
