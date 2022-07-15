#! /bin/bash

set -e

#contract
if [[ "$1" == "arbitration" ]]; then
    contract=arbitration
else
    echo "need contract"
    exit 1
fi


echo ">>> Copying $contract files to testing folder to add required hydra features"
cp -r contracts/$contract/. ./testing/contracts_source/$contract

cd testing

echo ">>> Adding hydra.pp $contract.hpp"
sed -i '1i #include "../../../tests/hydra.hpp"\' ./contracts_source/$contract/include/$contract.hpp

echo ">>> Removing singleton tables from $contract.hpp"
sed -i '/TABLE config/,/config\_singleton/d' ./contracts_source/$contract/include/$contract.hpp

echo ">>> Adding Hydra-helper to $contract.hpp"
line=$(grep -n '};' ./contracts_source/$contract/include/$contract.hpp | tail -1 | cut -d : -f 1)
sed -i "$(($line-1)) r ./hydra-helper.cpp"  ./contracts_source/$contract/include/$contract.hpp


echo ">>> Building $contract with Hydra required features"
cd contracts_source/$contract
eosio-cpp -I include src/$contract.cpp && wait
cd ../../


echo ">>> Removing $contract files from testing folder"
rm -r ./contracts_source/$contract/include
rm -r ./contracts_source/$contract/src

echo ">>> Installing necessary packages"
npm i 

echo ">>> Starting tests for $contract"
npm test 

cd ..

