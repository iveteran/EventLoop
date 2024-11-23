# Connect to a remote server and show certificate
#openssl s_client -connect example.com:443 -showcerts

# For DTLS specifically
openssl s_client -dtls1_2 -connect localhost:10000 -showcerts
