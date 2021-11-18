#!/bin/sh

kubectl create namespace wams

kubectl create -f ../nm-poc/k8s/00-nmpoc-secret.yaml -n wams
kubectl create -f ../nm-poc/k8s/01-configmap-opc-ua-certificate.yaml -n wams
kubectl create -f ../nm-poc/k8s/01-configmap-opc-ua-private-key.yaml -n wams
kubectl create -f ../nm-poc/k8s/01-configmap-opc-ua-quality.yaml -n wams
kubectl create -f ../nm-poc/k8s/01-configmap-grafana-config-dashboards.yaml -n wams
kubectl create -f ../nm-poc/k8s/01-configmap-grafana-config-datasources.yaml -n wams
kubectl create -f ../nm-poc/k8s/01-configmap-grafana-dashboards.yaml -n wams
kubectl create -f ../nm-poc/k8s/01-configmap-prometheus.yaml -n wams
kubectl create -f ../nm-poc/k8s/01-configmap-victoriametrics-ingestion-storage-policies.yaml -n wams

kubectl create -f ../nm-poc/k8s/kafka.yaml -n wams

sleep 5

kubectl create -f ../nm-poc/k8s/opc-ua-server.yaml -n wams
kubectl create -f ../nm-poc/test/performance_tests/k8s/datapoint_generator.yaml -n wams
kubectl create -f ../nm-poc/k8s/metrics.yaml -n wams
kubectl create -f ../nm-poc/k8s/victoriametrics-ingestion.yaml -n wams
