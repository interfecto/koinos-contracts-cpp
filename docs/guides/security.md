# Koinos Smart Contract Security Best Practices

## Overview

This guide provides comprehensive security guidelines for developing, auditing, and maintaining secure smart contracts on the Koinos blockchain. Security is paramount in blockchain development as contracts are immutable and handle valuable assets.

## Security Principles

### Core Security Tenets

1. **Defense in Depth**: Multiple layers of security
2. **Least Privilege**: Minimal permissions required
3. **Fail Secure**: Safe defaults on failure
4. **Zero Trust**: Verify all inputs and callers
5. **Transparency**: Open source and auditable

## Common Vulnerabilities

### 1. Integer Overflow/Underflow

**Vulnerability**: Arithmetic operations exceeding type limits

```cpp
// VULNERABLE CODE
uint64_t add_unchecked(uint64_t a, uint64_t b) {
    return a + b;  // Can overflow
}

uint64_t subtract_unchecked(uint64_t a, uint64_t b) {
    return a - b;  // Can underflow
}
```

**SECURE CODE**
```cpp
#include <boost/multiprecision/cpp_int.hpp>

using safe_uint64 = boost::multiprecision::checked_uint64_t;

uint64_t add_safe(uint64_t a, uint64_t b) {
    try {
        safe_uint64 result = safe_uint64(a) + safe_uint64(b);
        return result.convert_to<uint64_t>();
    } catch (const std::overflow_error& e) {
        system::fail("Integer overflow detected");
    }
}

// Alternative: Manual checking
uint64_t add_checked(uint64_t a, uint64_t b) {
    if (a > UINT64_MAX - b) {
        system::fail("Addition would overflow");
    }
    return a + b;
}
```

### 2. Reentrancy Attacks

**Vulnerability**: External calls allowing recursive entry

```cpp
// VULNERABLE CODE
void withdraw(uint64_t amount) {
    if (balances[msg.sender] >= amount) {
        // External call before state update - VULNERABLE!
        send_tokens(msg.sender, amount);
        balances[msg.sender] -= amount;
    }
}
```

**SECURE CODE**
```cpp
// Using Checks-Effects-Interactions pattern
void withdraw_secure(uint64_t amount) {
    // Checks
    if (balances[msg.sender] < amount) {
        system::fail("Insufficient balance");
    }
    
    // Effects (state changes first)
    balances[msg.sender] -= amount;
    
    // Interactions (external calls last)
    send_tokens(msg.sender, amount);
}

// Using reentrancy guard
class ReentrancyGuard {
private:
    static thread_local bool locked;

public:
    ReentrancyGuard() {
        if (locked) {
            system::fail("Reentrancy detected");
        }
        locked = true;
    }
    
    ~ReentrancyGuard() {
        locked = false;
    }
};

void withdraw_with_guard(uint64_t amount) {
    ReentrancyGuard guard;
    // Function body protected
    // ...
}
```

### 3. Access Control Vulnerabilities

**Vulnerability**: Missing or incorrect authorization checks

```cpp
// VULNERABLE CODE
void admin_function() {
    // No authorization check!
    perform_admin_action();
}
```

**SECURE CODE**
```cpp
// Role-based access control
class AccessControl {
private:
    std::map<std::string, std::set<std::string>> roles;
    
public:
    void require_role(const std::string& role) {
        auto caller = system::get_caller().first;
        if (roles[role].find(caller) == roles[role].end()) {
            system::fail("Unauthorized: missing role " + role);
        }
    }
    
    void grant_role(const std::string& role, const std::string& account) {
        require_role("admin");
        roles[role].insert(account);
        emit_role_granted(role, account);
    }
};

void admin_function_secure() {
    access_control.require_role("admin");
    perform_admin_action();
}

// Multi-signature requirement
void critical_function() {
    std::vector<std::string> signers = {
        "1ABC...",
        "1DEF...",
        "1GHI..."
    };
    
    uint32_t confirmations = 0;
    for (const auto& signer : signers) {
        if (system::check_authority(signer)) {
            confirmations++;
        }
    }
    
    if (confirmations < 2) {
        system::fail("Requires 2 of 3 signatures");
    }
    
    perform_critical_action();
}
```

