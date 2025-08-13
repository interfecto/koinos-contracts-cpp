# How to Listen to KOIN Events

Track transfers, mints, and burns in real-time.

## Subscribe to Transfer Events

```javascript
const { Contract, Provider } = require('koilib');

async function watchTransfers() {
  const provider = new Provider('https://api.koinos.io');
  const koin = new Contract({
    id: '15DJN4a8SgrbGhhGksSBASiSYjGnMU8dGL',
    provider
  });

  // Subscribe to all transfer events
  koin.events.subscribe('transfer_event', (event) => {
    const from = event.args.from;
    const to = event.args.to;
    const value = parseInt(event.args.value) / 100000000;
    
    console.log(`Transfer: ${from} → ${to}: ${value} KOIN`);
  });
  
  console.log('Watching for transfers...');
}

watchTransfers();
```

## Filter Events for Specific Address

```javascript
async function watchMyTransfers(myAddress) {
  const provider = new Provider('https://api.koinos.io');
  const koin = new Contract({
    id: '15DJN4a8SgrbGhhGksSBASiSYjGnMU8dGL',
    provider
  });

  koin.events.subscribe('transfer_event', (event) => {
    const from = event.args.from;
    const to = event.args.to;
    
    // Only process events involving my address
    if (from === myAddress || to === myAddress) {
      const value = parseInt(event.args.value) / 100000000;
      const direction = from === myAddress ? 'Sent' : 'Received';
      
      console.log(`${direction} ${value} KOIN`);
      console.log(`  Transaction: ${event.transaction.id}`);
      console.log(`  Block: ${event.blockNumber}`);
    }
  });
}

watchMyTransfers('1FaSvLjQJsCJKq5ybmGsMMQs8RQYyVv8ju');
```

## Track All Token Activity

```javascript
async function trackAllActivity() {
  const provider = new Provider('https://api.koinos.io');
  const koin = new Contract({
    id: '15DJN4a8SgrbGhhGksSBASiSYjGnMU8dGL',
    provider
  });

  // Listen to multiple event types
  const events = ['transfer_event', 'mint_event', 'burn_event'];
  
  events.forEach(eventType => {
    koin.events.subscribe(eventType, (event) => {
      handleEvent(eventType, event);
    });
  });
}

function handleEvent(type, event) {
  const value = parseInt(event.args.value) / 100000000;
  
  switch(type) {
    case 'transfer_event':
      console.log(`[TRANSFER] ${event.args.from} → ${event.args.to}: ${value} KOIN`);
      break;
    case 'mint_event':
      console.log(`[MINT] Created ${value} KOIN for ${event.args.to}`);
      break;
    case 'burn_event':
      console.log(`[BURN] Destroyed ${value} KOIN from ${event.args.from}`);
      break;
  }
}
```

## Process Historical Events

```javascript
async function getRecentTransfers(blocks = 100) {
  const provider = new Provider('https://api.koinos.io');
  
  // Get current block height
  const info = await provider.getChainInfo();
  const headBlock = parseInt(info.head_topology.height);
  
  // Query historical events
  const events = await provider.getEvents({
    contract_id: '15DJN4a8SgrbGhhGksSBASiSYjGnMU8dGL',
    event_name: 'transfer_event',
    from_block: headBlock - blocks,
    to_block: headBlock
  });
  
  return events.map(e => ({
    from: e.args.from,
    to: e.args.to,
    amount: parseInt(e.args.value) / 100000000,
    block: e.blockNumber,
    txId: e.transaction.id
  }));
}

// Get last 100 blocks of transfers
getRecentTransfers(100).then(transfers => {
  console.log(`Found ${transfers.length} transfers`);
  transfers.forEach(t => 
    console.log(`${t.from} → ${t.to}: ${t.amount} KOIN`)
  );
});
```

## Event Data Structure

```javascript
// Transfer Event
{
  name: 'transfer_event',
  args: {
    from: '1ABC...',     // Sender address
    to: '1DEF...',       // Recipient address  
    value: '100000000'   // Amount in satoshis
  },
  blockNumber: 12345,
  transaction: {
    id: '0x...',
    payer: '1ABC...'
  }
}

// Mint Event  
{
  name: 'mint_event',
  args: {
    to: '1ABC...',       // Recipient address
    value: '100000000'   // Amount created
  }
}

// Burn Event
{
  name: 'burn_event', 
  args: {
    from: '1ABC...',     // Address burning tokens
    value: '100000000'   // Amount destroyed
  }
}
```

## Common Pitfalls

⚠️ **Missed events**: WebSocket connections can drop - implement reconnection
⚠️ **Event ordering**: Events in same block may arrive out of order
⚠️ **Large values**: Use BigInt for precise calculations with satoshis

## Minimal C++ Reference

```cpp
// Emit custom event from your contract after KOIN interaction
void notify_koin_received(uint64_t amount) {
    // First receive KOIN
    // Then emit your own event
    
    struct koin_received_event {
        std::string contract;
        uint64_t amount;
    };
    
    koin_received_event event{
        system::get_contract_id(),
        amount
    };
    
    system::event("koin.received", event, {});
}
```

---

### How This Relates to Official Docs

**Overlaps:**
- Event subscription patterns
- Event data structures

**What's complementary here:**
- Practical filtering examples
- Historical event queries
- Event type handling

**Links:**
- [Koinos Events System](https://docs.koinos.io/architecture/events/)
- [Event Subscription Guide](https://docs.koinos.io/developers/guides/events/)