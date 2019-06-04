using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using NetMQ;
using NetMQ.Sockets;
using System; // TimeSpan
using System.Linq; // functional ops

using System.Collections.Concurrent;
using System.ComponentModel;

namespace interop
{

public class ZMQReceiver : MonoBehaviour {

    public string m_address = "tcp://localhost:12346"; // user may set address string where to connect via ZMQ
    public bool m_logging = false;

    private void log(string msg)
    {
        if(m_logging)
            Debug.Log("ZMQReceiver: " + msg);
    }

    private SubscriberSocket m_socket = null; // ZMQ Socket

    private ConcurrentDictionary<string, string> m_receivedValues = new ConcurrentDictionary<string, string>(); // address name -> received value
    private List<(IJsonStringReceivable,string)> m_recvsAndNames = new List<(IJsonStringReceivable,string)>(); // all Convertible scripts attached to objects in the scene which give us json data

    private static BackgroundWorker m_workerThread = new BackgroundWorker();

	void Start () {
        // get all IJsonStringReceivable scripts which are attached to objects in the scene
        m_recvsAndNames = GameObject.FindObjectsOfType<GameObject>() // gelt all GameObjects in scene
            .Where(o => o.GetComponents<IJsonStringReceivable>().Length > 0) // take only objects which have IJsonStringReceivable component
            .Select(o => // for each object, make tuple (IJsonStringReceivable[] convertibles, string objectName)
                (o.GetComponents<IJsonStringReceivable>(), // take IJsonStringReceivable[] from all IJsonStringReceivable in the components
                Enumerable.Repeat(o.name, o.GetComponents<IJsonStringReceivable>().Length).ToList()) ) // list with name of object as entries
            .Select(o => o.Item1.Zip(o.Item2, (conv, name) => (conv, name)).ToList()) // merge the two seperate Convertible and name arrays to get one array of (IJsonStringReceivable, fatherObjectName)[]
            .Aggregate(new List<(IJsonStringReceivable,string)>(), (current, item) =>  current.Concat(item).ToList()); // merge the multiple arrays into one (IJsonStringReceivable,string)[]
        // => in the end, we collected into an array all IJsonStringReceivable scripts in the scene with the names of the objects they are attached to

        foreach(var j in m_recvsAndNames)
        {
            string convName = j.Item1.nameString();
            string objectName = j.Item2;
            Debug.Log("ZMQReceiver found Receivable '"+ convName +"' in Object '" + objectName + "'");
        }
        Debug.Log("ZMQReceiver has " + m_recvsAndNames.Count + " receivables");

        start();
    }

    private void receiveMessagesAsync(object sender, DoWorkEventArgs e)
    {
        Debug.Log("ZMQReceiver starting async worker");

        // need to create ZMQ socket in thread that is going to use it
        if(m_socket == null)
        {
            AsyncIO.ForceDotNet.Force();
            m_socket = new SubscriberSocket();
            m_socket.Connect(m_address);

            foreach(var i in m_recvsAndNames)
                m_socket.Subscribe(i.Item1.nameString());
            Debug.Log("ZMQReceiver socket init ok");
        }

        const int messagePartsCount = 2;
        while(!m_workerThread.CancellationPending)
        {
            NetMQMessage multipartMessage = new NetMQMessage(messagePartsCount);
            var hasMessage = m_socket.TryReceiveMultipartMessage(TimeSpan.FromMilliseconds(100), ref multipartMessage, messagePartsCount);

            if (multipartMessage.IsEmpty)
            {
                log("ZMQReceiver: failed to receive message");
                continue;
            }

            string address = multipartMessage[0].ConvertToString();
            string message = multipartMessage[1].ConvertToString();

            m_receivedValues.AddOrUpdate(address, message, (key, oldValue) => message);
            log("ZMQReceiver: " + address + " / " + message);
        }

        m_socket.Dispose();
        NetMQConfig.Cleanup(false);
        e.Cancel = true;
        m_socket = null;
        Debug.Log("ZMQReceiver: async thread finished");
    }

    public string getValue(string name)
    {
        string value = null;

        if(m_receivedValues.TryGetValue(name, out value))
            return value;

        return null;
    }

    void start()
    {
        if(!m_workerThread.IsBusy)
        {
            m_workerThread.DoWork += receiveMessagesAsync;
            m_workerThread.WorkerSupportsCancellation = true;
            m_workerThread.RunWorkerAsync();
        }
    }

    void stop()
    {
        if(m_workerThread.IsBusy)
        {
            m_workerThread.CancelAsync();
        }
    }

	//void PreUpdate () { // since Unity 2019.1.1
	void Update () {
        foreach(var j in m_recvsAndNames)
        {
            var receiver = j.Item1;
            string value;

            if(m_receivedValues.TryGetValue(receiver.nameString(), out value))
            {
                receiver.setJsonString(value);
            }
        }
	}

    private void OnDisable()
    {
        stop();
        System.Threading.Thread.Sleep(1000);
        Debug.Log("ZMQReceiver: stopped");
    }
}

} // namespace