### 4. Front-Running Attacks

**Vulnerability**: Transaction ordering manipulation

```cpp
// VULNERABLE CODE
void buy_token(uint64_t amount) {
    // Price can be manipulated by front-running
    uint64_t price = get_current_price();
    uint64_t cost = amount * price;
    
    transfer_payment(cost);
    mint_tokens(msg.sender, amount);
}
```

**SECURE CODE**
```cpp
// Commit-reveal scheme
struct Commitment {
    bytes32 hash;
    uint64_t block_number;
    bool revealed;
};

std::map<std::string, Commitment> commitments;

void commit_buy(const bytes32& commitment_hash) {
    commitments[msg.sender] = {
        commitment_hash,
        get_block_number(),
        false
    };
}

void reveal_buy(uint64_t amount, uint64_t nonce) {
    auto& commitment = commitments[msg.sender];
    
    // Verify commitment age (prevent front-running)
    if (get_block_number() < commitment.block_number + MIN_BLOCKS) {
        system::fail("Too early to reveal");
    }
    
    // Verify commitment hash
    bytes32 expected = hash(msg.sender + amount + nonce);
    if (commitment.hash != expected) {
        system::fail("Invalid reveal");
    }
    
    // Process buy at committed price
    uint64_t price = get_price_at_block(commitment.block_number);
    process_buy(amount, price);
    
    commitment.revealed = true;
}
```

### 5. Denial of Service (DoS)

**Vulnerability**: Operations that can block contract functionality

```cpp
// VULNERABLE CODE
void distribute_rewards() {
    // Unbounded loop - can exceed gas limit
    for (const auto& user : all_users) {
        send_reward(user, calculate_reward(user));
    }
}
```

**SECURE CODE**
```cpp
// Pagination to prevent gas exhaustion
void distribute_rewards_paginated(uint32_t start, uint32_t count) {
    require_role("distributor");
    
    uint32_t end = std::min(start + count, uint32_t(all_users.size()));
    
    for (uint32_t i = start; i < end; i++) {
        send_reward(all_users[i], calculate_reward(all_users[i]));
    }
    
    last_distribution_index = end;
}

// Pull pattern instead of push
std::map<std::string, uint64_t> pending_rewards;

void claim_reward() {
    uint64_t reward = pending_rewards[msg.sender];
    if (reward == 0) {
        system::fail("No pending reward");
    }
    
    // Clear before sending (prevent reentrancy)
    pending_rewards[msg.sender] = 0;
    
    send_tokens(msg.sender, reward);
}
```

## Input Validation

### Comprehensive Validation

```cpp
class InputValidator {
public:
    static void validate_address(const std::string& address) {
        if (address.empty()) {
            system::fail("Address cannot be empty");
        }
        
        if (address.length() > MAX_ADDRESS_LENGTH) {
            system::fail("Address too long");
        }
        
        if (!is_valid_base58(address)) {
            system::fail("Invalid address format");
        }
        
        if (is_blacklisted(address)) {
            system::fail("Address is blacklisted");
        }
    }
    
    static void validate_amount(uint64_t amount) {
        if (amount == 0) {
            system::fail("Amount must be positive");
        }
        
        if (amount > MAX_TRANSFER_AMOUNT) {
            system::fail("Amount exceeds maximum");
        }
    }
    
    static void validate_percentage(uint32_t percentage) {
        if (percentage > 10000) { // 100.00%
            system::fail("Percentage exceeds 100%");
        }
    }
    
    static void validate_timestamp(uint64_t timestamp) {
        uint64_t current = get_block_timestamp();
        
        // Prevent past timestamps
        if (timestamp < current) {
            system::fail("Timestamp in the past");
        }
        
        // Prevent too far future
        if (timestamp > current + MAX_FUTURE_TIME) {
            system::fail("Timestamp too far in future");
        }
    }
    
    static void validate_array_size(size_t size) {
        if (size > MAX_ARRAY_SIZE) {
            system::fail("Array too large");
        }
    }
};
```

