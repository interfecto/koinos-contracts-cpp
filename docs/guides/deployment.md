# Koinos Smart Contract Deployment Guide

## Overview

This guide covers the complete deployment process for Koinos smart contracts, from local testing to mainnet deployment, including best practices, security considerations, and monitoring.

## Deployment Environments

### Environment Overview

| Environment | Purpose | Network | Persistence |
|------------|---------|---------|-------------|
| Local | Development | Local node | Temporary |
| Testnet | Testing | Public testnet | Persistent |
| Staging | Pre-production | Private network | Persistent |
| Mainnet | Production | Public mainnet | Permanent |

## Pre-Deployment Checklist

### Code Review Checklist

- [ ] Code reviewed by team
- [ ] Security audit completed
- [ ] Unit tests passing (100% coverage)
- [ ] Integration tests passing
- [ ] Gas optimization verified
- [ ] Documentation complete
- [ ] ABI validated
- [ ] Error handling comprehensive
- [ ] Authorization checks in place
- [ ] No hardcoded values

### Build Verification

```bash
# Clean build
rm -rf build/
mkdir build && cd build

# Production build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=${KOINOS_SDK_ROOT}/cmake/koinos-wasm-toolchain.cmake \
      ..
make

# Verify contract size
ls -lh *.wasm
# Should be < 500KB for optimal performance

# Optimize WASM
wasm-opt -Oz contract.wasm -o contract-optimized.wasm

# Verify optimization
ls -lh contract-optimized.wasm
```

## Local Deployment

### Setting Up Local Node

```bash
# Install Koinos node
git clone https://github.com/koinos/koinos.git
cd koinos
docker-compose up -d

# Wait for node to sync
docker logs -f koinos_node

# Check node status
curl http://localhost:8080/v1/chain/get_chain_info
```

### Deploying to Local Node

```javascript
// deploy-local.js
const { Contract, Provider, Signer } = require('@koinos/sdk-js');
const fs = require('fs');

async function deployLocal() {
    // Connect to local node
    const provider = new Provider('http://localhost:8080');
    
    // Create signer with test account
    const privateKey = 'your-test-private-key';
    const signer = new Signer(privateKey, provider);
    
    // Read contract files
    const wasmBytes = fs.readFileSync('./contract.wasm');
    const abi = JSON.parse(fs.readFileSync('./contract.abi', 'utf8'));
    
    // Generate contract address
    const contractId = generateContractAddress();
    
    // Deploy contract
    const contract = new Contract({
        id: contractId,
        abi: abi,
        provider: provider,
        signer: signer,
        bytecode: wasmBytes
    });
    
    const transaction = await contract.deploy();
    console.log('Contract deployed:', transaction.id);
    console.log('Contract address:', contractId);
    
    // Verify deployment
    const info = await contract.getInfo();
    console.log('Contract info:', info);
    
    return contractId;
}

deployLocal().catch(console.error);
```

## Testnet Deployment

### Testnet Configuration

```javascript
// config/testnet.js
module.exports = {
    network: {
        rpc: 'https://testnet.koinos.io',
        chainId: 'testnet'
    },
    faucet: 'https://faucet.koinos.io',
    explorer: 'https://testnet-explorer.koinos.io'
};
```

### Deployment Script

