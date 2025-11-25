#!/usr/bin/env bash

set -euo pipefail

PORT=8545
FLAG=$(cat /home/foundry/flag.txt)

start_instance() {
    # Start geth if not running
    if ! pgrep -x "geth" >/dev/null; then
        geth --dev --http --http.addr 0.0.0.0 --http.port $PORT --http.api eth,net,web3 --dev.gaslimit 10000000000000 --datadir /home/foundry/.ethereum &>/dev/null &
        sleep 2

        # Retrieve pre-funded account info
        PRE_FUNDED_ACCOUNT_PRIVATE_KEY=$(cast wallet decrypt-keystore /home/foundry/.ethereum/keystore/* --unsafe-password "" | awk -F 'private key is: ' '{print $2}')

        # Create and fund player account
        cast wallet new --json >player_account.json
        PLAYER_ADDRESS=$(jq -r '.[0].address' player_account.json)
        cast send --rpc-url "http://127.0.0.1:$PORT" --private-key "$PRE_FUNDED_ACCOUNT_PRIVATE_KEY" --value 1ether "$PLAYER_ADDRESS" --async >/dev/null

        # Deploy contract
        forge create src/Setup.sol:Setup \
            --rpc-url "http://127.0.0.1:$PORT" \
            --private-key "$PRE_FUNDED_ACCOUNT_PRIVATE_KEY" \
            --value 100ether \
            --broadcast \
            --json >deploy.json
    fi

    # Get private key and address
    PRIVATE_KEY=$(jq -r '.[0].private_key' player_account.json)
    ADDRESS=$(jq -r '.[0].address' player_account.json)

    # Retrieve deployed contract addresses
    SETUP=$(jq -r '.deployedTo' deploy.json)
    TARGET=$(cast call "$SETUP" "TARGET()(address)" --rpc-url "http://127.0.0.1:$PORT")

    export PRIVATE_KEY ADDRESS SETUP TARGET
}

print_info() {
    cat <<EOF

    Private key     :  $PRIVATE_KEY
    Address         :  $ADDRESS
    Setup contract  :  $SETUP
    Target contract :  $TARGET
EOF
}

start_instance
print_info

echo
echo "1 - Connection information"
echo "2 - Restart Instance"
echo "3 - Get flag"
echo -n "action? "

while IFS= read -r action; do
    case "$action" in
    1)
        print_info
        ;;
    2)
        echo
        echo "Restarting instance..."
        pkill -f geth || true
        rm -rf /home/foundry/.ethereum
        sleep 2
        start_instance
        echo "Instance restarted."
        print_info
        ;;
    3)
        SOLVED=$(cast call "$SETUP" "isSolved()(bool)" --rpc-url "http://127.0.0.1:$PORT")
        if [ "$SOLVED" = "true" ]; then
            echo
            echo "Flag: $FLAG"
        else
            echo
            echo "Challenge not solved yet."
        fi
        ;;
    *)
        echo
        echo "Invalid option."
        ;;
    esac
    echo
    echo "1 - Connection information"
    echo "2 - Restart Instance"
    echo "3 - Get flag"
    echo -n "action? "
done
