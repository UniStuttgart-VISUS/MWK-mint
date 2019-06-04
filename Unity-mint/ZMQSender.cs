using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using NetMQ;
using NetMQ.Sockets;
using System.Linq;


namespace interop
{

// Opens a ZMQ socket and sends Json data on each LateUpdate() call.
// The Json data comes from Script Objects which implement the
// IJsonStringSendable interface and are attached to GameObjects.
// IJsonStringSendable script objects are found automatically in the scene.
public class ZMQSender : MonoBehaviour {

    public string m_address = "tcp://127.0.0.1:12345"; // user may set address string where to connect via ZMQ
    public bool m_logging = false;

    private PublisherSocket m_socket = null; // ZMQ Socket
    private List<(IJsonStringSendable,string)> m_sendsAndNames; // all sendable scripts attached to objects in the scene which give us json data

    private void log(string msg)
    {
        if(m_logging)
            Debug.Log("ZMQSender: " + msg);
    }

	void Start () {
        if(m_socket == null)
        {
            AsyncIO.ForceDotNet.Force();
            m_socket = new PublisherSocket();
            m_socket.Options.SendHighWatermark = 1000;
            m_socket.Bind(m_address);
        }
        
        // get all IJsonStringSendable scripts which are attached to objects in the scene
        m_sendsAndNames = GameObject.FindObjectsOfType<GameObject>() // gelt all GameObjects in scene
            .Where(o => o.GetComponents<IJsonStringSendable>().Length > 0) // take only objects which have IJsonStringSendable component
            .Select(o => // for each object, make tuple (IJsonStringSendable[] sendables, string objectName)
                (o.GetComponents<IJsonStringSendable>(), // take IJsonStringSendable[] from all IJsonStringSendable in the components
                Enumerable.Repeat(o.name, o.GetComponents<IJsonStringSendable>().Length).ToList()) ) // list with name of object as entries
            .Select(o => o.Item1.Zip(o.Item2, (send, name) => (send, name)).ToList()) // merge the two seperate sendable and name arrays to get one array of (IJsonStringSendable, fatherObjectName)[]
            .Aggregate(new List<(IJsonStringSendable,string)>(), (current, item) =>  current.Concat(item).ToList()); // merge the multiple arrays into one (IJsonStringSendable,string)[]
        // => in the end, we collected into an array all IJsonStringSendable scripts in the scene with the names of the objects they are attached to

        foreach(var j in m_sendsAndNames)
        {
            string sendName = j.Item1.nameString();
            string objectName = j.Item2;
            Debug.Log("ZMQSender: found sendable '"+ sendName +"' in Object '" + objectName + "'");
        }
        Debug.Log("ZMQSender: has " + m_sendsAndNames.Count + " sendables");
    }

	// called once per frame, after all Updates are done
	void LateUpdate () {
        if(m_sendsAndNames != null)
        {
            foreach(var j in m_sendsAndNames)
            {
                var send = j.Item1;
                string name = send.nameString();
                string json = send.jsonString();

                log("'" + j.Item2 + "' sends '" + name + "': " + json);
                m_socket.SendFrame(name, true);
                m_socket.SendFrame(json, false);
            }
        }
	}

    private void OnDisable()
    {
        if(m_socket != null)
        {
            m_socket.Dispose();
            NetMQConfig.Cleanup(false);
            m_socket = null;
            m_sendsAndNames = null;
        }
    }
}

} // namespace