```javascript
// deploy-testnet.js
const { Contract, Provider, Signer } = require('@koinos/sdk-js');
const config = require('./config/testnet');
const fs = require('fs');

class TestnetDeployer {
    constructor(privateKey) {
        this.provider = new Provider(config.network.rpc);
        this.signer = new Signer(privateKey, this.provider);
    }
    
    async deployContract(wasmPath, abiPath, contractId) {
        console.log('Starting testnet deployment...');
        
        // Load contract files
        const wasmBytes = fs.readFileSync(wasmPath);
        const abi = JSON.parse(fs.readFileSync(abiPath, 'utf8'));
        
        // Check account balance
        const balance = await this.checkBalance();
        console.log('Account balance:', balance, 'KOIN');
        
        if (balance < 10) {
            console.log('Insufficient balance. Requesting from faucet...');
            await this.requestFromFaucet();
        }
        
        // Create contract instance
        const contract = new Contract({
            id: contractId,
            abi: abi,
            provider: this.provider,
            signer: this.signer,
            bytecode: wasmBytes
        });
        
        // Deploy with retry logic
        let attempts = 0;
        const maxAttempts = 3;
        
        while (attempts < maxAttempts) {
            try {
                const tx = await contract.deploy({
                    rcLimit: '10000000',
                    payer: this.signer.getAddress()
                });
                
                console.log('Transaction submitted:', tx.id);
                
                // Wait for confirmation
                const receipt = await this.waitForReceipt(tx.id);
                console.log('Contract deployed successfully!');
                console.log('Receipt:', receipt);
                
                // Verify deployment
                await this.verifyDeployment(contractId);
                
                return receipt;
            } catch (error) {
                attempts++;
                console.error(`Deployment attempt ${attempts} failed:`, error);
                
                if (attempts >= maxAttempts) {
                    throw error;
                }
                
                // Wait before retry
                await new Promise(resolve => setTimeout(resolve, 5000));
            }
        }
    }
    
    async checkBalance() {
        const address = this.signer.getAddress();
        const koin = new Contract({
            id: '1FaSvLjQJsCJKq5ybmGsMMQs8RQYyVv8ju',
            provider: this.provider
        });
        
        const result = await koin.functions.balance_of({ owner: address });
        return parseInt(result.value) / 100000000; // Convert to KOIN
    }
    
    async requestFromFaucet() {
        const address = this.signer.getAddress();
        const response = await fetch(`${config.faucet}/api/faucet`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ address })
        });
        
        if (!response.ok) {
            throw new Error('Faucet request failed');
        }
        
        console.log('Faucet request successful');
        // Wait for transaction to be mined
        await new Promise(resolve => setTimeout(resolve, 10000));
    }
    
    async waitForReceipt(txId, timeout = 30000) {
        const startTime = Date.now();
        
        while (Date.now() - startTime < timeout) {
            try {
                const receipt = await this.provider.getTransactionReceipt(txId);
                if (receipt) {
                    return receipt;
                }
            } catch (error) {
                // Transaction not yet mined
            }
            
            await new Promise(resolve => setTimeout(resolve, 2000));
        }
        
        throw new Error('Transaction timeout');
    }
    
    async verifyDeployment(contractId) {
        console.log('Verifying deployment...');
        
        // Check contract exists
        const accountInfo = await this.provider.getAccountInfo(contractId);
        if (!accountInfo) {
            throw new Error('Contract not found on chain');
        }
        
        console.log('Contract verified on chain');
        console.log('Explorer URL:', `${config.explorer}/address/${contractId}`);
    }
}

// Usage
async function main() {
    const deployer = new TestnetDeployer(process.env.PRIVATE_KEY);
    
    await deployer.deployContract(
        './build/contract.wasm',
        './abi/contract.abi',
        '1YourContractAddress'
    );
}

main().catch(console.error);
```

## Mainnet Deployment

### Pre-Mainnet Requirements

1. **Security Audit**: Professional audit completed
2. **Testnet Validation**: Minimum 2 weeks on testnet
3. **Monitoring Setup**: Alerts and dashboards ready
4. **Rollback Plan**: Emergency procedures documented
5. **Team Sign-off**: All stakeholders approve

### Mainnet Deployment Process

