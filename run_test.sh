#!/bin/bash

set -e

PAM_CONFIG_PATH=/etc/pam.d
PAM_CONFIG_NAME=id_test
PAM_MODULE_PATH=/lib/x86_64-linux-gnu/security
PAM_MODULE_NAME=pam_bacchus.so

sudo cp "./${PAM_MODULE_NAME}" "${PAM_MODULE_PATH}/${PAM_MODULE_NAME}"

sudo mkdir -p ${PAM_CONFIG_PATH}
echo "auth required pam_bacchus.so url=http://localhost:10101/" | sudo tee ${PAM_CONFIG_PATH}/${PAM_CONFIG_NAME}

python3 ./test/test_server.py &
SERVER_PID=$!
sleep 3s

echo "Running test for valid user..."
echo "doge" | ./test/id_test "test" "true"

# echo "Running test for invalid user..."
echo "not_doge" | ./test/id_test "test" "false"

kill ${SERVER_PID}
