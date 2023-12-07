import time
import zmq # pip install pyzmq
import subprocess
import os
import glob

gpuname = "TitanRTX"

render_fps = ["0", "10", "20", "50", "100"]

unity_path = "C:\\KolabBW\\MWK-VRAUKE-Benchmark\\"
unity_exe = "BaseSpoutInterop.exe"
rendering_path = "C:\\KolabBW\\mint-rendering\\"
rendering_exe = "rendering.exe"

topic_string = "mintclose"

protocols = "--zmq=tcp --spout=gpu"

context = zmq.Context()
socket = context.socket(zmq.SUB)
socket.setsockopt_string(zmq.SUBSCRIBE, topic_string)
socket.connect("tcp://localhost:12345")

for r in render_fps:
    print("BENCHMARK: {}ms rendering ".format(r))
    time.sleep(5.0)
    subprocess.Popen(rendering_path + rendering_exe + " --render-ms={} {}".format(r, protocols), shell=True)
    time.sleep(5.0)
    subprocess.Popen(unity_path + unity_exe, shell=True)
    while(True):
        time.sleep(0.005)
        tpc, msg = socket.recv_multipart()

        #print("tpc={}; msg={}".format(tpc, msg))
        if(tpc.decode() == topic_string and msg.decode().find("1") != -1):
            oldname = glob.glob(rendering_path + "unity_steering_tcp+gpu_2023*.txt")[0]
            newname = rendering_path + "unity_steering_{}_tcp+gpu_{}ms.txt".format(gpuname, r)
            if(os.path.isfile(oldname)):
                if(os.path.isfile(newname)):
                    os.remove(newname)
                print("renaming file: " + oldname + " -> " + newname)
                os.rename(oldname, newname)
            else:
                print("ERROR RENAMING UNITY STEERING OUTPUT FILE")
            
            has_one = True
            while(has_one):
                try:
                    time.sleep(0.050)
                    tpc, msg = socket.recv_multipart(flags=zmq.NOBLOCK)
                    has_one = tpc.decode() == topic_string and msg.decode().find("1") != -1
                except:
                    has_one = False
            break

socket.close()
context.destroy()

print("DONE")