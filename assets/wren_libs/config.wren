import "json" for JSON

class Config {
    foreign static getConfigStringInternal()
    static get() {
        if(__json == null) {
            __json = JSON.parse(getConfigStringInternal())
        }
        return __json
    }
}