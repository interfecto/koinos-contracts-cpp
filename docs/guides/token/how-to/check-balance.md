# How to Check KOIN Balance

Query KOIN balances for any address on the Koinos blockchain.

## Basic Balance Check

```javascript
const { Contract, Provider } = require('koilib');

async function getBalance(address) {
  const provider = new Provider('https://api.koinos.io');
  const koin = new Contract({
    id: '15DJN4a8SgrbGhhGksSBASiSYjGnMU8dGL',
    provider
  });

  const { result } = await koin.functions.balance_of({
    owner: address
  });
  
  // Convert from satoshis (8 decimals)
  return parseInt(result.value) / 100000000;
}

// Usage
getBalance('1FaSvLjQJsCJKq5ybmGsMMQs8RQYyVv8ju')
  .then(balance => console.log(`Balance: ${balance} KOIN`))
  .catch(err => console.error('Failed:', err.message));
```

## Check Multiple Balances

```javascript
async function getMultipleBalances(addresses) {
  const provider = new Provider('https://api.koinos.io');
  const koin = new Contract({
    id: '15DJN4a8SgrbGhhGksSBASiSYjGnMU8dGL',
    provider
  });

  const balances = {};
  
  for (const address of addresses) {
    try {
      const { result } = await koin.functions.balance_of({
        owner: address
      });
      balances[address] = parseInt(result.value) / 100000000;
    } catch (error) {
      balances[address] = 'Error: Invalid address';
    }
  }
  
  return balances;
}

// Example
const addresses = [
  '1FaSvLjQJsCJKq5ybmGsMMQs8RQYyVv8ju',
  '1NsQbH5AhQXgtSNg1ejpFqTi2hmCWz1eQS'
];

getMultipleBalances(addresses).then(console.log);
// Output: { '1FaSv...': 42.5, '1NsQb...': 100.0 }
```

## Format Balance for Display

```javascript
function formatKoin(satoshis) {
  const koin = parseInt(satoshis) / 100000000;
  
  // Format with commas and 2 decimal places
  return new Intl.NumberFormat('en-US', {
    minimumFractionDigits: 2,
    maximumFractionDigits: 8
  }).format(koin);
}

// Usage
const rawBalance = '4250000000'; // 42.5 KOIN in satoshis
console.log(formatKoin(rawBalance)); // "42.50"
```

## Handle Errors Gracefully

```javascript
async function safeGetBalance(address) {
  try {
    // Validate address format
    if (!address || !address.startsWith('1')) {
      throw new Error('Invalid Koinos address format');
    }
    
    const provider = new Provider('https://api.koinos.io');
    const koin = new Contract({
      id: '15DJN4a8SgrbGhhGksSBASiSYjGnMU8dGL',
      provider
    });

    const { result } = await koin.functions.balance_of({
      owner: address
    });
    
    return {
      success: true,
      balance: parseInt(result.value) / 100000000
    };
    
  } catch (error) {
    return {
      success: false,
      error: error.message,
      balance: 0
    };
  }
}
```

## Common Pitfalls

⚠️ **Zero vs Error**: A zero balance is valid - distinguish from errors
⚠️ **Number precision**: Use `parseInt()` or `BigInt` for large balances
⚠️ **Rate limits**: Add delays when checking many addresses

## Minimal C++ Reference

```cpp
// From within a smart contract
#include <koinos/token.hpp>

void check_user_balance() {
    std::string user = "1FaSvLjQJsCJKq5ybmGsMMQs8RQYyVv8ju";
    uint64_t balance = koinos::token::koin().balance_of(user);
    
    // Balance is in satoshis
    system::log("Balance: " + std::to_string(balance));
}
```

## Tips

- Cache balance results for better UX
- Show pending transactions separately
- Consider showing mana alongside balance

---

### How This Relates to Official Docs

**Overlaps:**
- Standard balance_of method usage
- Address format validation

**What's complementary here:**
- Batch balance checking pattern
- Display formatting utilities
- Error handling strategies

**Links:**
- [Official Token Methods](https://docs.koinos.io/architecture/tokens/#methods)
- [Koinos Address Format](https://docs.koinos.io/architecture/addresses/)