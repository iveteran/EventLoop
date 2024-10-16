host="localhost"
port_4=10001
port_6=10002

echo -n hello_from_nc | nc -4u $host $port_4
#echo -n hello_from_nc | nc -6u $host $port_6