## State Management Security

### Secure State Patterns

```cpp
// Atomic state updates
class SecureState {
private:
    struct StateSnapshot {
        std::map<std::string, uint64_t> balances;
        uint64_t total_supply;
        uint64_t timestamp;
    };
    
    StateSnapshot current_state;
    
public:
    void atomic_update(std::function<void(StateSnapshot&)> update) {
        // Create backup
        StateSnapshot backup = current_state;
        
        try {
            // Apply update
            update(current_state);
            
            // Validate invariants
            if (!validate_invariants()) {
                throw std::runtime_error("Invariant violation");
            }
            
            // Commit to storage
            commit_state();
            
        } catch (const std::exception& e) {
            // Rollback on any error
            current_state = backup;
            system::fail("State update failed: " + std::string(e.what()));
        }
    }
    
private:
    bool validate_invariants() {
        // Check: sum of balances equals total supply
        uint64_t sum = 0;
        for (const auto& [addr, balance] : current_state.balances) {
            if (sum > UINT64_MAX - balance) {
                return false; // Overflow
            }
            sum += balance;
        }
        
        return sum == current_state.total_supply;
    }
};
```

## Cryptographic Security

### Secure Random Number Generation

```cpp
// INSECURE: Predictable randomness
uint64_t insecure_random() {
    return get_block_timestamp() % 100;  // Predictable!
}

// SECURE: Commit-reveal randomness
class SecureRandom {
private:
    struct CommitData {
        bytes32 commitment;
        uint64_t block_number;
    };
    
    std::map<std::string, CommitData> commits;
    std::vector<bytes32> reveals;
    
public:
    void commit_random(const bytes32& commitment) {
        commits[msg.sender] = {
            commitment,
            get_block_number()
        };
    }
    
    void reveal_random(const std::string& value, const std::string& nonce) {
        auto& commit = commits[msg.sender];
        
        // Verify timing
        if (get_block_number() < commit.block_number + REVEAL_BLOCKS) {
            system::fail("Too early to reveal");
        }
        
        // Verify commitment
        bytes32 hash = sha256(value + nonce);
        if (hash != commit.commitment) {
            system::fail("Invalid reveal");
        }
        
        reveals.push_back(hash);
    }
    
    uint64_t generate_random() {
        if (reveals.size() < MIN_REVEALS) {
            system::fail("Insufficient entropy");
        }
        
        // Combine all reveals with block hash
        bytes32 seed = get_block_hash(get_block_number() - 1);
        for (const auto& reveal : reveals) {
            seed = sha256(seed + reveal);
        }
        
        return uint64_from_bytes(seed);
    }
};
```

### Signature Verification

```cpp
class SignatureVerifier {
public:
    static bool verify_signature(
        const std::string& message,
        const std::string& signature,
        const std::string& public_key
    ) {
        // Verify signature length
        if (signature.length() != SIGNATURE_LENGTH) {
            return false;
        }
        
        // Recover public key from signature
        auto recovered_key = system::recover_public_key(signature, message);
        
        // Compare with expected key
        return recovered_key == public_key;
    }
    
    static bool verify_multisig(
        const std::string& message,
        const std::vector<std::string>& signatures,
        const std::vector<std::string>& public_keys,
        uint32_t threshold
    ) {
        uint32_t valid_sigs = 0;
        std::set<std::string> used_keys;
        
        for (const auto& sig : signatures) {
            for (const auto& key : public_keys) {
                // Skip if key already used
                if (used_keys.count(key) > 0) {
                    continue;
                }
                
                if (verify_signature(message, sig, key)) {
                    valid_sigs++;
                    used_keys.insert(key);
                    break;
                }
            }
            
            if (valid_sigs >= threshold) {
                return true;
            }
        }
        
        return false;
    }
};
```

