#include <iostream>

extern int algrbraic(int a, int b);
extern int constantFolding();
extern int constantPropagation();
extern int deadCodeElimination();
extern int strengthReduction();

int main() {
    std::cout << "algrbraic(10, 20) = " << algrbraic(10, 20) << std::endl;
    std::cout << "constantFolding() = " << constantFolding() << std::endl;
    std::cout << "constantPropagation() = " << constantPropagation() << std::endl;
    std::cout << "deadCodeElimination() = " << deadCodeElimination() << std::endl;
    std::cout << "strengthReduction() = " << strengthReduction() << std::endl;
    return 0;
}
