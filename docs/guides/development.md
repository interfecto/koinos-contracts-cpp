# Koinos Smart Contract Development Guide

## Introduction

This guide provides comprehensive instructions for developing smart contracts on the Koinos blockchain using C++. It covers environment setup, contract development, testing, and deployment.

## Prerequisites

### Required Software

1. **Koinos CDT (Contract Development Toolkit)**
   - Version: Latest stable
   - [GitHub Repository](https://github.com/koinos/koinos-cdt)

2. **WASI SDK**
   - WebAssembly System Interface SDK
   - Required for WASM compilation

3. **Build Tools**
   - CMake 3.10.2+
   - Make or Ninja
   - Git

4. **C++ Compiler**
   - Clang 10+ (recommended)
   - GCC 9+ (alternative)

### Development Environment Setup

#### Linux/macOS

```bash
# Clone and build Koinos CDT
git clone https://github.com/koinos/koinos-cdt.git
cd koinos-cdt
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
sudo make install

# Set environment variables
export KOINOS_SDK_ROOT=/usr/local/koinos-cdt
export KOINOS_WASI_SDK_ROOT=/usr/local/wasi-sdk
```

#### Windows

```powershell
# Using WSL2 is recommended
wsl --install

# Follow Linux instructions within WSL2
```

## Project Structure

### Recommended Directory Layout

```
my-contract/
├── CMakeLists.txt          # Build configuration
├── src/
│   └── my_contract.cpp     # Contract implementation
├── include/
│   └── my_contract.hpp     # Contract headers
├── abi/
│   └── my_contract.abi     # Contract ABI definition
├── tests/
│   └── test_contract.cpp   # Unit tests
├── scripts/
│   ├── deploy.sh           # Deployment script
│   └── interact.js         # Interaction examples
└── docs/
    └── README.md           # Contract documentation
```

## Writing Your First Contract

### Basic Contract Template

```cpp
// my_contract.cpp
#include <koinos/system/system_calls.hpp>
#include <koinos/contracts.hpp>
#include <koinos/buffer.hpp>

using namespace koinos;

// Define constants
namespace constants {
    constexpr std::size_t max_buffer_size = 2048;
    constexpr std::size_t max_string_size = 256;
}

// Define entry points
enum entries : uint32_t {
    get_value_entry = 0x12345678,
    set_value_entry = 0x87654321
};

// State management
namespace state {
    system::object_space contract_space() {
        static system::object_space space;
        static bool initialized = false;
        
        if (!initialized) {
            auto contract_id = system::get_contract_id();
            space.mutable_zone().set(
                reinterpret_cast<const uint8_t*>(contract_id.data()),
                contract_id.size()
            );
            space.set_id(0);
            space.set_system(false);
            initialized = true;
        }
        
        return space;
    }
}

// Contract functions
struct value_object {
    uint64_t value;
};

uint64_t get_value() {
    value_object obj;
    system::get_object(state::contract_space(), "value", obj);
    return obj.value;
}

void set_value(uint64_t new_value) {
    // Check authorization
    if (!system::check_authority(system::get_contract_id())) {
        system::fail("Unauthorized", chain::error_code::authorization_failure);
    }
    
    value_object obj;
    obj.value = new_value;
    system::put_object(state::contract_space(), "value", obj);
}

// Main entry point
int main() {
    auto [entry_point, args] = system::get_arguments();
    
    std::array<uint8_t, constants::max_buffer_size> retbuf;
    koinos::write_buffer buffer(retbuf.data(), retbuf.size());
    
    switch(entry_point) {
        case entries::get_value_entry: {
            auto value = get_value();
            // Serialize result
            buffer.write(value);
            break;
        }
        case entries::set_value_entry: {
            koinos::read_buffer rdbuf(
                reinterpret_cast<uint8_t*>(const_cast<char*>(args.c_str())),
                args.size()
            );
            uint64_t new_value;
            rdbuf.read(new_value);
            set_value(new_value);
            break;
        }
        default:
            system::revert("Unknown entry point");
    }
    
    system::result r;
    r.mutable_object().set(buffer.data(), buffer.get_size());
    system::exit(0, r);
    
    return 0;
}
```

### CMakeLists.txt Configuration

```cmake
cmake_minimum_required(VERSION 3.10.2)
project(my_contract)

# Find Koinos SDK
find_package(KoinosSDK REQUIRED)

# Add contract executable
add_executable(my_contract src/my_contract.cpp)

# Set properties for WASM compilation
set_target_properties(my_contract PROPERTIES
    SUFFIX ".wasm"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# Link Koinos libraries
target_link_libraries(my_contract
    koinos::sdk
    koinos::system
)

# Include directories
target_include_directories(my_contract PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
```

## Advanced Contract Features

### 1. Token Implementation

```cpp
class Token {
private:
    struct balance_object {
        uint64_t amount;
    };
    
    system::object_space balance_space() {
        // Implementation
    }

public:
    void transfer(const std::string& from, 
                 const std::string& to, 
                 uint64_t amount) {
        // Check authorization
        if (!system::check_authority(from)) {
            system::fail("Not authorized");
        }
        
        // Get balances
        balance_object from_bal, to_bal;
        system::get_object(balance_space(), from, from_bal);
        system::get_object(balance_space(), to, to_bal);
        
        // Check sufficient balance
        if (from_bal.amount < amount) {
            system::fail("Insufficient balance");
        }
        
        // Update balances
        from_bal.amount -= amount;
        to_bal.amount += amount;
        
        // Save state
        system::put_object(balance_space(), from, from_bal);
        system::put_object(balance_space(), to, to_bal);
        
        // Emit event
        emit_transfer_event(from, to, amount);
    }
    
    uint64_t balance_of(const std::string& account) {
        balance_object bal;
        system::get_object(balance_space(), account, bal);
        return bal.amount;
    }
};
```

### 2. Multi-signature Authorization

```cpp
bool check_multisig_authority(const std::vector<std::string>& signers,
                              uint32_t threshold) {
    uint32_t valid_signatures = 0;
    
    for (const auto& signer : signers) {
        if (system::check_authority(signer)) {
            valid_signatures++;
            if (valid_signatures >= threshold) {
                return true;
            }
        }
    }
    
    return false;
}

void protected_operation() {
    std::vector<std::string> required_signers = {
        "1ABC...",
        "1DEF...",
        "1GHI..."
    };
    
    if (!check_multisig_authority(required_signers, 2)) {
        system::fail("Requires 2 of 3 signatures");
    }
    
    // Perform protected operation
}
```

### 3. Time-based Logic

```cpp
struct time_lock {
    uint64_t unlock_time;
    uint64_t amount;
};

void create_timelock(const std::string& beneficiary,
                    uint64_t amount,
                    uint64_t lock_duration_ms) {
    auto current_time = system::get_head_info().head_block_time();
    
    time_lock lock;
    lock.unlock_time = current_time + lock_duration_ms;
    lock.amount = amount;
    
    system::put_object(timelock_space(), beneficiary, lock);
}

void withdraw_timelock(const std::string& beneficiary) {
    time_lock lock;
    if (!system::get_object(timelock_space(), beneficiary, lock)) {
        system::fail("No timelock found");
    }
    
    auto current_time = system::get_head_info().head_block_time();
    if (current_time < lock.unlock_time) {
        system::fail("Timelock not yet expired");
    }
    
    // Transfer funds
    transfer_to(beneficiary, lock.amount);
    
    // Remove timelock
    system::remove_object(timelock_space(), beneficiary);
}
```

## Testing Contracts

### Unit Testing Framework

```cpp
// test_contract.cpp
#include <catch2/catch.hpp>
#include "my_contract.hpp"

TEST_CASE("Contract initialization", "[init]") {
    // Setup test environment
    TestEnvironment env;
    
    // Deploy contract
    auto contract = env.deploy_contract("my_contract.wasm");
    
    // Test initial state
    REQUIRE(contract.call("get_value") == 0);
}

TEST_CASE("Value updates", "[update]") {
    TestEnvironment env;
    auto contract = env.deploy_contract("my_contract.wasm");
    
    // Set value
    contract.call_with_auth("set_value", 42);
    
    // Verify update
    REQUIRE(contract.call("get_value") == 42);
}

TEST_CASE("Authorization checks", "[auth]") {
    TestEnvironment env;
    auto contract = env.deploy_contract("my_contract.wasm");
    
    // Attempt unauthorized update
    REQUIRE_THROWS_AS(
        contract.call_without_auth("set_value", 42),
        authorization_exception
    );
}
```

### Integration Testing

```javascript
// test_integration.js
const { Contract, Provider, Signer } = require('@koinos/sdk-js');

describe('Contract Integration Tests', () => {
    let contract;
    let signer;
    
    beforeEach(async () => {
        const provider = new Provider('http://localhost:8080');
        signer = new Signer(privateKey, provider);
        
        contract = new Contract({
            id: contractId,
            abi: contractAbi,
            provider,
            signer
        });
    });
    
    test('should update value', async () => {
        await contract.functions.set_value({ value: 42 });
        const result = await contract.functions.get_value();
        expect(result.value).toBe(42);
    });
});
```

## Building and Optimization

### Build Commands

```bash
# Debug build
mkdir build-debug && cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_TOOLCHAIN_FILE=${KOINOS_SDK_ROOT}/cmake/koinos-wasm-toolchain.cmake \
      ..
make

# Release build with optimizations
mkdir build-release && cd build-release
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=${KOINOS_SDK_ROOT}/cmake/koinos-wasm-toolchain.cmake \
      ..
make

# Size optimization
wasm-opt -Oz contract.wasm -o contract-optimized.wasm
```

### Optimization Techniques

#### 1. Minimize State Reads
```cpp
// Bad: Multiple state reads
auto balance1 = get_balance(account);
process1(balance1);
auto balance2 = get_balance(account);  // Redundant
process2(balance2);

// Good: Single state read
auto balance = get_balance(account);
process1(balance);
process2(balance);
```

#### 2. Use Stack Allocation
```cpp
// Bad: Heap allocation
auto* buffer = new uint8_t[1024];
// ... use buffer
delete[] buffer;

// Good: Stack allocation
std::array<uint8_t, 1024> buffer;
// ... use buffer
```

#### 3. Inline Small Functions
```cpp
// Add inline hint for small functions
inline uint64_t add_fees(uint64_t amount, uint64_t fee_rate) {
    return amount + (amount * fee_rate / 10000);
}
```

## Debugging Techniques

### Logging

```cpp
void debug_function() {
    system::log("Entering debug_function");
    
    auto value = get_some_value();
    system::log("Value: " + std::to_string(value));
    
    if (value > 100) {
        system::log("Value exceeds threshold");
    }
    
    system::log("Exiting debug_function");
}
```

### State Inspection

```cpp
void dump_contract_state() {
    system::log("=== Contract State Dump ===");
    
    // Iterate through state
    auto it = system::get_object_iterator(state_space());
    while (it.has_next()) {
        auto [key, value] = it.next();
        system::log("Key: " + key + ", Size: " + 
                   std::to_string(value.size()));
    }
}
```

### Error Handling

```cpp
enum class error_code : uint32_t {
    success = 0,
    insufficient_balance = 1,
    unauthorized = 2,
    invalid_argument = 3
};

void handle_operation() {
    try {
        perform_operation();
    } catch (const insufficient_balance_error& e) {
        system::fail("Insufficient balance: " + e.what(), 
                    error_code::insufficient_balance);
    } catch (const authorization_error& e) {
        system::fail("Unauthorized: " + e.what(), 
                    error_code::unauthorized);
    } catch (const std::exception& e) {
        system::fail("Unexpected error: " + e.what());
    }
}
```

## Gas Optimization

### Measuring Gas Usage

```cpp
class GasProfiler {
private:
    uint64_t start_gas;
    std::string operation_name;

public:
    GasProfiler(const std::string& name) : operation_name(name) {
        start_gas = system::get_remaining_gas();
    }
    
    ~GasProfiler() {
        uint64_t gas_used = start_gas - system::get_remaining_gas();
        system::log(operation_name + " used " + 
                   std::to_string(gas_used) + " gas");
    }
};

void expensive_operation() {
    GasProfiler profiler("expensive_operation");
    // ... perform operation
}
```

### Gas-Efficient Patterns

```cpp
// Use batch operations
void batch_transfer(const std::vector<Transfer>& transfers) {
    // Single authorization check
    if (!system::check_authority(sender)) {
        system::fail("Unauthorized");
    }
    
    // Batch state updates
    std::map<std::string, balance_object> balances;
    
    // Load all balances once
    for (const auto& transfer : transfers) {
        if (balances.find(transfer.to) == balances.end()) {
            system::get_object(balance_space(), transfer.to, 
                              balances[transfer.to]);
        }
    }
    
    // Apply all transfers
    for (const auto& transfer : transfers) {
        balances[transfer.to].amount += transfer.amount;
    }
    
    // Save all balances once
    for (const auto& [account, balance] : balances) {
        system::put_object(balance_space(), account, balance);
    }
}
```

## Security Best Practices

### 1. Input Validation

```cpp
void validate_transfer(const std::string& from,
                      const std::string& to,
                      uint64_t amount) {
    // Check address format
    if (from.empty() || from.length() > MAX_ADDRESS_LENGTH) {
        system::fail("Invalid from address");
    }
    
    if (to.empty() || to.length() > MAX_ADDRESS_LENGTH) {
        system::fail("Invalid to address");
    }
    
    // Check for self-transfer
    if (from == to) {
        system::fail("Cannot transfer to self");
    }
    
    // Check amount
    if (amount == 0) {
        system::fail("Amount must be positive");
    }
    
    if (amount > MAX_TRANSFER_AMOUNT) {
        system::fail("Amount exceeds maximum");
    }
}
```

### 2. Reentrancy Protection

```cpp
class ReentrancyGuard {
private:
    static bool locked;

public:
    ReentrancyGuard() {
        if (locked) {
            system::fail("Reentrancy detected");
        }
        locked = true;
    }
    
    ~ReentrancyGuard() {
        locked = false;
    }
};

void protected_function() {
    ReentrancyGuard guard;
    // Function body protected against reentrancy
}
```

### 3. Integer Overflow Protection

```cpp
#include <boost/multiprecision/cpp_int.hpp>

using safe_uint64 = boost::multiprecision::checked_uint64_t;

void safe_arithmetic() {
    try {
        safe_uint64 a = UINT64_MAX;
        safe_uint64 b = 1;
        safe_uint64 result = a + b;  // Throws on overflow
    } catch (const std::overflow_error& e) {
        system::fail("Integer overflow detected");
    }
}
```

## Common Patterns

### Factory Pattern

```cpp
template<typename T>
class ObjectFactory {
public:
    static T create_default() {
        T obj;
        initialize_defaults(obj);
        return obj;
    }
    
    static T create_from_args(const Arguments& args) {
        T obj;
        initialize_from_args(obj, args);
        return obj;
    }
};
```

### Observer Pattern

```cpp
class EventEmitter {
private:
    std::vector<std::function<void(const Event&)>> listeners;

public:
    void subscribe(std::function<void(const Event&)> listener) {
        listeners.push_back(listener);
    }
    
    void emit(const Event& event) {
        for (const auto& listener : listeners) {
            listener(event);
        }
    }
};
```

### Singleton Pattern

```cpp
class ContractConfig {
private:
    static ContractConfig* instance;
    ContractConfig() = default;

public:
    static ContractConfig& get_instance() {
        if (!instance) {
            instance = new ContractConfig();
        }
        return *instance;
    }
};
```

## Troubleshooting

### Common Issues and Solutions

| Issue | Solution |
|-------|----------|
| "Unknown entry point" | Verify entry point hash matches |
| "Authorization failure" | Check signer and authority |
| "Insufficient resources" | Increase gas limit or optimize code |
| "Serialization error" | Verify ABI matches implementation |
| "State not found" | Initialize state before reading |

### Debug Checklist

1. ✓ Environment variables set correctly
2. ✓ Contract compiles without warnings
3. ✓ ABI matches contract implementation
4. ✓ Entry points correctly defined
5. ✓ State initialization handled
6. ✓ Authorization checks in place
7. ✓ Error handling implemented
8. ✓ Gas usage within limits
9. ✓ Tests passing
10. ✓ Security review completed

## Resources

### Documentation
- [Koinos Developer Documentation](https://docs.koinos.io)
- [Protocol Documentation](https://github.com/koinos/koinos-proto)
- [SDK Reference](https://github.com/koinos/koinos-sdk-cpp)

### Tools
- [Koinos CLI](https://github.com/koinos/koinos-cli)
- [Block Explorer](https://explorer.koinos.io)
- [Testnet Faucet](https://faucet.koinos.io)

### Community
- [Discord](https://discord.koinos.io)
- [Forum](https://forum.koinos.io)
- [GitHub](https://github.com/koinos)