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
    status { _type }
    type { _type }
    body { _body }
    toString {
        return "HttpResponse{ type=%(_type), body=%(_body) }"
    }
}

class HttpClient {
    foreign static sendInternal(hostname, port, url, type, params)
    static send(hostname, port, url, type, params) {
        var resp = HttpClient.sendInternal(hostname, port, url, type, params)
        return HttpResponse.new(resp[0], resp[1])
    }
}

/*
class NotificationClient {
    construct new(type) {
        if(type != "gotify") {
            throw "Unknown type: %(type)"
        }
        var config = Config.get()["notification"]["gotify"]
        _connection = HttpClient.new(config["hostname"], config["port"])
    }
}*/