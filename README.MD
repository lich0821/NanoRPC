# A Toy RPC with NamoMSG and NanoPB
References:
* [NanoMsgDemo](https://github.com/lich0821/NanoMsgDemo)
* [NanopbDemo](https://github.com/lich0821/NanopbDemo)

## Python client
```sh
protoc.exe --python_out=. --proto_path=..\Common\proto\ wcferry.proto
```

```sh
python client.py
```
