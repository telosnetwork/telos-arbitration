#! /bin/bash

#contract
if [[ "$1" == "arbitration" ]]; then
    contract=arbitration
else
    echo "need contract"
    exit 0
fi

#account
account=$2
#network
if [[ "$3" == "mainnet" ]]; then 
    url=https://telos.greymass.com
    network="TLOS Mainnet"
    permission="deploy"

elif [[ "$3" == "testnet" ]]; then
    url=https://testnet.telos.caleos.io
    network="TLOS Testnet"
    permission="active"
elif [[ "$3" == "local" ]]; then
    url=http://127.0.0.1:8888
    network="Local"
    permission="active"
else
    echo "need network"
    exit 0
fi

echo ">>> Deploying $contract contract to $account on $network..."

# eosio v1.8.0
cleos -u $url set contract $account ./build/ $contract.wasm $contract.abi -p $account@$permission
