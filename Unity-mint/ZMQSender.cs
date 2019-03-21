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
// IJsonStringConvertible interface and are attached to the GameObject of this Script.
public class ZMQSender : MonoBehaviour {

    public string m_address = "tcp://localhost:12345"; // user may set address string where to connect via ZMQ

    private PublisherSocket m_socket; // ZMQ Socket
    private List<(IJsonStringConvertible,string)> m_convsAndNames; // all Convertible scripts attached to objects in the scene which give us json data

	void Start () {
        AsyncIO.ForceDotNet.Force();
        if(m_socket == null)
        {
            m_socket = new PublisherSocket();
            m_socket.Options.SendHighWatermark = 1000;
            m_socket.Bind(m_address);
        }
        
        // get all IJsonStringConvertible scripts which are attached to objects in the scene
        m_convsAndNames = GameObject.FindObjectsOfType<GameObject>() // gelt all GameObjects in scene
            .Where(o => o.GetComponents<IJsonStringConvertible>().Length > 0) // take only objects which have IJsonStringConvertible component
            .Select(o => // for each object, make tuple (IJsonStringConvertible[] convertibles, string objectName)
                (o.GetComponents<IJsonStringConvertible>(), // take IJsonStringConvertible[] from all IJsonStringConvertible in the components
                Enumerable.Repeat(o.name, o.GetComponents<IJsonStringConvertible>().Length).ToList()) ) // list with name of object as entries
            .Select(o => o.Item1.Zip(o.Item2, (conv, name) => (conv, name)).ToList()) // merge the two seperate Convertible and name arrays to get one array of (IJsonStringConvertible, fatherObjectName)[]
            .Aggregate(new List<(IJsonStringConvertible,string)>(), (current, item) =>  current.Concat(item).ToList()); // merge the multiple arrays into one (IJsonStringConvertible,string)[]
        // => in the end, we collected into an array all IJsonStringConvertible scripts in the scene with the names of the objects they are attached to

        foreach(var j in m_convsAndNames)
        {
            string convName = j.Item1.nameString();
            string objectName = j.Item2;
            Debug.Log("ZMQSender found Convertible '"+ convName +"' in Object '" + objectName + "'");
        }
        Debug.Log("ZMQSender has " + m_convsAndNames.Count + " convertibles");
    }

	// called once per frame, after all Updates are done
	void LateUpdate () {
        if(m_convsAndNames != null)
        {
            foreach(var j in m_convsAndNames)
            {
                var conv = j.Item1;
                string name = conv.nameString();
                string json = conv.jsonString();

                Debug.Log("'" + j.Item2 + "' sends '" + name + "': " + json);
                m_socket.SendMoreFrame(name).SendFrame(json);
            }
        }
	}

    private void OnApplicationQuit()
    {
        if(m_socket != null)
        {
            m_socket.Dispose();
            NetMQConfig.Cleanup(false);
        }
    }
}

} // namespace
