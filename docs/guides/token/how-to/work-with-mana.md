# How to Work with Mana

Understanding and managing KOIN's unique mana system.

## What is Mana?

Mana is your spendable KOIN balance that regenerates over time. Think of it as a rechargeable battery:
- **Mana ≤ Balance**: Can never exceed your KOIN balance
- **Regenerates**: Fully recharges in 5 days
- **Required for**: All transfers and burns

## Check Available Mana

```javascript
const { Contract, Provider } = require('koilib');

async function checkMana(address) {
  const provider = new Provider('https://api.koinos.io');
  
  // Get account RC (mana) from chain
  const result = await provider.call('get_account_rc', {
    account: address
  });
  
  const mana = parseInt(result.value) / 100000000;
  
  // Also get balance for comparison
  const koin = new Contract({
    id: '15DJN4a8SgrbGhhGksSBASiSYjGnMU8dGL',
    provider
  });
  
  const { result: balanceResult } = await koin.functions.balance_of({
    owner: address
  });
  
  const balance = parseInt(balanceResult.value) / 100000000;
  
  console.log(`Balance: ${balance} KOIN`);
  console.log(`Mana: ${mana} KOIN (${(mana/balance*100).toFixed(1)}%)`);
  
  return { balance, mana };
}

checkMana('1FaSvLjQJsCJKq5ybmGsMMQs8RQYyVv8ju');
```

## Calculate Mana Regeneration

```javascript
function calculateManaRegeneration(currentMana, maxBalance, hoursElapsed) {
  const REGEN_DAYS = 5;
  const REGEN_HOURS = REGEN_DAYS * 24;
  
  // Mana regenerates linearly
  const regenRate = maxBalance / REGEN_HOURS;
  const regenAmount = regenRate * hoursElapsed;
  
  // Cannot exceed balance
  const newMana = Math.min(currentMana + regenAmount, maxBalance);
  
  return {
    newMana,
    regenPerHour: regenRate,
    hoursToFull: (maxBalance - currentMana) / regenRate
  };
}

// Example: 50 KOIN mana, 100 KOIN balance, 12 hours passed
const result = calculateManaRegeneration(50, 100, 12);
console.log(`New mana: ${result.newMana} KOIN`);
console.log(`Full in: ${result.hoursToFull.toFixed(1)} hours`);
```

## Wait for Mana to Regenerate

```javascript
async function waitForMana(address, requiredMana) {
  const provider = new Provider('https://api.koinos.io');
  
  while (true) {
    // Check current mana
    const rc = await provider.call('get_account_rc', {
      account: address
    });
    
    const currentMana = parseInt(rc.value) / 100000000;
    
    if (currentMana >= requiredMana) {
      console.log(`✓ Mana available: ${currentMana} KOIN`);
      return true;
    }
    
    // Estimate wait time
    const deficit = requiredMana - currentMana;
    const hoursToWait = (deficit / balance) * 120; // 5 days = 120 hours
    
    console.log(`Waiting... Current: ${currentMana}, Need: ${requiredMana}`);
    console.log(`Estimated time: ${hoursToWait.toFixed(1)} hours`);
    
    // Check again in 5 minutes
    await new Promise(resolve => setTimeout(resolve, 300000));
  }
}
```

## Optimize Mana Usage

```javascript
async function smartTransfer(privateKey, transfers) {
  const provider = new Provider('https://api.koinos.io');
  const signer = new Signer(privateKey, provider);
  
  // Check available mana
  const rc = await provider.call('get_account_rc', {
    account: signer.address
  });
  
  const availableMana = parseInt(rc.value);
  
  // Sort transfers by priority
  transfers.sort((a, b) => b.priority - a.priority);
  
  const executed = [];
  let usedMana = 0;
  
  for (const transfer of transfers) {
    const amount = transfer.amount * 100000000;
    
    if (usedMana + amount <= availableMana) {
      // Execute transfer
      const koin = new Contract({
        id: '15DJN4a8SgrbGhhGksSBASiSYjGnMU8dGL',
        provider,
        signer
      });
      
      await koin.functions.transfer({
        from: signer.address,
        to: transfer.to,
        value: amount.toString()
      });
      
      executed.push(transfer);
      usedMana += amount;
    } else {
      console.log(`Skipped transfer to ${transfer.to} - insufficient mana`);
    }
  }
  
  return executed;
}
```

## Mana vs Balance Examples

| Scenario | Balance | Mana | Can Transfer? |
|----------|---------|------|---------------|
| Fresh wallet | 100 KOIN | 100 KOIN | ✓ 100 KOIN |
| After sending 40 | 60 KOIN | 60 KOIN | ✓ 60 KOIN |
| Received 50 more | 110 KOIN | 60 KOIN | ✓ 60 KOIN only |
| After 2.5 days | 110 KOIN | 85 KOIN | ✓ 85 KOIN |
| After 5 days | 110 KOIN | 110 KOIN | ✓ 110 KOIN |

## Common Pitfalls

⚠️ **Mana ≠ Balance**: After receiving KOIN, mana doesn't instantly increase
⚠️ **No mana transfer**: You can't transfer mana separately from KOIN
⚠️ **Governance exception**: Governance address has unlimited mana

## Minimal C++ Reference

```cpp
// Check mana from smart contract
uint64_t get_available_mana(const std::string& account) {
    // Get account RC (mana)
    auto rc_result = system::call(
        "get_account_rc",
        account
    );
    
    return rc_result.value;
}
```

---

### How This Relates to Official Docs

**Overlaps:**
- Mana regeneration formula
- RC system basics

**What's complementary here:**
- Practical mana checking
- Regeneration calculations
- Usage optimization strategies

**Links:**
- [Official Mana Documentation](https://docs.koinos.io/architecture/mana/)
- [Resource Credit System](https://docs.koinos.io/architecture/resources/)