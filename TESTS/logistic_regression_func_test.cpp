#include <iostream>
#include <stdlib.h>
#include <math.h>


/*
Data from offline logistic regression classifier training

Weights ->>>  [[1.751 1.132 0.005]]
B = Bias ->>>>  [-7.898]
z = B + W1X1 + W2X2 + W3X3

sigmoid func =  1/(1 + e^-z)
*/
bool validate_data(float x1, float x2, float x3){


    float bias = -7.898;
    float w1 = 1.751;
    float w2 = 1.132;
    float w3 = 0.005; 
  
  
    float z = bias + w1 * x1 + w2*x2 + w3 * x3;
  
  
    float logi = 1.0 / (1.0  + (exp(-(double)z)));

    return (logi >= 1.0); //Returns true if fail
}  

int main(){

    //More tests can be done with different values, the script can be made more robust by adding command line in arguements

    //failed attempts, button presses , time since last unlock, 
    if(validate_data(2.0,1.0,9000.0)){
        printf("fail \n");
    }else{
        printf("success \n");
    }



    return 0;
}

