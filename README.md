### How to build

```bash
g++ -O3 spike-client.cpp -Lprecompiled -lopen62541 -o spike-client && g++ -O3 spike-server.cpp -Lprecompiled -lopen62541 -o spike-server
```

### How to test results

```bash
parallel ::: ./spike-client ./spike-client ./spike-client ./spike-client ./spike-client ./spike-client ./spike-client ./spike-client ./spike-client ./spike-client | rg TS
```
