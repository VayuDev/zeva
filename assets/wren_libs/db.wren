class QueryResult {
    construct new(query, columns, data) {
        _status = "ok"
        _columns = columns
        _data = data
        _query = query
    }
    status { _status }
    hasData { _data == null }
    data { _data }
    columns { _columns }
    query { _query }
    toString {
        return "QueryResult{ status=%(status) }"
    }
}

foreign class Database {
    foreign construct new()
    foreign construct new(dbname, username, password, hostname, port)
    foreign queryInternal(request)
    query(request) {
        var rawResult = queryInternal(request)
        var status = rawResult[0]
        var data = rawResult[1]
        var columns = rawResult[2]
        return QueryResult.new(request, columns, data)
    }
}