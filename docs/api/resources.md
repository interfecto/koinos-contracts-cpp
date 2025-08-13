# Resources Contract API Documentation

## Overview

The Resources contract manages the allocation and consumption of blockchain resources using a market-based pricing mechanism. It implements a three-tier resource system with dynamic pricing based on supply and demand.

## Resource Types

### 1. Disk Storage
- **Purpose**: Permanent blockchain storage
- **Budget**: 39,600 bytes/block (10GB/month)
- **Limit**: 512KB per block
- **Use Cases**: State storage, contract deployment

### 2. Network Bandwidth
- **Purpose**: Data transmission across network
- **Budget**: 256KB per block
- **Limit**: 1MB per block
- **Use Cases**: Transaction size, event emissions

### 3. Compute Bandwidth
- **Purpose**: Computational cycles
- **Budget**: 57.5M gas (~0.1s)
- **Limit**: 287.5M gas (~0.5s)
- **Use Cases**: Smart contract execution

## Entry Points

### `get_resource_limits()`

Returns current resource limits and costs.

- **Entry Point**: `0x427a0394`
- **Read-only**: Yes
- **Arguments**: None
- **Returns**: `chain::get_resource_limits_result`
  ```cpp
  struct get_resource_limits_result {
    resource_limits value;
  }
  
  struct resource_limits {
    uint64 disk_storage_limit;      // Max disk bytes available
    uint64 disk_storage_cost;       // RC cost per byte
    uint64 network_bandwidth_limit; // Max network bytes
    uint64 network_bandwidth_cost;  // RC cost per byte
    uint64 compute_bandwidth_limit; // Max compute gas
    uint64 compute_bandwidth_cost;  // RC cost per gas
  }
  ```

### `consume_block_resources(args)`

Consumes resources for block production.

- **Entry Point**: `0x9850b1fd`
- **Read-only**: No
- **Authorization**: Kernel mode only
- **Arguments**: `chain::consume_block_resources_arguments`
  ```cpp
  struct consume_block_resources_arguments {
    uint64 disk_storage_consumed;      // Disk bytes used
    uint64 network_bandwidth_consumed; // Network bytes used
    uint64 compute_bandwidth_consumed; // Compute gas used
  }
  ```
- **Returns**: `chain::consume_block_resources_result`
  ```cpp
  struct consume_block_resources_result {
    bool value;  // Success status
  }
  ```

### `get_resource_markets()`

Returns current market parameters for all resources.

- **Entry Point**: `0xebe9b9e7`
- **Read-only**: Yes
- **Arguments**: None
- **Returns**: `resources::get_resource_markets_result`
  ```cpp
  struct get_resource_markets_result {
    resource_markets value;
  }
  
  struct resource_markets {
    market disk_storage;      // Disk market state
    market network_bandwidth; // Network market state
    market compute_bandwidth; // Compute market state
  }
  
  struct market {
    uint64 resource_supply; // Current supply (exponentially decaying)
    uint64 block_budget;    // Target consumption per block
    uint64 block_limit;     // Maximum consumption per block
  }
  ```

### `set_resource_markets_parameters(args)`

Updates market parameters (governance action).

- **Entry Point**: `0x4b31e959`
- **Read-only**: No
- **Authorization**: System authority required
- **Arguments**: `resources::set_resource_markets_parameters_arguments`
  ```cpp
  struct set_resource_markets_parameters_arguments {
    market_parameters disk_storage;
    market_parameters network_bandwidth;
    market_parameters compute_bandwidth;
  }
  
  struct market_parameters {
    uint64 block_budget;  // New budget
    uint64 block_limit;   // New limit
  }
  ```
- **Returns**: None

### `get_resource_parameters()`

Returns system-wide resource parameters.

- **Entry Point**: `0xf53b5216`
- **Read-only**: Yes
- **Arguments**: None
- **Returns**: `resources::get_resource_parameters_result`
  ```cpp
  struct get_resource_parameters_result {
    resource_parameters value;
  }
  
  struct resource_parameters {
    uint64 block_interval_ms;        // Block time (3000ms)
    uint64 rc_regen_ms;              // RC regeneration (5 days)
    uint64 decay_constant;           // Supply decay factor
    uint64 one_minus_decay_constant; // Complement for calculations
    uint64 print_rate_premium;       // Premium multiplier
    uint64 print_rate_precision;     // Premium divisor
  }
  ```

