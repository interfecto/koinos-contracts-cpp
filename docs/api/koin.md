# KOIN Token Contract API Documentation

## Overview

The KOIN token contract is the native cryptocurrency implementation for the Koinos blockchain. It provides standard token functionality with an innovative mana system for resource management.

## Contract Address

- **Mainnet**: `1FaSvLjQJsCJKq5ybmGsMMQs8RQYyVv8ju`
- **Testnet**: `1FaSvLjQJsCJKq5ybmGsMMQs8RQYyVv8ju` (uses tKOIN symbol)

## Entry Points

### Token Information Methods

#### `name()` 
Returns the token name.

- **Entry Point**: `0x82a3537f`
- **Read-only**: Yes
- **Arguments**: None
- **Returns**: `token::name_result`
  ```cpp
  struct name_result {
    string value;  // "Koin" or "Test Koin"
  }
  ```

#### `symbol()`
Returns the token symbol.

- **Entry Point**: `0xb76a7ca1`
- **Read-only**: Yes
- **Arguments**: None
- **Returns**: `token::symbol_result`
  ```cpp
  struct symbol_result {
    string value;  // "KOIN" or "tKOIN"
  }
  ```

#### `decimals()`
Returns the token decimal precision.

- **Entry Point**: `0xee80fd2f`
- **Read-only**: Yes
- **Arguments**: None
- **Returns**: `token::decimals_result`
  ```cpp
  struct decimals_result {
    uint32 value;  // 8
  }
  ```

#### `total_supply()`
Returns the total token supply.

- **Entry Point**: `0xb0da3934`
- **Read-only**: Yes
- **Arguments**: None
- **Returns**: `token::total_supply_result`
  ```cpp
  struct total_supply_result {
    uint64 value;  // Current total supply
  }
  ```

### Balance Operations

#### `balance_of(owner)`
Returns the token balance for an address.

- **Entry Point**: `0x5c721497`
- **Read-only**: Yes
- **Arguments**: `token::balance_of_arguments`
  ```cpp
  struct balance_of_arguments {
    bytes owner;  // Address to check (max 25 bytes)
  }
  ```
- **Returns**: `token::balance_of_result`
  ```cpp
  struct balance_of_result {
    uint64 value;  // Balance amount
  }
  ```

### Token Operations

#### `transfer(from, to, value)`
Transfers tokens between addresses.

- **Entry Point**: `0x27f576ca`
- **Read-only**: No
- **Arguments**: `token::transfer_arguments`
  ```cpp
  struct transfer_arguments {
    bytes from;   // Sender address (max 25 bytes)
    bytes to;     // Recipient address (max 25 bytes)
    uint64 value; // Amount to transfer
  }
  ```
- **Returns**: `token::transfer_result` (empty)
- **Authorization**: Requires authorization from `from` address
- **Events**: Emits `koinos.contracts.token.transfer_event`
- **Errors**:
  - "cannot transfer to self"
  - "from has not authorized transfer"
  - "account 'from' has insufficient balance"
  - "account 'from' has insufficient mana for transfer"

#### `mint(to, value)`
Creates new tokens and assigns them to an address.

- **Entry Point**: `0xdc6f17bb`
- **Read-only**: No
- **Arguments**: `token::mint_arguments`
  ```cpp
  struct mint_arguments {
    bytes to;     // Recipient address (max 25 bytes)
    uint64 value; // Amount to mint
  }
  ```
- **Returns**: `token::mint_result` (empty)
- **Authorization**: 
  - Production: Requires kernel mode
  - Testing: Requires contract authority
- **Events**: Emits `koinos.contracts.token.mint_event`
- **Errors**:
  - "can only mint token from kernel context" (production)
  - "can only mint token with contract authority" (testing)
  - "mint would overflow supply"

#### `burn(from, value)`
Destroys tokens from an address.

- **Entry Point**: `0x859facc5`
- **Read-only**: No
- **Arguments**: `token::burn_arguments`
  ```cpp
  struct burn_arguments {
    bytes from;   // Address to burn from (max 25 bytes)
    uint64 value; // Amount to burn
  }
  ```
- **Returns**: `token::burn_result` (empty)
- **Authorization**: Requires authorization from `from` address
- **Events**: Emits `koinos.contracts.token.burn_event`
- **Errors**:
  - "from has not authorized burn"
  - "account 'from' has insufficient balance"
  - "account 'from' has insufficient mana for burn"
  - "burn would underflow supply"

### Mana System Methods

#### `get_account_rc(account)`
Returns the current mana (resource credits) for an account.

- **Entry Point**: `0x2d464aab`
- **Read-only**: Yes
- **Arguments**: `chain::get_account_rc_arguments`
  ```cpp
  struct get_account_rc_arguments {
    string account;  // Account name (max 32 chars)
  }
  ```