## Time-based Security

### Secure Time Locks

```cpp
class TimeLock {
private:
    static constexpr uint64_t MIN_DELAY = 86400000; // 24 hours
    static constexpr uint64_t MAX_DELAY = 2592000000; // 30 days
    
    struct PendingAction {
        std::function<void()> action;
        uint64_t execute_time;
        bool executed;
    };
    
    std::map<uint64_t, PendingAction> pending_actions;
    uint64_t next_action_id = 1;
    
public:
    uint64_t schedule_action(
        std::function<void()> action,
        uint64_t delay
    ) {
        // Validate delay
        if (delay < MIN_DELAY) {
            system::fail("Delay too short");
        }
        if (delay > MAX_DELAY) {
            system::fail("Delay too long");
        }
        
        uint64_t execute_time = get_block_timestamp() + delay;
        uint64_t action_id = next_action_id++;
        
        pending_actions[action_id] = {
            action,
            execute_time,
            false
        };
        
        emit_action_scheduled(action_id, execute_time);
        
        return action_id;
    }
    
    void execute_action(uint64_t action_id) {
        auto it = pending_actions.find(action_id);
        if (it == pending_actions.end()) {
            system::fail("Action not found");
        }
        
        auto& pending = it->second;
        
        if (pending.executed) {
            system::fail("Action already executed");
        }
        
        if (get_block_timestamp() < pending.execute_time) {
            system::fail("Action not ready");
        }
        
        // Mark as executed before calling (reentrancy protection)
        pending.executed = true;
        
        // Execute the action
        pending.action();
        
        emit_action_executed(action_id);
    }
    
    void cancel_action(uint64_t action_id) {
        require_role("admin");
        
        auto it = pending_actions.find(action_id);
        if (it == pending_actions.end()) {
            system::fail("Action not found");
        }
        
        if (it->second.executed) {
            system::fail("Cannot cancel executed action");
        }
        
        pending_actions.erase(it);
        emit_action_cancelled(action_id);
    }
};
```

## Emergency Procedures

### Circuit Breakers

```cpp
class CircuitBreaker {
private:
    bool paused = false;
    uint64_t pause_end_time = 0;
    std::set<std::string> pausers;
    
public:
    void require_not_paused() {
        if (paused) {
            if (get_block_timestamp() < pause_end_time) {
                system::fail("Contract is paused");
            }
            // Auto-unpause after timeout
            paused = false;
            pause_end_time = 0;
        }
    }
    
    void emergency_pause(uint64_t duration) {
        // Require multiple pausers for activation
        auto caller = get_caller();
        pausers.insert(caller);
        
        if (pausers.size() >= REQUIRED_PAUSERS) {
            paused = true;
            pause_end_time = get_block_timestamp() + duration;
            
            // Max pause duration: 7 days
            if (duration > 604800000) {
                pause_end_time = get_block_timestamp() + 604800000;
            }
            
            emit_emergency_pause(pause_end_time);
            pausers.clear();
        }
    }
    
    void unpause() {
        require_role("admin");
        require_multisig(2, 3);
        
        paused = false;
        pause_end_time = 0;
        emit_unpause();
    }
};

// Use in functions
void critical_function() {
    circuit_breaker.require_not_paused();
    // Function logic
}
```

### Rate Limiting

```cpp
class RateLimiter {
private:
    struct UserLimit {
        uint64_t last_action;
        uint32_t action_count;
    };
    
    std::map<std::string, UserLimit> user_limits;
    
    static constexpr uint64_t TIME_WINDOW = 3600000; // 1 hour
    static constexpr uint32_t MAX_ACTIONS = 10;
    
public:
    void check_rate_limit(const std::string& user) {
        auto& limit = user_limits[user];
        uint64_t current_time = get_block_timestamp();
        
        // Reset if outside time window
        if (current_time > limit.last_action + TIME_WINDOW) {
            limit.action_count = 0;
            limit.last_action = current_time;
        }
        
        // Check limit
        if (limit.action_count >= MAX_ACTIONS) {
            system::fail("Rate limit exceeded");
        }
        
        limit.action_count++;
    }
    
    void check_value_limit(const std::string& user, uint64_t value) {
        static constexpr uint64_t DAILY_LIMIT = 1000000000; // 10 KOIN
        static std::map<std::string, uint64_t> daily_totals;
        
        // Reset daily totals at midnight
        if (is_new_day()) {
            daily_totals.clear();
        }
        
        if (daily_totals[user] + value > DAILY_LIMIT) {
            system::fail("Daily limit exceeded");
        }
        
        daily_totals[user] += value;
    }
};
```