### `set_resource_parameters(args)`

Updates system parameters (governance action).

- **Entry Point**: `0xa08e6b90`
- **Read-only**: No
- **Authorization**: System authority required
- **Arguments**: `resources::set_resource_parameters_arguments`
  ```cpp
  struct set_resource_parameters_arguments {
    resource_parameters params;  // New parameters
  }
  ```
- **Returns**: None

### `authorize()`

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

## Resource Economics

### Pricing Model

The resource pricing follows a bonding curve model:

```
Price = k / (supply * (supply - consumed))
```

Where:
- `k` = Constant pool factor
- `supply` = Current resource supply
- `consumed` = Resources being purchased

### Supply Dynamics

Each block, the resource supply adjusts:

```cpp
new_supply = (old_supply * decay_constant >> 64) + print_rate - consumed
```

Components:
- **Decay**: Exponential decay towards equilibrium
- **Print Rate**: New resources added per block
- **Consumption**: Resources used in block

### RC (Resource Credits) System

Resource Credits are the currency for resource consumption:

- **Generation**: RC = (KOIN_balance * time) / (5_days * 3_resources)
- **Regeneration**: Linear over 5 days
- **Maximum**: Equal to KOIN balance
- **Usage**: Pays for all three resource types

## Market Parameters

### Default Values

| Parameter | Disk | Network | Compute | Description |
|-----------|------|---------|---------|-------------|
| Budget/Block | 39,600 B | 256 KB | 57.5M gas | Target usage |
| Limit/Block | 512 KB | 1 MB | 287.5M gas | Hard cap |
| Monthly Budget | 10 GB | 7.5 GB | ~250 hrs | Approximate |

### System Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| `block_interval_ms` | 3,000 | 3 second blocks |
| `rc_regen_ms` | 432,000,000 | 5 day regeneration |
| `decay_constant` | 18446596084619782819 | ~1 month half-life |
| `print_rate_premium` | 1688 | 68.8% premium |
| `print_rate_precision` | 1000 | Divisor for premium |

## Resource Calculation Examples

### RC Required for Operations

```javascript
// Calculate RC needed for a transaction
function calculateRC(diskBytes, networkBytes, computeGas) {
  const limits = await resources.get_resource_limits();
  
  const diskRC = diskBytes * limits.disk_storage_cost;
  const networkRC = networkBytes * limits.network_bandwidth_cost;
  const computeRC = computeGas * limits.compute_bandwidth_cost;
  
  return diskRC + networkRC + computeRC;
}
```

### Available Resources for RC

```javascript
// Calculate resources available with given RC
function availableResources(rcBalance) {
  const limits = await resources.get_resource_limits();
  
  return {
    disk: rcBalance / limits.disk_storage_cost,
    network: rcBalance / limits.network_bandwidth_cost,
    compute: rcBalance / limits.compute_bandwidth_cost
  };
}
```

## Usage Patterns

### Transaction Cost Estimation

```javascript
const { Contract } = require('@koinos/sdk-js');

async function estimateTransactionCost(transaction) {
  const resources = new Contract({
    id: resourcesContractId,
    abi: resourcesAbi,
    provider
  });
  
  // Get current resource costs
  const limits = await resources.functions.get_resource_limits();
  
  // Estimate resource usage
  const diskUsage = transaction.stateChanges * 100; // bytes
  const networkUsage = transaction.size; // bytes
  const computeUsage = transaction.operations * 1000; // gas
  
  // Calculate RC cost
  const totalRC = 
    diskUsage * limits.value.disk_storage_cost +
    networkUsage * limits.value.network_bandwidth_cost +
    computeUsage * limits.value.compute_bandwidth_cost;
    
  return totalRC;
}
```

### Market Monitoring

```javascript
async function monitorMarkets() {
  const markets = await resources.functions.get_resource_markets();
  
  // Calculate utilization rates
  const diskUtilization = 
    (markets.value.disk_storage.block_budget / 
     markets.value.disk_storage.resource_supply) * 100;
     
  const networkUtilization = 
    (markets.value.network_bandwidth.block_budget / 
     markets.value.network_bandwidth.resource_supply) * 100;
     
  const computeUtilization = 
    (markets.value.compute_bandwidth.block_budget / 
     markets.value.compute_bandwidth.resource_supply) * 100;
     
  return {
    disk: diskUtilization,
    network: networkUtilization,
    compute: computeUtilization
  };
}
```

