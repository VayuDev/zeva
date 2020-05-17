function logToOutputAndMsg(msg, isError) {
    let result = JSON.parse(msg)["return"];
    let output = $("#output");
    if(output.length) {
        output.append(result + "\n");
        output.animate({scrollTop: output.prop("scrollHeight")});
    }
    if(isError) {
        notifyError(result);
    } else {
        notify(result);
    }
}

function runScript(scriptid) {
    $.ajax({
        type: "POST",
        url: "/api/scripts/run",
        data: {
            "scriptid": scriptid,
        },
        error: function(err, textStatus, errorThrown) {
            logToOutputAndMsg(err.responseText, true);
        },
        success: function(data, status, jqXHR) {
            logToOutputAndMsg(jqXHR.responseText, false);
        }
    })
}