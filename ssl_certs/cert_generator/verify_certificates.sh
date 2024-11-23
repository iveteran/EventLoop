# Verify server certificate against CA
openssl verify -CAfile ca-cert.pem server-cert.pem

# Verify client certificate against CA
openssl verify -CAfile ca-cert.pem client-cert.pem
