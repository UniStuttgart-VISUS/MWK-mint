import time
import zmq # pip install pyzmq
import subprocess

gpuname = "TitanRTX"

zmq_modes = ["ipc", "tcp"]
spout_modes = ["gpu", "memshare"] # "cpu" mode broken in Spout, textures remain black

mini_size = "100 100"
image_size = "800 600"
vive_pro_image_size = "1998 2130"
hd_size = "1280 720"
full_hd_size = "1920 1080"
full_hd_size_quad = "3840 2160"

# a bug in soput seems to break memshare mode if we resize the texture at some point
# so we pass the "correct" texture size for memshare mode to the rendering, to make it work
image_size_steering = full_hd_size
image_size_rendering = full_hd_size_quad

topic_string = "mintclose"

context = zmq.Context()
socket = context.socket(zmq.SUB)
socket.setsockopt_string(zmq.SUBSCRIBE, topic_string)
# as we test IPC and TCP for ZMQ, we rely on a separate channel for the close signal
socket.connect("tcp://localhost:12349")

for z in zmq_modes:
    for s in spout_modes:
        protocols = "--zmq={} --spout={}".format(z, s)
        latencyfile = "mint_steering_{}_{}+{}_0ms.txt".format(gpuname,z, s)
        latencystartup_sec = "10"
        latencyduration_sec = "30"
        image_size_opt_rendering = ""
        if(s == "memshare"):
            image_size_opt_rendering = " --image-size {}".format(image_size_rendering)
        print("BENCHMARK: {} x {}".format(z, s))
        time.sleep(5.0)
        subprocess.Popen("rendering.exe --texture-send=stereo --render-ms=0 {} {}".format(protocols, image_size_opt_rendering), shell=True)
        time.sleep(5.0)
        subprocess.Popen("steering.exe --latency-file={} --latency-startup={} --latency-measure={} {} --image-size {}".format(latencyfile, latencystartup_sec, latencyduration_sec, protocols, image_size_steering), shell=True)
        while(True):
            time.sleep(5.0)
            tpc, msg = socket.recv_multipart()
            print("tpc={}; msg={}".format(tpc, msg))
            if(tpc.decode() == topic_string and msg.decode().find("1") != -1):
                break

socket.close()
context.destroy()

print("DONE")