import "config" for Config
import "json" for JSON

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

class NetworkUtil {
    static urlEncode(str) {
        if(__conversions == null) {
            __conversions = Map.new()
            __conversions["!"] = "\%21"
            __conversions["#"] = "\%23"
            __conversions["$"] = "\%24"
            __conversions["\%"] = "\%25"
            __conversions["&"] = "\%26"
            __conversions["'"] = "\%27"
            __conversions["("] = "\%28"
            __conversions[")"] = "\%29"
            __conversions["*"] = "\%2A"
            __conversions["+"] = "\%2B"
            __conversions[","] = "\%2C"
            __conversions["/"] = "\%2F"
            __conversions[":"] = "\%3A"
            __conversions[";"] = "\%3B"
            __conversions["="] = "\%3D"
            __conversions["?"] = "\%3F"
            __conversions["@"] = "\%40"
            __conversions["["] = "\%5B"
            __conversions["]"] = "\%5D"
            __conversions[" "] = "\%20"
            __conversions["\""] = "\%22"
            __conversions["\%"] = "\%25"
            __conversions["-"] = "\%2D"
            __conversions["."] = "\%2E"
            __conversions["<"] = "\%3C"
            __conversions[">"] = "\%3E"
            __conversions["\\"] = "\%5C"
            __conversions["^"] = "\%5E"
            __conversions["_"] = "\%5F"
            __conversions["<"] = "\%3C"
        }

        var res = ""
        for(c in str) {
            if(__conversions[c] != null) {

                c = __conversions[c]
            }
            res = res + c
        }
        return res
    }
}

class NotificationClient {
    static notify(type, msg, priority) {
        if(type != "gotify") {
            Fiber.abort("Unknown type: %(type)")
        }
        var config = Config.get()["notifications"]["gotify"]
        var host = config["host"]
        var port = config["port"]
        var token = config["token"]
        msg = NetworkUtil.urlEncode(msg)
        return HttpClient.send(host, port, "/message?token=%(token)&message=%(msg)&priority=%(priority)", "POST", "")
    }
}