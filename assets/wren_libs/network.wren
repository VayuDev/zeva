foreign class TcpClient {
    construct new(ip, port) {}
    foreign isConnected()
    foreign sendByte(byte)
    foreign recvByte()
    foreign sendString(str)
    foreign recvString()
    foreign close()
}

class HttpResponse {
    construct new(type, body) {
        _type = type
        _body = body
    }
    construct new(error) {
        _error = error
    }
    error { _error }
    type { _type }
    body { _body }
    toString {
        if(_error == null) {
            return "HttpResponse{ type=%(_type), body=%(_body) }"
        } else {
            return "HttpResponse{ error=%(_error) }"
        }
    }
}
/*
class HttpClient {
    construct new(hostname) {
        _hostname = hostname
    }
    foreign sendInternal(hostname, url, type, body)
    send(type, url, body) {
        var resp = sendInternal(_hostname, type, url, body)
        var type = resp[0]
        if(type == "ok") {
            var body = resp[1]
            return HttpResponse.new(type, body)
        } else {
            return HttpResponse.new(type)
        }
    }
}

class NotificationClient {
    construct new(type) {
        if(type != "gotify") {
            throw "Unknown type: %(type)"
        }
        var config = Config.get()["notification"]["gotify"]
        _connection = HttpClient.new(config["hostname"], config["port"])
    }
}*/