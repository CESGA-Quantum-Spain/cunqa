#include <iostream>

void test_func(){

    std::cout << "Hola \n" ;
}

void call_test(void(*lolo)()){

    lolo();

}

int main(){


    call_test(&test_func);



    return 0;
}