```javascript
// deploy-mainnet.js
const { Contract, Provider, Signer } = require('@koinos/sdk-js');
const readline = require('readline');
const crypto = require('crypto');

class MainnetDeployer {
    constructor() {
        this.provider = new Provider('https://api.koinos.io');
    }
    
    async deploy() {
        // Multi-signature deployment for security
        console.log('=== MAINNET DEPLOYMENT ===');
        console.log('This is a PRODUCTION deployment');
        
        // Require multiple confirmations
        if (!await this.confirmDeployment()) {
            console.log('Deployment cancelled');
            return;
        }
        
        // Load secure credentials
        const credentials = await this.loadSecureCredentials();
        
        // Create deployment transaction
        const deploymentTx = await this.createDeploymentTransaction(credentials);
        
        // Require multi-sig approval
        const approved = await this.getMultiSigApproval(deploymentTx);
        if (!approved) {
            console.log('Multi-sig approval failed');
            return;
        }
        
        // Execute deployment
        const receipt = await this.executeDeployment(deploymentTx);
        
        // Post-deployment verification
        await this.postDeploymentChecks(receipt);
        
        // Archive deployment info
        await this.archiveDeployment(receipt);
        
        console.log('Mainnet deployment complete!');
    }
    
    async confirmDeployment() {
        const rl = readline.createInterface({
            input: process.stdin,
            output: process.stdout
        });
        
        return new Promise(resolve => {
            rl.question('Type "DEPLOY TO MAINNET" to confirm: ', answer => {
                rl.close();
                resolve(answer === 'DEPLOY TO MAINNET');
            });
        });
    }
    
    async loadSecureCredentials() {
        // Load from secure storage (e.g., HSM, AWS KMS)
        // This is a simplified example
        return {
            signers: [
                process.env.MAINNET_SIGNER_1,
                process.env.MAINNET_SIGNER_2,
                process.env.MAINNET_SIGNER_3
            ],
            contractId: process.env.MAINNET_CONTRACT_ID
        };
    }
    
    async createDeploymentTransaction(credentials) {
        const wasmBytes = fs.readFileSync('./build/contract-mainnet.wasm');
        const abi = JSON.parse(fs.readFileSync('./abi/contract.abi', 'utf8'));
        
        // Calculate deployment hash for verification
        const deploymentHash = crypto
            .createHash('sha256')
            .update(wasmBytes)
            .digest('hex');
        
        console.log('Deployment hash:', deploymentHash);
        
        return {
            contractId: credentials.contractId,
            bytecode: wasmBytes,
            abi: abi,
            hash: deploymentHash,
            timestamp: Date.now()
        };
    }
    
    async getMultiSigApproval(deploymentTx) {
        console.log('Requesting multi-sig approval...');
        
        // In production, this would involve secure communication
        // with multiple signers, possibly through a UI
        
        const approvals = [];
        const requiredApprovals = 2;
        
        // Simulate approval process
        for (let i = 0; i < requiredApprovals; i++) {
            const approved = await this.requestApproval(i, deploymentTx);
            if (approved) {
                approvals.push(approved);
            }
        }
        
        return approvals.length >= requiredApprovals;
    }
    
    async executeDeployment(deploymentTx) {
        console.log('Executing deployment...');
        
        const signer = new Signer(
            process.env.DEPLOYMENT_KEY,
            this.provider
        );
        
        const contract = new Contract({
            id: deploymentTx.contractId,
            abi: deploymentTx.abi,
            provider: this.provider,
            signer: signer,
            bytecode: deploymentTx.bytecode
        });
        
        // Deploy with high RC limit for mainnet
        const tx = await contract.deploy({
            rcLimit: '100000000',
            payer: signer.getAddress()
        });
        
        console.log('Transaction ID:', tx.id);
        
        // Wait for confirmation with extended timeout
        const receipt = await this.waitForReceipt(tx.id, 60000);
        
        return receipt;
    }
    
    async postDeploymentChecks(receipt) {
        console.log('Running post-deployment checks...');
        
        const checks = [
            this.verifyContractCode(receipt.contractId),
            this.testBasicFunctionality(receipt.contractId),
            this.verifyPermissions(receipt.contractId),
            this.checkEventEmission(receipt.contractId)
        ];
        
        const results = await Promise.all(checks);
        
        if (results.some(r => !r)) {
            throw new Error('Post-deployment checks failed');
        }
        
        console.log('All checks passed ✓');
    }
    
    async archiveDeployment(receipt) {
        const deploymentInfo = {
            timestamp: new Date().toISOString(),
            contractId: receipt.contractId,
            transactionId: receipt.transactionId,
            blockNumber: receipt.blockNumber,
            deployer: receipt.payer,
            gasUsed: receipt.rcUsed,
            status: 'SUCCESS'
        };
        
        // Save to secure storage
        fs.writeFileSync(
            `./deployments/mainnet-${Date.now()}.json`,
            JSON.stringify(deploymentInfo, null, 2)
        );
        
        console.log('Deployment archived');
    }
}
```

