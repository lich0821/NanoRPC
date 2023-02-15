import time
import pynng
import signal
from datetime import datetime
from wcferry_pb2 import Request, Response


def handler(sig, frame):
    exit(0)


if __name__ == "__main__":
    signal.signal(signal.SIGINT, handler)
    url = "tcp://127.0.0.1:5555"
    with pynng.Pair1() as sock:
        sock.dial(url)
        while True:
            req = Request()
            req.func = 0x01  # FUNC_IS_LOGIN
            data = req.SerializeToString()
            print(f"{datetime.now()}: {data}")
            sock.send(data)
            rsp = Response()
            rsp.ParseFromString(sock.recv_msg().bytes)
            print(f"{rsp}")

            req.func = 0x11  # FUNC_GET_MSG_TYPES
            data = req.SerializeToString()
            print(f"{datetime.now()}: {data}")
            sock.send(data)
            rsp = Response()
            rsp.ParseFromString(sock.recv_msg().bytes)
            print(f"{rsp}")

            req.func = 0x20  # FUNC_SEND_TXT
            req.txt.msg = "This is message"
            req.txt.receiver = "Chuck"
            req.txt.aters = "@all"
            data = req.SerializeToString()
            print(f"{datetime.now()}: {data}")
            sock.send(data)
            rsp = Response()
            rsp.ParseFromString(sock.recv_msg().bytes)
            print(f"{rsp}")

            time.sleep(1)
