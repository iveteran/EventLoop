Testing with username/password for mosquitto

git clone https://github.com/eclipse/mosquitto.git
cd mosquitto && mkidr build && cd build && cmake ..

1) $ cd /home/yuu/projects/mosquitto/build

2) $ ./src/mosquitto_passwd -c my_mosq_pwd_file my_mqtt_user   // password: 123456

3) Edit ../mosquitto.conf, add or uncomment like following:
allow_anonymous false
apassword_file /home/yuu/projects/mosquitto/my_mosq_pwd_file

4) $ ./src/mosquitto -c ../mosquitto.conf

5) sub client (with tls):
  ./client/mosquitto_sub -t "test_topic_" -d -u "my_mqtt_user" -P 123456 -p 8883
  --cafile ../ssl_certs_2/ca.crt

6) pub client (with tls):
  ./client/mosquitto_pub -t "test_topic_" -m "my_message" -u "my_mqtt_user" -P 123456
  -p 8883 --cafile ../ssl_certs_2/ca.crt
