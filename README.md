# Koinos System Smart Contracts

Core system contracts for the Koinos blockchain, written in C++.

## Quick Navigation

### 🚀 Getting Started
- [**Quick Start Guide**](docs/guides/token/quickstart.md) - First KOIN interaction in 5 minutes

### 💰 KOIN Token Guides
- [Check Balance](docs/guides/token/how-to/check-balance.md) - Query any address
- [Transfer Tokens](docs/guides/token/how-to/transfer-tokens.md) - Send KOIN safely  
- [Understand Mana](docs/guides/token/how-to/work-with-mana.md) - Manage regenerating resources
- [Listen to Events](docs/guides/token/how-to/listen-to-events.md) - Track transfers in real-time
- [Approve Patterns](docs/guides/token/how-to/approve-and-transferFrom.md) - Authorization alternatives

### 📖 API Reference
- [KOIN Token API](docs/api/koin.md) - Complete method reference
- [PoW Contract API](docs/api/pow.md) - Mining consensus (historical)
- [Resources API](docs/api/resources.md) - Resource management

## Building Contracts

```bash
# Prerequisites
export KOINOS_SDK_ROOT=/path/to/sdk
export KOINOS_WASI_SDK_ROOT=/path/to/wasi

# Build
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=${KOINOS_SDK_ROOT}/cmake/koinos-wasm-toolchain.cmake ..
make -j
```

## Contract Addresses

| Contract | Mainnet/Testnet Address |
|----------|------------------------|
| KOIN Token | `15DJN4a8SgrbGhhGksSBASiSYjGnMU8dGL` |
| Resources | `198RuEouhgiiaQm7uGfaXS6jqZr6g6nyoR` |
| PoW | `18tWNU7E4yuQzz7hMVpceb9ixmaWLVyQsr` |

## Resources

- [Official Koinos Docs](https://docs.koinos.io)
- [Koinos Discord](https://discord.koinos.io)
- [Block Explorer](https://explorer.koinos.io)

## License

MIT - See [LICENSE.md](LICENSE.md)