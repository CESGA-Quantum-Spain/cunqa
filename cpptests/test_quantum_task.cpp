#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "src/quantum_task.hpp"



class QuantumTaskTest : public ::testing::Test{
    protected:
        // Setup method that runs before each test
        void SetUp() override {
            quantum_task = new QuantumTask();
        }

        // Teardown method that runs after each test
        void TearDown() override {
            delete quantum_task;
        }

        // Shared test fixture
        QuantumTask* quantum_task;
}

// Test to_string
TEST_F(QuantumTaskTest, ToStringTest) {
    
}

// Test string_constructor
TEST_F(QuantumTaskTest, ToStringConstructorTest) {
    
}

// Test update_circuit
TEST_F(QuantumTaskTest, UpdateCircuitTest) {
    
}
// Test update_params_
TEST_F(QuantumTaskTest, UpdateParamsTest) {
    
}

// Main test runner
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}