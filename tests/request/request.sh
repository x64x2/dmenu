#!/bin/bash
curl -X POST \
     -H 'Content-Type: application/json' \
     -d '{"id": "5135958352", "jsonrpc": "2.0", "method": "query", "params": {"sql": "SELECT * FROM mappings;"}}' \
     http://127.0.0.1:50882
# -d @./request.json \
