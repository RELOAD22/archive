openssl genrsa -out privkey.pem 2048
openssl req -new -x509 -key privkey.pem -out cacert.pem -days 1095
umask 0177
mkdir userspace/
chmod -R 700 userspace/
