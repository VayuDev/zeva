$(function() {
    $.getJSON("/api/scripts/all", "", function(data) {
        let result = data["result"];
        for(let i = 0; i < result.length; ++i) {
            $("#scriptList").append(result[i] + "<br>");
        }
    })
})