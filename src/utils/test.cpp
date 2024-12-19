#include <iostream>
#include <string>

void Log (std::string *message){
    std::cout << message << "\n";
}

int Multiply(int a, int b){
    std::string m = "Multiply";
    Log(&m);
    return a*b;
}

int main(){

    char* x = "Hola";
    std::cout << Multiply(2,3) << "\n";

    return 0;
}