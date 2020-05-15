class ScriptModule is Script {
    construct new() {
        _a = 3
        System.print("Constructed!")

        /*var db = Database.new("localhost", 5120)
        var ret = db.query("SELECT * FROM scripts")
        if(ret[0] == "Success!") {
            for(row in ret[1]) {
                System.print(row)
            }
        } else {
            System.print(ret)
        }*/
    }

    onRunOnce(b) {
        _a = b + _a
        System.print("Running once: %(_a)")
        return "Hallo %(_a)"
    }
}