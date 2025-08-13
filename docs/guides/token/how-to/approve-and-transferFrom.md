# Approve and TransferFrom in KOIN

Understanding KOIN's authorization model vs traditional ERC-20 tokens.

## Important: No Allowance System

Unlike ERC-20 tokens, KOIN **does not have** `approve()` or `transferFrom()` methods. There's no allowance system where you pre-approve spending limits for other addresses.

## How KOIN Authorization Works

KOIN uses Koinos's native authorization system. Only two ways to move tokens:

1. **Direct transfer**: The owner signs the transaction
2. **Contract authority**: Smart contracts can transfer their own KOIN

```javascript
// ❌ This DOES NOT exist in KOIN
await koin.functions.approve({ spender: '...', value: '...' });
await koin.functions.transferFrom({ from: '...', to: '...', value: '...' });

// ✅ This is how KOIN works
await koin.functions.transfer({ 
  from: signerAddress,  // Must match the signer
  to: recipientAddress,
  value: amount 
});
```

## Alternative: Multi-Signature Wallets

For shared control, use a multi-signature contract:

```javascript
// Deploy a multi-sig contract that holds KOIN
const multiSig = new Contract({
  id: 'YOUR_MULTISIG_CONTRACT_ADDRESS',
  provider,
  signer
});

// Multiple signers approve a transaction
await multiSig.functions.submitTransaction({
  to: recipientAddress,
  value: amount,
  data: transferData
});

await multiSig.functions.confirmTransaction({
  transactionId: txId
});
```

## Alternative: Smart Contract Escrow

Create an escrow contract for conditional transfers:

```javascript
// Deploy an escrow contract
const escrow = new Contract({
  id: 'YOUR_ESCROW_CONTRACT',
  provider,
  signer
});

// User sends KOIN to escrow
await koin.functions.transfer({
  from: signer.address,
  to: escrow.id,
  value: amount
});

// Escrow releases funds when conditions are met
await escrow.functions.releaseFunds({
  recipient: beneficiary,
  condition: proofData
});
```

## Pattern: Delegated Operations

For recurring payments or automated transfers, use a smart contract:

```cpp
// C++ Smart contract that manages KOIN on behalf of users
class PaymentProcessor {
private:
    struct Subscription {
        std::string recipient;
        uint64_t amount;
        uint64_t interval;
        uint64_t next_payment;
    };
    
    std::map<std::string, Subscription> subscriptions;

public:
    void create_subscription(
        const std::string& recipient,
        uint64_t amount,
        uint64_t interval
    ) {
        // User must first transfer KOIN to this contract
        // Then contract can manage recurring payments
        
        if (!system::check_authority(msg.sender)) {
            system::fail("Unauthorized");
        }
        
        subscriptions[msg.sender] = {
            recipient,
            amount,
            interval,
            get_block_timestamp() + interval
        };
    }
    
    void process_payment(const std::string& subscriber) {
        auto& sub = subscriptions[subscriber];
        
        if (get_block_timestamp() >= sub.next_payment) {
            // Contract transfers its own KOIN
            koinos::token::koin().transfer(
                system::get_contract_id(),  // from: this contract
                sub.recipient,
                sub.amount
            );
            
            sub.next_payment += sub.interval;
        }
    }
};
```

## Common Use Cases and Solutions

### DEX Trading
```javascript
// DEX contracts hold KOIN and execute trades
// Users transfer KOIN to DEX, then DEX manages it
await koin.functions.transfer({
  from: signer.address,
  to: dexContract,
  value: amount
});

// DEX executes the trade
await dex.functions.swap({
  tokenIn: 'KOIN',
  tokenOut: 'OTHER',
  amountIn: amount
});
```

### Payment Streaming
```javascript
// Stream contract receives lump sum, releases over time
await koin.functions.transfer({
  from: signer.address,
  to: streamContract,
  value: totalAmount
});

await stream.functions.createStream({
  recipient: beneficiary,
  rate: amountPerSecond,
  duration: streamDuration
});
```

## Migration from ERC-20 Patterns

| ERC-20 Pattern | KOIN Equivalent |
|---------------|-----------------|
| `approve() + transferFrom()` | Use multi-sig or escrow contract |
| Unlimited allowance | Not supported - use contract custody |
| Allowance queries | Check contract balance instead |
| Delegated transfers | Smart contract intermediary |

## Common Pitfalls

⚠️ **No approval transactions**: Don't look for approve() - it doesn't exist
⚠️ **Trust requirement**: Smart contracts must hold tokens directly
⚠️ **Different mental model**: Think "custody" not "allowance"

---

### How This Relates to Official Docs

**Overlaps:**
- Koinos authorization system
- Smart contract patterns

**What's complementary here:**
- Clear explanation of what's NOT available
- Migration guide from ERC-20
- Practical alternatives

**Links:**
- [Koinos Authorization System](https://docs.koinos.io/architecture/authorization/)
- [Multi-Signature Contracts](https://docs.koinos.io/developers/guides/multisig/)
- [Smart Contract Development](https://docs.koinos.io/developers/guides/contract-development/)