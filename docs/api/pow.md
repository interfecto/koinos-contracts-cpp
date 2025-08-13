# Proof of Work (PoW) Contract API Documentation

## Overview

The PoW contract manages the mining-based consensus mechanism for the Koinos blockchain. It validates proof-of-work submissions, adjusts difficulty dynamically, and rewards successful miners with KOIN tokens.

## Contract Details

- **Purpose**: Block production through proof-of-work mining
- **Algorithm**: SHA-256 based mining
- **End Date**: December 31, 2022 (1672531199000 ms)
- **Status**: Historical (PoW phase has ended)

## Entry Points

### `get_difficulty()`

Returns the current mining difficulty metadata.

- **Entry Point**: `0x2e40cb65`
- **Read-only**: Yes
- **Arguments**: None
- **Returns**: `pow::get_difficulty_metadata_result`
  ```cpp
  struct get_difficulty_metadata_result {
    difficulty_metadata value;
  }
  
  struct difficulty_metadata {
    bytes target;                 // 32 bytes - Current target hash
    bytes difficulty;             // 32 bytes - Current difficulty
    uint64 last_block_time;       // Last block timestamp
    uint64 target_block_interval; // Target interval (10s)
  }
  ```

### `process_block_signature(args)`

Validates a proof-of-work submission and processes block production.

- **Entry Point**: Called by system
- **Read-only**: No
- **Authorization**: Kernel mode only
- **Arguments**: `chain::process_block_signature_arguments`
  ```cpp
  struct process_block_signature_arguments {
    bytes digest;      // Block digest
    block_header header;
    bytes signature;   // PoW signature data
  }
  ```
- **Returns**: `chain::process_block_signature_result`
  ```cpp
  struct process_block_signature_result {
    bool value;  // Success status
  }
  ```

## Data Structures

### PoW Signature Data
```cpp
struct pow_signature_data {
  bytes nonce;                  // 32 bytes - Mining nonce
  bytes recoverable_signature;  // 65 bytes - Producer signature
}
```

### Difficulty Metadata
```cpp
struct difficulty_metadata {
  bytes target;                 // 32 bytes - Target hash value
  bytes difficulty;             // 32 bytes - Current difficulty
  uint64 last_block_time;       // Last block timestamp
  uint64 target_block_interval; // Target interval (constant: 10)
}
```

## Mining Algorithm

### Proof-of-Work Validation

1. **Hash Calculation**:
   ```
   pow_hash = SHA256(nonce || block_digest)
   ```

2. **Target Comparison**:
   - PoW hash must be less than or equal to target
   - Target = MAX_UINT256 / difficulty

3. **Signature Verification**:
   - Recover public key from signature
   - Verify signer matches block producer

### Difficulty Adjustment

The difficulty adjusts every block using the following algorithm:

```cpp
difficulty = difficulty + difficulty/2048 * max(1 - (block_time - last_block_time)/7000, -99)
```

**Parameters**:
- Adjustment factor: 1/2048 of current difficulty
- Time window: 7 seconds
- Maximum adjustment: -99x to +1x per block
- Target block time: 10 seconds

## Mining Rewards

- **Block Reward**: 100 KOIN (10,000,000,000 base units)
- **Distribution**: Minted directly to successful miner's address
- **Mechanism**: Automatic KOIN minting upon successful PoW validation

## Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `target_block_interval_s` | 10 | Target seconds between blocks |
| `sha256_id` | 0x12 | SHA-256 algorithm identifier |
| `pow_end_date` | 1672531199000 | PoW end timestamp (Dec 31, 2022) |
| `block_reward` | 10000000000 | 100 KOIN in base units |
| `initial_difficulty_bits` | 24 | Initial difficulty (2^24) |
| `max_signature_size` | 65 | Maximum signature size |
| `max_proof_size` | 128 | Maximum proof size |

## Mining Process