## Contract Upgrade Strategies

### 1. Proxy Pattern

```cpp
// Proxy contract
class ProxyContract {
private:
    std::string implementation_address;
    
public:
    void upgrade(const std::string& new_implementation) {
        // Check admin authorization
        if (!is_admin()) {
            system::fail("Only admin can upgrade");
        }
        
        // Validate new implementation
        if (!validate_implementation(new_implementation)) {
            system::fail("Invalid implementation");
        }
        
        // Update implementation
        implementation_address = new_implementation;
        
        // Emit upgrade event
        emit_upgrade_event(new_implementation);
    }
    
    void delegate_call(uint32_t entry_point, const std::string& args) {
        // Forward call to implementation
        system::call_contract(implementation_address, entry_point, args);
    }
};
```

### 2. Migration Pattern

```javascript
// Migration script
async function migrateContract(oldAddress, newAddress) {
    console.log('Starting contract migration...');
    
    // 1. Deploy new contract
    const newContract = await deployContract(newAddress);
    
    // 2. Pause old contract
    await pauseContract(oldAddress);
    
    // 3. Migrate state
    const state = await exportState(oldAddress);
    await importState(newAddress, state);
    
    // 4. Verify migration
    const verified = await verifyMigration(oldAddress, newAddress);
    if (!verified) {
        throw new Error('Migration verification failed');
    }
    
    // 5. Update references
    await updateReferences(oldAddress, newAddress);
    
    // 6. Deprecate old contract
    await deprecateContract(oldAddress);
    
    console.log('Migration complete');
}
```

## Deployment Automation

### CI/CD Pipeline

```yaml
# .github/workflows/deploy.yml
name: Deploy Contract

on:
  push:
    tags:
      - 'v*'

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build Contract
        run: |
          mkdir build && cd build
          cmake .. -DCMAKE_BUILD_TYPE=Release
          make
      
      - name: Run Tests
        run: make test
      
      - name: Security Scan
        run: |
          # Run security tools
          slither .
          mythril analyze contract.wasm
  
  deploy-testnet:
    needs: test
    runs-on: ubuntu-latest
    if: github.ref == 'refs/tags/v*-rc*'
    steps:
      - name: Deploy to Testnet
        env:
          TESTNET_KEY: ${{ secrets.TESTNET_KEY }}
        run: |
          node scripts/deploy-testnet.js
  
  deploy-mainnet:
    needs: test
    runs-on: ubuntu-latest
    if: github.ref == 'refs/tags/v*' && !contains(github.ref, '-rc')
    environment: production
    steps:
      - name: Deploy to Mainnet
        env:
          MAINNET_KEY: ${{ secrets.MAINNET_KEY }}
        run: |
          node scripts/deploy-mainnet.js
```

## Monitoring and Verification

### Health Check Script

```javascript
// monitor.js
class ContractMonitor {
    constructor(contractId, provider) {
        this.contractId = contractId;
        this.provider = provider;
        this.metrics = {
            callCount: 0,
            errorCount: 0,
            gasUsed: 0,
            lastCheck: Date.now()
        };
    }
    
    async checkHealth() {
        const checks = {
            responsive: await this.checkResponsiveness(),
            stateValid: await this.checkStateValidity(),
            eventsWorking: await this.checkEvents(),
            gasEfficient: await this.checkGasUsage()
        };
        
        const healthy = Object.values(checks).every(v => v);
        
        if (!healthy) {
            await this.alertTeam(checks);
        }
        
        return checks;
    }
    
    async checkResponsiveness() {
        try {
            const start = Date.now();
            await this.contract.functions.ping();
            const responseTime = Date.now() - start;
            
            return responseTime < 1000; // Under 1 second
        } catch {
            return false;
        }
    }
    
    async checkStateValidity() {
        // Implement state validation logic
        const totalSupply = await this.contract.functions.total_supply();
        const sumOfBalances = await this.calculateSumOfBalances();
        
        return totalSupply.value === sumOfBalances;
    }
    
    async checkEvents() {
        // Subscribe to events and verify emission
        return new Promise(resolve => {
            const timeout = setTimeout(() => resolve(false), 5000);
            
            this.contract.events.subscribe('test_event', () => {
                clearTimeout(timeout);
                resolve(true);
            });
            
            // Trigger test event
            this.contract.functions.emit_test_event();
        });
    }
    
    async checkGasUsage() {
        // Monitor gas consumption trends
        const recentTransactions = await this.getRecentTransactions();
        const avgGas = recentTransactions.reduce((sum, tx) => 
            sum + tx.rcUsed, 0) / recentTransactions.length;
        
        return avgGas < 1000000; // Under threshold
    }
    
    async alertTeam(checks) {
        // Send alerts via multiple channels
        console.error('Contract health check failed:', checks);
        
        // Email alert
        await sendEmail({
            to: 'team@example.com',
            subject: `Contract ${this.contractId} Health Alert`,
            body: JSON.stringify(checks, null, 2)
        });
        
        // Slack notification
        await sendSlackMessage({
            channel: '#alerts',
            text: `⚠️ Contract health check failed for ${this.contractId}`
        });
    }
}
```

