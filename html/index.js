$(function() {
    $("#header").load("header.html", "", () => {
        let pathname = window.location.pathname;
        let filename = pathname.substring(pathname.lastIndexOf("/") + 1, pathname.length);
        if(filename == "index.html") {
            $("#nav_hub").addClass("selected");
        } else if(filename == "scripts.html") {
            $("#nav_script").addClass("selected");
        } else if(filename == "db.html") {
            $("#nav_db").addClass("selected");
        }
    });
})