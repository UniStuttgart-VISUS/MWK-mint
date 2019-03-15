using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using NetMQ;
using NetMQ.Sockets;

namespace interop
{
    // When a class implements this interface,
    // we can ask it for a Json representation of some kind of data.
    // The ZMQ Sender uses this to send arbitrary data.
    // TODO: we may use byte[] instead of string, for a more general interface.
    public interface IJsonStringConvertible {
            string nameString();
            string jsonString();
    }
}