## State Management

### Object Spaces
- **Contract Space**: Stores markets and parameters
  - Zone: Contract ID
  - ID: 0
  - System: true

### Storage Keys
- **Markets**: "markets"
- **Parameters**: "parameters"

## Error Messages

| Error | Description |
|-------|-------------|
| "can only set market parameters with system authority" | Unauthorized parameter update |
| "can only set resource parameters with system authority" | Unauthorized parameter update |
| "The system call consume_block_resources must be called from kernel context" | Invalid caller |
| Resource supply/limit exceeded | Block would exceed resource limits |

## Governance Actions

### Adjusting Market Parameters

```javascript
// Governance proposal to adjust disk storage limits
const proposal = {
  operation: 'set_resource_markets_parameters',
  parameters: {
    disk_storage: {
      block_budget: 50000,  // Increase to 50KB
      block_limit: 1048576  // Increase to 1MB
    },
    network_bandwidth: {
      block_budget: 262144,  // Keep at 256KB
      block_limit: 1048576   // Keep at 1MB
    },
    compute_bandwidth: {
      block_budget: 57500000,  // Keep at 57.5M
      block_limit: 287500000   // Keep at 287.5M
    }
  }
};
```

### Modifying System Parameters

```javascript
// Adjust RC regeneration time
const proposal = {
  operation: 'set_resource_parameters',
  parameters: {
    block_interval_ms: 3000,
    rc_regen_ms: 259200000,  // 3 days instead of 5
    decay_constant: 18446596084619782819,
    one_minus_decay_constant: 147989089768795,
    print_rate_premium: 1688,
    print_rate_precision: 1000
  }
};
```

## Integration Examples

### Smart Contract Resource Planning

```cpp
// In your smart contract
void resource_intensive_operation() {
  // Check available resources first
  auto limits = get_resource_limits();
  
  // Estimate operation cost
  uint64_t expected_compute = 10000000; // 10M gas
  uint64_t expected_disk = 1024;        // 1KB
  
  // Verify resources available
  if (limits.compute_bandwidth_limit < expected_compute) {
    system::fail("Insufficient compute resources");
  }
  
  // Perform operation
  // ...
}
```

### Resource-Aware Application Design

```javascript
class ResourceManager {
  constructor(contract) {
    this.contract = contract;
    this.cache = new Map();
  }
  
  async canAffordOperation(operation) {
    const limits = await this.getResourceLimits();
    const cost = this.estimateOperationCost(operation);
    const userRC = await this.getUserRC();
    
    return userRC >= cost;
  }
  
  async batchOperations(operations) {
    // Group operations to optimize resource usage
    const batches = [];
    let currentBatch = [];
    let currentCost = 0;
    
    for (const op of operations) {
      const cost = this.estimateOperationCost(op);
      if (currentCost + cost > MAX_BATCH_COST) {
        batches.push(currentBatch);
        currentBatch = [op];
        currentCost = cost;
      } else {
        currentBatch.push(op);
        currentCost += cost;
      }
    }
    
    if (currentBatch.length > 0) {
      batches.push(currentBatch);
    }
    
    return batches;
  }
}
```

## Performance Optimization

### Best Practices

1. **Batch Operations**: Combine multiple operations to amortize fixed costs
2. **State Caching**: Minimize storage writes by caching in memory
3. **Efficient Algorithms**: Choose O(log n) over O(n) when possible
4. **Resource Monitoring**: Track resource usage in development
5. **Graceful Degradation**: Handle resource exhaustion gracefully

### Resource Profiling

```javascript
class ResourceProfiler {
  async profileTransaction(tx) {
    const before = await resources.get_resource_markets();
    const result = await tx.submit();
    const after = await resources.get_resource_markets();
    
    return {
      disk: before.disk_storage.resource_supply - 
            after.disk_storage.resource_supply,
      network: before.network_bandwidth.resource_supply - 
               after.network_bandwidth.resource_supply,
      compute: before.compute_bandwidth.resource_supply - 
               after.compute_bandwidth.resource_supply
    };
  }
}
```

## Related Systems

- **KOIN Token**: Provides RC through token holdings
- **Governance**: Controls resource parameters
- **Block Production**: Consumes resources per block