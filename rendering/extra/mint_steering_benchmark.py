import time
import zmq # pip install pyzmq
import subprocess

zmq_modes = ["ipc", "tcp"]
spout_modes = ["gpu", "memshare"] # "cpu" mode broken in Spout, textures remain black

image_size = "1920 1080"
vive_pro_image_size = "1998 2130"

image_size = vive_pro_image_size

topic_string = "mintclose"

context = zmq.Context()
socket = context.socket(zmq.SUB)
socket.setsockopt_string(zmq.SUBSCRIBE, topic_string)
socket.connect("tcp://localhost:12349")

for z in zmq_modes:
    for s in spout_modes:
        protocols = "--zmq={} --spout={}".format(z, s)
        latencyfile = "mint_steering_{}+{}.txt".format(z, s)
        latencystartup_sec = "10"
        latencyduration_sec = "30"
        print("BENCHMARK: {} x {}".format(z, s))
        time.sleep(5.0)
        subprocess.Popen("rendering.exe --render-ms=0 {}".format(protocols), shell=True)
        time.sleep(5.0)
        subprocess.Popen("steering.exe --latency-file={} --latency-startup={} --latency-measure={} --image-size {} {}".format(latencyfile, latencystartup_sec, latencyduration_sec, image_size, protocols), shell=True)
        # wait for closing signal from steering process
        while(True):
            time.sleep(5.0)
            tpc, msg = socket.recv_multipart()
            print("tpc={}; msg={}".format(tpc, msg))
            if(tpc.decode() == topic_string and msg.decode().find("1") != -1):
                break

socket.close()
context.destroy()