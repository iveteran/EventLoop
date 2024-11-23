#!/bin/bash

# Generate CA private key and certificate
openssl genrsa -out ca-key.pem 2048
openssl req -new -x509 -days 3650 -config ca.conf -extensions v3_ca \
    -key ca-key.pem -out ca-cert.pem

# Generate server private key and CSR
openssl genrsa -out server-key.pem 2048
openssl req -new -config server.conf -key server-key.pem -out server.csr

# Generate server certificate
openssl x509 -req -days 365 -in server.csr -CA ca-cert.pem -CAkey ca-key.pem \
    -CAcreateserial -out server-cert.pem -extfile server.conf -extensions server

# Generate client private key and CSR
openssl genrsa -out client-key.pem 2048
openssl req -new -config client.conf -key client-key.pem -out client.csr

# Generate client certificate
openssl x509 -req -days 365 -in client.csr -CA ca-cert.pem -CAkey ca-key.pem \
    -CAcreateserial -out client-cert.pem -extfile client.conf -extensions client

# Clean up CSR files
rm *.csr

# Display certificate information
echo "CA Certificate:"
openssl x509 -in ca-cert.pem -text -noout | grep "Subject:"
echo -e "\nServer Certificate:"
openssl x509 -in server-cert.pem -text -noout | grep "Subject:"
echo -e "\nClient Certificate:"
openssl x509 -in client-cert.pem -text -noout | grep "Subject:"

# Set appropriate permissions
chmod 400 *-key.pem
chmod 444 *-cert.pem
