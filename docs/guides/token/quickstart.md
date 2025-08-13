# KOIN Token Quick Start

Get up and running with KOIN tokens in under 5 minutes.

## What is KOIN?

KOIN is the native cryptocurrency of the Koinos blockchain. Every KOIN token has "mana" - a regenerating resource that pays for transactions. Think of it like a battery that recharges over 5 days.

## Prerequisites

- Node.js 14+ installed
- A Koinos wallet address (get one at [Koinos CLI](https://github.com/koinos/koinos-cli))
- Basic JavaScript knowledge

## Step 1: Install koilib

```bash
npm install koilib
```

## Step 2: Connect to Koinos

```javascript
const { Contract, Provider } = require('koilib');

// Connect to Koinos (mainnet or testnet)
const provider = new Provider('https://api.koinos.io');

// KOIN contract address (same on mainnet and testnet)
const koinAddress = '15DJN4a8SgrbGhhGksSBASiSYjGnMU8dGL';

// Create contract instance
const koin = new Contract({
  id: koinAddress,
  provider
});
```

## Step 3: Check a Balance

```javascript
async function checkBalance() {
  try {
    // Check balance for any address
    const { result } = await koin.functions.balance_of({
      owner: '1FaSvLjQJsCJKq5ybmGsMMQs8RQYyVv8ju'
    });
    
    // Result is in smallest unit (satoshis)
    // Divide by 10^8 for human-readable KOIN
    const balance = parseInt(result.value) / 100000000;
    console.log(`Balance: ${balance} KOIN`);
    
  } catch (error) {
    console.error('Error:', error.message);
  }
}

checkBalance();
```

### Output Example
```
Balance: 42.5 KOIN
```

## Common Pitfalls

⚠️ **Wrong network endpoint**: Make sure you're using the correct RPC endpoint
- Mainnet: `https://api.koinos.io`  
- Testnet: `https://harbinger-api.koinos.io`

⚠️ **Address format**: Koinos addresses are base58 encoded strings starting with "1"

## What's Next?

- [Check any balance](./how-to/check-balance.md) - Query balances efficiently
- [Transfer tokens](./how-to/transfer-tokens.md) - Send KOIN to others
- [Listen to events](./how-to/listen-to-events.md) - Track transfers in real-time

## Minimal C++ Reference

If you're building a contract that interacts with KOIN:

```cpp
// Check KOIN balance from another contract
#include <koinos/token.hpp>

uint64_t get_koin_balance(const std::string& address) {
    return koinos::token::koin().balance_of(address);
}
```

---

### How This Relates to Official Docs

**Overlaps:**
- Token contract address matches official deployment
- Standard token interface (name, symbol, balance_of)

**What's complementary here:**
- Simplified mana explanation
- Quick copy-paste examples
- Common error solutions

**Links:**
- [Official Koinos Token Documentation](https://docs.koinos.io/architecture/tokens/)
- [Koinos SDK Reference](https://docs.koinos.io/developers/sdk/)
- [Contract Interaction Guide](https://docs.koinos.io/developers/guides/contract-interaction/)