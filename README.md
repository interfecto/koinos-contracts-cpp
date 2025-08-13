# Koinos System Smart Contracts

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md)
[![C++](https://img.shields.io/badge/language-C%2B%2B17-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/platform-WASM-orange.svg)](https://webassembly.org/)

## Overview

This repository contains the core system smart contracts for the Koinos blockchain, implemented in C++. These contracts provide fundamental blockchain functionality including the native token (KOIN), proof-of-work consensus mechanism, and resource management system.

## Table of Contents

- [Features](#features)
- [Architecture](#architecture)
- [Contracts](#contracts)
- [Building](#building)
- [Installation](#installation)
- [Usage](#usage)
- [Testing](#testing)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [Security](#security)
- [License](#license)

## Features

- **Native Token (KOIN)**: Full ERC-20-compatible token implementation with mana system
- **Proof of Work**: Mining-based consensus with dynamic difficulty adjustment
- **Resource Management**: Three-tier resource system (disk, network, compute)
- **Mana System**: Regenerative resource credits for transaction fees
- **WASM Compilation**: Contracts compile to WebAssembly for cross-platform execution

## Architecture

The Koinos smart contracts follow a modular architecture:

```
koinos-contracts-cpp/
├── contracts/
│   ├── koin/          # Native token implementation
│   ├── pow/           # Proof-of-work consensus
│   ├── resources/     # Resource management
│   └── test utilities/
├── cmake/             # Build configuration
└── ci/                # Continuous integration
```

Each contract is self-contained with:
- Entry point dispatch system
- State management via object spaces
- Authorization checks
- Event emission

## Contracts

### 1. KOIN Token Contract

The native token contract implementing the KOIN cryptocurrency with advanced features:

**Key Features:**
- **Token Standard**: ERC-20 compatible interface
- **Mana System**: Regenerative credits for transaction fees
- **Supply Management**: Mint and burn capabilities
- **Authorization**: Multi-level permission system

**Specifications:**
- Symbol: KOIN (tKOIN in test mode)
- Decimals: 8
- Mana Regeneration: 5 days (432,000,000 ms)
- Maximum Address Size: 25 bytes

### 2. Proof of Work (PoW) Contract

Manages the mining-based consensus mechanism:

**Key Features:**
- **Dynamic Difficulty**: Automatic adjustment based on block time
- **Block Rewards**: 100 KOIN per block
- **Mining Algorithm**: SHA-256 based proof-of-work
- **End Date**: December 31, 2022 (timestamp: 1672531199000)

**Specifications:**
- Target Block Interval: 10 seconds
- Initial Difficulty: 2^24
- Difficulty Adjustment: Per block with dampening factor

### 3. Resources Contract

Manages blockchain resource allocation and consumption:

**Resource Types:**
1. **Disk Storage**: Permanent blockchain storage
   - Budget: 39,600 bytes/block (10GB/month)
   - Maximum: 512KB per block

2. **Network Bandwidth**: Data transmission
   - Budget: 256KB per block
   - Maximum: 1MB per block

3. **Compute Bandwidth**: Computational resources
   - Budget: 57.5M gas (~0.1s)
   - Maximum: 287.5M gas (~0.5s)

**Features:**
- **Market-based Pricing**: Dynamic resource costs
- **RC System**: Resource credits linked to KOIN holdings
- **Regeneration**: 5-day RC regeneration cycle

## Building

### Prerequisites

1. **Koinos CDT (Contract Development Toolkit)**
   ```bash
   git clone https://github.com/koinos/koinos-cdt
   cd koinos-cdt
   # Follow CDT build instructions
   ```

2. **Environment Variables**
   ```bash
   export KOINOS_SDK_ROOT=/path/to/koinos-sdk
   export KOINOS_WASI_SDK_ROOT=/path/to/wasi-sdk
   ```

### Build Instructions

#### Standard Build
```bash
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=${KOINOS_SDK_ROOT}/cmake/koinos-wasm-toolchain.cmake \
      -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

#### Test Build
```bash
cmake -DCMAKE_TOOLCHAIN_FILE=${KOINOS_SDK_ROOT}/cmake/koinos-wasm-toolchain.cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_FOR_TESTING=ON ..
make -j$(nproc)
```

The compiled `.wasm` files will be in the `build/contracts/*/` directories.

## Installation

### Deploying Contracts

1. **Using Koinos CLI**
   ```bash
   koinos-cli upload contract.wasm <contract-address>
   ```

2. **Using Koinos SDK**
   ```javascript
   const { Contract } = require('@koinos/sdk-js');
   const contract = new Contract({
     id: contractAddress,
     abi: contractABI,
     bytecode: wasmBytecode
   });
   await contract.deploy();
   ```

## Usage

### Interacting with KOIN Token

```cpp
// Transfer tokens
token::transfer_arguments args;
args.set_from(sender_address);
args.set_to(recipient_address);
args.set_value(amount);

// Check balance
token::balance_of_arguments balance_args;
balance_args.set_owner(address);
auto result = balance_of(balance_args);
```

### Resource Management

```cpp
// Get resource limits
auto limits = get_resource_limits(parameters);

// Consume resources
consume_block_resources_arguments consume_args;
consume_args.set_disk_storage_consumed(disk_bytes);
consume_args.set_network_bandwidth_consumed(network_bytes);
consume_args.set_compute_bandwidth_consumed(compute_gas);
```

## Testing

### Unit Tests
Tests are integrated with the build system:
```bash
cd build
make test
```

### Integration Tests
Using Docker:
```bash
git clone https://github.com/koinos/koinos-integration-tests.git
cd koinos-integration-tests/tests
./run.sh
```

## Documentation

### API Documentation
- [KOIN Token API](docs/api/koin.md)
- [PoW Consensus API](docs/api/pow.md)
- [Resources API](docs/api/resources.md)

### Developer Guides
- [Contract Development Guide](docs/guides/development.md)
- [Deployment Guide](docs/guides/deployment.md)
- [Security Best Practices](docs/guides/security.md)

### Architecture Documentation
- [System Architecture](docs/architecture/overview.md)
- [State Management](docs/architecture/state.md)
- [Authorization Model](docs/architecture/authorization.md)

## Contributing

We welcome contributions! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details.

### Development Workflow
1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to your fork
5. Submit a pull request

### Code Style
- Follow the existing C++ style
- Use meaningful variable names
- Document complex logic
- Add tests for new features

## Security

### Audit Status
These contracts are critical infrastructure for the Koinos blockchain. Security considerations:

- **Authorization Checks**: All privileged operations verify caller permissions
- **Integer Overflow Protection**: Safe math operations using boost::multiprecision
- **Resource Limits**: Bounded execution with gas metering
- **State Validation**: Comprehensive input validation

### Reporting Security Issues
Please report security vulnerabilities to security@koinos.io

## Technical Specifications

### Performance Metrics
- **Block Time**: 3 seconds
- **Transaction Throughput**: Variable based on resource usage
- **Contract Size**: < 100KB per contract (WASM)

### Dependencies
- Koinos SDK
- WASI SDK
- Boost.Multiprecision
- Protocol Buffers (for ABI)

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.

## Contact

- **Website**: [koinos.io](https://koinos.io)
- **GitHub**: [github.com/koinos](https://github.com/koinos)
- **Discord**: [Koinos Community](https://discord.koinos.io)

---

*Built with ❤️ by the Koinos Group*