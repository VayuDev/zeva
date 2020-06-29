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

function timeoutChange() {
    $.ajax({
        type: "POST",
        url: "/api/scripts/setTimeout",
        data: {
            "scriptid": SCRIPTID,
            "timeout": $("#timeout").val()
        },
        error: function(err, textStatus, errorThrown) {
            notifyError(err.responseText);
        },
        success: function(data, status, jqXHR) {
            notify("Changed timeout");
        }
    })
}
$(function() {
    $.getJSON("/api/scripts/get", "scriptid=" + SCRIPTID, function(json) {
        $("#editor").text(json["code"])
        $("#timeout").val(json["timeout"]);
        editor = ace.edit("editor");
        editor.session.setMode("ace/mode/lua");
        editor.commands.addCommand({
            name: 'save',
            bindKey: {win: "Ctrl-S", "mac": "Cmd-S"},
            exec: function(editor) {
                saveScript();
            }
        });
    });
})