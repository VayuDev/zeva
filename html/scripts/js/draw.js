const params = new URLSearchParams(window.location.search);
const SCRIPTID = Number(params.get("scriptid"));

function loadImage() {
    $("#output").attr("src", "/api/scripts/draw?scriptid=" + SCRIPTID + "&seed=" + Math.random());
}

function checkCheckbox() {
    if($("#streambox").is(":checked")) {
        console.log("Streaming")
        $("#output").on("load", loadImage);
        loadImage();
    } else {
        $("#output").off("load");
    }
}

$(function() {
    loadImage();
    $("#streambox").click(checkCheckbox);
    checkCheckbox();
})