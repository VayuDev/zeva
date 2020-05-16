class ScriptModule is Script {
    construct new() {
        System.print("Constructed!")
        var db = Database.new("iifiles", "postgres", "postgres", "127.0.0.1", 5432)
        var res = db.query("SELECT * FROM ii_sample")
        System.print(res)
    }

    onRunOnce(b) {
    }
}