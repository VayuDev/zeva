function addLogger(name) {
    let row = $("<div></div>").text(name).click(() => {
       window.location = "/html/log/view.html?logname=" + name;
    }).css("cursor", "pointer");
    $(".elementList").append(row);
}

$(function() {
    $.getJSON("/api/log/all", "", function(data) {
        for(let i = 0; i < data.length; ++i) {
            addLogger(data[i])
        }
    })
})