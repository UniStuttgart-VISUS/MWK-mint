using System;
using System.Threading;
using UnityEngine;

using NetMQ;


namespace interop
{

// NetMQ needs to be cleaned up after ZMQ Sockets got closed.
// Since NetMQ is basically a singleton for the whole Unity process,
// we keep track of all ZMQ Receiver/Sender objects and shut down NetMQ
// after all ZMQ Sockets signaled their own shutdown.
public class NetMQShutdown : MonoBehaviour {

    public void OnApplicationQuit() {
        Debug.Log("NetMQConfig Cleanup");
        NetMQConfig.Cleanup(false);
    }
}


} // namespace
