const params = new URLSearchParams(window.location.search);
const TABLENAME = params.get("tablename");

$(function() {
    CsvToHtmlTable.init({
        csv_path: '/api/db/csv/' + TABLENAME,
        element: 'tablediv',
        allow_download: true,
        csv_options: {separator: ',', delimiter: '"'},
        datatables_options: {"paging": false}
    });
})
