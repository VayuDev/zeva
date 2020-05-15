class ScriptModule is Script {
    construct new() {
        _a = 3
        System.print("Constructed!")

        var db = Database.new("localhost", 5120)
        System.print(db.query("SELECT * FROM sample_min_csv"))
    }

    onRunOnce(b) {
        _a = b + _a
        System.print("Running once: %(_a)")
        return "Hallo %(_a)"
    }
}