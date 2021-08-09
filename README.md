### How to build

```bash
g++ -O3 spike-client.cpp -Lprecompiled -lopen62541 -o spike-client && g++ -O3 spike-server.cpp -Lprecompiled -lopen62541 -o spike-server
```

