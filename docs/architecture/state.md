# State Management Architecture

## Overview

State management in Koinos smart contracts uses a hierarchical object space system that provides isolated, persistent storage for contract data. This document details the architecture, patterns, and best practices for managing state.

## Object Space Model

### Core Concept

Object spaces are isolated storage contexts that organize contract state:

```cpp
struct object_space {
  bytes zone;     // Contract identifier (address)
  uint32 id;      // Space type identifier
  bool system;    // System privilege flag
}
```

### Hierarchy

```
Contract Address (Zone)
         │
         ├── Space 0 (Supply)
         │     └── Key: "" → Total Supply
         │
         ├── Space 1 (Balances)
         │     ├── Key: "address1" → Balance Object
         │     ├── Key: "address2" → Balance Object
         │     └── Key: "addressN" → Balance Object
         │
         └── Space N (Custom)
               └── Key-Value Pairs
```

## State Objects

### KOIN Contract State

#### Supply Object
```cpp
struct balance_object {
  uint64 value;  // Total token supply
}
```

#### Mana Balance Object
```cpp
struct mana_balance_object {
  uint64 balance;           // Token balance
  uint64 mana;              // Available mana
  uint64 last_mana_update;  // Last regeneration timestamp
}
```

### Resources Contract State

#### Resource Markets
```cpp
struct resource_markets {
  market disk_storage;
  market network_bandwidth;
  market compute_bandwidth;
}

struct market {
  uint64 resource_supply;  // Available supply
  uint64 block_budget;     // Target per block
  uint64 block_limit;      // Maximum per block
}
```

#### Resource Parameters
```cpp
struct resource_parameters {
  uint64 block_interval_ms;
  uint64 rc_regen_ms;
  uint64 decay_constant;
  uint64 one_minus_decay_constant;
  uint64 print_rate_premium;
  uint64 print_rate_precision;
}
```

### PoW Contract State

#### Difficulty Metadata
```cpp
struct difficulty_metadata {
  bytes target;                 // 32 bytes
  bytes difficulty;             // 32 bytes
  uint64 last_block_time;
  uint64 target_block_interval;
}
```

## State Access Patterns

### Read Operations

```cpp
// Generic read pattern
template<typename T>
T read_state(const object_space& space, const string& key) {
  T object;
  system::get_object(space, key, object);
  return object;
}

// Specific example
koin::mana_balance_object get_balance(const string& address) {
  koin::mana_balance_object balance;
  system::get_object(state::balance_space(), address, balance);
  return balance;
}
```

### Write Operations

```cpp
// Generic write pattern
template<typename T>
void write_state(const object_space& space, 
                 const string& key, 
                 const T& object) {
  system::put_object(space, key, object);
}

// Specific example
void update_balance(const string& address, 
                   const koin::mana_balance_object& balance) {
  system::put_object(state::balance_space(), address, balance);
}
```

### Delete Operations

```cpp
// Remove state entry
void remove_state(const object_space& space, const string& key) {
  system::remove_object(space, key);
}
```

## State Initialization

### Lazy Initialization Pattern

```cpp
resource_parameters get_resource_parameters() {
  resource_parameters params;
  if (!system::get_object(state::contract_space(), 
                          constants::parameters_key, 
                          params)) {
    // Initialize with defaults
    initialize_params(params);
    // Optionally save initialized state
    system::put_object(state::contract_space(), 
                      constants::parameters_key, 
                      params);
  }
  return params;
}
```

### Static Initialization

```cpp
namespace state {
namespace detail {
  system::object_space create_balance_space() {
    system::object_space space;
    auto contract_id = system::get_contract_id();
    space.mutable_zone().set(
      reinterpret_cast<const uint8_t*>(contract_id.data()), 
      contract_id.size()
    );
    space.set_id(constants::balance_id);
    space.set_system(true);
    return space;
  }
}

const system::object_space& balance_space() {
  static const auto space = detail::create_balance_space();
  return space;
}
}
```

## State Consistency

### Atomic Operations

All state changes within a transaction are atomic:

```cpp
void transfer(const string& from, const string& to, uint64 amount) {
  // Read states
  auto from_balance = get_balance(from);
  auto to_balance = get_balance(to);
  
  // Validate
  if (from_balance.balance < amount) {
    system::fail("Insufficient balance");
  }
  
  // Update states atomically
  from_balance.balance -= amount;
  to_balance.balance += amount;
  
  // Write states
  update_balance(from, from_balance);
  update_balance(to, to_balance);
  
  // If any operation fails, all changes are reverted
}
```

