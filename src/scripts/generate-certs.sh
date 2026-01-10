#!/bin/bash
mkdir -p certs
openssl req -x509 -newkey rsa:2048 \
   -keyout certs/server.key \
   -out certs/server.crt \
   -days 365 -nodes \
   -subj "/CN=10.100.94.45"
echo "Certificados generados en ./certs/"