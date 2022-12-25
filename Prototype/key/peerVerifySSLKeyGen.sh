#!/bin/bash
if [ -d sslKeys ]
then
    rm -rf sslKeys
fi

mkdir sslKeys
cd sslKeys

openssl req -newkey rsa:2048 -passout pass:123456 -keyout ca_rsa_private.pem -x509 -days 365 -out ca.crt -subj "/C=CN/ST=GD/L=SZ/O=COM/OU=NSP/CN=CA/emailAddress=youremail@qq.com"

openssl req -newkey rsa:2048 -passout pass:server -keyout server_rsa_private.pem  -out server.csr -subj "/C=CN/ST=GD/L=SZ/O=COM/OU=NSP/CN=SERVER/emailAddress=youremail@qq.com"

openssl x509 -req -days 365 -in server.csr -CA ca.crt -CAkey ca_rsa_private.pem -passin pass:123456 -CAcreateserial -out server.crt

openssl rsa -in server_rsa_private.pem -out server_rsa_private.pem

openssl req -newkey rsa:2048 -passout pass:client -keyout client_rsa_private.pem -out client.csr -subj "/C=CN/ST=GD/L=SZ/O=COM/OU=NSP/CN=CLIENT/emailAddress=youremail@qq.com"

openssl x509 -req -days 365 -in client.csr -CA ca.crt -CAkey ca_rsa_private.pem -passin pass:123456 -CAcreateserial -out client.crt

openssl rsa -in client_rsa_private.pem -out client_rsa_private.pem

cd ..