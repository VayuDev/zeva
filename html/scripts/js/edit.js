let editor;
const params = new URLSearchParams(window.location.search);
const SCRIPTID = Number(params.get("scriptid"));
function saveScript() {
    $.ajax({
        type: "POST",
        url: "/api/scripts/update",
        data: {
            "scriptid": SCRIPTID,
            "code": editor.getValue()
        },
        error: function(err, textStatus, errorThrown) {
            logToOutputAndMsg(err.responseText, true);
        },
        success: function(data, status, jqXHR) {
            notify("Successfully compiled script!");
        }
    })
}

function drawImage() {
    let img = $("<img>").attr("src", "/api/scripts/draw?scriptid=" + SCRIPTID + "&cachefix=" + Math.random());
    logImage(img);
}

function clearLog() {
    $("#output").text("");
}

$(function() {
    $.getJSON("/api/scripts/get", "scriptid=" + SCRIPTID, function(json) {
        $("#editor").text(json["code"])
        editor = ace.edit("editor");
        editor.commands.addCommand({
            name: 'save',
            bindKey: {win: "Ctrl-S", "mac": "Cmd-S"},
            exec: function(editor) {
                saveScript();
            }
        });
    });
})