const params = new URLSearchParams(window.location.search);
const SCRIPTID = Number(params.get("scriptid"));

$(function() {
    $("#output").attr("src", "/api/scripts/draw?scriptid=" + SCRIPTID);
})