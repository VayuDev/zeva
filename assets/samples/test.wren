class ScriptModule is Script {
    construct new() {
        _a = 3
        System.print("Constructed!")
    }

    onRunOnce(b) {
        _a = b + _a
        System.print("Running once: %(_a)")
        return "Hallo %(_a)"
    }
}