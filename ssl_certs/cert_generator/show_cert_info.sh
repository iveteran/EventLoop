#!/bin/bash

function show_cert_info() {
    local cert_file=$1
    echo "==============================================="
    echo "Certificate Information for: $cert_file"
    echo "==============================================="
    
    echo -e "\n=== Basic Information ==="
    openssl x509 -in "$cert_file" -noout -subject -issuer -dates
    
    echo -e "\n=== Serial Number ==="
    openssl x509 -in "$cert_file" -noout -serial
    
    echo -e "\n=== Fingerprints ==="
    openssl x509 -in "$cert_file" -noout -fingerprint
    openssl x509 -in "$cert_file" -noout -sha256 -fingerprint
    
    #echo -e "\n=== Full Certificate Details ==="
    #openssl x509 -in "$cert_file" -text -noout
    
    echo -e "\n===============================================\n"
}

# Usage example
show_cert_info "server-cert.pem"
show_cert_info "client-cert.pem"
show_cert_info "ca-cert.pem"
