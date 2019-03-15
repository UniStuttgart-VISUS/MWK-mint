using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using NetMQ;
using NetMQ.Sockets;

namespace interop
{

// Opens a ZMQ socket and sends Json data on each LateUpdate() call.
// The Json data comes from Script Objects which implement the
// IJsonStringConvertible interface and are attached to the GameObject of this Script.
public class ZMQSender : MonoBehaviour {

    public string m_address = "tcp://localhost:12345"; // user may set address string where to connect via ZMQ

    private PublisherSocket m_socket; // ZMQ Socket
    private IJsonStringConvertible[] jsons; // scripts attached to this object which may give us json data

	void Start () {
        AsyncIO.ForceDotNet.Force();
        if(m_socket == null)
        {
            m_socket = new PublisherSocket();
            m_socket.Options.SendHighWatermark = 1000;
            m_socket.Bind(m_address);
        }

        jsons = gameObject.GetComponents<IJsonStringConvertible>();
    }

	// called once per frame, after all Updates are done
	void LateUpdate () {
        if(jsons != null)
        {
            foreach(IJsonStringConvertible j in jsons)
            {
                string name = j.nameString();
                string json = j.jsonString();

                Debug.Log(name + " JsonString: " + json);
                m_socket.SendMoreFrame(name).SendFrame(json);
            }
        }
	}

    private void OnApplicationQuit()
    {
        m_socket.Dispose();
        NetMQConfig.Cleanup(false);
    }
}

} // namespace
