#include <iostream>
#include <string>

void arr_fun(int* x []){

    *x[0] = 3;
    std::cout << "Dentro de la funcion: " << x[0] << "\n";

     
}

void t(char* args[]){

    std::cout << "Hola \n";

}



int main(){

    int w1 [3] = {1,2,3};
    int w2 [3] = {4,5,6};
    
    //int* x1 = w1;
    //int* x2 = w2;
     

    int* y []  = {w1,w2};
    int v [] = {1,2};

    arr_fun(y);

    std::cout << y[0] << "\n";


    std::cout << y << " " << &w1 << "\n";

    char a = 'Hola';
    char b = 'b';

    char* ab[] = {&a,&b};

    t(ab);

    return 0;
}