### 1. Block Template Creation
```javascript
const template = {
  header: {
    previous: lastBlockId,
    height: lastHeight + 1,
    timestamp: Date.now(),
    signer: minerAddress
  },
  transactions: [...] // Transaction pool
};
```

### 2. Mining Loop
```javascript
while (!found) {
  nonce = generateNonce();
  hash = sha256(nonce + blockDigest);
  
  if (hash <= target) {
    found = true;
  }
}
```

### 3. Signature Creation
```javascript
const signature = {
  nonce: nonce,
  recoverable_signature: sign(blockDigest, minerKey)
};
```

### 4. Block Submission
```javascript
const result = await submitBlock({
  header: blockHeader,
  signature: signature,
  transactions: transactions
});
```

## Error Messages

| Error | Description |
|-------|-------------|
| "Testnet has ended" | PoW phase completed (after Dec 31, 2022) |
| "PoW contract must be called from kernel" | Invalid calling context |
| "PoW did not meet target" | Hash exceeds difficulty target |
| "Signature and signer are mismatching" | Invalid block producer signature |
| "Could not mint KOIN to producer address" | Reward distribution failure |

## State Management

### Object Spaces
- **Contract Space**: Stores difficulty metadata
  - Zone: Contract ID
  - ID: 0
  - System: true

### Storage Keys
- **Difficulty Metadata**: Empty string key ("")

## Security Considerations

1. **Time-based Termination**: PoW automatically ends at predetermined timestamp
2. **Kernel-only Access**: Only system kernel can invoke block processing
3. **Signature Verification**: Producer identity cryptographically verified
4. **Difficulty Bounds**: Adjustment algorithm prevents extreme swings
5. **Overflow Protection**: Uses boost::multiprecision for safe math

## Mining Statistics

### Difficulty Progression
- **Initial**: 2^24 (~16.7 million)
- **Adjustment Rate**: ~0.05% per block
- **Stabilization**: Converges to network hashrate

### Block Time Analysis
- **Target**: 10 seconds
- **Actual Range**: 1-60 seconds typical
- **Adjustment Response**: ~20 blocks to stabilize

## Implementation Examples

### Mining Client (Pseudocode)
```cpp
class Miner {
  void mine() {
    auto difficulty = getDifficulty();
    auto target = calculateTarget(difficulty);
    
    while (mining) {
      auto nonce = generateNonce();
      auto hash = sha256(nonce + blockDigest);
      
      if (hash <= target) {
        submitBlock(nonce, signature);
        break;
      }
    }
  }
};
```

### Difficulty Query
```javascript
const { Contract } = require('@koinos/sdk-js');

const pow = new Contract({
  id: powContractId,
  abi: powAbi,
  provider
});

const difficulty = await pow.functions.get_difficulty();
console.log('Current target:', difficulty.value.target);
console.log('Difficulty:', difficulty.value.difficulty);
```

## Migration to Proof of Burn

After the PoW end date (December 31, 2022), the network transitioned to Proof of Burn consensus:

1. **PoW Termination**: Contract rejects all mining attempts
2. **Consensus Switch**: Automatic transition to PoB
3. **Block Production**: Validators burn KOIN for block production rights
4. **Rewards**: Block rewards continue through PoB mechanism

## Historical Data

### PoW Phase Statistics
- **Duration**: Genesis to December 31, 2022
- **Total Blocks**: Variable based on network activity
- **Total Rewards**: Blocks × 100 KOIN
- **Final Difficulty**: Network-dependent

## Monitoring and Analysis

### Key Metrics
1. **Hashrate**: `difficulty × 2^32 / block_time`
2. **Block Time Variance**: Standard deviation from 10s target
3. **Difficulty Change Rate**: Per-block adjustment percentage
4. **Miner Distribution**: Unique addresses producing blocks

### Network Health Indicators
- Consistent block times near target
- Gradual difficulty adjustments
- Distributed miner participation
- Successful reward distributions

## Related Contracts

- **KOIN Token**: Receives minting calls for rewards
- **Governance**: May modify consensus parameters
- **Resources**: Allocates block space and compute