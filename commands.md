1. Add to env to OPC UA deployment:
```yaml
            - name: ALLOW_WITHOUT_ENCRYPTION
              value: "1"
```

2.
```sh
./deploy-generator.sh
```

3.
```sh
kafkacat -P -b localhost:30092 -t object-models -H "content-type=json" -l -T nm_demo_mappings.json
```

4.
```sh
kubectl port-forward -n wams service/test-datapoint-generator 8888:8888

curl --request POST \
--url http://localhost:8888/start \
--header 'Content-Type: application/json' \
--data '{
"no_of_datapoints": 11,
"frequency": 10,
"model": "actual",
"telemetry": "value"
}'
```

5.
```sh
kubectl port-forward -n wams service/opc-ua-server 4840:4840
```
