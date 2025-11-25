#!/bin/bash

HOST='object Host "host@@ID@@" {
          import "dummy-template"
          address = "8.8.8.8"
          groups = ["tenant2"]
      }'

for i in $(seq 1 10); do
    echo "${HOST//@@ID@@/$i}" >> /etc/icinga2/conf.d/tenant_hosts.conf
done

HOST='object Host "myHost@@ID@@" {
          import "dummy-template"
          address = "8.8.8.8"
          groups = ["tenant1"]
      }'

for i in $(seq 1 10); do
    echo "${HOST//@@ID@@/$i}" >> /etc/icinga2/conf.d/tenant_hosts.conf
done