### State Validation

```cpp
void validate_and_update_state(const market& m) {
  // Validate invariants
  assert(m.block_budget <= m.block_limit);
  assert(m.resource_supply > 0);
  
  // Check business rules
  if (m.resource_supply < m.block_budget) {
    system::revert("Invalid market state");
  }
  
  // Safe to update
  system::put_object(state::contract_space(), "market", m);
}
```

## State Migration

### Version Management

```cpp
struct versioned_state {
  uint32 version;
  bytes data;
};

void migrate_state_if_needed() {
  versioned_state state;
  system::get_object(space, key, state);
  
  if (state.version < CURRENT_VERSION) {
    switch(state.version) {
      case 1:
        migrate_v1_to_v2(state);
        [[fallthrough]];
      case 2:
        migrate_v2_to_v3(state);
        [[fallthrough]];
      // ... continue for all versions
    }
    state.version = CURRENT_VERSION;
    system::put_object(space, key, state);
  }
}
```

### Schema Evolution

```cpp
// Version 1
struct balance_v1 {
  uint64 amount;
};

// Version 2 - Added timestamp
struct balance_v2 {
  uint64 amount;
  uint64 last_updated;
};

// Migration function
balance_v2 migrate_balance(const balance_v1& old) {
  balance_v2 new_balance;
  new_balance.amount = old.amount;
  new_balance.last_updated = system::get_head_info().head_block_time();
  return new_balance;
}
```

## Performance Optimization

### Caching Strategies

```cpp
class StateCache {
private:
  mutable std::map<string, koin::mana_balance_object> balance_cache;
  mutable bool cache_dirty = false;

public:
  const koin::mana_balance_object& get_balance(const string& address) const {
    auto it = balance_cache.find(address);
    if (it != balance_cache.end()) {
      return it->second;
    }
    
    koin::mana_balance_object balance;
    system::get_object(state::balance_space(), address, balance);
    balance_cache[address] = balance;
    return balance_cache[address];
  }
  
  void update_balance(const string& address, 
                     const koin::mana_balance_object& balance) {
    balance_cache[address] = balance;
    cache_dirty = true;
  }
  
  void flush() {
    if (!cache_dirty) return;
    
    for (const auto& [address, balance] : balance_cache) {
      system::put_object(state::balance_space(), address, balance);
    }
    cache_dirty = false;
  }
};
```

### Batch Operations

```cpp
void batch_update_balances(const vector<pair<string, uint64>>& updates) {
  // Read all balances first
  map<string, koin::mana_balance_object> balances;
  for (const auto& [address, _] : updates) {
    balances[address] = get_balance(address);
  }
  
  // Apply updates
  for (const auto& [address, amount] : updates) {
    balances[address].balance += amount;
    balances[address].mana += amount;
  }
  
  // Write all at once
  for (const auto& [address, balance] : balances) {
    update_balance(address, balance);
  }
}
```

## State Size Management

### Efficient Storage Patterns

```cpp
// Compact representation
struct compact_balance {
  uint64 packed_data;  // Lower 40 bits: balance, Upper 24 bits: flags
  
  uint64 get_balance() const {
    return packed_data & 0xFFFFFFFFFF;
  }
  
  void set_balance(uint64 balance) {
    packed_data = (packed_data & 0xFFFFFF0000000000) | 
                  (balance & 0xFFFFFFFFFF);
  }
};
```

### State Pruning

```cpp
void prune_zero_balances() {
  // Iterate through balance space
  auto it = system::get_object_iterator(state::balance_space());
  
  while (it.has_next()) {
    auto [key, value] = it.next();
    koin::mana_balance_object balance;
    deserialize(value, balance);
    
    if (balance.balance == 0 && balance.mana == 0) {
      system::remove_object(state::balance_space(), key);
    }
  }
}
```

## Concurrency Considerations

### Read-Write Patterns

```cpp
// Read-heavy optimization
class ReadOptimizedState {
private:
  mutable shared_mutex mutex;
  map<string, balance> balances;

public:
  balance read_balance(const string& address) const {
    shared_lock lock(mutex);
    return balances.at(address);
  }
  
  void write_balance(const string& address, const balance& bal) {
    unique_lock lock(mutex);
    balances[address] = bal;
  }
};
```

