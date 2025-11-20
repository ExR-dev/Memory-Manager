#include <iostream>
#include "lib.hpp"


int main()
{
    std::cout << "Hello world!\n" << "The meaning of life is " << DummyLibNamespace::libFunc() << "\n";
    return 0;
}