- **Returns**: `chain::get_account_rc_result`
  ```cpp
  struct get_account_rc_result {
    uint64 value;  // Current mana amount
  }
  ```
- **Note**: Governance address returns max uint64 value

#### `consume_account_rc(account, value)`
Consumes mana from an account.

- **Entry Point**: `0x80e3f5c9`
- **Read-only**: No
- **Arguments**: `chain::consume_account_rc_arguments`
  ```cpp
  struct consume_account_rc_arguments {
    string account;  // Account name (max 32 chars)
    uint64 value;    // Mana to consume
  }
  ```
- **Returns**: `chain::consume_account_rc_result`
  ```cpp
  struct consume_account_rc_result {
    bool value;  // Success status
  }
  ```
- **Authorization**: Must be called from kernel mode
- **Errors**:
  - "The system call consume_account_rc must be called from kernel context"
  - "Account has insufficient mana for consumption"

### Authorization

#### `authorize()`
Checks system authority.

- **Entry Point**: `0x4a2dbd90`
- **Read-only**: Yes
- **Arguments**: None
- **Returns**: `chain::authorize_result`
  ```cpp
  struct authorize_result {
    bool value;  // Authorization status
  }
  ```

## Data Structures

### Balance Object
Stored in the balance object space:
```cpp
struct mana_balance_object {
  uint64 balance;           // Token balance
  uint64 mana;              // Current mana
  uint64 last_mana_update;  // Last regeneration timestamp
}
```

### Supply Object
Stored in the supply object space:
```cpp
struct balance_object {
  uint64 value;  // Total supply
}
```

## Events

### Transfer Event
```cpp
struct transfer_event {
  bytes from;    // Sender address
  bytes to;      // Recipient address
  uint64 value;  // Transfer amount
}
```

### Mint Event
```cpp
struct mint_event {
  bytes to;      // Recipient address
  uint64 value;  // Minted amount
}
```

### Burn Event
```cpp
struct burn_event {
  bytes from;    // Source address
  uint64 value;  // Burned amount
}
```

## Mana System Details

### Regeneration
- **Rate**: Mana regenerates linearly over 5 days
- **Formula**: `new_mana = current_mana + (delta_time * balance / regen_time)`
- **Maximum**: Mana cannot exceed token balance
- **Auto-regeneration**: Occurs on any balance query or operation

### Usage
- Mana is consumed 1:1 with token transfers
- Mana is required for burns
- Mana regenerates automatically based on token holdings

## Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `koinos_decimals` | 8 | Decimal precision |
| `mana_regen_time_ms` | 432,000,000 | 5 days in milliseconds |
| `max_address_size` | 25 | Maximum address size in bytes |
| `max_name_size` | 32 | Maximum name string length |
| `max_symbol_size` | 8 | Maximum symbol string length |
| `max_buffer_size` | 2048 | Maximum buffer size |

## Error Codes

| Code | Description |
|------|-------------|
| `authorization_failure` | Operation not authorized |
| `revert` | Operation reverted |
| `failure` | General failure |

## Usage Examples

### JavaScript SDK

```javascript
const { Contract, Provider, Signer } = require('@koinos/sdk-js');

// Initialize
const provider = new Provider('https://api.koinos.io');
const signer = new Signer(privateKey, provider);
const koin = new Contract({
  id: '1FaSvLjQJsCJKq5ybmGsMMQs8RQYyVv8ju',
  abi: koinAbi,
  provider,
  signer
});

// Check balance
const balance = await koin.functions.balance_of({
  owner: accountAddress
});

// Transfer tokens
const result = await koin.functions.transfer({
  from: senderAddress,
  to: recipientAddress,
  value: '100000000' // 1 KOIN (8 decimals)
});
```

### Direct RPC Call

```json
{
  "jsonrpc": "2.0",
  "id": 1,
  "method": "chain.read_contract",
  "params": {
    "contract_id": "1FaSvLjQJsCJKq5ybmGsMMQs8RQYyVv8ju",
    "entry_point": "0x5c721497",
    "args": "BASE64_ENCODED_ARGS"
  }
}
```

## Security Considerations

1. **Authorization**: All state-modifying operations require proper authorization
2. **Integer Overflow**: Protected using boost::multiprecision
3. **Mana Requirements**: Prevents spam by requiring mana for operations
4. **Supply Integrity**: Mint/burn operations check for overflow/underflow
5. **Self-transfer Protection**: Prevents transfers to self

## Migration Notes

### From ERC-20
- KOIN includes all standard ERC-20 functions
- Additional mana system for resource management
- No approval/allowance system (use multi-sig instead)

### Test vs Production
- Test mode uses "tKOIN" symbol
- Test mode allows minting with contract authority
- Production requires kernel mode for minting