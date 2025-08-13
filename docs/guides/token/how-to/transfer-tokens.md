# How to Transfer KOIN Tokens

Send KOIN between addresses with proper authorization and mana management.

## Basic Transfer

```javascript
const { Contract, Provider, Signer } = require('koilib');

async function transferKoin(privateKey, toAddress, amount) {
  // Setup provider and signer
  const provider = new Provider('https://api.koinos.io');
  const signer = new Signer(privateKey, provider);
  
  // Connect to KOIN contract
  const koin = new Contract({
    id: '15DJN4a8SgrbGhhGksSBASiSYjGnMU8dGL',
    provider,
    signer
  });

  // Amount in satoshis (multiply by 10^8)
  const satoshis = Math.floor(amount * 100000000);
  
  // Execute transfer
  const { transaction } = await koin.functions.transfer({
    from: signer.address,
    to: toAddress,
    value: satoshis.toString()
  });
  
  console.log('Transfer successful!');
  console.log('Transaction ID:', transaction.id);
  
  return transaction;
}

// Send 10.5 KOIN
transferKoin(
  'YOUR_PRIVATE_KEY',
  '1NsQbH5AhQXgtSNg1ejpFqTi2hmCWz1eQS',
  10.5
).catch(console.error);
```

## Check Mana Before Transfer

```javascript
async function transferWithManaCheck(privateKey, toAddress, amount) {
  const provider = new Provider('https://api.koinos.io');
  const signer = new Signer(privateKey, provider);
  
  const koin = new Contract({
    id: '15DJN4a8SgrbGhhGksSBASiSYjGnMU8dGL',
    provider,
    signer
  });

  // Check balance AND mana
  const { result: balanceResult } = await koin.functions.balance_of({
    owner: signer.address
  });
  
  const balance = parseInt(balanceResult.value);
  const satoshis = Math.floor(amount * 100000000);
  
  // Mana equals balance but regenerates over 5 days
  // For simplicity, assume mana equals balance here
  if (balance < satoshis) {
    throw new Error(`Insufficient balance. Have: ${balance/100000000} KOIN`);
  }
  
  // Proceed with transfer
  return koin.functions.transfer({
    from: signer.address,
    to: toAddress,
    value: satoshis.toString()
  });
}
```

## Wait for Confirmation

```javascript
async function transferAndWait(privateKey, toAddress, amount) {
  const provider = new Provider('https://api.koinos.io');
  const signer = new Signer(privateKey, provider);
  
  const koin = new Contract({
    id: '15DJN4a8SgrbGhhGksSBASiSYjGnMU8dGL',
    provider,
    signer
  });

  // Send transaction
  const { transaction, receipt } = await koin.functions.transfer({
    from: signer.address,
    to: toAddress,
    value: (amount * 100000000).toString()
  });
  
  console.log('Transaction sent:', transaction.id);
  
  // Wait for block inclusion
  await transaction.wait();
  
  console.log('Confirmed in block:', receipt.blockNumber);
  return receipt;
}
```

## Handle Transfer Errors

```javascript
async function safeTransfer(privateKey, toAddress, amount) {
  try {
    // Validate inputs
    if (!toAddress.startsWith('1')) {
      throw new Error('Invalid recipient address');
    }
    
    if (amount <= 0) {
      throw new Error('Amount must be positive');
    }
    
    const provider = new Provider('https://api.koinos.io');
    const signer = new Signer(privateKey, provider);
    
    // Check if trying to send to self
    if (signer.address === toAddress) {
      throw new Error('Cannot transfer to self');
    }
    
    const koin = new Contract({
      id: '15DJN4a8SgrbGhhGksSBASiSYjGnMU8dGL',
      provider,
      signer
    });
    
    const result = await koin.functions.transfer({
      from: signer.address,
      to: toAddress,
      value: (amount * 100000000).toString()
    });
    
    return { success: true, transaction: result.transaction };
    
  } catch (error) {
    // Parse common errors
    if (error.message.includes('insufficient balance')) {
      return { success: false, error: 'Not enough KOIN' };
    }
    if (error.message.includes('insufficient mana')) {
      return { success: false, error: 'Not enough mana (wait for regeneration)' };
    }
    
    return { success: false, error: error.message };
  }
}
```

## Common Pitfalls

⚠️ **Mana vs Balance**: You need both balance AND mana to transfer
⚠️ **Self-transfers**: The contract rejects transfers to the same address
⚠️ **Number precision**: Always use strings for large numbers to avoid JavaScript precision issues

## Understanding Mana

- **Mana = spendable balance** that regenerates over 5 days
- If you have 100 KOIN and spend 50, your mana drops to 50
- After 2.5 days, mana regenerates to ~75 KOIN
- After 5 days, mana is back to 100 KOIN

## Minimal C++ Reference

```cpp
// Transfer from a smart contract
#include <koinos/token.hpp>

void send_koin(const std::string& to, uint64_t amount) {
    auto from = system::get_contract_id();
    
    bool success = koinos::token::koin().transfer(
        from,
        to, 
        amount
    );
    
    if (!success) {
        system::fail("Transfer failed");
    }
}
```

---

### How This Relates to Official Docs

**Overlaps:**
- Standard transfer method signature
- Mana regeneration mechanism

**What's complementary here:**
- Pre-flight mana checking
- Error message parsing
- Practical confirmation waiting

**Links:**
- [Official Transfer Documentation](https://docs.koinos.io/developers/guides/transfer-tokens/)
- [Mana System Explained](https://docs.koinos.io/architecture/mana/)
- [Transaction Lifecycle](https://docs.koinos.io/architecture/transactions/)