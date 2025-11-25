#!/bin/bash
echo "object ApiUser \"root\" { password = \"$(cat /icinga_pass)\", permissions = [ \"*\" ] }" > /etc/icinga2/conf.d/api-users.conf

ICINGA_PASS=$(cat /icinga_pass) php -S 0.0.0.0:9090 /opt/runtime.php &

icinga2 api setup
icinga2 daemon &
sleep 888888888888