## Audit Checklist

### Pre-Audit Preparation

```markdown
## Security Audit Checklist

### Access Control
- [ ] All admin functions protected
- [ ] Role-based access implemented
- [ ] Multi-sig for critical operations
- [ ] Ownership transfer mechanism secure

### Input Validation
- [ ] All external inputs validated
- [ ] Array bounds checked
- [ ] Integer overflow protection
- [ ] Address validation

### State Management
- [ ] Atomic state updates
- [ ] Invariants maintained
- [ ] No uninitialized storage
- [ ] State consistency checks

### External Calls
- [ ] Reentrancy protection
- [ ] Check-effects-interactions pattern
- [ ] Return values checked
- [ ] Gas limits considered

### Cryptography
- [ ] Secure randomness
- [ ] Proper signature verification
- [ ] No hardcoded secrets
- [ ] Hash functions used correctly

### Error Handling
- [ ] All errors handled
- [ ] Fail securely
- [ ] Clear error messages
- [ ] No sensitive data in errors

### Gas Optimization
- [ ] No unbounded loops
- [ ] Storage access minimized
- [ ] Batch operations where possible
- [ ] Dead code removed

### Emergency Procedures
- [ ] Pause mechanism tested
- [ ] Upgrade path documented
- [ ] Recovery procedures defined
- [ ] Incident response plan

### Documentation
- [ ] Code comments complete
- [ ] API documented
- [ ] Security considerations noted
- [ ] Known limitations listed

### Testing
- [ ] Unit tests complete
- [ ] Integration tests passing
- [ ] Fuzz testing performed
- [ ] Edge cases covered
```

## Security Tools

### Static Analysis

```bash
# Slither - Static analyzer
slither contracts/ --print human-summary

# MythX - Security analysis platform
mythx analyze contracts/

# Echidna - Fuzzing tool
echidna-test contracts/Token.sol --contract Token
```

### Dynamic Analysis

```javascript
// Security test suite
describe('Security Tests', () => {
    test('Integer overflow protection', async () => {
        const maxUint64 = '18446744073709551615';
        
        await expect(
            contract.add(maxUint64, '1')
        ).rejects.toThrow('Integer overflow');
    });
    
    test('Reentrancy protection', async () => {
        const maliciousContract = await deployMalicious();
        
        await expect(
            maliciousContract.attack(contract.address)
        ).rejects.toThrow('Reentrancy detected');
    });
    
    test('Access control enforcement', async () => {
        const unauthorizedSigner = new Signer(randomKey);
        
        await expect(
            contract.connect(unauthorizedSigner).adminFunction()
        ).rejects.toThrow('Unauthorized');
    });
});
```

## Incident Response

### Response Plan Template

```markdown
## Incident Response Plan

### 1. Detection
- Monitor alerts triggered
- Verify incident severity
- Assess potential impact

### 2. Containment
- Pause affected contracts
- Prevent further damage
- Preserve evidence

### 3. Investigation
- Analyze root cause
- Identify affected users
- Calculate losses

### 4. Recovery
- Deploy fixes
- Restore service
- Compensate users if needed

### 5. Post-Incident
- Document lessons learned
- Update security measures
- Communicate with community
```

## Conclusion

Security is an ongoing process, not a destination. Regular audits, continuous monitoring, and staying updated with the latest security practices are essential for maintaining secure smart contracts on the Koinos blockchain.