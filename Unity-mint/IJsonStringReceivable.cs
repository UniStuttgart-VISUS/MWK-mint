using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using NetMQ;
using NetMQ.Sockets;

namespace interop
{
    public interface IJsonStringReceivable {
            string nameString();
            void setJsonString(string json);
    }
}
