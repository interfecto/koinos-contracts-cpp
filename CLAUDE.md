# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview
This repository contains the Koinos blockchain system smart contracts written in C++. These contracts include critical system components like the native token (KOIN), proof-of-work consensus, and resource management.

## Build Commands

### Prerequisites
Before building, ensure these environment variables are set:
- `KOINOS_SDK_ROOT` - Path to the Koinos SDK
- `KOINOS_WASI_SDK_ROOT` - Path to the WASI SDK

### Building Contracts
```bash
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=${KOINOS_SDK_ROOT}/cmake/koinos-wasm-toolchain.cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
```

### Building for Testing
```bash
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_FOR_TESTING=ON ..
make -j
```

## Architecture

### Contract Structure
Each contract is a self-contained directory under `contracts/` with:
- `.cpp` source file implementing the contract logic
- `CMakeLists.txt` for build configuration
- `.abi` files (where applicable) defining the contract interface

### Key Contracts

1. **koin** - The native KOIN token contract
   - Implements ERC-20-like token functionality
   - Manages mana regeneration (5-day cycle)
   - Handles minting, burning, and transfers
   - Test mode uses "tKOIN" symbol when built with BUILD_FOR_TESTING

2. **pow** - Proof-of-Work consensus contract
   - Manages mining difficulty adjustments
   - Handles block rewards (100 KOIN per block)
   - Target block interval: 10 seconds
   - PoW end date: December 31, 2022 (timestamp: 1672531199000)

3. **resources** - Resource management system
   - Tracks three resource types: disk, network, compute
   - Default budgets per block:
     - Disk: 39,600 bytes (10GB/month)
     - Network: 256KB
     - Compute: 57.5M gas (~0.1s)
   - RC regeneration time: 5 days

4. **add_thunk** - Test utility contract
5. **call_nop** - Test utility contract
6. **failures** - Test utility contract

### Common Patterns

All contracts follow similar patterns:
- Use Koinos SDK system calls via `koinos/system/system_calls.hpp`
- Define entry points as enum values
- Use object spaces for state management
- Implement authorization checks
- Use boost::multiprecision for large integer math
- Maximum buffer sizes typically 2048 bytes

### State Management
Contracts use object spaces with:
- Zone: Contract ID
- ID: Unique identifier for the space type
- System flag: Set to true for system contracts

## Testing

Integration tests are run via Docker:
```bash
cd koinos-integration-tests/tests
./run.sh
```

## Important Constants

- Mana regeneration: 432,000,000 ms (5 days)
- Block interval: 3,000 ms (3 seconds)
- Maximum address size: 25 bytes
- Maximum buffer size: 2048 bytes
- KOIN decimals: 8