## Rollback Procedures

### Emergency Rollback

```javascript
// rollback.js
class EmergencyRollback {
    async execute(contractId, reason) {
        console.log('EMERGENCY ROLLBACK INITIATED');
        console.log('Contract:', contractId);
        console.log('Reason:', reason);
        
        // 1. Pause contract immediately
        await this.pauseContract(contractId);
        
        // 2. Notify users
        await this.notifyUsers(contractId, reason);
        
        // 3. Snapshot current state
        const snapshot = await this.snapshotState(contractId);
        
        // 4. Deploy previous version
        const previousVersion = await this.getPreviousVersion(contractId);
        const newContract = await this.deployContract(previousVersion);
        
        // 5. Restore state
        await this.restoreState(newContract.id, snapshot);
        
        // 6. Update references
        await this.updateAllReferences(contractId, newContract.id);
        
        // 7. Verify rollback
        const success = await this.verifyRollback(
            contractId, 
            newContract.id
        );
        
        if (success) {
            console.log('Rollback successful');
            await this.notifyUsers(newContract.id, 'Service restored');
        } else {
            console.error('Rollback failed - manual intervention required');
            await this.escalateToTeam();
        }
        
        return success;
    }
}
```

## Best Practices

### 1. Deployment Checklist Template

```markdown
## Deployment Checklist - [Contract Name]

### Pre-Deployment
- [ ] Code freeze implemented
- [ ] All tests passing
- [ ] Security audit complete
- [ ] Documentation updated
- [ ] ABI version bumped
- [ ] Migration plan ready

### Deployment
- [ ] Backup current state
- [ ] Deploy to staging
- [ ] Staging tests passed
- [ ] Deploy to production
- [ ] Verify deployment
- [ ] Update DNS/ENS

### Post-Deployment
- [ ] Monitor for 24 hours
- [ ] Performance metrics normal
- [ ] No critical errors
- [ ] User communications sent
- [ ] Documentation published
- [ ] Team retrospective

### Rollback (if needed)
- [ ] Issue identified
- [ ] Rollback decision made
- [ ] Rollback executed
- [ ] Service restored
- [ ] Post-mortem scheduled
```

### 2. Security Considerations

- Never expose private keys in code
- Use hardware wallets for mainnet
- Implement time-locks for critical operations
- Require multi-sig for admin functions
- Monitor for unusual activity
- Have incident response plan ready

### 3. Gas Optimization for Deployment

- Minimize contract size
- Remove debug code
- Use optimal data structures
- Batch initial setup operations
- Consider deployment in stages

## Troubleshooting

### Common Deployment Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| "Insufficient RC" | Not enough resources | Increase RC limit or optimize contract |
| "Contract too large" | Bytecode exceeds limit | Optimize and minimize code |
| "Invalid signature" | Wrong key or network | Verify credentials and network |
| "Duplicate contract" | Address already used | Generate new unique address |
| "State migration failed" | Incompatible schemas | Implement migration adapter |

## Post-Deployment

### Verification Steps

1. Contract responds to calls
2. State initialized correctly
3. Events emitting properly
4. Permissions set correctly
5. Integration tests passing
6. Monitoring active
7. Documentation published
8. Users notified