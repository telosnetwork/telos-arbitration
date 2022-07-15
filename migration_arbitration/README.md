Script designed to automate and simplify migrations in EOSIO networks Smart Contracts.

## Contents


<!-- TOC -->
- [Contents](#contents)
- [What is it?](#what-is-it)
- [Main features](#main-features)
- [Project Structure](#project-structure)
- [Getting Started](#getting-started)
  - [Prerequisites](#prerequisites)
  - [Configuration](#configuration)
  - [Initialization](#initialization)
  - [Tips](#tips)
- [Example](#example)
- [Development](#development)
  - [Contributing Guidelines](#contributing-guidelines)
- [Acknowledgements](#acknowledgements)
<!-- /TOC -->

## What is it?

It is an script that allows EOSIO developers to migrate an Smart Contract in an easier and faster way.

EOSIO Smart Contracts, unlike Ethereum ones, can be modified and upgraded after deployment. However, if a table is modified, a migration is required so that all data stored (old and new) follows the same structure.

Currently, there is no easy way to perform those migrations and the process can became quite tedious.

## Main Features

This script provides users three main advantges:
- It automatically frozes all the Smart Contract actions, so that nobody can interact with it while the migration is being done.
- It can be easily integrated with pipelines such as Gitlab
- It doesn't modify tables names neither type, which could lead to some issues if other SC or some frontends are relying on it.


## How it works?


The script works as follows:

1.  Creates support tables for those tables that are going to be migrated.
2.  Adds a temporary migration table that controls the migration status. 
3.  Creates and executes the first migration action, which copies all the data from the selected tables to the support ones. Those rows are then deleted from the original table. Since tables may have many rows, a maximum of rows will be migrated each time the migration action is called. 
4.  Creates and executes the second migration action, which moves back all the data from the support tables to the original ones.

During all the migration process, all the Smart Contract actions will be frozen to avoid unexpected behaviours.

## Project Structure 

```
📁 migration
├─📁 contract                   - Migration source code
│ ├─📁 new_contract             - New contract source code
│ │ ├─📁 src
│ │ └─📁 include
│ ├─📁 old_contract             - Old contract source code
│ │ ├─📁 src
│ │ └─📁 include   
│ ├─📁 tablesToMigrate           - Contains JSON files containing tables to migrate
│ │ ├─ ...
│ │ └─📄 tableX.json               
│ ├─📄 first_migrate.cpp         - First migration Code
│ ├─📄 second_migrate.cpp        - Second Migration Code
│ └─📄 tablesToMigrate.js        - File containing the tables to Migrate
│
├─📁 helpers                     - Migration helpers source code (DO NOT MODIFY)
│ ├─📄 check_is_migrating.cpp    - Code added to the SC actions to freeze them during migration
│ ├─📄 createSupportTables.sh    - Script to add the support Tables to the Smart Contract  
│ └─📄 migrateTable.cpp          - Migration table code
│
├─📄 migrate.sh                  - Migration Script
├─📄 migrate-test.sh             - Migration Script without cleos fucntionality
├─📄 LICENSE
└─📄 README.md

```

## Getting Started

### Prerequisites

Please make sure you have the following installed and running properly

- [EOSIO Software](https://developers.eos.io/manuals/eos/latest/install/index)
- [cleos](https://developers.eos.io/manuals/eos/latest/cleos/index) 

### Configuration

To configure the migration script, the following steps are required: 

1. Add the original SmartContract files to the **old_contract directory**. Files need to be split in *include* and *src*.

2. Add the new upgraded SmartContract files to the **new_contract directory**. Files need to be split in *include* and *src*.

3. Add the tables that will be migrated in the **tablesToMigrate** folder. Each table to be migrated has to be added in a separate JSON file (doesn't matter the name) in the following format: 

```
{
    "tableStruct": "",
    "supportTableStruct": "",
    "tableName": "",
    "supportTableName": "",
    "tableType": "",
    "supportTableType": ""
}
``` 


5. Modify the migrate action in **first_migrate.cpp** file to copy the tables that are going to be migrated to the corresponding support tables. It is important to use the support table names described in the JSON file contained in **tablesToMigrate** folder.

6. Modify migrate action in **second_migrate.cpp** file so that the rows contained into the support tables are moved back to the original ones with the modifications applied.

### Initialization

To execute the script, you will first need to unlock the wallet where the SC keys are stored:
```bash
cleos wallet unlock
```

Once unlocked, you can run the script using the following command:
```bash
chmod +x migrate.sh && chmod +x migrate-test.sh
./migrate.sh **INSERT YOUR SC NAME**
```
### Tips

1. Migrating is always a delicate process, since data can be lost or end up in a bad state. Therefore, it is always recommended to create a copy of the Smart Contract (either in testnet or locally) and try the migration there.

2. Use **migrate-test.sh** to ensure that the migrate actions are valid. This script is the same as the main one but it doesn't have the *cleos* calls, so no changes will be made in the Blockchain.


## Example 

To a better understanding of this script, a toy Smart Contract has already been prepared so that one can play can try it.

The original SC is a simple Smart Contract to stake tokens, while the new one is an advanced version where tokens are locked for a certain period of time.

In order to execute the steps, the following steps are required: 
1. Create an account in testnet. You can use this [faucet](https://waxsweden.org/create-testnet-account)
2. Add the keys to your cleos wallet.
```bash
cleos wallet import
```
2. Build and Deploy the SC to the account you have previously created.
```bash
eosio-cpp -I include src testcontract.cpp
cleos -u https://testnet.waxsweden.org set contract account . testcontract.wasm testcontract.abi
```
3. Init the SC
```bash
cleos -u https://testnet.waxsweden.org push action testmigrate1 init '{"contract_name": "testmigrate", "initial_admin": "rogertaule12" }' -p account
```
4. Do a couple of transfers to prepopulate the Table
```bash
cleos -u https://testnet.waxsweden.org transfer account1 account "50.00000000 WAX" "test"
```
5. Run the migrate script
```bash
./migrate.sh testcontract
```


## Development

### Contributing Guidelines

All contributions, bug reports, bug fixes, documentation improvements, enhancements, and ideas are welcome.

If you find a bug, just open a issue with a tag "BUG".

If you want to request a new feature, open an issue with a tag "ENH" (for enhancement).

If you feel like that our docs could be better, please open one with a tag "DOC".


## Acknowledgements