### State Partitioning

```cpp
// Partition state by address prefix for parallel access
class PartitionedState {
private:
  static constexpr int PARTITIONS = 16;
  array<object_space, PARTITIONS> partitions;
  
  int get_partition(const string& key) {
    return hash(key) % PARTITIONS;
  }

public:
  void put(const string& key, const balance& value) {
    int partition = get_partition(key);
    system::put_object(partitions[partition], key, value);
  }
  
  balance get(const string& key) {
    int partition = get_partition(key);
    balance value;
    system::get_object(partitions[partition], key, value);
    return value;
  }
};
```

## State Recovery

### Checkpoint System

```cpp
struct state_checkpoint {
  uint64 block_height;
  bytes state_root;
  map<string, bytes> critical_state;
};

void create_checkpoint() {
  state_checkpoint checkpoint;
  checkpoint.block_height = system::get_head_info().head_topology().height();
  
  // Save critical state
  checkpoint.critical_state["total_supply"] = serialize(get_total_supply());
  checkpoint.critical_state["markets"] = serialize(get_markets());
  
  system::put_object(checkpoint_space(), 
                    to_string(checkpoint.block_height), 
                    checkpoint);
}
```

### Recovery Procedures

```cpp
void recover_from_checkpoint(uint64 block_height) {
  state_checkpoint checkpoint;
  if (!system::get_object(checkpoint_space(), 
                          to_string(block_height), 
                          checkpoint)) {
    system::fail("Checkpoint not found");
  }
  
  // Restore state
  for (const auto& [key, value] : checkpoint.critical_state) {
    if (key == "total_supply") {
      restore_total_supply(deserialize<uint64>(value));
    } else if (key == "markets") {
      restore_markets(deserialize<resource_markets>(value));
    }
  }
}
```

## Best Practices

### 1. Minimize State Reads
```cpp
// Bad: Multiple reads
auto balance1 = get_balance(address);
auto balance2 = get_balance(address);  // Redundant read

// Good: Single read
auto balance = get_balance(address);
// Use balance multiple times
```

### 2. Batch State Updates
```cpp
// Bad: Individual updates
for (const auto& address : addresses) {
  auto balance = get_balance(address);
  balance.mana += 1;
  update_balance(address, balance);
}

// Good: Batched updates
map<string, koin::mana_balance_object> updates;
for (const auto& address : addresses) {
  updates[address] = get_balance(address);
  updates[address].mana += 1;
}
for (const auto& [address, balance] : updates) {
  update_balance(address, balance);
}
```

### 3. Use Appropriate Data Structures
```cpp
// For frequent lookups: Use indexed storage
map<string, balance> indexed_balances;

// For sequential access: Use array storage
vector<balance> sequential_balances;

// For sparse data: Use optional storage
optional<special_data> sparse_data;
```

### 4. Implement State Invariants
```cpp
class InvariantProtectedState {
private:
  void check_invariants() const {
    assert(total_minted >= total_burned);
    assert(sum_of_balances == total_supply);
  }

public:
  void update_state() {
    // Perform update
    // ...
    
    // Verify invariants
    check_invariants();
  }
};
```

## Debugging State Issues

### State Inspection Tools

```cpp
void dump_state_debug() {
  system::log("=== State Dump ===");
  
  // Dump supply
  auto supply = get_total_supply();
  system::log("Total Supply: " + to_string(supply));
  
  // Dump balances
  auto it = system::get_object_iterator(state::balance_space());
  while (it.has_next()) {
    auto [key, value] = it.next();
    koin::mana_balance_object balance;
    deserialize(value, balance);
    system::log("Balance[" + key + "]: " + to_string(balance.balance));
  }
}
```

### State Verification

```cpp
bool verify_state_consistency() {
  uint64 calculated_supply = 0;
  
  // Sum all balances
  auto it = system::get_object_iterator(state::balance_space());
  while (it.has_next()) {
    auto [_, value] = it.next();
    koin::mana_balance_object balance;
    deserialize(value, balance);
    calculated_supply += balance.balance;
  }
  
  // Compare with stored supply
  auto stored_supply = get_total_supply();
  return calculated_supply == stored_supply;
}
```